
/*
 * @file mtdm_misc.h
 *
 * @Author  Frank Wang
 * @Date    2015-10-06
 */

#ifndef _MTDM_MISC_H_
#define _MTDM_MISC_H_

#include "include/mtdm_types.h"

#define DEBUGMSG        0
#define DEBUGSLEEP      0
#define DEBUGWITHTIME   0

#define MAX_COMMAND_LEN        1024

/**
 * @define CONSOLE
 * @brief console path
 */
#define CONSOLE        "/dev/console"

/**
 * @define stdprintf
 * @brief print message to stdout
 */
#if DEBUGMSG
void __stdprintf_info__(FILE *cfp, const char *file, const int line, const char *function);
#define stdprintf(fmt, args...) do { \
            FILE *cfp = fopen(CONSOLE, "a"); \
            if (cfp) { \
                __stdprintf_info__(cfp,__FILE__ , __LINE__, __FUNCTION__); \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)
#else
#define stdprintf(fmt, args...) do { \
            FILE *cfp = fopen(CONSOLE, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)
#endif
/**
 * @define mtdmDebug
 * @brief print debug message
 */
#define mtdmDebug(fmt, args...) do { \
            FILE *cfp = fopen(CONSOLE, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)

#ifndef MAX
/**
 * @define MAX
 * @brief return bigger one
 */
#define MAX(a,b)    ((a)>(b) ? (a) : (b))
#endif

#ifndef MIN
/**
 * @define MIN
 * @brief return smaller one
 */
#define MIN(a,b)    ((a)<(b) ? (a) : (b))
#endif

#define _system(fmt, args...)   __system(__FILE__ , __LINE__, __FUNCTION__, fmt, ## args)
#define mtdm_kill(pid,sig)       __kill(__FILE__ , __LINE__, __FUNCTION__,pid,sig)

/**
 *  system command with parameters
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return number of command length
 *
 */
int __system(const char *file, const int line, const char *func, const char *fmt, ...);

/**
 *  kill command w/ debug message
 *
 *  @param pid
 *  @param sig
 *
 *  @return status code
 *
 */
int __kill(const char *file, const int line, const char *func, int pid, int sig);

/**
 *  get a random number
 *
 *  @return random number
 *
 */
unsigned int get_random();

/**
 *  execute system command
 *  @param *fmt... format of command.
 *  @return the number of command characters.
 *
 */
int _execl(const char *fmt, ...);


/**
 *  file exist or not
 *  @param *file_name file name
 *  @return 1 for exist, 0 for no.
 *
 */
int _isFileExist(char *file_name);

#if DEBUGMSG
#define BUG_ON()    stdprintf("CHECK!!\n")
#define OOM()       stdprintf("OOM!!\n")
#define DBG()       stdprintf("DEBUG!!\n")
#define DBGM(msg)   stdprintf("%s\n",msg)
#else
#define BUG_ON()
#define OOM()
#define DBG()
#define DBGM(msg)
#endif

#define NI(str)     stdprintf("Need Implement:[%s]\n", str);

#if DEBUGSLEEP
#define my_sleep(sec)   stdprintf("sleep %d sec\n",sec);    sleep(sec)
#define my_usleep(usec) stdprintf("usleep %d usec\n",usec); usleep(usec)
#else
#define my_sleep(sec)   sleep(sec)
#define my_usleep(usec) usleep(usec)
#endif

pid_t my_gettid(void);

int my_atoi(char *str);
int my_mac2val(u8 *val, s8 *mac);
int my_val2mac(s8 *mac, u8 *val);
int read_pid_file(char *file);

#define OMNICFG_MODE_DISABLE    0
#define OMNICFG_MODE_SNIFFER    1
#define OMNICFG_MODE_DIRECT     2
int is_directmode(void);

int mtdm_monitor_init(char *argv0);
int hotplug_block(char *type, char *action, char *name );
int save_diskinfo(void) ;
void hotplug_button(char *button, char *action, char *seen);
void mount_tf_and_usb(char *dev_type,char *action,char *dev_name);
void umount_tf_and_usb(char *dev_type,char *action,char *dev_name);

#endif

