#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <linux/watchdog.h>

#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)

int xopen3(const char *pathname, int flags, int mode)
{
    int ret = open(pathname, flags, mode);
    if (ret < 0) {
        cprintf("can't open %s\n", pathname);        
    }
    return ret;
}

int xopen(const char *pathname, int flags)
{
    return xopen3(pathname, flags, 0666);
}

void xdup2(int from, int to)
{
    if (dup2(from, to) != to)
        cprintf("can't duplicate file descriptor\n");        
}

void xmove_fd(int from, int to)
{
    if (from == to)
        return;
    xdup2(from, to);
    close(from);
}

int ioctl_or_warn(int fd, unsigned request, void *argp)
{
    int ret = ioctl(fd, request, argp);
    if (ret < 0)
        cprintf("ioctl %#x failed\n", request);        
    return ret;
}

pid_t get_pid_by_name(char *pidName)
{
    char filename[128];
    char name[128];
    struct dirent *next;
    FILE *file;
    DIR *dir;
    pid_t retval = 0;

    dir = opendir("/proc");
    if (!dir) {
        cprintf("Cannot open /proc\n");
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

        if (retval) {
            cprintf("%s pid %d\n\n", pidName, retval);
            break;
        }
    }
    closedir(dir);

    return retval;
}

int main(int argc, char* argv[])
{
    pid_t wdt_pid = get_pid_by_name("watchdog");
    unsigned htimer_duration = 0; /* reboots after N ms if not restarted */
    /* WDIOC_SETTIMEOUT takes seconds, not milliseconds */
    htimer_duration = htimer_duration / 1000;

    if (wdt_pid != 0) {
        kill(wdt_pid, SIGKILL);
    }
    xmove_fd(xopen("/dev/watchdog", O_WRONLY), 3);
    ioctl_or_warn(3, WDIOC_SETTIMEOUT, &htimer_duration);
    close(3);

    return 0;
}
