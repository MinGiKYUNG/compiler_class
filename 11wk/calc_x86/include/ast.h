#ifndef AST_H
#define AST_H

typedef enum {
    AST_INT,
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV
} ASTType;

typedef struct AST {
    ASTType type;
    int value;
    struct AST* left;
    struct AST* right;
} AST;

AST* ast_int(int v);
AST* ast_bin(ASTType t, AST* l, AST* r);
void ast_free(AST* node);

#endif // AST_H
