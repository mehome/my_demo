
/**
 * @file mtdm_fsm.c
 *
 * @author  Frank Wang
 * @date    2015-10-06
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "include/mtdm_misc.h"
#include "include/mtdm.h"

#include "include/mtdm_fsm.h"

static int fsm_pipe[2];

static fsm_private_t fsm_data;

fsm_mode mtdm_get_fsm_mode()
{
    mtdm_lock(semFSMData);
    fsm_mode ret = fsm_data.mode;
    mtdm_unlock(semFSMData);

    return ret;
}

void mtdm_set_fsm_mode(fsm_mode mode)
{
    mtdm_lock(semFSMData);
    fsm_data.mode = mode;
    mtdm_unlock(semFSMData);
}

static void mtdm_fsm_process(unsigned char isTimeout)
{
    if(fsm_data.mode == fsm_mode_stop)
        return;

    mtdm_lock(semFSMData);
    if(isTimeout) {
        if(fsm_data.mode != fsm_mode_idle)
            fsm_data.mode = fsm_mode_idle;
    }
    else {
        if(fsm_data.mode != fsm_mode_running) {
            fsm_data.mode = fsm_mode_running;
        }
    }
    mtdm_unlock(semFSMData);
}

static void mtdm_fsm_init()
{
    memset(&fsm_data, 0, sizeof(fsm_private_t));
    fsm_data.mode = fsm_mode_running;

    return;
}

void mtdm_fsm_schedule(fsm_event event)
{
    int reSendCount = 0;

reSend:
    if (reSendCount++>5) {
        stdprintf("Can't Send Event to Schedule\n");
        return;
    }

    if (send(fsm_pipe[1], &event, sizeof(event), MSG_DONTWAIT) < 0) {
        if ( errno == EINTR || errno == EAGAIN ) {
            goto reSend;    /* try again */
        }
        stdprintf("Could not send event: %d\n", event);
    }
}

void mtdm_fsm_boot()
{
    return;
}

void mtdm_fsm_stop()
{
    mtdm_set_fsm_mode(fsm_mode_stop);
    return;
}

void mtdm_fsm()
{
    mtdm_fsm_init();

    fd_set lfdset;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fsm_pipe)!=0) {
        BUG_ON();
        return;
    }

    while(fsm_data.mode != fsm_mode_stop) {
reSelect:
        FD_ZERO( &lfdset );
        FD_SET(fsm_pipe[0], &lfdset);

        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ret = select(fsm_pipe[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*)&timeout);

        if (ret < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                goto reSelect;    /* try again */
            }
        }

        if (ret == 0) {
            mtdm_fsm_process(TRUE);
            continue;
        }

        fsm_event sig[BUF_LEN_16];
        memset(sig,0,sizeof(sig));
        int getLen=safe_read(fsm_pipe[0], sig, sizeof(sig));

        if (getLen < 0) {
            continue;
        }

        int i;
        for (i=0;i<getLen/sizeof(fsm_event);i++) {
            if (sig[i]==fsm_boot) {
                mtdm_fsm_boot();
                goto reSelect;
            }
            else if (sig[i]==fsm_stop) {
                mtdm_fsm_stop();
                break;
            }
        }
        mtdm_fsm_process(FALSE);
    }

    return;
}

