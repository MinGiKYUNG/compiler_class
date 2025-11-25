#include <stdio.h>          // fprintf 등을 사용하기 위한 표준 입출력 헤더
#include <string.h>
#include "codegen_x86.h"    // AST 타입과 gen_x86_program 선언이 들어 있는 헤더


/**********************************************************************************
64비트 모드(x86-64)에서는 “일반 목적 레지스터”가 여러 개 있음:
- rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp
- r8 ~ r15

각각은 64비트(8바이트) 레지스터고, 하위 32비트(eax), 16비트(ax), 8비트(al)처럼 잘라 쓸 수도 있음.

우리가 쓰는 리눅스 x86-64(SysV ABI)에서 관습적으로 다음을 사용함:
- 함수 반환값 → rax에 넣어서 리턴
- 스택 포인터 → rsp
- 프레임 포인터 → rbp

일부 레지스터는 “함수 호출해도 값이 유지돼야 하는 레지스터(callee-saved)”,
나머지는 “마음대로 써도 되는 레지스터(caller-saved)”로 약속되어 있음.
***********************************************************************************/

FILE *asm_out = NULL;   // ✨ 전역 변수 실제 정의

// 간단한 심볼 테이블: 로컬 변수만, 최대 128개라고 가정
typedef struct {
    char* name;
    int   offset;   // rbp 기준 음수 오프셋 (예: -4, -8, ...)
} Symbol;

static Symbol symtab[128];
static int symcount = 0;
static int next_offset = -4;   // 첫 번째 로컬 변수 위치

static Symbol* find_symbol(const char* name) {
    for (int i = 0; i < symcount; ++i) {
        if (strcmp(symtab[i].name, name) == 0)
            return &symtab[i];
    }
    return NULL;
}

static Symbol* add_symbol(char* name) {
    Symbol* s = find_symbol(name);
    if (s) return s; // 이미 있으면 그대로 사용

    if (symcount >= 128) {
        fprintf(stderr, "symbol table overflow\n");
        return NULL;
    }
    symtab[symcount].name = name;   // 이름은 free는 하지 않음 (AST 쪽에서 관리)
    symtab[symcount].offset = next_offset;
    next_offset -= 4;               // 다음 변수는 그 아래
    return &symtab[symcount++];
}

// 전방 선언
static void gen_stmt(AST* node, FILE* out);
static void gen_expr(AST* node, FILE* out);

// 산술 표현식 코드 생성
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

    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV: {
        // left 계산 -> eax, 스택에 저장
        gen_expr(node->left, out);
        fprintf(out, "    pushq %%rax\n");

        // right 계산 -> eax
        gen_expr(node->right, out);
        fprintf(out, "    movl %%eax, %%ebx\n"); // right -> ebx

        // 스택에서 left 꺼내서 eax
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
            // edx:eax / ebx -> eax(몫)
            fprintf(out, "    cltd\n");           // eax sign-extend -> edx:eax
            fprintf(out, "    idivl %%ebx\n");
            break;
        default:
            break;
        }
        break;
    }

    default:
        // 표현식이 아닌 노드(AST_ASSIGN 등)는 gen_stmt에서 처리
        fprintf(out, "    movl $0, %%eax\n");
        break;
    }
}

// 문장 코드 생성
static void gen_stmt(AST* node, FILE* out) {
    if (!node) return;

    switch (node->type) {
    case AST_STMT_LIST:
        gen_stmt(node->left, out);
        gen_stmt(node->right, out);
        break;

    case AST_VAR_DECL: {
        // 스택에 16바이트 공간 확보
        Symbol* s = add_symbol(node->name);
        if (!s) break;
        fprintf(out, "    subq $16, %%rsp\n");
        // 초기값이 있으면 거기에 저장
        if (node->left) {
            gen_expr(node->left, out);
            fprintf(out, "    movl %%eax, %d(%%rbp)\n", s->offset);
        }
        break;
    }

    case AST_ASSIGN: {
        // 오른쪽 expr 계산 -> eax
        gen_expr(node->right, out);
        // 왼쪽은 AST_VAR라고 가정
        AST* var = node->left;
        if (var && var->type == AST_VAR) {
            Symbol* s = find_symbol(var->name);
            if (!s) {
                // 선언 안 된 변수면 새로 만들어 줄 수도 있지만,
                // 여기서는 에러만 찍고 무시
                fprintf(stderr, "assign to undefined variable: %s\n", var->name);
            } else {
                fprintf(out, "    movl %%eax, %d(%%rbp)\n", s->offset);
            }
        }
        break;
    }

    case AST_PRINTF: {
        // expr 계산 -> eax
        gen_expr(node->left, out);
        // SysV ABI: 첫 번째 인자 rdi, 두 번째 인자 rsi
        // int printf(const char *fmt, ...);
        fprintf(out, "    movl %%eax, %%esi\n");      // value -> 2번째 인자
        fprintf(out, "    leaq fmt_print_int(%%rip), %%rdi\n"); // 포맷 문자열 주소
        fprintf(out, "    movl $0, %%eax\n");         // 가변 인자 호출 전 eax = 0
        fprintf(out, "    call printf\n");
        break;
    }

    case AST_SCANF: {
        // scanf("%d", &x);
        Symbol* s = find_symbol(node->name);
        if (!s) {
            fprintf(stderr, "scanf to undefined variable: %s\n", node->name);
            break;
        }
        fprintf(out, "    leaq %d(%%rbp), %%rsi\n", s->offset); // &x
        fprintf(out, "    leaq fmt_scanf_int(%%rip), %%rdi\n"); // "%d"
        fprintf(out, "    movl $0, %%eax\n");
        fprintf(out, "    call scanf\n");
        break;
    }

    default:
        // expr; 같은 것도 여기서 처리 (결과는 버림)
        gen_expr(node, out);
        break;
    }
}

void gen_x86_program(AST* root, FILE* out) {
    // 심볼 테이블 초기화
    symcount = 0;
    next_offset = -4;

    // 데이터 영역: printf/scanf용 포맷 문자열
    fprintf(out, "    .section .rodata\n");
    fprintf(out, "fmt_print_int:\n");
    fprintf(out, "    .string \"%%d\\n\"\n");
    fprintf(out, "fmt_scanf_int:\n");
    fprintf(out, "    .string \"%%d\"\n");

    // 코드 영역
    fprintf(out, "    .text\n");
    fprintf(out, "    .globl main\n");
    fprintf(out, "main:\n");

    // 함수 프로로그
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");

    // 문장들 코드 생성
    gen_stmt(root, out);

    // 리턴 값: 0
    fprintf(out, "    movl $0, %%eax\n");

    // 함수 에필로그
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}