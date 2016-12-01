#ifndef SHARED_PPAT_VARIABLES_H
#define SHARED_PPAT_VARIABLES_H

#include "dictionary.h"
#include "llvm/Support/CommandLine.h"

extern std::vector<ASTFunctionDecl> Functions;
extern std::map<std::string, bool> analyzedFuncs;
extern std::vector<DictionaryFunction> dictionary;
extern std::vector<int> dictionaryAdds;
extern std::vector<std::string> whiteList;

/* command line argument */
extern llvm::cl::OptionCategory ToolingSampleCategory;
extern llvm::cl::extrahelp MoreHelp;
extern llvm::cl::opt<bool> PipelineOption;
extern llvm::cl::opt<bool> MapOption;
extern llvm::cl::opt<bool> ReduceOption;
extern llvm::cl::opt<bool> FarmOption;
extern llvm::cl::opt<bool> omp;
extern llvm::cl::opt<bool> grppi;
extern llvm::cl::opt<bool> CleanOption;

#endif
