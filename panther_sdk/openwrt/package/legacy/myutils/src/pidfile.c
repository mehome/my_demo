#include "pidfile.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pwd.h>
#include <errno.h>

#include "debug.h"

static int pid_fd = -1;

static pid_t read_pid(char *path)
{
	char *endptr = NULL;
	char buf[16];
	pid_t pid ;
	int fd , i;
    fd = open(path , O_RDONLY);
    if (fd == -1)
	return (pid_t)-1;

    
    i = read(fd, buf, sizeof(buf) - 1);
    close(fd);
	
    if (i <= 0)
		return (pid_t)-1;
    buf[i] = '\0';

    pid = strtol(buf, &endptr, 10);
    if (endptr != &buf[i])
		return (pid_t)-1;
    return pid;
}

static int flopen(const char *path)
{
    if ((pid_fd = open(path, O_RDWR|O_CREAT, 0644)) == -1) {
		error("Open failed: [%s]: %s",path, strerror(errno));
		return -1;
    }

    int operation = LOCK_EX | LOCK_NB;
    if (flock(pid_fd, operation) == -1) {
		close(pid_fd);
		error("flock failed");
		return -1;
    }

    if (ftruncate(pid_fd, 0) != 0) {
	/* can't happen [tm] */
		close(pid_fd);
		error("ftruncate failed");
		return -1;
    }
    return 0;
}

static pid_t pid_open(const char *path)
{
    if (flopen(path) < 0) {
		return read_pid(path);
    }
    return (pid_t)0;
}

static int pid_write()
{

	 char pidstr[20];
    /* truncate to allow multiple calls */
    if (ftruncate(pid_fd, 0) == -1) {
		error("ftruncate failed");
		return -1;
    }
  
    sprintf(pidstr, "%u", getpid());
    lseek(pid_fd, 0, 0);
	
    if (write(pid_fd, pidstr, strlen(pidstr)) != (ssize_t)strlen(pidstr)) {
		error("write failed");
		return -1;
    }
    return 0;
}

static int pid_close()
{
	int ret = 0;
	if(pid_fd > 0)
		ret = close(pid_fd);
	pid_fd = -1;	
    return ret;
}

static int pid_remove(const char *path)
{
    return unlink(path);
}



void LockPid(char *exec, char *pidfile)
{
	
	pid_t pid;
	if ((pid = pid_open(pidfile)) != 0) {
		error("%s is ruring , return (other pid?): %d",exec, pid);
		exit(-1);
	}
	if (pid_write() != 0) {
		error("Can't write pidfile failed");
		exit(-1);
	}
	//pid_close();
}
void UnLockPid()
{
	pid_close();
}


int lock_pid(char *pidfile)
{
    int fd = -1; 
    char buf[32]; 
    fd = open(pidfile, O_WRONLY | O_CREAT, 0666); 
    if (fd < 0) { 
       perror("Fail to open"); 
       exit(1); 
   } 
   struct flock lock; 
   bzero(&lock, sizeof(lock)); 
   if (fcntl(fd, F_GETLK, &lock) < 0) { 
      DEBUG_INFO("Fail to fcntl F_GETLK"); 
      exit(1); 
    } 
  lock.l_type = F_WRLCK; 
  lock.l_whence = SEEK_SET; 
  if (fcntl(fd, F_SETLK, &lock) < 0) { 
     DEBUG_INFO("Fail to fcntl F_SETLK"); 
     exit(1); 
  } 
  pid_t pid = getpid(); 
  int len = snprintf(buf, 32, "%d\n", (int)pid); 
  return 0;
}


