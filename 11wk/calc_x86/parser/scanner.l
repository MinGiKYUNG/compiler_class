%option noyywrap

%{
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"
%}

%%

[0-9]+      { yylval.ival = atoi(yytext); return NUMBER; }
[ \t\r]+    { /* skip whitespace */ }
\n          { /* ignore newline */ }

";"         { return ';'; }
"("         { return '('; }
")"         { return ')'; }
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }

.           { return yytext[0]; }

%%
