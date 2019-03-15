#ifndef UTILS_H_
#define UTILS_H_
#include "stdio.h"

#define MAX_COMMAND_LEN	 1024
extern void my_eval(const char *fmt,...);
extern int my_system(const char *fmt,...);
extern int my_popen(const char *fmt,...);
extern int mpc_play(void);
extern int write_str_to_file(char *path,char *str,char *mode);
extern int read_str_from_file(char **p_str,char *path,char *mode);
extern int _evalpid(char *const argv[], char *path, int timeout, int *ppid);
extern int my_mpc_clear();
extern int my_curl_get(char **p_str,char* url);

int _evalpid(char *const argv[], char *path, int timeout, int *ppid);
#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})
#define eval_nowait(cmd, args...) ({ \
		char *argv[] = { cmd, ## args, NULL }; \
		int ppid = 0; \
		pid_t pid; \
		if ((pid = fork()) < 0) { \
			perror("fork"); \
			exit(0); \
		} \
		else if (pid > 0) { \
			_waitpid(pid, NULL, 0); \
		} \
		else { \
			_evalpid(argv, ">/dev/console", 0, &ppid); \
			exit(0); \
		} \
	})

#define FREE(x) if(x){free(x);x=NULL;}
#define MY_MIN(x, y) ({int _x=x;int _y=y;_x<_y?_x:_y;})

#if TIME_TRACE
	#define OUT_PATH "/tmp/out_path.txt"
	extern long int timer_interval;
	extern long int timer_last;
	extern long int delay_note;
	extern void out_val_to_file(char *desc,long int val, char *path);
	extern void out_tv_to_file(char *desc, char *path);
#endif

extern pthread_mutex_t popen_mtx;
void cleanup_curl_session();


#endif
