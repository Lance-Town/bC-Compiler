/*
 * @author Lance Townsend
 *
 * @brief functions for code generation targeted
 * at the Tiny Virtual Machine
 */

#include <stdio.h>
#include "emitcode.h"
#include "treeUtils.h"
#include "semantics.h"
#include "scanType.h"
#include "symbolTable.h"
#include "parser.tab.h"

extern int numErrors;
extern int numWarnings;
extern int yyparse();
extern int yydebug;
extern TreeNode *syntree;
extern char **largerTokens;
extern void initTokenStrings();

#define OFPOFF 0
#define RETURNOFFSET -1

// Prototypes
void codegenGeneral(TreeNode *current);

int toffset;                    // next available termporary space
FILE *code;                     // shared global code
static bool linenumFlag;        // mark with line numbers
static int breakloc;            // which while to break ot
static SymbolTable *globals;    // global symbol table 

/*
 * @brief Output basic header information about compiler
 */
void codegenHeader(char *srcFile) {
   emitComment((char *)"bC compiler version bC-Su23");
   emitComment((char *)"File compiled: ", srcFile);
   return;
}

/*
 * @brief Comment out line number for node
 */
void commentLineNum(TreeNode *current) {
   char buff[16];

   if (linenumFlag) {
      sprintf(buff, "%d", current->lineno);
      emitComment((char *)"Line: ", buff);
   }
   return;
}

/*
 * @brief Generate code for library functions
 */
void codegenLibraryFun(TreeNode *current) {
   emitComment((char *)"");
   emitComment((char *)"** ** ** ** ** ** ** ** ** ** ** **");
   emitComment((char *)"FUNCTION", current->attr.name);

   // remember where this function is
   current->offset = emitSkip(0);

   // store return address
   emitRM((char *)"ST", AC, RETURNOFFSET, FP, (char *)"Store return address");

   if (strcmp(current->attr.name, (char *)"input") == 0) {
      emitRO((char *)"IN", RT, RT, RT, (char *)"Grab int input");
   } else if (strcmp(current->attr.name, (char *)"inputb") == 0) {
      emitRO((char *)"INB", RT, RT, RT, (char *)"Grab bool input");
   } else if (strcmp(current->attr.name, (char *)"inputc") == 0) {
      emitRO((char *)"INC", RT, RT, RT, (char *)"Grab char input");
   } else if (strcmp(current->attr.name, (char *)"output")==0) {
      emitRM((char *)"LD", AC, -2, FP, (char *)"Load parameter");
      emitRO((char *)"OUT", AC, AC, AC, (char *)"Output integer");
   } else if (strcmp(current->attr.name, (char *)"outputb")==0) {
      emitRM((char *)"LD", AC, -2, FP, (char *)"Load parameter");
      emitRO((char *)"OUTB", AC, AC, AC, (char *)"Output bool");
   } else if (strcmp(current->attr.name, (char *)"outputc")==0) {
      emitRM((char *)"LD", AC, -2, FP, (char *)"Load parameter");
      emitRO((char *)"OUTC", AC, AC, AC, (char *)"Output char");
   } else if (strcmp(current->attr.name, (char *)"outnl")==0) {
      emitRO((char *)"OUTNL", AC, AC, AC, (char *)"Output a newline");
   } else {
      emitComment((char *)"ERROR(LINKER): No support for special function");
      emitComment(current->attr.name);
   }

   emitRM((char *)"LD", AC, RETURNOFFSET, FP, (char *)"Load return address");
   emitRM((char *)"LD", FP, OFPOFF, FP, (char *)"Adjust fp");
   emitGoto(0, AC, (char *)"Return");

   emitComment((char *)"END FUNCTION", current->attr.name);

   return;
}


/*
 * @brief Generate code for functions
 */
void codegenFun(TreeNode *current) {
   emitComment((char *)"");
   emitComment((char *)"** ** ** ** ** ** ** ** ** ** ** **");
   emitComment((char *)"FUNCTION", current->attr.name);
   toffset = current->size;
   emitComment((char *)"TOFF set:", toffset);

   // IMPORTANT: For function nodes, the offset is defined to be the
   // position of the function in the code space. This is accesible
   // via the symbol table.
   current->offset = emitSkip(0); // offset holds the instruction address

   // store return address
   emitRM((char *)"ST", AC, RETURNOFFSET, FP, (char *)"Store return address");

   // generate code for the statements
   codegenGeneral(current->child[1]);

   // in case there was no return statement, set return register to 0 and return
   emitComment((char *)"Add standard closing in case there is no return statement");
   emitRM((char *)"LDC", RT, 0, 6, (char *)"Set return value to 0");
   emitRM((char *)"LD", AC, RETURNOFFSET, FP, (char *)"Load return address");
   emitRM((char *)"LD", FP, OFPOFF, FP, (char *)"Adjust fp");
   emitGoto(0, AC, (char *)"Return");
   emitComment((char *)"END FUNCTION", current->attr.name);

   return;
}

/*
 * @brief Generate code for statements 
 */
void codegenStatement(TreeNode *current) {
   commentLineNum(current);

   switch (current->kind.stmt) {
      case IfK:

         break;

      case WhileK:

         break;

      case ForK:

         break;

      case CompoundK:

         break;

      case ReturnK:

         break;

      case BreakK:

         break;

      case RangeK:

         break;

   }    

   return;
}

/*
 * @brief Generate code for expressions
 */
void codegenExpression(TreeNode *current) {
   commentLineNum(current);

   switch (current->kind.exp) {
      case AssignK:

         break;

      case CallK:
         break;

      case ConstantK:
         if (current->type = Char) {
            if (current->isArray) {
               emitStrLit(current->offset, current->attr.string);
               emitRM((char *)"LDA", AC, current->offset, 0, (char *)"Load address of char array");
            } else {
               emitRM((char *)"LDC", AC, int(current->attr.cvalue), 6, (char *)"Load char constant");
            }
         }

         break;

      case IdK:

         break;

      case OpK:

         break;
   }    

   return;
}

/*
 * @brief Generate code for declarations
 */
void codegenDecl(TreeNode *current) {
   commentLineNum(current);

   switch (current->kind.decl) {
      case VarK:
         // lots to do here
         break;

      case FuncK:
         if (current->lineno == -1) {
            codegenLibraryFun(current);
         } else {
            codegenFun(current);
         }
         break;

      case ParamK:
         // IMPORTANT: no instructions need to be made
         // for parameters
         break;
   }    

   return;
}

/*
 * @brief Generate code for the 3 kinds of nodes
 */
void codegenGeneral(TreeNode *current) {
   TreeNode *tmp = current;
   while (tmp) {
      switch (tmp->nodekind) {
         case StmtK:
            codegenStatement(tmp);
            break;
         case ExpK:
            emitComment((char *)"EXPRESSION");
            codegenExpression(tmp);
            break;
         case DeclK:
            codegenDecl(tmp);
            break;
      }
      tmp = tmp->sibling;
   }

   return;
}

/*
 * @brief top level code generator call
 *
 * @param codeIn - Where the code will be outputted
 * @param srcFile - name of file compiled
 * @param syntaxTree - tree to process
 * @param globalsIn - globals so function info can be found
 * @param globalOffset - size of the global frame
 * @param linenumFlagIn - comment with line numbers
 *
 */
void codegen(FILE *codeIn, char *srcFile, 
         TreeNode *syntaxTree, SymbolTable *globalsIn, 
         int globalOffset, bool linenumFlagIn) {
   int initJump;

   code = codeIn;
   globals = globalsIn;
   linenumFlag = linenumFlagIn;
   breakloc = 0;

   // save a plave for the jump to init
   initJump = emitSkip(1);

   // generate comments describing what is compiled
   codegenHeader(srcFile);
   
   // general code generation including IO Library
   codegenGeneral(syntaxTree);
   
   // generation of initialization for run
   //codegeninit(initJump, globalOffset);
}
