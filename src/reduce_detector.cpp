template <typename Ref>
bool MyASTVisitor::checkFeedbackSameLine(Ref refs, int i) {
  // Check on the same line
  bool feedbackVAR = false;
  SourceManager &SM = TheRewriter.getSourceMgr();
  auto nextVar = refs + 1;
  if (nextVar != Loops[i].VarRef.end()) {
    auto loc1 = SM.getDecomposedLoc((*refs).RefSourceLoc);
    auto loc2 = SM.getDecomposedLoc((*nextVar).RefSourceLoc);
    auto line1 = SM.getLineNumber(loc1.first, (*refs).RefLoc);
    auto line2 = SM.getLineNumber(loc2.first, (*nextVar).RefLoc);
    while (line1 == line2) {
      if ((*refs).Name == (*nextVar).Name && (*nextVar).rvalue) {
        feedbackVAR = true;
      }
      nextVar++;
      if (nextVar != Loops[i].VarRef.end()) {
        loc1 = SM.getDecomposedLoc((*refs).RefSourceLoc);
        loc2 = SM.getDecomposedLoc((*nextVar).RefSourceLoc);
        line1 = SM.getLineNumber(loc1.first, (*refs).RefLoc);
        line2 = SM.getLineNumber(loc2.first, (*nextVar).RefLoc);
      } else {
        line2++;
      }
    }
  }
  return feedbackVAR;
}

/**
        bool checkFeedback(int i, std::stringstream &SSComments, bool &comments)

        This function checks if exists any feedback in the loop "i". If exists
   feedback
        return true.

        \param[in]      i       Loop to be analyzed
        \param[out]     SSComments      Comments about the feedback
        \param[out]     comments        bool to check if exists comments

*/

bool MyASTVisitor::checkFeedback(int i, std::vector<std::string> &feedbackVars,
                                 std::vector<std::string> &operators) {
  bool feedback = false;
  // bool firstPosFeed = true;
  for (auto refs = (Loops[i].VarRef.end() - 1);
       refs != (Loops[i].VarRef.begin() - 1); refs--) {
    bool feedbackVAR = false;
    bool local = false;
    for (unsigned dec = 0; dec < Loops[i].VarDecl.size(); dec++) {
      if (Loops[i].VarDecl[dec].Name == (*refs).Name &&
          Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc) {
        local = true;
      }
    }

    if ((*refs).lvalue && !local) {
      bool memoryAccess = false;
      int mem = 0;
      bool inFor = false;
      for (unsigned loop = 0; loop < Loops[i].InternLoops.size(); loop++) {
        if ((*refs).RefLoc >
                Loops[i].InternLoops[loop].getBegin().getRawEncoding() &&
            (*refs).RefLoc < Loops[i].BodyStarts[loop].getRawEncoding()) {
          inFor = true;
        }
      }

      for (unsigned memAcc = 0; memAcc < Loops[i].MemAccess.size(); memAcc++) {
        if ((*refs).Name == Loops[i].MemAccess[memAcc].Name &&
            (*refs).RefLoc == Loops[i].MemAccess[memAcc].RefLoc) {
          memoryAccess = true;
          mem = memAcc;
        }
      }
      bool privated = false;
      if (!memoryAccess) {
        privated = checkPrivate(i, (*refs));
      }
      // Check on the same line
      bool feedbackOp = checkFeedbackSameLine(refs, i);
      if (feedbackOp && !memoryAccess) {
        /*		bool add = false;
                        for(unsigned fb = 0; fb < feedbackVars.size(); fb++){
                                if((*refs).Name == feedbackVars[fb]) add = true;
                        }
                        if(!add){
                                feedbackVars.push_back((*refs).Name);

                        }*/
      }

      if ((*refs).cmpAssign && !memoryAccess && !inFor && !privated) {
        bool add = false;
        for (unsigned fb = 0; fb < feedbackVars.size(); fb++) {
          if ((*refs).Name == feedbackVars[fb])
            add = true;
        }
        if (!add) {
          feedbackVars.push_back((*refs).Name);
          operators.push_back((*refs).cmpOper);
          // feedbackVAR = true;
        }
      }

      for (auto refs2 = refs - 1;
           refs2 != (Loops[i].VarRef.begin() - 1) && !privated; refs2--) {
        if ((*refs).Name == (*refs2).Name &&
            (*refs).DeclLoc == (*refs).DeclLoc) {
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
          if (memoryAccess && memoryAccess2) {
            //	llvm::errs()<<"DOUBLE MEM REFERENCE";
            if ((*refs2).lvalue) {
              feedbackVAR = false;
            }
            if ((*refs2).rvalue) {
              if (Loops[i].MemAccess[mem].index.size() ==
                  Loops[i].MemAccess[mem2].index.size()) {
                feedbackVAR = true;
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
          if (refs == refs2 && (*refs).cmpAssign && !memoryAccess) {
            bool add = false;
            for (unsigned fb = 0; fb < feedbackVars.size(); fb++) {
              if ((*refs).Name == feedbackVars[fb])
                add = true;
            }
            if (!add) {
              feedbackVars.push_back((*refs).Name);
              operators.push_back((*refs).cmpOper);

              // feedbackVAR = true;
            }
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
                // inFor=true;
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
                if ((*refs2).Name == (*ref3).Name && (*ref3).rvalue) {
                  /*	bool add = false;
                          for(unsigned fb = 0; fb < feedbackVars.size(); fb++){
                                  if((*refs).Name == feedbackVars[fb]) add =
                     true;
                          }
                          if(!add) feedbackVars.push_back((*refs).Name);*/
                  // feedbackVAR =true;
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
          if ((*refs2).rvalue && !memoryAccess && !(*refs2).cmpAssign) {
            bool add = false;
            /*for(unsigned fb = 0; fb < feedbackVars.size(); fb++){
                    if((*refs).Name == feedbackVars[fb]) add = true;
            }
            if(!add) feedbackVars.push_back((*refs).Name);*/
            feedbackVAR = true;
            // posFeed = false;
            // If is an L/Rvalue varible set as possible feedback
            if ((*refs2).lvalue && !(*refs2).cmpAssign) {
              feedbackVAR = false;

              //        posFeed = true;
              //         comments = true;
            }
          }
        }
      }
    }
    if (feedbackVAR) {
      feedback = true;
      //	feedbackVars.push_back((*refs).Name);
    }
  }
  return feedback;
}

bool MyASTVisitor::reduceOperation(std::vector<std::string> feedbackVars,
                                   std::vector<std::string> outputs,
                                   std::vector<std::string> &reduceVars) {
  bool reduce = false;

  for (unsigned i = 0; i < outputs.size(); i++) {
    for (unsigned j = 0; j < feedbackVars.size(); j++) {
      if (outputs[i] == feedbackVars[j]) {
        reduce = true;
        reduceVars.push_back(feedbackVars[j]);
      }
    }
  }

  return reduce;
}

void MyASTVisitor::reduceDetect() {
  // Analize function declarations with bodies
  analyzeCodeFunctions();
  int numreduce = 0;
  // int numpipelines = 0;
  for (int i = 0; i < numLoops; i++) {
    std::stringstream SSComments;
    // bool comments = false;
    // Analyze function calls
    analyzeCall(i);
    // Check for L/Rvalue
    analyzeLRLoop(i);
    // Global variable LValue : Check if modified globals (potential side
    // effects)
    // bool globallvalue = false;
    auto fileName = Loops[i].FileName;

    std::vector<std::string> outputs;
    std::vector<std::string> inputs;

    // Get inputs and outputs
    getMapIO(inputs, outputs, i);
    std::vector<std::string> feedbackVars;
    std::vector<std::string> operators;
    // Check input memory access

    // Check feedback
    bool feedback = checkFeedback(i, feedbackVars, operators);

    bool hasreturn = false;
    if (Loops[i].Returns.size() > 0) {
      hasreturn = true;
    }
    bool hasbreak = check_break(i);
    bool hasgoto = false;
    if (Loops[i].Gotos.size() > 0) {
      hasgoto = true;
    }

    /*
                    for(unsigned dbg = 0 ; dbg < feedbackVars.size(); dbg++){
                            llvm::errs()<<feedbackVars[dbg]<<"\n";

                    }
    */
    std::vector<std::string> reduceVars;
    // Check if the outputs are the same as feedback variables
    bool valid = reduceOperation(feedbackVars, outputs, reduceVars);
    for (auto funs = Loops[i].fCalls.begin(); funs != Loops[i].fCalls.end();
         funs++) {
      if ((*funs).Name.find("fprintf") != std::string::npos)
        valid = false;
      if ((*funs).Name.find("fscanf") != std::string::npos)
        valid = false;
    }

    if (valid && !feedback) {
      //			std::cout<<"======= REDUCE PATTERN DETECTED
      //=======\n";
      Loops[i].reduce = true;
      numreduce++;

      std::stringstream SSBefore;
      SSBefore << "[[rph::reduce";
      // ADD INPUTS
      if (inputs.size() > 0) {
        SSBefore << " , rph::in(";
        for (int in = 0; in < inputs.size(); in++) {
          SSBefore << inputs[in];
          if (in != inputs.size() - 1)
            SSBefore << ", ";
        }
        SSBefore << ") ";
      }
      // ADD OUTPUTS
      if (outputs.size() > 0) {
        SSBefore << " , rph::out(";
        for (int out = 0; out < outputs.size(); out++) {
          SSBefore << outputs[out];
          if (out != outputs.size() - 1)
            SSBefore << ", ";
        }
        SSBefore << ")";
      }
      // ADD REDUCE OUTPUTS
      if (reduceVars.size() > 0) {
        SSBefore << " , rph::reduce(";
        for (int red = 0; red < reduceVars.size(); red++) {
          SSBefore << reduceVars[red];
          Loops[i].reduceVar.push_back(reduceVars[red]);
          if (red != reduceVars.size() - 1)
            SSBefore << ", ";
        }
        SSBefore << ") ";
      }

      SSBefore << "]]\n";

      for (unsigned ops = 0; ops < operators.size(); ops++) {
        // std::cout<<"OPERATOR " << operators[ops] << "\n";
        Loops[i].operators.push_back(operators[ops]);
      }

      if (!omp)
        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(),
                               true, true);

      // llvm::errs()<<"\nLOOP "<<i<<" IS A REDUCE\n";
    } else {
      std::stringstream SSBefore;
      SourceManager &SM = TheRewriter.getSourceMgr();

      auto line = SM.getPresumedLineNumber(Loops[i].RangeLoc.getBegin());
      SSBefore << "== Loop in " << fileName << ":" << line
               << " does not match a reduce pattern!\n";
      if (feedback)
        SSBefore << "\tFeedback detected.\n ";
      //            if(globallvalue) SSBefore<<"\tFound a write on a global
      //            variable.\n";
      if (hasreturn)
        SSBefore << "\tFound a return statement.\n";
      //            if(empty) SSBefore<<"\tFound no parallelizable
      //            statements.\n";
      if (hasbreak)
        SSBefore << "\tFound break statement.\n";
      if (hasgoto)
        SSBefore << "\tFound goto statement.\n";
      if (!valid)
        SSBefore << "\tNo reduce operation detected\n";

      std::cout << SSBefore.str();
    }
    //        else llvm::errs()<<"LOOP "<<i<<" IS NOT A REDUCE\n";
  }
  std::cout << "Reduce patterns detected: " << numreduce;
  std::cout << std::flush;
}
