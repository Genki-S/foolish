#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <error.h>
#include <errno.h>

#include "common.h"
#include "util.h"
#include "command.h"
#include "path.h"

#define PIPE_FILE ".foolish_pipe"
#define PIPE_FILE_COPY ".foolish_pipe_copy"

extern struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(char *);

static char *g_prompt = "% ";
static char *g_input_line = NULL; /* input from user */
static pid_t g_working_child_pid;

void terminate(void);
void trap(int sig);

/* Trap SIGINT */
void trap(int sig)
{
	if (g_working_child_pid == 0) { /* No child process */
		/* Terminate Foolish */
		terminate();
		exit(EXIT_FAILURE);
	}
	else {
		/* send SIGINT to child process (actions are up to programs) */
		kill(g_working_child_pid, SIGINT);
		printf("\n");
	}
}

int main(int argc, char const* argv[])
{
	int i;
	size_t len = 0;
	ssize_t read;

	/* Initialize */
	g_working_child_pid = 0;
	init_path();

	/* Set trap */
	signal(SIGINT, trap);

	/* Prompt -> read -> analyze -> execute loop */
	while (true) {
		printf("%s", g_prompt);
		read = getline(&g_input_line, &len, stdin);
		if (read == -1) { /* Ctrl-D */
			break;
		}
		if (strcmp(g_input_line, "exit\n") == 0) {
			msg("Wise command. Obviously, you should use zsh :)\n");
			break;
		}

		init_gv();
		dprt("Parse start: %s\n", g_input_line);
		yy_scan_string(g_input_line);
		yyparse();
		dprt("Parse end.\n");
		command* com;
		int save_stdin_fd = -1, save_stdout_fd = -1;

		/* Execute command (commands if pipe is used) */
		while ( (com = pop_command()) != NULL ) {
			dprt("Command: %s\n", com->bin);
			dprt("Args:\n");
			for (i = 0; i < com->argc; i++) {
				dprt("\t%s\n", com->argv[i]);
			}
			dprt("Infile: %s, Outfile: %s, Errfile: %s\n", com->infile, com->outfile, com->errfile);
			dprt("PipeIn: %d, PipeOut: %d\n", com->pipein, com->pipeout);

			if ((g_working_child_pid = fork()) == -1) {
				fprintf(stderr, "fork error\n");
			}
			else if (g_working_child_pid == 0) {
				/* child process */

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
				dprt("Found executable file: %s\n", bin);

				/* Error check about file descriptors */
				if (strcmp(com->infile, "") != 0 && com->pipein) {
					msg("Please do not redirect from file when a command has pipe input.\n");
					exit(EXIT_FAILURE);
				}
				if (strcmp(com->outfile, "") != 0 && com->pipeout) {
					msg("Please do not redirect to file when a command has pipe output. Use tee instead.\n");
					exit(EXIT_FAILURE);
				}

				/* Redirection settings */
				if (strcmp(com->infile, "") != 0) { /* infile is specified */
					if (access(com->infile, R_OK) != 0) {
						error(0, errno, "Cannot read from %s", com->infile);
						exit(EXIT_FAILURE);
					}
					save_stdin_fd = dup(STDIN_FILENO);
					close(STDIN_FILENO);
					open(com->infile, O_RDONLY); /* opens with STDIN_FILENO */
				}
				if (strcmp(com->outfile, "") != 0) {
					save_stdout_fd = dup(STDOUT_FILENO);
					close(STDOUT_FILENO);
					open(com->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* opens with STDOUT_FILENO */
				}

				/* Pipe settings */
				if (com->pipein) {
					save_stdin_fd = dup(STDIN_FILENO);
					close(STDIN_FILENO);
					/* Copy pipe file to avoid read/write at once */
					if (cp(PIPE_FILE_COPY, PIPE_FILE) != 0) {
						fprintf(stderr, "Cannot copy pipe file\n");
						exit(EXIT_FAILURE);
					}
					open(PIPE_FILE_COPY, O_RDONLY); /* opens with STDIN_FILENO */
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

			/* Clear child pid */
			g_working_child_pid = 0;

			/* Restore file descriptors */
			if (save_stdin_fd != -1) {
				close(STDIN_FILENO);
				dup2(save_stdin_fd, STDIN_FILENO);
			}
			if (save_stdout_fd != -1) {
				close(STDOUT_FILENO);
				dup2(save_stdout_fd, STDOUT_FILENO);
			}

			/* Free memory */
			for (i = 0; i < argc; i++) {
				free(com->argv[i]);
			}
			free(com->argv);
			free(com);

			/* Stop executing command queue if an error has occurred */
			if (WEXITSTATUS(status) == EXIT_FAILURE) {
				while ( (com = pop_command()) != NULL );
				break;
			}
		}
	}

	/* Termination */
	terminate();

	return EXIT_SUCCESS;
}

void terminate(void)
{
	/* Remove temporary file for pipe */
	remove(PIPE_FILE);
	remove(PIPE_FILE_COPY);

	/* Free memory */
	free(g_input_line);

	msg("Bye.\n");
}
