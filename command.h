#ifndef _COMMAND_H_
#define _COMMAND_H_

#define COMMAND_BIN_MAX_LEN 128
#define COMMAND_MAX_ARGC 32
#define COMMAND_QUEUE_SIZE 10
#define FILE_MAX_LEN 64

/* GLOBAL VARIABLES */
#ifdef _DEFINE_COMMAND_GV_
	#define EXTERN /* nothing */
#else
	#define EXTERN extern
#endif
EXTERN char g_bin[COMMAND_BIN_MAX_LEN];
EXTERN int g_argc;
EXTERN char **g_argv;
EXTERN char g_infile[FILE_MAX_LEN], g_outfile[FILE_MAX_LEN], g_errfile[FILE_MAX_LEN];
EXTERN bool g_pipein, g_pipeout;

typedef struct {
	char bin[COMMAND_BIN_MAX_LEN];
	int argc;
	char **argv;
	char infile[FILE_MAX_LEN], outfile[FILE_MAX_LEN], errfile[FILE_MAX_LEN];
	bool pipein, pipeout;
} command;

void init_gv(void);
void register_command(char *bin, int argc, char **argv, char *infile, char *outfile, char *errfile, bool pipein, bool pipeout);
void register_command_gv(void);
void push_command(command *c);
command* pop_command(void);

#endif
