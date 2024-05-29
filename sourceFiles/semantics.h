#ifndef __SEMANTICS_H__
#define __SEMANTICS_H__

#include <string.h>
#include "treeNodes.h"
#include "treeUtils.h"
#include "symbolTable.h"

// Perform semantic analysis on an AST
TreeNode *semanticAnalysis(TreeNode *syntree, SymbolTable *symtabX, int &globalOffset);

#endif /* __SEMANTICS_H__ */
