std::vector<std::string> MyASTVisitor::getOtherIterators ( int i) {
  std::vector<std::string> iterators;
  for(auto j = 0; j< Loops.size(); j++){
    for(auto inLoop = 0; i!=j && inLoop != Loops[i].InternLoops.size(); inLoop++){
      if(Loops[i].InternLoops[inLoop].getBegin().getRawEncoding() <= Loops[j].RangeLoc.getBegin().getRawEncoding()  &&
          Loops[i].InternLoops[inLoop].getEnd().getRawEncoding() >= Loops[j].RangeLoc.getBegin().getRawEncoding()){
        iterators.push_back(Loops[j].Iterator.Name);
      }
    }
  }
  return iterators;

}

bool MyASTVisitor::checkNeightbor(std::vector<std::string>& inputs, std::vector<std::string> iterators, int i){
  bool usesIt = false;
  bool usesOtherIt = false;
  for(auto refs = Loops[i].VarRef.begin();refs != Loops[i].VarRef.end();refs++)
  {  
    for(auto names = inputs.begin(); names != inputs.end(); names++){
      if((*names) == (*refs).Name){
        for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
          if((*refs).Name == Loops[i].MemAccess[memAcc].Name)
          {  
            for(auto index = 0; index < Loops[i].MemAccess[memAcc].index.size(); index++){
              if(Loops[i].MemAccess[memAcc].index[index] == Loops[i].Iterator.Name){
                usesIt=true;
              }
              for(auto iter = 0; iter < iterators.size(); iter++){
                if(Loops[i].MemAccess[memAcc].index[index] ==iterators[iter]){
                  usesOtherIt=true;
                }
              }
            }
          }
        }
      }
    }
  }
  return (usesIt && usesOtherIt);
}

std::vector<std::string> MyASTVisitor::checkKernel(std::vector<std::string>& inputs,std::vector<std::string> iterators, int i){
  std::vector<std::string> kernels;
  for(auto refs = Loops[i].VarRef.begin();refs != Loops[i].VarRef.end();refs++)
  {
    for(auto names = inputs.begin(); names != inputs.end(); names++){
      if((*names) == (*refs).Name){
        for(unsigned memAcc = 0; memAcc<Loops[i].MemAccess.size(); memAcc++){
          if((*refs).Name == Loops[i].MemAccess[memAcc].Name)
          {
            bool isKernel = true;
            bool usesOtherIt= false;
            for(auto index = 0; index < Loops[i].MemAccess[memAcc].index.size(); index++){
              if(Loops[i].MemAccess[memAcc].index[index] == Loops[i].Iterator.Name){
                isKernel=false;
              }
              for(auto iter = 0; iter < iterators.size(); iter++){
                if(Loops[i].MemAccess[memAcc].index[index] ==iterators[iter]){
                  usesOtherIt=true;
                }
              }
            }
            if(isKernel&&usesOtherIt)
              kernels.push_back((*refs).Name);

          }
        }
      }
    }
  }
  return kernels;
}




void MyASTVisitor::stencilDetect(){
  //Analize function declarations with bodies
  analyzeCodeFunctions();
  int numStencil = 0;
  //int numpipelines = 0;
  for(int i = 0; i < numLoops;i++){
    SourceManager &SM = TheRewriter.getSourceMgr();
    auto fileName = Loops[i].FileName;
    auto currfile = fileName;
    //Annotate only if the loop is in the current file. 
    if(fileName == currfile){
      std::stringstream SSComments;		
      bool comments = false;
      //Analyze function calls
      analyzeCall(i);
      //Check for L/Rvalue
      analyzeLRLoop(i);
      //Global variable LValue
      bool globallvalue = globalLValue(i);

      std::vector<std::string> outputs;
      std::vector<std::string> inputs;
      //Get inputs and outputs
      getMapIO(inputs, outputs,i);
      std::vector<std::string> outStreams;
      std::vector<std::string> inStreams;

      //Get IO data streams
      getDataStreams(inputs, inStreams,i);
      getDataStreams(outputs, outStreams,i);

      auto iterators = getOtherIterators (i);
      auto neigh = checkNeightbor(inputs,iterators, i);
      auto kernels = checkKernel(inputs,iterators, i);
      bool convolution = false;
      if(kernels.size()>0) convolution = true;
      //Check feedback        
      bool feedback = checkFeedback(i,SSComments,comments);

      //Get output data stream dependencies
      getStreamDependencies(outStreams,i);
      //Check if output streams are only used as write or read in the same position. 
      checkMemAccess(outStreams,i);	
      bool invalid = true;
      if(outputs.size()>0&&inputs.size()>0){
        invalid = false;
      }
      if(Loops[i].type == 1){
        invalid =true;
      }	
      bool hasreturn = false;	
      if(Loops[i].Returns.size()>0){
        hasreturn=true;
      }
      bool hasbreak = check_break(i);

      bool hasgoto = false;
      if(Loops[i].Gotos.size()>0){
        hasgoto = true;
      }
      bool empty = false;
      if(Loops[i].numOps == 0) empty=true;

      if(!invalid&&!feedback&&!globallvalue&&!hasreturn&&!empty&&!hasgoto&&!hasbreak&&neigh){
        Loops[i].stencil = true;
        if(convolution&&ConvolutionOption){
          Loops[i].convolution = true;
        }
        numStencil++;
        //ADD I/O IN LOOPS
        if(!Loops[i].map){
          for(unsigned in=0;in<inputs.size();in++){
            Loops[i].inputs.push_back(inputs[in]);
          }
          for(unsigned out=0;out<outputs.size();out++){
            Loops[i].outputs.push_back(outputs[out]);
          }
        }

      }else{
        std::stringstream SSBefore;
        auto line = SM.getPresumedLineNumber(Loops[i].RangeLoc.getBegin());

        if(ConvolutionOption){
          SSBefore << "== Loop in "<<fileName<< ":"<<line<<" does not match a stencil/convolution pattern!\n";

          if(!convolution) SSBefore<<"\tConvolution kernel not detected.\n ";
        }else{
          SSBefore << "== Loop in "<<fileName<< ":"<<line<<" does not match a stencil pattern!\n";
        }
        if(feedback) SSBefore<<"\tFeedback detected.\n ";
        if(invalid)  SSBefore<<"\tOutput does not depend on any Input\n";
        if(globallvalue) SSBefore<<"\tFound a write on a global variable.\n";
        if(hasreturn) SSBefore<<"\tFound a return statement.\n";
        if(hasbreak) SSBefore<<"\tFound break statement.\n";
        if(hasgoto) SSBefore<<"\tFound goto statement.\n";
        if(!neigh) SSBefore<<"\tDoes not access to neighbor data.\n";
        std::cout<<SSBefore.str();

      }
    }
  }
  std::cout<<"Stencil patterns detected : "<<numStencil;
  std::cout<<std::flush;
}
