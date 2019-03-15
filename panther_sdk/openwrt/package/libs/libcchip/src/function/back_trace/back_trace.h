#ifndef _BACK_TRACE_H
#define _BACK_TRACE_H

#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <memory.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/ucontext.h>
#include <dlfcn.h>

void trace_signal(int tbl[]);
#define traceSig(x...)	{int sigTbl[]={x,-1};trace_signal(sigTbl);}

#endif