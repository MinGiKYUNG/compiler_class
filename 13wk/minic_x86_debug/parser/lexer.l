%option noyywrap

%{
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"
%}

%%
[ \t\r\n]+              ;
"//".*                  ;

"int"                   { return INT; }
"return"                { return RETURN; }

[0-9]+                  { yylval.int_value = atoi(yytext); return NUMBER; }

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.ident = strdup(yytext); return IDENT; }

"{"                     { return '{'; }
"}"                     { return '}'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
","                     { return ','; }
"="                     { return '='; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }

.                       { fprintf(stderr, "Unknown character: %s\n", yytext); }

%%
