#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen_x86.h"

/* 매우 단순한 x86-64 SysV 코드 생성기
 * - int 타입만
 * - 지역변수/매개변수는 모두 스택에 int(8바이트)로 저장
 * - 표현식은 rax를 결과 레지스터로 사용
 * - 함수 호출은 최대 2개 인자만 지원 (rdi, rsi)
 * - DEBUG 매크로로 디버그 정보 출력 가능
 */

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

typedef struct {
    const char *name;
    int offset;       /* rbp 기준 음수 오프셋 */
    int is_param;
    int param_index;  /* 0,1,... */
} Var;

static const char *arg_regs[] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };

static int find_var(const Var *vars, int n, const char *name) {
    for (int i = 0; i < n; ++i) {
        if (strcmp(vars[i].name, name) == 0) return i;
    }
    return -1;
}

static void alloc_locals(Function *f, Var *vars, int *var_count, int *stack_size) {
    int count = 0;
    int offset = -8;
    int min_offset = 0;

    DEBUG_PRINT("Allocating locals for function: %s", f->name);

    /* 매개변수 먼저 */
    int param_index = 0;
    if (f->params) {
        Param *p = f->params->head;
        while (p) {
            vars[count].name = p->name;
            vars[count].offset = offset;
            vars[count].is_param = 1;
            vars[count].param_index = param_index;
            DEBUG_PRINT("  Param[%d]: %s at offset %d", param_index, p->name, offset);
            count++;
            min_offset = offset;
            offset -= 8;
            param_index++;
            p = p->next;
        }
    }

    /* 지역변수: stmt_list를 훑으면서 vardecl 수집 */
    if (f->body) {
        Stmt *s = f->body->head;
        while (s) {
            if (s->kind == STMT_VARDECL) {
                if (find_var(vars, count, s->u.vardecl.var_name) < 0) {
                    vars[count].name = s->u.vardecl.var_name;
                    vars[count].offset = offset;
                    vars[count].is_param = 0;
                    vars[count].param_index = -1;
                    DEBUG_PRINT("  Local var: %s at offset %d", s->u.vardecl.var_name, offset);
                    count++;
                    min_offset = offset;
                    offset -= 8;
                }
            }
            s = s->next;
        }
    }

    if (count == 0) {
        *var_count = 0;
        *stack_size = 0;
    } else {
        *var_count = count;
        *stack_size = -min_offset;
    }
    
    DEBUG_PRINT("Total variables: %d, Stack size: %d", *var_count, *stack_size);
}

static void gen_expr(Expr *e, Var *vars, int var_count);

/* 변수 위치에서 rax 로드 */
static void load_var_to_rax(const char *name, Var *vars, int var_count) {
    int idx = find_var(vars, var_count, name);
    if (idx < 0) {
        fprintf(stderr, "Unknown variable: %s\n", name);
        exit(1);
    }
    int off = vars[idx].offset;
    DEBUG_PRINT("Loading variable '%s' from offset %d to rax", name, off);
    printf("    movq %d(%%rbp), %%rax\n", off);
}

static void gen_binop(Expr *e, Var *vars, int var_count) {
    /* rhs 먼저, lhs 나중 */
    gen_expr(e->u.binop.rhs, vars, var_count);  /* rax = rhs */
    printf("    pushq %%rax\n");
    gen_expr(e->u.binop.lhs, vars, var_count);  /* rax = lhs */
    printf("    popq %%rcx\n");                 /* rcx = rhs */

    switch (e->u.binop.op) {
    case BIN_ADD:
        printf("    addq %%rcx, %%rax    # lhs + rhs\n");
        break;
    case BIN_SUB:
        printf("    subq %%rcx, %%rax    # lhs - rhs\n");
        break;
    case BIN_MUL:
        printf("    imulq %%rcx, %%rax   # lhs * rhs\n");
        break;
    case BIN_DIV:
        /* lhs / rhs, lhs=rax, rhs=rcx */
        printf("    cqto                 # sign-extend rax -> rdx:rax\n");
        printf("    idivq %%rcx          # lhs / rhs, quotient in rax\n");
        break;
    default:
        printf("    # unknown binop\n");
        break;
    }
}

static void gen_call(Expr *e, Var *vars, int var_count) {
    /* 최대 2개 인자만 지원 (rdi, rsi) */
    int argc = 0;
    ExprList *arg = e->u.call.args;

    DEBUG_PRINT("Generating function call to '%s'", e->u.call.func_name);

    /* 먼저 인자들을 평가해서 스택에 역순으로 쌓는다 (마지막 인자가 top) */
    /* 간단히: 왼쪽부터 차례로 평가해서 push, 나중에 레지스터로 옮김 */
    while (arg) {
        gen_expr(arg->expr, vars, var_count);  /* rax = arg */
        printf("    pushq %%rax\n");
        argc++;
        arg = arg->next;
    }

    if (argc > 6) {
        fprintf(stderr, "Too many arguments, only up to 6 supported\n");
        exit(1);
    }

    DEBUG_PRINT("Function '%s' has %d arguments", e->u.call.func_name, argc);

    /* 스택에서 꺼내서 역순으로 레지스터에 넣기 */
    for (int i = argc - 1; i >= 0; --i) {
        printf("    popq %s\n", arg_regs[i]);
    }

    printf("    call %s\n", e->u.call.func_name);
    /* 반환값은 rax에 있음 */
}

static void gen_expr(Expr *e, Var *vars, int var_count) {
    if (!e) return;

    switch (e->kind) {
    case EXPR_INT:
        printf("    movq $%d, %%rax\n", e->u.int_value);
        break;
    case EXPR_VAR:
        load_var_to_rax(e->u.var_name, vars, var_count);
        break;
    case EXPR_BINOP:
        gen_binop(e, vars, var_count);
        break;
    case EXPR_CALL:
        gen_call(e, vars, var_count);
        break;
    default:
        printf("    # unknown expr kind\n");
        break;
    }
}

static void gen_stmt(Stmt *s, Var *vars, int var_count, const char *end_label) {
    if (!s) return;

    switch (s->kind) {
    case STMT_VARDECL:
        /* 이미 스택에 공간을 잡았으므로 별도 코드 필요 없음 */
        /* 디버깅용 주석만 출력 */
        printf("    # var %s\n", s->u.vardecl.var_name);
        break;
    case STMT_ASSIGN: {
        gen_expr(s->u.assign.value, vars, var_count); /* rax = value */
        int idx = find_var(vars, var_count, s->u.assign.var_name);
        if (idx < 0) {
            fprintf(stderr, "Unknown variable in assign: %s\n", s->u.assign.var_name);
            exit(1);
        }
        int off = vars[idx].offset;
        printf("    movq %%rax, %d(%%rbp)   # %s = rax\n", off, s->u.assign.var_name);
        break;
    }
    case STMT_EXPR:
        gen_expr(s->u.expr, vars, var_count);
        break;
    case STMT_RETURN:
        gen_expr(s->u.expr, vars, var_count);   /* rax = return value */
        printf("    jmp %s\n", end_label);
        break;
    default:
        printf("    # unknown stmt kind\n");
        break;
    }
}

static void gen_function(Function *f) {
    Var vars[128];
    int var_count = 0;
    int stack_size = 0;

    DEBUG_PRINT("Generating code for function: %s", f->name);

    alloc_locals(f, vars, &var_count, &stack_size);

    int is_main = (strcmp(f->name, "main") == 0);

    if (is_main) {
        printf("    .globl main\n");
        printf("main:\n");
    } else {
        printf("    .globl %s\n", f->name);
        printf("%s:\n", f->name);
    }

    printf("    pushq %%rbp\n");
    printf("    movq %%rsp, %%rbp\n");
    if (stack_size > 0) {
        printf("    subq $%d, %%rsp\n", stack_size);
    }

    /* 매개변수 레지스터 -> 스택 변수 */
    for (int i = 0; i < var_count; ++i) {
        if (vars[i].is_param) {
            int pi = vars[i].param_index;
            if (pi < 6) {
                printf("    movq %s, %d(%%rbp)   # param %s\n",
                       arg_regs[pi], vars[i].offset, vars[i].name);
            } else {
                /* 6개 초과는 미지원 */
            }
        }
    }

    char end_label[64];
    snprintf(end_label, sizeof(end_label), ".Lend_%s", f->name);

    if (f->body) {
        Stmt *s = f->body->head;
        while (s) {
            gen_stmt(s, vars, var_count, end_label);
            s = s->next;
        }
    }

    /* 기본 반환: 값이 없으면 0 */
    printf("%s:\n", end_label);

    /*
    if (is_main) {
        // main이 명시적으로 반환하지 않으면 0 반환
        printf("    # default return 0 for main\n");
        printf("    movl $0, %%eax\n");
    }
    */
    printf("    leave\n");
    printf("    ret\n\n");
    
    DEBUG_PRINT("Finished generating code for function: %s", f->name);
}

void gen_x86_program(FunctionList *prog) {
    if (!prog) {
        fprintf(stderr, "No program.\n");
        return;
    }

    printf("    .text\n");

    Function *f = prog->head;
    while (f) {
        gen_function(f);
        f = f->next;
    }
}
