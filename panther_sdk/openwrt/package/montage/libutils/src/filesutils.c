/*
 * =====================================================================================
 *
 *       Filename:  filesutils.c
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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdarg.h>

#include "shutils.h"
#include "filesutils.h"

int f_exists_and_not_zero_byte(const char *path)
{
    struct stat st;
    memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode)) && (st.st_size!=0);
}

int f_exists(const char *path)    // note: anything but a directory
{
    struct stat st;
    memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}

int f_isdir(const char *path)    // note: anything but a directory
{
    struct stat st;
		memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (S_ISDIR(st.st_mode));
}

unsigned long f_size(const char *path)  // 4GB-1    -1 = error
{
    struct stat st;
    memset(&st, 0 , sizeof(st));
    if (stat(path, &st) == 0) return st.st_size;
    return (unsigned long)-1;
}

int f_read(const char *path, void *buffer, int max)
{
    int f;
    int n;

    if ((f = open(path, O_RDONLY)) < 0) return -1;
    n = read(f, buffer, max);
    close(f);
    return n;
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

int f_read_string(const char *path, char *buffer, int max)
{
    if (max <= 0) return -1;
    int n = f_read(path, buffer, max - 1);
    buffer[(n > 0) ? n : 0] = 0;
    return n;
}

int f_write_string(const char *path, const char *buffer, unsigned flags, unsigned cmode)
{
    return f_write(path, buffer, strlen(buffer), flags, cmode);
}

int f_write_sprintf(const char *path, unsigned flags, unsigned cmode, const char *fmt, ...)
{
    va_list args;
    char *buf;
    int len;

    va_start(args, (char *)fmt);
    len = vsnprintf(NULL, 0, fmt , args);
    if ((buf = malloc(len + 1)) != NULL) {
        if (vsnprintf(buf, len + 1, fmt, args) < 0) {
            free(buf);
            buf = NULL;
        }
    }
    va_end(args);
    if (buf) {
        len = f_write_string(path, buf, flags, cmode);
        free(buf);
    }
    else {
        len = 0;
    }

    return len;
}

static int _f_read_alloc(const char *path, char **buffer, int max, int z)
{
    unsigned long n;

    *buffer = NULL;
    if (max >= 0) {
        if ((n = f_size(path)) != (unsigned long)-1) {
            if (n < max) max = n;
            if ((!z) && (max == 0)) return 0;
            if ((*buffer = malloc(max + z)) != NULL) {
                if ((max = f_read(path, *buffer, max)) >= 0) {
                    if (z) *(*buffer + max) = 0;
                    return max;
                }
                free(buffer);
            }
        }
    }
    return -1;
}

int f_read_alloc(const char *path, char **buffer, int max)
{
    return _f_read_alloc(path, buffer, max, 0);
}

int f_read_alloc_string(const char *path, char **buffer, int max)
{
    return _f_read_alloc(path, buffer, max, 1);
}


static int _f_wait_exists(const char *name, int max, int invert)
{
    while (max-- > 0) {
        if (f_exists(name) ^ invert) return 1;
        sleep(1);
    }
    return 0;
}

int f_wait_exists(const char *name, int max)
{
    return _f_wait_exists(name, max, 0);
}

int f_wait_notexists(const char *name, int max)
{
    return _f_wait_exists(name, max, 1);
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
		char varbuf[2048];
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

