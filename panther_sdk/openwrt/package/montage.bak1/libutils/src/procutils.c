/*
 * =====================================================================================
 *
 *       Filename:  procutils.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/16/2016 10:36:05 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#include "filesutils.h"
#include "procutils.h"


//# cat /proc/1/stat
//1 (init) S 0 0 0 0 -1 256 287 10043 109 21377 7 110 473 1270 9 0 0 0 27 1810432 126 2147483647 4194304 4369680 2147450688 2147449688 717374852 0 0 0 514751 2147536844 0 0 0 0

char *psname(int pid, char *buffer, int maxlen)
{
    char buf[512];
    char path[64];
    char *p;

    if (maxlen <= 0) return NULL;
    *buffer = 0;
    sprintf(path, "/proc/%d/stat", pid);
    if ((f_read_string(path, buf, sizeof(buf)) > 4) && ((p = strrchr(buf, ')')) != NULL)) {
        *p = 0;
        if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
            strlcpy(buffer, p + 1, maxlen);
        }
    }
    return buffer;
}

static int _pidof(const char *name, pid_t** pids)
{
    const char *p;
    char *e;
    DIR *dir;
    struct dirent *de;
    pid_t i;
    int count;
    char buf[256];

    count = 0;
    *pids = NULL;
    if ((p = strchr(name, '/')) != NULL) name = p + 1;
    if ((dir = opendir("/proc")) != NULL) {
        while ((de = readdir(dir)) != NULL) {
            i = strtol(de->d_name, &e, 10);
            if (*e != 0) continue;
            if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
                if ((*pids = realloc(*pids, sizeof(pid_t) * (count + 1))) == NULL) {
                    return -1;
                }
                (*pids)[count++] = i;
            }
        }
    }
    closedir(dir);
    return count;
}

int pidof(const char *name)
{
    pid_t *pids;
    pid_t p;

    if (_pidof(name, &pids) > 0) {
        p = *pids;
        free(pids);
        return p;
    }
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
int killall(const char *name, int sig)
{
    pid_t *pids;
    int i;
    int r;

    if ((i = _pidof(name, &pids)) > 0) {
        r = 0;
        do {
            r |= kill(pids[--i], sig);
        } while (i > 0);
        free(pids);
        return r;
    }
    return -2;
}

