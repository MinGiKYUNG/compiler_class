#ifndef CODEGEN_X86_H              // 헤더가 중복 포함되는 것을 막기 위한 include guard 시작
#define CODEGEN_X86_H              // include guard의 매크로 정의

#include <stdio.h>                 // FILE* 타입을 사용하기 위해 stdio.h 포함
#include "ast.h"                   // AST 구조체와 ASTType 열거형을 사용하기 위해 ast.h 포함

// root: 프로그램의 최상위 AST 노드 (보통 전체 expr)
// out : 어셈블리 코드를 출력할 FILE* (예: fopen("out.s","w")로 연 파일)
void gen_x86_program(AST* root, FILE* out);  // AST에서 x86-64 어셈블리를 생성하는 함수 선언

#endif // CODEGEN_X86_H            // include guard 끝