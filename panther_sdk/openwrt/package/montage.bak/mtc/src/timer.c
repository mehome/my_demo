
/**
 * @file timer.c   Timer Relation Function
 *
 * @author  
 * @date   
 */
#define _GNU_SOURCE
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include "shutils.h"
#include "linklist.h"
#include "ipc.h"

#include "include/mtdm.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/
/**
 * @def TIMERDEBUG
 * @brief The config of debug. default as closed.
 */
#define TIMERDEBUG      0
#define TIMERFUNCDEBUG  0

#if TIMERDEBUG
#define timer_lock(semid)   down_debug(semid)
#define timer_unlock(semid) up_debug(semid)
#else
#define timer_lock(semid)   down(semid)
#define timer_unlock(semid) up(semid)
#endif

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/
extern MtdmData *mtdm;

/**
 *  @enum operatorName
 *  @brief Operator code
 */
typedef enum
{
    timer_addTimer = 0, /**< Add timer */
    timer_delTimer,     /**< Delete timer */
    timer_modTimer,     /**< Modify timer */
    timer_reSchedule,   /**< Reschedule */
}operatorName;

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/
/**
 *  @brief Get absolute time in millisecond
 *  @param *currentTime when get time successed, you will get current time in this field.
 *
 *  @return non-zero for success and 0 for failed.
 */
unsigned long getCurrentTime(unsigned long *currentTime)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (currentTime)
        *currentTime = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}


static void sendNotifyToTimerThread(operatorName caller)
{
    int sig = caller;
reSend:
    if (send(mtdm->socket_pair[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
        if ( errno == EINTR || errno == EAGAIN ) {
            goto reSend;    /* try again */
        }
        cprintf("[%d]Could not send signal: %d %d %d\n", caller, sig, mtdm->socket_pair[1], errno);
    }
#if TIMERDEBUG
    cprintf("[%d]Send signal: %d\n", caller, sig);
#endif
}

static void *processTimerListThread(void *arg)
{
    struct list_head *pos, *n;
    mytimer_t *timer;

#if TIMERDEBUG || TIMERFUNCDEBUG
    cprintf("Thread Create[processTimerListThread]\n");
#endif

    for(;;) {
        timer_lock(mtdm_get_semid(semTimer));
        if (!list_empty(&mtdm->timerListExec)) {
            list_for_each_safe(pos, n, &mtdm->timerListExec) {
                list_move(pos, &mtdm->timerListExpired);
                break;
            }
        }
        else {
            timer_unlock(mtdm_get_semid(semTimer));
            goto exit;
        }
        timer_unlock(mtdm_get_semid(semTimer));

        timer = (mytimer_t *)list_entry(pos, mytimer_t, list);
#if TIMERFUNCDEBUG
        cprintf("Start run %p\n", timer->func);
        timer->func(timer->sData);
        cprintf("End   run %p\n", timer->func);
#else
        timer->func(timer->sData);
#endif

    }

exit:
    exit(0);
}

//call sem down before this func calling
static void checkTimerList(unsigned long now)
{
    struct list_head *pos, *n;

    if (list_empty(&mtdm->timerListRunning))
        return;

    list_for_each_safe(pos, n, &mtdm->timerListRunning) {
        mytimer_t *timer = (mytimer_t *)list_entry(pos, mytimer_t, list);

        if (time_after_eq(now, timer->expire)) {
            list_move(pos, &mtdm->timerListExec);
        }
    }

    return;
}

//call sem down before this func calling
static unsigned long checkNearestTimeout(unsigned long now)
{
    if (list_empty(&mtdm->timerListRunning)) {
        return 0;
    }

    struct list_head *pos;
    unsigned long nearest = now;
    unsigned char firstSet = FALSE;

    list_for_each(pos, &mtdm->timerListRunning) {
        mytimer_t *timer = (mytimer_t *)list_entry(pos, mytimer_t, list);

        if (firstSet) {
            if (time_after(nearest, timer->expire)) {
                nearest = timer->expire;
            }
        }
        else {
            firstSet= TRUE;
            nearest = timer->expire;
        }
    }

    return nearest;
}

//only using in timerThread()
static unsigned long reSchedule(struct timeval *timeout)
{
    unsigned long nextTimeout, now;

    now = getCurrentTime(NULL);

    timer_lock(mtdm_get_semid(semTimer));
    checkTimerList(now);
    nextTimeout = checkNearestTimeout(now);
    timer_unlock(mtdm_get_semid(semTimer));

    if (nextTimeout == 0)
        goto exit;

    if (now < nextTimeout) {
        nextTimeout = (nextTimeout - now);
    }
    else {
        nextTimeout = (nextTimeout + ~now);
    }

exit:
    if (timeout) {
        timeout->tv_usec = (nextTimeout % 1000) * 1000;
        timeout->tv_sec = nextTimeout / 1000;
    }
    return nextTimeout;
}

static void launchProcessTimerListThread(void)
{
    pid_t child;

    child = fork();
    if(child < 0)
    {
        return;
    }
    else if(child > 0)
    {
        return;
    }

    processTimerListThread(NULL);

    exit(0);
    return;
}

#if TIMERDEBUG
static unsigned int listLen(struct list_head *head)
{
    struct list_head *pos;
    unsigned int count = 0;

    if (list_empty(head)) {
        return count;
    }

    list_for_each(pos, head) {
        count++;
    }

    return count;
}

static void printCounter()
{
    cprintf("timerListRunning [%u]\n", listLen(&mtdm->timerListRunning));
    cprintf("timerListExpired [%u]\n", listLen(&mtdm->timerListExpired));
    cprintf("timerListExec    [%u]\n", listLen(&mtdm->timerListExec));
}
#endif

static void *timerThread(void)
{
#if TIMERDEBUG
    cprintf("Thread Create[timerThread]\n");
#endif

#if TIMERDEBUG
    cprintf("Timer Thread Running\n");
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret;

    for(;;) {
        fd_set lfdset;
        FD_ZERO(&lfdset);
        FD_SET(mtdm->socket_pair[0], &lfdset);

#if TIMERDEBUG
        cprintf("[For Loop] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif

        if ((timeout.tv_sec > 0) || (timeout.tv_usec > 0)) {
            //cprintf("Wait Select w/ %d seconds timeout\n", (int )timeout.tv_sec);
            ret = select(mtdm->socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*)&timeout);
            //cprintf("Exit Select\n");
        }
        else {
            //cprintf("Wait Select until signal\n");
            ret = select(mtdm->socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, NULL);
            //cprintf("Exit Select\n");
        }

        if(mtdm->terminate)
            break;

        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            }
            perror("Timer Select:");
        }

        if (ret == 0) {
            reSchedule(&timeout);
#if TIMERDEBUG
            cprintf("[Timeout ] next wake up after %d s %d us\n", (int )timeout.tv_sec, (int )timeout.tv_usec);
#endif
            launchProcessTimerListThread();
        }
        else {
            if (FD_ISSET(mtdm->socket_pair[0], &lfdset)) {
                int sig;
                safe_read(mtdm->socket_pair[0], &sig, sizeof(sig));    //don't care what we read
                reSchedule(&timeout);
#if TIMERDEBUG
                cprintf("[Signal  ] next wake up after %d s %d us\n", (int )timeout.tv_sec, (int)timeout.tv_usec);
#endif
                launchProcessTimerListThread();
            }
        }

#if TIMERDEBUG
        printCounter();
#endif
    }

    exit(0);
}

static void freeTimerList(struct list_head *head)
{
    if (list_empty(head))
        return;

    struct list_head *pos, *n;

    list_for_each_safe(pos, n, head) {
        list_move(pos, &mtdm->timerListExpired);
    }
}


/**
 *  @brief Destroy timer thread
 *
 *  @return None.
 */
static void destoryTimerThread()
{
    if (mtdm->timerThreadAlreadyLaunch) {
        timer_lock(mtdm_get_semid(semTimer));
        //cancel timer thread
        kill(mtdm->TimerThread_pid, SIGTERM);
        //lock list and move timerListRunning and timerListExec to timerListExpired
        freeTimerList(&mtdm->timerListRunning);
        freeTimerList(&mtdm->timerListExec);

        //release socketpair
        close(mtdm->socket_pair[0]);
        close(mtdm->socket_pair[1]);

        mtdm->timerThreadAlreadyLaunch = FALSE;
        timer_unlock(mtdm_get_semid(semTimer));
    }
}

/**
 *  @brief Add timer
 *  @param *timer Timer user defined.
 *  @return None.
 */
static void addTimer(mytimer_t *timer)
{
    timer_lock(mtdm_get_semid(semTimer));
    list_add_tail(&timer->list, &mtdm->timerListRunning);
    sendNotifyToTimerThread(timer_addTimer);
    timer_unlock(mtdm_get_semid(semTimer));
}

static void __delTimer(mytimer_t *timer)
{
    list_del(&timer->list);

    sendNotifyToTimerThread(timer_delTimer);
}

/**
 *  @brief Delete timer
 *  @param *timer Timer you want to delete. 
 *  @return None.
 */
static void delTimer(mytimer_t *timer)
{
    timer_lock(mtdm_get_semid(semTimer));
    __delTimer(timer);
    timer->list.next = timer->list.prev = &timer->list;
    timer_unlock(mtdm_get_semid(semTimer));
}

static void __modTimer(mytimer_t *timer, unsigned long expire)
{
    list_del(&timer->list);
    timer->expire = expire;
    list_add_tail(&timer->list, &mtdm->timerListRunning);

    sendNotifyToTimerThread(timer_modTimer);
}

/**
 *  @brief Modify timer
 *  @param *timer Timer you want to delete. 
 *  @param expire The new expire you want add to timer.
 *  @return None.
 */
void modTimer(mytimer_t *timer, unsigned long expire)
{
    timer_lock(mtdm_get_semid(semTimer));
    __modTimer(timer,expire);
    timer_unlock(mtdm_get_semid(semTimer));
}










/*=====================================================================================+
 | Functions for extern call                                                           |
 +=====================================================================================*/

int TimerInit(char *argv0)
{
    pid_t child;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, mtdm->socket_pair)!=0) {
        cprintf("can't create signal pipe\n");
        return -1;
    }

    child = fork();
    if(child < 0)
    {
        cprintf("[TimerInit Fail*****]\n");
        return -1;
    }
    else if(child > 0)
    {
        mtdm->TimerThread_pid = child;
        return 0;
    }

    memset(argv0, 0, strlen(argv0));
    strcpy(argv0, "timer");

    mtdm->timerThreadAlreadyLaunch = TRUE;

    timerThread();
    exit(0);

    return -1;
}

void TimerDeInit(void)
{
    destoryTimerThread();
}

mytimer_t *getTimerById(int timer_id, int timer_alloc)
{
    /* timerTest is reserved/unused */
    if((timer_id>0) && (timer_id<timerMax))
    {
        if(timer_alloc) 
        {
            mtdm->timer[timer_id].id = timer_id;
            return &mtdm->timer[timer_id];
        }
        else
        {
            if(timer_id == mtdm->timer[timer_id].id)
                return &mtdm->timer[timer_id];
        }
    }

    return NULL;
}

void TimerDel(int timer_id)
{
    mytimer_t *timer = getTimerById(timer_id, 0);

    if (timer) {
#if TIMERDEBUG
        cprintf("[TimerDel][%d][S]\n", timer_id);
        delTimer(timer);
        cprintf("[TimerDel][%d][E]\n", timer_id);
#else
        delTimer(timer);
#endif
    }
}

void TimerAdd(int timer_id, timer_func func, int sData, unsigned long second)
{
    unsigned long now = getCurrentTime(NULL);
    mytimer_t *timer;

    TimerDel(timer_id);

    if ((timer = getTimerById(timer_id, 1)) != NULL) {
        timer->list.next = timer->list.prev = &timer->list;
        timer->func   = func;
        timer->sData  = sData;
        timer->expire = now + (second * 1000);

#if TIMERDEBUG
        cprintf("[TimerAdd][%d][%lu][%lu][%lu][S]\n", timer_id, now, second, timer->expire);
        addTimer(timer);
        cprintf("[TimerAdd][%d][%lu][%lu][%lu][E]\n", timer_id, now, second, timer->expire);
#else
        addTimer(timer);
#endif
    }
    else {
        cprintf("Fail to [TimerAdd]!!\n");
    }
}

void TimerMod(int timer_id, unsigned long second)
{
    mytimer_t *timer = getTimerById(timer_id, 0);
    unsigned long now = getCurrentTime(NULL);

    if (timer) {
#if TIMERDEBUG
        cprintf("[TimerMod][%d][%lu][%lu][%lu][S]\n", timer_id, now, second, timer->expire);
        modTimer(timer, now + (second * 1000));
        cprintf("[TimerMod][%d][%lu][%lu][%lu][E]\n", timer_id, now, second, timer->expire);
#else
        modTimer(timer, now + (second * 1000));
#endif
    }
}

