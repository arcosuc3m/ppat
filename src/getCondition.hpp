
template <typename T>
void MyASTVisitor::getCondition(Expr * condition, T& Loop){
     ASTLoop aux;
     searchVars(condition,aux);
     searchFunc(condition,aux);
     ASTCondition cond;
     for(auto i = aux.VarDecl.begin();i!=aux.VarDecl.end(); i++){
        cond.VarDecl.push_back(*i);
     }
     for(auto i = aux.VarRef.begin();i!=aux.VarRef.end(); i++){
        cond.VarRef.push_back(*i);
     }
     for(auto i = aux.fCalls.begin();i!=aux.fCalls.end(); i++){
        cond.fCalls.push_back(*i);
     }
     searchOperator(condition, cond);
     Loop.cond = cond;
}

template <typename T>
void MyASTVisitor::searchOperator(Stmt * s, T& Loop){ 
     if(s!=nullptr){
        if(isa<BinaryOperator>(s)){
           BinaryOperator* bin = cast<BinaryOperator>(s);
           ASTOperators newOp;
           newOp.Name =  bin->getOpcodeStr().str();
           newOp.Location = bin->getOperatorLoc();
           std::cout<<" NOMBRE : " << newOp.Name << " LOCATION : " << bin->getLocStart().getRawEncoding();
           newOp.type = bin->getType().getAsString();
           Loop.ops.push_back(newOp);
        }
        if(isa<CompoundAssignOperator>(s)){
           //FIXME: TO BE IMPLEMENTED    
        }
        if(isa<DeclRefExpr>(s)){
           DeclRefExpr * ref = cast<DeclRefExpr>(s);
           if(ref->getDecl() != nullptr){
               std::string name = ref->getDecl()->getNameAsString();
               if(name.find("operator")!=std::string::npos){
                   //LOCATION
                   ASTOperators newOp;
                   newOp.Name = name;
                   newOp.Location = ref->getLocStart();
                   newOp.type = ref->getType().getAsString();
                   Loop.ops.push_back(newOp);
               }
           }
        }

        //Look for childrens
        if(s->children().begin() != s->children().end()){
              for(Stmt::child_iterator child = s->child_begin();child!=s->child_end();child++){
                    MyASTVisitor::searchOperator(*child,Loop);
             }
         }
     }
}
