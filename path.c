#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>

#define _DEFINE_PATH_GV_
#include "path.h"

void init_path(void)
{
	FILE *fp;
	char *row;
	int i, len;

	fp = fopen(PATH_FILE, "r");
	if (fp == NULL) {
		error(0, errno, "File cannot be opened.");
		exit(EXIT_FAILURE);
	}
	row = malloc(sizeof(char) * MAX_PATH_NUM * MAX_PATH_LENGTH);
	fgets(row, sizeof(char) * MAX_PATH_NUM * MAX_PATH_LENGTH, fp);
	paths[0] = row;
	path_size = 1;
	len = strlen(row);
	for (i = 0; i < len; i++) {
		if (row[i] == PATH_DELIMITER) {
			row[i] = '\0';
		}
		if (i > 0 && row[i-1] == '\0') {
			paths[path_size++] = &(row[i]);
		}
	}
}

void print_paths(void)
{
	int i;
	for (i = 0; i < path_size; i++) {
		printf("%s\n", paths[i]);
	}
}
