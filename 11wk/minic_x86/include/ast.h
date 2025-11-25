#ifndef AST_H
#define AST_H

// AST 노드 종류
typedef enum {
    AST_INT,        // 정수 리터럴

    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,

    AST_VAR,        // 변수 참조:   x
    AST_ASSIGN,     // 대입:       x = expr

    AST_VAR_DECL,   // 변수 선언:   int x;   또는 int x = expr;
    AST_STMT_LIST,  // 문장 리스트: stmt1; stmt2; ...

    AST_PRINTF,     // printf(expr);
    AST_SCANF       // scanf(&x);
} ASTType;

// AST 노드 구조
typedef struct AST {
    ASTType type;

    int value;         // INT 리터럴 값 등
    char *name;        // 변수 이름 / 함수 이름 (필요한 노드에서 사용)

    struct AST* left;
    struct AST* right;
} AST;

// 기존 + 새로운 생성 함수들
AST* ast_int(int v);
AST* ast_bin(ASTType t, AST* l, AST* r);

// 변수/문장 관련
AST* ast_var(char* name);                     // x
AST* ast_assign(AST* var, AST* expr);         // x = expr
AST* ast_var_decl(char* name, AST* init);     // int x; / int x = expr;
AST* ast_stmt_list(AST* first, AST* second);  // first; second;

// I/O 관련
AST* ast_printf(AST* expr);                   // printf(expr);
AST* ast_scanf(char* name);                   // scanf(&x);

void ast_free(AST* node);

#endif // AST_H
