#ifndef _COMMON_H_
#define _COMMON_H_

/* Debug stuffs */
// #define DEBUG_ON
#ifdef DEBUG_ON
	#define dprt(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
	#define dprt(fmt, ...)
#endif

/* Message from Foolish */
#define msg(fmt, ...) { printf("Foolish: "); printf(fmt, ##__VA_ARGS__); }

typedef enum {
	false, true
} bool;

#endif
