#include "MyASTVisitor.hpp"

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
