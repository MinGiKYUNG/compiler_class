#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "codegen_x86.h"

/* --- 심볼 테이블 -------------------------------------------------- */

typedef struct {
    char* name;
    int   offset;   // rbp 기준 음수 오프셋 (예: -4, -8, ...)
} Symbol;

static Symbol symtab[128];
static int symcount    = 0;
static int next_offset = -4;   // 첫 변수는 -4(%rbp) 에 둔다.

/* 레이블 번호 */
static int label_counter = 0;

static int new_label(void) {
    return label_counter++;
}

static Symbol* find_symbol(const char* name) {
    for (int i = 0; i < symcount; ++i) {
        if (strcmp(symtab[i].name, name) == 0)
            return &symtab[i];
    }
    return NULL;
}

/* 변수 선언을 심볼 테이블에 추가 (스택은 나중에 한 번에 빼기) */
static Symbol* add_symbol(char* name) {
    Symbol* s = find_symbol(name);
    if (s) return s;

    if (symcount >= 128) {
        fprintf(stderr, "symbol table overflow\n");
        return NULL;
    }
    symtab[symcount].name   = name;       // 이름은 AST에서 관리
    symtab[symcount].offset = next_offset;
    next_offset -= 4;                      // 다음 변수는 아래로 4바이트
    return &symtab[symcount++];
}

/* --- 1패스: AST를 돌면서 변수 선언 수집 -------------------------------- */

static void collect_symbols(AST* node) {
    if (!node) return;

    switch (node->type) {
    case AST_VAR_DECL:
        /* int x; / int x = expr; */
        add_symbol(node->name);
        /* 초기값 expr도 훑어줘야 하므로 left 재귀 */
        collect_symbols(node->left);
        break;

    default:
        collect_symbols(node->left);
        collect_symbols(node->right);
        break;
    }
}

/* --- 코드 생성기 본체 ------------------------------------------------ */

static void gen_stmt(AST* node, FILE* out);
static void gen_expr(AST* node, FILE* out);

static void gen_expr(AST* node, FILE* out) {
    if (!node) return;

    switch (node->type) {
    case AST_INT:
        fprintf(out, "    movl $%d, %%eax\n", node->value);
        break;

    case AST_VAR: {
        Symbol* s = find_symbol(node->name);
        if (!s) {
            fprintf(stderr, "undefined variable: %s\n", node->name);
            fprintf(out, "    movl $0, %%eax\n");
        } else {
            fprintf(out, "    movl %d(%%rbp), %%eax\n", s->offset);
        }
        break;
    }

    /* 산술 + 비교 연산을 한 덩어리로 처리 */
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_LT:
    case AST_GT: {
        /* left → eax → 스택에 저장 */
        gen_expr(node->left, out);
        fprintf(out, "    pushq %%rax\n");

        /* right → eax → ebx */
        gen_expr(node->right, out);
        fprintf(out, "    movl %%eax, %%ebx\n");

        /* 스택에서 left 꺼내서 eax */
        fprintf(out, "    popq %%rax\n");

        switch (node->type) {
        case AST_ADD:
            fprintf(out, "    addl %%ebx, %%eax\n");
            break;
        case AST_SUB:
            fprintf(out, "    subl %%ebx, %%eax\n");
            break;
        case AST_MUL:
            fprintf(out, "    imull %%ebx, %%eax\n");
            break;
        case AST_DIV:
            fprintf(out, "    cltd\n");
            fprintf(out, "    idivl %%ebx\n");
            break;

        case AST_LT:
            /* eax(left) < ebx(right) → eax = 0 or 1 */
            fprintf(out, "    cmpl %%ebx, %%eax\n");
            fprintf(out, "    setl %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            break;

        case AST_GT:
            /* eax(left) > ebx(right) → eax = 0 or 1 */
            fprintf(out, "    cmpl %%ebx, %%eax\n");
            fprintf(out, "    setg %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            break;

        default:
            break;
        }
        break;
    }

    default:
        /* stmt에서 처리해야 하는 노드(ASSIGN 등)는 여기선 0만 리턴 */
        fprintf(out, "    movl $0, %%eax\n");
        break;
    }
}

static void gen_stmt(AST* node, FILE* out) {
    if (!node) return;

    switch (node->type) {
    case AST_STMT_LIST:
        gen_stmt(node->left, out);
        gen_stmt(node->right, out);
        break;

    case AST_VAR_DECL: {
        /* 심볼 테이블에는 collect_symbols 단계에서 이미 등록됨.
           여기서는 초기값이 있을 경우에만 실제 대입 수행. */
        if (node->left) {
            gen_expr(node->left, out);
            Symbol* s = find_symbol(node->name);
            if (!s) {
                fprintf(stderr, "var_decl for undefined symbol: %s\n", node->name);
            } else {
                fprintf(out, "    movl %%eax, %d(%%rbp)\n", s->offset);
            }
        }
        break;
    }

    case AST_ASSIGN: {
        gen_expr(node->right, out);
        AST* var = node->left;
        if (var && var->type == AST_VAR) {
            Symbol* s = find_symbol(var->name);
            if (!s) {
                fprintf(stderr, "assign to undefined variable: %s\n", var->name);
            } else {
                fprintf(out, "    movl %%eax, %d(%%rbp)\n", s->offset);
            }
        }
        break;
    }

    case AST_PRINTF:
        /* expr → eax, printf("%d\n", eax) */
        gen_expr(node->left, out);
        fprintf(out, "    movl %%eax, %%esi\n");
        fprintf(out, "    leaq fmt_print_int(%%rip), %%rdi\n");
        fprintf(out, "    movl $0, %%eax\n");
        fprintf(out, "    call printf\n");
        break;

    case AST_SCANF: {
        /* scanf("%d", &x); */
        Symbol* s = find_symbol(node->name);
        if (!s) {
            fprintf(stderr, "scanf to undefined variable: %s\n", node->name);
            break;
        }
        fprintf(out, "    leaq %d(%%rbp), %%rsi\n", s->offset);
        fprintf(out, "    leaq fmt_scanf_int(%%rip), %%rdi\n");
        fprintf(out, "    movl $0, %%eax\n");
        fprintf(out, "    call scanf\n");
        break;
    }

    case AST_IF: {
        int lbl_end = new_label();
        gen_expr(node->left, out);             // cond → eax
        fprintf(out, "    cmpl $0, %%eax\n");
        fprintf(out, "    je .Lend_%d\n", lbl_end);
        gen_stmt(node->right, out);            // then
        fprintf(out, ".Lend_%d:\n", lbl_end);
        break;
    }

    case AST_WHILE: {
        int lbl_begin = new_label();
        int lbl_end   = new_label();

        fprintf(out, ".Lbegin_%d:\n", lbl_begin);
        gen_expr(node->left, out);             // cond → eax
        fprintf(out, "    cmpl $0, %%eax\n");
        fprintf(out, "    je .Lend_%d\n", lbl_end);

        gen_stmt(node->right, out);            // body

        fprintf(out, "    jmp .Lbegin_%d\n", lbl_begin);
        fprintf(out, ".Lend_%d:\n", lbl_end);
        break;
    }

    default:
        /* expr; 같은 것도 여기서 처리 (결과는 버림) */
        gen_expr(node, out);
        break;
    }
}

/* --- 프로그램 전체 코드 생성 ------------------------------------------- */

void gen_x86_program(AST* root, FILE* out) {
    /* 심볼 테이블 초기화 */
    symcount      = 0;
    next_offset   = -4;
    label_counter = 0;

    /* 1패스: 변수 선언 모으기 */
    collect_symbols(root);

    /* 로컬 변수 전체 크기 계산: int 4바이트 * 개수 */
    int frame_bytes = symcount * 4;
    /* 16바이트 배수로 맞추기 */
    int aligned = (frame_bytes + 15) & ~15;

    /* 읽기 전용 데이터: printf/scanf 포맷 문자열 */
    fprintf(out, "    .section .rodata\n");
    fprintf(out, "fmt_print_int:\n");
    fprintf(out, "    .string \"%%d\\n\"\n");
    fprintf(out, "fmt_scanf_int:\n");
    fprintf(out, "    .string \"%%d\"\n");

    /* 코드 섹션 */
    fprintf(out, "    .text\n");
    fprintf(out, "    .globl main\n");
    fprintf(out, "main:\n");

    /* 프로로그 */
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");
    if (aligned > 0) {
        fprintf(out, "    subq $%d, %%rsp\n", aligned);
    }

    /* 본문 문장들 */
    gen_stmt(root, out);

    /* return 0; */
    fprintf(out, "    movl $0, %%eax\n");

    /* 에필로그 */
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}
