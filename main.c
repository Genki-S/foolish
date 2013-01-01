#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "common.h"
#include "command.h"
#include "path.h"

#define PIPE_FILE ".foolish_pipe"

extern struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(char *);

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
		int save_stdin_fd = -1, save_stdout_fd = -1;

		/* Execute command (commands if pipe is used) */
		while ( (com = pop_command()) != NULL ) {
			printf("Command: %s\n", com->bin);
			printf("Args:\n");
			for (i = 0; i < com->argc; i++) {
				printf("\t%s\n", com->argv[i]);
			}
			printf("Infile: %s, Outfile: %s, Errfile: %s\n", com->infile, com->outfile, com->errfile);
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
				printf("Found executable file: %s\n", bin);

				/* Pipe settings */
				if (com->pipein) {
					save_stdin_fd = dup(STDIN_FILENO);
					close(STDIN_FILENO);
					open(PIPE_FILE, O_RDONLY); /* opens with STDIN_FILENO */
				}
				if (com->pipeout) {
					save_stdout_fd = dup(STDOUT_FILENO);
					close(STDOUT_FILENO);
					open(PIPE_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* opens with STDOUT_FILENO */
				}
				execv(bin, com->argv);

			}
			/* parent */
			int status;
			if (wait(&status) == (pid_t)-1) {
				fprintf(stderr, "wait error\n");
				exit(EXIT_FAILURE);
			}

			/* CLEAN UP */

			/* Restore file descriptors */
			if (save_stdin_fd != -1) {
				close(STDIN_FILENO);
				dup2(save_stdin_fd, STDIN_FILENO);
			}
			if (save_stdout_fd != -1) {
				close(STDOUT_FILENO);
				dup2(save_stdout_fd, STDOUT_FILENO);
			}
			/* Remove temporary file for pipe */
			if (com->pipein) {
				if (remove(PIPE_FILE) == -1) {
					error(0, errno, "Can't remove pipefile: %s\n", PIPE_FILE);
					exit(EXIT_FAILURE);
				}
			}

			/* Free memory */
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
