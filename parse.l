D [0-9]
L [a-zA-Z_]
S [./'"\-\\]

%{
#include "parse.tab.h"
%}

%%

({L}|{D}|{S})({L}|{D}|{S})* { yylval.wval = strdup(yytext); return WORD; }

"|" { return '|'; }
">" { return '>'; }
"<" { return '<'; }
"\n" { return '\n'; }

. { /* ignore other characters */ }

%%
