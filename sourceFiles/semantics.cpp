/*
 * @author Lance Townsend
 *
 * @brief functions to perform semantic analysis on the
 * abstract syntax tree
 * 
 */

#include <string.h>
#include "treeNodes.h"
#include "treeUtils.h"
#include "symbolTable.h"
#include "parser.tab.h"

static int goffset = 0;
static int foffset = 0;
static int varCounter = 0;
static int newScope = 0;
static TreeNode *funcInside = NULL; // store which function the children are currently inside

extern int numErrors;
extern int numWarnings;
extern char *largerTokens[LASTTERM+1];

void treeTraverse(TreeNode *current, SymbolTable *symtab);
TreeNode *loadIOLib(TreeNode *syntree);
bool insertError(TreeNode *current, SymbolTable *symtab);
void treeStmtTraverse(TreeNode *current, SymbolTable *symtab);
void treeExpTraverse(TreeNode *current, SymbolTable *symtab);
void treeDeclTraverse(TreeNode *current, SymbolTable *symtab);
void treeTraverse(TreeNode *current, SymbolTable *symtab);
TreeNode *semanticAnalysis(TreeNode *syntree, SymbolTable *symtabX, int &globalOffset);

/*
 * @brief load IO libraries and link them to syntax tree
 *
 * @return head to tree after being linked
 */
TreeNode *loadIOLib(TreeNode *syntree) {
   TreeNode *input, *output, *paramOutput;
   TreeNode *inputb, *outputb, *paramOutputb;
   TreeNode *inputc, *outputc, *paramOutputc;
   TreeNode *outnl;

   input = newDeclNode(FuncK, Integer);
   input->lineno = -1;
   input->attr.name = strdup("input");
   input->type = Integer;

   inputb = newDeclNode(FuncK, Boolean);
   inputb->lineno = -1;
   inputb->attr.name = strdup("inputb");
   inputb->type = Boolean;

   inputc = newDeclNode(FuncK, Boolean);
   inputc->lineno = -1;
   inputc->attr.name = strdup("inputc");
   inputc->type = Char;

   paramOutput = newDeclNode(ParamK, Void);
   paramOutput->lineno = -1;
   paramOutput->attr.name = strdup("*dummy*");
   paramOutput->type = Integer;

   output = newDeclNode(FuncK, Void);
   output->lineno = -1;
   output->attr.name = strdup("output");
   output->type = Void;
   output->child[0] = paramOutput;

   paramOutputb = newDeclNode(ParamK, Void);
   paramOutputb->lineno = -1;
   paramOutputb->attr.name = strdup("*dummy*");
   paramOutputb->type = Boolean;

   outputb = newDeclNode(FuncK, Void);
   outputb->lineno = -1;
   outputb->attr.name = strdup("outputb");
   outputb->type = Void;
   outputb->child[0] = paramOutputb;

   paramOutputc = newDeclNode(ParamK, Void);
   paramOutputc->lineno = -1;
   paramOutputc->attr.name = strdup("*dummy*");
   paramOutputc->type = Char;

   outputc = newDeclNode(FuncK, Void);
   outputc->lineno = -1;
   outputc->attr.name = strdup("outputc");
   outputc->type = Void;
   outputc->child[0] = paramOutputc;

   outnl = newDeclNode(FuncK, Void, NULL);
   outnl->lineno = -1;
   outnl->attr.name = strdup("outnl");
   outnl->type = Void;

   // link them and prefix the tree we are interested in traversing
   // this will put the symbols in the symbol table
   input->sibling = output;
   output->sibling = inputb;
   inputb->sibling = outputb;
   outputb->sibling = inputc;
   inputc->sibling = outputc;
   outputc->sibling = outnl;
   outnl->sibling = syntree; // add in the tree we are given
   
   return input;
}

/*
 * @brief handle error conditions for operators and assignments
 * not having the correct lhs and rhs types
 */
void handleOpErrors(TreeNode *current, SymbolTable *symtab) {
   int op = current->attr.op;
   TreeNode *lhs = NULL, *rhs = NULL; 

   TreeNode *lookupNode = (TreeNode *)symtab->lookup(current->attr.name);

   if (current->child[0] == NULL) {
      printf("SYNTAX ERROR(%d): child 0 cannot be NULL\n", current->lineno);
      numErrors++;
      return;
   } else {
      lhs = (TreeNode *)symtab->lookup(current->child[0]->attr.name);
   }

   if (current->child[1] != NULL) {
      rhs = (TreeNode *)symtab->lookup(current->child[1]->attr.name);
   }

   if (current->child[0] != NULL && lhs == NULL) {
      lhs = current->child[0];
   }
   if (current->child[1] != NULL && rhs == NULL) {
      rhs = current->child[1];
   }

   if (lhs->type == UndefinedType) {
      return;
   }


   if (op == '/' || op == '-' || op == '*' || op == '+' || op == '%' || op == MIN
         || op == MAX || op == ADDASS || op == SUBASS || op == MULASS
         || op == DIVASS) {
            
      if (lhs->type != Integer) {
         printf("SEMANTIC ERROR(%d): '%s' requires operands of type int but lhs is of %s.\n",
               current->lineno, largerTokens[op], expToStr(lhs->type, false, false));
         numErrors++;
      }

      if (rhs->type != Integer) {
         printf("SEMANTIC ERROR(%d): '%s' requires operands of type int but rhs is of %s.\n",
               current->lineno, largerTokens[op], expToStr(rhs->type, false, false));
         numErrors++;
      }

      if (lhs->isArray || rhs->isArray) {
         printf("SEMANTIC ERROR(%d): The operation '%s' does not work with arrays.\n", 
            current->lineno, largerTokens[op]);
         numErrors++;
      }
   } else if (op == AND || op == OR) {
      if (lhs->type != Boolean) {
         printf("SEMANTIC ERROR(%d): '%s' requires operands of type bool but lhs is of %s.\n",
               current->lineno, largerTokens[op], expToStr(lhs->type, false, false));
         numErrors++;
      }

      if (rhs->type != Boolean) {
         printf("SEMANTIC ERROR(%d): '%s' requires operands of type bool but rhs is of %s.\n",
               current->lineno, largerTokens[op], expToStr(rhs->type, false, false));
         numErrors++;
      }

      if (lhs->isArray || rhs->isArray) {
         printf("SEMANTIC ERROR(%d): The operation '%s' does not work with arrays.\n", 
            current->lineno, largerTokens[op]);
         numErrors++;
      }
   } else if (op == '=' || op == EQ || op == NEQ || op == '>' || op == GEQ || op == '<' || op == LEQ) {
      if (lhs->type != rhs->type) {
         printf("SEMANTIC ERROR(%d): '%s' requires operands of the same type but lhs is %s and rhs is %s.\n",
               current->lineno, largerTokens[op], expToStr(lhs->type, false, false), expToStr(rhs->type, false, false));
         numErrors++;
      }

      if (lhs->attr.op != '[') {
         if (lhs->isArray && !rhs->isArray) {
            printf("SEMANTIC ERROR(%d): '%s' requires both operands be arrays or not but lhs is an array and rhs is not an array.\n", 
                  current->lineno, largerTokens[op]);
            numErrors++;
         } else if (!lhs->isArray && rhs->isArray) {
            printf("SEMANTIC ERROR(%d): '%s' requires both operands be arrays or not but lhs is not an array and rhs is an array.\n",
                  current->lineno, largerTokens[op]);
            numErrors++;
         }
      } 
   } else if (op == SIZEOF) {
     if (!lhs->isArray) {
        printf("SEMANTIC ERROR(%d): The operation 'sizeof' only works with arrays.\n", current->lineno);
        numErrors++;
     } 
   } else if (op == '?' || op == CHSIGN || op == INC || op == DEC) {
      if (lhs->type != Integer) {
         printf("SEMANTIC ERROR(%d): Unary '%s' requires an operand of type int but was given %s.\n", 
               current->lineno, largerTokens[op], expToStr(lhs->type, false, false));
         numErrors++;
      }

      if (lhs->isArray) {
         printf("SEMANTIC ERROR(%d): The operation '%s' does not work with arrays.\n", current->lineno, largerTokens[op]);
        numErrors++;
      } 
   } else if (op == '[') {
      if (!lhs->isArray) {
         printf("SEMANTIC ERROR(%d): Cannot index nonarray '%s'.\n", current->lineno, lhs->attr.name);
         numErrors++;
      }
      if (rhs->type != Integer) {
         printf("SEMANTIC ERROR(%d): Array '%s' should be indexed by type int but got %s.\n", current->lineno, lhs->attr.name, expToStr(rhs->type, false, false));
         numErrors++;
      }
      if (rhs->isArray) {
         printf("SEMANTIC ERROR(%d): Array index is the unindexed array '%s'.\n", current->lineno, rhs->attr.name);
         numErrors++;
      }
   }


   return;
}

/*
 * @brief Count number of siblings a node has
 */
int countSiblings(TreeNode *current) {
   int count = 0;
   TreeNode *tmp = current;

   while (tmp) {
      count++;
      tmp = tmp->sibling;
   }

   return count;
}

/*
 * @brief Attempt to insert a node into the
 * symbol table
 * 
 * @param current - Tree node to attempt to insert
 * @param symtab - Symbol table to insert into
 * 
 * @return true if insert succeeds, false otherwise
 */
bool insertError(TreeNode *current, SymbolTable *symtab) {
   if (!symtab->insert(current->attr.name, current)) {
      //put an error msg for ASN6
      return false;
   }
   return true;
}

/*
 * @brief Perform semantic analysis on a Statement node
 * 
 * @param current - Tree node to be analyzed
 * @param symtab - Symbol table for looking up children nodes
 * 
 * @return void
 */
void treeStmtTraverse(TreeNode *current, SymbolTable *symtab) {
   if (current->kind.stmt != CompoundK) {
      newScope = 1;
   }
   
   TreeNode *lookupNode;
   int remOffset;

   switch (current->kind.stmt) {
      case IfK:
         if (newScope) {   
            symtab->enter((char *)"IfStmt");
            remOffset = foffset;
            if (current->child[0] && current->child[0]->type != Boolean) {
               printf("SEMANTIC ERROR(%d): Expecting Boolean test condition in if statement but got %s.\n", current->lineno, expToStr(current->child[0]->type, false, false));
               numErrors++;
            }

            if (current->child[0] && current->child[0]->isArray) {
               printf("SEMANTIC ERROR(%d): Cannot use array as test condition in if statement.\n", current->lineno);
               numErrors++;
            }
            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
            foffset = remOffset;
            symtab->leave();
         } else {
            newScope = 1;
            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
         }
         break;

      case WhileK:
         if (newScope) {   
            symtab->enter((char *)"WhileStmt");
            remOffset = foffset;
            if (current->child[0] && current->child[0]->type != Boolean) {
               printf("SEMANTIC ERROR(%d): Expecting Boolean test condition in while statement but got %s.\n", current->lineno, expToStr(current->child[0]->type, false, false));
               numErrors++;
            }

            if (current->child[0] && current->child[0]->isArray) {
               printf("SEMANTIC ERROR(%d): Cannot use array as test condition in while statement.\n", current->lineno);
               numErrors++;
            }
            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
            foffset = remOffset;
            symtab->leave();
         } else {
            newScope = 1;
            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
         }
         break;

      case ForK:
         if (newScope) {   
            symtab->enter((char *)"ForStmt");
            remOffset = foffset;
            treeTraverse(current->child[0], symtab);
            foffset -= 2;
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
            foffset = remOffset;
            symtab->leave();
         } else {
            newScope = 1;
            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
         }
         break;

      case CompoundK:
         if (newScope) {
            symtab->enter((char*)"CompoundStatement");
            remOffset = foffset;

            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);

            foffset = remOffset;
            symtab->leave();
         } else {
            newScope = 1;

            treeTraverse(current->child[0], symtab);
            current->size = foffset;
            treeTraverse(current->child[1], symtab);
            treeTraverse(current->child[2], symtab);
         }
         break;

      case ReturnK:

         treeTraverse(current->child[0], symtab);
         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         if (current->child[0] == NULL && funcInside != NULL) {
            printf("SEMANTIC ERROR(%d): Function '%s' at line %d is expecting to return %s but return has no value.\n",
                  current->lineno, funcInside->attr.name, funcInside->lineno, expToStr(funcInside->type, false, false));
            numErrors++;
         } 
         
         if (current->child[0] != NULL) {
            lookupNode = (TreeNode *) symtab->lookup(current->child[0]->attr.name);

            if (current->child[0]->type == UndefinedType) {
               // error thrown elsewhere
            } else if (lookupNode != NULL && lookupNode->isArray) {
               printf("SEMANTIC ERROR(%d): Cannot return an array.\n", current->lineno);
               numErrors++;
            } else if (funcInside != NULL && funcInside->type != current->child[0]->type) {
               if (funcInside->type == Void) {
                  printf("SEMANTIC ERROR(%d): Function '%s' at line %d is expecting no return value, but return has a value.\n", 
                        current->lineno, funcInside->attr.name, funcInside->lineno);
               } else {
                  printf("SEMANTIC ERROR(%d): Function '%s' at line %d is expecting to return %s but returns %s.\n",
                     current->lineno, funcInside->attr.name, funcInside->lineno,
                     expToStr(funcInside->type, false, false), expToStr(current->child[0]->type, false, false));
               }
               numErrors++;
            }
         }

         break;

      case BreakK:
         if (symtab->depth() <= 2) {
            printf("SEMANTIC ERROR(%d): Cannot have a break statement outside of loop.\n", current->lineno);
            numErrors++;
         }
         treeTraverse(current->child[0], symtab);
         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         break;

      case RangeK:
         for (int i = 0; i < MAXCHILDREN; i++) {
            if (current->child[i] != NULL) {
               if (current->child[i]->type != Integer) {
                  printf("SEMANTIC ERROR(%d): Expecting type int in position %d in range of for statement but got %s.\n",
                        current->lineno, i+1, expToStr(current->child[i]->type, false, false));
                  numErrors++;
               }
               if (current->child[i]->isArray) {
                  printf("SEMANTIC ERROR(%d): Cannot use array in position %d in range of for statement.\n",
                        current->lineno, i+1);
                  numErrors++;
               }
            }

         }

         treeTraverse(current->child[0], symtab);
         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);


         break;
   }
}

/*
 * @brief Perform semantic analysis on an Expression node
 * 
 * @param current - Tree node to be analyzed
 * @param symtab - Symbol table for looking up children nodes
 * 
 * @return void
 */
void treeExpTraverse(TreeNode *current, SymbolTable *symtab) {
   newScope = 1;

   TreeNode *lookupNode;

   switch(current->kind.exp) {
      case AssignK:
         

         if (current->attr.op == '=') {
//            current->isAssigned = true;
         }

         if (current->child[0] == NULL) {
            //printf("ERROR: left child has no type - semantics.cpp::treeExpTraverse()\t%s\n", current->attr.name);
         } else {
            // look up childs type and set it to current type
            lookupNode = 
               (TreeNode *) symtab->lookup(current->child[0]->attr.name);

            if (lookupNode == NULL) {
               // child is not in the symbol table, but it does exist
               // FIX for array not being in symbol table, but the child existing. 
               current->type = current->child[0]->type;
               current->child[0]->isAssigned = true;
               current->child[0]->isUsed = true;
            } else {
               lookupNode->isAssigned = true;
               lookupNode->isUsed = true;
               current->type = lookupNode->type;
            }

         }

         treeTraverse(current->child[0], symtab);
         handleOpErrors(current, symtab);
         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         break;

      case CallK:
         treeTraverse(current->child[0], symtab);
         lookupNode = (TreeNode *) symtab->lookup(current->attr.name);

         if (lookupNode == NULL) {
            printf("SEMANTIC ERROR(%d): Symbol '%s' is not declared.\n", current->lineno, current->attr.name);
            current->type = UndefinedType;
            numErrors++;
         } else {
            current->type = lookupNode->type;
            //current->size = lookupNode->size;
            current->offset = lookupNode->offset;
            current->isUsed = true;

            TreeNode *params = current->child[0];
            TreeNode *lookups = lookupNode->child[0];
            TreeNode *tmp;
            int i = 1;

            while (params && lookups) {
               tmp = params->sibling;
               params->sibling = NULL;
               treeTraverse(params, symtab);

               params->sibling = tmp;

               if (params->type != lookups->type) {
                  printf("SEMANTIC ERROR(%d): Expecting %s in parameter %d of call to '%s' declared on line %d but got %s.\n",
                     current->lineno, expToStr(lookups->type, false, false), i, lookupNode->attr.name, lookupNode->lineno,
                     expToStr(params->type, false, false));
                  numErrors++;
               }
               if (lookups->isArray && !params->isArray) {
                  printf("SEMANTIC ERROR(%d): Expecting array in parameter %d of call to '%s' declared on line %d.\n",
                        current->lineno, i, lookupNode->attr.name, lookupNode->lineno);
                  numErrors++;
               } else if (!lookups->isArray && params->isArray) {
                  printf("SEMANTIC ERROR(%d): Not expecting array in parameter %d of call to '%s' declared on line %d.\n",
                        current->lineno, i, lookupNode->attr.name, lookupNode->lineno);
                  numErrors++;
               }

               params = params->sibling;
               lookups = lookups->sibling;
               i++;
            }

            if (params && !lookups) {
               printf("SEMANTIC ERROR(%d): Too many parameters passed for function '%s' declared on line %d.\n",
                  current->lineno, current->attr.name, lookupNode->lineno);
               numErrors++;
            } else if (!params && lookups) {
               printf("SEMANTIC ERROR(%d): Too few parameters passed for function '%s' declared on line %d.\n",
                  current->lineno, current->attr.name, lookupNode->lineno);
               numErrors++;
            }
         }

         break;

      case ConstantK:
         // All constant strings are global
         if (current->type == Char && current->isArray) {
            current->varKind = Global;
            current->offset = goffset-1;
            goffset -= current->size;
         }
         break;

      case IdK:
         treeTraverse(current->child[0], symtab);

         lookupNode = (TreeNode *)symtab->lookup(current->attr.name);

         if (lookupNode != NULL) {
            if (lookupNode->kind.decl == FuncK) {
               printf("SEMANTIC ERROR(%d): Cannot use function '%s' as a variable.\n", current->lineno, lookupNode->attr.name);
               numErrors++;
            }

            current->offset = lookupNode->offset;
            current->type = lookupNode->type;
            current->size = lookupNode->size;
            current->varKind = lookupNode->varKind;
            current->isArray = lookupNode->isArray;
            current->isStatic = lookupNode->isStatic;
            current->isUsed = true;
            lookupNode->isUsed = true;

            if (!lookupNode->isAssigned && !lookupNode->isArray && lookupNode->kind.decl == VarK) {
               printf("SEMANTIC WARNING(%d): Variable '%s' may be uninitialized when used here.\n", current->lineno, lookupNode->attr.name);
               lookupNode->isAssigned = true;
               numWarnings++;
            }
         } else {
            printf("SEMANTIC ERROR(%d): Symbol '%s' is not declared.\n", current->lineno, current->attr.name);
            current->type = UndefinedType;
            numErrors++;
           
         }

         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         break;

      case OpK:
         int op = current->attr.op;
         treeTraverse(current->child[0], symtab);

         handleOpErrors(current, symtab);

         if (op == EQ || op == NEQ || op == LEQ || op == GEQ || op == '<' || op == '>') {
            current->type = Boolean;
         } else if (op == SIZEOF) {
            current->type = Integer;
         } else {

            if (current->child[0] == NULL) {
               printf("ERROR: Op child can not be NULL - semantics.cpp::treeExpTraverse\n");
            } else {
               lookupNode = (TreeNode *)symtab->lookup(current->child[0]->attr.name);
               if (lookupNode == NULL) {
                  current->type = current->child[0]->type;
               } else {
                  current->type = lookupNode->type;
               }
            }
         }

         if (op == '[') {
            current->isArray = true;
         }


         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);
         break;
   }

   return;
}

/*
 * @brief Perform semantic analysis on a Declaration node
 * 
 * @param current - Tree node to be analyzed
 * @param symtab - Symbol table for looking up children nodes
 * 
 * @return void
 */
void treeDeclTraverse(TreeNode *current, SymbolTable *symtab) {
   newScope = 1;
   TreeNode *lookupNode;

   switch (current->kind.decl) {
      case VarK:
         treeTraverse(current->child[0], symtab);      
         if (current->child[0] != NULL) {
//            printf("current: %s\tcurrent->child: %s\n", current->attr.name, current->child[0]->attr.name);
            if (current->type != current->child[0]->type) {
               printf("SEMANTIC ERROR(%d): Initializer for variable '%s' of %s is of %s\n", 
                     current->lineno, current->attr.name, expToStr(current->type, false, false), 
                     expToStr(current->child[0]->type, false, false));
               numErrors++;
            }

            if (current->child[0]->kind.exp != ConstantK) {
               printf("SEMANTIC ERROR(%d): Initializer for variable '%s' is not a constant expression.\n", current->lineno, current->attr.name);
               numErrors++;
            }

            if (current->isArray && !current->child[0]->isArray) {
               printf("SEMANTIC ERROR(%d): Initializer for variable '%s' requires both operands be arrays or not but variable is an array and rhs is not an array.\n",
                     current->lineno, current->attr.name);
               numErrors++;
            }
            if (!current->isArray && current->child[0]->isArray) {
               printf("SEMANTIC ERROR(%d): Initializer for variable '%s' requires both operands be arrays or not but variable is not an array and rhs is an array.\n",
                     current->lineno, current->attr.name);
               numErrors++;
            }
         }
         // no break on purpose, follow through into ParamK
      case ParamK:
         if (current->child[0] != NULL) {
           current->isAssigned = true; 
         }

         if (insertError(current, symtab)) {
            if (symtab->depth() == 1) {
               current->varKind = Global;
               current->offset = goffset;
               goffset -= current->size;
            } else if (current->isStatic) {
               current->varKind = LocalStatic;
               current->offset = goffset;
               goffset -= current->size;

               {
                  char *newName;
                  newName = new char[strlen(current->attr.name)+10];
                  sprintf(newName, "%s-%d", current->attr.name, ++varCounter);
                  symtab->insertGlobal(newName, current);
                  delete [] newName;
               }

            } else {
               current->varKind = Local;
               current->offset = foffset;
               foffset -= current->size;
            }
         } else {
            lookupNode = (TreeNode *)symtab->lookup(current->attr.name);
            printf("SEMANTIC ERROR(%d): Symbol '%s' is already declared at line %d.\n", 
                  current->lineno, current->attr.name, lookupNode->lineno);
            numErrors++;
         }

         if (current->kind.decl == ParamK) {
            current->varKind = Parameter;
         } else if (current->isArray) {
            current->offset--;
         }

         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         break;
      case FuncK:
         newScope = 0;
         foffset = -2;
         
         if (insertError(current, symtab) == false) {
            lookupNode = (TreeNode *)symtab->lookup(current->attr.name);
            printf("SEMANTIC ERROR(%d): Symbol '%s' is already declared at line %d.\n", 
                  current->lineno, current->attr.name, lookupNode->lineno);
            numErrors++;
         }

         symtab->enter(current->attr.name);


         // store which function we are in for children to reference
         funcInside = current;
         
         treeTraverse(current->child[0], symtab);
         current->size = foffset;
         treeTraverse(current->child[1], symtab);
         treeTraverse(current->child[2], symtab);

         current->varKind = Global;

         symtab->leave();

         break;
   }
}

/*
 * @brief recursively analyze every node in the tree
 * 
 * @param current - Current node to be analyzed
 * @param symtab - Symbol table for looking up children nodes
 * 
 * @return void
 */
void treeTraverse(TreeNode *current, SymbolTable *symtab) {
   if (current == NULL) {
      return;
   }

   switch(current->nodekind) {
      case DeclK:
         treeDeclTraverse(current, symtab);
         break;
      case ExpK:
         treeExpTraverse(current, symtab);
         break;
      case StmtK:
         treeStmtTraverse(current, symtab);
         break;
   }

   if (current->sibling) {
      treeTraverse(current->sibling, symtab);
   }
}

void checkIsUsed(std::string str, void *node) {
   TreeNode *tree = (TreeNode *)node;

   if (tree && !tree->isUsed && tree->lineno != -1) {
      switch (tree->kind.decl) {
         case VarK: {
            char *pos = strchr(tree->attr.name, '-');
            if (pos != NULL) {
               *pos = '\0';
            }

            printf("SEMANTIC WARNING(%d): The variable '%s' seems not to be used.\n",
                  tree->lineno, tree->attr.name);
            tree->isUsed = true;
            numWarnings++;
            break;           
         }
         case ParamK:
            printf("SEMANTIC WARNING(%d): The parameter '%s' seems not to be used.\n",
                  tree->lineno, tree->attr.name);
            numWarnings++;
            tree->isUsed = true;
            break;

         case FuncK:
            if (strcmp(tree->attr.name, "main") == 0) {
               break;
            }
            
            printf("SEMANTIC WARNING(%d): The function '%s' seems not to be used.\n",
                  tree->lineno, tree->attr.name);
            tree->isUsed = true;
            numWarnings++;
            break;

      }

   }
}

/*
 * @brief Perform semantic analysis on an AST
 * 
 * @param syntree - syntax tree to perform analysis on
 * @param symtabX - symbol table to be filled in 
 * @param globalOffset - will be set past the last global values
 * 
 * @return Annotated syntax tree
 */
TreeNode *semanticAnalysis(TreeNode *syntree, SymbolTable *symtabX, int &globalOffset) {

   syntree = loadIOLib(syntree);

   treeTraverse(syntree, symtabX);

//   symtabX->applyToAll(checkIsUsed);

   globalOffset = goffset;
   
   return syntree;
}
