#include "MyASTVisitor.hpp"
#include "ASTInfo.h"
#include "clang/AST/AST.h" /* Stmt */
#include <iostream>

using namespace clang;
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
