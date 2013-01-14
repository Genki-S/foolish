#include <string.h>
#include <fcntl.h>

#include "common.h"
#include "command.h"
#include "blankcmd.h"

static char blankcmd_str[BLANKCMD_MAX_LEN];

void init_blankcmd(void)
{
	int fd = open(BLANKCMD_FILE, O_RDONLY);
	if (fd == -1) {
		strcpy(blankcmd_str, "\n");
		return;
	}
	read(fd, blankcmd_str, COMMAND_BIN_MAX_LEN);
}

void blankcmd(char* g_input_line)
{
	strcpy(g_input_line, blankcmd_str);
}
