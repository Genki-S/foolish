#ifndef _PATH_H_
#define _PATH_H_

#define PATH_FILE ".path"
#define PATH_DELIMITER ';'
#define MAX_PATH_NUM 64
#define MAX_PATH_LENGTH 64

/* GLOBAL VARIABLES */
#ifdef _DEFINE_PATH_GV_
	#define EXTERN /* nothing */
#else
	#define EXTERN extern
#endif
EXTERN int g_path_size;
EXTERN char* g_paths[MAX_PATH_NUM];

void init_path(void);
void print_paths(void);

#endif
