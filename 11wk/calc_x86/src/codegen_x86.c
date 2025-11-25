#include <stdio.h>          // fprintf 등을 사용하기 위한 표준 입출력 헤더
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

/*
 * x86-64 레지스터 사용 규칙 (이 파일에서)
 *
 * - %rax : 표현식 평가 결과를 담는 레지스터.
 *          항상 "이 AST의 계산 결과"가 %rax에 남도록 코드를 생성한다.
 *
 * - %rbx : 이항 연산에서 오른쪽 피연산자 값을 잠시 저장하는 임시 레지스터.
 *          left → %rax, push; right → %rax, then %rbx 로 옮겨서 사용.
 *
 * - %rbp : 함수의 프레임 포인터. 
 *          함수 프로로그/에필로그에서 스택 프레임 기준점으로 사용한다.
 *
 * - %rsp : 스택 포인터.
 *          push/pop 명령으로 자동으로 증감되며, 임시 값(왼쪽 결과 등)을
 *          스택에 저장/복구할 때 사용한다.
 *
 * - %rdx : 나눗셈(idivq)에서 RDX:RAX 형태의 피제수 상위 부분으로 사용된다.
 *          cqto로 RAX를 RDX:RAX로 부호 확장한 뒤 idivq를 수행하면,
 *          몫은 %rax, 나머지는 %rdx에 남는다.
 */

// 내부에서만 쓰이는 재귀 함수:
// 주어진 AST node를 계산하는 x86-64 어셈블리를 FILE* out에 출력한다.
// 규칙: 이 함수들은 "계산 결과를 항상 %rax 레지스터에 남긴다".
static void gen_expr(AST* node, FILE* out) {
    if (!node) return;      // 방어 코드: node가 NULL이면 아무 것도 하지 않고 반환

    switch (node->type) {   // 노드 타입에 따라 분기 (정수 / + / - / * / / )
    case AST_INT:           // 정수 리터럴 노드인 경우
        // 정수 값을 그대로 %rax 레지스터에 로드한다.
        // 예: value가 42면 "movq $42, %rax"
        fprintf(out, "    movq $%d, %%rax\n", node->value);
        break;

    case AST_ADD:           // 덧셈 노드: left + right
        // 1) 왼쪽 서브트리를 먼저 계산해서 결과를 %rax에 넣는다.
        gen_expr(node->left, out);
        // 2) 왼쪽 결과를 스택에 잠시 저장해 둔다.
        fprintf(out, "    pushq %%rax\n");
        // 3) 오른쪽 서브트리를 계산해서 결과를 다시 %rax에 넣는다.
        gen_expr(node->right, out);
        // 4) 오른쪽 결과(%rax)를 %rbx로 옮겨 둔다.
        fprintf(out, "    movq %%rax, %%rbx\n");
        // 5) 스택에서 왼쪽 결과를 꺼내 %rax로 복구한다.
        fprintf(out, "    popq %%rax\n");
        // 6) %rax = %rax + %rbx  (왼쪽 + 오른쪽)
        fprintf(out, "    addq %%rbx, %%rax\n");
        break;

    case AST_SUB:           // 뺄셈 노드: left - right
        // 1) 왼쪽 서브트리 계산 → 결과는 %rax
        gen_expr(node->left, out);
        // 2) 왼쪽 결과를 스택에 저장
        fprintf(out, "    pushq %%rax\n");
        // 3) 오른쪽 서브트리 계산 → 결과는 %rax
        gen_expr(node->right, out);
        // 4) 오른쪽 결과를 %rbx로 이동
        fprintf(out, "    movq %%rax, %%rbx\n");
        // 5) 스택에서 왼쪽 결과를 꺼내 %rax에 복구
        fprintf(out, "    popq %%rax\n");
        // 6) %rax = %rax - %rbx  (왼쪽 - 오른쪽)
        fprintf(out, "    subq %%rbx, %%rax\n");
        break;

    case AST_MUL:           // 곱셈 노드: left * right
        // 1) 왼쪽 서브트리 계산 → %rax
        gen_expr(node->left, out);
        // 2) 왼쪽 결과를 스택에 저장
        fprintf(out, "    pushq %%rax\n");
        // 3) 오른쪽 서브트리 계산 → %rax
        gen_expr(node->right, out);
        // 4) 오른쪽 결과를 %rbx로 이동
        fprintf(out, "    movq %%rax, %%rbx\n");
        // 5) 스택에서 왼쪽 결과를 꺼내 %rax에 복구
        fprintf(out, "    popq %%rax\n");
        // 6) 부호 있는 정수 곱셈: %rax = %rax * %rbx
        fprintf(out, "    imulq %%rbx, %%rax\n");
        break;

    case AST_DIV:           // 나눗셈 노드: left / right
        // x86-64에서 idivq는 (RDX:RAX) / 피연산자 형태로 동작하므로
        // 1) 왼쪽 값(left)을 %rax에 두고
        // 2) 오른쪽 값(right)을 다른 레지스터에 두고
        // 3) cqto로 RDX:RAX를 준비한 뒤 idivq 실행

        // 1) 왼쪽 서브트리 계산 → %rax
        gen_expr(node->left, out);
        // 2) 왼쪽 결과를 스택에 저장
        fprintf(out, "    pushq %%rax\n");
        // 3) 오른쪽 서브트리 계산 → %rax
        gen_expr(node->right, out);
        // 4) 오른쪽 결과를 %rbx로 이동 (나눌 수)
        fprintf(out, "    movq %%rax, %%rbx\n");
        // 5) 스택에서 왼쪽 결과를 꺼내 %rax에 복구 (피제수)
        fprintf(out, "    popq %%rax\n");
        // 6) RAX를 부호 확장해서 RDX:RAX 레지스터 쌍 준비 (idivq 요구사항)
        fprintf(out, "    cqto\n");
        // 7) (RDX:RAX) / %rbx → 몫은 %rax, 나머지는 %rdx
        fprintf(out, "    idivq %%rbx\n");
        // 이후 결과(몫)는 %rax에 남고, 이 함수를 호출한 쪽에서 계속 사용
        break;
    }
}

// 하나의 표현식 AST를 "main 함수" 형태의 x86-64 어셈블리 프로그램으로 감싸서 출력한다.
// root: 최상위 표현식 AST
// out : 어셈블리를 기록할 FILE* (예: out.s)
void gen_x86_program(AST* root, FILE* out) {
    // 코드 섹션 선언
    fprintf(out, "    .text\n");
    // 전역 심볼 main 선언 (링커가 entry point로 찾을 수 있게)
    fprintf(out, "    .globl main\n");
    // main 레이블 정의 시작
    fprintf(out, "main:\n");
    // 함수 프로로그: 이전 베이스 포인터(%rbp)를 스택에 저장
    fprintf(out, "    pushq %%rbp\n");
    // 현재 스택 포인터를 새 베이스 포인터로 설정 (스택 프레임 생성)
    fprintf(out, "    movq %%rsp, %%rbp\n");

    // 표현식을 계산하는 코드 생성:
    // gen_expr가 끝나면 결과 값은 %rax에 들어 있게 된다.
    gen_expr(root, out);

    // 함수 에필로그: 스택 프레임을 원래대로 복구
    fprintf(out, "    popq %%rbp\n");
    // 호출자로 복귀 (main 종료 → 프로그램 종료)
    fprintf(out, "    ret\n");
}
