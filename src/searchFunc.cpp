//#include "/opt/llvm-install-3.7.0/include/clang/AST/Stmt.h"
#include "clang/AST/Stmt.h"

clang::SourceLocation MyASTVisitor::getEndLocation(clang::SourceLocation EndLoc, ASTLoop &Loop){
	
	clang::SourceManager &sm = TheRewriter.getSourceMgr();	
	auto currfile = sm.getFileEntryForID(sm.getMainFileID())->getName();
	if(currfile == Loop.FileName){
		clang::SourceLocation AuxLoc = EndLoc.getLocWithOffset(20);
		auto source = std::string(sm.getCharacterData(EndLoc),sm.getCharacterData(AuxLoc)-sm.getCharacterData(EndLoc));
		std::size_t blankfound = source.find(" ");
		std::size_t linefound = source.find("\n");
		int offset = blankfound;
		if(blankfound>linefound){
			offset = linefound;
		}
		
		return EndLoc.getLocWithOffset(offset);
	}
	return EndLoc;
}


void MyASTVisitor::searchFunc(Stmt *s,ASTLoop &Loop){
//	s->dump();
	if(s!=nullptr){
	        if(isa<ForStmt>(s)){
			
			Loop.InternLoops.push_back(s->getSourceRange());
			ForStmt * lp = cast<ForStmt>(s);
			Stmt *body = lp->getBody();
			Loop.BodyStarts.push_back(body->getLocStart());
		}
		if(isa<WhileStmt>(s)){
			Loop.InternLoops.push_back(s->getSourceRange());
			WhileStmt * lp = cast<WhileStmt>(s);
			Stmt *body = lp->getBody();
                        Loop.BodyStarts.push_back(body->getLocStart());
		}
		if(isa<SwitchStmt>(s)){
			Loop.InternLoops.push_back(s->getSourceRange());
			SwitchStmt * lp = cast<SwitchStmt>(s);
                        Stmt *body = lp->getBody();
                        Loop.BodyStarts.push_back(body->getLocStart());
		}
		if(isa<IfStmt>(s)){
			Loop.ConditionalsStatements.push_back(s->getSourceRange());
			IfStmt * ifst = cast<IfStmt>(s);
			Loop.CondEnd.push_back(getEndLocation(ifst->getLocEnd(),Loop));

		}
		if(isa<SwitchStmt>(s)){
			Loop.ConditionalsStatements.push_back(s->getSourceRange());
		}
	        if(isa<CXXMemberCallExpr>(s)){
			//s->dump();
                       // auto mem = cast<MemberExpr>(s);
                       // ValueDecl *decl = mem->getMemberDecl();
                        //llvm::errs()<<"TYPE : "<<decl->getType().getAsString();
		//	CallExpr *call = cast<CallExpr>(s);
                       // Decl *decl = call->getCalleeDecl();
			if(isa<MemberExpr>(*(s->child_begin()))){
				auto mem = cast<MemberExpr>((*s->child_begin()));
 	                        ValueDecl *Vdecl = mem->getMemberDecl();
			 	//llvm::errs()<<Vdecl->getName();	
	                        auto expr = mem->getBase();
				ASTFunction newFun;
				newFun.Name = Vdecl->getName();
				newFun.SLoc = s->getLocStart();
	                        newFun.EndLoc = s->getLocEnd();
                                /*SHOULD LOOK FOR ARGUMENTS BUT NOT TODAY*/
				Loop.fCalls.push_back(newFun);
				//searchVars(expr,Loop);
				//searchMemAcc(expr,Loop);
                                //expr->dump();
				if(Vdecl->getType().getAsString().find("const")!= std::string::npos){
					searchVarsMember(expr, Loop, true);	
				}else{
					searchVarsMember(expr,Loop,false);
				}
				 //NEED TO PERFORM SOME ACTION FOR NON CONSTANT MEMBER FUNCTIONS SINCE THEY CAN MODIFY THE OBJECT
                                        /*W.I.P.
                                                Add base member as variable since can be modified
                                                Add function into the list
                                                GOOD LUCK!
                                        */
				return;
			}
                }

		//If is a user-defined class operator
		if(isa<CXXOperatorCallExpr>(s)){
			//Access to operator call declaration
			auto cxxOp = cast<CXXOperatorCallExpr>(s);
			Decl *decl = cxxOp->getCalleeDecl();
			ASTFunction newFun;
			//Get location
			newFun.SLoc = s->getLocStart();
                        newFun.EndLoc = s->getLocEnd().getLocWithOffset(2);
                        newFun.FunLoc= cxxOp->getExprLoc().getRawEncoding();
			if(decl!=nullptr){
			//If is a Method of a class
	                        if(auto *fun = dyn_cast<clang::CXXMethodDecl>(decl)){
					CXXRecordDecl * rd = fun->getParent();
					//Get operator name 
                        	        newFun.Name = rd->getNameAsString()+"."+fun->getNameAsString();
                                	newFun.returnType = fun->getReturnType().getAsString();
					unsigned numParam = fun->getNumParams();
					//If is an operator ()
					if(fun->getNameAsString() == "operator()"){
                        	        	newFun.paramTypes.push_back(fun->getReturnType().getAsString());
                                	}
				//Get every parameter type
                                for(unsigned i = 0;i<numParam;i++){
                                        auto param = fun->getParamDecl(i);
                                        newFun.paramTypes.push_back(param->getType().getAsString());

                                }
			    	unsigned numArgs = cxxOp->getNumArgs();
				//Get call arguments

			   	for(unsigned i=0;i<numArgs;i++){
                                	Expr *arg = cxxOp->getArg(i);
                              	 	 Stmt *stmt = cast<Stmt>(arg);

                	                MyASTVisitor::getArguments(stmt,&newFun);

        	                }
				//Push on functions vector	
				Loop.fCalls.push_back(newFun);
			}
			}
//			return;
                }else{

		//If is a function call 
		if(isa<CallExpr>(s)){
			//Get call declaration
			ASTFunction newFun;
			CallExpr *call = cast<CallExpr>(s);
			Decl *decl = call->getCalleeDecl();
			//Get location
			newFun.SLoc = s->getLocStart();
			newFun.EndLoc = s->getLocEnd().getLocWithOffset(2);
			newFun.FunLoc= call->getExprLoc().getRawEncoding();
			if(decl!=nullptr){
			//If is a function

			if(FunctionDecl *fun = dyn_cast<clang::FunctionDecl>(decl)){
				//Get function name
				newFun.Name = fun->getNameAsString();
				newFun.returnType = fun->getReturnType().getAsString();
				unsigned numParam = fun->getNumParams();
				//Get parameter types
				for(unsigned i = 0;i<numParam;i++){
					auto param = fun->getParamDecl(i);
					newFun.paramTypes.push_back(param->getType().getAsString());
	
				}
                        }
			unsigned numArgs = call->getNumArgs();
			//Get Arguments
			for(unsigned i=0;i<numArgs;i++){
				Expr *arg = call->getArg(i);
				Stmt *stmt = cast<Stmt>(arg);

				MyASTVisitor::getArguments(stmt,&newFun);
			
			}
		        Loop.fCalls.push_back(newFun);
			}

		}
		}
		if(s->children().begin() != s->children().end()){
		//Seach for functions in statment childrens
      		 for(Stmt::child_iterator child = s->child_begin();child!=s->child_end();child++){
			
                	MyASTVisitor::searchFunc(*child,Loop);
        	}
		}
        }

}

void MyASTVisitor::getArguments(Stmt *s, ASTFunction* f){
	if(s!=nullptr){
	//If is a implicit cast
	if(isa<ImplicitCastExpr>(s)){
		
                ImplicitCastExpr *ice = cast<ImplicitCastExpr>(s);
		Stmt::child_iterator child = ice->child_begin();
		if(child!=ice->child_end()){
		//if cast is a function call
		 if(isa<CallExpr>(*child)){
	 		CallExpr *call = cast<CallExpr>(*child);
                	Decl *decl = call->getCalleeDecl();
			//Save argument as a function. This function call will be analyzed independently.
                	//if(FunctionDecl *fun = dyn_cast<clang::FunctionDecl>(decl)){
                	if(dyn_cast<clang::FunctionDecl>(decl)){
				ASTVar varArg;
				varArg.Name = "Function";
                                varArg.RefLoc = 0;
                                varArg.rvalue = true;
                                f->Args.push_back(varArg);

			}
	 	}
		//If is a reference 
		if(isa<DeclRefExpr>(*child)){
			
			ASTVar varArg;
                	DeclRefExpr *ref = cast<DeclRefExpr>(*child);
			//check if is a variable reference
			if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
				//Get name an location
				varArg.Name = var->getNameAsString();
				varArg.RefLoc = ref->getLocStart().getRawEncoding();
				varArg.RefSourceLoc = ref->getLocStart();//.getRawEncoding();
				//Set as rvalue
				varArg.rvalue = true;
				f->Args.push_back(varArg);
			}
		}
		//If is an array susbscript. 
	 	if(isa<ArraySubscriptExpr>(*child)){
			//Look for childrens
			auto imp = (*child)->child_begin();
			MyASTVisitor::getArguments((*imp), f);

		}	
		//If is a constructor
		if(isa<CXXConstructExpr>(*child)){
			//look for childrens
			auto next = s->child_begin();
                         MyASTVisitor::getArguments((*next), f);

		}
		if(isa<IntegerLiteral>(*child)||isa<StringLiteral>(*child)||isa<FloatingLiteral>(*child)||isa<CXXBoolLiteralExpr>(*child)){
                          ASTVar varArg;
                          varArg.Name = "Literal";
                          varArg.RefLoc = 0;
                          varArg.rvalue = true;
                          f->Args.push_back(varArg);
			
		}
		}
		return;
	}else{
	   //If is a var reference
		if(isa<DeclRefExpr>(s)){
                        ASTVar varArg;
                        DeclRefExpr *ref = cast<DeclRefExpr>(s);
                        if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
				//Get name, location and set as rvalue
                                varArg.Name = var->getNameAsString();
                                varArg.RefLoc = ref->getLocStart().getRawEncoding();
				varArg.RefSourceLoc = ref->getLocStart();//.getRawEncoding();

                                varArg.rvalue = true;
                                f->Args.push_back(varArg);
                        }
                }else{
			//If is a literal save as literal argument.
			if(isa<IntegerLiteral>(s)||isa<StringLiteral>(s)||isa<FloatingLiteral>(s)||isa<CXXBoolLiteralExpr>(s)){
				    ASTVar varArg;
				varArg.Name = "Literal";
				varArg.RefLoc = 0;
				varArg.rvalue = true;
				f->Args.push_back(varArg);
			}else{
				//If is a expresion save as expresion
				if(isa<CXXDefaultArgExpr>(s)){
					    ASTVar varArg;

					varArg.Name = "Expr";
                                	varArg.RefLoc = 0;
                        	        varArg.rvalue = true;
                	                f->Args.push_back(varArg);
		
						
				}else{	
				   if(isa<CXXTemporaryObjectExpr>(s)||isa<MemberExpr>(s)){
					ASTVar varArg;
					varArg.Name = "Expr";
                                        varArg.RefLoc = 0;
                                        varArg.rvalue = true;
                                        f->Args.push_back(varArg);
				   }else{
					/*if(isa<CXXConstructExpr>(s)){
						  ASTVar varArg;
                	                        varArg.Name = "Constructor";
        	                                varArg.RefLoc = 0;
	                                        varArg.rvalue = true;
                        	                f->Args.push_back(varArg);

					
				   	}else{*/
						//If is not any kind defined above.
						if(s->children().begin() != s->children().end()){
							auto next = s->child_begin();
				   		//Look for childrens
					
                  			   		MyASTVisitor::getArguments((*next), f);
						}else{
							 ASTVar varArg;
                        	        	        varArg.Name = "DEFAULT";
                	        	                varArg.RefLoc = 0;
        	                                	varArg.rvalue = true;
	                                       		 f->Args.push_back(varArg);

						}
					}
				}
			}
		}
	}
	}	
}


void MyASTVisitor::searchFunc(Stmt *s,ASTFunctionDecl &Loop){
        if(s!=nullptr){
                //s->dump();

		//Get loops for metrics
		if(isa<ForStmt>(s)){
                        Loop.InternLoops.push_back(s->getSourceRange());
                }
                if(isa<WhileStmt>(s)){
                        Loop.InternLoops.push_back(s->getSourceRange());
                }
                if(isa<SwitchStmt>(s)){
                        Loop.InternLoops.push_back(s->getSourceRange());
                }
                if(isa<IfStmt>(s)){
                        Loop.ConditionalsStatements.push_back(s->getSourceRange());
                }
                if(isa<SwitchStmt>(s)){
                        Loop.ConditionalsStatements.push_back(s->getSourceRange());
                }
		if(isa<MemberExpr>(s)){
			//auto mem = cast<MemberExpr>(s);
			//ValueDecl *decl = mem->getMemberDecl();
		}
		//If is a class operator
		 if(isa<CXXOperatorCallExpr>(s)){
                        auto cxxOp = cast<CXXOperatorCallExpr>(s);

                        Decl *decl = cxxOp->getCalleeDecl();

                        ASTFunction newFun;
                        //Get location
			newFun.SLoc = s->getLocStart();
                        newFun.EndLoc = s->getLocEnd().getLocWithOffset(2);
                        newFun.FunLoc= cxxOp->getExprLoc().getRawEncoding();
			if(decl!=nullptr){
			//If is a method
			if(isa<clang::CXXMethodDecl>(decl)){
                        if(auto *fun = dyn_cast<clang::CXXMethodDecl>(decl)){
                                //Get operator name

				newFun.Name = fun->getNameAsString();
                                newFun.returnType = fun->getReturnType().getAsString();
                                unsigned numParam = fun->getNumParams();
				//If is operator () get return type
                                if(fun->getNameAsString() == "operator()"){
                                        newFun.paramTypes.push_back(fun->getReturnType().getAsString());

                                }
				//Get parameter types
                                for(unsigned i = 0;i<numParam;i++){
                                        auto param = fun->getParamDecl(i);
                                        newFun.paramTypes.push_back(param->getType().getAsString());

                                }
                                unsigned numArgs = cxxOp->getNumArgs();
				//Get arguments

                                for(unsigned i=0;i<numArgs;i++){
                                        Expr *arg = cxxOp->getArg(i);
                                         Stmt *stmt = cast<Stmt>(arg);

                                        MyASTVisitor::getArguments(stmt,&newFun);

                                }

                                Loop.fCalls.push_back(newFun);
                        }
			}
			}
//			return;

                }else{
		//If is a function call
                if(isa<CallExpr>(s)){
                        ASTFunction newFun;
                        CallExpr *call = cast<CallExpr>(s);
                        Decl *decl = call->getCalleeDecl();
                        //Get location
			newFun.SLoc = s->getLocStart();
                        newFun.EndLoc = s->getLocEnd().getLocWithOffset(2);
                        newFun.FunLoc= call->getExprLoc().getRawEncoding();
			if(decl!=nullptr){
                        if(FunctionDecl *fun = dyn_cast<clang::FunctionDecl>(decl)){
                                //Get function name
				newFun.Name = fun->getNameAsString();
                                newFun.returnType = fun->getReturnType().getAsString();
                                unsigned numParam = fun->getNumParams();
                                //Get parameters
				for(unsigned i = 0;i<numParam;i++){
                                        auto param = fun->getParamDecl(i);
                                        newFun.paramTypes.push_back(param->getType().getAsString());

                                }
                        }
                        unsigned numArgs = call->getNumArgs();
                        //Get arguments
			for(unsigned i=0;i<numArgs;i++){
                                Expr *arg = call->getArg(i);
                                Stmt *stmt = cast<Stmt>(arg);
                                MyASTVisitor::getArguments(stmt,&newFun);

                        }
                        Loop.fCalls.push_back(newFun);
			}
                }
		}
		if(s->children().begin() != s->children().end()){
		//Look for childrens
                 for(Stmt::child_iterator child = s->child_begin();child!=s->child_end();child++){

                        MyASTVisitor::searchFunc(*child,Loop);
                }
		}
        }

}

