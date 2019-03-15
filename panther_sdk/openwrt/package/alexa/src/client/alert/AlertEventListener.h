#ifndef __ALERT_EVENT_LISTENER_H__
#define __ALERT_EVENT_LISTENER_H__

#include <stdlib.h>
#include "debug.h"

#define ALERT_STARTED_EVENT   			0
#define ALERT_ENTERED_BACKGROUND_EVENT	1
#define ALERT_ENTERED_FOREGROUND_EVENT	2
#define ALERT_SET_EVENT		 3
#define ALERT_STOP_EVENT		 4
#define ALERT_DELETE_EVENT		 5



extern void alerteventlistener_on_alert_started(char * alertToken);

extern void alerteventlistener_on_alert_set(char * alertToken, int success);

extern void alerteventlistener_on_alert_delete(char * alertToken,int success);

extern void alerteventlistener_on_alert_stopped(char * alertToken);


#endif




