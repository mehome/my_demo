
/**
 * @file ipc.c  IPC Relation Function
 *
 * @author  
 * @date   
 */

#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shutils.h" 
#include "linklist.h" 
#include "ipc.h" 

/********************************************************************************************************/
/***********************************************SEM UTILITY**********************************************/
/********************************************************************************************************/

/**
 * @define SEMDEBUG
 * @brief config of debug, default as disable
 */
#define SEMDEBUG    0

#if SEMDEBUG
LIST_HEAD(semRecord);

/**
 *  @struct semListNode
 *  @brief Node of each list.
 *
 */
typedef struct
{
    struct list_head list;
    int              semid;
    unsigned int     ownner;
}semListNode;
#endif

/**
 *  do "V" operation
 *
 *  @section Note
 *  Please use Macro ==> up(semid) to operate this function
 *
 *  @param semid    semaphore id
 *  @param *c1      file name
 *  @param d        line number
 *  @param *c2      function name
 *
 *  @return RET_OK for success and RET_ERR for other else.
 */
Ret upup(IN int semid, char *c1, int d, const char *c2, unsigned char print)
{
    struct sembuf sops;
    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=SEM_UNDO;

#if SEMDEBUG
    if (!list_empty(&semRecord)) {
        struct list_head *next;

        list_for_each(next,&semRecord) {
            semListNode *node = (semListNode *)list_entry(next, semListNode, list);

            if (node->semid == semid) {
                node->ownner = 0;
            }
        }
    }
    else {
        cprintf("==============================list_empty[%d]==============================\n",gettid());
    }
#endif

    if (semop(semid,&sops,1)!=0) {
        cprintf("%s:%d:::%s:::semid=%d up fail\n",c1,d,c2,semid);
        perror("sem up");
        return RET_ERR;
    }
    if (print)
        cprintf("[%d]%s:%d:::%s:::semid=%d up success\n",gettid(),c1,d,c2,semid);
    return RET_OK;
}

/**
 *  do "P" operation
 *
 *  @section Note
 *  Please use Macro ==> down(semid) to operate this function
 *
 *  @param semid    semaphore id
 *  @param *c1      file name
 *  @param d        line number
 *  @param *c2      function name
 *
 *  @return RET_OK for success and RET_ERR for other else.
 */
Ret downdown(IN int semid, char *c1, int d, const char *c2, unsigned char print)
{
    if (print)
        cprintf("[%d]%s:%d:::%s:::semid=%d down start\n",gettid(),c1,d,c2,semid);

#if SEMDEBUG
    if (!list_empty(&semRecord)) {
        struct list_head *next;

        list_for_each(next,&semRecord) {
            semListNode *node = (semListNode *)list_entry(next, semListNode, list);

            if (node->semid == semid) {
                if (node->ownner != 0 ) {
                    if (node->ownner == gettid()) {
                        cprintf("==============================Lock cycle======================== Hang!!\n");
                    }
                    else {
                        cprintf("==============================Lock by other pid %u======================== Waiting a while!!\n", node->ownner);
                    }
                }
            }
        }
    }
    else {
        cprintf("==============================list_empty[%d]==============================\n",gettid());
    }
#endif
    struct sembuf sops;
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=SEM_UNDO;
    if (semop(semid,&sops,1)!=0) {
        cprintf("%s:%d:::%s:::semid=%d down fail\n",c1,d,c2,semid);
        perror("sem down");
        return RET_ERR;
    }
#if SEMDEBUG
    if (!list_empty(&semRecord)) {
        struct list_head *next;

        list_for_each(next,&semRecord) {
            semListNode *node = (semListNode *)list_entry(next, semListNode, list);

            if (node->semid == semid) {
                node->ownner = gettid();
            }
        }
    }
    else {
        cprintf("==============================list_empty[%d]==============================\n",gettid());
    }
#endif
    if (print)
        cprintf("[%d]%s:%d:::%s:::semid=%d down end\n",gettid(),c1,d,c2,semid);

    return RET_OK;
}

/**
 *  create semaphore
 *
 *  @param key      semaphore key (IPC Key)
 *  @param *semid   when create success, you'll get a semaphore id in this field
 *
 *  @return RET_OK for success and RET_ERR for other else.
 */
Ret sem_create(IN int key, OUT int *semid)
{
    union{
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *_buf;
    }arg;

    if ((*semid=semget((key_t)key,1,IPC_CREAT|0666))==-1) {
        cprintf("key=%d semget fail\n",key);
        return RET_ERR;
    }
    else {
        arg.val=1;
        if (semctl(*semid,0,SETVAL,arg)==-1) {
            cprintf("semid=%d semctl fail\n",*semid);
            return RET_ERR;
        }
    }

#if SEMDEBUG
    semListNode *node = (semListNode *)calloc(sizeof(semListNode),1);

    if (node) {
        node->semid = *semid;
        node->ownner = 0;
        node->list.next = node->list.prev = &node->list;
        list_add_tail(&semRecord, &node->list);
        cprintf("add semid = %d\n", node->semid);
    }
#endif

    return RET_OK;
}

/**
 *  delete semaphore by semaphore id
 *
 *  @param semid    semaphore id
 *
 *  @return Always Return RET_OK.
 */
Ret sem_delete(int semid)
{
    semctl(semid,0,IPC_RMID);
#if SEMDEBUG
redo:
    if (!list_empty(&semRecord)) {
        struct list_head *next;

        list_for_each(next,&semRecord) {
            semListNode *node = (semListNode *)list_entry(next, semListNode, list);

            if (node->semid == semid) {
                list_del(&node->list);
                free(node);
                cprintf("del semid = %d\n", node->semid);
                goto redo;
            }
        }
    }
#endif
    return RET_OK;
} 

/********************************************************************************************************/
/***********************************************SHM UTILITY**********************************************/
/********************************************************************************************************/

/**
 *  create share memory
 *
 *  @param key      share memory key (IPC Key)
 *  @param size     share memory size that you want to create
 *  @param *shmid   when create success, you'll get a share memory id in this field
 *
 *  @return RET_OK for success and RET_ERR for other else.
 */
Ret shm_create(IN int key, IN unsigned int size, OUT int *shmid)
{
    if ((*shmid = shmget((key_t)key, size, 0666 | IPC_CREAT))==-1) {
        cprintf("key=%d shmget fail\n",key);
        return RET_ERR;
    }
    return RET_OK;
}

#if defined(OMNICFG_IPC_LOCK)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <signal.h>

#define OCFG_SHMSZ  16

static int ocfg_shmid = -1;
static unsigned char *ocfg_shm = NULL;
static int ocfg_semid = -1;
static volatile int *ocfg_pid = NULL;
static int ocfg_mypid;

static unsigned long long ocfg_sig_hdl_flag = 0;
static volatile unsigned long long ocfg_sig_rcv_flag = 0;

int omnicfg_ipc_connect(void)
{
    if(ocfg_pid)
        return 0;

    key_t key = ftok("/lib/wdk/omnicfg", 3000);

    if ((ocfg_semid = semget(key, 1, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((ocfg_shmid = shmget(key, OCFG_SHMSZ, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((ocfg_shm = shmat(ocfg_shmid, NULL, 0)) == (unsigned char *) -1)
        goto error;

    ocfg_pid = (int *) ocfg_shm;

    return 0;

error:
    return -1;
}

void omnicfg_ipc_init(void)
{
    memset(ocfg_shm, 0, OCFG_SHMSZ);

    semctl(ocfg_semid, 0, SETVAL, 1);
}

static int ocfg_ipc_unlock(void)
{
    struct sembuf sops[1];

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = 1;         /* Increment value by one */
    sops[0].sem_flg = 0;

retry:
    if (semop(ocfg_semid, sops, 1) < 0)
    {
       if((errno==EAGAIN)||(errno==EINTR))
           goto retry;
    }

    return 0;
}

static int ocfg_ipc_trylock(void)
{
    struct sembuf sops[1];

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = -1;
    sops[0].sem_flg = IPC_NOWAIT;

    if (semop(ocfg_semid, sops, 1) < 0)
    {
       if((errno==EAGAIN)||(errno==EINTR))
           return 1;
       else
           return -1;
    }

    return 0;
}

int omnicfg_shall_exit(void)
{
    if(ocfg_sig_rcv_flag)
    {
        ocfg_sig_rcv_flag = 0;
        return 1;
    }

    if(*ocfg_pid != ocfg_mypid)
        return 1;
    else
        return 0;
}

void ocfg_sig_reg(int sig)
{
    if (sig > 0)
        ocfg_sig_hdl_flag |= (1 << (sig - 1));
}

int ocfg_sig_not_reg(int sig)
{
    if (sig < 1)
        return 0;
    return (0 == (ocfg_sig_hdl_flag & (1 << (sig - 1))));
}

void ocfg_sig_handler(int sig)
{
    if (sig > 0)
        ocfg_sig_rcv_flag |= (1 << (sig - 1));
}

int omnicfg_lock(void)
{
    int ret;

    if(0 > omnicfg_ipc_connect())
        return -1;

    if(ocfg_sig_not_reg(SIGINT))
    {
        signal(SIGINT, ocfg_sig_handler);
        ocfg_sig_reg(SIGINT);
    }

    ocfg_mypid = getpid();
    *ocfg_pid = ocfg_mypid;

    while(1)
    {
        ret = ocfg_ipc_trylock();
        if(0==ret)
            break;

        if(0 > ret)
            return -1;

        usleep(100 * 1000);

        if(omnicfg_shall_exit())
            return -1;
    }

    return 0;
}

void omnicfg_unlock(void)
{
    ocfg_ipc_unlock();
    return;
}

#else

int omnicfg_shall_exit(void)
{
    return 0;
}

int omnicfg_lock(void)
{
    return 0;

}

void omnicfg_unlock(void)
{

}

#endif

