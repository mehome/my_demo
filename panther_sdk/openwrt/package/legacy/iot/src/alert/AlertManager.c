#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include "Alert.h"
#include "AlertHandler.h"
#include "AlertScheduler.h"
#include "AlertsStatePayload.h"

#include "AlertManager.h"
#include "hashmap_itr.h"
#include "hashset_itr.h"
#include "Crontab.h"
#include "myutils/debug.h"

char *iPrompt = NULL;
static void active_alerts_remove(map_t activeAlerts, char *alertToken)
{
	hashmap_remove(activeAlerts,alertToken);
}


static void      scheduers_remove(map_t *schedulers, char *alertToken)
{
	AlertScheduler *scheduler = NULL;
	int err = hashmap_get(schedulers, alertToken, (void **)&scheduler);
	hashmap_remove(schedulers, alertToken);
	if(err == MAP_OK && scheduler) {
		alertscheduler_free(scheduler);
	}
}


// toke
AlertManager *alertmanager_new(void)
{
	int ret;
	AlertManager *alertManager = NULL;
	alertManager= (AlertManager *)calloc(1, sizeof(AlertManager));
	if(NULL == alertManager) {
		error("malloc for AlertManager failed");
		return NULL;
	}	

	alertManager->handler = alerthandler_new();
	if(NULL == alertManager->handler) {
		error("alerthandler_new failed");
		goto failed;
	}

	alertManager->schedulers = hashmap_new();
	if(NULL == alertManager->schedulers) {
		error("hashmap_new failed");
		goto failed1;
	}

	alertManager->activeAlerts = hashmap_new();

	if(NULL == alertManager->schedulers) {
		error("hashmap_new failed");
		goto failed2;
	}

	ret = pthread_mutex_init(&alertManager->aMutex,NULL);
	if(ret) {
		error("pthread_mutex_init failed");
		goto failed3;
	}	
	return alertManager;
failed3:
	hashmap_free(alertManager->activeAlerts);
failed2:
	hashmap_free(alertManager->schedulers);
failed1:
	alerthandler_free(alertManager->handler);
failed:
	free(alertManager);
	alertManager = NULL;
	return NULL;
		
}


AlertScheduler *alertmanager_get_alertscheduler(AlertManager *alertManager, char *alertToken)
{
	int err;
	AlertScheduler *scheduler = NULL;
	//pthread_mutex_lock(&alertManager->aMutex);
	err = hashmap_get(alertManager->schedulers, alertToken, (void**)(&scheduler));
	if(err != MAP_OK)
		scheduler = NULL;
	//pthread_mutex_unlock(&alertManager->aMutex);

	return scheduler;
}

int alertmanager_has_alert(AlertManager *alertManager, char *alertToken)
{
	int ret ;
	//pthread_mutex_lock(&alertManager->aMutex);
	ret = hashmap_is_member(alertManager->schedulers, alertToken);
	//pthread_mutex_unlock(&alertManager->aMutex);
	return !ret;
}

int alertmanager_has_activealerts(AlertManager *alertManager)
{
	int ret;

	//pthread_mutex_lock(&alertManager->aMutex);	
	ret= hashmap_length(alertManager->schedulers) > 0;
	//pthread_mutex_unlock(&alertManager->aMutex);
	
	return ret;
}

map_t alertmanager_get_activealerts(AlertManager *alertManager)
{
	map_t activeAlerts = NULL;

	//pthread_mutex_lock(&alertManager->aMutex);
	activeAlerts = alertManager->activeAlerts;
	//pthread_mutex_unlock(&alertManager->aMutex);

	return activeAlerts;
}

map_t alertmanager_get_schedulers(AlertManager *alertManager)
{
	map_t schedulers = NULL;


	//pthread_mutex_lock(&alertManager->aMutex);

	schedulers = alertManager->schedulers;

	//pthread_mutex_unlock(&alertManager->aMutex);


	return schedulers;
}

list_t * alertmanager_get_allalerts(AlertManager *alertManager)
{
	int  len;

	list_t *list = list_new();
	AlertScheduler *scheduler = NULL; 
	
	AlertScheduler *schedulers = NULL;
	//pthread_mutex_lock(&alertManager->aMutex);
	schedulers = alertManager->schedulers;

	hashmap_itr_t itr = hashmap_iterator(schedulers);

	len = hashmap_length(schedulers);
	info("len hashmap_length :%d", len);
	while(hashmap_iterator_has_next(itr)) {
		Alert *alert = NULL;	
		hashmap_iterator_next(itr);
	
		scheduler = hashmap_iterator_value(itr);
		if(NULL != scheduler) {
			alert = alertscheduler_get_alert(scheduler);
			if (alert != NULL) {
				info("list add %s", alert_get_token( alert));
				info("list add iprompt :%s",strdup(alert->ptone));
				list_add(list, alert);
			}
		}

	}
	//pthread_mutex_unlock(&alertManager->aMutex);
	hashmap_iterator_free(itr);
	return list;
}


AlertHandler *alertmanager_get_alerthandler(AlertManager *alertManager)
{
	return alertManager->handler;
}


void alertmanager_drop(Alert *alert) 
{
	//alerteventlistener_on_alert_stopped(alert_get_token(alert));
}

/*  return 0 is ok */
int alertmanager_write_current_alerts_to_disk(AlertManager * alertManager)
{

	int ret;
	list_t *all = NULL;
	list_iterator_t *it = NULL ;
	
	//pthread_mutex_unlock(&alertManager->aMutex);
	all = alertmanager_get_allalerts(alertManager);
//	pthread_mutex_lock(&alertManager->aMutex);
	it = list_iterator_new(all,LIST_HEAD);
	while(list_iterator_has_next(it)) {
			char *token = NULL ;
			Alert *alert = NULL; 
			alert			=  list_iterator_next_value(it);
			if(alert != NULL) {
					token  =  alert_get_token(alert);
			}
	}

	info("all list length : %d ", list_length(all));

#if 1
	ret = crontab_wirte_to_disk(all);
#else
	ret = crontab_wirte_to_disk(alertManager);
#endif
	if(it) {
		list_iterator_destroy(it);
		it = NULL;
	}
	if(all) {
		list_destroy(all);
		all = NULL;
	}

	return ret;
}



static void hashmap_print( map_t map) 
{
	int len ;
	AlertScheduler *scheduler;
	hashmap_itr_t itr = hashmap_iterator(map);
	len = hashmap_length(map);
	info(" map len:%d" ,len);
	while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			scheduler = (AlertScheduler *)hashmap_iterator_value(itr);
			if(scheduler != NULL) {
				Alert *alert = NULL ;
				alert = alertscheduler_get_alert(scheduler);
				if(alert != NULL)
					info("token:%s" ,alert_get_token(alert));
				
			}
	}
	if(itr)
		hashmap_iterator_free(itr);
}

static void hashmap_print_key( map_t map) 
{
	int len ;
	char *key;
	hashmap_itr_t itr = hashmap_iterator(map);
	len = hashmap_length(map);
	info("len:%d" ,len);
	while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			key = hashmap_iterator_key(itr);
			info("key:%s" ,key);
	}
	if(itr)
		hashmap_iterator_free(itr);
}



AlertsStatePayload * alertmanager_get_state(AlertManager * alertManager)
{
	int len;
	list_t *all = list_new();
	list_t *active = list_new();

	//pthread_mutex_lock(&alertManager->aMutex);
	hashmap_itr_t itr = hashmap_iterator(alertManager->schedulers);;
	len = hashmap_length(alertManager->schedulers);
	while(hashmap_iterator_has_next(itr)) {
		AlertScheduler *scheduler;
		Alert *alert = NULL ;
		char *token = NULL;
		hashmap_iterator_next(itr);
		scheduler = (AlertScheduler *)hashmap_iterator_value(itr);
		if(NULL != scheduler) {
			alert = alertscheduler_get_alert(scheduler);
			if(NULL != alert) {
				list_add(all, alert);
				token = alert_get_token(alert) ;
				info("all list add :%s %s %s",  token,
					alert_get_repate(alert), alert_get_scheduledtime(alert));
			}
			len = hashmap_length(alertManager->activeAlerts);
			
			int flag = 1;
		
			hashmap_print_key(alertManager->activeAlerts);
			flag = hashmap_is_member(alertManager->activeAlerts, token);
			if(flag == 0) {
				
				list_add(active, alert);
				len = list_length(active);
				info("active list add :%s %s %s", token, alert_get_repate(alert), alert_get_scheduledtime(alert));
				
			}
		}//end if(NULL != scheduler) 
		
	} //	while(hashmap_iterator_has_next(itr))
	//pthread_mutex_unlock(&alertManager->aMutex);
	if(itr)
		hashmap_iterator_free(itr);
	
	return alertsstatepayload_new(all,active);
}




void alertmanager_add(AlertManager * alertManager, Alert *alert, int suppressEvent)
{
	int ret, len ;
	char *token = NULL;
	char *repate = NULL;
	char *schedulerTime = NULL;
	AlertScheduler *scheduler  = NULL;
	
	map_t *schedulers = NULL;

	//pthread_mutex_lock(&alertManager->aMutex);
	schedulers = alertManager->schedulers;
	
	token 			= alert_get_token(alert);
	repate 			= alert_get_repate(alert);
	schedulerTime   = alert_get_scheduledtime(alert);

	info("token : %s", token);
	scheduler = alertscheduler_new(alert, alertManager);
	if(NULL == scheduler) {
		error("alertscheduler_new %s failed", token);
		
	}

	info("hashmap_put %s", token);
	ret = hashmap_put(schedulers, token, scheduler);
	if(ret != MAP_OK) {
		error("hashmap_put %s failed", token);
	}
	info("after hashmap_put");
	//len = hashmap_length(schedulers);
	//hashmap_member_print(schedulers);

	ret = alertmanager_write_current_alerts_to_disk(alertManager);
	
	if(ret == 0) {
	
	} else  {	
			//pthread_mutex_unlock(&alertManager->aMutex);
			alertscheduler_cancel(scheduler, alertManager);	
			//pthread_mutex_lock(&alertManager->aMutex);
	}
    
	//pthread_mutex_unlock(&alertManager->aMutex);
	
}

void alertmanager_delete(AlertManager * alertManager, char *alertToken)
{
	int err; 
	char *token = NULL;
	AlertScheduler *scheduler = NULL;
	map_t *schedulers = alertManager->schedulers;

	//pthread_mutex_lock(&alertManager->aMutex);
	err = hashmap_get(schedulers,alertToken ,(void **)&scheduler);
	error("err:%d",err);
	//err = hashmap_remove(schedulers, alertToken);

	if(err == MAP_OK && NULL != scheduler) {

		Alert *alert = NULL ;
		alertscheduler_cancel(scheduler, alertManager);
	
		err = alertmanager_write_current_alerts_to_disk(alertManager);
		if(err == 0 ) {
			//alertscheduler_free(scheduler);
		} else  {
			info("alertmanager delete %s failed", token);
		}
	}
	
	//pthread_mutex_unlock(&alertManager->aMutex);
}


void alertmanager_start_alert(AlertManager *alertManager,char * alertToken)
{
	int len ;
	//pthread_mutex_lock(&alertManager->aMutex);
    info("alertToken = %s", alertToken);
	hashmap_put(alertManager->activeAlerts,alertToken ,"token");
	len = 	hashmap_length(alertManager->activeAlerts);

	hashmap_print_key(alertManager->activeAlerts);
	int flag ;
	flag = hashmap_is_member(alertManager->activeAlerts, alertToken);

	alerthandler_start_alert(alertManager->handler,alertManager);
	//
	//pthread_mutex_unlock(&alertManager->aMutex);
	return;
}



void alertmanager_stop_alert(AlertManager *alertManager,char *alertToken)
{

	char token[128] = {0};
	memcpy(token, alertToken, strlen(alertToken));

	info("token = %s", token);
	//pthread_mutex_lock(&alertManager->aMutex);
	active_alerts_remove(alertManager->activeAlerts,alertToken);
	scheduers_remove(alertManager->schedulers,alertToken);
    info("-------------");
	alerthandler_stop_alert(alertManager->handler,alertManager);
    info("-------------");

	//pthread_mutex_unlock(&alertManager->aMutex);
	return;
}


void alertmanager_stop_active_alert(AlertManager *alertManager,char *alertToken)
{
	int i;
	int len;
	hashset_itr_t it =  NULL;
	hashset_t activeAlerts = NULL;
	if(alertmanager_has_activealerts(alertManager)) {
		//pthread_mutex_lock(&alertManager->aMutex);	
		hashmap_itr_t itr = hashmap_iterator(alertManager->activeAlerts);;
		len = hashmap_length(alertManager->activeAlerts);

		while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			char *token=NULL;
			token = (char *)hashmap_iterator_key(it);
			info("token = %s ", token);
			//pthread_mutex_unlock(&alertManager->aMutex);	
			alertmanager_stop_alert(alertManager, token);
			//pthread_mutex_lock(&alertManager->aMutex);	
		}
		//pthread_mutex_unlock(&alertManager->aMutex);	
	}
}

#define TRUE 1 
#define FALSE 0 



void *schedulers_thread (void *arg) 
{
	AlertManager *manager = (void *)arg;
	AlertScheduler *scheduler = NULL;

	char buf[BUFF_LENGTH] = {0};
	
	int fd = -1, length , ret;
	ret = unlink(ALERT_FIFO_FILE); 
	if(ret < 0) {
		warning("unlink %s failed errno:%d", ALERT_FIFO_FILE , errno);
	}
	ret = mkfifo(ALERT_FIFO_FILE, 0666);
	if(ret < 0) {
		warning("mkfifo %s failed errno:%d", ALERT_FIFO_FILE , errno);
		exit(-1);	
	}

	info("thread starting ...");
	while(1) {
		if ((fd = open(ALERT_FIFO_FILE, O_RDONLY)) == -1) {	
			error("open %s failed ",ALERT_FIFO_FILE);
			exit(-1);
		}
		bzero(buf, BUFF_LENGTH);
		length = read(fd, buf, BUFF_LENGTH);
		//info("length:%d",length);
		buf[length-1]=0;
		info("buf:\"%s\"",buf);
		if(length > 0)
        {
			char token[128] = {0};
			char *active = NULL;
			char *tmp = NULL;
			int flag = 0, len;
			char *token1 = NULL;
			char *turing = "turing_";
			if ((tmp = strstr(buf, turing)) == NULL) {
				info("buf:%s", buf);
				warning("input not right");
				close(fd);
				continue;
			}
			
			tmp = strchr(buf, ' ');             //turing_1527775012 true
			memcpy(token, buf, tmp-buf);
			active = tmp+1;
			
			info("active:\"%s\"",active);
			int mem;
			mem = hashmap_is_member(manager->schedulers,token);
			if(mem == 0){
				info("[%s] is member", token);
			} else {
				error("[%s] is not member", token);
				close(fd);
				fd = -1;
				continue;
			}
			error("token %d :\"%s\"", strlen(token), token);
			error("active %d :\"%s\"", strlen(active), active);
			
			if(strncmp(active, "true" , strlen("true")) == 0) {
				error("active is true");
				flag = true;
				Alert *alert = NULL;
				scheduler = alertmanager_get_alertscheduler(manager,token);
				if(scheduler == NULL) {
					close(fd);
					fd = -1;
					error("alertmanager_get_alertscheduler %s failed", token);
					continue;
				}
				alert = scheduler->alert;
				iPrompt = strdup(alert->ptone);
				alertscheduler_set_active(scheduler, 1);
				alertmanager_start_alert(manager,token);
			}
			else if(strcmp(active, "sleep") == 0)
			{
				error("goto sleep mode");
		
				TuringSuspend();
			}
			else if(strcmp(active, "wakeup") == 0)
			{
				error("wakeup from sleep mode");
				TuringWakeUp();
			}
			else if(strcmp(active, "timeoff") == 0)
			{
				error("timeoff mode");
				MpdStop();

			}
		}
		
		close(fd);
		fd = -1;
	}
	
	
}


pthread_t alertmanager_schedulers(AlertManager *alertManager)
{
	int ret;
	ret = pthread_create(&alertManager->tid, NULL, schedulers_thread, (void *)alertManager);
	if(ret != 0) {
		error("pthread_create failed");
		return 0;
	}

	return alertManager->tid;
}


void alertmanager_free(AlertManager *alertManager)
{	
	if(NULL != alertManager) {
		hashmap_free(alertManager->schedulers);
		hashmap_free(alertManager->activeAlerts);
		alerthandler_free(alertManager->handler);
		if(alertManager->tid) 
				pthread_join(alertManager->tid, NULL);
		
		pthread_mutex_destroy(&alertManager->aMutex);
		free(alertManager);
		alertManager=NULL;
	}
	
	return ;
}


