#ifndef MYASTVISITOR_H
#define MYASTVISITOR_H

#include "ASTInfo.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {

private:
  Rewriter &TheRewriter;

public:
  int numLoops;
  bool fullCode = true;
  std::vector<ASTLoop> Loops;
  std::vector<ASTLoop> Kernels;
  //  std::vector<ASTFunctionDecl> Functions;
  ASTLoop Code;

  // Constructor
  MyASTVisitor(Rewriter &R) : TheRewriter(R), numLoops{0} {}
  // CODE ANALYSIS
  bool analyzeCodeFunctions();
  bool analyzeCall(int i);
  void moveVariableDeclarations(int i, std::stringstream &declarations);

  // PIPELINE DETECTOR
  bool AnalyzeLoops();
  bool analyzeFunction(ASTFunctionDecl &function, int deep);
  bool analyzeLRValues(ASTFunctionDecl &function);
  bool analyzeLRLoop(int i);
  bool analyzeGlobalsLoop(int i, std::stringstream &SSComments,
                          std::vector<unsigned> &stagesLocEnds);
  void getStageIO(std::vector<std::vector<std::string>> &inputs,
                  std::vector<std::vector<std::string>> &outputs, int stages,
                  int i, std::vector<unsigned> stagesLocEnds);
  bool checkOutVar(ASTVar var, unsigned end);
  bool checkInVar(ASTVar var, unsigned start);
  void getPipelineStream(std::vector<std::vector<std::string>> &inputs,
                         std::vector<std::vector<std::string>> &outputs,
                         int stages,
                         std::vector<std::string> &stream); //
  void stageDetection(int i, int &stages, std::vector<unsigned> &stagesLocEnds);
  bool checkStages(int i, int stages, std::vector<unsigned> stagesLocEnds);
  bool checkFeedback(int i, std::stringstream &SSComments, bool &comments,
                     int stages, std::vector<unsigned> stagesLocEnds);
  void calculateExecutionOrder();                               //
  void getKernelDependency(std::stringstream &SSBefore, int i); //
  void moveDeclarations(int i, std::stringstream &declarations);
  void checkFarmStages(std::vector<unsigned> stagesLocEnds, int i,
                       std::vector<bool> &farmstage);

  template <typename T> void searchOperator(Stmt *s, T &Loop);
  template <typename T> void getCondition(Expr *condition, T &actualLoop);

  // MAP DETECTOR
  bool globalLValue(int i);
  void getMapIO(std::vector<std::string> &inputs,
                std::vector<std::string> &outputs, int i);

  void getMapIO(std::vector<std::string> &inputs,
                std::vector<std::string> &inputTypes,
                std::vector<std::string> &outputs,
                std::vector<std::string> &outputTypes, int i);

  void getDataStreams(std::vector<std::string> &ioVector,
                      std::vector<std::string> &dataStreams, int i); //
  bool checkFeedback(int i, std::stringstream &SSComments, bool &comments);
  void getStreamDependencies(std::vector<std::string> stream, int i);
  bool checkMapDependencies(std::vector<std::string> instream,
                            std::vector<std::string> outstream, int i);
  bool checkMemAccess(std::vector<std::string> dataStreams, int i);
  bool check_break(int i);
  bool checkPrivate(int i, ASTVar variable);
  void mapDetect();

  // FARM DETECTOR
  void farmDetect();
  bool getStreamGeneration(int i);
  std::vector<ASTVar> getOtherGenerations(int i, SourceLocation &fWrite,
                                          SourceLocation &lWrite);
  bool checkEmptyTask(int i);
  // REDUCE DETECTOR
  bool checkFeedback(int i, std::vector<std::string> &feedbackVars,
                     std::vector<std::string> &operators);
  bool reduceOperation(std::vector<std::string> feedbackVars,
                       std::vector<std::string> outputs,
                       std::vector<std::string> &reduceVars);
  void reduceDetect();
  template <typename Ref> bool checkFeedbackSameLine(Ref refs, int i);

  // ANNOTATION
  void PPATAnnotate();

  // TRANSFORMATION
  void PPATtoGRPPI();
  void writeGenFunc(ASTCondition cond, std::stringstream &GenFunc,
                    clang::SourceLocation e, clang::SourceLocation b);
  std::string getNext(clang::SourceLocation &current, ASTCondition cond,
                      clang::SourceLocation e);
  void getPrivated(int i);
  void getStreamGeneratorOut(int i);
  bool getSinkFunction(int i);

  // SEARCH VARIABLES
  void searchVars(Stmt *s, ASTLoop &Loop);
  void searchVars(Stmt *s, ASTFunctionDecl &Loop);
  void varsRefDepend(Stmt *s, ASTVar &var);
  void searchVarDependency(Stmt *s, ASTVar &var);
  void searchVarsMember(Stmt *s, ASTLoop &Loop, bool read);
  clang::SourceLocation getEndLocation(clang::SourceLocation, ASTLoop &Loop);

  // Detect variables
  template <typename Struct> bool varsUnaryOperator(Stmt *s, Struct &Loop);
  template <typename Struct> bool varsDeclaration(Stmt *s, Struct &Loop);
  template <typename Struct> bool varsImplicitCast(Stmt *s, Struct &Loop);
  template <typename Struct> bool varsReference(Stmt *s, Struct &Loop);
  template <typename Struct> bool varsArraySubscript(Stmt *s, Struct &Loop);
  template <typename Struct>
  bool recursiveArraySubscript(Stmt *s, ArraySubscriptExpr *uo, Struct &Loop);
  template <typename Struct>
  bool RecursiveCXXOperatorCallExpr(Stmt *s, ImplicitCastExpr *Expr,
                                    Struct &Loop);
  template <typename Struct>
  bool varsCompoundAssignment(CompoundAssignOperator *cmp, Stmt *s,
                              Struct &Loop);
  template <typename Struct> bool varsCompoundOperators(Stmt *s, Struct &Loop);
  // Variable dependencies
  template <typename Struct> void BinaryOperators(Stmt *s, Struct &Loop);
  template <typename Struct> bool GetLeftSideBO(Stmt *s, Struct &Var);

  // DETECT MEMORY ACCESS
  template <typename T> void searchMemAcc(Stmt *s, T &Loop);
  template <typename T> bool searchMemOperator(Stmt *s, T &Loop);
  template <typename T>
  bool getOperator(Stmt *s, T &Loop, ASTMemoryAccess &newMemAcc,
                   int &dimension);
  template <typename T>
  bool getIterator(Stmt *s, T &Loop, ASTMemoryAccess &newMemAcc);
  template <typename T>
  bool searchArrayAcc(Stmt *s, T &Loop, ASTMemoryAccess &newMemAcc);
  // SEARCH FUNCTIONS
  void searchFunc(Stmt *s, ASTLoop &Loop);
  void getArguments(Stmt *s, ASTFunction *f);
  void searchFunc(Stmt *s, ASTFunctionDecl &Loop);

  // CLEAN ATTRIBUTES
  void clean();
  // METRICS // NO
  template <typename T> void getMetrics(Stmt *s, T &Loop);
  template <typename T> void binaryOperatorMetric(Stmt *s, T &Loop);
  void calculateCyclomaticComplexity(Stmt *s, int &McCC);
  void CalculateLNL(int i);

  // Get variable references
  bool VisitDeclRefExpr(DeclRefExpr *s); 

  // Extract the number of statements for a given fragment of code
  int getNumOps(Stmt *s);

  // Data collection if the loop does not contains a compound statement
  bool nonCompundLoop(Stmt *s, ASTLoop &Loop);

  // Data collection for compound statements
  bool PipeLineDetector(Stmt *s, ASTLoop &Loop);

  bool KernelDetectorLoop(Stmt *s, ASTLoop &Kernel);
  bool KernelDetector(Stmt *s, ASTLoop &Kernel);

  bool getIterator(Stmt *s, ASTLoop &currentLoop); 

  void getStatementLocation(Stmt *s, ASTLoop &lp);

  int loops = 0;

  // General visitor
  bool VisitStmt(Stmt *s);

  bool VisitFunctionDecl(FunctionDecl *f);

  bool TraverseDecl(Decl *D);

  ~MyASTVisitor();
};

/* include template implementation */

#include "getMetrics.tpp"
#include "getCondition.tpp"
#include "searchVars.tpp"
#include "searchMemoryAccess.tpp"

#endif
