#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define SPOTIFY_NAME     "mspotify"
#define SPOTIFY_CMDS     "/usr/bin/"SPOTIFY_NAME
#define SPOTIFY_PIDFILE  "/var/run/"SPOTIFY_NAME".pid"

static int start(void)
{
    char mac0[18] = "";
    char *bmac0 = (mtdm->boot).MAC0;
    int i, j = 0;

    for (i=0; i<strlen(bmac0) && i<18; i++) {
        if (isxdigit(bmac0[i])) {
            mac0[j] = bmac0[i];
            j++;
        }
    }
    start_stop_daemon(SPOTIFY_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, SPOTIFY_PIDFILE, 
        "Spotify@%s", mac0);

    return 0;
}

static int stop(void)
{
    if( f_exists(SPOTIFY_PIDFILE) ) {
        start_stop_daemon(SPOTIFY_CMDS, SSDF_STOP_DAEMON, 0, SIGTERM, SPOTIFY_PIDFILE, NULL);
        unlink(SPOTIFY_PIDFILE);
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int spotify_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

