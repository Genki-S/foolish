#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _DEFINE_COMMAND_GV_
#include "common.h"
#include "command.h"

command* command_queue[COMMAND_QUEUE_SIZE];
int q_head, q_tail;

void register_command(char *bin, int argc, char **argv, int fdin, int fdout, int fderr, bool pipein, bool pipeout)
{
	command* c = malloc(sizeof(command));
	strcpy(c->bin, bin);
	c->argc = argc;
	c->argv = argv;
	c->fdin = fdin;
	c->fdout = fdout;
	c->fderr = fderr;
	c->pipein = pipein;
	c->pipeout = pipeout;
	push_command(c);
}
void register_command_gv(void)
{
	register_command(g_bin, g_argc, g_argv, g_fdin, g_fdout, g_fderr, g_pipein, g_pipeout);
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
