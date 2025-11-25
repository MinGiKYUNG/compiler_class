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
    int ival;
    AST* node;
}

%token <ival> NUMBER
%type  <node> expr

%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%%

input:
      expr ';' {
          gen_x86_program($1, asm_out);
          ast_free($1);
      }
    ;

expr:
      NUMBER               { $$ = ast_int($1); }
    | expr '+' expr        { $$ = ast_bin(AST_ADD, $1, $3); }
    | expr '-' expr        { $$ = ast_bin(AST_SUB, $1, $3); }
    | expr '*' expr        { $$ = ast_bin(AST_MUL, $1, $3); }
    | expr '/' expr        { $$ = ast_bin(AST_DIV, $1, $3); }
    | '-' expr %prec UMINUS{ $$ = ast_bin(AST_SUB, ast_int(0), $2); }
    | '(' expr ')'         { $$ = $2; }
    ;

%%

void yyerror(const char* s)
{
    fprintf(stderr, "parse error: %s\n", s);
}
