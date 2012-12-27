%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void yyerror(char *s);

char g_command[1024] = { '\0' };
int g_fdin, g_fdout, g_fderr;
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
			printf("Command: %s\n", g_command);
			printf("In: %d, Out: %d, Err: %d\n", g_fdin, g_fdout, g_fderr);
		}
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
		 WORD { strcat(g_command, " "); strcat(g_command, $1); }
		 | word_list WORD { strcat(g_command, " "); strcat(g_command, $2); }
		 ;

redirection:
		   '>' WORD
			{
				printf("Redirect to %s\n", $2);
				g_fdout = open($2, O_CREAT | O_RDWR | O_TRUNC);
			}
		   | '<' WORD
			{
				printf("Redirect from %s\n", $2);
				if (access($2, R_OK) == -1) {
					printf("Cannot read from file %s.\n", $2);
					exit(EXIT_FAILURE);
				}
				g_fdin = open($2, O_RDONLY);
			}
		   ;

redirection_list:
				redirection
				| redirection_list redirection
				;

%%

main() {
	g_fdin = STDIN_FILENO;
	g_fdout = STDOUT_FILENO;
	g_fderr = STDERR_FILENO;
	yyparse();
}

void yyerror(char *s) {
	printf("Parse Error: %s\n", s);
	exit(EXIT_FAILURE);
}
