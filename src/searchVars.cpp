#include <iostream>
/**
 *      @brief Store variable under UnaryOperators in ASTstruct.
 *
 *      This function will search on the AST every occurence of
 *      variable under UnaryOperator and it will store
 *      name, location, type and kind (lvalue,rvalue,xvalue and glvalue).
 *
 *      @param[in]      s, pointer to the analyzed statement.
 *      @param[in,out]  Loop, AST struct to store every variable information.
 *
 */
template <typename Struct>
bool MyASTVisitor::varsUnaryOperator(Stmt *s, Struct &Loop) {
  UnaryOperator *uo = cast<UnaryOperator>(s);
  auto child = uo->child_begin();
  if (isa<ImplicitCastExpr>(*child)) {
    ImplicitCastExpr *UoExpr = cast<ImplicitCastExpr>((*child));
    if (isa<DeclRefExpr>((*UoExpr->child_begin()))) {
      DeclRefExpr *ref = cast<DeclRefExpr>((*UoExpr->child_begin()));
      // if is a variable reference
      if (ref->getDecl() != NULL) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          ASTVar newVarRef;
          // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
          newVarRef.Name = var->getNameAsString();
          newVarRef.lvalue = uo->isLValue();
          newVarRef.rvalue = uo->isRValue();
          newVarRef.xvalue = uo->isXValue();
          newVarRef.glvalue = uo->isGLValue();
          newVarRef.RefSourceLoc = ref->getLocStart();
          // Get reference and declaration location
          unsigned location = var->getLocStart().getRawEncoding();
          newVarRef.DeclLoc = location;
          newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
          // Get variable type
          newVarRef.type = clang::QualType::getAsString(var->getType().split());
          newVarRef.globalVar = var->hasGlobalStorage();
          std::stringstream stream;
          stream << std::hex << location;
          std::string locstring(stream.str());

          Loop.VarRef.push_back(newVarRef);
          return true;
        }
      }
    } else {
      Stmt::child_iterator children = UoExpr->child_begin();
      bool found = false;
      while (!found && *children != nullptr) {
        /*	if(isa<CXXThisExpr>(*children)){
						return true;
					}

				*/ if (isa<DeclRefExpr>(
                                                         *children)) {
          DeclRefExpr *ref = cast<DeclRefExpr>((*children));
          // if is a variable reference
          if (ref->getDecl() != NULL) {
            if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
              ASTVar newVarRef;
              // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
              newVarRef.Name = var->getNameAsString();
              newVarRef.lvalue = uo->isLValue();
              newVarRef.rvalue = uo->isRValue();
              newVarRef.xvalue = uo->isXValue();
              newVarRef.glvalue = uo->isGLValue();
              newVarRef.RefSourceLoc = ref->getLocStart();
              // Get reference and declaration location
              unsigned location = var->getLocStart().getRawEncoding();
              newVarRef.DeclLoc = location;
              newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
              // Get variable type
              newVarRef.type =
                  clang::QualType::getAsString(var->getType().split());
              newVarRef.globalVar = var->hasGlobalStorage();
              std::stringstream stream;
              stream << std::hex << location;
              std::string locstring(stream.str());
              if (uo->getOpcodeStr(uo->getOpcode()).find("operator") ||
                  uo->getOpcodeStr(uo->getOpcode()).find("++") ||
                  uo->getOpcodeStr(uo->getOpcode()).find("--")) {
                newVarRef.opModifier = true;
                newVarRef.lvalue = true;
                newVarRef.rvalue = true;
              }

              Loop.VarRef.push_back(newVarRef);
              return true;
            }
          }
        } else {
          if (isa<CXXThisExpr>(*children))
            return false;
          children = children->child_begin();
        }
      }
    }
  }
  if (isa<DeclRefExpr>(*child)) {
    DeclRefExpr *ref = cast<DeclRefExpr>((*child));
    // Detect unary operators like ++, --, etc.
    if (uo->getOpcodeStr(uo->getOpcode()).find("operator") ||
        uo->getOpcodeStr(uo->getOpcode()).find("++") ||
        uo->getOpcodeStr(uo->getOpcode()).find("--")) {
      if (ref->getDecl() != NULL) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          ASTVar newVarRef;
          newVarRef.Name = var->getNameAsString();
          // these operator can produce feedbacks.
          newVarRef.lvalue = true;
          newVarRef.rvalue = true;
          newVarRef.opModifier = true;
          newVarRef.xvalue = uo->isXValue();
          newVarRef.glvalue = uo->isGLValue();
          // Get reference and declaration location
          unsigned location = var->getLocStart().getRawEncoding();
          newVarRef.DeclLoc = location;
          newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
          newVarRef.RefSourceLoc = ref->getLocStart();
          // Get variable type
          newVarRef.type = clang::QualType::getAsString(var->getType().split());
          newVarRef.globalVar = var->hasGlobalStorage();
          std::stringstream stream;
          stream << std::hex << location;
          std::string locstring(stream.str());

          Loop.VarRef.push_back(newVarRef);
          return true;
        }
      }
    }
  } else {
    //---------------------------------
    if (*child != nullptr) {
      Stmt::child_iterator children = child->child_begin();
      Stmt::child_iterator children2 = child->child_end();
      bool found = false;
      while (!found && children != children2) {
        if (isa<DeclRefExpr>(*children)) {
          DeclRefExpr *ref = cast<DeclRefExpr>((*children));
          // if is a variable reference
          if (ref->getDecl() != NULL) {
            if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
              ASTVar newVarRef;
              // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
              newVarRef.Name = var->getNameAsString();
              newVarRef.lvalue = false;
              newVarRef.rvalue = false;
              newVarRef.xvalue = false;
              newVarRef.glvalue = false;
              newVarRef.RefSourceLoc = ref->getLocStart();
              // Get reference and declaration location
              unsigned location = var->getLocStart().getRawEncoding();
              newVarRef.DeclLoc = location;
              newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
              // Get variable type
              newVarRef.type =
                  clang::QualType::getAsString(var->getType().split());
              newVarRef.globalVar = var->hasGlobalStorage();
              std::stringstream stream;
              stream << std::hex << location;
              std::string locstring(stream.str());
              if (uo->getOpcodeStr(uo->getOpcode()).find("operator") ||
                  uo->getOpcodeStr(uo->getOpcode()).find("++") ||
                  uo->getOpcodeStr(uo->getOpcode()).find("--")) {
                newVarRef.opModifier = true;
                newVarRef.lvalue = true;
                newVarRef.rvalue = true;
              }

              Loop.VarRef.push_back(newVarRef);
              return true;
            }
          }
          return false;
        } else {
          if (isa<CXXThisExpr>(*children)) {
            return false;
          }
          children = children->child_begin();
        }
      }
      return false;
    }
    //--------------------------------
  }
  return false;
}

/**
        Store variable declarations in ASTstruct.
        This function will search on the AST every occurence of
        variable declaration and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).

        @param[in]      s, pointer to the analyzed statement.
        @param[in,out]  Loop, AST struct to store every variable information.

*/

template <typename Struct>
bool MyASTVisitor::varsDeclaration(Stmt *s, Struct &Loop) {
  DeclStmt *dec = cast<DeclStmt>(s);
  // look for every declaration inside the statement
  for (auto decIt = dec->decl_begin(); decIt != dec->decl_end(); decIt++) {
    if ((*decIt) != nullptr) {
      // If is a variable declaration
      if (VarDecl *var = dyn_cast<VarDecl>((*decIt))) {
        // Get location
        unsigned location = var->getLocStart().getRawEncoding();
        // newVarDecl.RefSourceLoc = var->getLocStart();
        std::stringstream stream;
        stream << std::hex << location;
        std::string locstring(stream.str());
        ASTVar newVarDecl;
        newVarDecl.RefSourceLoc = var->getLocStart();
        newVarDecl.RefEndLoc = var->getLocEnd();
        // Get var name
        newVarDecl.Name = var->getNameAsString();
        // Get type
        newVarDecl.type = clang::QualType::getAsString(var->getType().split());
        // Set reference and declaration location
        newVarDecl.DeclLoc = location;
        newVarDecl.RefLoc = location;
        newVarDecl.globalVar = var->hasGlobalStorage();
        Loop.VarDecl.push_back(newVarDecl);
        // If it has initializers this reference will be lvalue
        if (var->getAnyInitializer() != nullptr) {
          newVarDecl.lvalue = true;
        } else {
          // else it will be rvalue
          newVarDecl.lvalue = false;
          newVarDecl.rvalue = true;
        }

        Loop.VarRef.push_back(newVarDecl);
        return true;
      }
      return true;
    }
  }
  return false;
}

template <typename Struct>
bool MyASTVisitor::RecursiveCXXOperatorCallExpr(Stmt *s, ImplicitCastExpr *Expr,
                                                Struct &Loop) {
  int childs = 0;
  if (s != nullptr) {
    for (Stmt::child_iterator child = s->child_begin(); child != s->child_end();
         child++) {
      childs++;
      if (childs == 2) {
        if (*child != nullptr) {
          if (isa<CXXOperatorCallExpr>(*child))
            return RecursiveCXXOperatorCallExpr(*child, Expr, Loop);
          if (isa<DeclRefExpr>(*child)) {
            DeclRefExpr *ref = cast<DeclRefExpr>(*child);
            if (ref->getDecl() != nullptr) {
              // If is a reference
              if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
                // Get variable name
                ASTVar newVarRef;
                newVarRef.Name = var->getNameAsString();
                // Get variable kind
                newVarRef.lvalue = Expr->isLValue();
                newVarRef.rvalue = Expr->isRValue();
                newVarRef.xvalue = Expr->isXValue();
                newVarRef.glvalue = Expr->isGLValue();
                newVarRef.RefSourceLoc = ref->getLocStart();
                // Get variable type
                newVarRef.type =
                    clang::QualType::getAsString(var->getType().split());
                newVarRef.globalVar = var->hasGlobalStorage();
                // Get reference and declaration location
                unsigned location = var->getLocStart().getRawEncoding();
                std::stringstream stream;
                stream << std::hex << location;
                std::string locstring(stream.str());
                newVarRef.DeclLoc = location;
                newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
                Loop.VarRef.push_back(newVarRef);
                return true;
              }
            }
          }
        }
      }
    }
    // return false;
  }
  return false;
}

/**
        Store variable under ImplicitCast in ASTstruct.
        This function will search on the AST every occurence of
        variable under UnaryOperator and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).

        @param[in]      s, pointer to the analyzed statement.
        @param[in,out]  Loop, AST struct to store every variable information.

*/

template <typename Struct>
bool MyASTVisitor::varsImplicitCast(Stmt *s, Struct &Loop) {
  ImplicitCastExpr *Expr = cast<ImplicitCastExpr>(s);

  // If isa CXXOperatorCallExpr
  if (isa<CXXOperatorCallExpr>(*s->child_begin())) {
    return RecursiveCXXOperatorCallExpr(*s->child_begin(), Expr, Loop);
  }
  // If is a reference
  if (isa<DeclRefExpr>((*s->child_begin()))) {
    DeclRefExpr *ref = cast<DeclRefExpr>((*s->child_begin()));
    // if is a variable reference
    if (ref->getDecl() != NULL) {
      if (isa<VarDecl>(ref->getDecl()))
        if (VarDecl *var = cast<clang::VarDecl>(ref->getDecl())) {

          ASTVar newVarRef;
          // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
          newVarRef.Name = var->getNameAsString();
          newVarRef.lvalue = Expr->isLValue();
          newVarRef.rvalue = Expr->isRValue();
          newVarRef.xvalue = Expr->isXValue();
          newVarRef.glvalue = Expr->isGLValue();
          newVarRef.RefSourceLoc = ref->getLocStart();

          // Get reference and declaration location
          unsigned location = var->getLocStart().getRawEncoding();
          newVarRef.DeclLoc = location;
          newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
          // Get variable type
          newVarRef.type = clang::QualType::getAsString(var->getType().split());
          newVarRef.globalVar = var->hasGlobalStorage();
          std::stringstream stream;
          stream << std::hex << location;
          std::string locstring(stream.str());
          // IF IS LVALUE THEN LOOK FOR DEPENDENCIES
          /*	if(newVarRef.lvalue){
                          llvm::errs()<<"-----------------------------------"<<newVarRef.Name<<"----\n";
                           searchVarDependency(s, newVarRef);
                  }
*/
          Loop.VarRef.push_back(newVarRef);
        }
    }
    return true;
  }
  // If is an arraySubscript
  if (isa<ArraySubscriptExpr>((*s->child_begin()))) {
    ArraySubscriptExpr *uo = cast<ArraySubscriptExpr>((*s->child_begin()));
    auto child = uo->child_begin();
    if (isa<ImplicitCastExpr>(*child)) {
      ImplicitCastExpr *UoExpr = cast<ImplicitCastExpr>((*child));
      if (isa<DeclRefExpr>((*UoExpr->child_begin()))) {
        DeclRefExpr *ref = cast<DeclRefExpr>((*UoExpr->child_begin()));
        // if is a variable reference
        if (ref->getDecl() != NULL) {
          if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
            ASTVar newVarRef;
            // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
            newVarRef.Name = var->getNameAsString();
            newVarRef.lvalue = Expr->isLValue();
            newVarRef.rvalue = Expr->isRValue();
            newVarRef.xvalue = Expr->isXValue();
            newVarRef.glvalue = Expr->isGLValue();
            newVarRef.RefSourceLoc = ref->getLocStart();
            // Get reference and declaration location
            unsigned location = var->getLocStart().getRawEncoding();
            newVarRef.DeclLoc = location;
            newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
            // Get variable type
            newVarRef.type =
                clang::QualType::getAsString(var->getType().split());
            newVarRef.globalVar = var->hasGlobalStorage();
            std::stringstream stream;
            stream << std::hex << location;
            std::string locstring(stream.str());
            // IF IS LVALUE THEN LOOK FOR DEPENDENCIES
            /*
                                                    if(newVarRef.lvalue){
                                                            llvm::errs()<<"-----------------------------------"<<newVarRef.Name<<"----\n";

                                                            searchVarDependency(s,
               newVarRef);
                                                    }
            */
            Loop.VarRef.push_back(newVarRef);
            return true;
          }
        }
      }
    }
  }
  return false;
}

/**
        Store variable reference in ASTstruct.
        This function will search on the AST every occurence of
        variable reference and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).

        @param[in]      s, pointer to the analyzed statement.
        @param[in,out]  Loop, AST struct to store every variable information.

*/

template <typename Struct>
bool MyASTVisitor::varsReference(Stmt *s, Struct &Loop) {
  DeclRefExpr *ref = cast<DeclRefExpr>(s);
  if (ref->getDecl() != nullptr) {
    // If is a reference
    if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {

      // Get variable name
      ASTVar newVarRef;
      newVarRef.Name = var->getNameAsString();
      // Get variable kind
      newVarRef.lvalue = ref->isLValue();
      newVarRef.rvalue = ref->isRValue();
      newVarRef.xvalue = ref->isXValue();
      newVarRef.glvalue = ref->isGLValue();
      newVarRef.RefSourceLoc = ref->getLocStart();

      // Get variable type
      newVarRef.type = clang::QualType::getAsString(var->getType().split());
      newVarRef.globalVar = var->hasGlobalStorage();
      // Get reference and declaration location
      unsigned location = var->getLocStart().getRawEncoding();
      std::stringstream stream;
      stream << std::hex << location;
      std::string locstring(stream.str());
      newVarRef.DeclLoc = location;
      newVarRef.RefLoc = ref->getLocStart().getRawEncoding();

      // IF IS LVALUE THEN LOOK FOR DEPENDENCIES
      /*			if(newVarRef.lvalue){
                                      searchVarDependency(s, newVarRef);
                              }*/
      Loop.VarRef.push_back(newVarRef);
      return true;
    }
  }

  return false;
}
/**
        Store variable under ArraySubscript in ASTstruct.
        This function will search on the AST every occurence of
        variable under ArraySubscript and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).

        @param[in]      s, pointer to the analyzed statement.
        @param[in,out]  Loop, AST struct to store every variable information.

*/

template <typename Struct>
bool MyASTVisitor::varsArraySubscript(Stmt *s, Struct &Loop) {
  ArraySubscriptExpr *uo = cast<ArraySubscriptExpr>(s);
  auto child = uo->child_begin();
  if (isa<ImplicitCastExpr>(*child)) {
    ImplicitCastExpr *UoExpr = cast<ImplicitCastExpr>((*child));
    if (isa<DeclRefExpr>((*UoExpr->child_begin()))) {
      DeclRefExpr *ref = cast<DeclRefExpr>((*UoExpr->child_begin()));
      // if is a variable reference
      if (ref->getDecl() != NULL) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          ASTVar newVarRef;
          // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
          newVarRef.Name = var->getNameAsString();
          newVarRef.lvalue = uo->isLValue();
          newVarRef.rvalue = uo->isRValue();
          newVarRef.xvalue = uo->isXValue();
          newVarRef.glvalue = uo->isGLValue();
          newVarRef.RefSourceLoc = ref->getLocStart();
          // Get reference and declaration location
          unsigned location = var->getLocStart().getRawEncoding();
          newVarRef.DeclLoc = location;
          newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
          // Get variable type
          newVarRef.type = clang::QualType::getAsString(var->getType().split());
          newVarRef.globalVar = var->hasGlobalStorage();
          std::stringstream stream;
          stream << std::hex << location;
          std::string locstring(stream.str());
          Loop.VarRef.push_back(newVarRef);
          return true;
        }
      }
    } else {
      // If is an array member expression
      if (isa<MemberExpr>((*UoExpr->child_begin()))) {
        MemberExpr *member = cast<MemberExpr>((*UoExpr->child_begin()));
        // Get the implicit cast
        if (isa<ImplicitCastExpr>((*member->child_begin()))) {
          ImplicitCastExpr *Imp =
              cast<ImplicitCastExpr>((*member->child_begin()));
          // Get attribute array
          if (isa<DeclRefExpr>((*Imp->child_begin()))) {
            DeclRefExpr *ref = cast<DeclRefExpr>((*UoExpr->child_begin()));
            if (ref->getDecl() != NULL) {
              // if(VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())){
              if (isa<VarDecl>(ref->getDecl())) {
                VarDecl *var = cast<VarDecl>(ref->getDecl());
                ASTVar newVarRef;
                // var->dump();
                //								std::stringstream
                // stream;
                //								stream
                //<<
                // var->getNameAsString() << MemberExpr;
                if (var->getIdentifier()->getNameStart() == NULL) {
                  return false;
                }
                newVarRef.Name =
                    std::string(var->getIdentifier()->getNameStart());
                newVarRef.lvalue = uo->isLValue();
                newVarRef.rvalue = uo->isRValue();
                newVarRef.xvalue = uo->isXValue();
                newVarRef.glvalue = uo->isGLValue();
                unsigned location = var->getLocStart().getRawEncoding();
                newVarRef.DeclLoc = location;
                newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
                newVarRef.type =
                    clang::QualType::getAsString(var->getType().split());
                newVarRef.globalVar = var->hasGlobalStorage();
                std::stringstream stream;
                stream << std::hex << location;
                std::string locstring(stream.str());
                Loop.VarRef.push_back(newVarRef);

                return true;
              }
            }
          }
        }
      } else {
        return recursiveArraySubscript(*UoExpr->child_begin(), uo, Loop);
      }
    }
  }

  return false;
}

template <typename Struct>
bool MyASTVisitor::recursiveArraySubscript(Stmt *s, ArraySubscriptExpr *uo,
                                           Struct &Loop) {
  if (isa<ArraySubscriptExpr>(s)) {
    auto child = s->child_begin();
    if (isa<ImplicitCastExpr>(*child)) {
      ImplicitCastExpr *UoExpr = cast<ImplicitCastExpr>((*child));
      if (isa<DeclRefExpr>((*UoExpr->child_begin()))) {
        DeclRefExpr *ref = cast<DeclRefExpr>((*UoExpr->child_begin()));
        // if is a variable reference
        if (ref->getDecl() != NULL) {
          if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
            ASTVar newVarRef;
            // Get name and variable kind (lvalue,rvalue,glvalue,xvalue)
            newVarRef.Name = var->getNameAsString();
            newVarRef.lvalue = uo->isLValue();
            newVarRef.rvalue = uo->isRValue();
            newVarRef.xvalue = uo->isXValue();
            newVarRef.glvalue = uo->isGLValue();
            newVarRef.RefSourceLoc = ref->getLocStart();
            // Get reference and declaration location
            unsigned location = var->getLocStart().getRawEncoding();
            newVarRef.DeclLoc = location;
            newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
            // Get variable type
            newVarRef.type =
                clang::QualType::getAsString(var->getType().split());
            newVarRef.globalVar = var->hasGlobalStorage();
            std::stringstream stream;
            stream << std::hex << location;
            std::string locstring(stream.str());
            Loop.VarRef.push_back(newVarRef);
            return true;
          }
        }
      } else {
        return recursiveArraySubscript(*UoExpr->child_begin(), uo, Loop);
      }
    }

  } else {
    return false;
  }
  return true;
}

void MyASTVisitor::varsRefDepend(Stmt *s, ASTVar &varRef) {
  DeclRefExpr *ref = cast<DeclRefExpr>(s);
  if (ref->getDecl() != nullptr) {
    // If is a reference
    if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
      ASTRef depend;
      depend.Name = var->getNameAsString();
      unsigned location = var->getLocStart().getRawEncoding();
      depend.DeclLoc = location;
      depend.RefLoc = ref->getLocStart().getRawEncoding();
      varRef.dependencies.push_back(depend);
    }
  }
}

void MyASTVisitor::searchVarDependency(Stmt *s, ASTVar &var) {
  if (s != nullptr) {
    // If is a reference
    if (isa<DeclRefExpr>(s)) {
      varsRefDepend(s, var);
    }
    if (s->children().begin() != s->children().end()) {
      // Look for childrens
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {
        MyASTVisitor::searchVarDependency(*child, var);
      }
    }
  }
}
template <typename Struct>
void MyASTVisitor::BinaryOperators(Stmt *s, Struct &Loop) {
  if (s != nullptr) {
    if (s->children().begin() != s->children().end()) {
      if (isa<CompoundAssignOperator>(s) ||
          isa<UnresolvedMemberExpr>((*s->child_begin())) ||
          isa<CXXDependentScopeMemberExpr>((*s->child_begin()))) {

      } else {
        /*if(isa<MemberExpr>(s)){
                MemberExpr * mem = cast<MemberExpr>(s);
                if(isa<CXXThisExpr>((*s->child_begin()))){
                        ASTVar Var;
                       Var.Name = mem->getMemberDecl()->getNameAsString();
                       Var.lvalue = mem->isLValue();
                       Var.rvalue = mem->isRValue();
                       Var.RefLoc = mem-> getLocStart().getRawEncoding();
                        Loop.VarRef.push_back(Var);
// llvm::errs()<<mem->getMemberDecl()->getNameAsString()<<"\n";
                 }
        }*/
        if (isa<DeclStmt>(s) || isa<DeclRefExpr>(s)) {
          ASTVar newVarRef;
          bool valid = GetLeftSideBO(s, newVarRef);
          if (valid)
            Loop.Assign.push_back(newVarRef);
        } else {

          ASTVar newVarRef;
          bool valid = GetLeftSideBO((*s->child_begin()), newVarRef);
          if (valid) {
            auto child = s->child_begin();
            child++;
            searchVarDependency((*child), newVarRef);
            Loop.Assign.push_back(newVarRef);
            //		return;
          }
          return;
        }
      }
    }
  }
}

template <typename Struct>
bool MyASTVisitor::GetLeftSideBO(Stmt *s, Struct &Var) {
  if (s != nullptr) {
    if (isa<DeclStmt>(s)) {

      DeclStmt *dec = cast<DeclStmt>(s);
      // look for every declaration inside the statement
      for (auto decIt = dec->decl_begin(); decIt != dec->decl_end(); decIt++) {
        if ((*decIt) != nullptr) {
          if (isa<VarDecl>((*decIt))) {
            // If is a variable declaration
            if (VarDecl *var = cast<VarDecl>((*decIt))) {
              Var.Name = var->getNameAsString();
              unsigned location = var->getLocStart().getRawEncoding();
              Var.DeclLoc = location;
              Var.RefSourceLoc = var->getLocStart();
              Var.RefLoc = location;
              //*dec->children().empty()
              if (dec->children().begin() != dec->children().end()) {
                searchVarDependency((*dec->child_begin()), Var);
              }
            }
          }
        }
      }
      return true;
    }

    if (isa<DeclRefExpr>(s)) {
      //	s->dump();
      //	llvm::errs()<<"-----------------\n";
      DeclRefExpr *ref = cast<DeclRefExpr>(s);
      // if is a variable reference
      if (ref->getDecl() != nullptr) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          Var.Name = var->getNameAsString();
          unsigned location = var->getLocStart().getRawEncoding();
          Var.DeclLoc = location;
          Var.RefSourceLoc = ref->getLocStart();
          Var.RefLoc = ref->getLocStart().getRawEncoding();
        }
      }
      return true;
    } else {
      if (isa<CXXOperatorCallExpr>(s)) {

        auto child = s->child_begin();
        child++;
        return GetLeftSideBO((*child), Var);
      } else {
        if (isa<MemberExpr>(s)) {
          MemberExpr *mem = cast<MemberExpr>(s);
          // llvm::errs()<<"MEMBER DECL\n";
          //	llvm::errs()<<mem->getMemberDecl()->getNameAsString();
          // llvm::errs()<<mem->getMemberNameInfo().getName().getAsString()<<"\n";
          if (isa<CXXThisExpr>((*s->child_begin()))) {
            Var.Name = mem->getMemberDecl()->getNameAsString();
            Var.lvalue = mem->isLValue();
            Var.rvalue = mem->isRValue();
            Var.RefLoc = mem->getLocStart().getRawEncoding();
            //						llvm::errs()<<mem->getMemberDecl()->getNameAsString()<<"\n";
            return true;
          }
          return MyASTVisitor::GetLeftSideBO((*s->child_begin()), Var);

        } else {
          if (isa<CXXThisExpr>(s) || isa<UnresolvedMemberExpr>(s)) {
            // s->dump();
          } else {

            return MyASTVisitor::GetLeftSideBO((*s->child_begin()), Var);
          }
        }
      }
    }
  }
  return false;
}

/**


*/
template <typename Struct>
bool MyASTVisitor::varsCompoundAssignment(CompoundAssignOperator *cmp, Stmt *s,
                                          Struct &Loop) {
  if (s != nullptr) {
    bool firstOp = true;
    bool found = false;
    for (Stmt::child_iterator child = s->child_begin(); child != s->child_end();
         child++) {

      // FIRST OPERATOR
      if (firstOp) {
        if (isa<DeclRefExpr>(*child)) {
          DeclRefExpr *ref = cast<DeclRefExpr>(*child);
          if (ref->getDecl() != nullptr) {
            // If is a reference
            if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {

              // Get variable name
              ASTVar newVarRef;
              newVarRef.Name = var->getNameAsString();
              // Get variable kind
              newVarRef.lvalue = true;
              newVarRef.rvalue = true;
              newVarRef.xvalue = ref->isXValue();
              newVarRef.glvalue = ref->isGLValue();
              newVarRef.RefSourceLoc = ref->getLocStart();

              newVarRef.cmpAssign = true;
              // newVarRef.cmpOper = cmp->getOpcodeStr(cmp->getOpcode());
              // if(clang::BinaryOperator::isMultiplicativeOp(cmp->getOpcode())
              // || clang::BinaryOperator::isAdditiveOp(cmp->getOpcode())){

              if (!cmp->getOpcodeStr(cmp->getOpcode()).empty()) {
                if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("+=") !=
                    std::string::npos)
                  newVarRef.cmpOper = "+";
                else if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("-=") !=
                         std::string::npos)
                  newVarRef.cmpOper = "-";
                else if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("*=") !=
                         std::string::npos)
                  newVarRef.cmpOper = "*";
                else if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("/=") !=
                         std::string::npos)
                  newVarRef.cmpOper = "/";
                else
                  newVarRef.cmpOper = "?";
              }
              // Get variable type
              newVarRef.type =
                  clang::QualType::getAsString(var->getType().split());
              newVarRef.globalVar = var->hasGlobalStorage();
              // Get reference and declaration location
              unsigned location = var->getLocStart().getRawEncoding();
              std::stringstream stream;
              stream << std::hex << location;
              std::string locstring(stream.str());
              newVarRef.DeclLoc = location;
              newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
              Loop.VarRef.push_back(newVarRef);
            }
          }
        } else {
          if (isa<CXXOperatorCallExpr>(s)) {
            child++;
            /*varsCompoundAssignment(*child,Loop);*/
            if (isa<DeclRefExpr>(*child)) {
              DeclRefExpr *ref = cast<DeclRefExpr>(*child);
              if (ref->getDecl() != nullptr) {
                // If is a reference
                if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {

                  // Get variable name
                  ASTVar newVarRef;
                  newVarRef.Name = var->getNameAsString();
                  // Get variable kind
                  newVarRef.lvalue = true;
                  newVarRef.rvalue = true;
                  newVarRef.xvalue = ref->isXValue();
                  newVarRef.glvalue = ref->isGLValue();
                  newVarRef.RefSourceLoc = ref->getLocStart();
                  newVarRef.cmpAssign = true;
                  // newVarRef.cmpOper = cmp->getOpcodeStr(cmp->getOpcode());
                  /*if(cmp->isMultiplicativeOp(cmp->getOpcode()) ||
                  cmp->isAdditiveOp(cmp->getOpcode())){
                    if(cmp->getOpcodeStr(cmp->getOpcode()).find("+="))
                  newVarRef.cmpOper = "+";
                    if(cmp->getOpcodeStr(cmp->getOpcode()).find("-="))
                  newVarRef.cmpOper = "-";
                    if(cmp->getOpcodeStr(cmp->getOpcode()).find("*="))
                  newVarRef.cmpOper = "*";
                    if(cmp->getOpcodeStr(cmp->getOpcode()).find("/="))
                  newVarRef.cmpOper = "/";
                  }else newVarRef.cmpOper = "?";*/

                  if (!cmp->getOpcodeStr(cmp->getOpcode()).empty()) {
                    if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("+=") !=
                        std::string::npos)
                      newVarRef.cmpOper = "+";
                    else if (cmp->getOpcodeStr(cmp->getOpcode())
                                 .str()
                                 .find("-=") != std::string::npos)
                      newVarRef.cmpOper = "-";
                    else if (cmp->getOpcodeStr(cmp->getOpcode())
                                 .str()
                                 .find("*=") != std::string::npos)
                      newVarRef.cmpOper = "*";
                    else if (cmp->getOpcodeStr(cmp->getOpcode())
                                 .str()
                                 .find("/=") != std::string::npos)
                      newVarRef.cmpOper = "/";
                    else
                      newVarRef.cmpOper = "?";
                  }
                  // Get variable type
                  newVarRef.type =
                      clang::QualType::getAsString(var->getType().split());
                  newVarRef.globalVar = var->hasGlobalStorage();
                  // Get reference and declaration location
                  unsigned location = var->getLocStart().getRawEncoding();
                  std::stringstream stream;
                  stream << std::hex << location;
                  std::string locstring(stream.str());
                  newVarRef.DeclLoc = location;
                  newVarRef.RefLoc = ref->getLocStart().getRawEncoding();
                  Loop.VarRef.push_back(newVarRef);
                }
              }
            }
          } else {
            if (isa<MemberExpr>(*child)) {

              MemberExpr *mem = cast<MemberExpr>(*child);
              if (isa<CXXThisExpr>((*child->child_begin()))) {
                ASTVar Var;
                Var.Name = mem->getMemberDecl()->getNameAsString();
                Var.lvalue = mem->isLValue();
                Var.rvalue = mem->isRValue();
                Var.RefLoc = mem->getLocStart().getRawEncoding();
                Var.cmpAssign = true;
                /*if(cmp->isMultiplicativeOp(cmp->getOpcode())  ||
                cmp->isAdditiveOp(cmp->getOpcode())){
                  if(cmp->getOpcodeStr(cmp->getOpcode()).find("+=")) Var.cmpOper
                = "+";
                  if(cmp->getOpcodeStr(cmp->getOpcode()).find("-=")) Var.cmpOper
                = "-";
                  if(cmp->getOpcodeStr(cmp->getOpcode()).find("*=")) Var.cmpOper
                = "*";
                  if(cmp->getOpcodeStr(cmp->getOpcode()).find("/=")) Var.cmpOper
                = "/";
                }else Var.cmpOper = "?";*/
                // Var.cmpOper = cmp->getOpcodeStr(cmp->getOpcode());
                if (!cmp->getOpcodeStr(cmp->getOpcode()).empty()) {
                  if (cmp->getOpcodeStr(cmp->getOpcode()).str().find("+=") !=
                      std::string::npos)
                    Var.cmpOper = "+";
                  else if (cmp->getOpcodeStr(cmp->getOpcode())
                               .str()
                               .find("-=") != std::string::npos)
                    Var.cmpOper = "-";
                  else if (cmp->getOpcodeStr(cmp->getOpcode())
                               .str()
                               .find("*=") != std::string::npos)
                    Var.cmpOper = "*";
                  else if (cmp->getOpcodeStr(cmp->getOpcode())
                               .str()
                               .find("/=") != std::string::npos)
                    Var.cmpOper = "/";
                  else
                    Var.cmpOper = "?";
                }
                Loop.VarRef.push_back(Var);
              }

            } else {
              varsCompoundAssignment(cmp, *child, Loop);
            }
          }
        }
      }
      // REST OF OPERATORS
      else {
        found = varsCompoundOperators(*child, Loop);
      }
      firstOp = false;
    }
    return found;
  }
  //	return found;
  return false;
}

template <typename Struct>
bool MyASTVisitor::varsCompoundOperators(Stmt *s, Struct &Loop) {
  if (s != NULL) {
    // s->dump();
    bool found = false;
    // Unary operators
    if (isa<UnaryOperator>(s)) {
      found = varsUnaryOperator(s, Loop);
    }
    // If is a array subscript
    if (isa<ArraySubscriptExpr>(s)) {
      found = varsArraySubscript(s, Loop);
    }
    // If is a declaration statement
    if (isa<DeclStmt>(s)) {

      /*found =*/varsDeclaration(s, Loop);
      //              s->dump();
      BinaryOperators(s, Loop);
    }
    // If is a Implicit cast
    if (isa<ImplicitCastExpr>(s)) {
      found = varsImplicitCast(s, Loop);
    }
    // If is a reference
    if (isa<DeclRefExpr>(s)) {
      found = varsReference(s, Loop);
    }
    // If is a BinaryOperator
    if (isa<BinaryOperator>(s)) {
      BinaryOperator *bo = cast<BinaryOperator>(s);
      if (bo->isAssignmentOp()) {
        BinaryOperators(s, Loop);
      }
    }
    if (isa<CompoundAssignOperator>(s)) {
      CompoundAssignOperator *cmp = cast<CompoundAssignOperator>(s);
      found = varsCompoundAssignment(cmp, s, Loop);
    }
    if (isa<CXXThisExpr>(s)) {
      found = true;
    }
    if (isa<MemberExpr>(s)) {
      MemberExpr *mem = cast<MemberExpr>(s);
      // llvm::errs()<<"MEMBER DECL\n";
      //      llvm::errs()<<mem->getMemberDecl()->getNameAsString();
      // llvm::errs()<<mem->getMemberNameInfo().getName().getAsString()<<"\n";
      if (isa<CXXThisExpr>((*s->child_begin()))) {
        ASTVar Var;
        Var.Name = mem->getMemberDecl()->getNameAsString();
        Var.lvalue = mem->isLValue();
        Var.rvalue = mem->isRValue();
        Var.RefLoc = mem->getLocStart().getRawEncoding();
        Loop.VarRef.push_back(Var);
        return true;
        //      	          llvm::errs()<<mem->getMemberDecl()->getNameAsString()<<"\n";
      }

      found = true;
      // return true;
    }

    if (found) {
      return found;
    }
    if (s->children().begin() != s->children().end()) {
      // Look for childrens
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {

        MyASTVisitor::searchVars(*child, Loop);
      }
    }
  }

  return true;
}
/**
        Store every variable reference or declaration in ASTLoop struct.
        This function will search on the AST every occurence of
        variable declaration or reference and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).

        @param[in]	s, pointer to the analyzed statement.
        @param[in,out]	Loop, ASTLoop struct to store every variable
   information.

*/
void MyASTVisitor::searchVars(Stmt *s, ASTLoop &Loop) {
  if (s != nullptr) {
    bool found = false;
    // s->dump();
    if (isa<ReturnStmt>(s)) {
      ReturnStmt *ret = cast<ReturnStmt>(s);
      Loop.Returns.push_back(ret->getLocStart());
    }
    if (isa<BreakStmt>(s)) {
      BreakStmt *brk = cast<BreakStmt>(s);
      Loop.Breaks.push_back(brk->getLocStart());
    }
    if (isa<GotoStmt>(s)) {
      GotoStmt *gto = cast<GotoStmt>(s);
      Loop.Gotos.push_back(gto->getLocStart());
    }
    // Unary operators
    if (isa<UnaryOperator>(s)) {
      found = varsUnaryOperator(s, Loop);
    }
    // If is a array subscript
    if (isa<ArraySubscriptExpr>(s)) {
      found = varsArraySubscript(s, Loop);
    }
    // If is a declaration statement
    if (isa<DeclStmt>(s)) {
      /*found =*/varsDeclaration(s, Loop);
      //		s->dump();

      BinaryOperators(s, Loop);
    }
    // If is a Implicit cast
    if (isa<ImplicitCastExpr>(s)) {
      found = varsImplicitCast(s, Loop);
    }
    // If is a reference
    if (isa<DeclRefExpr>(s)) {
      found = varsReference(s, Loop);
    }
    // If is a BinaryOperator
    if (isa<BinaryOperator>(s)) {
      BinaryOperator *bo = cast<BinaryOperator>(s);
      if (bo->isAssignmentOp()) {
        BinaryOperators(s, Loop);
      }
    }
    if (isa<CompoundAssignOperator>(s)) {
      CompoundAssignOperator *cmp = cast<CompoundAssignOperator>(s);
      found = varsCompoundAssignment(cmp, s, Loop);
    }
    if (isa<ReturnStmt>(s)) {
      found = true;
    }
    if (isa<CXXThisExpr>(s)) {
      found = true;
    }
    if (isa<MemberExpr>(s)) {
      // Will be collected on searchFunc
      // s->dump();
      // SIZE
      MemberExpr *mem = cast<MemberExpr>(s);
      // llvm::errs()<<"MEMBER DECL\n";
      //      llvm::errs()<<mem->getMemberDecl()->getNameAsString();
      // llvm::errs()<<mem->getMemberNameInfo().getName().getAsString()<<"\n";
      if (isa<CXXThisExpr>((*s->child_begin()))) {
        ASTVar Var;
        Var.Name = mem->getMemberDecl()->getNameAsString();
        Var.lvalue = mem->isLValue();
        Var.rvalue = mem->isRValue();
        Var.RefLoc = mem->getLocStart().getRawEncoding();
        Loop.VarRef.push_back(Var);
        found = true;
        //                llvm::errs()<<mem->getMemberDecl()->getNameAsString()<<"\n";
      }
    }
    if (found) {
      return;
    }
    // Look for childrens
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {
        MyASTVisitor::searchVars(*child, Loop);
      }
    }
  }
}

/**
        Store every variable reference or declaration in ASTFunctionDecl struct.
        This function will search on the AST every occurence of
        variable declaration or reference and it will store
        name, location, type and kind (lvalue,rvalue,xvalue and glvalue).
        Used to collect variable informations for user-defined and accesible
   functions.

        @param[in]      s, pointer to the analyzed statement.
        @param[in,out]  Loop, ASTFunctionDecl struct to store every variable
   information.

*/

void MyASTVisitor::searchVars(Stmt *s, ASTFunctionDecl &Loop) {
  if (s != nullptr) {
    // s->dump();
    bool found = false;
    // Unary operators
    if (isa<UnaryOperator>(s)) {
      found = varsUnaryOperator(s, Loop);
    }
    // If is a array subscript
    if (isa<ArraySubscriptExpr>(s)) {
      found = varsArraySubscript(s, Loop);
    }

    // If is a declaration statement
    if (isa<DeclStmt>(s)) {
      found = varsDeclaration(s, Loop);
      BinaryOperators(s, Loop);
    }
    // If is a implicit cast
    if (isa<ImplicitCastExpr>(s)) {
      found = varsImplicitCast(s, Loop);
    }
    // If is a reference
    if (isa<DeclRefExpr>(s)) {
      found = varsReference(s, Loop);
    }
    // If is a BinaryOperator
    if (isa<BinaryOperator>(s)) {
      BinaryOperator *bo = cast<BinaryOperator>(s);
      if (bo->isAssignmentOp()) {
        BinaryOperators(s, Loop);
        found = true;
      }
    }
    if (isa<ReturnStmt>(s)) {
      found = true;
    }
    if (found) {
      return;
    }
    // Look for childrens
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end() && !found; child++) {
        MyASTVisitor::searchVars(*child, Loop);
      }
    }
  }
}

void MyASTVisitor::searchVarsMember(Stmt *s, ASTLoop &Loop, bool read) {
  if (s != nullptr) {
    // If is a declaration statement
    if (isa<DeclRefExpr>(s)) {
      // s->dump();

      DeclRefExpr *ref = cast<DeclRefExpr>(s);
      // if is a variable reference
      if (ref->getDecl() != nullptr) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          ASTVar Var;
          Var.Name = var->getNameAsString();
          unsigned location = var->getLocStart().getRawEncoding();
          Var.lvalue = !read;
          Var.rvalue = read;
          Var.memberExpr = true;
          Var.DeclLoc = location;
          Var.RefSourceLoc = ref->getLocStart();
          Var.RefLoc = ref->getLocStart().getRawEncoding();
          Loop.VarRef.push_back(Var);
        }
      }
      return;
    }
    // Look for childrens
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {
        MyASTVisitor::searchVarsMember(*child, Loop, read);
      }
    }
  }
}
