#ifndef _FIFO_CMD_H
#define _FIFO_CMD_H
#include <stdlib.h>
#include "../common/un_fifo.h"
#include "../common/misc.h"
#include "../log/slog.h"

#define FIFO_CMD_DELIM 	'/'

typedef struct item_arg_t{
	char ** argv;
	void * args;
}item_arg_t;

typedef struct fifo_cmd_t{
	char *cmd;
	int (*handle)(void *);
	void *args;
}fifo_cmd_t;

typedef struct cmd_args_t{
	char* path;
	void* tbl;
	size_t count;	
}cmd_args_t;

#define ADD_FIFO_CMD_ITEM(x) {#x,x##_handle},

void *fifo_cmd_thread_routine(void *pCmdArgs);
int fifo_cmd_thread_create(cmd_args_t *pCmdArgs);
#endif