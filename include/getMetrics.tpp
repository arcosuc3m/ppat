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
