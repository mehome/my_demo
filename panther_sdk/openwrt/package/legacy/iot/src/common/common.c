
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <sys/socket.h>  
#include <netinet/if_ether.h>  
#include <net/if.h>  
#include <net/if.h>  
#include <sys/types.h>


#include "myutils/debug.h"
#define MAX_COMMAND_LEN		1024

static pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;

int timeDebug = 0;
void PrintSysTime(char *pSrc)
{

	char *tmp[100]={0};
	int showlog = 0;
	struct tm *tme=NULL;
	struct timeval start;

	if(timeDebug) {
	 	gettimeofday(&start, NULL );
		printf("\033[1;31;40m [%s]---sys_time:%d.%f \033[0m\n", pSrc, start.tv_sec, start.tv_usec/1000.0);

		//time_t tm;
		//tm = time(NULL);
		//tme = localtime(&tm);
		//strftime(tmp, sizeof(tmp), "%Y-%m-%d,%H:%M:%S:%u",tme);
		//printf("\033[1;31;40m [%s] \033[0m\n", tmp);
	}
}
#if 1
void mkdirs(char *muldir) 
{
    int i,len;
    char str[512];    
    strncpy(str, muldir, 512);
    
    len=strlen(str);
    
    for( i=0; i<len; i++ )
    {
        if( str[i]=='/' )
        {
            str[i] = '\0';
            if( access(str,0)!=0 )
            {
                mkdir( str, 0777 );
            }
            str[i]='/';
        }
    }
    if( len>0 && access(str,0)!=0 )
    {
        mkdir( str, 0777 );
    }
    return;
}

int create_path(const char* path)  
{  
    int beginCmpPath;  
    int endCmpPath;  
    int fullPathLen;  
    int pathLen = strlen(path);  
    char currentPath[1024] = {0};  
    char fullPath[1024] = {0};  
  
    info("path = %s", path);  
	//path = /media/mmcblk0p1/收藏
	
    //相对路径  
    if('/' != path[0])  
    {     
        //获取当前路径  
        getcwd(currentPath, sizeof(currentPath));  
        strcat(currentPath, "/");  
        info("currentPath = %s", currentPath);  
        beginCmpPath = strlen(currentPath);  
        strcat(currentPath, path);  
        if(path[pathLen] != '/')  
        {  
            strcat(currentPath, "/");  
        }  
        endCmpPath = strlen(currentPath);  
          
    }  
    else  
    {  
        //绝对路径  
        int pathLen = strlen(path);  
        strcpy(currentPath, path);  
        if(path[pathLen] != '/')  
        {  
            strcat(currentPath, "/");  
        }  
        beginCmpPath = 1;  
        endCmpPath = strlen(currentPath);  
    }  
    //创建各级目录  
    int i;
    for(i = beginCmpPath; i < endCmpPath ; i++ )  
    {  
        if('/' == currentPath[i])  
        {  
            currentPath[i] = '\0';  
            if(access(currentPath, NULL) != 0)  
            {  
                if(mkdir(currentPath, 0755) == -1)  
                {  
                    //printf("currentPath = %s\n", currentPath);  
                    perror("mkdir error\n");  
                    return -1;  
                }  
            }  
            currentPath[i] = '/';  
        }  
    }  
    return 0;  
}  

#else
int create_path(const char *path)
{
	char *start;
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	if (path[0] == '/')
		start = strchr(path + 1, '/');
	else
		start = strchr(path, '/');

	while (start) {
		char *buffer = strdup(path);
		buffer[start-path] = 0x00;

		if (mkdir(buffer, mode) == -1 && errno != EEXIST) {
			fprintf(stderr, "Problem creating directory %s", buffer);
			perror("Problem creating directory");
			free(buffer);
			return -1;
		}
		free(buffer);
		start = strchr(start + 1, '/');
	}
	return 0;
}
#endif

/*int delete_file(char *filename)
{
    char cmd[64];
    
    memset(cmd,0,64);
    if(0 == access(filename,F_OK))
    {
        sprintf(cmd,"rm %s",filename);
        eval(cmd);
    }

}*/

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
		} else {
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
			} else if (!strncmp(path, ">", 1)) {
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
		} else {
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

int UriHasScheme(const char *uri)
{
	return strstr(uri, "://") != NULL;
	//http://appfile.tuling123.com/media/audio/20180524/47395b1a0d404db393894ba7dead86fc.mp3
}


char *SkipToBraces(const char *p)
{
	while ((*p) != '{')
		++p;
	return p;
}
long getCurrentTime()    
{
   struct timeval tv;    
   
   gettimeofday(&tv,NULL);    
   
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;  
}
int getCurrentTimeSec()    
{
   struct timeval tv;    
  // gettimeofday(&tv, NULL);    

  //time(NULL);
   //return tv.tv_sec;    
    return time(NULL);
}

unsigned long get_file_size(const char *path)
{
	int iRet = -1;
	struct stat buf;  

	if (0 == stat(path, &buf))
	{
		return buf.st_size;
	}

	return iRet;
}

char *GetUrlSongName(char *url)
{
	char *val = NULL;
	char *start = NULL;
	char *songName = NULL;
	start = url;
	char *tmp= NULL;
	while (1) {
		
		tmp = strchr(start, '/');
		if(tmp == NULL) {
			songName = start;
			break;
		}
		start = tmp + 1;
		debug("start:%s",start);
		//9830db67a23440a39a25e45de4c8a41e.mp3
	}
	if(songName) {
		debug("%s",songName);
		int len ;
		tmp = strchr(songName, '?');
		if (tmp) {
			len = tmp - songName;
			
		} else {
			len = strlen(songName);
		}
		debug("len:%d",len);
		val = compat_strdup_len(songName, len);
	}
	
	return val;
}



int exec_cmd(const char *cmd)
{
    char buf[MAX_COMMAND_LEN];
    FILE *pfile;
    int status = -2;

    pthread_mutex_lock(&popen_mtx);
    if ((pfile = popen(cmd, "r"))) {
        fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
        while(!feof(pfile)) {
            fgets(buf, sizeof buf, pfile);
        }
        status = pclose(pfile);
    }
    pthread_mutex_unlock(&popen_mtx);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return status;
}

int DelStrline(char *str)
{
	char ch;
    while( '\n' != *str && *str)
    {
    	++str;
    }
    *str = '\0';
    return 0;
}


int GetFileLength(char *file)
{
	FILE *dp =NULL;
	char *buf,*deline;
	int ret = -1;
	char * line = NULL;  
	size_t len = 0;
	int num  = 0;
	ssize_t read;  
	dp= fopen(file, "r");
	if(dp == NULL)
		return -1;
	while ((read = getline(&line, &len, dp)) != -1)  
    {  
        debug("Retrieved line of length:%zu", read);  
        debug("%s", line);  
		num++;	
		if(line) {
			free(line);
			line = NULL;
		}
    }  
	ret  = num;
	if(line) {
		free(line);
		line = NULL;
	}
	if(dp) {
		fclose(dp);
	}
	return ret;
}

char *GetFileFistLine(char *file)
{
	FILE *fp =NULL;
	int ret = -1;
	char *tmp = NULL;  
	char *buf = NULL;
	char line[1024] = {0};
	size_t len = 0;
	fp = fopen(file, "r");
	if(fp == NULL)
		return NULL;
	 memset(line, 0 , 1024);
	tmp = fgets(line, sizeof(line), fp);
	if(tmp == NULL)
		goto exit;
#if 0
    len =  strlen(line) -1;
	char ch = line[len];
	if(ch  == '\n' || ch  == '\r')
		line[strlen(line)-1] = 0;
    line[strlen(line)] = 0;
#endif
	DelStrline(line);
	buf = strdup(line);
exit:
	if(fp) {
		fclose(fp);
	}

	return buf;
}

long getFileSize(const char *path)  
{  
    long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
		perror("getfilesize is fail:");	//无文件
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}  

unsigned long GetDeviceTotalSize(char *path)
{
	struct statfs diskInfo;  
	size_t mbTotalsize ;
	unsigned long totalBlocks;
	unsigned long totalSize;
		
    statfs(path, &diskInfo);  
    totalBlocks = diskInfo.f_bsize;  
    totalSize = totalBlocks * diskInfo.f_blocks;  
    //mbTotalsize = totalSize>>10;  
  
    debug(" mbTotalsize=%dKB", mbTotalsize);  
	debug(" totalSize=%lldB", totalSize);  
	return totalSize;
}
unsigned long GetDeviceFreeSize(char *path)
{
	struct statfs diskInfo;  
	unsigned long freeDisk;
	size_t mbFreedisk;
	unsigned long totalBlocks;

	if (statfs(path, &diskInfo) < 0) {
		error("statfs failed for path->[%s]", path);
		return 0;
	}  
	
	totalBlocks = diskInfo.f_bsize;  
	debug(" totalBlocks=%ld", totalBlocks);  
	freeDisk = diskInfo.f_bfree*totalBlocks;  
	mbFreedisk = freeDisk >> 10;  
	debug(" mbFreedisk=%dKB", mbFreedisk);  
	debug(" mbFreedisk=%dMB", freeDisk >> 20);
	debug(" mbFreedisk=%dGB", freeDisk >> 30);
	debug(" freeDisk=%lu", freeDisk);  
	return freeDisk;

}

char* join(char *s1, char *s2)  
{  
    char *result = malloc(strlen(s1)+strlen(s2)+1);
	
    if (result == NULL) return NULL;  
  	memset(result , 0 , strlen(s1)+strlen(s2)+1);
	
    strcpy(result, s1);  
    strcat(result, s2);  
    return result;  
}

int get_pid(char *pidName)
{
    char filename[128];
    char name[128];
    struct dirent *next;
    FILE *file;
    DIR *dir;
    pid_t retval = 0;

    dir = opendir("/proc");
    if (!dir) 
    {
        return 0;
    }

    while ((next = readdir(dir)) != NULL) {
        /* Must skip ".." since that is outside /proc */
        if (strcmp(next->d_name, "..") == 0)
            continue;

        /* If it isn't a number, we don't want it */
        if (!isdigit(*next->d_name))
            continue;

        sprintf(filename, "/proc/%s/status", next->d_name);
        if (! (file = fopen(filename, "r")) )
            continue;

        if (fgets(filename, sizeof(filename)-1, file) != NULL) {
            /* Buffer should contain a string like "Name:   binary_name" */
            sscanf(filename, "%*s %s", name);
            if (strcmp(name, pidName) == 0) {
                retval = strtol(next->d_name, NULL, 0);
            }
        }
        fclose(file);

        if (retval) 
        {
            break;
        }
    }
    closedir(dir);

    return retval;
}

int GetMac(char *id)
{

	char *device = "br0"; 
	unsigned char macaddr[ETH_ALEN]; 
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0); 
	struct ifreq req;  
	int err,rc;  
	rc  = strcpy(req.ifr_name, device); 
	err = ioctl(s, SIOCGIFHWADDR, &req); 
	close(s);  

	if (err != -1 )  
	{  
		memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); 
		
		for(i = 0; i < ETH_ALEN; i++)  
		{
			//sprintf(&id[(3+i) * 2], "%02x", macaddr[i]);
			if(i == ETH_ALEN - 1)
				sprintf(id+(3*i),"%02x",macaddr[i]);
			else
				sprintf(id+(3*i),"%02x:",macaddr[i]);
		}
		
	}
	return 0;
}

int wav_play2(const char *file)
{	
	char buf[512] = {0};
    int ret = -1;
	exec_cmd("killall -9 wavplayer");
	snprintf(buf, 512, "wavplayer %s" ,file);
	ret = exec_cmd(buf);
    error("buf = %s",buf);
    return ret;
}



