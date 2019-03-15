#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>


#include "list.h"
#include "alert_api.h"
#include "Alert.h"
#include "Crontab.h"
#include "AlertManager.h"
#include "AlertScheduler.h"
#include "hashmap_itr.h"
#include "AlertsStatePayload.h"

static AlertManager *alertManager = NULL;
static pthread_mutex_t alertManagerMtx = PTHREAD_MUTEX_INITIALIZER; 

void    AlertAdd( char *token,  char *scheduledTime, char *repate, int type ,char *iprompt)
{

	info("alert_add token:%s scheduledTime:%s repate:%s type:%d",token, scheduledTime, repate, type);
	//alert_add token:turing_1540260437 scheduledTime:1540260437 repate:null type:0
	pthread_mutex_lock(&alertManagerMtx);
	assert(alertManager != NULL);

	if(alertmanager_has_alert(alertManager,token)) 
    {
		error("#######alertManager has %s",token);
		AlertScheduler *scheduler;
		Alert *alert;
		char *time   = NULL;
		char *pRepate = NULL;
		
		alertmanager_delete(alertManager, token);

	}
	Alert *alert = alert_new(token,scheduledTime, repate, type, iprompt);
	alertmanager_add(alertManager, alert,false);
    /* BEGIN: Added by Frog, 2018/6/1 */
    //闹钟信息写入文件后，重启crond,否则新添加的闹钟不会生效
    exec_cmd("/etc/init.d/crond restart");
    /* END:   Added by Frog, 2018/6/1 */
	pthread_mutex_unlock(&alertManagerMtx);
}

//把通过语音设置的闹钟删掉
int del_tmp_alert()
{
    
    char cmd[256]={0};
    snprintf(cmd,256,"sed -i '/\"%s/d' %s", "turing_152", "/etc/crontabs/root");
    exec_cmd(cmd);
}

void AlertInit()
{
	int ret;
	pthread_mutex_lock(&alertManagerMtx);
	
	alertManager = alertmanager_new();
	if(!alertManager) {
		pthread_mutex_unlock(&alertManagerMtx);
		error("alertmanager_new failed");
		exit(-1);
	}
	assert(alertManager != NULL);
	ret = alertmanager_schedulers(alertManager);
	if(ret <= 0)  {
		error("alertmanager_schedulers failed");
		pthread_mutex_unlock(&alertManagerMtx);
		exit(-1);
	}
    del_tmp_alert();
	crontab_load_from_disk(alertManager);
	pthread_mutex_unlock(&alertManagerMtx);
}
void AlertDeinit()
{
	pthread_mutex_lock(&alertManagerMtx);
	if(alertManager) {
		alertmanager_free(alertManager);
	}
	pthread_mutex_unlock(&alertManagerMtx);
}
void   AlertRemove(char *token)
{
	pthread_mutex_lock(&alertManagerMtx);
	if(alertManager) {
		alertmanager_delete(alertManager,token);
	}
	pthread_mutex_unlock(&alertManagerMtx);
}
void AlertFinshed()
{	
	pthread_mutex_lock(&alertManagerMtx);

	AlertHandler *handle = NULL;
	if(alertManager) {
		handle = alertmanager_get_alerthandler(alertManager);
		alerthandler_set_state(handle, ALERT_FINISHED);
	} 
	pthread_mutex_unlock(&alertManagerMtx);
}

void AlertInterrupted()
{	
	pthread_mutex_lock(&alertManagerMtx);

	AlertHandler *handler = NULL;
	if(alertManager) {
		handler = alertmanager_get_alerthandler(alertManager);
		int state = alerthandler_get_state(handler);
		if(state == ALERT_PLAYING)
			alerthandler_set_state(handler, ALERT_INTERRUPTED);
	} 
	pthread_mutex_unlock(&alertManagerMtx);
}

Alert *   GetAlert(char *token)
{
	Alert *alert = NULL;
	pthread_mutex_lock(&alertManagerMtx);
	if(alertManager) 
	{
		AlertScheduler *scheduler;
		scheduler = alertmanager_get_alertscheduler(alertManager,token);
		if(scheduler)
			alert = alertscheduler_get_alert(scheduler);
	}
	pthread_mutex_unlock(&alertManagerMtx);
	return alert;	
}

void AlertResume()
{
	pthread_mutex_lock(&alertManagerMtx);
	if(alertManager) 
		alerthandler_resume_alerts(alertManager);
	pthread_mutex_unlock(&alertManagerMtx);
	
}




