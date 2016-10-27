/**
	\fn void getMapIO(std::vector<std::string>& inputs,std::vector<std::string>& outputs,int i)
	
	Analyze each variable reference and extract inputs and outputs for the 
	possible Map.

	\param[out]	inputs	vector of input variables
	\param[out]	outputs	vector of output variables
	\param[in]	i	Loop to be analyzed 

*/
void MyASTVisitor::getMapIO(std::vector<std::string>& inputs,std::vector<std::string>& outputs,int i){
	for(auto refs = Loops[i].VarRef.begin();refs != Loops[i].VarRef.end();refs++){
		bool isLocal = false;
		for(auto decl = Loops[i].VarDecl.begin(); decl != Loops[i].VarDecl.end(); decl++){
			if((*refs).Name == (*decl).Name){
				isLocal = true;
			}
		}
		for(auto assign = Loops[i].Assign.begin(); assign != Loops[i].Assign.end(); assign++){
			if((*assign).Name == (*refs).Name && (*assign).RefLoc == (*refs).RefLoc){
				(*refs).lvalue = true;
				(*refs).rvalue = false;
			}
		}
		if((*refs).lvalue && !isLocal){
			bool add = true;
			for(unsigned j=0;j<outputs.size();j++){
				if(outputs[j]==(*refs).Name)
					add = false;
			}
			if(add)
				outputs.push_back((*refs).Name);

			add = true;
			for(unsigned j=0;j<inputs.size();j++){
                if(inputs[j]==(*refs).Name)
	                  add = false;
            }
			if(add){
				inputs.push_back((*refs).Name);
            }

		}
		if((*refs).rvalue && !isLocal){
			bool add = true;
                 for(unsigned j=0;j<inputs.size();j++){
                    if(inputs[j]==(*refs).Name)
                        add = false;
                 }
			//If is write before read.
			 for(unsigned j=0;j<outputs.size();j++){
                 if(outputs[j]==(*refs).Name)
                      add = false;
                }
                if(add){
				   inputs.push_back((*refs).Name);
                }
           }
	}
}


void MyASTVisitor::getMapIO(std::vector<std::string>& inputs, std::vector<std::string>& inputTypes, std::vector<std::string>& outputs,std::vector<std::string>& outputTypes, int i){
    for(auto refs = Loops[i].VarRef.begin();refs != Loops[i].VarRef.end();refs++){
        bool isLocal = false;
        for(auto decl = Loops[i].VarDecl.begin(); decl != Loops[i].VarDecl.end(); decl++){
            if((*refs).Name == (*decl).Name){
                isLocal = true;
            }
        }
        for(auto assign = Loops[i].Assign.begin(); assign != Loops[i].Assign.end(); assign++){
            if((*assign).Name == (*refs).Name && (*assign).RefLoc == (*refs).RefLoc){
                (*refs).lvalue = true;
                (*refs).rvalue = false;
            }
        }
        if((*refs).lvalue && !isLocal){
            bool add = true;
            for(unsigned j=0;j<outputs.size();j++){
                if(outputs[j]==(*refs).Name)
                    add = false;
            }
            if(add){
                outputs.push_back((*refs).Name);
                outputTypes.push_back((*refs).Name);
            
            }

            add = true;
            for(unsigned j=0;j<inputs.size();j++){
                if(inputs[j]==(*refs).Name)
                      add = false;
            }
            if(add){
                inputs.push_back((*refs).Name);
                inputTypes.push_back((*refs).type);
            }

        }
        if((*refs).rvalue && !isLocal){
            bool add = true;
                 for(unsigned j=0;j<inputs.size();j++){
                    if(inputs[j]==(*refs).Name)
                        add = false;
                 }
            //If is write before read.
             for(unsigned j=0;j<outputs.size();j++){
                 if(outputs[j]==(*refs).Name)
                      add = false;
                }
                if(add){
                   inputs.push_back((*refs).Name);
                   inputTypes.push_back((*refs).type);
                }
           }
    }
}

/**
	\fn void getDataStreams(std::vector<std::string>& ioVector, std::vector<std::string>& dataStreams, int i)

	Get inputs or outputs that are a data stream type. (i.e. vector, array ... )

	\param[in]	ioVector	Vector of input or output variables
	\param[out]	dataStreams	Vecot of extracted i/o data streams
	\param[in] 	i	Loop to be analyzed
*/
void MyASTVisitor::getDataStreams(std::vector<std::string>& ioVector, std::vector<std::string>& dataStreams, int i){
	for(unsigned io = 0; io<ioVector.size();io++){
		for(unsigned mem = 0; mem<Loops[i].MemAccess.size(); mem++){
			if(ioVector[io] == Loops[i].MemAccess[mem].Name){
				bool add = true;
	                        for(unsigned j=0;j<dataStreams.size();j++){
	                                if(dataStreams[j]==ioVector[io])
	                                        add = false;
	                        }
	                        if(add)
					dataStreams.push_back(ioVector[io]);
			}
		}
	}
}


/**
	bool checkPrivate(int i, ASTVar variable)
	
	This function checks if a variable should be declared as private.	

	\param[in]	i	 	Loop to be analyzed
	\param[in]	variable	Varaible to be analyzed

*/
bool MyASTVisitor::checkPrivate(int i, ASTVar variable){
	bool firstOcurr = true;
	SourceManager &SM = TheRewriter.getSourceMgr();

	//Look for the variable
	for(auto refs = Loops[i].VarRef.begin();refs != (Loops[i].VarRef.end()-1)&&firstOcurr;refs++){
		if(variable.Name == (*refs).Name&&
		   variable.DeclLoc == (*refs).DeclLoc){
		      //If it is the iterartor
		      if((*refs).Name == Loops[i].Iterator.Name){
			 return true;
		      }
		      //If it is an internal iterator
		      for(unsigned k = 0; k<Loops.size(); k++){
			if(i!=k && Loops[i].RangeLoc.getBegin().getRawEncoding()<= Loops[k].RangeLoc.getBegin().getRawEncoding()&&
			   Loops[i].RangeLoc.getEnd().getRawEncoding()>=Loops[k].RangeLoc.getEnd().getRawEncoding()){
				if((*refs).Name == Loops[k].Iterator.Name){ 
					return true;
				}
			   }
		      }

		      firstOcurr=false;
		      if((*refs).lvalue&&!(*refs).cmpAssign){
			   
			   auto refs2 = refs+1;
			   auto loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
                           auto loc2 = SM.getDecomposedLoc((*refs).RefSourceLoc);
                           auto line1 = SM.getLineNumber(loc1.first,(*refs2).RefLoc);
                           auto line2 = SM.getLineNumber(loc2.first,(*refs).RefLoc);
			   while(line1==line2){
 				if((*refs2).Name == (*refs).Name &&
				   (*refs2).DeclLoc == (*refs).DeclLoc &&
				   (*refs2).RefLoc != (*refs).RefLoc && 
				   (*refs2).rvalue){
					return false;				
				}
				refs2++;
				if(refs2<Loops[i].VarRef.end()){
					loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
		                        loc2 = SM.getDecomposedLoc((*refs).RefSourceLoc);
			                line1 = SM.getLineNumber(loc1.first,(*refs2).RefLoc);
		                        line2 = SM.getLineNumber(loc2.first,(*refs).RefLoc);
				}else{
					line1=0;
					line2=1;
				}
			   }
			   for(unsigned codeRef = 0; codeRef < Code.VarRef.size(); codeRef++){
				if(Code.VarRef[codeRef].Name == variable.Name && variable.DeclLoc == Code.VarRef[codeRef].DeclLoc &&
				Code.VarRef[codeRef].RefLoc > Loops[i].RangeLoc.getEnd().getRawEncoding()){
					if(Code.VarRef[codeRef].lvalue) return true;
					else if(Code.VarRef[codeRef].rvalue) return false;
					else return true;
				}
			   }
			   return true;
		      }else{
			return false;
		      }
	       }
	}
	return false;
}







/**
	bool checkFeedback(int i, std::stringstream &SSComments, bool &comments)
	
	This function checks if exists any feedback in the loop "i". If exists feedback
	return true.

	RULE: On map functions must not exists any feedback

	\param[in]	i	Loop to be analyzed
	\param[out]	SSComments	Comments about the feedback
	\param[out]	comments	bool to check if exists comments 

*/
bool MyASTVisitor::checkFeedback(int i, std::stringstream &SSComments, bool &comments){
                bool feedback = false;
                //bool firstPosFeed = true;
            for(auto refs = (Loops[i].VarRef.end()-1);refs != (Loops[i].VarRef.begin()-1);refs--){
                  if((*refs).RefLoc<Loops[i].genStart.getRawEncoding()||(*refs).RefLoc>Loops[i].genEnd.getRawEncoding()){
			      bool memoryAccess = false;
			      int mem= 0;
                       for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
                              if((*refs).Name == Loops[i].MemAccess[memAcc].Name&&
                              (*refs).RefLoc == Loops[i].MemAccess[memAcc].RefLoc){
                                 memoryAccess = true;
                                 mem=memAcc;
                             }
                        }



			bool feedbackVAR=false;
			bool local = false;
                        for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
                             if(Loops[i].VarDecl[dec].Name == (*refs).Name
                               && Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc){
                                   local=true;
                             }
                        }
			//If exists a compound assignement, also exists feedback 
                        bool privated=false;
                        if(!local&&!memoryAccess){
                           privated = checkPrivate(i, (*refs));
                        }

                        if(privated){
                                bool add = true;
                                for(unsigned pv = 0; pv<Loops[i].privated.size(); pv++){
                                        if((*refs).Name == Loops[i].privated[pv]) add = false;
                                }
                                if(add) Loops[i].privated.push_back((*refs).Name);
                        }

                        for(unsigned pv = 0; pv<Loops[i].privated.size(); pv++){
                              if((*refs).Name == Loops[i].privated[pv]) privated = true;
                        }

			if(privated){
				feedbackVAR= false;
				std::cout<<"PRIVATED "<<(*refs).Name<<"\n";
			}else{
				std::cout<<"NO PRIVATED "<<(*refs).Name<<"\n";

			}

                        if((*refs).cmpAssign&&!memoryAccess&&!local&&!privated) {
                                feedbackVAR= true;
				feedback=true;
                        }

			if((*refs).opModifier&&!memoryAccess&&!privated){
				bool localVar = false;
                                for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
                                      if(Loops[i].VarDecl[dec].Name == (*refs).Name
                                        && Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc){
                                          localVar=true;
                                      }
                                }
				if(!localVar){
					feedbackVAR=true;
			//		feedback= true;
				}
			}

			if((*refs).lvalue&&!privated&&!local){
				for(auto refs2 = (Loops[i].VarRef.end()-1); refs2 != (Loops[i].VarRef.begin()-1)  ;refs2--){
					if((*refs).Name == (*refs2).Name && (*refs).DeclLoc == (*refs).DeclLoc && ((*refs2).RefLoc<Loops[i].genStart.getRawEncoding()||(*refs2).RefLoc>Loops[i].genEnd.getRawEncoding()) ){
						/*bool memoryAccess = false;
                                                for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
                                                        if((*refs).Name == Loops[i].MemAccess[memAcc].Name&&
                                                          (*refs).RefLoc == Loops[i].MemAccess[memAcc].RefLoc){
                                                                memoryAccess = true;
								mem=memAcc;
                                                          }
                                                }*/
                                                bool memoryAccess2 = false;
                                                int mem2 = 0;
                                                for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
                                                        if((*refs2).Name == Loops[i].MemAccess[memAcc].Name &&
                                                          (*refs2).RefLoc == Loops[i].MemAccess[memAcc].RefLoc){
                                                                 memoryAccess2 = true;
                                                                 mem2=memAcc;
                                                          }
                                                }
                                                if(memoryAccess&&memoryAccess2){
                                                //      llvm::errs()<<"DOUBLE MEM REFERENCE";
                                                       // if((*refs2).lvalue){
                                                        feedbackVAR = false;
                                                       // }i
						 if((*refs2).lvalue || (*refs).lvalue){
                                                                if(Loops[i].MemAccess[mem].index.size()==Loops[i].MemAccess[mem2].index.size()){
                                                                        feedbackVAR = false;
                                                                        for(unsigned index=0;index<Loops[i].MemAccess[mem].index.size();index++){
                                                                                if(Loops[i].MemAccess[mem].index[index] !=
                                                                                        Loops[i].MemAccess[mem2].index[index]){
                                                                                        feedbackVAR = true;
									//		feedback = true;
                                                                                }
                                                                        }

                                                                }else{

                                                                        if(Loops[i].MemAccess[mem].dimension!=Loops[i].MemAccess[mem2].dimension){
                                                                                feedbackVAR = false;
									//	feedback = false;

                                                                        }else{
                                                                                feedbackVAR = true;
									//	feedback = true;
                                                                        }
                                                                }
                                                       }


							/*
                                                        if((*refs2).rvalue){
                                                                if(Loops[i].MemAccess[mem].index.size()==Loops[i].MemAccess[mem2].index.size()){
                                                                        feedbackVAR = true;
                                                                        for(unsigned index=0;index<Loops[i].MemAccess[mem].index.size();index++){
                                                                                if(Loops[i].MemAccess[mem].index[index] !=
                                                                                        Loops[i].MemAccess[mem2].index[index]){
                                                                                        feedbackVAR = false;
                                                                                }
                                                                        }

                                                                }
                                                        }
							*/
                                                }
                                                if(refs==refs2&&(*refs).cmpAssign&&!memoryAccess){
                                                        feedbackVAR = true;
                                                }

						//Check if is a loop-local variable
   					        bool localVar = false;
                                                for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
                                                        if(Loops[i].VarDecl[dec].Name == (*refs).Name
                                                         && Loops[i].VarDecl[dec].DeclLoc == (*refs).DeclLoc){
                                                                localVar=true;
                                                        }
                                                }
						//if it's not local and is wrote then could produce side effects
						if(!localVar&&(*refs).lvalue&&!memoryAccess){
                                                        feedbackVAR = true;
                                                }

						if((*refs2).lvalue&!memoryAccess&&localVar){
                                                        feedbackVAR = false;
                                                        //CHECK LINE
                                                        //If is in a for declaration.
                                                        bool inFor = false;
                                                        for(unsigned loop = 0; loop<Loops[i].InternLoops.size();loop++){
                                                                if((*refs).RefLoc>Loops[i].InternLoops[loop].getBegin().getRawEncoding()&&
                                                                (*refs).RefLoc<Loops[i].BodyStarts[loop].getRawEncoding()){
                                                                        inFor=true;
                                                                }
                                                        }
                                                        SourceManager &SM = TheRewriter.getSourceMgr();
                                                        auto ref3 = refs2+1;

                                                        if(ref3!=Loops[i].VarRef.end()&&!inFor&&!memoryAccess&&!localVar){
                                                                auto loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
                                                                auto loc2 = SM.getDecomposedLoc((*ref3).RefSourceLoc);
                                                                auto line1 = SM.getLineNumber(loc1.first,(*refs2).RefLoc);
                                                                auto line2 = SM.getLineNumber(loc2.first,(*ref3).RefLoc);
                                                                while(line1==line2){
                                                                        if((*refs2).Name == (*ref3).Name && (*ref3).rvalue){
                                                                                feedbackVAR =true;
                                                                }
                                                                ref3++;
                                                                if(ref3!=Loops[i].VarRef.end()){
                                                                         loc1 = SM.getDecomposedLoc((*refs2).RefSourceLoc);
                                                                        loc2 = SM.getDecomposedLoc((*ref3).RefSourceLoc);
                                                                        line1 = SM.getLineNumber(loc1.first,(*refs2).RefLoc);
                                                                        line2 = SM.getLineNumber(loc2.first,(*ref3).RefLoc);
                                                                }else{
                                                                          line2++;
                                                                }
                                                                }
                                                         }
                                                        //--------------------
                                                }
						if((*refs2).rvalue&&!memoryAccess){
                                                        feedbackVAR=true;
                                                        //posFeed = false;
                                                        //If is an L/Rvalue varible set as possible feedback
                                                        if((*refs2).lvalue&&!(*refs2).cmpAssign){
                                                                feedbackVAR=false;
                                                        //        posFeed = true;
                                                       //         comments = true;
                                                        }
                                                }
					}
				}
			}
			if(feedbackVAR){
                                feedback=true;
                                SSComments<<"//Feedback: "<<(*refs).Name;
                                SSComments<<"\n";
				std::cout<<"//Feedback: "<<(*refs).Name<<std::endl;

                        }

		}
       }
		return feedback;
}

/**
	\fn void getStreamDependencies(std::vector<std::string> stream, int i)

	This function check the dependencies between out data stream variables
	and retrieve the list of each dependency.

	\param[in]	stream	Vector of data stream to be checked
	\param[in] 	i	Loop to be analyzed

*/
void MyASTVisitor::getStreamDependencies(std::vector<std::string> stream, int i){
	std::vector<bool> found (stream.size());
	//For each data stream
	for(unsigned st = 0; st<stream.size();st++){
		found[st] = false;
		//Look for the last reference
		for(auto refs = (Loops[i].Assign.end()-1);refs != (Loops[i].Assign.begin()-1);refs--){
			//If is a data stream refrence...
 			if(stream[st] == (*refs).Name && !found[st]){
				found[st] = true;
				//...Then look for the dependencies references and include them as a dependencies of the stream 
				for(auto refs2 = refs;refs2 != (Loops[i].Assign.begin()-1);refs2--){
					for(unsigned dep = 0 ; dep < (*refs).dependencies.size();dep++){
						//If refs2 is a dependency of stream 
						if((*refs2).Name==(*refs).dependencies[dep].Name
						   /*&& (*refs2).DeclLoc==(*refs).dependencies[dep].DeclLoc*/){
							//Add every dependecies on the stream dependencies
							for(unsigned k = 0;k<(*refs2).dependencies.size();k++){
								bool add = true;
								for(unsigned dep2 = 0;dep2< (*refs).dependencies.size();dep2++){
									if((*refs).dependencies[dep2].Name == (*refs2).dependencies[k].Name
									/*&& (*refs).dependencies[dep2].DeclLoc == (*refs2).dependencies[k].DeclLoc*/){
										add = false;
									}
								}
								if(add)
									(*refs).dependencies.push_back((*refs2).dependencies[k]);
							}

						}
					}
				}
				//DEBUG--------
			/*	llvm::errs()<<(*refs).Name<<"DEPENDENCIES:\n";
				for(int dep = 0 ; dep < (*refs).dependencies.size();dep++){
					llvm::errs()<<" "<<(*refs).dependencies[dep].Name;
				}
				llvm::errs()<<"\n";*/
				//----END DEBUG
			}
			   //DEBUG--------
                               /* llvm::errs()<<(*refs).Name<<" DEPENDENCIES:\n";
                                for(int dep = 0 ; dep < (*refs).dependencies.size();dep++){
                                        llvm::errs()<<" "<<(*refs).dependencies[dep].Name;
                                }
                                llvm::errs()<<"\n";*/
                                //----END DEBUG
		
		}
	}
}
/**
	\fn bool checkMapDependencies(std::vector<std::string> instream, std::vector<std::string> outstream, int i)

	This function checks the dependencies between input data streams and output
	data streams. If the loops is a map, dependencies between input an output 
	data streams are necesary and one output should depends on only one 
	input data stream. 

	RULE:	Relatioship between input data stream and output data stream 1 to 1

	If the loop does not have input or output is not a map and this function will return
	false. If an output does not depend on any input, this dunction will return false.
	If an output depends on more than one input, this function will return false. If
	every output depends on only one input, this function will return true.

	\param[in]	instream	Vector of input data streams
	\param[in]	outstream	Vector of output data streams
	\param[in]	i	Loop to be analyzed
	
*/
bool MyASTVisitor::checkMapDependencies(std::vector<std::string> instream, std::vector<std::string> outstream, int i){
	bool invalid = false;
        std::vector<bool> found (outstream.size());

	//If doesnt have any input or output then its not a Map
	if(outstream.size() == 0 || instream.size() == 0){
		invalid = true;
	}
	//Check relationship between input and output
	for(unsigned st=0;st<outstream.size();st++){
//		bool found = false;
		bool used = false;
		//bool invalid = false;
		for(auto refs = (Loops[i].Assign.end()-1);refs != (Loops[i].Assign.begin()-1);refs--){
			if(outstream[st] == (*refs).Name && !found[st]){
                                found[st] = true;
				for(unsigned dep = 0 ; dep < (*refs).dependencies.size();dep++){
					
					for(unsigned ist=0;ist<instream.size();ist++){
						if(instream[ist] == (*refs).dependencies[dep].Name){
							//If not previously relate with other input
							if(!used){
								//llvm::errs()<<"USED";
								used = true;
							
							//If has relationship with more than one input stream
							}else{
								//llvm::errs()<<"INVALID";
								invalid = true;
							}
						}
					}
				}
			}
		}
		//If output is not relate to any input then is not a Map
		if(!used)	
			invalid = true;
	
	}
	return invalid;
}

/**
	\fn bool checkMemAccess(std::vector<std::string> dataStreams,int i)

	This function returns false if any data stream variable is used in more than one position.
	
	RULE: Output only can be accessed on only one position to be a map.

	\param[in]	dataStreams	Data stream varaibles name
	\param[in]	i	Loop to be analized

*/
bool MyASTVisitor::checkMemAccess(std::vector<std::string> dataStreams,int i){
	//For each data stream variable
	for(unsigned data=0;data<dataStreams.size();data++){
		bool notFound = true;
		//Search memory access
		for(unsigned memAcc = 0; memAcc < Loops[i].MemAccess.size()&&notFound; memAcc++){
			if(dataStreams[data] == Loops[i].MemAccess[memAcc].Name){
				//Check if every access is on the same position. If not return false.
				for(unsigned memAcc2 = 0; memAcc2 < Loops[i].MemAccess.size()&&notFound; memAcc2++){
				
					if(Loops[i].MemAccess[memAcc].index.size()!=Loops[i].MemAccess[memAcc2].index.size()){
						return false;
					}else{
						for(unsigned in= 0; in<Loops[i].MemAccess[memAcc].index.size(); in++){
							if(Loops[i].MemAccess[memAcc].index[in] != Loops[i].MemAccess[memAcc2].index[in])
								return false;
						}
					}
				}
			}
		}	
	}
	return true;
}


bool MyASTVisitor::globalLValue(int i){
    SourceManager &sm = TheRewriter.getSourceMgr();
    clang::LangOptions lopt;
    clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);

	//Check if no global simple variable is wrote
	for(auto refs = Loops[i].VarRef.begin(); refs!= Loops[i].VarRef.end(); refs++){
        if(isBefore(Loops[i].genEnd, (*refs).RefSourceLoc) || isBefore( (*refs).RefSourceLoc, Loops[i].genStart)){ 
		bool containerVar = false;
		for(unsigned memAcc = 0; memAcc < Loops[i].MemAccess.size(); memAcc++){
                        if(refs->Name == Loops[i].MemAccess[memAcc].Name && refs->RefLoc == Loops[i].MemAccess[memAcc].RefLoc){
				containerVar = true;
			}
		}
		if(!containerVar){
			if(refs->lvalue&&refs->globalVar){ 
                std::cout<<"GLOBAL VARIABLE " <<(*refs).Name<<std::endl;
				return true;	
			}
		}		
        }
	}
	//Check if functions uses global values
	for(auto calls = Loops[i].fCalls.begin(); calls != Loops[i].fCalls.end(); calls++){
        if(isBefore( Loops[i].genEnd, (*calls).EndLoc ) || isBefore((*calls).SLoc, Loops[i].genStart)){ 
		bool found = 0;
		//Function is accesible 
		for(auto fun = Functions.begin(); fun != Functions.end(); fun++){
			if((*calls).Name == (*fun).Name){
				if((*fun).globalsUsed.size()>0){
                    std::cout<<"GLOBAL FUN" <<(*fun).Name<<std::endl;
					return true;
				}
				found = 1;
			}
		}
		//Function no accesible
		if(!found){
			for(auto f = whiteList.begin(); f != whiteList.end(); f++){
                  if((*calls).Name == (*f)){
                         found = 1;
                   }
            }
            if(!found){
                    std::cout<<"GLOBAL FUN" <<(*calls).Name<<std::endl;
				return true;
            }
		}
      }
    }
	
	return false;
	
}


bool MyASTVisitor::check_break(int i){
	bool hasbreak = false;
	for(unsigned brk=0; brk<Loops[i].Breaks.size(); brk++){
		bool isInternal = false;
		for(unsigned internal = 0; internal < Loops[i].InternLoops.size(); internal++){
			if(Loops[i].Breaks[brk].getRawEncoding() > Loops[i].InternLoops[internal].getBegin().getRawEncoding() &&
		           Loops[i].Breaks[brk].getRawEncoding() > Loops[i].InternLoops[internal].getEnd().getRawEncoding()){
				isInternal = true;
			}
		}

		if(!isInternal)
			hasbreak = true;
	}
	return hasbreak;

}

/**
	\fn bool mapDetect()
	
	This function analyze each loop on the source code to determine if any loop is a Map pattern.

*/

void MyASTVisitor::mapDetect(){
        //Analize function declarations with bodies
        analyzeCodeFunctions();
	int numMaps = 0;
        //int numpipelines = 0;
	for(int i = 0; i < numLoops;i++){
		SourceManager &SM = TheRewriter.getSourceMgr();
/*		llvm::errs()<<"HERE 2\n";

                auto lineStart = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
		llvm::errs()<<"HERE 3 \n";

                auto lineEnd = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first,Loops[i].RangeLoc.getEnd().getRawEncoding());
		llvm::errs()<<"HERE 4\n";

                auto fileName = SM.getFileEntryForID(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first)->getName();
		llvm::errs()<<"HERE 5\n";
*/
		auto fileName = Loops[i].FileName;
                auto currfile = SM.getFileEntryForID(SM.getMainFileID())->getName();
//		llvm::errs()<<"END FILE CHECKING\n";

                //Annotate only if the loop is in the current file. 
                if(fileName == currfile){

		//llvm::errs()<<"LOOP: "<<i<<"\n"; 
		std::stringstream SSComments;		
		bool comments = false;
		//Analyze function calls
		analyzeCall(i);
		//Check for L/Rvalue
                analyzeLRLoop(i);
                //Global variable LValue
                bool globallvalue = globalLValue(i);

		std::vector<std::string> outputs;
                std::vector<std::string> inputs;

		//Get inputs and outputs
		getMapIO(inputs, outputs,i);
		//DEBUG----------
/*		llvm::errs()<<"INPUTS:\n";
		for(int debug = 0;debug < inputs.size(); debug ++ ){
			llvm::errs()<<inputs[debug]<<" ";
		}
 
	        llvm::errs()<<"OUTPUTS:\n";
                for(int debug = 0;debug < outputs.size(); debug ++ ){
                        llvm::errs()<<outputs[debug]<<" ";
                }
		llvm::errs()<<"\n";*/
		//------------END DEBUG		

		
                std::vector<std::string> outStreams;
                std::vector<std::string> inStreams;

		//Get IO data streams
		getDataStreams(inputs, inStreams,i);
		getDataStreams(outputs, outStreams,i);
	
		//DEBUG
/*		llvm::errs()<<"INSTREAMS:\n";
                for(int debug = 0;debug < inStreams.size(); debug ++ ){
                        llvm::errs()<<inStreams[debug]<<" ";
                }

                llvm::errs()<<"OUTPUTSTREAMS:\n";
                for(int debug = 0;debug < outStreams.size(); debug ++ ){
                        llvm::errs()<<outStreams[debug]<<" ";
                }
                llvm::errs()<<"\n";*/
                //------------END DEBUG  
		
                //Check feedback        
                bool feedback = checkFeedback(i,SSComments,comments);

		//Get output data stream dependencies
		getStreamDependencies(outStreams,i);

		//Check if in data streams are RValue 

		//Check if input streams uses iterator related positions
	
		//Check if output streams are only used as write or read in the same position. 
		checkMemAccess(outStreams,i);	
		//TODO: To be deleted
		//Check if output depends only on one data stream input
		//This is not true 
		//bool invalid = checkMapDependencies(inStreams, outStreams,i); 
		bool invalid = true;
		if(outputs.size()>0&&inputs.size()>0){
			invalid = false;
		}
		if(Loops[i].type == 1){
			invalid =true;
		}	
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
		if(Loops[i].numOps == 0) empty=true;

//			SSBefore<<"[[rpr::map]]\n";
//                        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
 
//			llvm::errs()<<"LOOP "<<i<<" IS A MAP\n";
		if(!invalid&&!feedback&&!globallvalue&&!hasreturn&&!empty&&!hasgoto&&!hasbreak){
                        Loops[i].map = true;
                        numMaps++;
			//ADD I/O IN LOOPS
                        for(unsigned in=0;in<inputs.size();in++){
                                Loops[i].inputs.push_back(inputs[in]);
                        }
                        for(unsigned out=0;out<outputs.size();out++){
                                Loops[i].outputs.push_back(outputs[out]);
                        }

                }else{
                        std::stringstream SSBefore;
                        //SourceManager &SM = TheRewriter.getSourceMgr();
                        auto line = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
                 //       auto fileName = SM.getFileEntryForID(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first)->getName();
                        SSBefore<< "== INVALID PATTERN == Invalid map on "<<fileName<<":"<<line<<"\n";
                        if(feedback) SSBefore<<"\tFeedback\n ";
                        if(invalid)  SSBefore<<"\tOutput does not depend on any Input\n";
                        if(globallvalue) SSBefore<<"\tWrite on a global variable\n";
			if(hasreturn) SSBefore<<"\tUses return statement\n";

		}
//		else llvm::errs()<<"LOOP "<<i<<" IS NOT A MAP\n";
		}
	}
	std::cout<<"MAPS FOUND : "<<numMaps<<"\n";
}


