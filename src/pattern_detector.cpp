//------------------------------------------------------------------------------
// Tooling sample. Demonstrates:
//
// * How to write a simple source tool using libTooling.
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "MyASTVisitor.hpp"
#include "ASTInfo.h"
#include "HotSpots.h"
#include "dictionary.h"
#include "shared_ppat_variables.hpp"
#include "stdextend.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/AttrKinds.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/LiteralSupport.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

//#include "clang/Lex/Lexer.h"

#define FUNCTION_DICTIONARY_PATH "../include/functionDictionary.txt"
#define WHITE_LIST_PATH "../include/whiteList.txt"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

std::vector<ASTFunctionDecl> Functions;
std::map<std::string, bool> analyzedFuncs;
std::vector<DictionaryFunction> dictionary;
std::vector<int> dictionaryAdds;
std::vector<std::string> whiteList;

llvm::cl::OptionCategory
    ToolingSampleCategory("Parallel Pattern Analyzer Tool");
// static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

llvm::cl::extrahelp
    MoreHelp("\nExecution command: ./PPAT [source_files] [options] -- "
             "[compiler_flags] \n");

llvm::cl::opt<bool>
    PipelineOption("pipeline", llvm::cl::desc("Detect and analyze pipelines"),
                   llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool> MapOption("map",
                              llvm::cl::desc("Detect and analyze map patterns"),
                              llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool>
    ReduceOption("reduce", llvm::cl::desc("Detect and analyze reduce patterns"),
                 llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool>
    FarmOption("farm", llvm::cl::desc("Detect and analyze farm patterns"),
               llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool> omp("omp",
                        llvm::cl::desc("Parallelize map patterns using OpenMP"),
                        llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool>
    grppi("grppi", llvm::cl::desc("Transform the original code to GrPPI"),
          llvm::cl::cat(ToolingSampleCategory));
// static llvm::cl::opt<bool> MapReduceOption("mapreduce",llvm::cl::desc("Detect
// and analyze map-reduce patterns"), llvm::cl::cat(ToolingSampleCategory));
llvm::cl::opt<bool> CleanOption("clean",
                                llvm::cl::desc("Erase pattern attributes"),
                                llvm::cl::cat(ToolingSampleCategory));

bool firstIteration = false;

// Get variable references
bool MyASTVisitor::VisitDeclRefExpr(DeclRefExpr *s) {
  /*  if(s->getDecl()!= NULL){
      if(isa<VarDecl>(s->getDecl())){
          VarDecl *var = cast<VarDecl>(s->getDecl());
          ASTVar newVarRef;
          newVarRef.Name =
 std::string(var->getIdentifier()->getNameStart());
      newVarRef.RefLoc = s->getLocStart().getRawEncoding();
          unsigned location = var->getLocStart().getRawEncoding();
                      newVarRef.DeclLoc = location;
          Code.VarRef.push_back(newVarRef);
      }
  }*/
  return true;
}

// Extract the number of statements for a given fragment of code
int MyASTVisitor::getNumOps(Stmt *s) {
  int numOps = 0;
  if (s->children().begin() != s->children().end()) {
    for (Stmt::child_iterator child = s->child_begin(); child != s->child_end();
         child++) {
      numOps++;
    }
  }
  return numOps;
}

// Data collection for compound statements
bool MyASTVisitor::PipeLineDetector(Stmt *s, ASTLoop &Loop) {
  if (s != nullptr) {
    if (isa<WhileStmt>(s) || isa<ForStmt>(s)) {
      Expr *condition;
      if (isa<WhileStmt>(s)) {
        WhileStmt *lp = cast<WhileStmt>(s);
        condition = lp->getCond();
      }
      if (isa<ForStmt>(s)) {
        ForStmt *lp = cast<ForStmt>(s);
        condition = lp->getCond();
      }
      getCondition(condition, Loop);
      /*= s->getCond();*/
      // std::cout<<"CONDITION \n";
    }
    // If is a declstmt is the iterator
    if (isa<DeclStmt>(s)) {
      DeclStmt *dec = cast<DeclStmt>(s);
      for (auto decIt = dec->decl_begin(); decIt != dec->decl_end(); decIt++) {
        if ((*decIt) != nullptr) {
          if (VarDecl *var = dyn_cast<VarDecl>((*decIt))) {
            Loop.Iterator.Name = var->getNameAsString();
            Loop.Iterator.DeclLoc = var->getLocStart().getRawEncoding();
          }
        }
      }
      // Store iterators declaration.
      searchVars(s, Loop);
    }
    if (isa<DeclRefExpr>(s)) {
      DeclRefExpr *ref = cast<DeclRefExpr>(s);
      if (ref->getDecl() != NULL) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          Loop.Iterator.Name = var->getNameAsString();
        }
      }
    }
    // If is a compound statement is the body of the loop.
    if (isa<CompoundStmt>(s)) {
      // Store location
      Loop.InitLoc = s->getLocStart().getLocWithOffset(2);
      // Search for var and function
      searchVars(s, Loop);
      searchFunc(s, Loop);
      searchMemAcc(s, Loop);
      Loop.numOps = getNumOps(s);
      //          getMetrics(s,Loop);
      return true;
    }
    if (s->children().begin() != s->children().end()) {
      // Else Look for the childrens
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {
        PipeLineDetector(*child, Loop);
      }
    }
  }
  return false;
}

bool MyASTVisitor::KernelDetectorLoop(Stmt *s, ASTLoop &Kernel) {
  if (s != NULL) {

    if (isa<DeclStmt>(s)) {
      DeclStmt *dec = cast<DeclStmt>(s);
      for (auto decIt = dec->decl_begin(); decIt != dec->decl_end(); decIt++) {
        if ((*decIt) != NULL) {
          if (VarDecl *var = cast<VarDecl>((*decIt))) {
            unsigned location = var->getLocStart().getRawEncoding();
            Kernel.Iterator.Name = var->getNameAsString();
            Kernel.Iterator.type =
                clang::QualType::getAsString(var->getType().split());
            Kernel.Iterator.DeclLoc = location;
            Kernel.Iterator.RefLoc = location;
          }
        }
      }
      searchVars(s, Kernel);
    }
    if (isa<CompoundStmt>(s)) {
      Kernel.InitLoc = s->getLocStart().getLocWithOffset(2);
      searchVars(s, Kernel);
      searchFunc(s, Kernel);
      searchMemAcc(s, Kernel);
      //          searchMemAcc(s,Kernel);
      //                        Kernel.numOps = getNumOps(s);
      //          getMetrics(s,Kernel);
      return true;
    }
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {

        KernelDetectorLoop(*child, Kernel);
      }
    }
  }
  return false;
}

bool MyASTVisitor::KernelDetector(Stmt *s, ASTLoop &Kernel) {
  if (s != NULL) {
    // s->dump();
    if (isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<CompoundStmt>(s)) {
      KernelDetectorLoop(s, Kernel);
      return true;
    } else {
      Kernel.InitLoc = s->getLocStart().getLocWithOffset(2);
      searchVars(s, Kernel);
      searchFunc(s, Kernel);
      searchMemAcc(s, Kernel);
      //                      Kernel.numOps = getNumOps(s);
      //          getMetrics(s,Kernel);
      return true;
    }
  }
  return false;
}

bool MyASTVisitor::getIterator(Stmt *s, ASTLoop &currentLoop) {
  // std::cout<<"GET ITERATOR\n";
  if (s != nullptr) {
    if (isa<DeclStmt>(s)) {
      //      std::cout<<"DECL\n";
      DeclStmt *dec = cast<DeclStmt>(s);
      for (auto decIt = dec->decl_begin(); decIt != dec->decl_end(); decIt++) {
        if ((*decIt) != NULL) {
          if (VarDecl *var = cast<VarDecl>((*decIt))) {
            //          std::cout<<"VAR NAME: "<<
            //          var->getNameAsString()<<"\n";
            unsigned location = var->getLocStart().getRawEncoding();
            currentLoop.Iterator.Name = var->getNameAsString();
            currentLoop.Iterator.type =
                clang::QualType::getAsString(var->getType().split());
            currentLoop.Iterator.DeclLoc = location;
            currentLoop.Iterator.RefLoc = location;
          }
        }
      }
      return true;
      //     searchVars(s,Kernel);
    }
    if (isa<DeclRefExpr>(s)) {
      //      std::cout<<"REF\n";
      DeclRefExpr *ref = cast<DeclRefExpr>(s);
      if (ref->getDecl() != NULL) {
        if (VarDecl *var = dyn_cast<clang::VarDecl>(ref->getDecl())) {
          //              std::cout<<"VAR NAME: "<<
          //              var->getNameAsString()<<"\n";

          currentLoop.Iterator.Name = var->getNameAsString();
          currentLoop.Iterator.type =
              clang::QualType::getAsString(var->getType().split());
          unsigned location = var->getLocStart().getRawEncoding();
          currentLoop.Iterator.DeclLoc = location;
          currentLoop.Iterator.RefLoc = ref->getLocStart().getRawEncoding();
        }
      }
      return true;
    }
    if (s->children().begin() != s->children().end()) {
      for (Stmt::child_iterator child = s->child_begin();
           child != s->child_end(); child++) {
        bool found = getIterator(*child, currentLoop);
        if (found)
          return true;
      }
    }
    return false;
  }
  return false;
}

void MyASTVisitor::getStatementLocation(Stmt *s, ASTLoop &lp) {
  if (s->children().begin() != s->children().end()) {
    for (Stmt::child_iterator child = s->child_begin(); child != s->child_end();
         child++) {
      lp.stmtLoc.push_back((*child)->getSourceRange());
    }
  }
}

bool MyASTVisitor::VisitStmt(Stmt *s) {
  searchVars(s, Code);
  //  searchFunc(s,Code);
  //  SourceManager &SM = TheRewriter.getSourceMgr();
  //      newFun.FileName =
  //      SM.getFileEntryForID(SM.getDecomposedLoc(newFun.RangeLoc.getEnd()).first)->getName();

  // If is a for
  if (isa<ForStmt>(s)) {
    // Store data for pipeline detector
    Stmt *Statement = cast<Stmt>(s);
    ASTLoop actualLoop;
    actualLoop.type = 0;
    actualLoop.LoopRef = Statement;
    actualLoop.RangeLoc = Statement->getSourceRange();
    actualLoop.LoopLoc = Statement->getLocStart();
    SourceManager &SM = TheRewriter.getSourceMgr();
    clang::PresumedLoc presumed = SM.getPresumedLoc(actualLoop.LoopLoc);
    actualLoop.FileName = presumed.getFilename();
    // actualLoop.FileName =
    // SM.getFileEntryForID(SM.getDecomposedLoc(actualLoop.RangeLoc.getEnd()).first)->getName();

    // ESTA PARTE NO ME CONVENCE. TAL VEZ SE PUEDA MODIFICAR PARA QUE SEA
    // COMPOUND O NO FUNCIONE EL MISMO SISTEMA. AHORA MISMO NO TENGO CLARO
    // COMO HACERLO; PERO LOOPS DE UNA LINEA NO CAPTURAN SUS ITERADORES.
    bool compound = PipeLineDetector(Statement, actualLoop);
    if (!compound) {
      getIterator(s, actualLoop);
      ForStmt *loop = cast<ForStmt>(s);
      Stmt *body = loop->getBody();
      nonCompundLoop(body, actualLoop);
    }
    bool add = true;
    std::vector<ASTLoop> LoopsBack{};
    for (auto l = Loops.begin(); l != Loops.end(); l++) {
      if ((actualLoop.RangeLoc.getBegin() != (*l).RangeLoc.getBegin()) ||
          (actualLoop.RangeLoc.getEnd() != (*l).RangeLoc.getEnd())) {
        LoopsBack.push_back(*l);
      }
    }
    Loops = std::move(LoopsBack);
    if (add) {
      Loops.push_back(actualLoop);
      numLoops = Loops.size();
    }
    return true;
  }
  // If is a while loop
  if (isa<WhileStmt>(s)) {
    // Store data for pipeline detector
    Stmt *Statement = cast<Stmt>(s);

    WhileStmt *loop = cast<WhileStmt>(s);
    ASTLoop actualLoop;
    actualLoop.LoopRef = Statement;
    actualLoop.RangeLoc = Statement->getSourceRange();
    actualLoop.LoopLoc = Statement->getLocStart();
    SourceManager &SM = TheRewriter.getSourceMgr();
    clang::PresumedLoc presumed = SM.getPresumedLoc(actualLoop.LoopLoc);
    actualLoop.FileName = presumed.getFilename();
    actualLoop.type = 1;
    Expr *condition = loop->getCond();
    Stmt *body = loop->getBody();
    getStatementLocation(body, actualLoop);
    actualLoop.conditionRange = condition->getSourceRange();
    actualLoop.bodyRange = body->getSourceRange();
    //        getLoopCondition(condition, actualLoop);

    //      actualLoop.FileName =
    //      SM.getFileEntryForID(SM.getDecomposedLoc(actualLoop.RangeLoc.getEnd()).first)->getName();

    // APLICA LO MISMO QUE PARA EL BUCLE FOR. REVISAR.
    bool compound = PipeLineDetector(Statement, actualLoop);
    if (!compound) {
      nonCompundLoop(body, actualLoop);
    }
    bool add = true;
    std::vector<ASTLoop> LoopsBack{};
    for (auto l = Loops.begin(); l != Loops.end(); l++) {
      if ((actualLoop.RangeLoc.getBegin() != (*l).RangeLoc.getBegin()) ||
          (actualLoop.RangeLoc.getEnd() != (*l).RangeLoc.getEnd())) {
        // Kernels.erase(l);
        LoopsBack.push_back(*l);
      }
    }
    Loops = std::move(LoopsBack);
    if (add) {
      Loops.push_back(actualLoop);
      numLoops = Loops.size();
    }
    return true;
  }
  // If is an attributed statement
  if (isa<AttributedStmt>(s)) {
    AttributedStmt *attr = cast<AttributedStmt>(s);
    auto attributes = attr->getAttrs();
    bool added = false;
    bool farm = false;
    bool map = false;
    bool pipeline = false;
    // Access every attribute defined
    for (auto it = attributes.begin(); it != attributes.end(); it++) {
      if ((*it)->getKind() == attr::Kind::RPRPipeline) {
        pipeline = true;
        if (added) {
          Kernels[Kernels.size() - 1].pipeline = true;
        }
      }
      if ((*it)->getKind() == attr::Kind::RPRMap) {
        map = true;
        if (added) {
          Kernels[Kernels.size() - 1].map = true;
        }
      }
      if ((*it)->getKind() == attr::Kind::RPRFarm) {
        farm = true;
        if (added) {
          Kernels[Kernels.size() - 1].farm = true;
        }
      }

      // Check attribute kind
      if ((*it)->getKind() == attr::Kind::RPRKernel) {

        // If is RPRKernel
        for (Stmt::child_iterator ci = s->child_begin(); ci != s->child_end();
             ci++) {
          Stmt *Statement = cast<Stmt>(*ci);
          ASTLoop actualKernel;
          actualKernel.RangeLoc = Statement->getSourceRange();
          actualKernel.LoopLoc = Statement->getLocStart();
          KernelDetector(Statement, actualKernel);
          if (farm)
            actualKernel.farm = true;
          if (map)
            actualKernel.map = true;
          if (pipeline)
            actualKernel.pipeline = true;
          bool add = true;
          std::vector<ASTLoop> KernelsBack{};
          for (auto l = Kernels.begin(); l != Kernels.end(); l++) {
            if ((actualKernel.RangeLoc.getBegin() !=
                 (*l).RangeLoc.getBegin()) ||
                (actualKernel.RangeLoc.getEnd() != (*l).RangeLoc.getEnd())) {
              // Kernels.erase(l);
              KernelsBack.push_back(*l);
            }
          }
          Kernels = std::move(KernelsBack);
          // KernelsBack.cleari();

          if (add) {
            Kernels.push_back(actualKernel);
            added = true;
          }
        }
      }
    }
  }
  return true;
}

bool MyASTVisitor::VisitFunctionDecl(FunctionDecl *f) {
  if (firstIteration) {
    // Only function definitions (with bodies), not declarations.
    if (f->hasBody()) {
      ASTFunctionDecl newFun;
      newFun.Name = f->getNameAsString();
      // If is not the main
      if (newFun.Name != "main") {
        /*if(isa<CXXConversionDecl>(f)||isa<CXXConstructorDecl>(f)){
//          DeclContext* cntx = castToDeclContext(f);
            return true;
        }*/
        // Store function as accessible function
        newFun.FuncDecl = f->getSourceRange().getBegin();
        newFun.RangeLoc = f->getSourceRange();
        // Get file name
        SourceManager &SM = TheRewriter.getSourceMgr();
        newFun.FileName =
            SM.getFileEntryForID(
                  SM.getDecomposedLoc(newFun.RangeLoc.getEnd()).first)
                ->getName();
        newFun.startLine = SM.getLineNumber(
            SM.getDecomposedLoc(newFun.RangeLoc.getBegin()).first,
            newFun.RangeLoc.getBegin().getRawEncoding());
        newFun.endLine = SM.getLineNumber(
            SM.getDecomposedLoc(newFun.RangeLoc.getEnd()).first,
            newFun.RangeLoc.getEnd().getRawEncoding());

        //      newFun.FileName = SM.getFilename(newFun.FuncDecl).str();
        // Get arguments
        unsigned numParam = f->getNumParams();
        for (unsigned i = 0; i < numParam; i++) {
          ASTVar arg;
          arg.Name = f->getParamDecl(i)->getNameAsString();
          newFun.Arguments.push_back(arg);
        }
        // Serach variables and functions
        Stmt *body = f->getBody();
        searchVars(body, newFun);
        searchFunc(body, newFun);
        // getMetrics(body,newFun);
        bool add = true;
        for (unsigned l = 0; l < Functions.size(); l++) {
          if (newFun.RangeLoc.getBegin() == Functions[l].RangeLoc.getBegin() &&
              newFun.RangeLoc.getEnd() == Functions[l].RangeLoc.getEnd() &&
              newFun.Name == Functions[l].Name)
            add = false;
        }
        if (add)
          Functions.push_back(newFun);
      }
    }
    return true;
  }
  return true;
}

bool MyASTVisitor::TraverseDecl(Decl *D) {
  bool rval;
  SourceManager &SM = TheRewriter.getSourceMgr();
  if (!D) {
    return true;
  }
  if (!SM.isInSystemHeader(D->getLocation()) ||
      std::string(D->getDeclKindName()) == "TranslationUnit") {
    rval = clang::RecursiveASTVisitor<MyASTVisitor>::TraverseDecl(D);
  } else {
    rval = true;
  }
  return rval;
}

// Data collection if the loop does not contains a compound statement
bool MyASTVisitor::nonCompundLoop(Stmt *s, ASTLoop &Loop) {
  // Store location
  Loop.InitLoc = s->getLocStart().getLocWithOffset(2);
  // Search for var and function
  searchVars(s, Loop);
  searchFunc(s, Loop);
  searchMemAcc(s, Loop);
  Loop.numOps = getNumOps(s);
  // getMetrics(s,Loop);
  return true;
}

// Analyze the colected data on the second iteration and destroys the visitor
MyASTVisitor::~MyASTVisitor() {
  if (!firstIteration) {
    //      llvm::errs()<<"---STARTING ANALYSIS---\n";
    std::cout << "\n\nStarting map detection phase... \n";
    if (MapOption)
      mapDetect();
    std::cout << "\n\nStarting farm detection phase... \n";
    if (FarmOption)
      farmDetect();
    std::cout << "\n\nStarting reduce detection phase...\n";
    if (ReduceOption)
      reduceDetect();
    std::cout << "\n\nStarting pipeline detection phase... \n";
    if (PipelineOption)
      AnalyzeLoops();
    if (CleanOption)
      clean();
    if (!PipelineOption)
      PPATAnnotate();
    if (grppi) {
      PPATtoGRPPI();
    }
    TheRewriter.overwriteChangedFiles();
    //      llvm::errs()<<"---ANALYSIS FINISHED---\n";
  }
}

//----------------WORK IN PROGRESS--------------
//----------------------------------------------
//
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      Visitor.TraverseDecl(*b);
    }
    return true;
  }

private:
  MyASTVisitor Visitor;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    if (!firstIteration)
      std::cout << "** EndSourceFileAction for: "
                << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
    auto consumer = getCompilerInstance().getASTConsumer();
    consumer.PrintStats();
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    if (!firstIteration)
      std::cout << "** Creating AST consumer for: " << file.str() << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

/**
 *  @brief parse into a @link dictionary @endlink the information about the
 *      argument used in non accesible functions
 *
 *  Read from the file FUNCTION_DICTIONARY_PATH the information
 *  about each non accessible functions. Then store the data in a list in which
 *  each entry has the format "name: function_name", "argv[arg_index]: kind"
 *  where kind can be:
 *      L  = Left or Write Value
 *      R  = Rigth or Read Value
 *      LR = Read and Write Value
 *      U  = Unknown argument
 *
 *  NOTE: The file must be filled by the final user in order to tell the
 *      program how should be used each non accesible function argument.
 */
void readDictionary() {
  std::string line;
  std::ifstream myfile(FUNCTION_DICTIONARY_PATH);
  if (myfile.is_open()) {
    while (getline(myfile, line)) {
      std::vector<std::string> tokens = split(line, ' ');
      if (tokens.size() > 0) {
        DictionaryFunction newFun;
        newFun.Name = tokens[0];
        for (unsigned i = 1; i < tokens.size(); i++) {
          std::vector<std::string> arguments = split(tokens[i], ':');
          int num = atoi(arguments[0].c_str());
          kind k;
          if (arguments[1] == "L") {
            k = L;
          }
          if (arguments[1] == "R") {
            k = R;
          }
          if (arguments[1] == "LR") {
            k = LR;
          }
          if (arguments[1] == "U") {
            k = U;
          }
          newFun.arguments[num] = k;
        }
        dictionary.push_back(newFun);
      }
    }
    myfile.close();
  }
}

/**
 *  @brief parse into a list the information about the functions that can be
 *      parallelized
 *
 *  Read from the file WHITE_LIST_PATH the name of each non
 *  accessible functions that can be paralelized. The name is store in a list
 *  @link whiteList @endlink, and all the functions in this list will be
 *  used as parallelizable functions.
 *
 *
 *  NOTE: The file WHITE_LIST_PATH must be filled by the final user in order to
 *      tell the program if a non accesible function is or not a accesible
 *      function.
 */
void readWhiteList() {
  std::string line;
  std::ifstream myfile(WHITE_LIST_PATH);
  if (myfile.is_open()) {
    while (getline(myfile, line)) {
      whiteList.push_back(line);
    }
    myfile.close();
  }
}

/**
 *  @brief stores non accesible functions detected in the code indicating the
 *      unknown argument kinds
 *
 *
 *  Write in the file FUNCTION_DICTIONARY_PATH the name of each non
 *  accessible functions whose arguments are unknown
 */
void writeDictionary() {
  std::ofstream ofs;
  ofs.open(FUNCTION_DICTIONARY_PATH, std::ofstream::out | std::ofstream::app);
  for (unsigned i = 0; i < dictionaryAdds.size(); i++) {
    std::stringstream function;
    function << "\n";
    function << dictionary[dictionaryAdds[i]].Name;
    for (auto it = dictionary[dictionaryAdds[i]].arguments.begin();
         it != dictionary[dictionaryAdds[i]].arguments.end(); it++) {
      function << " " << (it->first) << ":U";
    }
    ofs << function.str();
  }
  ofs.close();
}

int main(int argc, const char **argv) {
  // Read from dictionaries and parse options
  readDictionary();
  readWhiteList();
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  // pipelineDetect = PipelineOption;

  // Preform a first data collection phase
  firstIteration = true;
  Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  // Then perform the analysis phase
  firstIteration = false;
  Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  // Write new dictionary entries
  if (dictionaryAdds.size() > 0)
    writeDictionary();

  return 1;
}
