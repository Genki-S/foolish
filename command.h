#ifndef _COMMAND_H_
#define _COMMAND_H_

#define COMMAND_STRLEN 1024
#define COMMAND_QUEUE_SIZE 10

/* GLOBAL VARIABLES */
#ifdef _DEFINE_COMMAND_GV_
	#define EXTERN /* nothing */
#else
	#define EXTERN extern
#endif
EXTERN char g_command[COMMAND_STRLEN];
EXTERN int g_fdin, g_fdout, g_fderr;
EXTERN bool g_pipein, g_pipeout;

typedef struct {
	char str[COMMAND_STRLEN];
	int fdin, fdout, fderr;
	bool pipein, pipeout;
} command;

void init_gv(void);
void register_command(char *str, int fdin, int fdout, int fderr, bool pipein, bool pipeout);
void register_command_gv(void);
void push_command(command *c);
command* pop_command(void);

#endif
