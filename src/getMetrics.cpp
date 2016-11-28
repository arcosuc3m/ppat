#include "MyASTVisitor.hpp"

template <typename T> void MyASTVisitor::getMetrics(Stmt *s, T &Loop) {
  calculateCyclomaticComplexity(s, Loop.McCC);
  binaryOperatorMetric(s, Loop);
}

template <typename T>
void MyASTVisitor::binaryOperatorMetric(Stmt *s, T &Loop) {
  if (s != NULL) {
    if (isa<BinaryOperator>(s)) {
      BinaryOperator *bo = cast<BinaryOperator>(s);
      if (bo->isAdditiveOp()) {
        Loop.ADD++;
      }
      if (bo->isMultiplicativeOp()) {
        Loop.MUL++;
      }
    }
    if (isa<Expr>(s)) {
      Loop.EXP++;
    }
    if (isa<ArraySubscriptExpr>(s)) {
      Loop.ARR++;
    }
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {

        MyASTVisitor::binaryOperatorMetric(*child, Loop);
      }
    }
  }
}

void MyASTVisitor::calculateCyclomaticComplexity(Stmt *s, int &McCC) {
  if (s != NULL) {
    if (isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<IfStmt>(s) ||
        isa<SwitchStmt>(s) || isa<CXXCatchStmt>(s) || isa<CaseStmt>(s)) {
      McCC++;
    }
    if (s->children().begin() != s->children().end()) {

      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {

        MyASTVisitor::calculateCyclomaticComplexity(*child, McCC);
      }
    }
  }
}
