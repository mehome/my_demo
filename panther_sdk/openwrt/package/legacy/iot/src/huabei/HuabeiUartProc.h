#ifndef __HUABEI_UART_PROC_H__
#define __HUABEI_UART_PROC_H__


#include "debug.h"

typedef int (*proc)(void *arg);

typedef struct HuabeiProc{
    int operate;
    proc exec;
}HuabeiProc;

typedef struct huabei_iot_msg {
	unsigned char type;
	unsigned char operate;
	unsigned char arg0;
	unsigned char arg1;
	unsigned char res0;
	unsigned char res1;
	unsigned int  len;
    char data[128];
}  huabei_iot_msg;
typedef struct huabei_iot_msg HuabeiIotMsg;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

HuabeiIotMsg *dupHuabeiIotMsg(void *arg);

#endif