%{
/*
 * @author Lance Townsend
 *
 * @brief Performs parsing and runs the entire compilitaion process
 *
*/

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include "treeNodes.h"
#include "treeUtils.h"
#include "codegen.h"
#include "scanType.h"
#include "symbolTable.h"
#include "semantics.h"

using namespace std;

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *msg);

void printToken(TokenData myData, string tokenName, int type = 0) {
   cout << "Line: " << myData.linenum << " Type: " << tokenName;
   if(type==0)
     cout << " Token: " << myData.tokenstr;
   if(type==1)
     cout << " Token: " << myData.nvalue;
   if(type==2)
     cout << " Token: " << myData.cvalue;
   cout << endl;
}

/* Abstract Syntrax Tree */
static TreeNode* syntaxTree;
static int numErrors;
static int numWarnings;

%}

%union
{
   TokenData *tinfo ;
   TreeNode *tnode ;
   ExpType etype;
}

%type   <etype> typeSpec

%type   <tnode> program precomList declList decl localDecls
%type   <tnode> varDecl varDeclId scopedVarDecl varDeclList varDeclInit
%type   <tnode> funDecl parms parmList parmTypeList parmIdList parmId
%type   <tnode> stmt expStmt compoundStmt stmtList returnStmt breakStmt
%type   <tnode> factor matched unmatched mutable immutable
%type   <tnode> call args argList
%type   <tnode> iterRange
%type   <tnode> exp simpleExp andExp unaryRelExp relExp minmaxExp sumExp mulExp unaryExp
%type   <tnode> constant 

%token  <tinfo> FIRSTOP
%token  <tinfo> IF THEN ELSE 
%token  <tinfo> FOR TO BY 
%token  <tinfo> WHILE DO BREAK
%token  <tinfo> ADDASS SUBASS MULASS DIVASS
%token  <tinfo> MIN MAX
%token  <tinfo> ID
%token  <tinfo> CHARCONST NUMCONST BOOLCONST STRINGCONST
%token  <tinfo> CHAR INT BOOL STATIC
%token  <tinfo> ';' '(' ')' '{' '}' '<' '>' '=' ',' ':' '-' '?' '+' '/' '*' '%' '[' ']'
%token  <tinfo> NEQ EQ LEQ GEQ
%token  <tinfo> AND OR NOT
%token  <tinfo> INC DEC
%token  <tinfo> PRECOMPILER
%token  <tinfo> ERROR RETURN 
%token  <tinfo> CHSIGN SIZEOF
%token  <into>  LASTOP

%type   <tinfo> relop sumop assignop minmaxop mulop unaryop

%token   <tinfo> LASTTERM

%%

program    : precomList declList                    { syntaxTree = $2; }
           ;

precomList : precomList PRECOMPILER                   { $$ = NULL; printf("%s\n", yylval.tinfo->tokenstr); }
           | PRECOMPILER                                 { $$ = NULL; printf("%s\n", yylval.tinfo->tokenstr);}
           | /* empty */                                 {  $$ = NULL;  }
           ;

declList   : declList decl                          { $$ = addSibling($1, $2);  }
           | decl { $$ = $1; }
           ;

decl       : varDecl { $$ = $1; }
           | funDecl { $$ = $1; }
           ;

varDecl    : typeSpec varDeclList ';'               { setType($1, $2, false); $$ = $2; yyerrok;  }
           ;


scopedVarDecl : STATIC typeSpec varDeclList ';'      { $$ = $3; setType($2, $3, true); }
              | typeSpec varDeclList ';'             { $$ = $2; setType($1, $2, false); }
              ;

varDeclList  : varDeclList ',' varDeclInit           { $$ = addSibling($1, $3); }
             | varDeclInit { $$ = $1;  }
             ;

varDeclInit  : varDeclId                             { $$ = $1; }
             | varDeclId ':' simpleExp               { $$ = $1; if ($$ != NULL) $$->child[0] = $3; }
             ;

varDeclId   : ID                                     { $$ = newDeclNode(VarK, UndefinedType, $1); }
            | ID '[' NUMCONST ']'                    { $$ = newDeclNode(VarK, UndefinedType, $1);
                                                       $$->isArray = true;
                                                       $$->size = 1 + $3->nvalue; }
            ;

typeSpec   : INT                                        { $$ = Integer; }
           | BOOL                                       {$$ = Boolean;}
           | CHAR                                       { $$ = Char;}
           ;

funDecl    : typeSpec ID '(' parms ')' stmt       { $$ = newDeclNode(FuncK, $1, $2, $4, $6); }
           | ID '(' parms ')' stmt                   { $$ = newDeclNode(FuncK, Void, $1, $3, $5); }
           ;

parms      : parmList {$$ = $1;}
           | /* empty */                                { $$ = NULL; }
           ;

parmList   : parmList ';' parmTypeList              { $$ = addSibling($1, $3); }
           | parmTypeList { $$ = $1; }
           ;

parmTypeList : typeSpec parmIdList                  { setType($1, $2, false); $$ = $2; }
             ;

parmIdList   : parmIdList ',' parmId                 { $$ = addSibling($1, $3); }
             | parmId { $$ = $1; }
             ;

parmId     : ID                                          { $$ = newDeclNode(ParamK, UndefinedType, $1); }
           | ID '[' ']'                                   { $$ = newDeclNode(ParamK, UndefinedType, $1); $$->isArray = true; }
           ;

stmt       : matched {$$ = $1; }
           | unmatched {$$ = $1; }
           ;


matched    : IF simpleExp THEN matched ELSE matched   { $$ = newStmtNode(IfK, $1, $2, $4, $6);  }
           | WHILE simpleExp DO matched                  { $$ = newStmtNode(WhileK, $1, $2, $4);  }
           | FOR ID '=' iterRange DO matched             { $$ = newStmtNode(ForK, $1, NULL, $4, $6); $$->child[0] = newDeclNode(VarK, Integer, $2); }
           | expStmt {$$ = $1; }
           | compoundStmt {$$ = $1; }
           | returnStmt {$$ = $1; }
           | breakStmt {$$ = $1; }
           ;

iterRange  : simpleExp TO simpleExp                  { $$ = newStmtNode(RangeK, $2, $1, $3); }
           | simpleExp TO simpleExp BY simpleExp    { $$ = newStmtNode(RangeK, $2, $1, $3, $5); }
           ;

unmatched  : IF simpleExp THEN stmt                    { $$ = newStmtNode(IfK, $1, $2, $4); }
           | IF simpleExp THEN matched ELSE unmatched  { $$ = newStmtNode(IfK, $1, $2, $4, $6); }
           | WHILE simpleExp DO unmatched                { $$ = newStmtNode(WhileK, $1, $2, $4); }
           | FOR ID '=' iterRange DO unmatched           { $$ = newStmtNode(ForK, $1, NULL, $4, $6); $$->child[0] = newDeclNode(VarK, Integer, $2); }
           ;

expStmt    : exp ';'
           | ';'                                        { $$ = NULL; }
           ;

compoundStmt : '{' localDecls stmtList '}'         { $$ = newStmtNode(CompoundK, $1, $2, $3); yyerrok; }
             ;

localDecls : localDecls scopedVarDecl              { $$ = addSibling($1, $2);}
           | /* empty */                               {  $$ = NULL; }
           ;

stmtList   : stmtList stmt                         { $$ = addSibling($1, $2); }
           | /* empty */                               {  $$ = NULL; }
           ;

returnStmt : RETURN ';'                                {  $$ = newStmtNode(ReturnK, $1);}
           | RETURN exp ';'                           { $$ = newStmtNode(ReturnK, $1, $2); }
           ;

breakStmt  : BREAK ';'                                 { $$ = newStmtNode(BreakK, $1); }
           ;

exp        : mutable assignop exp                { $$ = newExpNode(AssignK, $2, $1, $3); }
           | mutable INC                              { $$ = newExpNode(AssignK, $2, $1); }
           | mutable DEC                              { $$ = newExpNode(AssignK, $2, $1); }
           | simpleExp {$$ = $1; }
           ;

assignop  : '=' {$$ = $1; }
          | ADDASS {$$ = $1;  }
          | SUBASS {$$ = $1; }
          | MULASS {$$ = $1; }
          | DIVASS {$$ = $1; }
          ;

simpleExp  : simpleExp OR andExp                 {  $$ = newExpNode(OpK, $2, $1, $3);}
           | andExp {$$ = $1; }
           ;

andExp     : andExp AND unaryRelExp              { $$ = newExpNode(OpK, $2, $1, $3); }
           | unaryRelExp {$$ = $1; }
           ;

unaryRelExp : NOT unaryRelExp                      { $$ = newExpNode(OpK, $1, $2); }
            | relExp {$$ = $1; }
            ;

relExp     : minmaxExp relop minmaxExp          { $$ = newExpNode(OpK, $2, $1, $3); }
           | minmaxExp {$$ = $1; }
           ;

relop      : LEQ {$$ = $1; }
           | '<' {$$ = $1; }
           | '>' {$$ = $1; }
           | GEQ {$$ = $1; }
           | EQ {$$ = $1; }
           | NEQ {$$ = $1; }
           ;

minmaxExp  : minmaxExp minmaxop sumExp              { $$ = newExpNode(OpK, $2, $1, $3); }
           | sumExp {$$ = $1; }
           ;

minmaxop   : MAX {$$ = $1; }
           | MIN {$$ = $1; }
           ;

sumExp     : sumExp sumop mulExp              { $$ = newExpNode(OpK, $2, $1, $3); }
           | mulExp {$$ = $1; }
           ;

sumop      : '+' {$$ = $1; }
           | '-' {$$ = $1; }
           ;

mulExp     : mulExp mulop unaryExp           { $$ = newExpNode(OpK, $2, $1, $3); }
           | unaryExp {$$ = $1; }
           ;

mulop      : '*' {$$ = $1;}
           | '/' {$$ = $1;}
           | '%' {$$ = $1;}
           ;

unaryExp   : unaryop unaryExp                   { $$ = newExpNode(OpK, $1, $2); }
           | factor {$$ = $1; }
           ;

unaryop    : '-'                                     { $$ = $1; $$->tokenclass = CHSIGN; $$->tokenstr = (char*)"chsign";  }
           | '*'                                      { $$ = $1; $$->tokenclass = SIZEOF; $$->tokenstr = (char*)"sizeof";  }
           | '?'                                      { $$ = $1; }
           ;

factor     : immutable {$$ = $1; }
           | mutable {$$ = $1; }
           ;

mutable    : ID                                       { $$ = newExpNode(IdK, $1); }
           | ID '[' exp ']'                          { TreeNode* tmp = newExpNode(IdK, $1);
                                                       tmp->isArray = true; 
                                                       $$ = newExpNode(OpK, $2, tmp, $3);
                                                       $$->isArray = true;
                                                     }
           ;

immutable  : '(' exp ')'                            { $$ = $2; }
           | call {$$ = $1; }
           | constant {$$ = $1; }
           ;

call       : ID '(' args ')'                        { $$ = newExpNode(CallK, $1, $3); }
           ;

args       : argList {$$ = $1; }
           | /* empty */                                { $$ = NULL; }
           ;

argList    : argList ',' exp                       { $$ = addSibling($1, $3);  }
           | exp {$$ = $1; }
           ;

constant   : NUMCONST                                   { $$ = newExpNode(ConstantK, $1); $$->type = Integer; }
           | CHARCONST                                   { $$ = newExpNode(ConstantK, $1); $$->type = Char; }
           | STRINGCONST                                 { $$ = newExpNode(ConstantK, $1); $$->type = Char; $$->isArray = true; $$->size = 1 + $1->nvalue;  }
           | BOOLCONST                                   { $$ = newExpNode(ConstantK, $1); $$->type = Boolean; }
           ;


%%

/*
 * @brief output error messages
 *
 * @param msg - error message to output
 *
 * @return void
*/
void yyerror (const char *msg) { 
   printf("Syntax error at line %d: %s\n", yylval.tinfo->linenum, msg);
}

/*
 * @brief driver code to run the compiler
*/
int main(int argc, char **argv) {
   int option, index;
   char *file = NULL;
   extern FILE *yyin;
   numErrors = 0, numWarnings = 0;
   SymbolTable *symtab;
   symtab = new SymbolTable();
   symtab->debug(false);
   int globalOffset;

   initTokenStrings();

   while ((option = getopt (argc, argv, "")) != -1)
      switch (option)
      {
      default:
         ;
      }
   if ( optind == argc ) yyparse();
   for (index = optind; index < argc; index++) 
   {
      yyin = fopen (argv[index], "r");
      yyparse();
      fclose (yyin);
   }

   if (numErrors == 0) {
      syntaxTree = semanticAnalysis(syntaxTree, symtab, globalOffset);
   }

   if (numErrors == 0) {
      codegen(stdout, argv[1], syntaxTree, symtab, globalOffset, false);
   }

   printf("Number of warnings: %d\n", numWarnings);
   printf("Number of errors: %d\n", numErrors);

   return 0;
}

