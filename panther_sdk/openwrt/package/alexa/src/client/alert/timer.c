
/**
 * @file timer.c   Timer Relation Function
 *
 * @author  
 * @date   
 */

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include <pthread.h>

#include "shutils.h"

#include "ipc.h"
#include "timer.h"
#include "debug.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/
/**
 * @def TIMERDEBUG
 * @brief The config of debug. default as closed.
 */
//#define TIMERDEBUG      0
//#define TIMERFUNCDEBUG  0
//#define DEBUGTIMER 1

#if TIMERDEBUG
#define timer_lock(semid)   down_debug(semid)
#define timer_unlock(semid) up_debug(semid)
#else
#define timer_lock(semid)   down(semid)
#define timer_unlock(semid) up(semid)
#endif



typedef struct {
    int semid;
    pthread_t id;
}tThreadCb;

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/
static unsigned char timerThreadAlreadyLaunch = FALSE;
static int socket_pair[2];
static LINK_LIST_HEAD(timerListRunning);
static LINK_LIST_HEAD(timerListExpired);
static LINK_LIST_HEAD(timerListExec);
static LINK_LIST_HEAD(timerListAll);
static unsigned char createTimerThread(tThreadCb *cb);
static tThreadCb    tCb;

static mytimer_t fixTimerList[MAX_TIMER];

//#define ENABLE_NTP_LOG 

#ifdef ENABLE_NTP_LOG
extern int ntpSuccess;

int ntp_log(int ntpSuccess,unsigned long expire, char *params, char *func, int line) 
{
	char buf[1024] = {0};
	time_t timep;
	unsigned now =  getCurrentTime(NULL);
	memset(buf,0, 1024);
	snprintf(buf, 1024, "echo \"expire:%lu now:%lu\"  >> /tmp/alert.txt ", expire, now);	
	exec_cmd(buf);
	struct tm utc_tm; 
	struct tm *timenow; 
	time(&timep);
	timenow = localtime(&timep);	
	char *str = asctime(timenow);
	str[strlen(str)-1] = 0;

	char *strExpire;
	struct tm *p;
	p = localtime(&expire);
	strExpire =  asctime(p);
	strExpire[strlen(strExpire)-1] = 0;

	memset(buf,0, 1024);
	snprintf(buf, 1024, "echo \"expire:%s now:%s\"  >> /tmp/alert.txt ", str, strExpire);	
	exec_cmd(buf);
	
	memset(buf,0, 1024);
	if(ntpSuccess) {
		snprintf(buf, 1024, "echo \"ntpSuccess wait[%d] %s\"  >> /tmp/alert.txt ",  expire-now, params);
	}
	else 
		snprintf(buf, 1024, "echo \"wait [%d]  %s\"  >> /tmp/alert.txt ",  expire-now, params);
	exec_cmd(buf);
}


#endif
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

static void sendNotifyToTimerThread(operatorName caller)
{
    int sig = caller;
reSend:
    if (send(socket_pair[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
        if ( errno == EINTR || errno == EAGAIN ) {
            goto reSend;    /* try again */
        }
        cprintf("[%d]Could not send signal: %d\n", caller, sig);
    }
#if TIMERDEBUG
    cprintf("[%d]Send signal: %d\n", caller, sig);
#endif
}

static void *processTimerListThread(void *arg)
{
    struct list_head *pos, *n;
    mytimer_t *timer;
	DEBUG_INFO("processTimerListThread start....\n");	
#if TIMERDEBUG || TIMERFUNCDEBUG
    cprintf("Thread Create[processTimerListThread]\n");
#endif

    for(;;) {
        timer_lock(tCb.semid);
        if (!list_empty(&timerListExec)) {
            list_for_each_safe(pos, n, &timerListExec) {
                list_move(pos, &timerListExpired);
                break;
            }
        }
        else {
            timer_unlock(tCb.semid);
            goto exit;
        }
        timer_unlock(tCb.semid);

        timer = (mytimer_t *)list_entry(pos, mytimer_t, list);
		DEBUG_INFO("timer->expire:%ld\n",timer->expire);	
#if TIMERFUNCDEBUG
        cprintf("Start run %p\n", timer->func);
        timer->func(timer->sData, timer->data);
        cprintf("End   run %p\n", timer->func);
#else
        timer->func(timer->sData, timer->data);
#endif

    }

exit:
    pthread_exit(0);
}

//call sem down before this func calling
static void checkTimerList(unsigned long now)
{
    struct list_head *pos, *n;

    if (list_empty(&timerListRunning))
        return;

    list_for_each_safe(pos, n, &timerListRunning) {
        mytimer_t *timer = (mytimer_t *)list_entry(pos, mytimer_t, list);

		if(time_after_eq(now - 30*60, timer->expire)) {
#ifdef ENABLE_NTP_LOG
			ntp_log(ntpSuccess, timer->expire, "add to timerListExpired ", __func__, __LINE__);
#endif
			list_move(pos, &timerListExpired);
		
			continue;
		}

        if (time_after_eq(now,timer->expire)) {
            list_move(pos, &timerListExec);
#ifdef ENABLE_NTP_LOG
			ntp_log(ntpSuccess,timer->expire,  "add to timerListExec ", __func__, __LINE__);
#endif
        }
			
    }

    return;
}

//call sem down before this func calling
static unsigned int checkNearestTimeout(unsigned int now)
{
    if (list_empty(&timerListRunning)) {
        return 0;
    }

    struct list_head *pos;
    unsigned long nearest = now;
    unsigned char firstSet = FALSE;

    list_for_each(pos, &timerListRunning) {
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
static unsigned int reSchedule()
{
    unsigned long nextTimeout, now;

    now = getCurrentTime(NULL);

    timer_lock(tCb.semid);
    checkTimerList(now);
    nextTimeout = checkNearestTimeout(now);
    timer_unlock(tCb.semid);
	DEBUG_INFO("nextTimeout:%ld\n",nextTimeout);	
#ifdef ENABLE_NTP_LOG
		ntp_log(ntpSuccess,nextTimeout,  "add to nextTimeout", __func__, __LINE__);
#endif

    if (nextTimeout == 0)
        goto exit;

    if (now < nextTimeout) {
        nextTimeout = (nextTimeout - now);
    }
    else {
        nextTimeout = (nextTimeout + ~now);
    }

exit:
    return nextTimeout;
}

static void launchProcessTimerListThread()
{
    pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);

    //pthread_attr_setstacksize(&a_thread_attr, 512*1024);

    if(pthread_create(&a_thread, &a_thread_attr, processTimerListThread, NULL)!=0) {
        return;
    }

    pthread_attr_destroy(&a_thread_attr);
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
    cprintf("timerListRunning [%u]\n", listLen(&timerListRunning));
    cprintf("timerListExpired [%u]\n", listLen(&timerListExpired));
    cprintf("timerListExec    [%u]\n", listLen(&timerListExec));
}
#endif

static void *timerThread(void *arg)
{
	DEBUG_INFO("timerThread start...");
#if TIMERDEBUG
    cprintf(LOG_INFO, "Thread Create[timerThread]");
#endif

    if (!(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)==0 && pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0)) {
        pthread_exit(0);
    }

#if TIMERDEBUG
    cprintf(LOG_INFO, "Timer Thread Running");
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret;

    for(;;) {
        fd_set lfdset;
        FD_ZERO(&lfdset);
        FD_SET(socket_pair[0], &lfdset);

#if TIMERDEBUG
        cprintf("[For Loop] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif

        if (timeout.tv_sec > 0) {
            //cprintf("Wait Select w/ %d seconds timeout\n", (int )timeout.tv_sec);
            ret = select(socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*)&timeout);
            //cprintf("Exit Select\n");
        }
        else {
            //cprintf("Wait Select until signal\n");
            ret = select(socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, NULL);
            //cprintf("Exit Select\n");
        }

        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            }
            perror("Timer Select:");
        }

        if (ret == 0) {
            timeout.tv_sec = reSchedule();
			DEBUG_INFO("timeout.tv_sec:%ld\n" ,timeout.tv_sec);	
#if TIMERDEBUG
            cprintf("[Timeout ] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif
            launchProcessTimerListThread();
        }
        else {
            if (FD_ISSET(socket_pair[0], &lfdset)) {
                int sig;
                read(socket_pair[0], &sig, sizeof(sig));    //don't care what we read
                timeout.tv_sec = reSchedule();
#if TIMERDEBUG
                cprintf("[Signal  ] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif
                launchProcessTimerListThread();
            }
        }

#if TIMERDEBUG
        printCounter();
#endif
    }

    pthread_exit(0);
}

/**
 *  @brief Create timer thread. Only one timer thread can be create in the one process.
 *  @param *cb Must give a valid semaphore id. 
 *
 *  @return TRUE for success and FALSE for failed.
 */
static unsigned char createTimerThread(tThreadCb *cb)
{
	DEBUG_INFO("createTimerThread start...");
    if (timerThreadAlreadyLaunch)   //Fix Me: racing issue!?
        return FALSE;

    memcpy(&tCb, cb, sizeof(tThreadCb));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair)!=0) {
        cprintf("can't create signal pipe\n");
        return FALSE;
    }

    pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);

    if(pthread_create(&a_thread, &a_thread_attr, timerThread, (void *)&tCb)!=0) {
        return FALSE;
    }

    pthread_attr_destroy(&a_thread_attr);
    timerThreadAlreadyLaunch = TRUE;

    tCb.id = a_thread;

    return TRUE;
}

static void freeTimerList(struct list_head *head)
{
    if (list_empty(head))
        return;

    struct list_head *pos, *n;

    list_for_each_safe(pos, n, head) {
        list_move(pos, &timerListExpired);
    }
}


/**
 *  @brief Destroy timer thread
 *
 *  @return None.
 */
static void destoryTimerThread()
{
    if (timerThreadAlreadyLaunch) {
        timer_lock(tCb.semid);
        //cancel timer thread
        pthread_cancel(tCb.id);
        //lock list and move timerListRunning and timerListExec to timerListExpired
        freeTimerList(&timerListRunning);
        freeTimerList(&timerListExec);

        //release socketpair
        close(socket_pair[0]);
        close(socket_pair[1]);
        timer_unlock(tCb.semid);
    }
}

/**
 *  @brief Add timer
 *  @param *timer Timer user defined.
 *  @return None.
 */
static void addTimer(mytimer_t *timer)
{	  
	timer_lock(tCb.semid);
    list_add_tail(&timer->list, &timerListRunning);
    sendNotifyToTimerThread(timer_addTimer);
    timer_unlock(tCb.semid);
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
    timer_lock(tCb.semid);
    __delTimer(timer);
    timer->list.next = timer->list.prev = &timer->list;
	freeTimer(timer);
    timer_unlock(tCb.semid);
}

static void __modTimer(mytimer_t *timer, unsigned int expire)
{
    list_del(&timer->list);
    timer->expire = expire;
    list_add_tail(&timer->list, &timerListRunning);

    sendNotifyToTimerThread(timer_modTimer);
}

/**
 *  @brief Modify timer
 *  @param *timer Timer you want to delete. 
 *  @param expire The new expire you want add to timer.
 *  @return None.
 */
void modTimer(mytimer_t *timer, unsigned int expire)
{
    timer_lock(tCb.semid);
    __modTimer(timer,expire);
    timer_unlock(tCb.semid);
}




mytimer_t * newTimer(timer_func func, int sData, void *data, unsigned int second)
{
	unsigned int now = getCurrentTime(NULL);
	mytimer_t * timer = (mytimer_t * )calloc(sizeof(mytimer_t), 1);
	if(	NULL == timer ) {
		DEBUG_ERR("malloc newTimer failed\n");
		return NULL;
	}
	timer->data   = data;
    timer->func   = func;
    timer->sData  = sData;
    timer->expire = now + second;
	DEBUG_INFO("newTimer add expire :%d", second);
#ifdef ENABLE_NTP_LOG
	ntp_log(ntpSuccess, timer->expire,  "newTimer", __func__, __LINE__);
#endif
	return timer;
}

void freeTimer(mytimer_t *timer)
{
	if(timer->data) {
		free(timer->data);
		timer->data = NULL;
	}
	if(timer) {
		free(timer);
		timer = NULL;
	}
}




/*=====================================================================================+
 | Functions for extern call                                                           |
 +=====================================================================================*/

/**
 *  @brief Get absolute time in second
 *  @param *currentTime when get time successed, you will get current time in this field.
 *
 *  @return non-zero for success and 0 for failed.
 */
unsigned int getCurrentTime(unsigned int *currentTime)
{
    struct timespec ts;
	time_t now;
	#if 0
    clock_gettime(CLOCK_MONOTONIC, &ts);
	#else
	time(&now);
	#endif
   // if (currentTime)
    //    *currentTime = ts.tv_sec;
	if (currentTime)
		 *currentTime = now;
	//return ts.tv_sec;
    return now;
}

void TimerInit(int semid)
{
    tThreadCb t;
    int i;
	
    if(semid != -1) {
        t.semid = semid;
	
        createTimerThread(&t);
    }
    else {
        cprintf("[TimerInit Fail*****]\n");
    }
}

void TimerDeInit(void)
{
    destoryTimerThread();
}

void TimerDel(mytimer_t *timer)
{
#if TIMERDEBUG
        cprintf("[TimerDel][S]\n");
        delTimer(timer);
        cprintf("[TimerDel][E]\n");
#else
        delTimer(timer);
#endif
}

void TimerAdd( mytimer_t *timer)

{
    unsigned int now = getCurrentTime(NULL);
  
#if DEBUGTIMER
        cprintf("[TimerAdd][%u][%lu][S]\n",  now, timer->expire);
        addTimer(timer);
        cprintf("[TimerAdd][%u][%lu][E]\n", now,  timer->expire);
#else
        addTimer(timer);
#endif

}

void TimerMod(mytimer_t *timer, unsigned int second)
{
    unsigned int now = getCurrentTime(NULL);

#if TIMERDEBUG
        cprintf("[TimerMod][%u][%u][%lu][S]\n" , now, second, timer->expire);
        modTimer(timer, now + second);
        cprintf("[TimerMod][%u][%u][%lu][E]\n", now, second, timer->expire);
#else
        modTimer(timer, now + second);
#endif

}

