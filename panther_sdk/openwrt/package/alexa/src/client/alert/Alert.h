#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>


#include "debug.h"
#include "AlertHead.h"
#if 0
//typedef char * AlertType;

typedef struct alert_s{
	char *token;
	AlertType type;
	char *scheduledTime;
	//struct timeval scheduledTime;
}Alert;
#endif
 extern time_t get_now_time();

 /* create a Alert  instance */
extern  Alert * alert_new(Alert *pAlert);


extern char *alert_get_token(Alert *alert);

extern char *alert_get_type(Alert *alert);

extern char *alert_get_scheduledtime(Alert *alert);

extern char *alert_get_assetPlayOrder(Alert *alert);

extern char *alert_get_backgroundAlertAsset(Alert *alert);

extern int alert_get_loopCount(Alert *alert);

extern int alert_get_loopPauseInMilliseconds(Alert *alert);



extern time_t alert_convert_scheduledtime(Alert *alert);

extern void alert_convert_scheduledtime_tm(Alert *alert, struct tm **time);

extern time_t get_now_time();


extern int alert_equals(Alert *alert, Alert *otherAlert);

extern time_t iso8601_to_UTC(char *scheduledTime);


/* free  Alert   */
extern void alert_free(Alert *alert);
extern time_t get_now_time_utc();

extern int alert_get_stop(Alert *alert);
void alert_set_stop(Alert *alert, int stop);



#endif
