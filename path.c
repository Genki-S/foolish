#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#define _DEFINE_PATH_GV_
#include "path.h"

void init_path(void)
{
	int fd;
	char *row;
	int i, len;

	fd = open(PATH_FILE, O_RDONLY);
	if (fd == -1) {
		error(0, errno, "File cannot be opened.");
		exit(EXIT_FAILURE);
	}
	row = malloc(sizeof(char) * MAX_PATH_NUM * MAX_PATH_LENGTH);
	read(fd, row, sizeof(char) * MAX_PATH_NUM * MAX_PATH_LENGTH);
	g_paths[0] = row;
	g_path_size = 1;
	len = strlen(row);
	for (i = 0; i < len; i++) {
		if (row[i] == PATH_DELIMITER) {
			row[i] = '\0';
		}
		if (i > 0 && row[i-1] == '\0') {
			g_paths[g_path_size++] = &(row[i]);
		}
	}
}

void print_paths(void)
{
	int i;
	for (i = 0; i < g_path_size; i++) {
		printf("%s\n", g_paths[i]);
	}
}
