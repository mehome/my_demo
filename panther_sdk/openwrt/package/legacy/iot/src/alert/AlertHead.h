
#ifndef __ALERT_HEAD_H__
#define __ALERT_HEAD_H__

#include <stdlib.h>
#include <stdio.h>

#include "hashmap.h"
#include "hashset.h"
#include "list.h"





//typedef char * AlertType;

typedef enum {
	ALERT_PLAYING,
	ALERT_INTERRUPTED,
	ALERT_FINISHED,
}AlertState;

typedef enum {
	ALERT_TYPE_ALARM = 0,
	ALERT_TYPE_SLEEP,
	ALERT_TYPE_WAKEUP,
	ALERT_TYPE_TIMEOFF,
}AlertType;

typedef struct alert_handler_st {
	volatile AlertState alertState;
	pthread_mutex_t aMutex;
	pthread_t tid;
}AlertHandler;


typedef struct alert_s{
	char *token;
	//int id;
	char *scheduledTime;
	char *repate;
	int type;
	char *ptone;
}Alert;

typedef struct alert_manager_s {
	AlertHandler *handler;
	map_t schedulers; //
	map_t activeAlerts;
	//hashset_t activeAlerts;  //active alerts token set
	pthread_t tid;
	pthread_mutex_t aMutex; 

}AlertManager;


typedef struct alert_scheduler_st {
	Alert *alert;
	//mytimer_t *timer;
	AlertManager *handler;
	pthread_mutex_t aMutex;
	int active;
}AlertScheduler;



typedef struct alerts_state_payload_st {
    list_t *allAlerts;
    list_t *activeAlerts;
} AlertsStatePayload;

#define SECONDS_AFTER_PAST_ALERT_EXPIRES  1800

#define ALARM_FILE  "/etc/crontabs/root"



#endif







