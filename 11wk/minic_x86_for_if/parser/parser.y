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
%token IF_KW
%token WHILE_KW
%token FOR_KW

%type <node> program stmt_list stmt block expr opt_expr opt_for_init for_step

/* ✨ 비교 연산자 < 을 우선순위에 추가 */
%left '<'
%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program
    : stmt_list
      {
          gen_x86_program($1, asm_out);
          ast_free($1);
      }
    ;

stmt_list
    : stmt
      { $$ = $1; }
    | stmt_list stmt
      { $$ = ast_stmt_list($1, $2); }
    ;

block
    : '{' stmt_list '}'
      { $$ = $2; }
    ;

stmt
    /* int x; */
    : INT_KW IDENT ';'
      { $$ = ast_var_decl($2, NULL); }

    /* int x = expr; */
    | INT_KW IDENT '=' expr ';'
      { $$ = ast_var_decl($2, $4); }

    /* x = expr; */
    | IDENT '=' expr ';'
      { $$ = ast_assign(ast_var($1), $3); }

    /* printf(expr); */
    | PRINTF_KW '(' expr ')' ';'
      { $$ = ast_printf($3); }

    /* scanf(&x); */
    | SCANF_KW '(' '&' IDENT ')' ';'
      { $$ = ast_scanf($4); }

    /* if (cond) stmt; */
    | IF_KW '(' expr ')' stmt
      { $$ = ast_if($3, $5); }

    /* while (cond) stmt; */
    | WHILE_KW '(' expr ')' stmt
      { $$ = ast_while($3, $5); }

    /* for (init; cond; step) stmt; */
    | FOR_KW '(' opt_for_init ';' opt_expr ';' for_step ')' stmt
      {
          AST* init = $3;
          AST* cond = $5 ? $5 : ast_int(1);  /* cond 비었으면 항상 true */
          AST* step = $7;
          AST* body = $9;

          /* while_body = { body; step; } 또는 body; */
          AST* while_body = body;
          if (step) {
              while_body = ast_stmt_list(body, step);
          }

          AST* while_node = ast_while(cond, while_body);

          if (init)
              $$ = ast_stmt_list(init, while_node);
          else
              $$ = while_node;
      }

    /* { ... } */
    | block
      { $$ = $1; }

    /* expr; */
    | expr ';'
      { $$ = $1; }
    ;

opt_for_init
    : /* empty */
      { $$ = NULL; }
    | IDENT '=' expr
      { $$ = ast_assign(ast_var($1), $3); }
    ;

opt_expr
    : /* empty */
      { $$ = NULL; }
    | expr
      { $$ = $1; }
    ;

for_step
    : /* empty */
      { $$ = NULL; }
    | IDENT '=' expr
      { $$ = ast_assign(ast_var($1), $3); }
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

    /* ✨ i < x 같은 비교 연산 */
    | expr '<' expr
      { $$ = ast_bin(AST_LT, $1, $3); }

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
