bool MyASTVisitor::checkPool(int i,int stages,std::vector<unsigned> stagesLocEnds){
  std::vector<int> feedbackStages(stages+1,0);
  std::vector<std::string> analyzedVars;
  for(unsigned dec = 0;dec<Loops[i].VarDecl.size();dec++){
    analyzedVars.push_back(Loops[i].VarDecl[dec].Name);
  } 

  for(auto refs = (Loops[i].VarRef.end()-1);refs != (Loops[i].VarRef.begin()-1);refs--){
    bool notAnalized = true;
    for(auto doneVars = analyzedVars.begin(); doneVars != analyzedVars.end(); doneVars++){
      if((*doneVars) == (*refs).Name) notAnalized = false;
    }
    if(notAnalized&&(*refs).lvalue){
      analyzedVars.push_back((*refs).Name);
      int currentStage = 0;
      for(auto st = 0; st < stages; st++){
        if((*refs).RefLoc < stagesLocEnds[st]){ 
          currentStage = st;
          break;
        }
      }
      if((*refs).RefLoc >= stagesLocEnds[stagesLocEnds.size()-1]){
        currentStage =stagesLocEnds.size();
      }
      for(auto refs2 = Loops[i].VarRef.begin();refs2 != Loops[i].VarRef.end()&&notAnalized;refs2++){
        int currentStage2 = 0;
        for(auto st = 0; st < stages; st++){
          if((*refs2).RefLoc < stagesLocEnds[st]){ 
            currentStage2 = st;
            break;
          }
        }
        if((*refs2).RefLoc >= stagesLocEnds[stagesLocEnds.size()-1]){
          currentStage2 =stagesLocEnds.size();
        }
        if((*refs2).Name == (*refs).Name && currentStage != currentStage2){
          for(auto refs3 = Loops[i].VarRef.begin();refs3 != Loops[i].VarRef.end();refs3++){
            if((*refs3).Name == (*refs2).Name && (*refs2).RefLoc == (*refs3).RefLoc){
              if((*refs3).rvalue){
                feedbackStages[currentStage] = 1;
                feedbackStages[currentStage2] = 1;
              }
            }
          }
        } 
      }
    }
  }
  bool isPool = false;
  bool ffeed = false;
  bool lfeed  = false;
  bool mfeed = true;
  for(auto j = 0; j <= stages; j++){
    if(feedbackStages[j] ==1 && !ffeed) ffeed= true;
    if(ffeed && feedbackStages[j] ==0 ) mfeed = false;
    if(!mfeed && feedbackStages[j] ==1 ) lfeed = true;
  }
  if(ffeed && lfeed && !mfeed) isPool = true; 
  return isPool;
}

/**
  \fn bool analyzeLRLoop(int i)

  This function analyze every Loop detected and detect if any is 
  a possible pipeline or not. This function modifies struct members
  "Loops", "Functions" and "Code" adding addicional information.

 */
bool MyASTVisitor::PoolDetect(){
  //Analize function declarations with bodies
  analyzeCodeFunctions();
  //Counter farms = maps
  int feqm = 0;
  //Counter farms = pipes
  int feqp = 0;
  int nestedFarm = 0;

  int numpipelines = 0;
  int pipelinesCounter = 0;
  for(int i = 0; i < numLoops;i++){
    SourceManager &SM = TheRewriter.getSourceMgr();
    auto fileName = Loops[i].FileName;
    auto currfile = Loops[i].FileName;//SM.getFileEntryForID(SM.getMainFileID())->getName();
    //Annotate only if the loop is in the current file. 
    if(fileName == currfile){
      std::stringstream SSComments;
      SSComments<<"";
      bool comments = false;
      std::vector<ASTVar> globals;
      std::vector<unsigned> stagesLocEnds; 
      std::stringstream SS;
      int stages = 0;
      //Check non accessible functions usage
      bool nonaccesible = false;
      //Analyze function calls
      analyzeCall(i);

      //Check for L/Rvalue
      analyzeLRLoop(i);

      //Stages detection
      stageDetection(i,stages,stagesLocEnds);
      //Global variable LValue
      bool globallvalue = analyzeGlobalsLoop(i, SSComments, stagesLocEnds);
      //Check if at least 1 modified variable in a stage is used in the next stage
      bool isUsed = true;
      //Check feedback 	
      bool feedback = false;
      bool isPool = false;
      if(stages>0){
        feedback = checkFeedback(i,SSComments,comments,stages,stagesLocEnds);
        isPool = checkPool(i,stages,stagesLocEnds);
      }
      //GET INPUTS AND OUTPUTS 
      std::vector<std::vector<std::string>> outputs(stages+1);
      std::vector<std::vector<std::string>> inputs(stages+1);
      getStageIO(inputs, outputs,stages,i,stagesLocEnds);
      std::vector<bool> farmstage(stagesLocEnds.size());
      checkFarmStages(stagesLocEnds,i,farmstage);
      std::vector<std::string> stream;
      getPipelineStream(inputs,outputs,stages,stream);
      //Annotation
      //If every requirement is satisfied
      if(!Loops[i].wfarm&&isPool&&feedback&&globallvalue == false && isUsed == true && Loops[i].numOps >1&&stages>=1){ 
        // Add comment before
        std::stringstream SSBefore;
        std::stringstream declarations;
        if(comments||nonaccesible){
          SSBefore<<"[[rph::pool, rph::id("<<numpipelines<<"), rph::with_warnings\n, rph::warnings(\n";
          if(nonaccesible){

            SSBefore<<" \"functioninaccessible\"\n,";

          }

          SSBefore<<SSComments.str();
          SSBefore<<")\n";
        }else{

          Loops[i].pipeline = true;



          SSBefore << "[[rph::pool, rph::id("<<numpipelines<<") ";
          if(Loops[i].map||Loops[i].farm){
            if(Loops[i].map){
              SSBefore<<", rph::map";
            }
            if(Loops[i].farm){
              SSBefore<<", rph::farm";
              feqp ++ ;
            }
            //ADD MAP I/O
            if(Loops[i].inputs.size()>0){
              SSBefore<<", rph::in(";
              for(unsigned in = 0; in<Loops[i].inputs.size();in++){
                SSBefore<<Loops[i].inputs[in];
                if(in!=Loops[i].inputs.size()-1){
                  SSBefore<<",";
                }
              }
              SSBefore<<")";
            }
            if(Loops[i].outputs.size()>0){
              SSBefore<<", rph::out(";
              for(unsigned out = 0; out<Loops[i].outputs.size();out++){
                SSBefore<<Loops[i].outputs[out];
                if(out!=Loops[i].outputs.size()-1){
                  SSBefore<<",";
                }
              }
              SSBefore<<")";
            }		
          }


          auto lineStart = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getBegin()).first,Loops[i].RangeLoc.getBegin().getRawEncoding());
          auto lineEnd = SM.getLineNumber(SM.getDecomposedLoc(Loops[i].RangeLoc.getEnd()).first,Loops[i].RangeLoc.getEnd().getRawEncoding());
          pipelinesCounter++;

        }
        SSBefore << "]]\n";
        TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);

        for(int st = 0; st<=stages;st++){
          std::stringstream SSStage;
          if(st!=0) SSStage <<"\n}";
          if(st==0) SSStage << declarations.str();
          SSStage<<"\n\n[[rph::stage("<<st<<"), rph::poolid("<<numpipelines<<")";
          if(farmstage[st]&&st!=0) {
            SSStage<<", rph::farm";
            nestedFarm++;
          }
          if(inputs[st].size()>0){
            SSStage<< ", rph::in(";
            for(auto in = inputs[st].begin();in!= inputs[st].end();in++){
              SSStage<<(*in);
              if(in!=inputs[st].end()-1)
                SSStage<<",";
            }
            SSStage<<")";
          }
          if(outputs[st].size()>0){
            SSStage<< ", rph::out(";
            for(auto out = outputs[st].begin();out!= outputs[st].end();out++){
              SSStage<<(*out);
              if(out!=outputs[st].end()-1)
                SSStage<<",";
            }                                
            SSStage<<")";                   
          }
          SSStage<<"]]\n{\n";
          if(st==0){
            TheRewriter.InsertText(Loops[i].InitLoc, SSStage.str(), true, true);
          }else{
            auto StageLocation = SourceLocation::getFromRawEncoding(stagesLocEnds[st-1]);
            TheRewriter.InsertText(StageLocation, SSStage.str(), true, true);
          }
          std::stringstream SSEndStage;
          if(st==stages){
            std::stringstream SSEndStage;
            SSEndStage <<"\n\t}\n";
            TheRewriter.InsertText(Loops[i].RangeLoc.getEnd(), SSEndStage.str(), true, true);

          }


        }
        numpipelines++;



      }
    }
  }
  std::cout<<"Pool patterns detected: "<<numpipelines<<"\n";
  std::cout<<"Nested farms: "<<nestedFarm<<"\n";
  std::cout<<std::flush;       
  return true;
}
