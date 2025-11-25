// src/main.c

#include <stdio.h>                 // printf, fopen, fclose 등을 사용하기 위한 헤더
#include <stdlib.h>                // exit, EXIT_FAILURE 등을 위해 포함(옵션)
#include "ast.h"                   // AST 타입 선언 포함
#include "codegen_x86.h"           // gen_x86_program 함수 선언 포함

int yyparse(void);                 // Bison이 생성한 파서 함수 선언 (parser.y에서 구현됨)

// parser.y에서 extern으로 선언한 asm_out을 여기서 정의한다.
FILE* asm_out = NULL;              // 어셈블리 출력 파일 포인터 전역 변수 (초기값은 NULL)

int main(int argc, char** argv) {  // 프로그램 진입점: argc/argv는 명령행 인자 (여기서는 사용하지 않음)
    // 어셈블리 출력 파일 out.s를 쓰기 모드로 연다.
    asm_out = fopen("out.s", "w"); // "out.s" 파일을 생성 또는 덮어쓰기로 오픈
    if (!asm_out) {                // 파일 열기에 실패한 경우
        perror("out.s");           // 시스템 에러 메시지 출력
        return 1;                  // 비정상 종료 코드 반환
    }

    // 사용자가 표현식을 입력하라는 안내 출력
    printf("Enter expression (end with Ctrl+D):\n");  // 표준 출력으로 안내 문구 출력

    // Bison 파서를 호출해서 입력을 읽고 파싱을 수행한다.
    // 파서 내부의 액션에서 AST를 만들고, gen_x86_program을 호출할 것이다.
    if (yyparse() == 0) {           // 표준 입력으로부터 토큰을 읽어 파싱 시작
        printf("\nout.s 파일이 생성되었습니다.\n");
        printf("다음 명령으로 실행 파일을 만들 수 있습니다:\n");
        printf("  gcc -no-pie out.s -o prog\n");
        printf("  ./prog; echo $?\n");
    } else {
        fprintf(stderr, "파싱 중 오류가 발생했습니다.\n");
    }
    // 어셈블리 출력 파일을 닫는다.
    fclose(asm_out);               // out.s 파일 닫기
    asm_out = NULL;                // 포인터를 NULL로 초기화 (안전상)

    return 0;                      // 정상 종료
}
