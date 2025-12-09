%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
%}

%union {
    int int_value;
    char *ident;
    Expr *expr;
    ExprList *expr_list;
    Stmt *stmt;
    StmtList *stmt_list;
    ParamList *param_list;
    Function *function;
    FunctionList *function_list;
}

%token INT RETURN
%token <int_value> NUMBER
%token <ident> IDENT

%type <expr> expr primary
%type <expr_list> arg_list arg_list_opt
%type <stmt> stmt vardecl assign
%type <stmt_list> stmt_list stmt_list_opt compound_stmt
%type <param_list> param_list param_list_opt
%type <function> function
%type <function_list> func_list program

%left '+' '-'
%left '*' '/'

%%

program
    : func_list                { g_program = $1; }
    ;

func_list
    : func_list function       { $$ = function_list_append($1, $2); }
    | function                 { $$ = function_list_append(NULL, $1); }
    ;

function
    : INT IDENT '(' param_list_opt ')' compound_stmt
                              { $$ = new_function($2, $4, $6); }
    ;

param_list_opt
    : /* empty */              { $$ = NULL; }
    | param_list               { $$ = $1; }
    ;

/* C 스타일: int a, int b, ... */
param_list
    : INT IDENT                    { $$ = param_list_append(NULL, $2); }
    | param_list ',' INT IDENT     { $$ = param_list_append($1, $4); }
    ;

compound_stmt
    : '{' stmt_list_opt '}'    { $$ = $2; }
    ;

stmt_list_opt
    : /* empty */              { $$ = NULL; }
    | stmt_list                { $$ = $1; }
    ;

stmt_list
    : stmt_list stmt           { $$ = stmt_list_append($1, $2); }
    | stmt                     { $$ = stmt_list_append(NULL, $1); }
    ;

stmt
    : vardecl ';'              { $$ = $1; }
    | assign ';'               { $$ = $1; }
    | RETURN expr ';'          { $$ = new_return_stmt($2); }
    | expr ';'                 { $$ = new_expr_stmt($1); }
    ;

vardecl
    : INT IDENT                { $$ = new_vardecl_stmt($2); }
    ;

assign
    : IDENT '=' expr           { $$ = new_assign_stmt($1, $3); }
    ;

expr
    : expr '+' expr            { $$ = new_binop_expr(BIN_ADD, $1, $3); }
    | expr '-' expr            { $$ = new_binop_expr(BIN_SUB, $1, $3); }
    | expr '*' expr            { $$ = new_binop_expr(BIN_MUL, $1, $3); }
    | expr '/' expr            { $$ = new_binop_expr(BIN_DIV, $1, $3); }
    | primary                  { $$ = $1; }
    ;

primary
    : NUMBER                   { $$ = new_int_expr($1); }
    | IDENT                    { $$ = new_var_expr($1); }
    | IDENT '(' arg_list_opt ')' { $$ = new_call_expr($1, $3); }
    | '(' expr ')'             { $$ = $2; }
    ;

arg_list_opt
    : /* empty */              { $$ = NULL; }
    | arg_list                 { $$ = $1; }
    ;

arg_list
    : expr                     { $$ = expr_list_append(NULL, $1); }
    | arg_list ',' expr        { $$ = expr_list_append($1, $3); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "parse error: %s\n", s);
}
