#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "Alert.h"
#include "AlertManager.h"
#include "AlertScheduler.h"



enum {ALERT_SCHEDULER_UNACTIVE=0, ALERT_SCHEDULER_ACTIVE};


void alert_func(int sData, void *data)
{	
	AlertScheduler *scheduler = NULL;
	AlertManager   *manager   = NULL;
	Alert          *alert     = NULL;
	char 		   *token     = NULL;
	
	
	scheduler = (AlertScheduler *)data;
	manager   = alertscheduler_get_alertmanager(scheduler);
	alert     = alertscheduler_get_alert(scheduler);
  	token     = alert_get_token(alert);
		
	alertscheduler_set_active(scheduler,ALERT_SCHEDULER_ACTIVE);
	alertmanager_start_alert(manager, token);
	
}
AlertScheduler *alertscheduler_new(Alert *alert, AlertManager *handler)
{
	int ret ;
	unsigned long second; 
	AlertScheduler *alertScheduler = NULL;
		
	alertScheduler = calloc(sizeof(AlertScheduler), 1);
	if(NULL == alertScheduler) {
		error("malloc AlertScheduler failed");
		return NULL;
	}

	ret = pthread_mutex_init(&alertScheduler->aMutex,NULL);
		if(ret) {
			error("pthread_mutex_init failed");
			goto failed;
	}

	alertScheduler->alert = alert ;
	alertScheduler->active 	= ALERT_SCHEDULER_UNACTIVE;
	alertScheduler->handler = handler;
	
	#if 0
	second = alert_get_time_diff(alert);
	DEBUG_INFO("***************second:%ld\n",second);		
	alertScheduler->timer = newTimer(alert_func, 1,(void *)alertScheduler, second);
	if( NULL == alertScheduler->timer) {
		DEBUG_ERR("newTimer failed");
		goto failed1;
	}
	TimerAdd(alertScheduler->timer);
	#endif
	return alertScheduler;
	
failed1:
	pthread_mutex_destroy(&alertScheduler->aMutex);
failed:
	if(NULL!= alert) {
		alert_free(alert);
	}
	free(alertScheduler);
	alertScheduler = NULL;
	return NULL;
}

int alertscheduler_is_active(AlertScheduler * alertScheduler)
{
	int ret;	
	ret = (alertScheduler->active == ALERT_SCHEDULER_ACTIVE);
	return ret;
}


void alertscheduler_set_active(AlertScheduler * alertScheduler, int active)
{	

	alertScheduler->active = active;
}


Alert * alertscheduler_get_alert(AlertScheduler * alertScheduler)
{	
	return alertScheduler->alert;
}

AlertManager* alertscheduler_get_alertmanager(AlertScheduler * alertScheduler)
{	
	return alertScheduler->handler;
}


void alertscheduler_cancel(AlertScheduler * alertScheduler, AlertManager *handler)
{
	Alert *alert = NULL;
	char *token  = NULL;
	
	alert = alertscheduler_get_alert(alertScheduler);
	token = alert_get_token(alert);
	info("cancel token:%s",token);
	//if(alertscheduler_is_active(alertScheduler)) {
		alertscheduler_set_active(alertScheduler,ALERT_SCHEDULER_UNACTIVE);
		alertmanager_stop_alert(handler,token);
		
	//}
}



void alertscheduler_free(AlertScheduler * alertScheduler) 
{
	if(NULL != alertScheduler) {
#if 0
		if(NULL!= alertScheduler->timer) {
			//freeTimer(alertScheduler->timer);
			TimerDel(alertScheduler->timer);
		}
#else
		Alert *alert = alertScheduler->alert;	
		char *token = alert_get_token(alert);
		
		crontab_delete_alert(token);
        
#endif
		if(NULL != alertScheduler->alert) {
			alert_free(alertScheduler->alert);
		}
		pthread_mutex_destroy(&alertScheduler->aMutex);
		free(alertScheduler);
		alertScheduler = NULL;
	}
}



