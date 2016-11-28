/* ---------------------------------------------------*
 | Copyright 2016. Universidad Carlos III de Madrid.  |
 |Todos los derechos reservados / All rights reserved |
 *----------------------------------------------------*/
#pragma once

#include <string>
#include <vector>
#include <map>

#include "clang/Basic/SourceLocation.h" /* clang::SourceLocation type */
#include "clang/AST/Stmt.h"             /* clang::stmt type */

// Dependencies between variables
struct ASTRef {
  std::string Name;
  unsigned DeclLoc;
  unsigned RefLoc;
};

struct ASTVar {
  // Name and Declaration Location
  std::string Name;
  std::string type;
  unsigned DeclLoc;
  // Variable type
  bool lvalue = false;
  bool rvalue = false;
  bool xvalue = false;
  bool glvalue = false;
  bool globalVar = false;
  bool cmpAssign = false;
  bool memberExpr = false;
  bool opModifier = false;
  std::string cmpOper;
  unsigned RefLoc;
  clang::SourceLocation RefSourceLoc;
  clang::SourceLocation RefEndLoc;
  bool sureKind = false;
  std::vector<ASTRef> dependencies;
};

struct ASTMemoryAccess {
  std::string Name;
  int dimension;
  std::vector<std::string> index;
  unsigned RefLoc;
};

struct ASTFunction {
  std::string Name;
  std::string returnType;
  std::vector<ASTVar> Args;
  std::vector<std::string> paramTypes;
  unsigned startLine;
  unsigned endLine;
  unsigned FunLoc;
  clang::SourceLocation SLoc;
  clang::SourceLocation EndLoc;
  bool returnUsed;
};

struct ASTOperators {
  std::string Name;
  clang::SourceLocation Location;
  std::string type;
};

struct ASTCondition {
  std::vector<ASTVar> VarDecl;
  std::vector<ASTVar> VarRef;
  std::vector<ASTFunction> fCalls;
  std::vector<ASTOperators> ops;
};

struct ASTLoop {
  clang::Stmt *LoopRef;
  clang::SourceRange RangeLoc;
  clang::SourceLocation LoopLoc;
  clang::SourceLocation InitLoc;
  clang::SourceLocation genStart;
  clang::SourceLocation genEnd;
  std::map<std::string, std::string> genVar;
  std::map<std::string, std::string> taskVar;
  std::map<std::string, ASTVar> privatedVars;
  std::map<std::string, std::pair<clang::SourceLocation, clang::SourceLocation>>
      sinkZones;
  bool genBefore = false;
  bool genAfter = false;

  std::vector<clang::SourceRange> stmtLoc;
  clang::SourceRange conditionRange;
  clang::SourceRange bodyRange;
  std::vector<ASTVar> VarDecl;
  std::vector<ASTVar> VarRef;
  std::vector<ASTFunction> fCalls;
  ASTCondition cond;
  int numOps;
  ASTVar Iterator;
  std::vector<ASTMemoryAccess> MemAccess;
  std::vector<ASTVar> Assign;
  std::string FileName;

  std::vector<clang::SourceRange> InternLoops;
  std::vector<clang::SourceRange> ConditionalsStatements;
  std::vector<clang::SourceLocation> CondEnd;

  std::vector<clang::SourceLocation> BodyStarts;

  std::vector<clang::SourceLocation> Returns;
  std::vector<clang::SourceLocation> Breaks;
  std::vector<clang::SourceLocation> Gotos;

  std::vector<std::string> privated;
  std::vector<std::string> reduceVar;
  std::vector<std::string> operators;

  bool hasRecursive = false;
  // METRICS
  int EXP = 0;
  int ARR = 0;
  int MUL = 0;
  int ADD = 0;
  int McCC = 1;
  int LNL = 1;
  double timePerf = 0;
  // Specifies if its a for loop (type == 0) or a while loop (type == 1)
  int type;

  std::string insideFunction = "";
  double score = 0;
  bool pipeline = false;
  bool farm = false;
  bool reduce = false;
  bool map = false;
  // MAP I/O
  std::vector<std::string> outputs;
  std::vector<std::string> inputs;
  std::vector<std::string> inputTypes;
  std::vector<std::string> outputTypes;
};

struct ASTFunctionDecl {
  std::string Name;
  std::string returnType;
  clang::SourceRange RangeLoc;
  clang::SourceLocation FuncDecl;
  std::string FileName;

  std::vector<ASTVar> VarDecl;
  std::vector<ASTVar> VarRef;
  std::vector<ASTFunction> fCalls;
  std::vector<ASTVar> Arguments;
  std::vector<clang::SourceRange> InternLoops;
  std::vector<clang::SourceRange> ConditionalsStatements;
  std::vector<clang::SourceLocation> CondEnd;
  std::vector<ASTVar> Assign;

  std::vector<clang::SourceLocation> Returns;

  unsigned startLine;
  unsigned endLine;
  unsigned numCalls;
  std::vector<ASTMemoryAccess> MemAccess;
  std::vector<int> KernelExecutionOrder;
  bool globalLvalue = false;
  std::vector<std::string> globalsUsed;
  std::vector<std::string> globalsWrited;
  bool partialAccess = false;
  bool recursive = false;
  // METRICS
  int EXP = 0;
  int ARR = 0;
  int MUL = 0;
  int ADD = 0;
  int McCC = 1;
  int LNL = 0;
  double timePerf = 0;
};
