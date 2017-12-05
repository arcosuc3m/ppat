
bool MyASTVisitor::windowDetector(std::vector<std::string> feedvars, int i){
  //Analize function declarations with bodies
  analyzeCodeFunctions();
  std::stringstream SSComments;
  //Analyze function calls
  analyzeCall(i);
  //Check for L/Rvalue
  analyzeLRLoop(i);
  auto fileName = Loops[i].FileName;

  std::vector<std::string> outputs;
  std::vector<std::string> inputs;

  //Get inputs and outputs
  getMapIO(inputs, outputs,i);
  std::vector<std::string> feedbackVars;
  std::vector<std::string> operators;
  //Check input memory access	

  bool feedback = checkFeedback(i,feedbackVars, operators);

  bool hasreturn = false;
  if(Loops[i].Returns.size()>0){
    hasreturn=true;
  }
  bool hasbreak = check_break(i);
  bool hasgoto = false;
  if(Loops[i].Gotos.size()>0){
    hasgoto = true;
  }

  bool valid=false;
  for(auto it = 0; it< feedbackVars.size(); it++){
    for(auto k = 0; k< feedvars.size() ; k++){
      if(feedbackVars[it]==feedvars[k]){ valid = true;
      }
    }
  }
  if(!hasgoto && valid) return true;
  return false;	
}

