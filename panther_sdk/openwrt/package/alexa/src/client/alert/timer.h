
/*
 * @file timer.h
 *
 * @author  
 * @date    
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif
#include "linklist.h"



/**This function be called when a timer expire*/
typedef void (*timer_func)(int sData, void *data);


typedef struct mytimer_st{
    struct list_head    list;
    timer_func          func;
    int                 sData;
    void                *data;  /*!< must can be free()!!!*/
    unsigned long       expire; /*!< jiffies */
}mytimer_t;


#define MAX_TIMER    10

#define PTHREAD_SET_UNATTACH_ATTR(attr) \
    pthread_attr_init(&attr); \
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)

/**
 * @define typecheck
 * @brief check type
 */
#define typecheck(type,x) \
({  type __dummy; \
    typeof(x) __dummy2; \
    (void)(&__dummy == &__dummy2); \
    1; \
})

/**
 * @define time_after(a, b)
 * @brief is b bigger than a
 */
#define time_after(a,b) \
    (typecheck(unsigned long, a) && \
     typecheck(unsigned long, b) && \
     ((long)(b) - (long)(a) < 0))
/**
 * @define time_before(a, b)
 * @brief is b smller than a
 */
#define time_before(a,b)    time_after(b,a)

/**
 * @define time_after_eq(a, b)
 * @brief is b bigger or equal than a
 */
#define time_after_eq(a,b) \
    (typecheck(unsigned long, a) && \
     typecheck(unsigned long, b) && \
     ((long)(a) - (long)(b) >= 0))
/**
 * @define time_before_eq(a, b)
 * @brief is b smller or equal than a
 */
#define time_before_eq(a,b) time_after_eq(b,a)



/**
 *  @brief Get current time. Must support jiffer export in /proc file system!!
 *  @param *currentTime when get time successed, you will get current time in this field.
 *
 *  @return non-zero for success and 0 for failed.
 */
unsigned int getCurrentTime(unsigned int *currentTime);

void TimerInit();
void TimerDeInit();

void TimerAdd(mytimer_t *timer);
void TimerDel(mytimer_t *timer);
void TimerMod(mytimer_t *timer, unsigned expire);
mytimer_t * newTimer(timer_func func, int sData, void *data, unsigned int second);
void freeTimer(mytimer_t * timer);

#endif

