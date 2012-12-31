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
	strcpy(g_bin, "");
	g_argc = 1;
	g_argv = malloc(sizeof(char *) * COMMAND_MAX_ARGC);
	g_fdin = STDIN_FILENO;
	g_fdout = STDOUT_FILENO;
	g_fderr = STDERR_FILENO;
	g_pipein = false;
	g_pipeout = false;
}

int main(int argc, char const* argv[])
{
	int i;
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

		/* Execute command (commands if pipe is used) */
		while ( (com = pop_command()) != NULL ) {
			printf("Command: %s\n", com->bin);
			printf("Args:\n");
			for (i = 0; i < com->argc; i++) {
				printf("\t%s\n", com->argv[i]);
			}
			printf("In: %d, Out: %d, Err: %d\n", com->fdin, com->fdout, com->fderr);
			printf("PipeIn: %d, PipeOut: %d\n", com->pipein, com->pipeout);

			pid_t cpid;
			if ((cpid = fork()) == -1) {
				fprintf(stderr, "fork error\n");
			}
			else if (cpid == 0) {
				/* child process */
				printf("This is child.\n");
				/* Search bin */
				char bin[1024];
				for (i = 0; i < g_path_size; i++) {
					strcpy(bin, g_paths[i]);
					strcat(bin, "/");
					strcat(bin, com->bin);
					if (access(bin, X_OK) == 0) { /* executable command found */
						break;
					}
				}
				if (i == g_path_size) {
					fprintf(stderr, "Command not found: %s\n", com->bin);
					exit(EXIT_FAILURE);
				}
				printf("Execute file: %s\n", bin);
				execv(bin, com->argv);
			}
			/* parent */
			int status;
			if (wait(&status) == (pid_t)-1) {
				fprintf(stderr, "wait error\n");
				exit(EXIT_FAILURE);
			}

			for (i = 0; i < argc; i++) {
				free(com->argv[i]);
			}
			free(com->argv);
			free(com);
		}
	}
	/* Termination */
	free(input);
	return 0;
}
