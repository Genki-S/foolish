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
#include "command.h"
#include "path.h"
#include "blankcmd.h"

/* Pipe syntax suger */
enum PIPE { READ, WRITE };

/* Externals (to use bison functionality) */
extern struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(char *);

/* Static globals */
static char *g_prompt = "% ";
static char *g_input_line = NULL; /* input from user */
static pid_t g_working_child_pid;

/* Prototype declarations */
void init(void);
void terminate(void);
void trap(int sig);

/* Main function does basic loop */
int main(int argc, char const* argv[])
{
	int i;
	size_t len = 0;
	ssize_t n;
	int pipe_p2c[2], pipe_c2p[2]; /* interaction between patent and child */
	int prevcom_pipe_output_fd = 100; /* dup pipe output for later use (use big integer to organize fds) */

	/* Initialize */
	init();

	/* Prompt -> read -> analyze -> execute loop */
	while (true) {
		printf("%s", g_prompt);
		n = getline(&g_input_line, &len, stdin);

		/* Exit check */
		if (n == -1) { /* Ctrl-D */
			break;
		}
		if (strcmp(g_input_line, "exit\n") == 0) {
			msg("Wise command. Obviously, you should use zsh :)\n");
			break;
		}

		/* Blank command */
		if (strcmp(g_input_line, "\n") == 0) {
			blankcmd(g_input_line);
		}

		/* Parse input */
		init_parser_gv();
		yy_scan_string(g_input_line);
		yyparse();

		/* Execute command (commands if pipes are used) */
		command* com;
		while ( (com = pop_command()) != NULL ) {
			/* Prepare pipes */
			if (pipe(pipe_c2p) < 0) {
				error(0, errno, "Pipe"); exit(EXIT_FAILURE);
			}
			if (pipe(pipe_p2c) < 0) {
				error(0, errno, "Pipe"); exit(EXIT_FAILURE);
			}

			/* Fork and execute */
			if ((g_working_child_pid = fork()) == -1) {
				fprintf(stderr, "fork error\n");
			}
			else if (g_working_child_pid == 0) {
				/* child process */
				close(pipe_p2c[WRITE]);
				close(pipe_c2p[READ]);

				/* Search bin */
				char bin[1024];
				if (com->bin[0] == '/') { /* Absolute path */
					strcpy(bin, com->bin);
					if (access(bin, X_OK) != 0) {
						fprintf(stderr, "Command not found: %s\n", com->bin);
						exit(EXIT_FAILURE);
					}
				}
				else { /* Search path for executable */
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
				}

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
					close(STDIN_FILENO);
					open(com->infile, O_RDONLY); /* opens with STDIN_FILENO */
				}
				if (strcmp(com->outfile, "") != 0) {
					close(STDOUT_FILENO);
					open(com->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* opens with STDOUT_FILENO */
				}

				/* Pipe settings */
				if (com->pipein) {
					dup2(pipe_p2c[READ], STDIN_FILENO);
				}
				if (com->pipeout) {
					dup2(pipe_c2p[WRITE], STDOUT_FILENO);
				}
				close(pipe_p2c[READ]);
				close(pipe_c2p[WRITE]);
				execv(bin, com->argv);
			}

			/* parent */
			close(pipe_p2c[READ]);
			close(pipe_c2p[WRITE]);

			/* Write previous pipe output if necessary */
			char buf[BUFSIZ];
			if (com->pipein) {
				while ((n = read(prevcom_pipe_output_fd, buf, BUFSIZ)) > 0) {
					write(pipe_p2c[WRITE], buf, n);
				}
			}
			close(pipe_p2c[WRITE]);

			int status;
			if (wait(&status) == (pid_t)-1) {
				fprintf(stderr, "wait error\n");
				exit(EXIT_FAILURE);
			}

			/* Keep pipe output if necessary */
			if (com->pipeout) {
				dup2(pipe_c2p[READ], prevcom_pipe_output_fd);
			}
			close(pipe_c2p[READ]);


			/* CLEAN UP */

			/* Clear child pid */
			g_working_child_pid = 0;

			/* Free memory */
			for (i = 0; i < com->argc; i++) {
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

void init(void)
{
	/* Welcome Message */
	printf("FOOLISH: Foolish Obtuse OS Learner's Incompetent SHell.\n");
	printf("Copyright 2012 Genki Sugimoto.\n");
	printf("This is foolish software with ABSOLUTELY NO WARRANTY.\n");
	/* Init path */
	init_path();
	/* Init blank command */
	init_blankcmd();
	/* child pid: 0 means no child is running */
	g_working_child_pid = 0;
	/* Set trap */
	signal(SIGINT, trap);
}

void terminate(void)
{
	/* Free memory */
	free(g_input_line);

	msg("Bye.\n");
}

void trap(int sig)
{
	if (g_working_child_pid == 0) { /* No child process */
		/* Terminate Foolish */
		terminate();
		exit(EXIT_FAILURE);
	}
	else {
		/* send signal to child process (actions are up to programs) */
		kill(g_working_child_pid, sig);
		printf("\n");
	}
}
