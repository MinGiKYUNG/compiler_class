#include <stdlib.h>
#include <string.h>
#include "ast.h"

// 공통 노드 생성 헬퍼
static AST* ast_new(ASTType type) {
    AST* n = (AST*)malloc(sizeof(AST));
    if (!n) return NULL;
    n->type = type;
    n->value = 0;
    n->name = NULL;
    n->left = n->right = NULL;
    return n;
}

AST* ast_int(int v) {
    AST* n = ast_new(AST_INT);
    if (!n) return NULL;
    n->value = v;
    return n;
}

AST* ast_bin(ASTType t, AST* l, AST* r) {
    AST* n = ast_new(t);
    if (!n) return NULL;
    n->left = l;
    n->right = r;
    return n;
}

// yylex에서 strdup으로 할당한 문자열을 그대로 넘겨받아서 소유한다고 가정
AST* ast_var(char* name) {
    AST* n = ast_new(AST_VAR);
    if (!n) return NULL;
    n->name = name;  // 추가로 strdup 하지 않고 이름 소유
    return n;
}

AST* ast_assign(AST* var, AST* expr) {
    AST* n = ast_new(AST_ASSIGN);
    if (!n) return NULL;
    n->left = var;   // var 노드 (AST_VAR)
    n->right = expr;
    return n;
}

AST* ast_var_decl(char* name, AST* init) {
    AST* n = ast_new(AST_VAR_DECL);
    if (!n) return NULL;
    n->name = name;  // 변수 이름
    n->left = init;  // 초기값 expr (없으면 NULL)
    return n;
}

AST* ast_stmt_list(AST* first, AST* second) {
    AST* n = ast_new(AST_STMT_LIST);
    if (!n) return NULL;
    n->left = first;
    n->right = second;
    return n;
}

AST* ast_printf(AST* expr) {
    AST* n = ast_new(AST_PRINTF);
    if (!n) return NULL;
    n->left = expr;  // 출력할 expr
    return n;
}

AST* ast_scanf(char* name) {
    AST* n = ast_new(AST_SCANF);
    if (!n) return NULL;
    n->name = name;  // 입력 받을 변수 이름
    return n;
}

AST* ast_if(AST* cond, AST* then_stmt) {
    AST* n = ast_new(AST_IF);
    if (!n) return NULL;
    n->left  = cond;       // 조건식
    n->right = then_stmt;  // then 문장 (block 포함)
    return n;
}

AST* ast_while(AST* cond, AST* body) {
    AST* n = ast_new(AST_WHILE);
    if (!n) return NULL;
    n->left  = cond;   // 조건식
    n->right = body;   // 루프 body
    return n;
}

void ast_free(AST* node) {
    if (!node) return;
    ast_free(node->left);
    ast_free(node->right);
    // name은 yylex에서 strdup한 것을 우리가 소유한다고 보고 free
    if (node->name) free(node->name);
    free(node);
}
