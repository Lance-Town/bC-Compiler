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
void codegenExpression(TreeNode *current);
void codegenDecl(TreeNode *current);
void codegenStatement(TreeNode *current);

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
 * @brief get offset register for different variable kinds
 */
int offsetRegister(VarKind v) {
   switch (v) {
      case Local:
         return FP;
      case Parameter:
         return FP;
      case Global:
         return GP;
      case LocalStatic:
         return GP;
      default:
         printf("ERROR(codegen): looking up offset register for a variable of type %d\n", v);
         return 666;
   }
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
         int currloc, skiploc;

         emitComment((char *)"WHILE");

         // return here to do the test 
         currloc = emitSkip(0);
         
         // test expression
         codegenExpression(current->child[0]); 

         emitRM((char *)"JNZ", AC, 1, PC, (char *)"Jump to while part");
         emitComment((char *)"DO");

         // save old break statement return point
         skiploc = breakloc;

         // address of instruction that jumps to end of loop, 
         // also the backpatch point
         breakloc = emitSkip(1);

         // do body of loop
         codegenGeneral(current->child[1]);

         emitGotoAbs(currloc, (char *)"go to beginning of loop");

         // backpatch jump to end of loop
         backPatchAJumpToHere(breakloc, (char *)"Jump past loop [backpatch]");

         // restore break statement
         breakloc = skiploc;
         emitComment((char *)"END WHILE");

         break;

      case ForK:

         break;

      case CompoundK:
         int savedToffset;

         savedToffset = toffset;
         toffset = current->size;
         emitComment((char *)"COMPOUND");
         emitComment((char *)"TOFF set:", toffset);
         codegenGeneral(current->child[0]); // process inits
         emitComment((char *)"Compound Body");
         codegenGeneral(current->child[1]); 
         toffset = savedToffset;
         emitComment((char *)"TOFF set:", toffset);
         emitComment((char *)"END COMPOUND");


         break;

      case ReturnK:

         break;

      case BreakK:

         break;

      case RangeK:

         break;

      default:
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
         {
            TreeNode *lhs = current->child[0], *rhs = current->child[1];
            int offReg;

            if (lhs->attr.op == '[') {
               // TODO
               lhs->isArray = true;
               TreeNode *var = lhs->child[0], *index = lhs->child[1];
               offReg = offsetRegister(var->varKind);

               if (var == NULL) {
                  printf("ERROR(codegenExpression) var is NULL\n");
                  break;
               }
               if (index == NULL) {
                  printf("ERROR(codegenExpression) var is NULL\n");
                  break;
               }

               codegenExpression(index);

               if (rhs != NULL) {
                  emitRM((char *)"ST", AC, toffset, FP, (char *)"Push index");
                  toffset--;
                  emitComment((char *)"TOFF dec:", toffset);
                  codegenExpression(rhs);
                  toffset++;
                  emitComment((char *)"TOFF inc:", toffset);
                  emitRM((char *)"LD", AC1, toffset, FP, (char *)"Pop index");
               }
               
               if (var->varKind == Parameter) {
                  emitRM((char *)"LD", AC2, var->offset, FP, (char *)"Load address of base of array", var->attr.name);
               } else if (var->varKind == Global || var->varKind == LocalStatic) {
                  emitRM((char *)"LDA", AC2, var->offset, GP, (char *)"Load address of base of array", var->attr.name);
               } else {
                  emitRM((char *)"LDA", AC2, var->offset, FP, (char *)"Load address of base of array", var->attr.name);
               }
               
               int op = current->attr.op;

               if (op == INC || op == DEC) {
                  emitRO((char *)"SUB", AC2, AC2, AC, (char *)"Compute offset of value");
               } else {
                  emitRO((char *)"SUB", AC2, AC2, AC1, (char *)"Compute offset of value");
               }
               
               switch(op) {
                  case INC:
                     emitRM((char *)"LD", AC, 0, 5, (char *)"load lhs variable", var->attr.name);
//                     emitRM((char *)"LD", AC1, var->offset, offReg, (char *)"load lhs variable", var->attr.name);
                     emitRM((char *)"LDA", AC, 1, AC, (char *)"increment value of", var->attr.name);
                     emitRM((char *)"ST", AC, 0, AC2, (char *)"Store variable", var->attr.name);
                     break;

                  case DEC:
                     emitRM((char *)"LD", AC, 0, 5, (char *)"load lhs variable", var->attr.name);
//                     emitRM((char *)"LD", AC1, var->offset, offReg, (char *)"load lhs variable", var->attr.name);
                     emitRM((char *)"LDA", AC, -1, AC, (char *)"decrement value of", var->attr.name);
                     emitRM((char *)"ST", AC, 0, AC2, (char *)"Store variable", var->attr.name);
                     break;

                  case ADDASS:
                     emitRM((char *)"LD", AC1, 0, 5, (char *)"load lhs variable", var->attr.name);
                     emitRO((char *)"ADD", AC, AC1, AC, (char *)"op +=");
                     emitRM((char *)"ST", AC, 0, 5, (char *)"Store variable", var->attr.name);
                     break;
               }
               

            } else {
               offReg = offsetRegister(lhs->varKind);

               if (rhs) {
                  codegenExpression(rhs);
               }
   
               switch (current->attr.op) {
                  case ADDASS:
                     emitRM((char *)"LD", AC1, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRO((char *)"ADD", AC, AC1, AC, (char *)"op +=");
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;
                  case SUBASS:
                     emitRM((char *)"LD", AC1, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRO((char *)"SUB", AC, AC1, AC, (char *)"op -=");
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;
                  
                  case DIVASS:
                     emitRM((char *)"LD", AC1, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRO((char *)"DIV", AC, AC1, AC, (char *)"op /=");
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;
   
                  case MULASS:
                     emitRM((char *)"LD", AC1, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRO((char *)"MUL", AC, AC1, AC, (char *)"op *=");
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;

                  case '=':
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;

                  case DEC:
                     emitRM((char *)"LD", AC, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRM((char *)"LDA", AC, -1, AC, (char *)"decrement value of", lhs->attr.name);
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;

                  case INC:
                     emitRM((char *)"LD", AC, lhs->offset, offReg, (char *)"load lhs variable", lhs->attr.name);
                     emitRM((char *)"LDA", AC, 1, AC, (char *)"increment value of", lhs->attr.name);
                     emitRM((char *)"ST", AC, lhs->offset, offReg, (char *)"Store variable", lhs->attr.name);
                     break;

                  default:
                     // ERROR
                     break;
               }
               
            }
            break;
         }

      case CallK:

         break;

      case ConstantK:
         if (current->type == Char) {
            if (current->isArray) {
               emitStrLit(current->offset, current->attr.string);
               emitRM((char *)"LDA", AC, current->offset, GP, (char *)"Load address of char array");
            } else {
               emitRM((char *)"LDC", AC, int(current->attr.cvalue), AC3, (char *)"Load char constant");
            }
         } else if (current->type == Boolean) {
            emitRM((char *)"LDC", AC, current->attr.value, AC3, (char *)"Load Boolean constant");
         } else if (current->type == Integer) {
            emitRM((char *)"LDC", AC, current->attr.value, AC3, (char *)"Load integer constant");
         }

         break;

      case IdK:
         if (current->isArray) {
            emitRM((char *)"LDA", AC2, current->offset, FP, (char *)"Load address of base of array", current->attr.name);
         } else {
            emitRM((char *)"LD", AC, -2, FP, (char *)"Load variable", current->attr.name);
         }
         break;

      case OpK:
         if (current->child[0]) {
            codegenExpression(current->child[0]);
         }

         if (current->child[1]) {
            emitRM((char *)"ST", AC, toffset, FP, (char *)"Push left side");
            toffset--; 
            emitComment((char *)"TOFF dec:", toffset);
            codegenExpression(current->child[1]);
            toffset++;
            emitComment((char *)"TOFF inc:", toffset);
            emitRM((char *)"LD", AC1, toffset, FP, (char *)"Pop left into ac1");

            switch(current->attr.op) {
               case '+':
                  emitRO((char *)"ADD", AC, AC1, AC, (char*)"Op +");
                  break;
               
            }
         }

         // TODO: more code to come

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
         {
            if (current->isArray) {
               switch(current->varKind) {
                  case Local:
                     {
                        emitRM((char *)"LDC", AC, current->size-1, 6, 
                              (char *)"load size of array", current->attr.name);
                        emitRM((char *)"ST", AC, current->offset+1, 
                              offsetRegister(current->varKind), (char *)"save size of array", 
                              current->attr.name);
                        break;
                     }

                  case LocalStatic:
                  case Parameter:
                  case Global:
                     // do nothing
                     break;
                  case None:
                     // ERROR
                     break;
               }

               // ARRAY VALUE initializatin
               if (current->child[0]) {
                  codegenExpression(current->child[0]);
                  emitRM((char *)"LDA", AC1, current->offset, offsetRegister(current->varKind), 
                        (char *)"address of lhs");
                  emitRM((char *)"LD", AC2, 1, AC, (char *)"size of rhs");
                  emitRM((char *)"LD", AC3, 1, AC1, (char *)"size of lhs");
                  emitRO((char *)"SWP", AC2, AC3, 6, (char *)"pick smallest size");
                  emitRO((char *)"MOV", AC1, AC, AC2, (char *)"array op =");
               }

            } else { /* !current->isArray */
               // SCALAR VALUE initialization
               if (current->child[0]) {
                  if (current->varKind == Local) {
                     // compute rhs -> AC
                     codegenExpression(current->child[0]);

                     // save it
                     emitRM((char *)"ST", AC, current->offset, FP, (char *)"Store variable", current->attr.name);
                  } else if (current->varKind == None) {
                     // ERROR CONDITION
                  }
               }
            }

            break;
         }

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

void initAGlobalSymbol(std::string stm, void *ptr) {
   TreeNode *current;
   
   // printf("Symbol: %s\n", sym.c_str()); // dump the symbol table
   current = (TreeNode *)ptr;

   // printf("lineno: %d\n", current->lineno); // dump the symbol table

   if (current->lineno != -1) {
      if (current->isArray) {
         emitRM((char *)"LDC", AC, current->size-1, 6, (char *)"load size of array", current->attr.name);
         emitRM((char *)"ST", AC, current->offset+1, GP, (char *)"save size of array", current->attr.name);
      }
      if (current->kind.decl==VarK && (current->varKind == Global || current->varKind == LocalStatic)) {
         if (current->child[0]) {
            // compute rhs -> AC;
            codegenExpression(current->child[0]);

            // save it
            emitRM((char *)"ST", AC, current->offset, GP, (char *)"Store variable", current->attr.name);
         }
      }
   }

   return;
}

/*
 * @brief initialize global array sizes
 */
void initGlobalArraySizes() {
   emitComment((char *)"INIT GLOBALS AND STATICS");
   globals->applyToAllGlobal(initAGlobalSymbol);
   emitComment((char *)"END INIT GLOBALS AND STATICS");

   return;
}

/*
 * @brief Generate init code
 */
void codegenInit(int initJump, int globalOffset) {
   backPatchAJumpToHere(initJump, (char *)"Jump to init [backpatch]");

   emitComment((char *)"INIT");
   //OLD pre 4.6 TM emitRM((char *)"LD", GP, 0, 0, (char *)"Set the global pointer");
   emitRM((char *)"LDA", FP, globalOffset, GP, (char *)"set first frame at end of globals");
   emitRM((char *)"ST", FP, 0, FP, (char *)"store old fp (point to self)");

   initGlobalArraySizes();

   emitRM((char *)"LDA", AC, 1, PC, (char *)"Return address in ac");

   {
      // Jump to main
      TreeNode *funcNode;

      funcNode = (TreeNode *)globals->lookup((char *)"main");
      if (funcNode) {
         emitGotoAbs(funcNode->offset, (char *)"Jump to main");
      } else {
         printf("ERROR(LINKER): Procedure main is not defined.\n");
         numErrors++;
      }
   }

   emitRO((char *)"HALT", 0, 0, 0, (char *)"DONE!");
   emitComment((char *)"END INIT");

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
   codegenInit(initJump, globalOffset);
}
