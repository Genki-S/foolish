#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _DEFINE_COMMAND_GV_
#include "common.h"
#include "command.h"

command* command_queue[COMMAND_QUEUE_SIZE];
int q_head, q_tail;

void init_gv()
{
	strcpy(g_bin, "");
	g_argc = 1;
	g_argv = malloc(sizeof(char *) * COMMAND_MAX_ARGC);
	strcpy(g_infile, "");
	strcpy(g_outfile, "");
	strcpy(g_errfile, "");
	g_pipein = false;
	g_pipeout = false;
}

void register_command(char *bin, int argc, char **argv, char *infile, char *outfile, char *errfile, bool pipein, bool pipeout)
{
	command* c = malloc(sizeof(command));
	strcpy(c->bin, bin);
	c->argc = argc;
	c->argv = argv;
	c->argv[argc] = NULL;
	strcpy(c->infile, infile);
	strcpy(c->outfile, outfile);
	strcpy(c->errfile, errfile);
	c->pipein = pipein;
	c->pipeout = pipeout;
	push_command(c);
}
void register_command_gv(void)
{
	register_command(g_bin, g_argc, g_argv, g_infile, g_outfile, g_errfile, g_pipein, g_pipeout);
}
void push_command(command *c)
{
	command_queue[q_tail] = c;
	q_tail == COMMAND_QUEUE_SIZE - 1? q_tail = 0 : ++q_tail;
	/* TODO: queue overflow */
}
command* pop_command(void)
{
	if (q_head == q_tail) {
		return NULL;
	}
	command* ret = command_queue[q_head];
	q_head == COMMAND_QUEUE_SIZE - 1? q_head = 0 : ++q_head;
	return ret;
}
