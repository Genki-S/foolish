#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <error.h>
#include <errno.h>

#include "common.h"
#include "command.h"
#include "path.h"

#define PIPE_FILE ".foolish_pipe"
#define PIPE_FILE_COPY ".foolish_pipe_copy"

extern struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(char *);

/*
 * To copy pipe output
 * From: http://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
 */
int cp(const char *to, const char *from)
{
	int fd_to, fd_from;
	char buf[4096];
	ssize_t nread;
	int saved_errno;

	fd_from = open(from, O_RDONLY);
	if (fd_from < 0)
		return -1;

	fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd_to < 0)
		goto out_error;

	while (nread = read(fd_from, buf, sizeof(buf)), nread > 0)
	{
		char *out_ptr = buf;
		ssize_t nwritten;

		do {
			nwritten = write(fd_to, out_ptr, nread);

			if (nwritten >= 0)
			{
				nread -= nwritten;
				out_ptr += nwritten;
			}
			else if (errno != EINTR)
			{
				goto out_error;
			}
		} while (nread > 0);
	}

	if (nread == 0)
	{
		if (close(fd_to) < 0)
		{
			fd_to = -1;
			goto out_error;
		}
		close(fd_from);

		/* Success! */
		return 0;
	}

out_error:
	saved_errno = errno;

	close(fd_from);
	if (fd_to >= 0)
		close(fd_to);

	errno = saved_errno;
	return -1;
}

int main(int argc, char const* argv[])
{
	int i;
	char *prompt = "% ";
	char *input = NULL;
	size_t len = 0;
	ssize_t read;

	init_path();

	/* Prompt -> read -> analyze -> execute loop */
	while (true) {
		printf("%s", prompt);
		read = getline(&input, &len, stdin);
		if (read == -1) { /* Ctrl-D */
			break;
		}
		if (strcmp(input, "exit\n") == 0) {
			msg("Wise command. Obviously, you should use zsh :)\n");
			break;
		}

		init_gv();
		dprt("Parse start: %s\n", input);
		yy_scan_string(input);
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

			pid_t cpid;
			if ((cpid = fork()) == -1) {
				fprintf(stderr, "fork error\n");
			}
			else if (cpid == 0) {
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

	/* Remove temporary file for pipe */
	remove(PIPE_FILE);
	remove(PIPE_FILE_COPY);

	/* Free memory */
	free(input);

	return EXIT_SUCCESS;
}
