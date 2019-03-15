/*
 * =====================================================================================
 *
 *       Filename:  shairport.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/25/2016 05:20:17 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

#define BUF_SIZE 512

#if defined(CONFIG_PACKAGE_shairport_sync_openssl)
#define DAEMON_BIN "/usr/bin/shairport-sync"
#else
#define DAEMON_BIN "/usr/sbin/shairport"
#endif
#define DAEMON_PID_FILE "/var/air.pid"

typedef struct {
    const char *callname;
    int (*exec_main)(int argc, char *argv[]);
} montage_rc_init_t;

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

extern MtdmData *mtdm;

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

static int shairport_stop(void)
{
    cdb_set("$air_nocheck", "1");

    #if defined(CONFIG_PACKAGE_shairport_sync_openssl)
    system("/usr/bin/shairport-sync -k");
    #else
    if(f_exists(DAEMON_PID_FILE) ) {
        start_stop_daemon(DAEMON_BIN, SSDF_STOP_DAEMON, 0, SIGKILL, DAEMON_PID_FILE, NULL);
        unlink(DAEMON_PID_FILE);
    }
    #endif

        return 0;
}

static int start(void)
{
    int  ra_func = cdb_get_int("$ra_func", 0);
    char shairport_args[BUF_SIZE];
    char ra_name[BUF_SIZE];
    char buffer[4];
    char tmp[BUF_SIZE];
    unsigned char val[6];
    char *ptr;

    shairport_stop();

    if ((ra_func != raFunc_DLNA_AIRPLAY) && (ra_func != raFunc_ALL_ENABLE)) {
        MTDM_LOGGER(this, LOG_INFO, "shairport: ignore -- ra_func=[%d]", ra_func);
        return 0;
    }

    cdb_get_str("$ra_name", ra_name, sizeof(ra_name), "");
    if ((ptr = strstr(ra_name, "_%6x"))) {
        boot_cdb_get("mac0", tmp);
        sscanf(tmp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
        sprintf(++ptr, "%2.2x%2.2x%2.2x", val[3], val[4], val[5]);
        cdb_set("$ra_name", ra_name);
    }
    if (strlen(ra_name) <= 0) {
        strcpy(ra_name, "AirPort");
    }

    cdb_get_str("$ra_air_buf", buffer, sizeof(buffer), "");
    if (strlen(buffer) <= 0) {
        strcpy(buffer, "256");
    }

    // -a : name
    // -b : buffer
    // -p : password
    // -o : port
#if defined(CONFIG_PACKAGE_shairport_sync_openssl)
    sprintf(shairport_args, "-d -a %s -- -d default -c All", ra_name);
#else
    sprintf(shairport_args, "-d -a %s -b %s -P %s -- -t hardware -d media -c Media",
            ra_name, buffer, DAEMON_PID_FILE);
#endif
    start_stop_daemon(DAEMON_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL,
        "%s", shairport_args);
    MTDM_LOGGER(this, LOG_INFO, "shairport: [%s %s] [%s]", DAEMON_BIN, DAEMON_PID_FILE, shairport_args);

    cdb_set("$air_nocheck", "0");

    return 0;
}

static int stop(void)
{
    return shairport_stop();
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int shairport_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

