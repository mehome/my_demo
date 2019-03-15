#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <mpd/connection.h>
#include <mpd/status.h>
// #include <curl/curl.h>
//#include "slog.h"
#include "utils.h"
//#include "flags.h"
//#include "search_and_play.h"

pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;
#if 0
int _evalpid(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	switch (pid = fork()) {
	case -1:        /* error */
		perror("fork");
		return errno;
	case 0:         /* child */
		/*
		 * Reset signal handlers set for parent process
		 */
		for (sig = 0; sig < (_NSIG - 1); sig++)
			signal(sig, SIG_DFL);

		/*
		 * Clean up
		 */
		ioctl(0, TIOCNOTTY, 0);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		setsid();

		/*
		 * We want to check the board if exist UART? , add by honor
		 * 2003-12-04
		 */
		if ((fd = open("/dev/console", O_RDWR)) < 0) {
			(void)open("/dev/null", O_RDONLY);
			(void)open("/dev/null", O_WRONLY);
			(void)open("/dev/null", O_WRONLY);
		}
		else {
			close(fd);
			(void)open("/dev/console", O_RDONLY);
			(void)open("/dev/console", O_WRONLY);
			(void)open("/dev/console", O_WRONLY);
		}

		/*
		 * Redirect stdout to <path>
		 */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/*
				 * append to <path>
				 */
				flags |= O_APPEND;
				path += 2;
			}
			else if (!strncmp(path, ">", 1)) {
				/*
				 * overwrite <path>
				 */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}

		/*
		 * execute command
		 */
		// for (i = 0; argv[i]; i++)
		// snprintf (buf + strlen (buf), sizeof (buf), "%s ", argv[i]);
		// cprintf("cmd=[%s]\n", buf);
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:		/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		}
		else {
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}
#endif
int my_popen(const char *fmt,...)
{
    char buf[MAX_COMMAND_LEN]="";
    char cmd[MAX_COMMAND_LEN]="";
    FILE *pfile;
    int status = -2;
    pthread_mutex_lock(&popen_mtx);
	va_list args;    
	va_start(args,fmt);
	vsprintf(cmd,fmt,args);
	va_end(args);
//  	inf("%s",cmd);	
    if ((pfile = popen(cmd, "r"))) {
        fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
        while(!feof(pfile)) {
            fgets(buf, sizeof buf, pfile);
        }
//		inf("%s",buf);
        status = pclose(pfile);
    }
    pthread_mutex_unlock(&popen_mtx);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}
int write_str_to_file(char *path,char *str,char *mode){
	int fail=1,ret=0,len=0;
	FILE *fd=NULL;
	if(!str){
		err("str==NULL");
		goto Err0;
	}
	if(!(fd=fopen(path,mode))){
		err("fopen %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err0;
	}
	if((len=strlen(str))<1){
		err("str content is empty!");
		goto Err1;
	}
	if((ret=fwrite(str,sizeof(char),len,fd))<len){
		err("fwrite %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err1;		
	}
	fail=0;
Err1:
	fclose(fd);
Err0:
	if(fail)return -1;
	return 0;
}


int read_str_from_file(char **p_str,char *path,char *mode)
{
	FILE *fd=NULL;
	int ret=0,fail=1;
	char *str=NULL;
	struct stat f_stat={0};
	if(!(fd=fopen(path,mode))){
		err("fopen %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err0;
	}
	if((ret=stat(path,&f_stat))<0){
		err("stat %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err1;
	}
	if(!(str=(char*)calloc(1,f_stat.st_size+1))){
		err("");
		goto Err1;
	}
	if((ret=fread(str,sizeof(char),f_stat.st_size,fd))<f_stat.st_size){
		err("fread %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err2;		
	}
	if(!str){
		err("");
		goto Err2;
	}
	*p_str=str;
	fail=0;
Err2:
	if(fail)FREE(str);
Err1:
	fclose(fd);
Err0:
	if(fail)return -1;
	return 0;
}

/*
int my_mpc_clear()
{
	if(my_popen("mpc clear && rm -rf %s %s",CURRENT_PLAY_PATH,CURRENT_M3U_PATH)<0){
		return -1;
	}
	return 0;
}
int my_mpc_resume(){
	if(my_popen("cp -f %s %s",	TMP_CURRENT_PLAY_PATH,CURRENT_PLAY_PATH)<0){
		return -1;
	}
	if(my_popen("mpc clear && rm -rf %s %s",CURRENT_PLAY_PATH,CURRENT_M3U_PATH)<0){
		return -1;
	}
	return 0;
}

typedef struct CURL_CTRL_T{
	char **p_data;
	unsigned int total_len;
	unsigned int rt_len;
	unsigned int idx;
}curl_ctrl_t;


static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *any) {
	curl_ctrl_t *p_ctrl=(curl_ctrl_t *)any;
    p_ctrl->rt_len = size*nmemb;
	if((!p_ctrl->rt_len)){
		return 0;
	}
	p_ctrl->idx=p_ctrl->total_len;
	p_ctrl->total_len+=p_ctrl->rt_len;
	*(p_ctrl->p_data)=(char *)realloc(*(p_ctrl->p_data),p_ctrl->total_len);
	if(NULL==*(p_ctrl->p_data)){
		err("realloc failure!");
		return 0;
	}	
	memcpy((*(p_ctrl->p_data))+p_ctrl->idx,ptr,p_ctrl->rt_len);
	return p_ctrl->rt_len;
}
static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{    
	if(check_u2i_stop_msg()<0){
		war("u2i stop proc!");
		return -1;
	}
    return 0;
}
int my_curl_get(char **p_str,char* url)
{
	CURL *curl;
	CURLcode res;	
	int fail=1,ret=0;
	curl_ctrl_t curl_ctrl={p_str,0,0,0};
	if(!(curl=curl_easy_init())){
		goto Err0;
	}
	curl_easy_setopt(curl, CURLOPT_URL				, url);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT			, 10);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS		, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION	, progress_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION	, write_data_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA		, &curl_ctrl);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	//curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10l);
	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
 	//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
	//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
    if(CURLE_OK != res) {
		err("");
		goto Err0;
    }	
	fail=0;
Err0:
	if(fail)return -1;
	return 0;
}

#if TIME_TRACE
long int timer_interval=0;
long int timer_last=0;
long int delay_note=0;

long int get_curret_time(void){
	struct timeval tv={0};
	//struct timezone tz={0};
	if(gettimeofday(&tv,NULL)<0){
		err("gettimeofday fail\n");
		return -1;
	}
	return (tv.tv_usec+tv.tv_sec*1000000)/1000;
}
void out_val_to_file(char *desc,long int val, char *path){
	char buf[512]="";
	snprintf(buf,sizeof(buf),"echo '%s:%ld' >> %s",desc,val,path);
	if(system(buf)<0){
		err("system(buf) excuse fail\n");
	}
}
void out_tv_to_file(char *desc, char *path){
	char buf[512]="";
	long int tv=get_curret_time()-timer_last;
	timer_last=get_curret_time();
	snprintf(buf,sizeof(buf),"echo '%s:%ld' >> %s",desc,tv,path);
	if(system(buf)<0){
		err("system(buf) excuse fail\n");
	}
}
#endif


*/
