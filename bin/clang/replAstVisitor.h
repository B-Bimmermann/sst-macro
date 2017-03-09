#ifndef bin_clang_replAstVisitor_h
#define bin_clang_replAstVisitor_h

#include "clangHeaders.h"
#include "findAstVisitor.h"
#include "foundMacro.h"

#define visitFxn(cls) \
  bool Visit##cls(clang::cls* c){ return TestStmtMacro(c); }

class ReplGlobalASTVisitor : public clang::RecursiveASTVisitor<ReplGlobalASTVisitor> {
 public:
  ReplGlobalASTVisitor(clang::Rewriter &R,  FindGlobalASTVisitor& F, MacroList& m) :
    TheRewriter(R), finder(F), mlist(m) {}

  bool VisitDeclRefExpr(clang::DeclRefExpr* expr);

  bool VisitDecl(clang::Decl *D);

  void setCompilerInstance(clang::CompilerInstance& c){
    CI = &c;
  }


  bool VisitStmt(clang::Stmt* s);

  //visitFxn(CompoundStmt)
  //visitFxn(Expr)

 private:
  bool TestStmtMacro(clang::Stmt* s);

  clang::Rewriter& TheRewriter;
  clang::CompilerInstance* CI;
  FindGlobalASTVisitor& finder;
  MacroList& mlist;

};


#endif
