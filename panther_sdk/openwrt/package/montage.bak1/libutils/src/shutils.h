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
 * $Id: shutils.h,v 1.4 2005/11/21 15:03:16 seg Exp $
 */

#ifndef __SHUTILS_H__
#define __SHUTILS_H__
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <byteswap.h>

#include "logger.h"
#include "strutils.h"
#include "filesutils.h"
#include "procutils.h"

#define MAX_NVPARSE 256
#define FW_CREATE       0
#define FW_APPEND       1
#define FW_NEWLINE      2
#define CDB_PROCFS_FILENAME     "/proc/cdb"
#define CDB_DEFAULT_CONFIG      "/etc/config/default.cdb"
#define CDB_COMMITTED_CONFIG    "/tmp/run/config.cdb"
#define CDB_TEMP_CONFIG         "/tmp/run/tmpcfg.cdb"
#define CDB_MTD									"/dev/mtd2"
#define CDB_BUF_SIZE 						512
#define CDB_OUTPUT_TYPE_FILE		2
#define CDB_OUTPUT_TYPE_BUF			1
#define CDBH_MAGIC              "#CDB"

#if !defined(htobe16)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe16(x) __bswap_16 (x)
#define htole16(x) (x)
#define be16toh(x) __bswap_16 (x)
#define le16toh(x) (x)

#define htobe32(x) __bswap_32 (x)
#define htole32(x) (x)
#define be32toh(x) __bswap_32 (x)
#define le32toh(x) (x)

#define htobe64(x) __bswap_64 (x)
#define htole64(x) (x)
#define be64toh(x) __bswap_64 (x)
#define le64toh(x) (x)
#else
#define htobe16(x) (x)
#define htole16(x) __bswap_16 (x)
#define be16toh(x) (x)
#define le16toh(x) __bswap_16 (x)

#define htobe32(x) (x)
#define htole32(x) __bswap_32 (x)
#define be32toh(x) (x)
#define le32toh(x) __bswap_32 (x)

#define htobe64(x) (x)
#define htole64(x) __bswap_64 (x)
#define be64toh(x) (x)
#define le64toh(x) __bswap_64 (x)
#endif
#endif

#define FREE_MEM_NONE  0
#define FREE_MEM_PAGE  1
#define FREE_MEM_INODE 2
#define FREE_MEM_ALL   3

#define output_console(fmt, args...) (				\
	{																						\
	  FILE *log = fopen ("/dev/console", "a");	\
	  if (log != NULL) {												\
		fprintf(log, "%s, ", __FUNCTION__); 			\
		fprintf(log, fmt, ##args); 								\
		fclose(log);															\
	  }																					\
	}																						\
)

extern long uptime(void);
extern void *xmalloc(size_t size);
/*
 * Reads file and returns contents
 * @param       fd      file descriptor
 * @return      contents of file or NULL if an error occurred
 */
extern char *fd2str(int fd);

/*
 * Reads file and returns contents
 * @param       path    path to file
 * @return      contents of file or NULL if an error occurred
 */
extern char *file2str(const char *path);

/*
 * Waits for a file descriptor to become available for reading or unblocked signal
 * @param       fd      file descriptor
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @return      1 if descriptor changed status or 0 if timed out or -1 on error
 */
extern int waitfor(int fd, int timeout);

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @param       path    NULL, ">output", or ">>output"
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @param       ppid    NULL to wait for child termination or pointer to pid
 * @return      return value of executed command or errno
 */

int _evalpid(char *const argv[], char *path, int timeout, int *ppid);
int _waitpid (pid_t pid, int *stat_loc, int options);

extern pid_t get_pid_by_name(char *pidName);
extern pid_t get_pid_by_pidfile(char *pidFile, char *progName);

extern void excute_shell_in_dir(char *path);
/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @return      stdout of executed command or NULL if an error occurred
 */
extern char *_backtick(char *const argv[]);

/*
 * Kills process whose PID is stored in plaintext in pidfile
 * @param       pidfile PID file
 * @return      0 on success and errno on failure
 */
extern int kill_pidfile(char *pidfile);

/*
 * fread() with automatic retry on syscall interrupt
 * @param       ptr     location to store to
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully read
 */
extern int safe_fread(void *ptr, size_t size, size_t nmemb, FILE * stream);
extern ssize_t safe_read(int fd, void *buf, size_t count);
extern ssize_t safe_read_line(int fd, char *s, size_t count);
/*
 * fwrite() with automatic retry on syscall interrupt
 * @param       ptr     location to read from
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully written
 */
extern int safe_fwrite(const void *ptr, size_t size, size_t nmemb,
               FILE * stream);
extern ssize_t safe_write(int fd, const void *buf, size_t count);

extern int indexof(char *str, char c);

extern int sysprintf(const char *fmt, ...);

extern void set_ip_forward(char c);
extern int writeproc(char *path, char *value);
extern int writevaproc(char *value, char *fmt, ...);

/*
 * Concatenate two strings together into a caller supplied buffer
 * @param       s1      first string
 * @param       s2      second string
 * @param       buf     buffer large enough to hold both strings
 * @return      buf
 */
extern char *strcat_r(const char *s1, const char *s2, char *buf);

/*
 * Check for a blank character; that is, a space or a tab 
 */
#define dd_isblank(c) ((c) == ' ' || (c) == '\t')

/*
 * Strip trailing CR/NL from string <s> 
 */
#define chomp(s) ({ \
    char *c = (s) + strlen((s)) - 1; \
    while ((c > (s)) && (*c == '\n' || *c == '\r' || *c == ' ')) \
        *c-- = '\0'; \
    s; \
})

/*
 * Simple version of _backtick() 
 */
#define backtick(cmd, args...) ({ \
    char *argv[] = { cmd, ## args, NULL }; \
    _backtick(argv); \
})
/*
 * Return NUL instead of NULL if undefined 
 */
#define safe_getenv(s) (getenv(s) ? : "")


/*
 * Print directly to the console 
 */

#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)


/*
 * Simple version of _evalpid() (no timeout and wait for child termination, output to console) 
 */
#define eval(cmd, args...) ({ \
    char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})

/*
 * Simple version 2 of _evalpid() (no timeout and don't wait for child termination, output to console) 
 */
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

/*
 * Copy each token in wordlist delimited by space into word 
 */
#define foreach(word, foreachwordlist, next) \
    for (next = &foreachwordlist[strspn(foreachwordlist, " ")], \
         strncpy(word, next, sizeof(word)), \
         word[strcspn(word, " ")] = '\0', \
         word[sizeof(word) - 1] = '\0', \
         next = strchr(next, ' '); \
         strlen(word); \
         next = next ? &next[strspn(next, " ")] : "", \
         strncpy(word, next, sizeof(word)), \
         word[strcspn(word, " ")] = '\0', \
         word[sizeof(word) - 1] = '\0', \
         next = strchr(next, ' ')) \

/*
 * Copy each token in wordlist delimited by enter into word 
 */     
#define foreach2(word, foreachwordlist, next) \
    for (next = &foreachwordlist[strspn(foreachwordlist, "\n")], \
         strncpy(word, next, sizeof(word)), \
         word[strcspn(word, "\n")] = '\0', \
         word[sizeof(word) - 1] = '\0', \
         next = strchr(next, '\n'); \
         strlen(word); \
         next = next ? &next[strspn(next, "\n")] : "", \
         strncpy(word, next, sizeof(word)), \
         word[strcspn(word, "\n")] = '\0', \
         word[sizeof(word) - 1] = '\0', \
         next = strchr(next, '\n')) \

/*
 * Debug print 
 */
#ifdef DEBUG
#define dprintf(fmt, args...) cprintf("%s: " fmt, __FUNCTION__, ## args)
#else
#define dprintf(fmt, args...)
#endif

#ifdef HAVE_MICRO
#define FORK(a) a;
#else
#define FORK(func) \
{ \
    switch ( fork(  ) ) \
    { \
    case -1: \
        break; \
    case 0: \
        ( void )setsid(  ); \
        func; \
        exit(0); \
        break; \
    default: \
    break; \
    } \
}
#endif

typedef enum {
    RET_OK=0,
    RET_ERR,
}Ret;

#define gettid()    (pid_t)syscall(SYS_gettid)

#ifndef MAX
#define MAX(a,b)    ((a)>(b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)    ((a)<(b) ? (a) : (b))
#endif

#define MAX_COMMAND_LEN     1024

extern char *new_fgets(char *s, int n, FILE *stream);
extern char *int2str(int i);
extern int exec_cmd(const char *cmd);
extern int exec_cmd2(const char *cmd, ...);
extern int exec_cmd3(char *rbuf, int rbuflen, const char *cmd, ...);
extern int exec_cmd4(void **hdr, char **ptr, int toFree, const char *cmd, ...);

#define SSDF_START_DAEMON               1
#define SSDF_BACKGROUND_DAEMON          2
#define SSDF_START_BACKGROUND_DAEMON    (SSDF_START_DAEMON | SSDF_BACKGROUND_DAEMON)
#define SSDF_STOP_DAEMON                4
typedef enum
{
    NICE_39  = 19,
    NICE_38  = 18,
    NICE_37  = 17,
    NICE_36  = 16,
    NICE_35  = 15,
    NICE_34  = 14,
    NICE_33  = 13,
    NICE_32  = 12,
    NICE_31  = 11,
    NICE_30  = 10,
    NICE_29  =  9,
    NICE_28  =  8,
    NICE_27  =  7,
    NICE_26  =  6,
    NICE_25  =  5,
    NICE_24  =  4,
    NICE_23  =  3,
    NICE_22  =  2,
    NICE_21  =  1,
    NICE_20  =  0,
    NICE_19  = -1,
    NICE_18  = -2,
    NICE_17  = -3,
    NICE_16  = -4,
    NICE_15  = -5,
    NICE_14  = -6,
    NICE_13  = -7,
    NICE_12  = -8,
    NICE_11  = -9,
    NICE_10  = -10,
    NICE_09  = -11,
    NICE_08  = -12,
    NICE_07  = -13,
    NICE_06  = -14,
    NICE_05  = -15,
    NICE_04  = -16,
    NICE_03  = -17,
    NICE_02  = -18,
    NICE_01  = -19,
    NICE_00  = -20,

    NICE_DEFAULT         = NICE_20,
    NICE_LEAST_FAVORABLE = NICE_39,
    NICE_MOST_FAVORABLE  = NICE_00,
} NiceNess;
extern int start_stop_daemon(char *prog, int flag, NiceNess nice, int sig, char *pdf, const char *fmt, ...);
extern int start_stop_daemon_args_with_space(char *prog, int flag, NiceNess nice, int sig, char *pdf, const char *fmt, ...);

extern int cp(const char *src, const char *dst);
extern long timevaldiff(struct timeval *starttime, struct timeval *finishtime);
extern long gettimevalmsec(struct timeval *tv);
extern void gettimeofbegin(struct timeval *start_tv);
extern long gettimevaldiff(struct timeval *start_tv);

extern int setup_codec_info(char *info, int len);
extern int sort_file(char *infile, char *outfile);

extern int gpio_read(int gpio );
extern int gpio_write(int gpio, int value );

#endif				/* _shutils_h_ */
