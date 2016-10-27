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
         if( (*var).lvalue && (*var).RefLoc >= fWrite.getRawEncoding() 
              && lWrite.getRawEncoding() > (*var).RefLoc && !isLocal ){ 
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
        std::cout<<"GENERATION VARIABLE "<< (*cond).Name<<"\n";
        if((*cond).lvalue){
            if(isBefore((*cond).RefSourceLoc, fWrite)) { fWrite = (*cond).RefSourceLoc; }
            if(isBefore(lWrite, (*cond).RefSourceLoc)) lWrite = (*cond).RefSourceLoc;
        }
     }

     return streamGenerationRefs; 
}

bool MyASTVisitor::getStreamGeneration(int i){
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
            if(isBefore(firstWrite, (*cond).RefSourceLoc)) { firstWrite = (*cond).RefSourceLoc; }
            if(isBefore((*cond).RefSourceLoc,lastWrite)) lastWrite = (*cond).RefSourceLoc;
        }
     }

     SourceLocation fWrite = firstWrite;
     SourceLocation lWrite = lastWrite;
     do{
         firstWrite = fWrite;
         lastWrite = lWrite;
         bool endStmt = true;
         for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
            if( isBefore(lWrite, (*stmt).getBegin())){ lWrite = (*stmt).getBegin() ; endStmt = false; break;  }
         }
         for(auto stmt = Loops[i].stmtLoc.end()-1 ; stmt != Loops[i].stmtLoc.begin()-1; stmt--){
            if( isBefore( (*stmt).getEnd(), fWrite)){ fWrite = (*(stmt+1)).getBegin(); break;}
         }
         if(endStmt) lWrite = Loops[i].RangeLoc.getEnd();
         condition_variables = getOtherGenerations(i, fWrite, lWrite);
     }while(fWrite.getRawEncoding() != firstWrite.getRawEncoding() && lWrite.getRawEncoding() != lastWrite.getRawEncoding());

     firstWrite = fWrite;
     lastWrite = lWrite;
     
     bool before = false;
     bool after = false;
     if(firstWrite == Loops[i].bodyRange.getBegin() && lastWrite == Loops[i].RangeLoc.getEnd()) return true;
     for(auto cond = condition_variables.begin(); cond != condition_variables.end(); cond++){
        if(isBefore( (*cond).RefSourceLoc, firstWrite)) before = true;
        if(isBefore( lastWrite, (*cond).RefSourceLoc)) after = true;
     }

     

     Loops[i].genBefore = before; 
     Loops[i].genAfter = after; 
  //   if(before){
  //     Loops[i].genStart = Loops[i].bodyRange.getBegin();
  //     Loops[i].genEnd = lastWrite;
  //   }
  //   if(after){  
     Loops[i].genStart = firstWrite;
     Loops[i].genEnd = lastWrite;
  //   }

     if(after&&before){ Loops[i].genStart = Loops[i].bodyRange.getBegin(); Loops[i].genEnd = Loops[i].bodyRange.getBegin();}
     if(!after&&!before){Loops[i].genStart = Loops[i].bodyRange.getBegin(); Loops[i].genEnd = Loops[i].bodyRange.getBegin();}

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
                        
                bool valid = getStreamGeneration(i);        

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
		        
        		bool empty = false;	
        		if(Loops[i].numOps == 0 ) empty = true;
//	            if(Loops[i].genStart == Loops[i].RangeLoc.getBegin() && Loops[i].genEnd == Loops[i].RangeLoc.getEnd() && !valid) empty = true;
 	
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
                        auto line = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
                        SSBefore<< "== INVALID PATTERN == Invalid farm on "<<fileName<<":"<<line<<"\n";
                        if(feedback) SSBefore<<"\tFeedback.\n ";
                        if(globallvalue) SSBefore<<"\tWrite on a global variable.\n";
            			if(hasreturn) SSBefore<<"\tUses return statement.\n";
            			if(empty) SSBefore<<"\tEmpty loop.\n";
            			if(hasbreak) SSBefore<<"\tUses break statement.\n";
            			if(hasgoto) SSBefore<<"\tUses goto statement.\n";


            			std::cout<<SSBefore.str()<<std::endl;

                }
		}
	}
	std::cout<<"FARMS FOUND: "<<numFarm<<"\n";
}
