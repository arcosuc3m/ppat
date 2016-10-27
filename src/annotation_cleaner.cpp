void MyASTVisitor::clean(){
         for(unsigned i = 0 ; i<Kernels.size();i++){
                //ERASE PREVIOS KERNEL ANNOTATION
                SourceManager &SM = TheRewriter.getSourceMgr();
                auto loc = SM.getDecomposedLoc(Kernels[i].RangeLoc.getBegin());
                auto line = SM.getLineNumber(loc.first,Kernels[i].RangeLoc.getBegin().getRawEncoding());
                auto fEntry = SM.getFileEntryForID(loc.first);
                auto beginLine = SM.translateFileLineCol(fEntry,(line-1),1);
                auto endLine =  SM.translateFileLineCol(fEntry,line,1);
                auto offset= endLine.getRawEncoding()-beginLine.getRawEncoding();
                TheRewriter.RemoveText(beginLine,offset,clang::Rewriter::RewriteOptions());
         }
}
