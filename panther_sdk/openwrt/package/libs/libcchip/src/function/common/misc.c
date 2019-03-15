#include "misc.h"

char *get_name_cmdline(char *name,size_t size){
	pid_t *pPid=NULL;
    size_t count=get_pids_by_name(name,&pPid,1);
	if(!(count>0&&pPid)){
		err("get_pids_by_name %s failure!",name);
		return -2;
	}
	return get_pid_cmdline(pPid[0],size);
}
char *get_pid_cmdline(pid_t pid,size_t size){
	char *cmd=NULL;inf("");
	char path[]="/proc/65535/cmdline";inf("");
	memset(path,0,sizeof(path));inf("");
	snprintf(path,sizeof(path),"/proc/%d/cmdline",pid);inf("");
	inf(path);
	int err=fp_getc_file(&cmd,size,path);inf("");
	if(err<0){
		err("get %s failure,err=%d",path,err);
		return NULL;
	}
	int i=0;
	for(i=0;i<size;i++){
		if(!cmd[i]){
			cmd[i]=' ';	
		}
	}inf(cmd);
	return cmd;
}
size_t get_pids_by_name(const char *pidName,pid_t **pp_pid,size_t max){
	char filename[256];
	char name[128];
	struct dirent *next;
	FILE *file;
	DIR *dir;
	int fail = 1;
	pid_t *p_pid=NULL;
	dir = opendir("/proc");
	if (!dir) {
		return -1;
		perror("Cannot open /proc\n");
	}
	int count=0;
	for (count=0;(next = readdir(dir)) != NULL && count<max;) {
		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		memset(filename, 0, sizeof(filename));
		sprintf(filename, "/proc/%s/status", next->d_name);
		if (!(file = fopen(filename, "r")))
			continue;

		memset(filename, 0, sizeof(filename));
		if (fgets(filename, sizeof(filename) - 1, file) != NULL) {
			/* Buffer should contain a string like "Name:   binary_name" */
			sscanf(filename, "%*s %s", name);
			if (!strcmp(name, pidName)) {
				if(pp_pid){
					if ((p_pid = realloc(p_pid, sizeof(pid_t) * (count + 1))) == NULL){
						err("realloc fail!");
						fclose(file);
						goto exit;
					}
					p_pid[count] = strtol(next->d_name, NULL, 0);					
				}
				count++;
			}
		}
		fail=0;
		if(pp_pid) *pp_pid=p_pid;
		fclose(file);
	}
exit:	
	closedir(dir);
	if(fail && count<=0)return -1;	
	return count;
}
int pkill(const char *name, int sig){
	int ret=0;
	pid_t *p_pid=NULL;
	if(!name){
		err("name:%s is invalid!",name);
		return -1;
	}
    size_t count=get_pids_by_name(name,&p_pid,1);
	if(!(count>0&&p_pid)){
		err("get_pids_by_name %s failure!",name);
		return -2;
	}
	ret=kill(p_pid[0],sig);
	FREE(p_pid);
	if(ret<0)return -3;
    return 0;
}
static pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;
int my_popen(const char *fmt,...){
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
int my_popen_get(char *rbuf, int rbuflen, const char *cmd, ...){
	char buf[MAX_COMMAND_LEN];
	va_list args;
	FILE *pfile;
	int status = -2;
	char *p = rbuf;

	rbuflen = (!rbuf) ? 0 : rbuflen;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	pthread_mutex_lock(&popen_mtx);
	if ((pfile = popen(buf, "r"))) {
		fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
		while (!feof(pfile)) {
			if ((rbuflen > 0) && fgets(buf, MIN(rbuflen, sizeof(buf)), pfile)) {
				int len = snprintf(p, rbuflen, "%s", buf);
				rbuflen -= len;
				p += len;
			}
			else {
				break;
			}
		}
		if ((rbuf) && (p != rbuf) && (*(p - 1) == '\n')) {
			*(p - 1) = 0;
		}
		status = pclose(pfile);
	}
	pthread_mutex_unlock(&popen_mtx);
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	return -1;
}
void argv_free(char *argv[]){
	int i=0;
	for(i=0;argv[i];i++){
		free(argv[i]);
	}
	free(argv);
	argv=NULL;
}
char *argv_to_argl(char *argv[],char delim){
	int i=0,rt_len=0,rt_idx=0;
	char *argl=NULL;
	if(!(argv && argv[0]))return NULL;
	for(i=0;;i++){
		if(!argv[i])break;
		rt_len=strlen(argv[i]);
		rt_idx+=rt_len;				
		argl=(char*)realloc(argl,rt_idx+2);
		if(!argl)return NULL;
		strcat(argl,argv[i]);
		argl[rt_idx++]=delim;
	}
	argl[rt_idx-1]='\0';
	return argl;
}

char** argl_to_argv(char argl[],char delim){
	int i=0,count=0;
	if(!(argl && argl[0]))return NULL;
	char **argv=(char**)calloc(1,sizeof(char*));
	if(!argv)return NULL;
	argv[0]=argl;
	for(i=0;;i++){
		if(argl[i]==delim||argl[i]=='\0'||argl[i]=='\n'){
			count++;
			argv=(char**)realloc(argv,(count+1)*sizeof(char *));
			if(!argv)return NULL;
			argv[count]=argl+i+1;
			char *arg=calloc(1,argv[count]-argv[count-1]);
			if(!arg){
				argv_free(argv);
				return NULL;
			}
			strncpy(arg,argv[count-1],argv[count]-argv[count-1]-1);
			argv[count-1]=arg;
		}
		if(argl[i]=='\0'||argl[i]=='\n')break;
	}
	argv[count]=NULL;
	return argv;
}

int vfexec(char *argl,char isBlock){
	char **argv=argl_to_argv(argl,' ');
	if(!(argv && argv[0]))return -1;
	pid_t pid=vfork();
	if(pid<0)return -1;
	if(pid==0){
		int ret=execvp(argv[0],argv);
		argv_free(argv);
		if(ret<0){
			show_errno(0,"execv");
			return -1;
		}
	}else{
		int ws=0;
		int ret=isBlock?pid=waitpid(pid,&ws,0):waitpid(pid,&ws,WNOHANG);
		if(ret<0){
			show_errno(0,"waitpid");
			return -1;
		}
	}
	return 0;
}

int fexec(char *argl,char isBlock){
	char **argv=argl_to_argv(argl,' ');
	if(!(argv && argv[0]))return -1;
	pid_t pid=fork();
	if(pid<0)return -1;
	if(pid==0){
		int ret=execvp(argv[0],argv);
		argv_free(argv);
		if(ret<0){
			show_errno(0,"execv");
			return -1;
		}
	}else{
		int ws=0;
		int ret=isBlock?pid=waitpid(pid,&ws,0):waitpid(pid,&ws,WNOHANG);
		if(ret<0){
			show_errno(0,"waitpid");
			return -1;
		}
	}
	return 0;
}