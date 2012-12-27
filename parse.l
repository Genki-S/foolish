D [0-9]
L [a-zA-Z_]
S [.\-\\]

%{
#include "parse.tab.h"
%}

%%

({L}|{D}|{S})({L}|{D}|{S})* { printf("WORD: %s\n", yytext); yylval.wval = strdup(yytext); return WORD; }

"|" { printf("CHAR: %c\n", '|'); return '|'; }
">" { printf("CHAR: %c\n", '>'); return '>'; }
"<" { printf("CHAR: %c\n", '<'); return '<'; }
"\n" { printf("CHAR: %s\n", "\\n"); return '\n'; }

. { /* ignore other characters */ }

%%
