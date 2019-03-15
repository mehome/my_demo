/*
 * Shell-like utility functions
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: shutils.c,v 1.2 2005/11/11 09:26:18 seg Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/route.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <sys/sysinfo.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <pthread.h>

#include "shutils.h"


#include "linklist.h"

static pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;

#ifndef HAVE_STRLCAT
/*
 * '_cups_strlcat()' - Safely concatenate two strings.
 */

size_t                        /* O - Length of string */
strlcat(char       *dst,      /* O - Destination string */
        const char *src,      /* I - Source string */
        size_t     size)      /* I - Size of destination string buffer */
{
    size_t    srclen;         /* Length of source string */
    size_t    dstlen;         /* Length of destination string */


    /*
     * Figure out how much room is left...
     */

    dstlen = strlen(dst);
    size   -= dstlen + 1;

    if (!size)
        return (dstlen);        /* No room, return immediately... */

    /*
     * Figure out how much room is needed...
     */

    srclen = strlen(src);

    /*
     * Copy the appropriate amount...
     */

    if (srclen > size)
        srclen = size;

    memcpy(dst + dstlen, src, srclen);
    dst[dstlen + srclen] = '\0';

    return (dstlen + srclen);
}
#endif /* !HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
/*
 * '_cups_strlcpy()' - Safely copy two strings.
 */

size_t                        /* O - Length of string */
strlcpy(char        *dst,     /* O - Destination string */
        const char  *src,     /* I - Source string */
        size_t      size)     /* I - Size of destination string buffer */
{
    size_t    srclen;         /* Length of source string */


    /*
     * Figure out how much room is needed...
     */

    size --;

    srclen = strlen(src);

    /*
     * Copy the appropriate amount...
     */

    if (srclen > size)
        srclen = size;

    memcpy(dst, src, srclen);
    dst[srclen] = '\0';

    return (srclen);
}
#endif /* !HAVE_STRLCPY */

long uptime(void)
{
    struct sysinfo info;

    sysinfo(&info);
    return info.uptime;
}

void *xmalloc(size_t size)
{
    void *value = malloc(size);
    if (value)
        return value;

    sysprintf("%s: memory exhausted", __FILE__);
    exit(1);
}

/*
 * Reads file and returns contents
 * @param       fd      file descriptor
 * @return      contents of file or NULL if an error occurred
 */
char *fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	}
	while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param       path    path to file
 * @return      contents of file or NULL if an error occurred
 */
char *file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param       fd      file descriptor
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @return      1 if descriptor changed status or 0 if timed out or -1 on error
 */
int waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @param       path    NULL, ">output", or ">>output"
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @param       ppid    NULL to wait for child termination or pointer to pid
 * @return      return value of executed command or errno
 */
__attribute__((used))
static void flog(const char *fmt, ...)
{
		char varbuf[256];
		va_list args;

		va_start(args, (char *)fmt);
		vsnprintf(varbuf, sizeof(varbuf), fmt, args);
		va_end(args);
		FILE *fp = fopen("/tmp/syslog.log", "ab");
		fprintf(fp, "%s", varbuf);
		fclose(fp);
}


int sysprintf(const char *fmt, ...)
{
	char varbuf[256];
	va_list args;

	va_start(args, (char *)fmt);
	vsnprintf(varbuf, sizeof(varbuf), fmt, args);
	va_end(args);
	return system(varbuf);
}

// FILE *debugfp=NULL;
int _evalpid(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	// if (debugfp==NULL)
	// debugfp = fopen("/tmp/evallog.log","wb");
	// char buf[254] = "";
/* #ifndef HAVE_SILENCE */
#if 0
	if ( 1 ) {
		int i = 0;

		while (argv[i] != NULL) {
			fprintf(stderr, "%s ", argv[i++]);
			flog("%s ", argv[i - 1]);
		}
		fprintf(stderr, "\n");
		flog("\n");
	}

	int i = 0;

	while (argv[i] != NULL) {
		fprintf(stderr, "%s ", argv[i++]);
	}
	fprintf(stderr, "\n");

#endif

	switch (pid = fork()) {
	case -1:		/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
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

int _waitpid (pid_t pid, int *stat_loc, int options)
{
	if (waitpid(pid, stat_loc, options) != pid)
		return -1;
	return 0;
}

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @return      stdout of executed command or NULL if an error occurred
 */
char *_backtick(char *const argv[])
{
	int filedes[2];
	pid_t pid;
	int status;
	char *buf = NULL;

	/*
	 * create pipe 
	 */
	if (pipe(filedes) == -1) {
		perror(argv[0]);
		return NULL;
	}

	switch (pid = fork()) {
	case -1:		/* error */
		return NULL;
	case 0:		/* child */
		close(filedes[0]);	/* close read end of pipe */
		dup2(filedes[1], 1);	/* redirect stdout to write end of
					 * pipe */
		close(filedes[1]);	/* close write end of pipe */
		execvp(argv[0], argv);
		exit(errno);
		break;
	default:		/* parent */
		close(filedes[1]);	/* close write end of pipe */
		buf = fd2str(filedes[0]);
		waitpid(pid, &status, 0);
		break;
	}

	return buf;
}

/*
 * Kills process whose PID is stored in plaintext in pidfile
 * @param       pidfile PID file
 * @return      0 on success and errno on failure
 */
int kill_pidfile(char *pidfile)
{
	FILE *fp = fopen(pidfile, "r");
	char buf[256];

	if (fp && fgets(buf, sizeof(buf), fp)) {
		pid_t pid = strtoul(buf, NULL, 0);

		fclose(fp);
		return kill(pid, SIGTERM);
	} else
		return errno;
}



/*
 * Kills process whose name is is proc
 * @proc       name of process
 * @return      0 on success and errno on failure
 */
int killall(char *proc)
{
	pid_t pid = 0;

	pid = get_pid_by_name(proc);
	if( pid > 0 )
		return kill(pid, SIGTERM);
	
	return -1;
}


/*
 * Kills process whose name is is proc
 * @proc       name of process
 * @return      0 on success and errno on failure
 * SIGHUP           1     Hangup (POSIX).
 * SIGINT           2     Interrupt (ANSI).
 * SIGQUIT          3      Quit (POSIX).
 * SIGILL           4      Illegal instruction (ANSI).
 * SIGTRAP          5      Trace trap (POSIX).
 * SIGIOT           6      IOT trap (4.2 BSD).
 * SIGABRT          SIGIOT Abort (ANSI).
 * SIGEMT           7
 * SIGFPE           8      Floating-point exception (ANSI).
 * SIGKILL          9      Kill, unblockable (POSIX).
 * SIGBUS          10      BUS error (4.2 BSD).
 * SIGSEGV         11      Segmentation violation (ANSI).
 * SIGSYS          12
 * SIGPIPE         13      Broken pipe (POSIX).
 * SIGALRM         14      Alarm clock (POSIX).
 * SIGTERM         15      Termination (ANSI).
 * SIGUSR1         16      User-defined signal 1 (POSIX).
 * SIGUSR2         17      User-defined signal 2 (POSIX).
 * SIGCHLD         18      Child status has changed (POSIX).
 * SIGCLD          SIGCHLD Same as SIGCHLD (System V).
 * SIGPWR          19      Power failure restart (System V).
 * SIGWINCH        20      Window size change (4.3 BSD, Sun).
 */
int killall2(char *proc , int sig)
{
	pid_t pid = 0;

	if( sig <= 0 )
		 sig = SIGTERM;
	pid = get_pid_by_name(proc);
	if( pid > 0 )
		return kill(pid, sig);
	
	return -1;
}

/*
 * fread() with automatic retry on syscall interrupt
 * @param       ptr     location to store to
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully read
 */
int safe_fread(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	}
	while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}


ssize_t safe_read(int fd, void *buf, size_t count)
{
    ssize_t n = 0;

    do {
        n = read(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}

ssize_t safe_read_line(int fd, char *s, size_t count)
{
    ssize_t n = 0;
    char ch = EOF;

		if( fd < 0 || count <= 0 )
			return 0;
			
    while ( (--count > 0) && (read(fd, &ch, 1) == 1) && (ch != EOF) && (ch != '\n') ) {    
        s[n++] = ch;    
    }    
    if (ch == '\n') {    
        s[n ++] = '\0';    
    }      
    return n; 
}


/*
 * fwrite() with automatic retry on syscall interrupt
 * @param       ptr     location to read from
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully written
 */
int safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	}
	while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}


ssize_t safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    do {
        n = write(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}



/*
 * Search a string backwards for a set of characters
 * This is the reverse version of strspn()
 *
 * @param       s       string to search backwards
 * @param       accept  set of chars for which to search
 * @return      number of characters in the trailing segment of s 
 *              which consist only of characters from accept.
 */
static size_t sh_strrspn(const char *s, const char *accept)
{
	const char *p;
	size_t accept_len = strlen(accept);
	int i;

	if (s[0] == '\0')
		return 0;

	p = s + (strlen(s) - 1);
	i = 0;

	do {
		if (memchr(accept, *p, accept_len) == NULL)
			break;
		p--;
		i++;
	}
	while (p != s);

	return i;
}



int f_exists(const char *path)	// note: anything but a directory
{
	struct stat st;

	return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}

int f_isdir(const char *path)	// note: anything but a directory
{
	struct stat st;

	return (stat(path, &st) == 0) && (S_ISDIR(st.st_mode));
}

int f_read(const char *path, void *buffer, int max)
{
	int f;
	int n;

	if ((f = open(path, O_RDONLY)) < 0)
		return -1;
	n = read(f, buffer, max);
	close(f);
	return n;
}

int f_read_string(const char *path, char *buffer, int max)
{
	if (max <= 0)
		return -1;
	int n = f_read(path, buffer, max - 1);

	buffer[(n > 0) ? n : 0] = 0;
	return n;
}

#define WLMBSS_DEV_NAME	"wlmbss"
#define WL_DEV_NAME "wl"
#define WDS_DEV_NAME	"wds"

int indexof(char *str, char c)
{
	if (str == NULL)
		return -1;
	int i;
	int slen = strlen(str);

	for (i = 0; i < slen; i++)
		if (str[(slen - 1) - i] == c)
			return (slen - 1) - i;
	return -1;
}

char *strcat_r(const char *s1, const char *s2, char *buf)
{
	strcpy(buf, s1);
	strcat(buf, s2);
	return buf;
}


char *psname(int pid, char *buffer, int maxlen)
{
	char buf[512];
	char path[64];
	char *p;

	if (maxlen <= 0)
		return NULL;
	*buffer = 0;
	sprintf(path, "/proc/%d/stat", pid);
	if ((f_read_string(path, buf, sizeof(buf)) > 4)
	    && ((p = strrchr(buf, ')')) != NULL)) {
		*p = 0;
		if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
			strlcpy(buffer, p + 1, maxlen);
		}
	}
	return buffer;
}

int wait_file_exists(const char *name, int max, int invert)
{
	while (max-- > 0) {
		if (f_exists(name) ^ invert)
			return 1;
		sleep(1);
	}
	return 0;
}


int writeproc(char *path, char *value)
{
	FILE *fp;
	fp = fopen(path, "wb");
	if (fp == NULL) {
		return -1;
	}
	fprintf(fp, "%s", value);
	fclose(fp);
	return 0;
}

int writevaproc(char *value, char *fmt, ...)
{
	char varbuf[256];
	va_list args;

	va_start(args, (char *)fmt);
	vsnprintf(varbuf, sizeof(varbuf), fmt, args);
	va_end(args);
	return writeproc(varbuf, value);
}

void set_ip_forward(char c)
{
	char ch[8];
	sprintf(ch, "%c", c);
	writeproc("/proc/sys/net/ipv4/ip_forward", ch);
}





int f_write(const char *path, const void *buffer, int len, unsigned flags, unsigned cmode)
{
	static const char nl = '\n';
	int f;
	int r = -1;
	mode_t m;

	m = umask(0);
	if (cmode == 0) cmode = 0666;
	if ((f = open(path, (flags & FW_APPEND) ? (O_WRONLY|O_CREAT|O_APPEND) : (O_WRONLY|O_CREAT|O_TRUNC), cmode)) >= 0) {
		if ((buffer == NULL) || ((r = write(f, buffer, len)) == len)) {
			if (flags & FW_NEWLINE) {
				if (write(f, &nl, 1) == 1) ++r;
			}
		}
		close(f);
	}
	umask(m);
	return r;
}



int f_write_string(const char *path, const char *buffer, unsigned flags, unsigned cmode)
{
	return f_write(path, buffer, strlen(buffer), flags, cmode);
}


/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @param       path    NULL, ">output", or ">>output"
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @param       ppid    NULL to wait for child termination or pointer to pid
 * @return      return value of executed command or errno
 */

int f_wsprintf(const char *file, const char *fmt, ...)
{
		char varbuf[1024];
		va_list args;

		if( !file )
			return -1;
		
		memset(varbuf, 0, sizeof(varbuf));
		va_start(args, (char *)fmt);
		vsnprintf(varbuf, sizeof(varbuf), fmt, args);
		va_end(args);
		FILE *fp = fopen(file, "ab");
		fprintf(fp, "%s", varbuf);
		fclose(fp);
		
		return 0;
}

pid_t get_pid_by_name(char *pidName)
{
    char filename[128];
    char name[128];
    struct dirent *next;
    FILE *file;
    DIR *dir;
    pid_t retval = -1;

    dir = opendir("/proc");
    if (!dir) {
        perror("Cannot open /proc\n");
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

        if (retval > 0) {
            break;
        }
    }
    closedir(dir);

    return retval;
}


void excute_shell_in_dir(char *path)
{
    DIR             *dir;
    struct dirent   *next;

    if ((dir = opendir(path)) == NULL) {           
            return;
    }

    while ((next = readdir(dir)) != NULL) {

            char buffer[256];

            /* If it is a ., or .. ignore it */
            if (!strcmp(next->d_name, ".") || !strcmp(next->d_name, "..") )
                    continue;

						sprintf(buffer, "%s/%s",  path, next->d_name );

						eval("/bin/sh", buffer );
    }

    closedir(dir);
    return;
}

char *
get_rfctime(void)
{
    static char s[129];
    struct tm t1;
    time_t t2;

		time(&t2);
    memcpy(&t1, localtime(&t2), sizeof(struct tm));
    strftime(s, 128, "%a %d %b %H:%M:%S %Y", &t1);
    return s;
}

char *new_fgets(char *s, int n, FILE *stream)
{
	char *r = fgets(s, n, stream);
	if ( !r || ferror(stream) || feof(stream) )
		return NULL;
	n = strlen(s);
	if(s[n - 1] == '\n')
		s[n - 1] = '\0';
	return r;
}

char *int2str(int i)
{
    static char str[12];
    sprintf(str, "%d", i);
    return str;
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
	return -1;
}

int exec_cmd2(const char *cmd, ...)
{
	char buf[MAX_COMMAND_LEN];
	va_list args;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	return exec_cmd(buf);
}

int exec_cmd3(char *rbuf, int rbuflen, const char *cmd, ...)
{
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
		while(!feof(pfile)) {
			if ((rbuflen > 0) && fgets(buf, MIN(rbuflen, sizeof(buf)), pfile)) {
				int len = snprintf(p, rbuflen, "%s", buf);
				rbuflen -= len;
				p += len;
			}
            else {
                break;
            }
		}
		if ((rbuf) && (p != rbuf) && (*(p-1) == '\n')) {
			*(p-1) = 0;
		}
		status = pclose(pfile);
	}
	pthread_mutex_unlock(&popen_mtx);
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	return -1;
}

typedef struct {
	struct list_head list;
	char *text;
} cmdListNode;

int exec_cmd4(void **hdr, char **ptr, int toFree, const char *cmd, ...)
{
	char buf[MAX_COMMAND_LEN];
	va_list args;
	FILE *pfile;
	int status = -2;
	char **rptr = ptr;
	struct list_head **rlh = (struct list_head **)hdr;
	struct list_head *lh;
	struct list_head *pos, *next;
	cmdListNode *node;

	if (!rlh || !rptr) {
		return -1;
	}

	if (*rptr) {
		free(*rptr);
		*rptr = NULL;
	}
	if ((lh = *rlh)) {
		if (list_empty(lh)) {
			free(lh);
			*rlh = NULL;
			return -1;
		}
		else {
			list_for_each_safe(pos, next, lh) {
				node = (cmdListNode *)list_entry(pos, cmdListNode, list);
				list_del(&node->list);
				if (toFree) {
					free(node->text);
					free(node);
				}
				else {
					*rptr = node->text;
					free(node);
					return 0;
				}
			}
			free(lh);
			*rlh = NULL;
			return -1;
		}
	}
	else {
		if ((!toFree) && (lh = calloc(sizeof(struct list_head), 1)) != NULL) {
			*rlh = lh->next = lh->prev = lh;
		}
		else {
			return -1;
		}
	}

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	pthread_mutex_lock(&popen_mtx);
	if ((pfile = popen(buf, "r"))) {
		fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
		while(!feof(pfile)) {
			if (new_fgets(buf, sizeof buf, pfile)) {
				if (*rptr == NULL) {
					*rptr = strdup(buf);
				}
				else {
					if ((node = (cmdListNode *)calloc(sizeof(cmdListNode), 1)) != NULL) {
						node->list.next = node->list.prev = &node->list;
						node->text = strdup(buf);
						if (node->text) {
							list_add_tail(&node->list, lh);
						}
						else {
							free(node);
						}
					}
				}
			}
		}
		status = pclose(pfile);
	}
	pthread_mutex_unlock(&popen_mtx);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0) {
			return WEXITSTATUS(status);
		}
	}
	return exec_cmd4(hdr, ptr, 1, "");
}

int cp(const char *src, const char *dst)
{
    int f1 = 0, f2 = 0, n = 0;
    int ret = EXIT_FAILURE;
    char buf[BUFSIZ];
    struct stat st;
    int openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    mode_t filePerms = S_IRUSR | S_IWUSR | S_IXUSR |
                       S_IRGRP | S_IWGRP | S_IXGRP |
                       S_IROTH | S_IWOTH | S_IXOTH;      /* rwxrwxrwx */;

//    if (!src || !dst || !f_exists(src) || f_exists(dst))
    if (!src || !dst || !f_exists(src))
        return 0;

    if (stat(src, &st) == 0) {
        st.st_mode &= filePerms;
        filePerms = st.st_mode;
        if ((f1 = open(src, O_RDONLY, 0)) == -1)
            goto err;
        if ((f2 = open(dst, openFlags, filePerms)) == -1)
            goto err;
        while((n = read(f1, buf, BUFSIZ)) > 0) {
            if(write(f2, buf, n) != n) {
                goto err;
            }
        }
        if (n == -1)
            goto err;
        ret = EXIT_SUCCESS;
    }
err:
    if (f1)
        close(f1);
    if (f2)
        close(f2);
    chmod(dst, st.st_mode);
    return ret;
}

long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
    long msec;
    msec = (finishtime->tv_sec - starttime->tv_sec) * 1000;
    msec += (finishtime->tv_usec - starttime->tv_usec) / 1000;
    return msec;
}

long gettimevalmsec(struct timeval *tv)
{
	struct timeval now_tv;
    long msec;
    if (!tv) {
        tv = &now_tv;
        gettimeofday(tv, NULL);
    }
    msec = tv->tv_sec * 1000;
    msec += tv->tv_usec / 1000;
    return msec;
}

void gettimeofbegin(struct timeval *start_tv)
{
    gettimeofday(start_tv, NULL);
}

long gettimevaldiff(struct timeval *start_tv)
{
    struct timeval tv;
    struct timeval *end_tv = &tv;
    gettimeofday(end_tv, NULL);
    return timevaldiff(start_tv, end_tv);
}








