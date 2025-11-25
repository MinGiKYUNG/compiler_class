#include <stdlib.h>
#include "ast.h"

AST* ast_int(int v) {
    AST* n = (AST*)malloc(sizeof(AST));
    if (!n) return NULL;
    n->type = AST_INT;
    n->value = v;
    n->left = n->right = NULL;
    return n;
}

AST* ast_bin(ASTType t, AST* l, AST* r) {
    AST* n = (AST*)malloc(sizeof(AST));
    if (!n) return NULL;
    n->type = t;
    n->value = 0;
    n->left = l;
    n->right = r;
    return n;
}

void ast_free(AST* node) {
    if (!node) return;
    ast_free(node->left);
    ast_free(node->right);
    free(node);
}
