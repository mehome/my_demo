#ifndef CPP_BOX_H_
#define CPP_BOX_H_
extern "C" {
	#include <libcchip/platform.h>
	#include <libcchip/function/back_trace/back_trace.h>	
}
typedef struct handle_item_t{
	const char *name;
	int (*handle)(char *argv[]);
	void *args;
}handle_item_t;

#endif