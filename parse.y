%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define COMMAND_STRLEN 1024
#define COMMAND_QUEUE_SIZE 10

void yyerror(char *s);

typedef enum {
	false, true
} bool;

char g_command[COMMAND_STRLEN] = { '\0' };
int g_fdin, g_fdout, g_fderr;
bool g_pipein, g_pipeout;

typedef struct {
	char str[COMMAND_STRLEN];
	int fdin, fdout, fderr;
	bool pipein, pipeout;
} command;

command* command_queue[COMMAND_QUEUE_SIZE];
int q_head, q_tail;

void register_command(char *str, int fdin, int fdout, int fderr, bool pipein, bool pipeout);
void register_command_gv();
void push_command(command *c);
command* pop_command();

void register_command(char *str, int fdin, int fdout, int fderr, bool pipein, bool pipeout)
{
	command* c = malloc(sizeof(command));
	strcpy(c->str, str);
	c->fdin = fdin;
	c->fdout = fdout;
	c->fderr = fderr;
	c->pipein = pipein;
	c->pipeout = pipeout;
	push_command(c);
}
void register_command_gv()
{
	register_command(g_command, g_fdin, g_fdout, g_fderr, g_pipein, g_pipeout);
}
void push_command(command *c)
{
	command_queue[q_tail] = c;
	q_tail == COMMAND_QUEUE_SIZE - 1? q_tail = 0 : ++q_tail;
	/* TODO: queue overflow */
}
command* pop_command()
{
	if (q_head == q_tail) {
		return NULL;
	}
	command* ret = command_queue[q_head];
	q_head == COMMAND_QUEUE_SIZE - 1? q_head = 0 : ++q_head;
	return ret;
}

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
	strcpy(g_command, "");
	g_fdin = STDIN_FILENO;
	g_fdout = STDOUT_FILENO;
	g_fderr = STDERR_FILENO;
	g_pipein = false;
	g_pipeout = false;
	yyparse();
	command* com;
	while ( (com = pop_command()) != NULL ) {
		printf("Command: %s\n", com->str);
		printf("In: %d, Out: %d, Err: %d\n", com->fdin, com->fdout, com->fderr);
		printf("PipeIn: %d, PipeOut: %d\n", com->pipein, com->pipeout);
		free(com);
	}
}

void yyerror(char *s) {
	printf("Parse Error: %s\n", s);
	exit(EXIT_FAILURE);
}
