%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "codegen_x86.h"

extern int yylex(void);
void yyerror(const char* s);

extern FILE* asm_out;
%}

%union {
    int   ival;
    char* sval;
    AST*  node;
}

%token <ival> NUMBER
%token <sval> IDENT
%token INT_KW
%token PRINTF_KW
%token SCANF_KW

%type <node> program stmt_list stmt expr

%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program
    : stmt_list
      {
          // 전체 프로그램 AST를 x86-64 어셈블리로 변환
          gen_x86_program($1, asm_out);
          ast_free($1);
      }
    ;

stmt_list
    : stmt                { $$ = $1; }
    | stmt_list stmt      { $$ = ast_stmt_list($1, $2); }
    ;

stmt
    // int x;
    : INT_KW IDENT ';'
      {
          $$ = ast_var_decl($2, NULL);
      }
    // int x = expr;
    | INT_KW IDENT '=' expr ';'
      {
          $$ = ast_var_decl($2, $4);
      }
    // x = expr;
    | IDENT '=' expr ';'
      {
          $$ = ast_assign(ast_var($1), $3);
      }
    // printf(expr);
    | PRINTF_KW '(' expr ')' ';'
      {
          $$ = ast_printf($3);
      }
    // scanf(&x);
    | SCANF_KW '(' '&' IDENT ')' ';'
      {
          $$ = ast_scanf($4);
      }
    // 그냥 expr; 도 허용 (마지막 값만 계산)
    | expr ';'
      {
          $$ = $1;
      }
    ;

expr
    : NUMBER
      { $$ = ast_int($1); }
    | IDENT
      { $$ = ast_var($1); }
    | expr '+' expr
      { $$ = ast_bin(AST_ADD, $1, $3); }
    | expr '-' expr
      { $$ = ast_bin(AST_SUB, $1, $3); }
    | expr '*' expr
      { $$ = ast_bin(AST_MUL, $1, $3); }
    | expr '/' expr
      { $$ = ast_bin(AST_DIV, $1, $3); }
    | '-' expr %prec UMINUS
      { $$ = ast_bin(AST_SUB, ast_int(0), $2); }
    | '(' expr ')'
      { $$ = $2; }
    ;

%%

void yyerror(const char* s)
{
    fprintf(stderr, "parse error: %s\n", s);
}
