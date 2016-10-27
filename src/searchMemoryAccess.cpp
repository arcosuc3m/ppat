template < typename T>
void MyASTVisitor::searchMemAcc(Stmt *s,T &Loop){
	if(s!=nullptr){
		bool found = false;
//		s->dump(i);
// WORK IN PROGRESS
		if(isa<ArraySubscriptExpr>(s)){
			ASTMemoryAccess newMemAcc;
			if(searchArrayAcc(s,Loop,newMemAcc)){
				Loop.MemAccess.push_back(newMemAcc);
			}			
			return;
			//found = true;
		}
//------------------
		if(isa<CXXOperatorCallExpr>(s)){
			 found = searchMemOperator(s,Loop);
		}
		
//		if(s->children().begin() != s->children().end()){
		
		if(s!=nullptr){
			Stmt::child_iterator check;
			for(Stmt::child_iterator child = s->child_begin();child!=s->child_end()&&!found;child++){
				MyASTVisitor::searchMemAcc(*child,Loop);
		     	}
		}
		
	}
}
/*
template < typename T>
bool MyASTVisitor::searchArrayAcc(Stmt *s,T &Loop, ASTMemoryAccess &newMemAcc){
        //child = operator
        //child 2 = iterator
        Stmt::child_iterator child2 = s->child_begin();
        child2++;
        Stmt::child_iterator child = s->child_begin();
        bool valid= true;
        if(*child!=nullptr && *child2 != nullptr){
                if(isa<ImplicitCastExpr>(*child)){
                         ImplicitCastExpr * imCast = cast<ImplicitCastExpr>((*child));
                         if(isa<DeclRefExpr>(*(imCast->child_begin()))){
                                //It is the array variable
                                DeclRefExpr *ref = cast<DeclRefExpr>(*imCast->child_begin());
                                if(ref->getDecl()!=nullptr){
                                        if(isa<VarDecl>(ref->getDecl())){
                                                if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
                                                        newMemAcc.Name =  var->getNameAsString();
                                                        newMemAcc.RefLoc = ref->getLocStart().getRawEncoding();
                                                }
                                        }
                                }

                         }else{
                                if(isa<ArraySubscriptExpr>(*(imCast->child_begin()))){
                                //Multidimensional array
                                        valid &= searchArrayAcc(*(imCast->child_begin()),Loop,newMemAcc);
                                }else{
                                        //Not valid for now
                                        valid = false;
                                }

                        }
                }
                if(isa<ImplicitCastExpr>(*child2)){
                        ImplicitCastExpr * imCast = cast<ImplicitCastExpr>((*child));
                        for(Stmt::child_iterator imchild = imCast->child_begin();imchild!=imCast->child_end();imchild++){
                               MyASTVisitor::getIterator(*imchild,Loop,newMemAcc);
                        }
                }
        }
        return valid;

}
*/
template < typename T>
bool MyASTVisitor::searchArrayAcc(Stmt *s,T &Loop, ASTMemoryAccess &newMemAcc){
	//child = operator
	//child 2 = iterator
	Stmt::child_iterator child2 = s->child_begin();
	child2++;
	Stmt::child_iterator child = s->child_begin();
	bool valid= true;	
	if(*child!=nullptr && *child2 != nullptr){
		if(isa<ImplicitCastExpr>(*child)){
			 ImplicitCastExpr * imCast = cast<ImplicitCastExpr>((*child));
			 if(isa<DeclRefExpr>(*(imCast->child_begin()))){
				//It is the array variable
                        	DeclRefExpr *ref = cast<DeclRefExpr>(*imCast->child_begin());
	                        if(ref->getDecl()!=nullptr){
        	                        if(isa<VarDecl>(ref->getDecl())){
                	                        if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
                        	                        newMemAcc.Name =  var->getNameAsString();
                                	                newMemAcc.RefLoc = ref->getLocStart().getRawEncoding();
	                                        }
	                                }
	                        }
				
			 }else{
			 	if(isa<ArraySubscriptExpr>(*(imCast->child_begin()))){
				//Multidimensional array
					valid &= searchArrayAcc(*(imCast->child_begin()),Loop,newMemAcc);
			 	}else{
					//Member array 
					if(isa<MemberExpr>(*(imCast->child_begin()))){
						MemberExpr *expr = cast<MemberExpr>(*(imCast->child_begin()));
						bool found = false;
						Stmt::child_iterator children = expr->child_begin();
						//Look for data structure
						//s->dump();
						if(*children != nullptr){
						
						//children->dump();
						while(!found && *children != nullptr){
							if(isa<DeclRefExpr>(*children)){
								 DeclRefExpr *ref = cast<DeclRefExpr>(*children);
								 if(ref->getDecl()!=NULL){
        		                                                if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
										newMemAcc.Name =  var->getNameAsString();
			                                                        newMemAcc.RefLoc = ref->getLocStart().getRawEncoding();
										newMemAcc.dimension = 0;
									}
								}
								found = true;
							}else{
								if(isa<CXXThisExpr>(*children)){
									valid = false;
									break;
								}
								children = children->child_begin();
							}		
						}
						if(!found){
							valid = false;
						}		
						}
					}else{
						//Not valid for now
						valid = false;			
					}
				}
	
			}
		}
	//	if(isa<ImplicitCastExpr>(*child2)){
	//		ImplicitCastExpr * imCast = cast<ImplicitCastExpr>((*child));
//	        for(Stmt::child_iterator child = ->child_begin();imchild!=imCast->child_end();imchild++){
      	        MyASTVisitor::getIterator(*child2,Loop,newMemAcc);
//	        }
	//	}
		return valid;
	}
	return valid;

}
/*
template < typename T>
void MyASTVisitor::searchArrayAcc(Stmt *s,T &Loop){
	if(isa<DeclRefExpr>(s)){
		DeclRefExpr *ref = cast<DeclRefExpr>(s);
                 //if is a variable reference
                 if(ref->getDecl()!=NULL){
                        if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
				llvm::errs()<<var->getNameAsString();
			}
		}
	}else{
		searchArrayAcc(*(s->child_begin()),Loop);
	}
}
*/
template <typename T>
bool  MyASTVisitor::searchMemOperator(Stmt *s, T &Loop){
	if(isa<CXXOperatorCallExpr>(s)){
		auto cxxOp = cast<CXXOperatorCallExpr>(s);
		Decl *decl = cxxOp->getCalleeDecl();
		if(decl != nullptr){
			if(isa<CXXMethodDecl>(decl)){
				auto *fun = cast<clang::CXXMethodDecl>(decl);
				if(fun->getNameAsString().find("operator[]")!=std::string::npos){
					//IS PROBABLY A MEMORY ACCESS
				//	llvm::errs()<<"MEMORY ACCESS ";
					ASTMemoryAccess newMemAcc;
					int numArgs = cxxOp->getNumArgs();
					int dimension = 0;
	                                for(int i=0;i<numArgs;i++){
						Expr *arg = cxxOp->getArg(i);
                                                //Stmt *stmt = cast<Stmt>(arg);
						//nt dimension = 0;
						if(i==1) getIterator(arg,Loop,newMemAcc);
						if(i==0) getOperator(arg,Loop,newMemAcc,dimension);
					}
					newMemAcc.dimension = dimension;
					Loop.MemAccess.push_back(newMemAcc);
					return true;
				
				}
			}	
		}
	}
	return false;
}


template <typename T>
bool MyASTVisitor::getOperator(Stmt *s, T &Loop, ASTMemoryAccess &newMemAcc, int &dimension){
	if(s!=nullptr){
		if(isa<CXXOperatorCallExpr>(s)){
		dimension++;
                auto cxxOp = cast<CXXOperatorCallExpr>(s);
                Decl *decl = cxxOp->getCalleeDecl();
                if(decl != nullptr){
                        if(isa<CXXMethodDecl>(decl)){
                                auto *fun = cast<clang::CXXMethodDecl>(decl);
                                if(fun->getNameAsString().find("operator[]")!=std::string::npos){
                                        //IS PROBABLY A MEMORY ACCESS
                                  //      llvm::errs()<<"MEMORY SUB ACCESS";
                                 //       ASTMemoryAccess newMemAcc;
                                        int numArgs = cxxOp->getNumArgs();
                                        for(int i=0;i<numArgs;i++){
                                                Expr *arg = cxxOp->getArg(i);
                                                //Stmt *stmt = cast<Stmt>(arg);
                                                if(i==1) getIterator(arg,Loop,newMemAcc);
                                                if(i==0) getOperator(arg,Loop,newMemAcc,dimension);
                                        }
                                        return true;

                                }
                        }
                }
		
        	}
		if(isa<DeclRefExpr>(s)){
		//	llvm::errs()<<"OPERATOR\n";
			DeclRefExpr *ref = cast<DeclRefExpr>(s);
                        if(ref->getDecl()!=nullptr){
                                if(isa<VarDecl>(ref->getDecl())){
					if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
						newMemAcc.Name =  var->getNameAsString();
						newMemAcc.RefLoc = ref->getLocStart().getRawEncoding();
        		        	        return true;
					}
				}
			}

		}
		for(Stmt::child_iterator child = s->child_begin();child!=s->child_end();child++){
                        MyASTVisitor::getIterator(*child,Loop,newMemAcc);
                }
	}

	return false;
}

template <typename T>
bool MyASTVisitor::getIterator(Stmt *s, T &Loop, ASTMemoryAccess &newMemAcc){
	if(s!=nullptr){
		if(isa<DeclRefExpr>(s)){
			DeclRefExpr *ref = cast<DeclRefExpr>(s);
			if(ref->getDecl()!=nullptr){
				if(isa<VarDecl>(ref->getDecl())){
                                        if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
                                                newMemAcc.index.push_back(var->getNameAsString());
                                                return true;
                                        }
				}
			}
		}
		if(isa<IntegerLiteral>(s)){
			IntegerLiteral *lit = cast<IntegerLiteral>(s);
			std::stringstream ss;
			ss<< lit->getValue().getSExtValue();
			newMemAcc.index.push_back(ss.str());
			return true;
		}
		for(Stmt::child_iterator child = s->child_begin();child!=s->child_end();child++){
	                MyASTVisitor::getIterator(*child,Loop, newMemAcc);
                }
	}

	return false;

}
