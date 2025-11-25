%option noyywrap

%{
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
%}

%%

[0-9]+          { yylval.ival = atoi(yytext); return NUMBER; }
[ \t\r]+        { /* skip whitespace */ }
\n              { /* ignore newline (세미콜론으로 문장 구분) */ }

"int"           { return INT_KW; }
"printf"        { return PRINTF_KW; }
"scanf"         { return SCANF_KW; }

[a-zA-Z_][a-zA-Z0-9_]* {
                    yylval.sval = strdup(yytext);
                    return IDENT;
                }

";"             { return ';'; }
"("             { return '('; }
")"             { return ')'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"="             { return '='; }
"&"             { return '&'; }

.               { return yytext[0]; }

%%
