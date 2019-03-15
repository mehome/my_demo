
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

/**This function be called when a timer expire*/
typedef void (*timer_func)(int sData);

typedef struct {
    struct list_head    list;
    int                 id;
    timer_func          func;
    int                 sData;
    unsigned long       expire; /*!< millisecond not jiffies */
}mytimer_t;

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
unsigned long getCurrentTime(unsigned long *currentTime);

int TimerInit(char *argv0);
void TimerDeInit(void);

void TimerAdd(int timer_id, timer_func func, int sData, unsigned long expire);
void TimerDel(int timer_id);
void TimerMod(int timer_id, unsigned long expire);

#endif

