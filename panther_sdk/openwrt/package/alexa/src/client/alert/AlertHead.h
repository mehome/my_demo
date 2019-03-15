
#ifndef __ALERT_HEAD_H__
#define __ALERT_HEAD_H__

#include <stdlib.h>
#include <stdio.h>

#include "hashmap.h"
#include "hashset.h"
#include "linklist.h"
#include "list.h"


/**This function be called when a timer expire*/
typedef void (*timer_func)(int sData, void *data);


typedef struct mytimer_st{
    struct list_head    list;
    timer_func          func;
    int                 sData;
    void                *data;  /*!< must can be free()!!!*/
    unsigned long       expire; /*!< jiffies */
}mytimer_t;
typedef char * AlertType;

typedef enum {
	ALERT_PLAYING,
	ALERT_INTERRUPTED,
	ALERT_FINISHED,
}AlertState;

typedef struct alert_handler_st {
	volatile AlertState alertState;
	pthread_mutex_t aMutex;
	pthread_t tid;
	void *data;
}AlertHandler;


typedef struct REMINDER
{
	// 音频文件必须播放的顺序。该列表由assetIds组成。这里我们把多个列表看成一个文件一起播放
	char *assetPlayOrder; 

	// 必须播放的音频文件，如果backgroundAlertAsset没有包含在列表中，默认为Amazon提供的音频文件。
	char *backgroundAlertAsset;
	
	int loopCount;		  // 音频播放的次数

	int loopPauseInMilliseconds; // 音频循环时中间的间隔时间

}REMINDER_STR;

typedef struct alert_s{
	char *token;
	AlertType type;
	char *scheduledTime;

	REMINDER_STR m_reminder;
	int stop ;
	//struct timeval scheduledTime;
}Alert;

typedef struct alert_manager_s {
	AlertHandler *handler;
	map_t schedulers; //<alert token, AlterScheduler>  һ��token��Ӧһ��AlterScheduler�����ڶ�ʱ
	map_t activeAlerts;
	//hashset_t activeAlerts;  //active alerts token set
	pthread_t tid;
	pthread_mutex_t aMutex; //������

}AlertManager;


typedef struct alert_scheduler_st {
	Alert *alert;
	mytimer_t *timer;
	AlertManager *handler;
	pthread_mutex_t aMutex;
	int active;
}AlertScheduler;



typedef struct alerts_state_payload_st {
    list_t *allAlerts;
    list_t *activeAlerts;
} AlertsStatePayload;

#define ALARM_FILE  "/etc/conf/alarms.json"
#define SECONDS_AFTER_PAST_ALERT_EXPIRES  1800



#endif







