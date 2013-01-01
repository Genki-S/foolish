%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "command.h"

void yyerror(char *s);

%}

%union {
	char cval;
	char *wval;
}

%token <cval> CONTROL
%token <wval> WORD

%%

input:
	 single_command command_terminator
		{
			register_command_gv();
		}
	 | single_command { g_pipeout = true; register_command_gv(); init_gv(); g_pipein = true; } '|' input
	 | '\n'
	 ;

command_terminator:
				  '\n'
				  ;

single_command:
	   word_list
	   | word_list redirection_list
	   ;

word_list:
		 WORD {
			strcpy(g_bin, $1);
			g_argv[0] = malloc(sizeof(char) * (strlen($1) + 1));
			strcpy(g_argv[0], $1); }
		 | word_list WORD {
			g_argv[g_argc] = malloc(sizeof(char) * (strlen($2) + 1));
			strcpy(g_argv[g_argc], $2);
			++g_argc; }
		 ;

redirection:
		   '>' WORD
			{
				strcpy(g_outfile, $2);
			}
		   | '<' WORD
			{
				strcpy(g_infile, $2);
			}
		   ;

redirection_list:
				redirection
				| redirection_list redirection
				;


%%

void yyerror(char *s) {
	printf("Parse Error: %s\n", s);
	exit(EXIT_FAILURE);
}
