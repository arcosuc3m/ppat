/**
  \fn void PPATtoGRPPI()

  Transforms the sequential original code to grppi parallel code. It takes the information 
  collected for the detected patterns and generates the corresponding parallel code for 
  those patterns.

*/
void MyASTVisitor::PPATtoGRPPI(){
    SourceManager &sm = TheRewriter.getSourceMgr();
    clang::LangOptions lopt;
	for(unsigned i = 0; i< Loops.size(); i++){
        //If the loop matches with a farm pattern
		if(Loops[i].farm){
            //Generated code
			std::stringstream SSBefore;
			std::stringstream GenLambda;
			std::stringstream TaskLambda;
			std::stringstream SinkLambda;
            //Gets the location of the end of the condition in a while construct
            clang::SourceLocation e(clang::Lexer::getLocForEndOfToken( Loops[i].conditionRange.getEnd()  , 0, sm, lopt));
            //Intoduces the farm function using a sequential execution model
            SSBefore<<"Farm(sequential_execution {},\n";
            //Generates the stream generation lambda of the farm function
            SSBefore<<"[&](){";
             
            //Gets the starting and ending points of the generation in the sequential code
            clang::SourceLocation genBegin = Loops[i].genStart;
            clang::SourceLocation genEnd = Loops[i].genEnd;
            clang::SourceLocation gb;
            clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);
             
            //FIXME: Multiple generation code sections
            //If the generation lambda is not in the loop header 
            if(genBegin!=Loops[i].RangeLoc.getBegin()&&genEnd!=Loops[i].RangeLoc.getBegin()){           
                //Gets the statement location of the starting and ending points of the stream generation
                bool endStmt = true;
                for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
                   if(  genEnd.getRawEncoding() <= (*stmt).getBegin().getRawEncoding() ){ genEnd = (*stmt).getBegin() ; endStmt = false; break;  }
                }          
                auto auxBegin = genBegin; 
                for(auto stmt = Loops[i].stmtLoc.end()-1 ; stmt != Loops[i].stmtLoc.begin()-1; stmt--){
                   if( genBegin.getRawEncoding() >= (*stmt).getEnd().getRawEncoding()  ){ 
                      auxBegin = clang::SourceLocation(clang::Lexer::getLocForEndOfToken( (*stmt).getEnd()  , 0, sm, lopt));      //(*(stmt)).getEnd();
                      break;
                   }
                }           
                genBegin = auxBegin;
                if(endStmt) genEnd = Loops[i].RangeLoc.getEnd();
                gb = genBegin;
                //Introduces the code corresponding to the stream generation located in the loop body
                GenLambda<< std::string(sm.getCharacterData(gb), sm.getCharacterData(genEnd) - sm.getCharacterData( gb));
            }else{
               //If the generation is in the loop header, it sets the end location of the stream generation to the begining of the body
               clang::SourceLocation b(clang::Lexer::getLocForEndOfToken( Loops[i].bodyRange.getBegin()  , 0, sm, lopt));
               genEnd = b;
            }
            Loops[i].genStart = genBegin;
            Loops[i].genEnd = genEnd;
            
            //Get the outputs of the stream generator function
            getStreamGeneratorOut(i);
            //If the stream generation is on the end of the body its necesary to not lose the first iteration. Then the returning element is stored and passed to the task phase before computing the next element.
            std::stringstream auxGen;
            if(Loops[i].genBefore){
                auxGen << GenLambda.str();
                GenLambda.str("");
                GenLambda<<"auto StreamItem = (((";
            }else{
            //Generates the stop condition of the farm pattern
                GenLambda<< "return ((";
            //Gets the tokens corresponding to the condition from the loop header
            }

            //FIXME: If the stop condition is in the loop body it should detect where is the break. The break should be in a if condition and then, the tool should extract the condition from the if.
            writeGenFunc(Loops[i].cond, GenLambda, Loops[i].bodyRange.getBegin() , Loops[i].RangeLoc.getBegin());
 
            //Generates the optional to be returned
            GenLambda<< ")) ? optional<";
            std::stringstream outputtype;
            std::stringstream outputname;
            //If there are multiple output elements it generates a tuple 
            if(Loops[i].genVar.size()>1){
                outputtype<<"std::tuple<";
            }
            //Every variable generated in the stream generator function its intorduced
            for(auto outvar = Loops[i].genVar.begin(); outvar != Loops[i].genVar.end(); outvar++){
                outputtype<<(*outvar).second;
                outputname<<(*outvar).first;
                auto aux = outvar;
                aux++;
                if( aux != Loops[i].genVar.end()){                  
                  outputtype<<",";
                  outputname<<",";
                }
            }
            if(Loops[i].genVar.size()>1){
                outputtype<<">";
            }

            GenLambda << outputtype.str();
            GenLambda << ">("<< outputname.str() <<") : optional<";
            GenLambda << outputtype.str();
            GenLambda << ">()";
            
            if(Loops[i].genBefore){
               GenLambda<<");";
               GenLambda << auxGen.str();
               GenLambda << "\n return StreamItem;";
            }

            GenLambda<<"},\n";
            //Generation of the sink lambda
            std::stringstream sinkinput;
            std::stringstream sinkname;
            //If there exists at least on part of the code corresponding to a sink lambda
            if(Loops[i].sinkZones.size() > 0 ){
                SourceLocation currentLoc = Loops[i].bodyRange.getBegin();
                //Introduces the sink lmabda
                SinkLambda << "[&](";
                //Generates the inputs of the sink lambda function
                if(Loops[i].taskVar.size()>1){
                    sinkinput << "std::tuple< ";
                }

                for(auto invar = Loops[i].taskVar.begin(); invar != Loops[i].taskVar.end(); invar++){
                    sinkinput<<(*invar).second;
                    sinkname<<(*invar).first;
                    auto aux = invar;
                    aux++;
                    if( aux != Loops[i].taskVar.end()){
                       sinkinput<<",";
                       sinkname<<",";
                    }
                }
                


                if(Loops[i].taskVar.size()>1){
                    sinkinput << "> StreamItem";
                }else{
                    sinkinput <<" "<< sinkname.str();
                }
                SinkLambda << sinkinput.str();
                SinkLambda << "){\n";

                //Decompress the different input from the tuple
                if(Loops[i].taskVar.size()>1){
                  for(auto invar = Loops[i].taskVar.begin(); invar != Loops[i].taskVar.end(); invar++){
                    SinkLambda << (*invar).second << " " << (*invar).first << ";\n";
                  }
                  SinkLambda << "std::tie("<<sinkname.str()<<") = StreamItem;\n";
                }
                //Introduce the different code sections related to the sink 
                for(auto sink = Loops[i].sinkZones.begin(); sink != Loops[i].sinkZones.end();  sink++){
                     if((*sink).second.first.getRawEncoding() > currentLoc.getRawEncoding()) currentLoc = (*sink).second.first;
                     SinkLambda<< std::string(sm.getCharacterData(currentLoc), sm.getCharacterData( (*sink).second.second ) - sm.getCharacterData( currentLoc));
                }
                SinkLambda<< " }";
            }
    
         
            clang::SourceLocation b(clang::Lexer::getLocForEndOfToken( Loops[i].bodyRange.getBegin()  , 0, sm, lopt));
            //Generates the task lambda
            TaskLambda<<"[=](";
            //Generates the task lambda argument
            TaskLambda<< outputtype.str();
            TaskLambda <<" ";
            if(Loops[i].genVar.size()==1){
                TaskLambda << outputname.str();
            }
            else{
                TaskLambda<<"StreamItem";
            }
            //Generates Stream item tuple unpacking
            TaskLambda<<"){";
            if(Loops[i].genVar.size()>1){
                for(auto iter = Loops[i].genVar.begin(); iter != Loops[i].genVar.end(); iter++){
                    TaskLambda <<(*iter).second;
                    TaskLambda << " ";
                    TaskLambda<<(*iter).first;
                    TaskLambda << ";\n";
                }
                TaskLambda << "std::tie(";
                for(auto iter = Loops[i].genVar.begin(); iter != Loops[i].genVar.end(); iter++){
                    TaskLambda<<(*iter).first;
                    auto aux = iter;
                    aux++;
                    if((aux) != Loops[i].genVar.end()){                  
                       TaskLambda<<","; 
                    }
                }
                TaskLambda << ") = StreamItem;\n";
            }
            

            //Generates the task lambda from the end of the stream generation lambda to the end of the loop.
            for(auto stmt = Loops[i].stmtLoc.begin() ; stmt != Loops[i].stmtLoc.end(); stmt++){
                  bool isTask = true;

                  if((*stmt).getBegin().getRawEncoding() >= gb.getRawEncoding() 
                      &&  genEnd.getRawEncoding() > (*stmt).getBegin().getRawEncoding() ) isTask= false;

                  for(auto sink = Loops[i].sinkZones.begin(); sink != Loops[i].sinkZones.end();  sink++){
                     if((*stmt).getBegin().getRawEncoding() >= (*sink).second.first.getRawEncoding() 
                      &&  (*sink).second.second.getRawEncoding() > (*stmt).getBegin().getRawEncoding() ) isTask= false;
                  }
                  if(isTask){ 
                      TaskLambda <<"\n";
                      TaskLambda << std::string(sm.getCharacterData((*stmt).getBegin() ), 
                              sm.getCharacterData( clang::Lexer::getLocForEndOfToken( (*stmt).getEnd()  , 0, sm, lopt) ) - sm.getCharacterData( (*stmt).getBegin() ) );
                 }

            }
            //If there exists a sink function and has input arguments, generates the outputs from the task lambda
            if(Loops[i].sinkZones.size()>0 && Loops[i].taskVar.size()>0){
                TaskLambda << "\nreturn ";
                if(Loops[i].taskVar.size()>1) TaskLambda << sinkinput.str()<<"(";
                TaskLambda << sinkname.str();
                if(Loops[i].taskVar.size()>1) TaskLambda << ")";
                TaskLambda <<";\n";
                
            }
          
            TaskLambda << "\n}\n";
            SSBefore << GenLambda.str();
            SSBefore << TaskLambda.str();
            SSBefore << SinkLambda.str();
            SSBefore<<");\n";
            //Remove original source code
            TheRewriter.RemoveText(Loops[i].RangeLoc);
            //Insert the farm funtion in the source code
		    TheRewriter.InsertText(Loops[i].RangeLoc.getBegin(), SSBefore.str(), true, true);
		}
	}
}
/**
  \fn void writeGenFunc(ASTCondition cond, std::stringstream & GenFunc,clang::SourceLocation e, clang::SourceLocation b)

  Generates the stop condition of the stream generation lambda. It extracts the statements that compose 
  the condition on the original code from the begin location to the end location. 

  \param[in]    cond    AST structure that contains the staments that compose the stop condition
  \param[out]   GenFunc string stream that contains the stream generation lambda code
  \param[in]    e   end location of the condition
  \param[in]    b   begin location of the condition
*/
void MyASTVisitor::writeGenFunc(ASTCondition cond, std::stringstream & GenFunc,clang::SourceLocation e, clang::SourceLocation b){
      clang::SourceLocation currentLoc = b;
      std::string element = "";
      int n = 0;
      do{
         element = getNext( currentLoc, cond, e);
         GenFunc << element;
      }while(element != "");
       
}

/**
  \fn std::string getNext(SourceLocation& current,ASTCondition cond, clang::SourceLocation e )
 
  This fuction returns the next token, in the AST structure "cond", to the current location and located
  before the end location. 
  
  \param[in,out]    current Its the current location. This location is updated before returning the next element.
  \param[in]    cond    AST structure that contains the staments composing the condition.
  \param[in]    e   Its the end location to stop getting next elements.

*/
std::string MyASTVisitor::getNext(SourceLocation& current,ASTCondition cond, clang::SourceLocation e ){
      SourceManager &sm = TheRewriter.getSourceMgr();
      clang::LangOptions lopt;

      clang::SourceLocation next = e;
      clang::SourceLocation nextEnd = e;
      std::string element = "";

      clang::BeforeThanCompare<clang::SourceLocation> isBefore(sm);
      //Checks if the next token is a variable declaration
      for(auto i = cond.VarDecl.begin(); i!= cond.VarDecl.end();i++){
          //If is potentially the next token, it updates the auzxiliar locations and stores the token in element
          if(!isBefore((*i).RefSourceLoc, current) && isBefore((*i).RefSourceLoc, next)){
             element = (*i).type + " " + (*i).Name;
             next= (*i).RefSourceLoc;
             nextEnd = (*i).RefSourceLoc;
          }
      }
 
      //Checks if the next token is a variable reference
      for(auto i = cond.VarRef.begin(); i!= cond.VarRef.end();i++){
          if( isBefore(current, (*i).RefSourceLoc ) && isBefore( (*i).RefSourceLoc , next) ){
             element = (*i).Name;
             next= (*i).RefSourceLoc ;
             nextEnd = (*i).RefSourceLoc;
          }
      }
      //Checks if the next token is a function call
      for(auto i = cond.fCalls.begin(); i!= cond.fCalls.end();i++){
          if(isBefore( current, (*i).SLoc ) && isBefore( (*i).SLoc, next) ){
             next = (*i).SLoc;
             nextEnd = (*i).EndLoc;
             element = std::string(sm.getCharacterData((*i).SLoc),sm.getCharacterData((*i).EndLoc) - sm.getCharacterData( (*i).SLoc) );
          }
      }
      //Checks if the next token is an operator
      for(auto i = cond.ops.begin(); i!= cond.ops.end();i++){
          if(isBefore(current, (*i).Location) && isBefore((*i).Location, next) ){
             element = (*i).Name;
             //remove the keyword operator from the operator name
             size_t remove = element.find("operator");
             if(remove != std::string::npos)
                element.replace(remove, std::string("operator").length(), "");
             next= (*i).Location;
             nextEnd = (*i).Location;
             //If the next operator its a boolean comparison operator, it introduces parenthesis
             if((*i).type.find("_Bool") != std::string::npos &&
                ((*i).Name.find("==") != std::string::npos || (*i).Name.find("!=") != std::string::npos ||
                 (*i).Name.find(">=") != std::string::npos || (*i).Name.find("<=") != std::string::npos ||
                 (*i).Name.find("&&") != std::string::npos || (*i).Name.find("||") != std::string::npos))  
                     element = ")"+element+"(";
          }
      }
      //Updates the current location and returns the next token
      current = nextEnd;
      return element;
}


