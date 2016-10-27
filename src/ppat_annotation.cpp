void MyASTVisitor::PPATAnnotate(){
	for(unsigned i = 0; i< Loops.size() && !grppi; i++){
		if(Loops[i].map || Loops[i].farm || Loops[i].reduce){
			std::stringstream SSBefore;
			//SourceManager &SM = TheRewriter.getSourceMgr();
//                        auto lineStart = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
  //                      auto lineEnd = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first,Loops[i].RangeLoc.getEnd().getRawEncoding());
    //                    auto fileName = SM.getFileEntryForID(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first)->getName();
			if(!omp){
				SSBefore << "[[";
				if(Loops[i].map)
					SSBefore << "rph::map";
				if(Loops[i].map&&Loops[i].farm)
					SSBefore << ", ";
				if(Loops[i].farm)
					SSBefore << "rph::farm";
				if(Loops[i].reduce)
					SSBefore << "rph::reduce";
			      if(Loops[i].inputs.size()>0){
	                           SSBefore<<", rph::in(";
	                           for(unsigned in = 0; in<Loops[i].inputs.size();in++){
	                               SSBefore<<Loops[i].inputs[in];
	                               if(in!=Loops[i].inputs.size()-1){
	                                       SSBefore<<",";
	                               }
	                           }
	                           SSBefore<<")";
	                       }
	                       if(Loops[i].outputs.size()>0){
	                                SSBefore<<", rph::out(";
	                                for(unsigned out = 0; out<Loops[i].outputs.size();out++){
	                                      SSBefore<<Loops[i].outputs[out];
	                                      if(out!=Loops[i].outputs.size()-1){
	                                          SSBefore<<",";
	                                     }
	                                }
	                                SSBefore<<")";
	                        }
				if(Loops[i].privated.size()>0){
					SSBefore<<", rph::private(";
					for(unsigned pv = 0; pv < Loops[i].privated.size(); pv++){
						SSBefore<<Loops[i].privated[pv];
						if(pv!=Loops[i].privated.size()-1){
	                                          SSBefore<<",";
	                                         }
					}
					SSBefore<<")";
				}
				if(Loops[i].reduceVar.size()>0){
					SSBefore<<", rph::reduce(";
					for(unsigned red = 0; red < Loops[i].reduceVar.size(); red++){
						SSBefore<<Loops[i].reduceVar[red];
						if(red!=Loops[i].reduceVar.size()-1){
							SSBefore<<",";
						}
					}
					SSBefore<<")";
				}
				SSBefore<<"]]\n";
				TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
		        }else{
				bool outer = true;
				for(unsigned k = 0; k<Loops.size(); k++){
        		                if(i!=k && Loops[i].RangeLoc.getBegin().getRawEncoding()>= Loops[k].RangeLoc.getBegin().getRawEncoding()&&
	                	           Loops[i].RangeLoc.getEnd().getRawEncoding()<=Loops[k].RangeLoc.getEnd().getRawEncoding()
					   &&(Loops[k].map||Loops[k].farm||Loops[k].reduce)){
						outer = false;
					}
				}
				if(outer&&Loops[i].map){
				  	SSBefore<<"#pragma omp parallel for";
				        if(Loops[i].privated.size()>0){
                                                SSBefore<<" private(";
        	                            	for(unsigned pv = 0; pv < Loops[i].privated.size(); pv++){
		                                       SSBefore<<Loops[i].privated[pv];
	        	                               if(pv!=Loops[i].privated.size()-1){
	                	                               SSBefore<<",";
		                                       }
	        	                       }
	                	               SSBefore<<")";
	                               	}
					SSBefore<<"\n";
					TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
				}
				std::stringstream SSTask;
				std::stringstream SSEnd;
				/*if(Loops[i].farm && outer && !Loops[i].map &&Loops[i].type == 1){
					SSBefore<<"#pragma omp parallel\n{\n";//OpenMP parallel section
					SSBefore<<"#pragma omp single nowait\n{\n"; //OpenMP single thread launch tasks

					SSTask<<"#pragma omp task \n{\n"; //Task section
					SSEnd<<"}\n";//End task
					
					SSEnd<<"}\n";//End single
					SSEnd<<"#pragma omp taskwait\n}\n";//Wait and end parallel section
					TheRewriter.InsertText(Loops[i].InitLoc, SSTask.str(), true,true);
					TheRewriter.InsertText(Loops[i].RangeLoc.getEnd(), SSEnd.str(), true, true);
					TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);


				}*/
				if(Loops[i].farm && outer && !Loops[i].map &&Loops[i].type == 0){
					SSBefore<<"#pragma omp parallel for";
                                        if(Loops[i].privated.size()>0){
                                                SSBefore<<" private(";
                                                for(unsigned pv = 0; pv < Loops[i].privated.size(); pv++){
                                                       SSBefore<<Loops[i].privated[pv];
                                                       if(pv!=Loops[i].privated.size()-1){
                                                               SSBefore<<",";
                                                       }
                                               }
                                               SSBefore<<")";
                                        }
                                        SSBefore<<"\n";
                                        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
				}
				if(Loops[i].reduce && outer){
					SSBefore<<"#pragma omp parallel for ";
					 if(Loops[i].privated.size()>0){
                                                SSBefore<<" private(";
                                                for(unsigned pv = 0; pv < Loops[i].privated.size(); pv++){
                                                       SSBefore<<Loops[i].privated[pv];
                                                       if(pv!=Loops[i].privated.size()-1){
                                                               SSBefore<<",";
                                                       }
                                               }
                                               SSBefore<<")";
                                        }

					for(unsigned red = 0; red < Loops[i].reduceVar.size() ; red++){
						SSBefore<<" reduction("<<Loops[i].operators[red]<<":"<<Loops[i].reduceVar[red]<<")";
					}
					SSBefore<<"\n";
					TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
				}
			}
		}
	}
}
