#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "codegen_x86.h"

int yyparse(void);
FILE* asm_out = NULL;    // ✨ 전역 변수 실제 정의
extern FILE* yyin;    // ✨ 추가: Flex가 사용하는 입력 스트림

int main(int argc, char** argv) {
    // 입력 파일 지정: ./prog source.minic 이런 식으로
    if (argc >= 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("입력 파일을 열 수 없습니다");
            return EXIT_FAILURE;
        }
    } else {
        yyin = stdin; // 없으면 표준 입력 사용
    }

    asm_out = fopen("out.s", "w");
    if (!asm_out) {
        perror("out.s를 열 수 없습니다");
        return EXIT_FAILURE;
    }

    if (yyparse() == 0) {
        printf("\nout.s 파일이 생성되었습니다.\n");
        printf("다음 명령으로 실행 파일을 만들 수 있습니다:\n");
        printf("  gcc -no-pie out.s -o prog\n");
        printf("  ./prog\n");
    } else {
        fprintf(stderr, "파싱 중 오류가 발생했습니다.\n");
    }

    fclose(asm_out);
    if (yyin != stdin) fclose(yyin);
    return 0;
}
