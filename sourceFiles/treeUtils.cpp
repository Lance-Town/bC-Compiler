/*
 * @author Lance Townsend
 *
 * @brief Utility functions to aid in parsing and building
 * the syntax tree
 *
 */

#include <string.h>
#include "treeUtils.h"
#include "semantics.h"
#include "parser.tab.h"

#define TYPE_SIZE 64

// GLOBALS
char *largerTokens[LASTTERM+1];
char expTypeToStrBuffer[80];

// function prototypes
char *expToStr(ExpType type, bool isStatic, bool isArray);
void showAllocation(FILE *out, TreeNode *tnode);
void initTokenStrings();
TreeNode* initNewNode(TreeNode *c0, TreeNode *c1, TreeNode *c2, TokenData *token);
TreeNode *newDeclNode(DeclKind kind, ExpType type, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2);
TreeNode *newStmtNode(StmtKind kind, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2);
TreeNode *newExpNode(ExpKind kind, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2);
TreeNode* addSibling(TreeNode* t, TreeNode* s);
void setType(ExpType eType, TreeNode* tree, bool isStatic);
static void printSpaces(FILE *listing, int depth);
char *expToStr(ExpType type, bool isStatic, bool isArray);
void printTreeNode(FILE *listing, TreeNode *tree, bool showExpType, bool showAlloc);
void printTreeRec(FILE *listing, int depth, int siblingCnt, TreeNode *tree, bool showExpType, bool showAlloc);
void printTree(FILE *listing, TreeNode *tree, bool showExpType, bool showAlloc);
char *varKindToString(int kind);
void showAllocation(FILE *out, TreeNode *tnode);

/*
 * @brief Initializes token strings for easy conversion
 *
 * @return void
 */
void initTokenStrings() {
   largerTokens[ADDASS] = (char *)"+=";
   largerTokens[AND] = (char *)"and";
   largerTokens[BOOL] = (char *)"bool";
   largerTokens[BOOLCONST] = (char *)"boolconst";
   largerTokens[BREAK] = (char *)"break";
   largerTokens[BY] = (char *)"by";
   largerTokens[CHAR] = (char *)"char";
   largerTokens[CHARCONST] = (char *)"charconst";
   largerTokens[CHSIGN] = (char *)"chsign";
   largerTokens[DEC] = (char *)"--";
   largerTokens[DIVASS] = (char *)"/=";
   largerTokens[DO] = (char *)"do";
   largerTokens[ELSE] = (char *)"else";
   largerTokens[FOR] = (char *)"for";
   largerTokens[GEQ] = (char *)">=";
   largerTokens[ID] = (char *)"id";
   largerTokens[IF] = (char *)"if";
   largerTokens[INC] = (char *)"++";
   largerTokens[INT] = (char *)"int";
   largerTokens[LEQ] = (char *)"<=";
   largerTokens[MAX] = (char *)":>:";
   largerTokens[MIN] = (char *)":<:";
   largerTokens[MULASS] = (char *)"*=";
   largerTokens[NEQ] = (char *)"!=";
   largerTokens[NOT] = (char *)"not";
   largerTokens[NUMCONST] = (char *)"numconst";
   largerTokens[OR] = (char *)"or";
   largerTokens[RETURN] = (char *)"return";
   largerTokens[SIZEOF] = (char *)"sizeof";
   largerTokens[STATIC] = (char *)"static";
   largerTokens[STRINGCONST] = (char *)"stringconst";
   largerTokens[SUBASS] = (char *)"-=";
   largerTokens[THEN] = (char *)"then";
   largerTokens[TO] = (char *)"to";
   largerTokens[WHILE] = (char *)"while";
   largerTokens[LASTTERM] = (char *)"lastterm";

}

/*
 * @brief Creates a new node and initializes its children
 *
 * @param c0, c1, c2 - child nodes to add to new node
 * @param token - data about the node to be initialized
 *
 * @return new node created
 */
TreeNode* initNewNode(TreeNode *c0, TreeNode *c1, TreeNode *c2, TokenData *token) {
   TreeNode *newNode = new TreeNode;

   newNode->child[0] = c0;
   newNode->child[1] = c1;
   newNode->child[2] = c2;
   newNode->sibling = NULL;

   if (token != NULL) {
      newNode->lineno = token->linenum;
      newNode->attr.op = token->tokenclass;
      newNode->attr.value = token->nvalue;
      newNode->attr.cvalue = token->cvalue;
      newNode->attr.name = token->tokenstr;
      newNode->attr.string = token->svalue;
   }

   newNode->isStatic = false;
   newNode->isArray = false;
   newNode->isConst = false;
   newNode->isUsed = false;
   newNode->isAssigned = false;
   newNode->size = 1;

   return newNode;
}

/*
 * @brief Initializes a new declaration node
 *
 * @param kind - The kind of declaration: VarK, FuncK, or ParamK
 * @param type - Expression type of: Void, Integer, Boolean, Char, UndefinedType
 * @param c0, c1, c2 - child nodes to add to new node
 * @param token - data about the node to be initialized
 *
 * @return new declaration node created
 */
TreeNode *newDeclNode(DeclKind kind, ExpType type, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2)
{
   TreeNode *newNode = initNewNode(c0, c1, c2, token);

   newNode->nodekind = DeclK;
   newNode->kind.decl = kind;
   newNode->type = type;
   
   return newNode;
}

/*
 * @brief Initializes a new statement node
 *
 * @param kind - The kind of statement: IfK, WhileK, ForK, etc.
 * @param c0, c1, c2 - child nodes to add to new node
 * @param token - data about the node to be initialized
 *
 * @return new statement node created
 */
TreeNode *newStmtNode(StmtKind kind, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2)
{
   TreeNode *newNode = initNewNode(c0, c1, c2, token);

   newNode->nodekind = StmtK;
   newNode->kind.stmt = kind;
   
   return newNode;
}

/*
 * @brief Initializes a new expression node
 *
 * @param kind - The kind of expression: Assign, CallK, ConstanK, etc
 * @param c0, c1, c2 - child nodes to add to new node
 * @param token - data about the node to be initialized
 *
 * @return new expression node created
 */
TreeNode *newExpNode(ExpKind kind, TokenData *token, TreeNode *c0, TreeNode *c1, TreeNode *c2)
{
   TreeNode *newNode = initNewNode(c0, c1, c2, token);

   newNode->nodekind = ExpK;
   newNode->kind.exp = kind;
   
   return newNode;
}

/*
 * @brief add a sibling to the node
 *
 * @param t - node to add the sibling to 
 * @param s - sibling to be added to t
 *
 * @return node with the new sibling attached
 */
TreeNode* addSibling(TreeNode* t, TreeNode* s) {
   if (s == NULL) {
      //printf("SYSTEM ERROR: SIBLING IS NULL\n");
      return t;
   }

   if (t == NULL) {
      return s;
   }

   TreeNode *tmp = t;
   while (tmp->sibling != NULL) {
      tmp = tmp->sibling;
   }
   tmp->sibling = s;
   return t;
}

/*
 * @brief set the type of the tree node with that of the expression type
 *
 * @return void
 */
void setType(ExpType eType, TreeNode* tree, bool isStatic) {
   TreeNode* tmp = tree;
   while (tmp != NULL) {
      tmp->type = eType;
      tmp->isStatic = isStatic;
      tmp = tmp->sibling;
   }

   return;
}

/*
 * @brief Print out the spaces and dots to signify levels of children
 *
 * @return void
 */
static void printSpaces(FILE *listing, int depth)
{
    for (int i=0; i<depth; i++) fprintf(listing, ".   ");
}

/*
 * @brief Turn the expression type into a string
 *
 * @return expression type as a string
 */
char *expToStr(ExpType type, bool isStatic, bool isArray) {
   char *typeName;
   switch (type) {
      case Void:
         typeName = (char *)"type void";
         break;
      case Integer:
         typeName = (char *)"type int";
         break;
      case Boolean:
         typeName = (char *)"type bool";
         break;
      case Char:
         typeName = (char *)"type char";
         break;
      case UndefinedType:
         typeName = (char *)"type UndefinedType";
         break;
   }

   sprintf(expTypeToStrBuffer, "%s%s%s", 
         (isStatic ? "static " : ""),
         (isArray ? "array of " : ""), typeName);

   return strdup(expTypeToStrBuffer); // memory leak
}

/*
 * @brief Print out the tree node
 *
 * @return void
 */
void printTreeNode(FILE *listing, TreeNode *tree, bool showExpType, bool showAlloc) {
   if (tree == NULL) {
      fprintf(listing, "DEBUG: Cannot print NULL tree\ttreeUtils.cpp::printTreeNode");
      return;
   }

   if (tree->nodekind == ExpK) {
      //initOperands(tree);
   }

   char *type;
   type = expToStr(tree->type, tree->isStatic, tree->isArray);

    if (tree->nodekind == DeclK) {
       if (tree->kind.decl == VarK) {
          fprintf(listing, "Var: %s of %s ", tree->attr.name, type);
          showAllocation(listing, tree);
       } else if (tree->kind.decl == FuncK) {
          fprintf(listing, "Func: %s returns %s ", tree->attr.name, type);
          showAllocation(listing, tree);
       } else if (tree->kind.decl == ParamK) {
          fprintf(listing, "Parm: %s of %s ", tree->attr.name, type);
          showAllocation(listing, tree);
       } else {
          fprintf(listing, "DEBUG: Invalid Decl Kind Declared\ttreeUtils.cpp::printTreeNode");
       }
    } else if (tree->nodekind == StmtK) {
       if (tree->kind.stmt == IfK) {
          fprintf(listing, "If ");
       } else if (tree->kind.stmt == WhileK) {
          fprintf(listing, "While ");
       } else if (tree->kind.stmt == ForK) {
          fprintf(listing, "For ");
          showAllocation(listing, tree);
       } else if (tree->kind.stmt == CompoundK) {
          fprintf(listing, "Compound ");
          showAllocation(listing, tree);
       } else if (tree->kind.stmt == ReturnK) {
          fprintf(listing, "Return ");
       } else if (tree->kind.stmt == BreakK) {
          fprintf(listing, "Break ");
       } else if (tree->kind.stmt == RangeK) {
          fprintf(listing, "Range ");
       } else {
          fprintf(listing, "DEBUG: Invalid Stmt Type\ttreeUtils.cpp::printTreeNode\n");
       }
    } else if (tree->nodekind == ExpK) {
       if (tree->kind.exp == AssignK) {
          fprintf(listing, "Assign: %s of %s ", tree->attr.name, type);
       } else if (tree->kind.exp == CallK) {
          fprintf(listing, "Call: %s of %s ", tree->attr.name, type);
       } else if (tree->kind.exp == ConstantK) {
          if (tree->type == Char) {
            if (tree->isArray) {
               fprintf(listing, "Const \"%s\" of %s ", tree->attr.name, type);
               showAllocation(listing, tree);
            } else {
               fprintf(listing, "Const '%c' of %s ", tree->attr.cvalue, type);
            }
          } else {
             fprintf(listing, "Const %s of %s ", tree->attr.name, type);
          }
       } else if (tree->kind.exp == IdK) {
          fprintf(listing, "Id: %s of %s ", tree->attr.name, type);
          showAllocation(listing, tree);
       } else if (tree->kind.exp == OpK) {
          type = expToStr(tree->type, tree->isStatic, false);
          fprintf(listing, "Op: %s of %s ", tree->attr.name, type); 
          
       } else {
          fprintf(listing, "DEBUG: Invalid ExpKind\ttreeUtils.cpp::printTreeNode");
       }
    } else {
       fprintf(listing, "DEBUG: Invalid NodeKind Declared\ttreeUtils.cpp::printTreeNode");
    }

   fprintf(listing, "[line: %d]", tree->lineno);
}

/*
 * @brief Recursively print out the AST
 *
 * @param listing - stream to output to
 * @param depth - depth of the node currently being visited
 * @param siblingCnt - count of siblings
 * @param tree - tree node to print out
 * @param showExpType, showAlloc - debugging parameters
 *
 * @return void
 */
void printTreeRec(FILE *listing, int depth, int siblingCnt, TreeNode *tree, bool showExpType, bool showAlloc)
{
   if (tree == NULL) {
      return;
   }

   int childCnt;
   if (tree!=NULL) {
      // print self
      printTreeNode(listing, tree, showExpType, showAlloc);
      fprintf(listing, "\n");

      // print children
      for (childCnt = 0; childCnt<MAXCHILDREN; childCnt++) {
         if (tree->child[childCnt]) {
            printSpaces(listing, depth);
            fprintf(listing, "Child: %d  ", childCnt);
            printTreeRec(listing, depth+1, 1, tree->child[childCnt], showExpType, showAlloc);
         }
      }
   }
   // print sibling
   tree = tree->sibling;
   if (tree!=NULL) {
      if (depth) {
         printSpaces(listing, depth-1);
         fprintf(listing, "Sibling: %d  ", siblingCnt);
      }
      printTreeRec(listing, depth, siblingCnt+1, tree, showExpType, showAlloc);
   }
   fflush(listing);
}

/*
 * @brief Print out the AST
 *
 * @param listing - stream to output to
 * @param tree - tree node to print out
 * @param showExpType, showAlloc - debugging parameters
 *
 * @return void
 */
void printTree(FILE *listing, TreeNode *tree, bool showExpType, bool showAlloc)
{
   if(tree == NULL){
      printf("NULL tree");
      return;
   }
   printTreeRec(listing, 1, 1, tree, showExpType, showAlloc);
}

/*
 * @brief Return a string containing the name of the variable kind
 */
char *varKindToString(int kind) {
   switch(kind) {
      case None: {
         return (char*)"None";
      }
      case Local: {
         return (char*)"Local";
      }
      case Global: {
         return (char*)"Global";
      }
      case Parameter: {
         return (char*)"Parameter";
      }
      case LocalStatic: {
         return (char*)"LocalStatic";
      }
      default: {
         return (char*)"unknownVarKind";
      }
   }
}


/*
 * @brief show the allocation values for the tree node 
 *
 * @return void
 */
void showAllocation(FILE *out, TreeNode *tnode) {
   fprintf(out, "[mem: %s loc: %d size: %d] ", varKindToString(tnode->varKind), tnode->offset, tnode->size);
   return;
}

