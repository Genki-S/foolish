#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "command.h"
#include "path.h"

extern struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(char *);

void init_gv()
{
	strcpy(g_command, "");
	g_fdin = STDIN_FILENO;
	g_fdout = STDOUT_FILENO;
	g_fderr = STDERR_FILENO;
	g_pipein = false;
	g_pipeout = false;
}

int main(int argc, char const* argv[])
{
	char *prompt = "% ";
	char *input = NULL;
	size_t len = 0;
	ssize_t read;

	init_path();

	while (true) {
		printf("%s", prompt);
		read = getline(&input, &len, stdin);
		if (read == -1) { /* Ctrl-D */
			break;
		}
		init_gv();
		printf("Parse start: %s\n", input);
		yy_scan_string(input);
		yyparse();
		printf("Parse end.\n");
		command* com;
		while ( (com = pop_command()) != NULL ) {
			printf("Command: %s\n", com->str);
			printf("In: %d, Out: %d, Err: %d\n", com->fdin, com->fdout, com->fderr);
			printf("PipeIn: %d, PipeOut: %d\n", com->pipein, com->pipeout);
			free(com);
		}
	}
	/* Termination */
	free(input);
	return 0;
}
