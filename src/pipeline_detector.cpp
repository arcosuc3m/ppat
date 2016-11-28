
void MyASTVisitor::moveDeclarations(int i, std::stringstream &declarations) {
  // Vector that stores declarations already moved
  std::vector<ASTVar> moved;
  for (unsigned dec = 0; dec < Loops[i].VarDecl.size(); dec++) {
    bool insideNested = false;
    // Check if Decl is inside a nested loop
    for (unsigned nested = 0; nested < Loops[i].InternLoops.size(); nested++) {
      if (Loops[i].VarDecl[dec].DeclLoc >=
              Loops[i].InternLoops[nested].getBegin().getRawEncoding() &&
          Loops[i].VarDecl[dec].DeclLoc <=
              Loops[i].InternLoops[nested].getEnd().getRawEncoding()) {
        insideNested = true;
      }
    }
    bool assigned = false;
    // Check if is an assignement
    for (unsigned assign = 0; assign < Loops[i].Assign.size(); assign++) {
      if (Loops[i].VarDecl[dec].Name == Loops[i].Assign[assign].Name &&
          Loops[i].VarDecl[dec].DeclLoc == Loops[i].Assign[assign].DeclLoc &&
          Loops[i].Assign[assign].RefLoc == Loops[i].VarDecl[dec].DeclLoc) {
        // llvm::errs()<<"WE HAVE TO REMOVE TYPE BEFORE VARIABLE DECLARATION TO
        // MOVE IT\n";
        assigned = true;
      }
    }
    bool iteratorDecl = false;
    // Check if is an iterator declaration
    if (Loops[i].VarDecl[dec].DeclLoc == Loops[i].Iterator.DeclLoc &&
        Loops[i].VarDecl[dec].Name == Loops[i].Iterator.Name) {
      iteratorDecl = true;
      // llvm::errs()<<"ITS THE ITERATOR DECLARATION -- DON'T MOVE IT\n";
    }
    // If is not the iterator and is not decrared in inner scopes, move this
    // declaration to the top of the scope.
    if (!insideNested && !iteratorDecl) {
      declarations << Loops[i].VarDecl[dec].type << " "
                   << Loops[i].VarDecl[dec].Name << ";\n";
      // SourceManager &SM = TheRewriter.getSourceMgr();
      ASTVar current;
      current.Name = Loops[i].VarDecl[dec].Name;
      current.RefLoc = Loops[i].VarDecl[dec].RefLoc;
      bool done = false;
      for (unsigned var = 0; var < moved.size(); var++) {
        if (moved[var].Name ==
            current.Name /*&& moved[var].RefLoc == current.RefLoc*/)
          done = true;
      }
      if (!done) {
        if (assigned) {
          auto offset = Loops[i].VarDecl[dec].type.size();
          TheRewriter.RemoveText(Loops[i].VarDecl[dec].RefSourceLoc, offset,
                                 clang::Rewriter::RewriteOptions());
        } else {
          auto offset = Loops[i].VarDecl[dec].RefEndLoc.getRawEncoding() -
                        Loops[i].VarDecl[dec].RefSourceLoc.getRawEncoding() + 1;
          TheRewriter.RemoveText(Loops[i].VarDecl[dec].RefSourceLoc,
                                 (offset + Loops[i].VarDecl[dec].Name.size()),
                                 clang::Rewriter::RewriteOptions());
        }
      }
    }
  }
}

/**     \fn bool analyzeCodeFunctions()

        Analyze every function declaration in the full code and set as L/RValue
        each variable which meets the constrains defined to be an L/RValue. This
        functions modifies the containing of the class member struct "Code".

*/

bool MyASTVisitor::analyzeCodeFunctions() {
  for (auto funs = Code.fCalls.begin(); funs != Code.fCalls.end(); funs++) {
    unsigned paramIt = 0;

    // Look for every argument
    for (auto arg = (*funs).Args.begin();
         arg != (*funs).Args.end() && (*funs).paramTypes.size() > paramIt;
         arg++) {
      // bool LRValue = false;
      // If is a modifiable argument or we know if is output or input
      if ((*funs).paramTypes[paramIt].find("&") != std::string::npos ||
          (*funs).paramTypes[paramIt].find("*") != std::string::npos ||
          (*funs).paramTypes[paramIt].find("Input") != std::string::npos ||
          (*funs).paramTypes[paramIt].find("Output") != std::string::npos) {
        // Check if is not a constant
        if ((*funs).paramTypes[paramIt].find("const") == std::string::npos) {
          // Seacrh for the varaible reference of the call parameter on the
          // variable referece list
          for (auto refs = Code.VarRef.begin(); refs != Code.VarRef.end();
               refs++) {
            if ((*arg).Name == (*refs).Name &&
                (*arg).RefLoc == (*refs).RefLoc) {
              // Set as L/R value
              (*refs).lvalue = true;
              (*refs).rvalue = true;
              // if is input type (Only for OpenCV library)
              if ((*funs).paramTypes[paramIt].find("Input") !=
                  std::string::npos) {
                // Set as Rvalue
                (*refs).lvalue = false;
                (*refs).rvalue = true;
              }
              // if is output type (Only for OpenCV library)
              if ((*funs).paramTypes[paramIt].find("Output") !=
                  std::string::npos) { // Set as Lvalue
                (*refs).lvalue = true;
                (*refs).rvalue = false;
              }
            }
          }
        }
      }
      paramIt++;
    }
  }
  return true;
}
/**
        \fn bool analyzeCall(int i)

        This function analyze every function reference in the Loop "i" which the
   tool
        has access to its declaration and body.

        \param[in]	i	Loop to be analyzed
*/
bool MyASTVisitor::analyzeCall(int i) {
  Loops[i].genStart = Loops[i].RangeLoc.getBegin();
  Loops[i].genEnd = Loops[i].RangeLoc.getBegin();

  for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
       funs++) {
    for (auto funs2 = Functions.begin(); funs2 != Functions.end(); funs2++) {
      if ((*funs).Name == (*funs2).Name) {
        analyzeFunction((*funs2), 0);
      }
    }
  }
  return true;
}

/**
        \fn bool analyzeLRLoop(int i)

        This function analyze every variable reference in the Loop "i" and set
   kind as
        LRValue if meets the constrains defined.

        \param[in]      i       Loop to be analyzed
*/

bool MyASTVisitor::analyzeLRLoop(int i) {

  // Loops[i].genStart = Loops[i].RangeLoc.getBegin();
  // Loops[i].genEnd = Loops[i].RangeLoc.getBegin();
  // Check assigns values
  for (auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end();
       var++) {
    // Check if references are on the right hand of a binary expression to solve
    // wrong types on the use of
    // multidimensional structures
    for (auto assigns = Loops[i].Assign.begin();
         assigns != Loops[i].Assign.end(); assigns++) {
      if ((*assigns).Name == (*var).Name &&
          (*assigns).RefLoc == (*var).RefLoc) {
        (*var).lvalue = true;
        (*var).rvalue = false;
      }

      for (auto depend = (*assigns).dependencies.begin();
           depend != (*assigns).dependencies.end(); depend++) {
        if ((*depend).Name == (*var).Name &&
            (*depend).DeclLoc == (*var).DeclLoc &&
            (*depend).RefLoc == (*var).RefLoc) {
          (*var).rvalue = 1;
          (*var).lvalue = 0;
        }
      }
    }
  }

  // For each function
  for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
       funs++) {
    unsigned paramIt = 0;
    // Look for every argument
    for (auto arg = (*funs).Args.begin();
         arg != (*funs).Args.end() && (*funs).paramTypes.size() > paramIt;
         arg++) {
      // bool LRValue = false;
      // If is a modifiable argument
      if ((*funs).paramTypes[paramIt].find("&") != std::string::npos ||
          (*funs).paramTypes[paramIt].find("*") != std::string::npos) {
        // If is not constant
        if ((*funs).paramTypes[paramIt].find("const") == std::string::npos) {
          //----------------------
          bool accesibleFunction = false;
          for (auto funs2 = Functions.begin(); funs2 != Functions.end();
               funs2++) {
            unsigned argumentIt = 0;
            if ((*funs2).Name == (*funs).Name) {
              for (auto arg = (*funs).Args.begin(); arg != (*funs).Args.end();
                   arg++) {
                for (auto refs = Loops[i].VarRef.begin();
                     refs != Loops[i].VarRef.end(); refs++) {
                  if ((*arg).Name == (*refs).Name &&
                      (*arg).RefLoc == (*refs).RefLoc &&
                      paramIt == argumentIt) {
                    // set as L/Rvalue
                    (*refs).lvalue = (*funs2).Arguments[argumentIt].lvalue;
                    (*refs).rvalue = (*funs2).Arguments[argumentIt].rvalue;
                    if (!(*funs2).partialAccess) {
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                    }
                  }
                }
                argumentIt++;
              }
              if ((*funs2).recursive) {
                Loops[i].hasRecursive = true;
              }
            }
          }
          // Search function on dictionary
          for (auto dic = dictionary.begin();
               dic != dictionary.end() && !accesibleFunction; dic++) {
            //	 for(auto arg =
            //(*funs).Args.begin();arg!=(*funs).Args.end();arg++){
            if ((*dic).Name == (*funs).Name) {
              for (auto refs = Loops[i].VarRef.begin();
                   refs != Loops[i].VarRef.end(); refs++) {
                if ((*arg).Name == (*refs).Name &&
                    (*arg).RefLoc == (*refs).RefLoc) {

                  if ((*dic).arguments.find(paramIt) !=
                      (*dic).arguments.end()) {
                    // Set variable kind as defined in the dictionary
                    if ((*dic).arguments[paramIt] == kind::L) {
                      (*refs).lvalue = true;
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                      (*refs).rvalue = false;
                    }
                    if ((*dic).arguments[paramIt] == kind::R) {
                      (*refs).lvalue = false;
                      accesibleFunction = true;
                      (*refs).sureKind = true;
                      (*refs).rvalue = true;
                    }
                    if ((*dic).arguments[paramIt] == kind::LR) {

                      (*refs).lvalue = true;
                      (*refs).rvalue = true;
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                    }
                    if ((*dic).arguments[paramIt] == kind::U) {
                      (*refs).lvalue = true;
                      (*refs).rvalue = true;
                    }
                  } else {
                    (*refs).lvalue = true;
                    (*refs).rvalue = true;
                  }
                }
              }
            }
          }

          // Search on the reference list and set defaul LR values for non
          // accesible function
          for (auto refs = Loops[i].VarRef.begin();
               refs != Loops[i].VarRef.end() && !accesibleFunction; refs++) {
            if ((*arg).Name == (*refs).Name &&
                (*arg).RefLoc == (*refs).RefLoc) {
              // set as L/Rvalue
              (*refs).lvalue = true;
              (*refs).rvalue = true;
              bool indic = false;
              // Check if exists an entry in the dictionary
              for (auto dic = dictionary.begin(); dic != dictionary.end();
                   dic++) {
                if ((*dic).Name == (*funs).Name) {
                  // llvm::errs()<<"UNKNOWN FUNCTION ARGUMENTS KIND\n";
                  (*dic).arguments[paramIt] = kind::U;
                  indic = true;
                }
              }
              if (!indic) {
                // If not, include the new function in the dictionary
                DictionaryFunction f;
                f.Name = (*funs).Name;
                f.arguments[paramIt] = kind::U;
                dictionaryAdds.push_back(dictionary.size());
                dictionary.push_back(f);
              }
            }
          }
        } else {

          for (auto refs = Loops[i].VarRef.begin();
               refs != Loops[i].VarRef.end(); refs++) {
            if ((*arg).Name == (*refs).Name &&
                (*arg).RefLoc == (*refs).RefLoc) {
              (*refs).rvalue = true;
            }
          }
        }
      } else {
        // For OpenCV
        for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
             refs++) {
          if ((*arg).Name == (*refs).Name && (*arg).RefLoc == (*refs).RefLoc) {
            if ((*funs).paramTypes[paramIt].find("Output") !=
                std::string::npos) {
              (*refs).rvalue = false;
              (*refs).lvalue = true;
            }
            if ((*funs).paramTypes[paramIt].find("Input") !=
                std::string::npos) {
              (*refs).lvalue = false;
              (*refs).rvalue = true;
            }
          }
        }
      }
      paramIt++;
    }
  }
  // STREAM TYPES:
  for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
       refs++) {
    if ((*refs).type.find("stream") != std::string::npos && (*refs).lvalue) {
      (*refs).rvalue = 1;
    }
  }

  return false;
}

/**
        \fn bool analyzeGlobalsLoop(int i,std::stringstream
   &SSComments,std::vector<unsigned> &stagesLocEnds)

        This function analyze if any global variable is used as Lvalue, that is
        is modified. This function returns true if any global is modified inside
        in more than one stage and/or is read in another stage else.

        RULE: Global variables can not be modified

        \param[in]      i       Loop to be analyzed
        \param[out]	SScomments	Stream of comments
        \param[in]	stagesLocEnds	Stages list
*/

bool MyASTVisitor::analyzeGlobalsLoop(int i, std::stringstream &SSComments,
                                      std::vector<unsigned> &stagesLocEnds) {
  bool globallvalue = false;
  // bool readGlobal = false;
  std::vector<std::string> lglobals;
  std::vector<int> stagesUsing;
  std::vector<std::string> stageswrite;
  // FOR EACH STAGE CHECK IF USES ANY GLOBAL VARIABLE AS LVALUE
  for (unsigned le = 0; le < stagesLocEnds.size(); le++) {
    // llvm::errs()<<"STAGE : "<<le<<"\n";
    // Check references in the loop
    for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
         refs++) {
      if ((*refs).RefLoc < stagesLocEnds[le] && (*refs).lvalue &&
          (*refs).globalVar) {
        lglobals.push_back((*refs).Name);
        stagesUsing.push_back(le);
        stageswrite.push_back((*refs).Name);
      }
      if ((*refs).RefLoc < stagesLocEnds[le] && (*refs).rvalue &&
          (*refs).globalVar) {
        lglobals.push_back((*refs).Name);
        stagesUsing.push_back(le);
      }
    }
    // Get globals used inside functions called on the stage i
    for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
         funs++) {
      for (auto funs2 = Functions.begin();
           funs2 != Functions.end() &&
           (*funs).SLoc.getRawEncoding() < stagesLocEnds[le];
           funs2++) {
        if ((*funs).Name == (*funs2).Name) {
          // Get every global used as L/RValue in the function
          for (unsigned g = 0; g < (*funs2).globalsUsed.size(); g++) {
            lglobals.push_back((*funs2).globalsUsed[g]);
            stagesUsing.push_back(le);
          }
          // Get every global used as lvalue in the function
          for (unsigned g = 0; g < (*funs2).globalsWrited.size(); g++) {
            stageswrite.push_back((*funs2).globalsWrited[g]);
          }
        }
      }
    }
  }
  // LAST STAGE
  if (stagesLocEnds.size() > 0) {
    // llvm::errs()<<"LAST STAGE\n";
    for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
         refs++) {
      if ((*refs).RefLoc > stagesLocEnds[stagesLocEnds.size() - 1] &&
          (*refs).lvalue && (*refs).globalVar) {
        lglobals.push_back((*refs).Name);
        stagesUsing.push_back(stagesLocEnds.size());
        stageswrite.push_back((*refs).Name);
      }
      if ((*refs).RefLoc < stagesLocEnds[i] && (*refs).rvalue &&
          (*refs).globalVar) {
        lglobals.push_back((*refs).Name);
        stagesUsing.push_back(stagesLocEnds.size());
      }
    }
    // Get globals used inside functions called on the last stage
    for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
         funs++) {
      for (auto funs2 = Functions.begin();
           funs2 != Functions.end() &&
           (*funs).SLoc.getRawEncoding() >
               stagesLocEnds[stagesLocEnds.size() - 1];
           funs2++) {
        if ((*funs).Name == (*funs2).Name) {
          // Get every global used as L/RValue in the function
          for (unsigned g = 0; g < (*funs2).globalsUsed.size(); g++) {
            lglobals.push_back((*funs2).globalsUsed[g]);
            stagesUsing.push_back(i);
          }
          // Get every global used as lvalue in the function
          for (unsigned g = 0; g < (*funs2).globalsWrited.size(); g++) {
            stageswrite.push_back((*funs2).globalsWrited[g]);
          }
        }
      }
    }
  }
  // Check if any writed lvalue is used in more than one stage
  // llvm::errs()<<"CHECK\n";
  for (unsigned i = 0; i < lglobals.size(); i++) {
    for (unsigned k = 0; k < stageswrite.size(); k++) {
      for (unsigned j = 0; j < lglobals.size() && stageswrite[k] == lglobals[i];
           j++) {
        if (lglobals[i] == lglobals[j] && stagesUsing[i] != stagesUsing[j]) {
          //	globallvalue = true;
          return true;
        }
      }
    }
  }
  return globallvalue;
}

/**
        \fn void stageDetection(int i, int &stages,  std::vector<unsigned>
   &stagesLocEnds)

        This function analyze Loop "i" to detect the stages of the pipeline.
   This function
        detects if any variable modified is used as input in any function to
   split in
        stages.

        \param[in]      i       Loop to be analyzed
        \param[in,out]	stages	Stages counter
        \param[in,out]	stagesLocEnds	Vector with stage ends locations
*/

void MyASTVisitor::stageDetection(int i, int &stages,
                                  std::vector<unsigned> &stagesLocEnds) {
  std::vector<unsigned> stagesLocDesordered;
  std::vector<unsigned> auxStagesLoc;
  std::vector<unsigned> forStages;

  // For each loop inside the loop i
  for (auto LoopSR = Loops[i].InternLoops.begin();
       LoopSR != Loops[i].InternLoops.end(); LoopSR++) {
    bool isEnd = true;
    // Check is an outer for inside the loop i
    for (auto LoopSR2 = Loops[i].InternLoops.begin();
         LoopSR2 != Loops[i].InternLoops.end(); LoopSR2++) {
      if ((*LoopSR).getBegin().getRawEncoding() >=
              (*LoopSR2).getBegin().getRawEncoding() &&
          (*LoopSR).getEnd().getRawEncoding() <=
              (*LoopSR2).getEnd().getRawEncoding() &&
          LoopSR != LoopSR2) {
        isEnd = false;
      }
    }
    // Generate a new stage for each outer loop
    if (isEnd) {
      stages++;
      stagesLocDesordered.push_back((*LoopSR).getBegin().getRawEncoding());
      stagesLocDesordered.push_back((*LoopSR).getEnd().getRawEncoding());
      forStages.push_back((*LoopSR).getBegin().getRawEncoding());
    }
  }

  unsigned bfLoc = 0;
  // For each function
  for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
       funs++) {
    bool noOut = false;
    bool insideLoop = false;
    // Eclude functions inside intern loops
    for (auto LoopSR = Loops[i].InternLoops.begin();
         LoopSR != Loops[i].InternLoops.end(); LoopSR++) {
      if ((*funs).SLoc.getRawEncoding() >
              (*LoopSR).getBegin().getRawEncoding() &&
          (*funs).SLoc.getRawEncoding() < (*LoopSR).getEnd().getRawEncoding()) {
        insideLoop = true;
      }
    }
    clang::SourceRange IfLoc;
    clang::SourceLocation ifEnd;
    bool insideCond = false;
    int CondIt = 0;
    for (auto Cond = Loops[i].ConditionalsStatements.begin();
         Cond != Loops[i].ConditionalsStatements.end(); Cond++) {
      if ((*funs).SLoc.getRawEncoding() > (*Cond).getBegin().getRawEncoding() &&
          (*funs).SLoc.getRawEncoding() < (*Cond).getEnd().getRawEncoding()) {

        //         std::cout<<(*funs).Name<<" is INSIDE A CONDITION\n";
        insideCond = true;
        IfLoc = (*Cond);
        ifEnd = Loops[i].CondEnd[CondIt];
      }
      CondIt++;
    }

    // If has outputs
    if (!noOut && !insideLoop) {
      // Ignore operators as stage limit
      if ((*funs).Name.find("operator") == std::string::npos) {
        // Split loops in stages
        if (funs != Loops[i].fCalls.end() - 1) {
          bool outputUsed = false;
          // Look for every variable reference used before
          for (auto refs = Loops[i].VarRef.begin();
               refs != Loops[i].VarRef.end(); refs++) {
            // unsigned bfLoc;
            // if is an Lvalue
            auto funs2 = funs + 1;
            // auto valid = false;
            // Exclude operators or print f as functions
            //   while(!valid){
            /* if((*funs2).Name.find("operator")!=std::string::npos
                     ||(*funs2).Name.find("printf")!=std::string::npos){
                     if(funs!=Loops[i].fCalls.end()-2){
                    // 	funs2++;
                     }else{
                             valid=true;
                     }
             }else{
                     valid = true;
             }*/
            //}

            // If any function output is used before
            if ((*refs).RefLoc < (*funs2).SLoc.getRawEncoding() &&
                (*refs).RefLoc >= bfLoc && (*refs).lvalue) {
              for (auto argsNext = (*funs2).Args.begin();
                   argsNext != (*funs2).Args.end(); argsNext++) {
                if ((*argsNext).Name == (*refs).Name) {
                  outputUsed = true;
                }
              }
            }
          }
          // If has any output used before, then generate a new stage
          if (outputUsed && !insideCond) {
            std::stringstream SSBefore;
            stages++;
            bfLoc = (*(funs)).EndLoc.getRawEncoding();
            stagesLocDesordered.push_back((*(funs)).EndLoc.getRawEncoding());
          }
          if (outputUsed && insideCond) {
            stages++;
            bfLoc = IfLoc.getBegin()
                        .getRawEncoding(); // (*(funs)).EndLoc.getRawEncoding();
            //		stagesLocDesordered.push_back(IfLoc.getBegin().getRawEncoding());
            //		stagesLocDesordered.push_back(IfLoc.getEnd().getRawEncoding());
            stagesLocDesordered.push_back(ifEnd.getRawEncoding());
          }
        } else {
          std::stringstream SSBefore;
          stages++;
          bfLoc = (*(funs)).SLoc.getRawEncoding();
          stagesLocDesordered.push_back((*(funs)).SLoc.getRawEncoding());
        }
      }
    }
  }
  // Reorder vector of stages
  unsigned last = 0;
  for (unsigned u = 0; u < stagesLocDesordered.size(); u++) {
    unsigned actual = std::numeric_limits<unsigned>::max();
    for (unsigned j = 0; j < stagesLocDesordered.size(); j++) {
      if (stagesLocDesordered[j] < actual && stagesLocDesordered[j] > last) {
        actual = stagesLocDesordered[j];
      }
    }
    auxStagesLoc.push_back(actual);
    last = actual;
  }
  // llvm::errs()<<"INITIAL STAGES : "<<auxStagesLoc.size()<<"\n";
  std::vector<unsigned> nonEmptyStages;
  // DISCARD EMPTY STAGES
  for (unsigned s = 0; s < auxStagesLoc.size(); s++) {
    bool empty = true;
    if (s != 0) {
      for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
           refs++) {
        if ((*refs).RefLoc < auxStagesLoc[s] &&
            (*refs).RefLoc > auxStagesLoc[s - 1])
          empty = false;
      }

      for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
           funs++) {
        if ((*funs).SLoc.getRawEncoding() < auxStagesLoc[s] &&
            (*funs).SLoc.getRawEncoding() > auxStagesLoc[s - 1])
          empty = false;
      }
    } else {
      for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
           refs++) {
        if ((*refs).RefLoc < auxStagesLoc[s])
          empty = false;
      }

      for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
           funs++) {
        if ((*funs).SLoc.getRawEncoding() < auxStagesLoc[s])
          empty = false;
      }
    }
    /*
    for(auto refs = Loops[i].VarRef.begin();refs!=
Loops[i].VarRef.end();refs++){
            if(s!=0){
                    if((*refs).RefLoc<=auxStagesLoc[s]&&(*refs).RefLoc>auxStagesLoc[s-1]){

                            bool declaration = false;
                            //Check if is a declaration
                            for(unsigned dec= 0;
dec<Loops[i].VarDecl.size();dec++){
                                    if(Loops[i].VarDecl[dec].Name ==
(*refs).Name &&
                                            Loops[i].VarDecl[dec].DeclLoc ==
(*refs).DeclLoc &&
                                            Loops[i].VarDecl[dec].RefLoc ==
(*refs).RefLoc){
                                            declaration  = true;
                                    }
                            }
                            if(!declaration){
                                    empty=false;
                            }else{
//If do not exists any reference, then look for functions
                                    for(auto funs =
Loops[i].fCalls.begin();funs!=Loops[i].fCalls.end();funs++){
                                            if((*funs).SLoc.getRawEncoding()<=auxStagesLoc[s]&&
                                                    (*funs).SLoc.getRawEncoding()>auxStagesLoc[s-1])
                                                    empty=false;
                                    }

                            }
                    }
            }
            if(s==0){
                    if((*refs).RefLoc<auxStagesLoc[s]){
                            bool declaration = false;
                            //Check if is a declaration
                            for(unsigned dec= 0;
dec<Loops[i].VarDecl.size();dec++){
                                    if(Loops[i].VarDecl[dec].Name ==
(*refs).Name &&
                                            Loops[i].VarDecl[dec].DeclLoc ==
(*refs).DeclLoc &&
                                            Loops[i].VarDecl[dec].RefLoc ==
(*refs).RefLoc){
                                            declaration  = true;
                                    }
                            }
                            if(!declaration){
                                    empty=false;
                            }else{
                             //If do not exists any reference, then look for
functions
                                    for(auto funs =
Loops[i].fCalls.begin();funs!=Loops[i].fCalls.end();funs++){
                                            if((*funs).SLoc.getRawEncoding()<auxStagesLoc[s])
                                                    empty=false;
                                    }
                            }
                    }
            }
    }*/
    /*for(auto funs =
    Loops[i].fCalls.begin();funs!=Loops[i].fCalls.end();funs++){

    }*/
    if (!empty) {
      nonEmptyStages.push_back(auxStagesLoc[s]);
      // stagesLocEnds.push_back(auxStagesLoc[s]);
    }
  }

  // llvm::errs()<<"NUM NON EMPTY STAGES : "<<nonEmptyStages.size()<<"\n";
  int lastValidStage = -1;
  // DISCARD STAGES WITHOUT OUTPUT USED ON THE NEXT STAGE
  for (unsigned s = 0;
       s < nonEmptyStages.size() - 1 && nonEmptyStages.size() > 1; s++) {
    bool noValidYet = false;
    if (lastValidStage == -1) {
      noValidYet = true;
    }
    // For each reference
    for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
         refs++) {
      // If checking the first stage
      if (noValidYet) {
        // If refs is on the actual stage
        if ((*refs).RefLoc <= nonEmptyStages[s]) {
          // Check if is used on the next stage
          for (auto refs2 = Loops[i].VarRef.begin();
               refs2 != Loops[i].VarRef.end(); refs2++) {
            if ((*refs2).RefLoc > nonEmptyStages[s] &&
                (*refs2).RefLoc < nonEmptyStages[s + 1]) {
              // If used
              if ((*refs2).Name == (*refs).Name &&
                  (*refs2).DeclLoc == (*refs).DeclLoc) {
                // Actual is the las valid stage
                lastValidStage = s;
                // Push on the stages vector if not previously added
                bool add = true;
                for (unsigned validStages = 0;
                     validStages < stagesLocEnds.size(); validStages++) {
                  if (nonEmptyStages[s] == stagesLocEnds[validStages]) {
                    add = false;
                  }
                }
                if (add)
                  stagesLocEnds.push_back(nonEmptyStages[s]);
              }
            }
          }
        }
      } else {
        // If refs is on the actual stage
        if ((*refs).RefLoc < nonEmptyStages[s] &&
            (*refs).RefLoc > nonEmptyStages[lastValidStage]) {
          // Check if is used on the next stage
          for (auto refs2 = Loops[i].VarRef.begin();
               refs2 != Loops[i].VarRef.end(); refs2++) {
            // Check if refs2 is a reference on the next stage
            if ((*refs2).RefLoc > nonEmptyStages[s] &&
                (*refs2).RefLoc < nonEmptyStages[s + 1]) {
              // If used
              if ((*refs2).Name == (*refs).Name &&
                  (*refs2).DeclLoc == (*refs).DeclLoc) {
                // Actual is the las valid stage
                lastValidStage = s;
                // Push on the stages vector if not previously added
                bool add = true;
                for (unsigned validStages = 0;
                     validStages < stagesLocEnds.size(); validStages++) {
                  if (nonEmptyStages[s] == stagesLocEnds[validStages]) {
                    add = false;
                  }
                }
                if (add)
                  stagesLocEnds.push_back(nonEmptyStages[s]);
              }
            }
          }
        }
      }
    }
  }
  // llvm::errs()<<"FINAL STAGES: "<<stagesLocEnds.size()<<"\n";
  stages = stagesLocEnds.size();
}

/**
        \fn bool checkStages(int i,int stages, std::vector<unsigned>
   stagesLocEnds)

        This function analyze Loop "i" to chek the stages of the pipeline. This
   function
        cheks if at least one variable modified in one stage is used as input in
   the next stage.
        Returns true if is a valid stage, false if not.

        RULE: Each stage should use as input at least one output from previous
   stages

        \param[in]	i	Loop to be analyzed
        \param[in]	stages	Stages counter
        \param[in]	stagesLocEnds	Vector with stage ends locations
        \return	true if is a valid stage, false if not.
*/

bool MyASTVisitor::checkStages(int i, int stages,
                               std::vector<unsigned> stagesLocEnds) {
  bool isUsed = true;
  bool varUsed = false;
  int actualStage = 0;
  // std::vector<std::vector<std::string>> outvec(stages);

  for (auto refs = Loops[i].VarRef.begin();
       refs != Loops[i].VarRef.end() && stages > 0; refs++) {
    // bool nextStage = false;
    // If is not the last stage

    if (actualStage <= stages) {
      // If reference is the next stage
      if (actualStage < stages) {
        if ((*refs).RefLoc > stagesLocEnds[actualStage]) {
          // Update stage
          actualStage++;
          //      nextStage++;
          // Check if any output is used on the next step
          if (varUsed == false) {
            isUsed = false;
          }
          if (actualStage != stages) {
            varUsed = false;
          } else {
            varUsed = true;
          }
        }
      } else {
        varUsed = true;
      }
      // If is lvalue
      if ((*refs).lvalue) {
        // Check if is rvalue on the next stage
        for (auto refs2 = Loops[i].VarRef.begin();
             refs2 != Loops[i].VarRef.end(); refs2++) {
          // If not the last stage
          if (actualStage < stages - 1) {
            // Check if reference is in the next stage
            if ((*refs2).RefLoc >= stagesLocEnds[actualStage] &&
                (*refs2).RefLoc < stagesLocEnds[actualStage + 1]) {
              if ((*refs).Name == (*refs2).Name &&
                  (*refs).DeclLoc == (*refs2).DeclLoc) {
                // if is rvalue set as used
                // if((*refs2).rvalue){
                varUsed = true;
                //}
              }
            }
          }
          if ((*refs2).RefLoc > stagesLocEnds[actualStage]) {
            if ((*refs).Name == (*refs2).Name &&
                (*refs).DeclLoc == (*refs2).DeclLoc) {
              // if is rvalue set as used
              // if((*refs2).rvalue){
              varUsed = true;
              //}
            }
          }
        }
      }
    }
  }

  return isUsed;
}
/**
    \fn bool checkFeedback(int i, std::stringstream &SSComments, bool &comments,
   int stages , std::vector<unsigned> stagesLocEnds)

        Checks if exists feedback in Loop "i". That is if exist any RaW
   dependency.

        RULE: On pipeline patterns must not exists any feedback

    \param[in]	i	Loop to be analyzed
        \param[out]	SSComments	Stream of commentaries about
   possible feedbacks
        \param[out]	comments	Set as true if exists any commentary
    \param[in]	stages	Stages counter
    \param[in]	stagesLocEnds	Vector with stage ends locations
    \return true if exists feedback, false if not.
*/

bool MyASTVisitor::checkFeedback(int i, std::stringstream &SSComments,
                                 bool &comments, int stages,
                                 std::vector<unsigned> stagesLocEnds) {
  bool feedback = false;
  bool firstPosFeed = true;
  for (auto refs = (Loops[i].VarRef.end() - 1);
       refs != (Loops[i].VarRef.begin() - 1); refs--) {
    bool feedbackVAR = false;
    bool isLocal = false;
    for (unsigned dec = 0; dec < Loops[i].VarDecl.size(); dec++) {
      if (Loops[i].VarDecl[dec].Name == (*refs).Name &&
          Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc) {
        isLocal = true;
      }
    }

    bool posFeed = false;
    // If is lvalue
    if ((*refs).lvalue && !isLocal) {
      // check if it was rvalue before
      for (auto refs2 = refs; refs2 != (Loops[i].VarRef.begin() - 1); refs2--) {
        if ((*refs).Name == (*refs2).Name) {
          // Check if is a memory access.
          // As we can't detect memory collisions we exclude them from this
          // checking
          bool memoryAccess = false;
          int mem = 0;
          for (unsigned memAcc = 0; memAcc < Loops[i].MemAccess.size();
               memAcc++) {
            if ((*refs).Name == Loops[i].MemAccess[memAcc].Name &&
                (*refs).RefLoc == Loops[i].MemAccess[memAcc].RefLoc) {
              memoryAccess = true;
              mem = memAcc;
            }
          }
          bool memoryAccess2 = false;
          int mem2 = 0;
          for (unsigned memAcc = 0; memAcc < Loops[i].MemAccess.size();
               memAcc++) {
            if ((*refs2).Name == Loops[i].MemAccess[memAcc].Name &&
                (*refs2).RefLoc == Loops[i].MemAccess[memAcc].RefLoc) {
              memoryAccess2 = true;
              mem2 = memAcc;
            }
          }
          // If both references are memory access
          if (memoryAccess && memoryAccess2) {
            if ((*refs2).lvalue) {
              feedbackVAR = false;
            }
            if ((*refs2).rvalue) {
              // Check if is the same position
              if (Loops[i].MemAccess[mem].index.size() ==
                  Loops[i].MemAccess[mem2].index.size()) {
                feedbackVAR = true;
                // If is not the same position, then is not a RaW dependency
                for (unsigned index = 0;
                     index < Loops[i].MemAccess[mem].index.size(); index++) {
                  if (Loops[i].MemAccess[mem].index[index] !=
                      Loops[i].MemAccess[mem2].index[index]) {
                    feedbackVAR = false;
                  }
                }
              }
            }
          }
          if ((*refs).opModifier) {
            feedback = true;
          }
          // If is a compound assignment exists feedback
          if ((refs == refs2 && (*refs).cmpAssign) || (*refs).opModifier) {
            feedbackVAR = true;
          }

          if ((*refs2).lvalue & !memoryAccess) {
            feedbackVAR = false;
            // CHECK LINE
            // If is in a for declaration.
            bool inFor = false;
            for (unsigned loop = 0; loop < Loops[i].InternLoops.size();
                 loop++) {
              if ((*refs).RefLoc >
                      Loops[i].InternLoops[loop].getBegin().getRawEncoding() &&
                  (*refs).RefLoc < Loops[i].BodyStarts[loop].getRawEncoding()) {
                inFor = true;
              }
            }
            SourceManager &SM = TheRewriter.getSourceMgr();
            auto ref3 = refs2 + 1;

            if (ref3 != Loops[i].VarRef.end() && !inFor && !memoryAccess) {
              auto loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
              auto loc2 = SM.getDecomposedLoc((*ref3).RefSourceLoc);
              auto line1 = SM.getLineNumber(loc1.first, (*refs2).RefLoc);
              auto line2 = SM.getLineNumber(loc2.first, (*ref3).RefLoc);
              while (line1 == line2) {
                if ((*refs2).Name == (*ref3).Name && (*ref3).rvalue &&
                    (*refs2).RefLoc != (*ref3).RefLoc) {
                  feedbackVAR = true;
                }
                ref3++;
                if (ref3 != Loops[i].VarRef.end()) {
                  loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
                  loc2 = SM.getDecomposedLoc((*ref3).RefSourceLoc);
                  line1 = SM.getLineNumber(loc1.first, (*refs2).RefLoc);
                  line2 = SM.getLineNumber(loc2.first, (*ref3).RefLoc);
                } else {
                  line2++;
                }
              }
            }
            //--------------------
          }
          if ((*refs2).rvalue && !memoryAccess) {
            feedbackVAR = true;
            posFeed = false;
            // If is an L/Rvalue varible set as possible feedback
            if ((*refs2).lvalue && !(*refs2).cmpAssign) {
              feedbackVAR = false;
              //        posFeed = true;
              //         comments = true;
            }
          }
        }
      }
      // If is writing on a stream type
      if ((*refs).type.find("stream") != std::string::npos) {
        feedbackVAR = true;
      }
      if ((*refs).lvalue && (*refs).rvalue && (*refs).sureKind) {
        feedbackVAR = true;
      }
    }
    // Insert comment for feedbacks or possible feedbacks
    if (feedbackVAR) {
      feedback = true;
      SSComments << "//Feedback: " << (*refs).Name;
      SSComments << "\n";
    }
    if (posFeed) {
      if (!firstPosFeed) {
        SSComments << ",";
      }
      firstPosFeed = false;
      SSComments << " \"PossibleFeedback(" << (*refs).Name << ")on_stage(";
      int st = 0;
      for (int it = 0; it < stages - 1; it++) {
        if ((*refs).RefLoc > stagesLocEnds[it]) {
          st++;
        }
      }
      SSComments << st << ")\"\n";
    }
  }

  return feedback;
}

/**
        \fn bool checkFarmStages(std::vector<unsigned> stagesLocEnds, int i)

        This function analyze every Loop detected and detect if any is
        a possible pipeline or not. This function modifies struct members
        "Loops", "Functions" and "Code" adding addicional information.

*/

void MyASTVisitor::checkFarmStages(std::vector<unsigned> stagesLocEnds, int i,
                                   std::vector<bool> &farmstage) {
  for (unsigned st = 0; st < stagesLocEnds.size(); st++) {
    farmstage[st] = 1;
    unsigned beg;
    if (st == 0) {
      beg = 0;
    } else {
      beg = stagesLocEnds[st - 1];
    }
    unsigned end = stagesLocEnds[st];
    for (auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();
         refs++) {
      if ((*refs).RefLoc < end && (*refs).RefLoc > beg) {
        if ((*refs).globalVar) {
          // EXISTS SIDE EFFECTS
          farmstage[st] = 0;
        }
      }
    }
    for (auto calls = Loops[i].fCalls.begin(); calls != Loops[i].fCalls.end();
         calls++) {
      bool found = 0;

      if ((*calls).SLoc.getRawEncoding() > beg &&
          (*calls).SLoc.getRawEncoding() < end) {

        for (auto fun = Functions.begin(); fun != Functions.end(); fun++) {
          if ((*calls).Name == (*fun).Name) {
            if ((*fun).globalsUsed.size() > 0) {
              // EXISTS SIDE EFFECTS
              farmstage[st] = 0;
            }
            found = 1;
          }
        }
        if (!found) {
          for (auto f = whiteList.begin(); f != whiteList.end(); f++) {
            if ((*calls).Name == (*f)) {
              found = 1;
            }
          }
          if (!found) {
            farmstage[st] = 0;
          }
        }
      }
    }
  }
}

/**
        \fn bool analyzeLRLoop(int i)

        This function analyze every Loop detected and detect if any is
        a possible pipeline or not. This function modifies struct members
        "Loops", "Functions" and "Code" adding addicional information.

*/
bool MyASTVisitor::AnalyzeLoops() {
  // Analize function declarations with bodies
  analyzeCodeFunctions();
  // Counter farms = maps
  int feqm = 0;
  // Counter farms = pipes
  int feqp = 0;
  int nestedFarm = 0;

  int numpipelines = 0;
  //	std::vector<int> dictionaryAdds;
  // llvm::errs()<<"ANALYZE LOOPS\n";
  int pipelinesCounter = 0;
  for (int i = 0; i < numLoops; i++) {
    // llvm::errs()<<"LOOP "<<i<<"\n";
    SourceManager &SM = TheRewriter.getSourceMgr();
    /*		auto lineStart =
       SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
                    auto lineEnd =
       SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first,Loops[i].RangeLoc.getEnd().getRawEncoding());
                    auto fileName =
       SM.getFileEntryForID(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first)->getName();

            //	SourceManager &SM = TheRewriter.getSourceMgr();*/
    auto fileName = Loops[i].FileName;
    auto currfile =
        Loops[i]
            .FileName; // SM.getFileEntryForID(SM.getMainFileID())->getName();
    // Annotate only if the loop is in the current file.
    if (fileName == currfile) {
      std::stringstream SSComments;
      SSComments << "";
      bool comments = false;
      std::vector<ASTVar> globals;
      std::vector<unsigned> stagesLocEnds;
      std::stringstream SS;
      // SS<<"\n//STAGE 1 \n";
      int stages = 0;
      // Check non accessible functions usage
      bool nonaccesible = false;
      // Analyze function calls
      // TODO: TO BE IMPLEMENTED (MULTIPLEITERATIONS) On multiple iterations,
      // this is the only one function to be executed on the first iteration.
      // llvm::errs()<<"ANALYZE CALLS\n";
      analyzeCall(i);

      //---------------
      /*		for(auto funs = Functions.begin(); funs!=
         Functions.end();funs++){
                              for(auto arguments =
         (*funs).Arguments.begin();arguments != (*funs).Arguments.end();
         arguments++){
                              }
                      }
      */
      // llvm::errs()<<"ANALYZE LR\n";
      //----------------
      // Check for L/Rvalue
      analyzeLRLoop(i);

      // llvm::errs()<<"STAGE DETECTION\n";
      // Stages detection
      stageDetection(i, stages, stagesLocEnds);
      // llvm::errs()<<"ANALIYZE GLOBALS\n";
      // Global variable LValue
      bool globallvalue = analyzeGlobalsLoop(i, SSComments, stagesLocEnds);
      //		bool globallvalue = false;
      // Check if at least 1 modified variable in a stage is used in the next
      // stage
      // llvm::errs()<<"CHECK STAGES\n";
      bool isUsed = true; // checkStages(i,stages,stagesLocEnds);
      // Check feedback
      // llvm::errs()<<"CHECK FEEDBACK\n";
      bool feedback = false;
      // bool feedback =
      // checkFeedback(i,SSComments,comments,stages,stagesLocEnds);
      // GET INPUTS AND OUTPUTS
      std::vector<std::vector<std::string>> outputs(stages + 1);
      std::vector<std::vector<std::string>> inputs(stages + 1);
      // llvm::errs()<<"GET IO\n";
      getStageIO(inputs, outputs, stages, i, stagesLocEnds);
      std::vector<bool> farmstage(stagesLocEnds.size());
      checkFarmStages(stagesLocEnds, i, farmstage);
      std::vector<std::string> stream;
      // llvm::errs()<<"GET STREAM\n";
      getPipelineStream(inputs, outputs, stages, stream);
      // Annotation
      // PRINT PIPELINE
      // If every requirement is satisfied
      if (!feedback && globallvalue == false && isUsed == true &&
          Loops[i].numOps > 1 && stages >= 1) {
        // Add comment before
        std::stringstream SSBefore;
        std::stringstream declarations;
        if (comments || nonaccesible) {
          SSBefore << "[[rph::pipeline, rph::id(" << numpipelines
                   << "), rph::with_warnings\n, rph::warnings(\n";
          if (nonaccesible) {

            SSBefore << " \"functioninaccessible\"\n,";
          }

          SSBefore << SSComments.str();
          SSBefore << ")\n";
        } else {

          Loops[i].pipeline = true;

          SSBefore << "[[rph::pipeline, rph::id(" << numpipelines << ") ";
          if (Loops[i].map || Loops[i].farm) {
            if (Loops[i].map) {
              SSBefore << ", rph::map";
            }
            if (Loops[i].farm) {
              SSBefore << ", rph::farm";
              feqp++;
            }
            // ADD MAP I/O
            if (Loops[i].inputs.size() > 0) {
              SSBefore << ", rph::in(";
              for (unsigned in = 0; in < Loops[i].inputs.size(); in++) {
                SSBefore << Loops[i].inputs[in];
                if (in != Loops[i].inputs.size() - 1) {
                  SSBefore << ",";
                }
              }
              SSBefore << ")";
            }
            if (Loops[i].outputs.size() > 0) {
              SSBefore << ", rph::out(";
              for (unsigned out = 0; out < Loops[i].outputs.size(); out++) {
                SSBefore << Loops[i].outputs[out];
                if (out != Loops[i].outputs.size() - 1) {
                  SSBefore << ",";
                }
              }
              SSBefore << ")";
            }
          }

          // SourceManager &SM = TheRewriter.getSourceMgr();
          auto lineStart = SM.getLineNumber(
              SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,
              Loops[i].RangeLoc.getBegin().getRawEncoding());
          auto lineEnd = SM.getLineNumber(
              SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first,
              Loops[i].RangeLoc.getEnd().getRawEncoding());
          //  auto fileName =
          //  SM.getFileEntryForID(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first)->getName();
          // llvm::errs()<<"PIPELINE FOUND ON:
          // "<<fileName<<":"<<lineStart<<"\n";
          // std::cout<<stages<<"
          // "<<fileName<<":"<<lineStart<<":"<<lineEnd<<"\n";
          pipelinesCounter++;
        }
        /*		if(stream.size()>0){
                                SSBefore << ", rph::stream(";
                                for(auto iter = stream.begin();iter!=
           stream.end();iter++){
                                        SSBefore << (*iter);
                                        if(iter!=stream.end()-1)
                                                SSBefore << ",";
                                }
                                SSBefore << ")";
                        }*/
        SSBefore << "]]\n";
        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(),
                               true, true);

        // moveDeclarations(i,declarations);
        // PRINT STAGES

        for (int st = 0; st <= stages; st++) {
          std::stringstream SSStage;
          // llvm::errs()<<stages<<" "<<stagesLocEnds.size()<<"
          // "<<stagesLocEnds[st]<<"\n";
          if (st != 0)
            SSStage << "\n}";
          if (st == 0)
            SSStage << declarations.str();
          SSStage << "\n\n[[rph::stage(" << st << "), rph::pipelineid("
                  << numpipelines << ")";
          if (farmstage[st]) {
            SSStage << ", rph::farm";
            nestedFarm++;
          }
          if (inputs[st].size() > 0) {
            SSStage << ", rph::in(";
            for (auto in = inputs[st].begin(); in != inputs[st].end(); in++) {
              SSStage << (*in);
              if (in != inputs[st].end() - 1)
                SSStage << ",";
            }
            SSStage << ")";
          }
          if (outputs[st].size() > 0) {
            SSStage << ", rph::out(";
            for (auto out = outputs[st].begin(); out != outputs[st].end();
                 out++) {
              SSStage << (*out);
              if (out != outputs[st].end() - 1)
                SSStage << ",";
            }
            SSStage << ")";
          }
          SSStage << "]]\n{\n";
          if (st == 0) {
            TheRewriter.InsertText(Loops[i].InitLoc, SSStage.str(), true, true);
          } else {

            auto StageLocation =
                SourceLocation::getFromRawEncoding(stagesLocEnds[st - 1]);
            //                    auto loc = SM.getDecomposedLoc(StageLocation);
            //                    auto line =
            //                    SM.getLineNumber(loc.first,StageLocation.getRawEncoding());
            //                   auto fEntry = SM.getFileEntryForID(loc.first);
            //                  auto col = 1;
            //                    auto beginLine =
            //                    SM.translateFileLineCol(fEntry,(line+1),col);

            TheRewriter.InsertText(StageLocation, SSStage.str(), true, true);

            //					auto StageLocation =
            // SourceLocation::getFromRawEncoding(stagesLocEnds[st-1]);
            //					TheRewriter.InsertText(StageLocation,
            // SSStage.str(),
            // true, true);
          }
          std::stringstream SSEndStage;
          if (st == stages) {
            std::stringstream SSEndStage;
            SSEndStage << "\n\t}\n";
            TheRewriter.InsertText(Loops[i].RangeLoc.getEnd(), SSEndStage.str(),
                                   true, true);
          }
        }
        numpipelines++;

      } else {
        // If not satisfied
        // Add comment before
        std::stringstream SSBefore;
        std::stringstream ErrorMessage;
        auto line = SM.getPresumedLineNumber(Loops[i].RangeLoc.getBegin());

        ErrorMessage << "== Loop in " << fileName << ":" << line
                     << " does not match a pipeline pattern!\n";

        ErrorMessage << SSComments.str() << " ";
        // Insert why is not a pipeline
        //			if(Loops[i].numOps<=1) ErrorMessage<<"\tNot
        // enough
        // compute\n";
        if (!isUsed)
          ErrorMessage << "\tNo output from previous stage is used as input\n";
        if (stages <= 1 || Loops[i].numOps <= 1)
          ErrorMessage << "\tFound less than 2 stages\n";
        if (globallvalue)
          ErrorMessage << "\tFound a write on a global variable.\n";
        std::cout << ErrorMessage.str();

        if (Loops[i].map || Loops[i].farm) {
          SSBefore << "[[";
          if (Loops[i].map) {
            SSBefore << "rph:map";
          }
          if (Loops[i].map && Loops[i].farm) {
            SSBefore << ", ";
          }
          if (Loops[i].farm) {
            SSBefore << "rph:farm";
          }
          if (Loops[i].farm && Loops[i].map) {
            feqm++;
          }

          // ADD MAP I/O
          if (Loops[i].inputs.size() > 0) {
            SSBefore << ", rph::in(";
            for (unsigned in = 0; in < Loops[i].inputs.size(); in++) {
              SSBefore << Loops[i].inputs[in];
              if (in != Loops[i].inputs.size() - 1) {
                SSBefore << ",";
              }
            }
            SSBefore << ")";
          }
          if (Loops[i].outputs.size() > 0) {
            SSBefore << ", rph::out(";
            for (unsigned out = 0; out < Loops[i].outputs.size(); out++) {
              SSBefore << Loops[i].outputs[out];
              if (out != Loops[i].outputs.size() - 1) {
                SSBefore << ",";
              }
            }
            SSBefore << ")";
          }
          SSBefore << "]]";
        }

        SSBefore << "\n";
        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(),
                               true, true);
      }
      //	std::cout<<"TOTAL PIPELINES: "<<pipelinesCounter<<"\n";
    }
  }
  std::cout << "Pipeline patterns detected: " << numpipelines << "\n";
  std::cout << "Nested farms: " << nestedFarm << "\n";
  //		std::cout<<"Total loops found : "<<numLoops<<"\n";
  std::cout << std::flush;
  return true;
}

/**
        \fn void getPipelineStream(std::vector<std::vector<std::string>>
&inputs, std::vector<std::vector<std::string>>&outputs, int stages,
std::vector<std::string> &stream)

        This function get the stream of the pipeline. That is every stage output
        which is used as input in othe stages plus outputs from the last stage.

        \param[in]	input	Vector of inputs variables
        \param[in]	output	Vector of outputs variables
        \param[in]	stages	Number of stages
        \param[out]	stream	Vector of stream variables
*/

void MyASTVisitor::getPipelineStream(
    std::vector<std::vector<std::string>> &inputs,
    std::vector<std::vector<std::string>> &outputs, int stages,
    std::vector<std::string> &stream) {
  std::vector<std::string> pipelineinputs;
  std::vector<std::string> pipelineoutputs;
  // Join inputs and outputs from every stage
  for (int i = 0; i <= stages; i++) {
    for (auto in = inputs[i].begin(); in != inputs[i].end(); in++) {
      bool add = true;
      for (auto list = pipelineinputs.begin(); list != pipelineinputs.end();
           list++) {
        if ((*in) == (*list))
          add = false;
      }
      if (add)
        pipelineinputs.push_back((*in));
    }
    for (auto out = outputs[i].begin(); out != outputs[i].end(); out++) {
      bool add = true;
      for (auto list = pipelineoutputs.begin(); list != pipelineoutputs.end();
           list++) {
        if ((*out) == (*list))
          add = false;
      }
      if (add)
        pipelineoutputs.push_back((*out));
    }
  }
  // Add to stream every input/output
  for (auto in = pipelineinputs.begin(); in != pipelineinputs.end(); in++) {
    for (auto out = pipelineoutputs.begin(); out != pipelineoutputs.end();
         out++) {
      if ((*in) == (*out)) {
        bool add = true;
        for (auto list = stream.begin(); list != stream.end(); list++) {
          if ((*in) == (*list))
            add = false;
        }
        if (add)
          stream.push_back((*in));
      }
    }
  }
  // Add outputs from the last stage
  for (auto out = outputs[stages].begin(); out != outputs[stages].end();
       out++) {
    bool add = true;
    for (auto list = stream.begin(); list != stream.end(); list++) {
      if ((*out) == (*list))
        add = false;
    }
    if (add)
      stream.push_back((*out));
  }
}
/**
        \fn bool getStageIO(std::vector<std::vector<std::string>> &inputs,
   std::vector<std::vector<std::string>>&outputs, int stages, int
   i,std::vector<unsigned> stagesLocEnds)

        This function get every input and outputs of each stage.
        Check if is local variable from a loop.

        \param[out]	input	Vector of inputs variables
        \param[out]	output	Vector of outputs variables
        \param[in]	stages	Number of stages
        \param[in]	stagesLocEnds	Vector of stage ends locations
*/
void MyASTVisitor::getStageIO(std::vector<std::vector<std::string>> &inputs,
                              std::vector<std::vector<std::string>> &outputs,
                              int stages, int i,
                              std::vector<unsigned> stagesLocEnds) {
  for (int stIt = 0; stIt <= stages && stages > 0; stIt++) {
    for (auto vars = Loops[i].VarRef.begin(); vars != Loops[i].VarRef.end();
         vars++) {
      // Chak if var is intern loop local variable
      bool inLocalLoop = false;
      bool internalIterator = false;
      bool loopHead = false;
      std::vector<ASTVar> iterators;
      int body = 0;
      for (auto LoopSR = Loops[i].InternLoops.begin();
           LoopSR != Loops[i].InternLoops.end(); LoopSR++) {
        if ((*vars).DeclLoc > (*LoopSR).getBegin().getRawEncoding() &&
            (*vars).DeclLoc < (*LoopSR).getEnd().getRawEncoding()) {
          inLocalLoop = true;
        }
        if ((*vars).RefLoc > (*LoopSR).getBegin().getRawEncoding() &&
            (*vars).RefLoc < Loops[i].BodyStarts[body].getRawEncoding()) {
          iterators.push_back((*vars));
          loopHead = true;
        }
        body++;
      }
      // CHECK IF ANY REFERENCE INSIDE THE FOR DECLARATION DEPENDS ON OUTFOR
      // VARIABLES
      bool insideStage = false;
      if (stIt != stages && stIt != 0) {
        if ((*vars).RefLoc < stagesLocEnds[stIt] &&
            (*vars).RefLoc > stagesLocEnds[stIt - 1])
          insideStage = true;
      }
      if (stIt == 0) {
        if ((*vars).RefLoc < stagesLocEnds[stIt])
          insideStage = true;
      }
      if (stIt == stages) {
        if ((*vars).RefLoc > stagesLocEnds[stIt - 1])
          insideStage = true;
      }
      // Check if varaibles defined or references on the head of the for depends
      // on other variables
      if (loopHead && insideStage) {
        for (unsigned as = 0; as < Loops[i].Assign.size(); as++) {
          for (unsigned dep = 0;
               dep < Loops[i].Assign[as].dependencies.size() &&
               (*vars).Name == Loops[i].Assign[as].Name &&
               (*vars).DeclLoc == Loops[i].Assign[as].DeclLoc;
               dep++) {
            bool add = true;
            // Every dependence of this variable is an input of the stage
            for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                 it++) {
              if ((*it) == Loops[i].Assign[as].dependencies[dep].Name)
                add = false;
            }
            if (add)
              inputs[stIt].push_back(
                  Loops[i].Assign[as].dependencies[dep].Name);
          }
        }
      }

      // Check if is a reference to the iterator
      for (unsigned it = 0; it < iterators.size(); it++) {
        if ((*vars).Name == iterators[it].Name &&
            (*vars).DeclLoc == iterators[it].DeclLoc)
          internalIterator = true;
      }
      // If is not the first or last stage
      if (stIt != stages && stIt != 0 && !internalIterator) {
        // If vars is on this stage
        if ((*vars).RefLoc < stagesLocEnds[stIt] &&
            (*vars).RefLoc >
                stagesLocEnds
                    [stIt - 1] /*&&(*vars).DeclLoc < stagesLocEnds[stIt-1]*/) {
          // Check if is a value used outside the stage
          bool usedOut = checkOutVar((*vars), stagesLocEnds[stIt]);
          bool usedIn = checkInVar((*vars), stagesLocEnds[stIt - 1]);
          // If is not a local variable of an intern loop
          if ((*vars).lvalue && !inLocalLoop) {
            bool add = true;
            for (auto it = outputs[stIt].begin(); it != outputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add && usedOut)
              outputs[stIt].push_back((*vars).Name);

            if ((*vars).DeclLoc < stagesLocEnds[stIt - 1]) {
              for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                   it++) {
                if ((*it) == (*vars).Name)
                  add = false;
              }
              if (add && usedIn)
                inputs[stIt].push_back((*vars).Name);
            }
          }
          if ((*vars).rvalue && (*vars).DeclLoc < stagesLocEnds[stIt - 1] &&
              !inLocalLoop) {
            bool add = true;
            for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add && usedIn)
              inputs[stIt].push_back((*vars).Name);
          }
        }
      }
      // If is the first stage
      if (stIt == 0 && !internalIterator) {
        if ((*vars).RefLoc < stagesLocEnds[stIt]) {
          bool usedOut = checkOutVar((*vars), stagesLocEnds[stIt]);
          bool usedIn = checkInVar((*vars), Loops[i].InitLoc.getRawEncoding());
          if ((*vars).lvalue && !inLocalLoop) {
            bool add = true;
            if (!(*vars).cmpAssign && (*vars).RefLoc == (*vars).DeclLoc)
              add = false;
            for (auto it = outputs[stIt].begin(); it != outputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add && usedOut)
              outputs[stIt].push_back((*vars).Name);

            if ((*vars).DeclLoc < Loops[i].InitLoc.getRawEncoding()) {
              for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                   it++) {
                if ((*it) == (*vars).Name)
                  add = false;
              }
              if (add && usedIn)
                inputs[stIt].push_back((*vars).Name);
            }
          }
          if ((*vars).rvalue &&
              (*vars).DeclLoc < Loops[i].InitLoc.getRawEncoding() &&
              !inLocalLoop) {
            bool add = true;
            for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add /*&&usedIn*/)
              inputs[stIt].push_back((*vars).Name);
          }
        }
      }
      // If is the last stage
      if (stIt == stages && !internalIterator) {
        if ((*vars).RefLoc > stagesLocEnds[stIt - 1] && !inLocalLoop &&
            (*vars).DeclLoc < stagesLocEnds[stIt - 1]) {
          bool usedOut =
              checkOutVar((*vars), Loops[i].RangeLoc.getEnd().getRawEncoding());
          bool usedIn = checkInVar((*vars), stagesLocEnds[stIt - 1]);
          // if is rvalue
          if ((*vars).lvalue) {
            // CHECK IF IS AN OUTPUT
            bool add = true;
            for (auto it = outputs[stIt].begin(); it != outputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add && usedOut)
              outputs[stIt].push_back((*vars).Name);
            // CHECK IF IS A INPUT
            if ((*vars).DeclLoc < stagesLocEnds[stIt - 1]) {
              for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                   it++) {
                if ((*it) == (*vars).Name)
                  add = false;
              }
              if (add && usedIn)
                inputs[stIt].push_back((*vars).Name);
            }
          }
          // CHECK IF IS AN INPUT
          if ((*vars).rvalue && (*vars).DeclLoc < stagesLocEnds[stIt - 1] &&
              !inLocalLoop) {
            bool add = true;
            for (auto it = inputs[stIt].begin(); it != inputs[stIt].end();
                 it++) {
              if ((*it) == (*vars).Name)
                add = false;
            }
            if (add /*&&usedIn*/)
              inputs[stIt].push_back((*vars).Name);
          }
        }
      }
    }
  }
}

/**
        \fn bool checkOutVar(ASTVar var, unsigned end)

        This function checks if the var is used after the point end

        \param[in]	var	Variable to be checked
        \param[in]	end 	Location of the end of the stage
*/
bool MyASTVisitor::checkOutVar(ASTVar var, unsigned end) {
  if (var.globalVar)
    return true;
  for (auto refs = Code.VarRef.begin(); refs != Code.VarRef.end(); refs++) {
    if ((*refs).RefLoc >= end) {
      if ((*refs).Name == var.Name && (*refs).DeclLoc == var.DeclLoc &&
          (*refs).RefLoc != (*refs).DeclLoc) {
        return true;
      }
    }
  }
  return false;
}

/**
        \fn bool checkInVar(ASTVar var, unsigned start)

        This function checks if the var is used after the point end

        \param[in]      var     Variable to be checked
        \param[in]      end     Location of the end of the stage
*/

bool MyASTVisitor::checkInVar(ASTVar var, unsigned start) {
  if (var.globalVar)
    return true;
  for (auto refs = Code.VarRef.begin(); refs != Code.VarRef.end(); refs++) {
    if ((*refs).RefLoc <= start) {
      if ((*refs).Name == var.Name && (*refs).DeclLoc == var.DeclLoc &&
          (*refs).RefLoc != (*refs).DeclLoc) {
        return true;
      }
    }
  }
  return false;
}

/**
        \fn bool analyzeLRValues(ASTFunctionDecl &function)

        This function analyze each variable reference in the function struct and
   set kind as
        LRValue if meets the constrains defined. Additionally this fucntion
   checks if other
        functions are called inside and are accesibles.

        \param[in]      function       Function to be analyzed
*/
bool MyASTVisitor::analyzeLRValues(ASTFunctionDecl &function) {
  for (auto funs = function.fCalls.begin(); funs != function.fCalls.end();
       funs++) {
    unsigned paramIt = 0;
    // Look for every argument
    for (auto arg = (*funs).Args.begin();
         arg != (*funs).Args.end() && (*funs).paramTypes.size() > paramIt;
         arg++) {
      // bool LRValue = false;
      // If is a modifiable argument
      if ((*funs).paramTypes[paramIt].find("&") != std::string::npos ||
          (*funs).paramTypes[paramIt].find("*") != std::string::npos) {
        // If is not constant
        if ((*funs).paramTypes[paramIt].find("const") == std::string::npos) {
          bool accesibleFunction = false;
          for (auto funs2 = Functions.begin(); funs2 != Functions.end();
               funs2++) {
            int argumentIt = 0;
            if ((*funs2).Name == (*funs).Name) {
              //              int argumentIt = 0;
              for (auto arg = (*funs).Args.begin(); arg != (*funs).Args.end();
                   arg++) {
                for (auto refs = function.VarRef.begin();
                     refs != function.VarRef.end(); refs++) {
                  if ((*arg).Name == (*refs).Name &&
                      (*arg).RefLoc == (*refs).RefLoc) {
                    // set as L/Rvalue/
                    (*refs).lvalue = (*funs2).Arguments[argumentIt].lvalue;
                    (*refs).rvalue = (*funs2).Arguments[argumentIt].rvalue;
                    if (!(*funs2).partialAccess) {
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                    }
                  }
                }
                argumentIt++;
              }
            }
          }
          for (auto dic = dictionary.begin();
               dic != dictionary.end() && !accesibleFunction; dic++) {
            //       for(auto arg =
            //       (*funs).Args.begin();arg!=(*funs).Args.end();arg++){
            if ((*dic).Name == (*funs).Name) {
              for (auto refs = function.VarRef.begin();
                   refs != function.VarRef.end(); refs++) {
                if ((*arg).Name == (*refs).Name &&
                    (*arg).RefLoc == (*refs).RefLoc) {

                  if ((*dic).arguments.find(paramIt) !=
                      (*dic).arguments.end()) {
                    if ((*dic).arguments[paramIt] == kind::L) {
                      (*refs).lvalue = true;
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                      (*refs).rvalue = false;
                    }
                    if ((*dic).arguments[paramIt] == kind::R) {
                      (*refs).lvalue = false;
                      accesibleFunction = true;
                      (*refs).sureKind = true;
                      (*refs).rvalue = true;
                    }
                    if ((*dic).arguments[paramIt] == kind::LR) {

                      (*refs).lvalue = true;
                      (*refs).rvalue = true;
                      (*refs).sureKind = true;
                      accesibleFunction = true;
                    }
                    if ((*dic).arguments[paramIt] == kind::U) {
                      (*refs).lvalue = true;
                      (*refs).rvalue = true;
                    }
                  } else {

                    (*refs).lvalue = true;
                    //  accesibleFunction = true;
                    // (*refs).sureKind = true;
                    (*refs).rvalue = true;
                  }
                }
              }
            }
            //      }
          }

          // Search on the reference list
          for (auto refs = function.VarRef.begin();
               refs != function.VarRef.end() && !accesibleFunction; refs++) {
            if ((*arg).Name == (*refs).Name &&
                (*arg).RefLoc == (*refs).RefLoc) {
              // set as L/Rvalue
              (*refs).lvalue = true;
              (*refs).rvalue = true;
              bool indic = false;
              for (auto dic = dictionary.begin(); dic != dictionary.end();
                   dic++) {
                if ((*dic).Name == (*funs).Name) {
                  // llvm::errs()<<"UNKNOWN FUNCTION ARGUMENTS KIND\n";
                  (*dic).arguments[paramIt] = kind::U;
                  indic = true;
                }
              }
              if (!indic) {
                DictionaryFunction f;
                f.Name = (*funs).Name;
                f.arguments[paramIt] = kind::U;
                dictionaryAdds.push_back(dictionary.size());
                dictionary.push_back(f);
              }
            }
          }
        }
      } else {
        for (auto refs = function.VarRef.begin(); refs != function.VarRef.end();
             refs++) {
          if ((*arg).Name == (*refs).Name && (*arg).RefLoc == (*refs).RefLoc) {
            if ((*funs).paramTypes[paramIt].find("Output") !=
                std::string::npos) {
              (*refs).lvalue = true;
              (*refs).rvalue = false;
            }
            if ((*funs).paramTypes[paramIt].find("Input") !=
                std::string::npos) {
              (*refs).lvalue = false;
              (*refs).rvalue = true;
            }
          }
        }
      }
      paramIt++;
    }
  }

  return false;
}

/**
        \fn bool analyzeFunction(ASTFunctionDecl &function)

        Analyze a function to determine L/RValues variable references and
        check possible feedbacks.

        \param[in]      function       Function to be analyzed
        \param[in]	deep	Value to detect recursivity
*/

bool MyASTVisitor::analyzeFunction(ASTFunctionDecl &function, int deep) {
  //	analyzeLRValues(function);
  // TODO: Check function calls inside functions
  deep++;
  for (auto funs = function.fCalls.begin();
       funs != function.fCalls.end() && deep <= 10; funs++) {
    for (auto funs2 = Functions.begin(); funs2 != Functions.end(); funs2++) {

      if ((*funs).Name == (*funs2).Name) {
        bool toodeep = analyzeFunction((*funs2), deep);
        if (!toodeep) {
          function.recursive = true;
          return false;
        }
      }
    }
  }
  if (deep > 10) {
    function.recursive = true;
    return false;
  }
  analyzeLRValues(function);
  // Check globals

  for (auto ref = function.VarRef.begin(); ref != function.VarRef.end();
       ref++) {
    if ((*ref).globalVar && (*ref).lvalue) {
      function.globalLvalue = true;
      function.globalsUsed.push_back((*ref).Name);
      function.globalsWrited.push_back((*ref).Name);
    }
    if ((*ref).globalVar && (*ref).lvalue) {
      function.globalsUsed.push_back((*ref).Name);
    }
  }
  // Check L/R values
  // For each argument
  for (auto farg = function.Arguments.begin(); farg != function.Arguments.end();
       farg++) {
    bool firstOc = true;
    // Check references for each argument
    for (auto ref = function.VarRef.begin(); ref != function.VarRef.end();
         ref++) {
      if ((*ref).Name == (*farg).Name) {
        // If is lvalue
        if ((*ref).lvalue) {
          // Set argument as lvalue
          (*farg).lvalue = true;
          SourceManager &SM = TheRewriter.getSourceMgr();
          auto ref2 = ref + 1;
          // If is the first ocurrence and exists more references check the same
          // line
          if (ref2 != function.VarRef.end() && firstOc) {
            // Extract fileId and line
            auto loc1 = SM.getDecomposedLoc((*ref).RefSourceLoc);
            auto loc2 = SM.getDecomposedLoc((*ref2).RefSourceLoc);
            auto line1 = SM.getLineNumber(loc1.first, (*ref).RefLoc);
            auto line2 = SM.getLineNumber(loc2.first, (*ref2).RefLoc);
            // While exists refrences on the same line
            while (line1 == line2) {
              // If is an rvalue in the same sentence then set as LRvalue ->
              // Pissible RAW dependence
              if ((*ref).Name == (*ref2).Name && (*ref2).rvalue && firstOc) {
                (*farg).rvalue = true;
              }
              // Get next reference
              ref2++;
              if (ref2 != function.VarRef.end()) {
                loc1 = SM.getDecomposedLoc((*ref).RefSourceLoc);
                loc2 = SM.getDecomposedLoc((*ref2).RefSourceLoc);
                line1 = SM.getLineNumber(loc1.first, (*ref).RefLoc);
                line2 = SM.getLineNumber(loc2.first, (*ref2).RefLoc);

              } else {
                // if is the last reference add 1 to line2
                line2++;
              }
            }
          }
        }
        // If is the first ocurrence an is Rvalue set as rvalue
        if ((*ref).rvalue && firstOc) {
          (*farg).rvalue = true;
        }
        firstOc = false;
      }
    }
  }
  return true;
}
