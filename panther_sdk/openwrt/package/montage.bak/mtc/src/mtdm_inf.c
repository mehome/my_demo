/**
 * @file mtdm_inf.c
 *
 * @author  Frank Wang
 * @date    2015-10-06
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "include/mtdm.h"
#include "include/mtdm_types.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_event.h"
#include "include/mtdm_proc.h"
#include "include/mtdm.h"

#define MAX_CONNECTION_SUPPORT    32

extern MtdmData *mtdm;

struct InfJob;
typedef unsigned char (*job_write_callback)(void *priv, char *buf, int size);
typedef int (*job_close_callback)(void *priv, int dofree);

typedef struct
{
    struct list_head list;
    MtdmPkt packet;
    int sockfd;
    unsigned char thread:1;
    unsigned char reservd:7;
    job_write_callback jobwrite;
    job_close_callback jobclose;
}InfJob;

//threadpool_t *pool = NULL;
//static threadpool_t *cmd_pool = NULL;

//static int inf_sock = UNKNOWN;

static const char *opc_data_name[] = {
    [OPC_DAT_IDX(OPC_DAT_INFO)]       = "info",
    [OPC_DAT_IDX(OPC_DAT_MAX)]        = NULL
};

void sendData(OpCodeData opc, OpCodeDataArg arg, InfJob *job)
{
    if (opc > OPC_DAT_MAX || opc < OPC_DAT_MIN) {
        logger (LOG_ERR, "Execute Data(0x%x) out of range", opc);
    }
    else {
        logger (LOG_INFO, "Execute Data:[%s](0x%x)", opc_data_name[OPC_DAT_IDX(opc)], opc);
        switch(opc) {
            case OPC_DAT_INFO:
            {
                char info[MSBUF_LEN];

                snprintf(info, sizeof(info), "MAC0=%s\nMAC1=%s\nMAC2=%s\n", 
                    mtdm->boot.MAC0, mtdm->boot.MAC1, mtdm->boot.MAC2);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "LAN=%s\nWAN=%s\nWANB=%s\n", 
                    mtdm->rule.LAN, mtdm->rule.WAN, mtdm->rule.WANB);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "STA=%s\nAP=%s\nBRSTA=%s\nBRAP=%s\nIFPLUGD=%s\n", 
                    mtdm->rule.STA, mtdm->rule.AP, mtdm->rule.BRSTA, mtdm->rule.BRAP, mtdm->rule.IFPLUGD);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "LMAC=%s\nWMAC=%s\nSTAMAC=%s\n", 
                    mtdm->rule.LMAC, mtdm->rule.WMAC, mtdm->rule.STAMAC);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "HOSTAPD=%d\nWPASUP=%d\n", 
                    mtdm->rule.HOSTAPD, mtdm->rule.WPASUP);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "OMODE=%d\nWMODE=%d\n", 
                    mtdm->rule.OMODE, mtdm->rule.WMODE);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "WIFLINK=%d\n", mtdm->rule.WIFLINK);
                job->jobwrite(job, info, strlen(info));
            }
            break;

            default:
            {
                logger (LOG_ERR, "Execute Data(0x%x) unknown", opc);
            }
            break;

        }
    }
}

int socket_close(int sock)
{
    close(sock);
    return 0;
}

static unsigned char setup_inf_sock()
{
#if 0
    if (mtdm->inf_sock!=UNKNOWN) {
        socket_close(mtdm->inf_sock);
    }
#endif

    //create socket
    if ((mtdm->inf_sock=socket(AF_UNIX,SOCK_STREAM,0))<0) {
        return FALSE;
    }

    unlink(MTDMSOCK);

    struct sockaddr_un addr;
    bzero(&addr,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, MTDMSOCK);
    int len = offsetof(struct sockaddr_un, sun_path) + strlen(MTDMSOCK);

bind_again:
    if (bind(mtdm->inf_sock, (struct sockaddr *)&addr, len) < 0) {
        //perror("INF@bind:");
        my_sleep(1);
        goto bind_again;
    }

listen_again:
    if (listen(mtdm->inf_sock, MAX_CONNECTION_SUPPORT) < 0) {
        //perror("INF@listen:");
        my_sleep(1);
        goto listen_again;
    }

    logger (LOG_INFO, "Setup inf socket success");

    return TRUE;
}


static unsigned char jobRead(int sock, char *buf, int size)
{
    int ret;

    ret = safe_read(sock, buf, size);

    if(ret == -1)
        return FALSE;

    return TRUE;
}

static unsigned char jobWrite(void *priv, char *buf, int size)
{
    InfJob *job = (InfJob *)priv;
    int cfd = job->sockfd;
    return safe_write(cfd, buf, size);
}

static int jobClose(void *priv, int dofree)
{
    InfJob *job = (InfJob *)priv;
    int cfd = job->sockfd;

    if (cfd) {
        socket_close(cfd);
        job->sockfd = 0;
    }
    if (dofree) {
        if (job->packet.data != NULL)
            free(job->packet.data);
        free(job);
    }

    return 0;
}

static IfCode peek_job(void *arg)
{
    InfJob *job = arg;
    PHdr *head = &job->packet.head;

    return head->ifc;
}

static void exec_job(void *arg)
{
    InfJob *job = arg;
    PHdr *head = &job->packet.head;

    switch(head->ifc) {
        case INFC_KEY:    // interface for key
        {
            //stdprintf("Process an INF_KEY interface!!");
            //handle_inf_key(job->sockfd, &job->packet.head, job->packet.data);

            KeyCB key_cb;

            memset(&key_cb, 0, sizeof(KeyCB));
            key_cb.key = head->opc;
            key_cb.data = head->arg;
            job->jobclose(job, 0);
            sendEvent(EventKey, 0, &key_cb, sizeof(KeyCB), TRUE);
        }
        break;

        case INFC_CMD:    // interface for cmd
        {
            job->jobclose(job, 0);
            sendCmd(head->opc, head->arg, job->packet.data);
        }
        break;

        case INFC_DAT:    // interface for data
        {
            sendData(head->opc, head->arg, job);
        }
        break;

        default:
        {
            logger (LOG_ERR, "Process an unknown interface!!");
        }
        break;
    }

    job->jobclose(job, 1);
}

#if defined(__PANTHER__)
void infHandler(char *argv0)
{
    logger (LOG_INFO, "Thread Create[infHandler]");

    if (!setup_inf_sock()) {
        BUG_ON();
        return;
    }

    while (1) {
        InfJob *job;
        int cfd;

        if ((cfd = accept(mtdm->inf_sock, NULL, 0)) == -1) {

            if(mtdm->terminate)
                break;

            continue;
        }

        if ((job = calloc(1, sizeof(InfJob))) != NULL) {
            list_init(&job->list);
            if (jobRead(cfd, (char *)&job->packet, sizeof(PHdr))) {
                //threadpool_t *mypool = pool;
                int enq = 1;

                if (peek_job(job) == INFC_CMD) {
                    // INFC_CMD MUST be executed by en-queue order
                    //mypool = cmd_pool;
                }
                job->sockfd = cfd;
                job->jobwrite = jobWrite;
                job->jobclose = jobClose;
                fcntl(cfd, F_SETFD, FD_CLOEXEC);

                if (job->packet.head.len > 0) {   //recv data
                    job->packet.data = calloc(1, job->packet.head.len + 1);

                    if (!(job->packet.data && jobRead(cfd, job->packet.data, job->packet.head.len))) {
                        enq = 0;
                        logger (LOG_ERR, "Recv Fail 2, socket %d close", cfd);
                    }
                }
                if (enq) {
                    #if 0
                    if (task_enqueue(mypool, exec_job, job, 0) == threadpool_success) {
                        continue;
                    }
                    #endif
                    //logger (LOG_ERR, "Task Enqueue removed, DIY!");
                    exec_job(job);
                    continue;
                }
            }
            else {
                logger (LOG_ERR, "Recv Fail 1, socket %d close", cfd);
            }
            jobClose(job, 1);
        }
        else {    //no memory
            socket_close(cfd);
            logger (LOG_ERR, "OOM, socket %d close", cfd);
        }
    }

    logger (LOG_INFO, "end of infHandler");
}
#else
static void infHandler(void *arg)
{
    logger (LOG_INFO, "Thread Create[infHandler]");

    if (!setup_inf_sock()) {
        BUG_ON();
        return;
    }

    while (1) {
        InfJob *job;
        int cfd;

        if ((cfd = accept(inf_sock, NULL, 0)) == -1) {
            continue;
        }

        if ((job = calloc(1, sizeof(InfJob))) != NULL) {
            list_init(&job->list);
            if (jobRead(cfd, (char *)&job->packet, sizeof(PHdr))) {
                threadpool_t *mypool = pool;
                int enq = 1;

                if (peek_job(job) == INFC_CMD) {
                    // INFC_CMD MUST be executed by en-queue order
                    mypool = cmd_pool;
                }
                job->sockfd = cfd;
                job->jobwrite = jobWrite;
                job->jobclose = jobClose;
                fcntl(cfd, F_SETFD, FD_CLOEXEC);

                if (job->packet.head.len > 0) {   //recv data
                    job->packet.data = calloc(1, job->packet.head.len + 1);
    
                    if (!(job->packet.data && jobRead(cfd, job->packet.data, job->packet.head.len))) {
                        enq = 0;
                        logger (LOG_ERR, "Recv Fail 2, socket %d close", cfd);
                    }
                }
                if (enq) {
                    if (task_enqueue(mypool, exec_job, job, 0) == threadpool_success) {
                        continue;
                    }
                    logger (LOG_ERR, "Task Enqueue Fail, socket %d close", cfd);
                }
            }
            else {
                logger (LOG_ERR, "Recv Fail 1, socket %d close", cfd);
            }
            jobClose(job, 1);
        }
        else {    //no memory
            socket_close(cfd);
            logger (LOG_ERR, "OOM, socket %d close", cfd);
        }
    }

    logger (LOG_INFO, "end of infHandler");
}


int mtdm_init_inf(void)
{
    pool = threadpool_init(2, 10, PTHREAD_STACK_MIN / 1024);
    cmd_pool = threadpool_init(1, 10, PTHREAD_STACK_MIN / 1024);

    if ((pool == NULL) || (cmd_pool == NULL)) {
        BUG_ON();
        return FALSE;
    }

    srand((unsigned)(time(0))); 

    if (task_enqueue(pool, infHandler, NULL, 0)!=threadpool_success) {
        BUG_ON();
        return FALSE;
    }

    return TRUE;
}

int mtdm_destroy_inf(void)
{

#if 0
    /*
     * FIXME: need a good solution to break infHandler
     * accept/select can't receive SIG
     */
    if (cmd_pool) {
        threadpool_destroy(cmd_pool, 0);
    }
    if (pool) {
        threadpool_destroy(pool, 0);
    }
#endif

    return TRUE;
}

#endif

