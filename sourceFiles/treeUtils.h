#ifndef _UTIL_H_
#define _UTIL_H_
#include "treeNodes.h"
#include "scanType.h"
#include <string.h>

// Initializes a new declaration node
TreeNode *newDeclNode(DeclKind kind,
                      ExpType type,
                      TokenData *token=NULL,
                      TreeNode *c0=NULL,
                      TreeNode *c1=NULL,
                      TreeNode *c2=NULL);  // save TokenData block!!

// Initializes a new statement node
TreeNode *newStmtNode(StmtKind kind,
                      TokenData *token,
                      TreeNode *c0=NULL,
                      TreeNode *c1=NULL,
                      TreeNode *c2=NULL);

// Initializes a new expression node
TreeNode *newExpNode(ExpKind kind,
                     TokenData *token,
                     TreeNode *c0=NULL,
                     TreeNode *c1=NULL,
                     TreeNode *c2=NULL);

// Turn the expression type into a string
char *expToStr(ExpType type, bool isStatic, bool isArray);

// Return a string containing the name of the variable kind
char *varKindToString(int kind);

// Print out the tree node
void printTreeNode(FILE *out, TreeNode *syntaxTree, bool showExpType, bool showAlloc);

// Print out the AST
void printTree(FILE *out, TreeNode *syntaxTree, bool showExpType, bool showAlloc);

// Initializes token strings for easy conversion
void initTokenStrings();

// add a sibling s to the node t
TreeNode* addSibling(TreeNode* t, TreeNode* s);

// set the type of the tree node with that of the expression type
void setType(ExpType eType, TreeNode* tree, bool isStatic);

#endif
