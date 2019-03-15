#ifndef __MISC_H__
#define __MISC_H__ 1

#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include "fp_op.h"

#define MAX_COMMAND_LEN	 2048

#define FREE(x) if(x){free(x);x=NULL;}

#define MIN(x,y) (x<y?x:y)

#define getCount(array) (sizeof(array)/sizeof(array[0]))

#define ADD_HANDLE_ITEM(name,args) {#name,name##_handle,args},

#define assert_arg(count,errCode){\
	int err=0;\
	if(!((argv[count]&&argv[count][0]))){\
		err("arg is can't less %d",count);\
		return errCode;\
	}\
}
extern int vfexec(char *argl,char block);
extern int fexec(char *argl,char block);
extern int my_popen(const char *fmt,...);
extern int my_popen_get(char *rbuf, int rbuflen, const char *cmd, ...);
extern int pkill(const char *name, int sig);
extern size_t get_pids_by_name(const char *pidName,pid_t **pp_pid,size_t max);
char** argl_to_argv(char argl[],char delim);
char *argv_to_argl(char *argv[],char delim);
char *get_name_cmdline(char *name,size_t size);
char *get_pid_cmdline(pid_t pid,size_t size);
void show_process_thread_id(char *);

#define showProcessThreadId(msg) ({\
	plog("/tmp/tid.rec","%s:%s,pid=%u,tid=%u",\
		msg,__func__,\
		getpid(),\
		syscall(SYS_gettid)\
	);\
})

#define showCmdResult(showFunc,cmd...) ({\
	char rBuf[1024]="";\
	my_popen_get(rBuf,sizeof(rBuf),cmd);\
	showFunc(rBuf);\
})

#define showMemFree(showFunc) showCmdResult(showFunc,"cat /proc/meminfo | grep MemFree")

#endif /* __PROCUTILS_H__ */
