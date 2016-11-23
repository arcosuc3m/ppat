/**
 *  @brief Determine wether a farm has or not a code region potentially 
 *    parallelizable or not. 
 *
 *  Check if there is no parallel section, if so then the loop is discarded 
 *  to be refactored as a farm and the code is not modified.
 *  @param i Index of the loop that is being analyzed
 */
bool MyASTVisitor::checkEmptyTask(int i){
   bool empty = true; /* by default the loop is empty */
   for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
      bool isTask = true;
      if((*stmt).getBegin().getRawEncoding() >= Loops[i].genStart.getRawEncoding()
          &&  Loops[i].genEnd.getRawEncoding() > (*stmt).getBegin().getRawEncoding() ) isTask= false;

      for(auto sink = Loops[i].sinkZones.begin(); sink != Loops[i].sinkZones.end();  sink++){
         if((*stmt).getBegin().getRawEncoding() >= (*sink).second.first.getRawEncoding()
          &&  (*sink).second.second.getRawEncoding() > (*stmt).getBegin().getRawEncoding() ) isTask= false;
      }
      if(isTask) empty =false;
   }
   return empty;
}


/**
 * @brief Get the variables used as non shared variables in the loop.
 *
 */
void MyASTVisitor::getPrivated(int i){
    for(auto refs = (Loops[i].VarRef.end()-1);refs != (Loops[i].VarRef.begin()-1);refs--){
        bool memoryAccess = false;
        int mem= 0;
        //Check if the variable is a container object or an array
        for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
            if((*refs).Name == Loops[i].MemAccess[memAcc].Name&&
               (*refs).RefLoc == Loops[i].MemAccess[memAcc].RefLoc){
                   memoryAccess = true;
                   mem=memAcc;
             }
        }
        //Check if is a local variable
        bool local = false;
        for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
             if(Loops[i].VarDecl[dec].Name == (*refs).Name
                && Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc){
                    local=true;
             }
        }
        //Check if is a private variable
        bool privated=false;
        if(!local&&!memoryAccess){
             privated = checkPrivate(i, (*refs));
        }
        if(privated){
          Loops[i].privatedVars.insert(std::pair<std::string,ASTVar> ((*refs).Name, (*refs)) );
        }
    }
}


/**
   bool getSinkFunction(int i)

   This function computes the code regions potentially related to the sink lambda function.
   It takes as sink variables, those variables that are shared among different computing 
   entities and written, not related to the generation part. Then it checks if the variable 
   is not used before the first write nor after the las write. If both requirements are met
   the code region can be used as a sink code region.

   Then this function computes which variables used in the sink zones are modified or comes from
   previous phases. Note that those variables cannot be the same as the "sink variables".

   Finally, this function returns true if the sink code regions are valid.

*/

bool MyASTVisitor::getSinkFunction(int i){
   std::map< std::string, std::pair<clang::SourceLocation, clang::SourceLocation> > sinkVars;
   for(auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();refs++){
        //Check if is a local variable
        bool local = false;
        for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
             if(Loops[i].VarDecl[dec].Name == (*refs).Name
                && Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc){
                    local=true;
             }
        }
        //Check if is a private variable
        bool privated=false;
        if(!local){
             privated = checkPrivate(i, (*refs));
        }
        //If the variable is shared and writen annotate it as potetial sink var
        if(!local && !privated && (*refs).lvalue){
           bool inserted = false;
           for(auto sink = sinkVars.begin(); sink != sinkVars.end(); sink++){
              if((*sink).first == (*refs).Name){
                  if((*sink).second.first.getRawEncoding() > (*refs).RefLoc ) (*sink).second.first = (*refs).RefSourceLoc;
                  if((*sink).second.second.getRawEncoding() < (*refs).RefLoc ) (*sink).second.second = (*refs).RefSourceLoc;
                  inserted = true;
              }
           }
           if(!inserted){
              sinkVars.insert( std::pair< std::string, std::pair <clang::SourceLocation, clang::SourceLocation> >( (*refs).Name, std::pair <clang::SourceLocation, clang::SourceLocation> ( (*refs).RefSourceLoc, (*refs).RefSourceLoc )));
           }
        }        
   }

   SourceManager &sm = TheRewriter.getSourceMgr();
   clang::LangOptions lopt;
   clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);


   //Get sink zone for the sink vars
   for(auto sink = sinkVars.begin(); sink != sinkVars.end(); sink++){
      bool endStmt = true;
      for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
         if( ( *sink).second.second.getRawEncoding() < (*stmt).getBegin().getRawEncoding() ){ (*sink).second.second = (*stmt).getBegin() ; endStmt = false; break;  }
      }
      for(auto stmt = Loops[i].stmtLoc.end()-1 ; stmt != Loops[i].stmtLoc.begin()-1; stmt--){
         if( (*stmt).getEnd().getRawEncoding() < (*sink).second.first.getRawEncoding() ){ (*sink).second.first = (*(stmt+1)).getBegin(); break;}
      }
      if(endStmt) (*sink).second.second = Loops[i].RangeLoc.getEnd();
   }
   //If the modified variable is never used after the last write or before the last write, its a sink variable
   bool sinkZone = false;
   for(auto sink = sinkVars.begin(); sink != sinkVars.end(); sink++){
      bool validZone = true;
      for(auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();refs++){
          if((*sink).first == (*refs).Name && (*refs).RefLoc < (*sink).second.first.getRawEncoding()) validZone = false;
          if((*sink).first == (*refs).Name && (*refs).RefLoc >= (*sink).second.second.getRawEncoding()) validZone = false;
      }
      if(validZone){
          Loops[i].sinkZones.insert( (*sink) );
          sinkZone = true;
      }
   }
   std::map<std::string, std::string> sinkUsedVars;
   if(sinkZone){
      //Get the variables used in the sink zone
      for(auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();refs++){
         bool used = false;
         for(auto sink = Loops[i].sinkZones.begin(); sink != Loops[i].sinkZones.end(); sink++){
           if((*refs).RefLoc >= (*sink).second.first.getRawEncoding() && (*refs).RefLoc < (*sink).second.second.getRawEncoding() ){
              used=true;
           }
         }
         if(used)
            sinkUsedVars.insert(std::pair<std::string,std::string> ( (*refs).Name, (*refs).type ) );
      }
      //Computes if the used variables comes from other phases
      for(auto refs = Loops[i].VarRef.begin(); refs != Loops[i].VarRef.end();refs++){
         bool isIn = false;
         for(auto sink = Loops[i].sinkZones.begin(); sink != Loops[i].sinkZones.end(); sink++){
           if((*refs).RefLoc < (*sink).second.first.getRawEncoding() || (*refs).RefLoc >= (*sink).second.second.getRawEncoding() ){
             for(auto used = sinkUsedVars.begin(); used!= sinkUsedVars.end(); used++){
               if((*used).first == (*refs).Name)
                  isIn = true;
             } 
           }
         }
         if(isIn)
            Loops[i].taskVar.insert(std::pair<std::string,std::string> ( (*refs).Name, (*refs).type ) );
             
      }


   }

   return sinkZone;
   
 
}

/**
   \fn void getStreamGeneratorOut (int i )

   Get the variables modified in the stream generation function.
*/

void MyASTVisitor::getStreamGeneratorOut (int i ){
  std::vector<ASTVar> modifiedVars;
  for(auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end(); var++){
      if( (*var).lvalue && (*var).RefLoc >= Loops[i].genStart.getRawEncoding() && (*var).RefLoc < Loops[i].genEnd.getRawEncoding() )
         modifiedVars.push_back((*var));
  }
  for(auto condVar = Loops[i].cond.VarRef.begin(); condVar != Loops[i].cond.VarRef.end(); condVar++){
       if((*condVar).lvalue) 
          modifiedVars.push_back((*condVar));
  }
  std::map<std::string, std::string> generatorVars;
  for(auto cond = modifiedVars.begin(); cond != modifiedVars.end(); cond++){
     for(auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end(); var++){
        if((*var).Name == (*cond).Name){
           if( (*var).RefLoc < Loops[i].genStart.getRawEncoding() || (*var).RefLoc >= Loops[i].genEnd.getRawEncoding() ) 
               generatorVars.insert( std::pair<std::string, std::string> ( (*cond).Name, (*cond).type) ) ;
        }
     }
  }

  Loops[i].genVar = generatorVars;

}

/**
  std::vector< ASTVar > getOtherGenerations(int i, SourceLocation &fWrite, SourceLocation &lWrite)

  Returns all the modified variables inside the code between fWrite and lWrite. This function is used to know
  if there are more generator variables.
*/
std::vector< ASTVar > MyASTVisitor::getOtherGenerations(int i, SourceLocation &fWrite, SourceLocation &lWrite){
     SourceManager &sm = TheRewriter.getSourceMgr();
     clang::LangOptions lopt;
     clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);
     std::vector< ASTVar > modified_variables;
     
     //Detect modified variables in the current stream generation code
     for(auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end(); var++){
         bool isLocal = false;
         for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
              if(Loops[i].VarDecl[dec].Name == (*var).Name
                 && Loops[i].VarDecl[dec].DeclLoc == (*var).DeclLoc){
                  isLocal=true;
             }
         }
         bool privated = false;
         //FIXME: THIS PART IS A BIT CONFUSSING
  /*       for(auto priv = Loops[i].privatedVars.begin(); priv != Loops[i].privatedVars.end(); priv++){
            if((*priv).first == (*var).Name) privated = true;
         }*/
         if( (*var).lvalue && (*var).RefLoc >= fWrite.getRawEncoding() 
              && lWrite.getRawEncoding() > (*var).RefLoc && !isLocal && !privated ){
            modified_variables.push_back((*var));  
         }
     }
     //Get references to every modified variables
     std::vector< ASTVar > streamGenerationRefs;
     for(auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end(); var++){
        for(auto mod = modified_variables.begin(); mod != modified_variables.end(); mod++){
           if((*mod).Name == (*var).Name){
              streamGenerationRefs.push_back((*var));
              break;
           }
        }
     }
     //Get new code locations
     for(auto cond = streamGenerationRefs.begin(); cond != streamGenerationRefs.end(); cond++){
        //std::cout<<"GENERATION VARIABLE "<< (*cond).Name<<"\n";
        if((*cond).lvalue){
            if(isBefore((*cond).RefSourceLoc, fWrite)) { fWrite = (*cond).RefSourceLoc; }
            if(isBefore(lWrite, (*cond).RefSourceLoc)) lWrite = (*cond).RefSourceLoc;
        }
     }

     return streamGenerationRefs; 
}

bool MyASTVisitor::getStreamGeneration(int i){
     //FIXME: It requires to detect multiple generation regions
     //TODO: Its necessary to add in this part the variables modified by an unknown function. If the variable modified by a unknown fucntion its never used or does not depends on the actual generation variables should be discarded and probably added as a sink variable. Also, its necesary to add as stream generation variables, those variables used in a if condition that perfroms a brek inside its body. For including multiple zones is required to detect where the different variables are used in the bode (before or after). //TODO: GOOD LUCK FUTURE ME!
     std::vector< ASTVar> condition_variables;
     //Get first and last writes of the modified condition variables
     //Reads are only allowed between both first and last writes and only after or before the write section. Otherwise is not a farm, since the stream generation is iteration dependent.
     for(auto var = Loops[i].VarRef.begin(); var != Loops[i].VarRef.end(); var++){
        for(auto condVar = Loops[i].cond.VarRef.begin(); condVar != Loops[i].cond.VarRef.end(); condVar++){
           if((*condVar).Name == (*var).Name)
              condition_variables.push_back(*var);
         }
     }
     clang::SourceLocation firstWrite = Loops[i].bodyRange.getBegin();
     clang::SourceLocation lastWrite = Loops[i].RangeLoc.getEnd();
     SourceManager &sm = TheRewriter.getSourceMgr();
     clang::LangOptions lopt;
     clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);
     for(auto cond = condition_variables.begin(); cond != condition_variables.end(); cond++){
        if((*cond).lvalue){
           // std::cout<< "CONDITION : " << (*cond).Name << std::endl;
            if(isBefore(firstWrite, (*cond).RefSourceLoc)) { firstWrite = (*cond).RefSourceLoc; }
            if(isBefore((*cond).RefSourceLoc,lastWrite)) lastWrite = (*cond).RefSourceLoc;
        }
     }
     //Recompute the code region related to the stream generation formula until the zone doesn't change.
     SourceLocation fWrite = firstWrite;
     SourceLocation lWrite = lastWrite;
     do{
         firstWrite = fWrite;
         lastWrite = lWrite;
         bool endStmt = true;
         for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
            if( lWrite.getRawEncoding() < (*stmt).getBegin().getRawEncoding() ){ lWrite = (*stmt).getBegin() ; endStmt = false; break;  }
         }
         for(auto stmt = Loops[i].stmtLoc.end()-1 ; stmt != Loops[i].stmtLoc.begin()-1; stmt--){
            if( (*stmt).getEnd().getRawEncoding() <= fWrite.getRawEncoding() ){ fWrite = (*(stmt+1)).getBegin(); break;}
         }
         if(endStmt) lWrite = Loops[i].RangeLoc.getEnd();
         //std::cout<<"LOCATIONS : " << fWrite.getRawEncoding() << " "<<lWrite.getRawEncoding() << std::endl;
         //for(auto cond = condition_variables.begin(); cond != condition_variables.end();cond++){
              //std::cout<<"CONDITION VARIABLES : " << (*cond).Name<<" "<<(*cond).lvalue<< " "<<(*cond).RefSourceLoc.getRawEncoding() <<std::endl;
         //}

         condition_variables = getOtherGenerations(i, fWrite, lWrite);
         //for(auto cond = condition_variables.begin(); cond != condition_variables.end();cond++){
         //     std::cout<<"CONDITION VARIABLES : " << (*cond).Name<<" "<<(*cond).lvalue<< std::endl;
         //}
     }while(fWrite.getRawEncoding() != firstWrite.getRawEncoding() && lWrite.getRawEncoding() != lastWrite.getRawEncoding());

     firstWrite = fWrite;
     lastWrite = lWrite;
     //Compute if the variable is used after or before
     bool before = false;
     bool after = false;
     if(firstWrite == Loops[i].bodyRange.getBegin() && lastWrite == Loops[i].RangeLoc.getEnd()) return true;
     for(auto cond = condition_variables.begin(); cond != condition_variables.end(); cond++){
        if(isBefore( (*cond).RefSourceLoc, firstWrite)) before = true;
        if(isBefore( lastWrite, (*cond).RefSourceLoc)) after = true;
     }

     

     Loops[i].genBefore = before; 
     Loops[i].genAfter = after;
 
     Loops[i].genStart = firstWrite;
     Loops[i].genEnd = lastWrite;
  //   }

     //If the generation variables are not used in the compute or used after and before their modification, the loop cannot be transformed into a farm pattern. 
     //std::cout<<"AFTER : " << after << " BEFORE: "<<before << std::endl;

     if(after&&before){ Loops[i].genStart = Loops[i].bodyRange.getBegin(); Loops[i].genEnd = Loops[i].bodyRange.getBegin();}
     //if(!after&&!before){Loops[i].genStart = Loops[i].bodyRange.getBegin(); Loops[i].genEnd = Loops[i].bodyRange.getBegin();}

     return true;
}


void MyASTVisitor::farmDetect(){
        analyzeCodeFunctions();
        int numFarm = 0;
        //int numpipelines = 0;
        for(int i = 0; i < numLoops;i++){
           //llvm::errs()<<"LOOP: "<<i<<"\n"; 
           SourceManager &SM = TheRewriter.getSourceMgr();
         auto fileName = Loops[i].FileName;
           auto currfile = Loops[i].FileName;
           //Annotate only if the loop is in the current file. 
           if(fileName == currfile){


            std::stringstream SSComments;
                bool comments = false;
                //Analyze function calls
                analyzeCall(i);
                //Check for L/Rvalue
                analyzeLRLoop(i);
                //Get stream generation stage
                getPrivated(i);        
                bool valid = getStreamGeneration(i);        
                bool sinkFun = getSinkFunction(i);
//                if(sinkFun) std::cout<<"TIENE FUNCION SINK\n";
                //Global variable LValue
                bool globallvalue = globalLValue(i);

                std::vector<std::string> outputs;
                std::vector<std::string> inputs;
                std::vector<std::string> inputTypes;
                std::vector<std::string> outputTypes;
    
                //Get inputs and outputs
                getMapIO(inputs, inputTypes, outputs, outputTypes, i);
                std::vector<std::string> outStreams;
                std::vector<std::string> inStreams;

                //Get IO data streams
                getDataStreams(inputs, inStreams,i);
                getDataStreams(outputs, outStreams,i);
                //Check feedback        
                bool feedback = checkFeedback(i,SSComments,comments);

                //Get output data stream dependencies
                getStreamDependencies(outStreams,i);
                //Check if output streams are only used as write or read in the same position. 
                checkMemAccess(outStreams,i);

            bool hasreturn = false;
                    if(Loops[i].Returns.size()>0){
                           hasreturn=true;
                 }

            bool hasbreak = check_break(i);

                bool hasgoto = false;
                if(Loops[i].Gotos.size()>0){
                        hasgoto = true;
                }
            
            bool empty = checkEmptyTask(i); 
            if(Loops[i].numOps == 0 ) empty = true;
//              if(Loops[i].genStart == Loops[i].RangeLoc.getBegin() && Loops[i].genEnd == Loops[i].RangeLoc.getEnd() && !valid) empty = true;
  
            for(auto funs = Loops[i].fCalls.begin();funs!=Loops[i].fCalls.end();funs++){
                        if((*funs).Name.find("fprintf")!=std::string::npos) valid = false;
                        if((*funs).Name.find("fscanf")!=std::string::npos) valid = false;
                        if((*funs).Name.find("strtok")!=std::string::npos) valid = false;
                        if((*funs).Name.find("fgets")!=std::string::npos) valid = false;
                }


    //----------- DEBUG ----------
    /*llvm::errs()<<"INPUTS\n";
    for(unsigned in=0;in<inputs.size();in++){
      llvm::errs()<<inputs[in]<<"\n";
    }
    llvm::errs()<<"Outputs\n";
                for(unsigned out=0;out<outputs.size();out++){
                        llvm::errs()<<outputs[out]<<"\n";
                }*/
    //----------------------------
                if(!feedback&&!globallvalue&&!hasreturn&&!empty&&!hasbreak&&!hasgoto&&valid){
                  numFarm++;
                        Loops[i].farm = true;
                        //ADD I/O IN LOOPS
                  if(!Loops[i].map){
                          for(unsigned in=0;in<inputs.size();in++){
                                  Loops[i].inputs.push_back(inputs[in]);
                                  Loops[i].inputTypes.push_back(inputTypes[in]);
                          }
                          for(unsigned out=0;out<outputs.size();out++){
                                  Loops[i].outputs.push_back(outputs[out]);
                                  Loops[i].outputTypes.push_back(outputTypes[out]);
                          }
                  }
                }else{

                        std::stringstream SSBefore;
//                        auto line = SM.getLineNumber(SM.getDecomposedSpellingLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());

                        auto line = SM.getPresumedLineNumber(Loops[i].RangeLoc.getBegin());
                        //SSBefore<< "== INVALID PATTERN == Invalid farm on "<<fileName<<":"<<line<<"\n";
                        SSBefore << "== Loop in "<<fileName<<":"<<line<< " does not match a farm pattern!\n";
                        if(feedback) SSBefore<<"\tFeedback detected.\n ";
                        if(globallvalue) SSBefore<<"\tFound a write on a global variable.\n";
                  if(hasreturn) SSBefore<<"\tFound a return statement.\n";
                  if(empty) SSBefore<<"\tFound no parallelizable statements.\n";
                  if(hasbreak) SSBefore<<"\tFound break statement.\n";
                  if(hasgoto) SSBefore<<"\tFound goto statement.\n";


                  std::cout<<SSBefore.str();

                }
    }
  }
  std::cout<<"Farm patterns found: "<<numFarm;
    std::cout<<std::flush;
}
