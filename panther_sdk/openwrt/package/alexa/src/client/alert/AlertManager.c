#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "Alert.h"
#include "AlertHandler.h"
#include "AlertScheduler.h"
#include "AlertsStatePayload.h"

#include "AlertManager.h"
#include "hashmap_itr.h"
#include "hashset_itr.h"
#include "Crontab.h"
#include "AlertEventListener.h"

static int ReplaceStr(char* sSrc, char* sMatchStr, char* sReplaceStr)
{
        int StringLen;
        char caNewString[256];
        char* FindPos;
        FindPos =(char *)strstr(sSrc, sMatchStr);
        if( (!FindPos) || (!sMatchStr) )
                return -1;

        while( FindPos )
        {
                memset(caNewString, 0, sizeof(caNewString));
                StringLen = FindPos - sSrc;
                strncpy(caNewString, sSrc, StringLen);
                strcat(caNewString, sReplaceStr);
                strcat(caNewString, FindPos + strlen(sMatchStr));
                strcpy(sSrc, caNewString);

                FindPos =(char *)strstr(sSrc, sMatchStr);
        }
        free(FindPos);
        return 0;
}


static void alertmanager_print_payload(AlertManager *alertManager)
{
	AlertsStatePayload *payload = NULL;
	json_object  *all = NULL;
	json_object  *active = NULL;
	
	payload = alertmanager_get_state(alertManager);
	all = alertsstatepayload_to_allalerts(payload);
	DEBUG_INFO("alertsstatepayload_to_allalerts");
	active = alertsstatepayload_to_activealerts(payload);
	if(payload) {
		alertsstatepayload_free(payload);
	}
	DEBUG_INFO("alertsstatepayload_to_activealerts");
	if(all)
		DEBUG_INFO(" allAlert:%s", json_object_to_json_string(all));
	if(active)
		DEBUG_INFO(" activeAlert:%s", json_object_to_json_string(active));
	if(all) 
		json_object_put(all);
	if(active) 
		json_object_put(active);
}


void active_alerts_remove(map_t activeAlerts, char *alertToken)
{
	//shset_remove(activeAlerts,alertToken);
	//char *tmp = NULL;
	//tmp = alertToken;
	hashmap_remove(activeAlerts,alertToken);
	//if(alertToken != NULL)
	//	free(alertToken);
}


static void      scheduers_remove(map_t *schedulers, char *alertToken)
{
	AlertScheduler *scheduler = NULL;
		
	int err;
	err = hashmap_get(schedulers, alertToken, (void **)&scheduler);
	DEBUG_INFO("err:%d",err);
	hashmap_remove(schedulers, alertToken);
	if(err == MAP_OK && scheduler) {
		alertscheduler_free(scheduler);
	}
	
	
}


// token 要 用strdup, 后面要free
AlertManager *alertmanager_new(void)
{
	int ret;
	AlertManager *alertManager = NULL;

	TimerInit(0);
	
	alertManager= (AlertManager *)calloc(1, sizeof(AlertManager));
	if(NULL == alertManager) {
		DEBUG_ERR("malloc for AlertManager failed");
		return NULL;
	}	

	alertManager->handler = alerthandler_new();
	if(NULL == alertManager->handler) {
		DEBUG_ERR("alerthandler_new failed");
		goto failed;
	}

	alertManager->schedulers = hashmap_new();
	if(NULL == alertManager->schedulers) {
		DEBUG_ERR("hashmap_new failed");
		goto failed1;
	}

	alertManager->activeAlerts = hashmap_new();

	if(NULL == alertManager->schedulers) {
		DEBUG_ERR("hashmap_new failed");
		goto failed2;
	}

	ret = pthread_mutex_init(&alertManager->aMutex,NULL);
	if(ret) {
		DEBUG_ERR("pthread_mutex_init failed");
		goto failed3;
	}
	
	//alertManager->schedulersIterator = hashmap_iterator(alertManager->schedulers);
	
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
	//
	//DEBUG_INFO("....");
	AlertScheduler *scheduler = NULL;
//	pthread_mutex_lock(&alertManager->aMutex);
	err = hashmap_get(alertManager->schedulers, alertToken, (void**)(&scheduler));
	if(err != MAP_OK)
		scheduler = NULL;
//	pthread_mutex_unlock(&alertManager->aMutex);

	return scheduler;
}

int alertmanager_has_alert(AlertManager *alertManager, char *alertToken)
{
	int ret ;
	//pthread_mutex_lock(&alertManager->aMutex);
	ret = hashmap_is_member(alertManager->schedulers, alertToken);
//	pthread_mutex_unlock(&alertManager->aMutex);
	return !ret;
}

int alertmanager_has_activealerts(AlertManager *alertManager)
{
	int ret;
	//DEBUG_INFO("....");
//	pthread_mutex_lock(&alertManager->aMutex);
	//ret= hashset_num_items(alertManager->schedulers) > 0 ;
	ret= hashmap_length(alertManager->activeAlerts) > 0;
//	pthread_mutex_unlock(&alertManager->aMutex);
	
	return ret;
}

map_t alertmanager_get_activealerts(AlertManager *alertManager)
{
	map_t activeAlerts = NULL;
//
	//	DEBUG_INFO("...");
	//pthread_mutex_lock(&alertManager->aMutex);
	activeAlerts = alertManager->activeAlerts;
	//pthread_mutex_unlock(&alertManager->aMutex);

	return activeAlerts;
}

map_t alertmanager_get_schedulers(AlertManager *alertManager)
{
	map_t schedulers = NULL;

	//DEBUG_INFO("...");
	//pthread_mutex_lock(&alertManager->aMutex);
//	DEBUG_INFO("...");
	schedulers = alertManager->schedulers;
	//DEBUG_INFO("...");
	//pthread_mutex_unlock(&alertManager->aMutex);
	//DEBUG_INFO("...");

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
	DEBUG_INFO("len hashmap_length :%d", len);
	while(hashmap_iterator_has_next(itr)) {
		Alert *alert = NULL;	
		hashmap_iterator_next(itr);
		scheduler = hashmap_iterator_value(itr);
		if(NULL != scheduler) {
			alert = alertscheduler_get_alert(scheduler);
			if (alert != NULL) {
				DEBUG_INFO("list add %s", alert_get_token( alert));
				list_add(list, alert);
			}
		}

	}
	//pthread_mutex_unlock(&alertManager->aMutex);
if(itr)
	hashmap_iterator_free(itr);
	return list;
}


AlertHandler *alertmanager_get_alerthandler(AlertManager *alertManager)
{
	return alertManager->handler;
}


void alertmanager_drop(Alert *alert) 
{
	alerteventlistener_on_alert_stopped(alert_get_token(alert));
}

/*  return 0 is ok */
int alertmanager_write_current_alerts_to_disk(AlertManager * alertManager)
{

	int ret;
	list_t *all = NULL;
	list_iterator_t *it = NULL ;

	//pthread_mutex_unlock(&alertManager->aMutex);
	all = alertmanager_get_allalerts(alertManager);

	it = list_iterator_new(all,LIST_HEAD);
	while(list_iterator_has_next(it)) {
			char *token = NULL ;
			Alert *alert = NULL; 
			alert			=  list_iterator_next_value(it);
			if(alert != NULL) {
					token  =  alert_get_token(alert);
					DEBUG_INFO("************** %s",token );
			}
	}

	
	DEBUG_INFO("all list length : %d ", list_length(all));

	list_iterator_destroy(it);

	//pthread_mutex_lock(&alertManager->aMutex);
#if 0
	ret = crontab_wirte_to_disk(all);
#else
	ret = crontab_wirte_to_disk(alertManager);
#endif
	

	return ret;
}



static void hashmap_print( map_t map) 
{
	int len ;
	AlertScheduler *scheduler;
	hashmap_itr_t itr = hashmap_iterator(map);
	len = hashmap_length(map);
	DEBUG_INFO("^^^^^^^^^^^^^^^^??????????? len:%d" ,len);
	while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			scheduler = (AlertScheduler *)hashmap_iterator_value(itr);
			if(scheduler != NULL) {
				Alert *alert = NULL ;
				alert = alertscheduler_get_alert(scheduler);
				if(alert != NULL)
					DEBUG_INFO("??????????? token:%s" ,alert_get_token(alert));
				
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
	DEBUG_INFO("^^^^^^^^^^^^^^^^??????????? len:%d" ,len);
	while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			key = hashmap_iterator_key(itr);
			DEBUG_INFO("###########^^^^^^^^^^^^^^^^??????????? key:%s" ,key);
				
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
	DEBUG_INFO("(alertManager->schedulers len :%d", len);
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
				DEBUG_INFO("all list add :%s %s %s",  token,
					alert_get_type(alert), alert_get_scheduledtime(alert));
			}
			//len = hashset_num_items(alertManager->activeAlerts);
			len = hashmap_length(alertManager->activeAlerts);
			//DEBUG_INFO("#################################################token: %s ", token);
			//DEBUG_INFO("#################################################alertManager->activeAlerts:len = %d", len);
			int flag = 1;
			//flag = hashset_is_member(alertManager->activeAlerts, token);
			hashmap_print_key(alertManager->activeAlerts);
			flag = hashmap_is_member(alertManager->activeAlerts, token);
			/*if(flag == 0) {
				DEBUG_INFO("#####################token:%s is in alertManager->activeAlerts", token);
			} else {
				DEBUG_INFO("#####################token:%s is not in alertManager->activeAlerts", token);
			}*/
			//if(0 == hashmap_is_member(alertManager->activeAlerts, token)) {
			if(flag == 0) {
				DEBUG_INFO("#################################################active list_add active %s ", token);
				list_add(active, alert);
				len = list_length(active);
				DEBUG_INFO("#################################################active:len = %d", len);
				DEBUG_INFO("active list add :%s %s %s", token, 
				alert_get_type(alert), alert_get_scheduledtime(alert));
			}
		}//end if(NULL != scheduler) 
		
	} //	while(hashmap_iterator_has_next(itr))
	//pthread_mutex_unlock(&alertManager->aMutex);
exit:
	if(itr)
		hashmap_iterator_free(itr);
			DEBUG_INFO("alertsstatepayload_new");
	return alertsstatepayload_new(all,active);
}

int alertmanager_readd(AlertManager * alertManager, Alert *alert, int suppressEvent)
{
	char *token = alert_get_token(alert);
	alertmanager_print_payload(alertManager);
	if (alertmanager_has_alert(alertManager, token)) {
		DEBUG_INFO("scheduers_remove %s ", token);
		scheduers_remove(alertManager->schedulers,token);
		suppressEvent = false;
	}
	alertmanager_print_payload(alertManager);
	
	alertmanager_add(alertManager, alert,suppressEvent);
}




void alertmanager_add(AlertManager * alertManager, Alert *alert, int suppressEvent)
{
	int ret, len ;
	char *token = NULL;
	//char *type = NULL;
	//char *schedulerTime = NULL;
	AlertScheduler *scheduler  = NULL;
	
	AlertScheduler *schedulers = NULL;
	alertmanager_print_payload(alertManager);
	//pthread_mutex_lock(&alertManager->aMutex);
	schedulers = alertManager->schedulers;
	
	token 			= alert_get_token(alert);
	//type 			= alert_get_type(alert);
	//schedulerTime   = alert_get_scheduledtime(alert);

	scheduler = alertscheduler_new(alert, alertManager);
	if(NULL == scheduler) {
		DEBUG_INFO("alertscheduler_new %s failed", token);
	}
	DEBUG_INFO(" ##########################################&&&&&&&&&&&&&&&&&&&7Adding alert with token { %s }     &&&&&&&&&&&&&&&&&&&##########################################" ,token);
	ret = hashmap_put(schedulers, token, scheduler);
	if(ret != MAP_OK) {
		DEBUG_INFO("hashmap_put %s failed", token);
	}
	len = hashmap_length(schedulers);
	//DEBUG_INFO(".........len = %d",len);
	hashmap_member_print(schedulers);
	
	//DEBUG_INFO("hashmap_length %d", len);
	//pthread_mutex_unlock(&alertManager->aMutex);
	//写入文件
	//#if 0
	ret = alertmanager_write_current_alerts_to_disk(alertManager);
	//ret = crontab_wirte_to_disk(alertManager);
	DEBUG_INFO("ret:%d", ret);
	if(ret == 0) {
			if(!suppressEvent)
				alerteventlistener_on_alert_set(token ,true);
	} else  {
			if(!suppressEvent)
				alerteventlistener_on_alert_set(token ,false);
			
			hashmap_remove(schedulers,token);
			alertscheduler_free(scheduler);
			//scheduers_remove(schedulers,token);		
	}
	alertmanager_print_payload(alertManager);
	//pthread_mutex_unlock(&alertManager->aMutex);
	
}
#if 1
void alertmanager_delete(AlertManager * alertManager, char *alertToken)
{
	int err; 
	char *token = NULL;
	AlertScheduler *scheduler = NULL;
	map_t *schedulers = alertManager->schedulers;
	DEBUG_INFO("alertToken:%s",alertToken);
	alertmanager_print_payload(alertManager);
	//pthread_mutex_lock(&alertManager->aMutex);
	err = hashmap_get(schedulers,alertToken, (void **)&scheduler);
	DEBUG_INFO("err:%d",err);
	if(err == MAP_OK && NULL != scheduler) {
		Alert *alert = NULL ;
		alert = alertscheduler_get_alert(scheduler);
		token = strdup(alert_get_token(alert));
	
		//pthread_mutex_unlock(&alertManager->aMutex);
		alertscheduler_cancel(scheduler, alertManager);
		alertmanager_print_payload(alertManager);
		//pthread_mutex_lock(&alertManager->aMutex);
		err = alertmanager_write_current_alerts_to_disk(alertManager);
		if(err == 0 ) {
			alerteventlistener_on_alert_delete(token, true);
		} else  {
			DEBUG_INFO("alertmanager delete %s failed", token);
			alerteventlistener_on_alert_delete(token, false);
		}
		if(token) {
			free(token);
			token = NULL;
		}
	}
	//pthread_mutex_unlock(&alertManager->aMutex);
	alertmanager_print_payload(alertManager);
}

#else
void alertmanager_delete(AlertManager * alertManager, char *alertToken)
{
	int err; 
	char *token = NULL;
	AlertScheduler *scheduler = NULL;
	map_t *schedulers = alertManager->schedulers;

	//pthread_mutex_lock(&alertManager->aMutex);

	err = hashmap_get(schedulers,alertToken ,(void **)&scheduler);

	err = hashmap_remove(schedulers, alertToken);

	if(err == MAP_OK && NULL != scheduler) {
		Alert *alert = NULL ;
		alert = alertscheduler_get_alert(scheduler);
		
		token = alert_get_token(alert);
		err = alertmanager_write_current_alerts_to_disk(alertManager);
		if(err == 0 ) {
			char *pToken = strdup(token);
			//DEBUG_INFO("alertmanager delete %s  successed", token);
			alertscheduler_cancel(scheduler,alertManager);
			alerteventlistener_on_alert_delete(pToken, true);
			if (pToken)
			{
				free(pToken);
				pToken = NULL;
			}
			//scheduers_remove(schedulers,alertToken);
			//alertscheduler_free(scheduler);
		} else  {
			DEBUG_INFO("alertmanager delete %s failed", token);
			alerteventlistener_on_alert_delete(token, false);
		}
	}
	
	//pthread_mutex_unlock(&alertManager->aMutex);
}
#endif


void alertmanager_start_alert(AlertScheduler *scheduler, char * alertToken)
{
	int len ;

	AlertManager   *alertManager   = NULL;

	alertManager   = alertscheduler_get_alertmanager(scheduler);
	alertmanager_print_payload(alertManager);
	//pthread_mutex_lock(&alertManager->aMutex);

	//hashset_add(alertManager->activeAlerts,(void *)alertToken);
	hashmap_put(alertManager->activeAlerts, alertToken ,"token");
	//len = hashset_num_items(alertManager->activeAlerts);
	len = 	hashmap_length(alertManager->activeAlerts);
	DEBUG_INFO("#################################################alertManager->activeAlerts:len = %d", len);

	hashmap_print_key(alertManager->activeAlerts);
	int flag ;
	//flag = hashset_is_member(alertManager->activeAlerts, alertToken);
	flag = hashmap_is_member(alertManager->activeAlerts, alertToken);
	if(flag == 0) {
				DEBUG_INFO("#####################token:%s is in alertManager->activeAlerts", alertToken);
	} else {
				DEBUG_INFO("#####################token:%s is not in alertManager->activeAlerts", alertToken);
	}
	alerteventlistener_on_alert_started(alertToken);
	alerthandler_start_alert(scheduler,alertManager);
	alertmanager_set_alert_stop(alertManager, alertToken, 0);
	alertmanager_print_payload(alertManager);
	//播放alert声音
	//pthread_mutex_unlock(&alertManager->aMutex);
	return;
}

void scheduers_delete(AlertManager *alertManager,char *alertToken)
{
	int len;	
	len = hashmap_length(alertManager->schedulers);
	DEBUG_INFO("before scheduers_remove len:%d", len);
	scheduers_remove(alertManager->schedulers,alertToken);
	len = hashmap_length(alertManager->schedulers);
	DEBUG_INFO("after scheduers_remove len:%d", len);
}

int alertmanager_set_alert_stop(AlertManager *alertManager,char *alertToken, int stop)
{
	int err;
	AlertScheduler *scheduler = NULL;
	map_t *schedulers = alertManager->schedulers;

	//pthread_mutex_lock(&alertManager->aMutex);

	err = hashmap_get(schedulers,alertToken ,(void **)&scheduler);
	if(err == MAP_OK && NULL != scheduler) {
		DEBUG_INFO("err == MAP_OK  && NULL != scheduler");
		Alert *alert = NULL ;
		alert = alertscheduler_get_alert(scheduler);
		if (alert) {
			alert_set_stop(alert, stop);
		}
	}
	alertmanager_write_current_alerts_to_disk(alertManager);
	return err;
}

void alertmanager_stop_alert(AlertManager *alertManager,char *alertToken)
{
	//char *token = NULL;
	char token[128] = {0};
	Alert *alert = NULL;
	memcpy(token, alertToken, strlen(alertToken));
	
	DEBUG_INFO("token = %s", alertToken);
	//pthread_mutex_lock(&alertManager->aMutex);
	active_alerts_remove(alertManager->activeAlerts,alertToken);
	alerteventlistener_on_alert_stopped(alertToken);
	alerthandler_stop_alert(alertManager->handler,alertManager);
	alertmanager_set_alert_stop(alertManager, alertToken, 1);
	
	//pthread_mutex_unlock(&alertManager->aMutex);
	return;
}

#if 0
void alertmanager_stop_active_alert(AlertManager *alertManager)
{
	int i;
	int len;
	hashset_itr_t it =  NULL;
	hashset_t activeAlerts = NULL;
	if(alertmanager_has_activealerts(alertManager)) {
	//pthread_mutex_lock(&alertManager->aMutex);	
	hashmap_itr_t itr = hashmap_iterator(alertManager->activeAlerts);;
	len = hashmap_length(alertManager->activeAlerts);
	hashmap_print_key(alertManager->activeAlerts);
	DEBUG_INFO("$$$$$$$$$$$$$$$$len :%d", len);
	while(hashmap_iterator_has_next(itr)) {
		hashmap_iterator_next(itr);
		char *token=NULL;
		token = (char *)hashmap_iterator_key(itr);
		DEBUG_INFO("token = %s ", token);
		//pthread_mutex_unlock(&alertManager->aMutex);	
		alertmanager_stop_alert(alertManager, token);
		//scheduers_remove(alertManager->schedulers,token);
		//pthread_mutex_lock(&alertManager->aMutex);	
	}
/*
		//len = hashset_num_items(activeAlerts);
		for(i = 0; i < len; i++) {
			char *token=NULL;
			token = (char *)hashset_iterator_value(it);	
			pthread_mutex_unlock(&alertManager->aMutex);
			alertmanager_stop_alert(alertManager, token);
			pthread_mutex_lock(&alertManager->aMutex);
			hashset_iterator_next(it);
		}*/	
	//pthread_mutex_unlock(&alertManager->aMutex);	
	}
}
#endif


void alertmanager_stop_active_alert(AlertManager *alertManager)
{
	int i;
	int len;
	hashset_itr_t it =  NULL;
	hashset_t activeAlerts = NULL;
	if(alertmanager_has_activealerts(alertManager)) {
	//pthread_mutex_lock(&alertManager->aMutex);	
	hashmap_itr_t itr = hashmap_iterator(alertManager->activeAlerts);;
	len = hashmap_length(alertManager->activeAlerts);
	hashmap_print_key(alertManager->activeAlerts);
	DEBUG_INFO("$$$$$$$$$$$$$$$$len :%d", len);
	while(hashmap_iterator_has_next(itr)) {
		hashmap_iterator_next(itr);
		char *token=NULL;
		token = (char *)hashmap_iterator_key(itr);
		DEBUG_INFO("token = %s ", token);
		//pthread_mutex_unlock(&alertManager->aMutex);	
		alertmanager_stop_alert(alertManager, token);
		//pthread_mutex_lock(&alertManager->aMutex);	
	}
	hashmap_iterator_free(itr);

	}
}


#if 0


void *schedulers_thread(void *arg) 
{
	AlertManager *manager = (void *)arg;
	AlertScheduler *scheduler = NULL;

	char buf[BUFF_LENGTH] = {0};
	
	int fd = -1, length , ret;
	ret = unlink(ALERT_FIFO_FILE); 
	if(ret < 0) {
		DEBUG_INFO("unlink %s failed errno:%d", ALERT_FIFO_FILE , errno);
	//	exit(-1);
		
	}
	ret = mkfifo(ALERT_FIFO_FILE, 0666);
	if(ret < 0) {
		DEBUG_INFO("mkfifo %s failed errno:%d", ALERT_FIFO_FILE , errno);
		exit(-1);	
	}

	DEBUG_INFO("thread starting ...");
	printf("\n");
	while(1) {
		if ((fd = open(ALERT_FIFO_FILE, O_RDONLY)) == -1) {	
			DEBUG_ERR("open %s failed ",ALERT_FIFO_FILE);
			
			exit(-1);
		}
		bzero(buf, BUFF_LENGTH);
		length = read(fd, buf, BUFF_LENGTH);
		
		if(length > 0) {
			char token[128] = {0};
			char *active = NULL;
			char *tmp = NULL;
			int flag = 0;
			char *token1 = NULL;

			if (NULL == strstr(buf, "amzn1.as-ct.v1.Domain:Application:Notifications")) {
				DEBUG_INFO("buf:%s", buf);
				DEBUG_INFO("input not right");
				continue;
			}
			DEBUG_INFO("buf:%s", buf);
			tmp = strchr(buf, ' ');
			
			memcpy(token, buf, tmp-buf);
			active = tmp+1;
			int mem;
			ReplaceStr(token ,"*",  "#");
			mem = hashmap_is_member(manager->schedulers,token);
			if(mem == 0){
				DEBUG_INFO("%s is member", token);
			} else 
				DEBUG_INFO("%s is not member", token);
			DEBUG_INFO("token %d :%s", strlen(token), token);
			DEBUG_INFO("active %d :%s", strlen(active), active);
			//DEBUG_INFO("token :%s,active:%s", token, active);
			token1 = strdup(token);
			
			if(strncmp(active, "true" , strlen("true")) == 0) {
				DEBUG_INFO("active is true");
				flag = true;
				Alert *alert = NULL;
				scheduler = alertmanager_get_alertscheduler(manager,token);
				if(scheduler == NULL) {
					close(fd);
					DEBUG_INFO("alertmanager_get_alertscheduler %s failed", token);
					continue;
				}

				///ertscheduler_get_token();
				
				alertscheduler_set_active(scheduler, true);
		 		//shmap_put(manager->activeAlerts,token1 ,"token");
				//shmap_print_key(manager->activeAlerts);
				
				alertmanager_start_alert(manager,token1);
				
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
		DEBUG_INFO("pthread_create failed");
		return 0;
	}

	return alertManager->tid;
}

#else

#endif
void alertmanager_free(AlertManager *alertManager)
{	
	if(NULL != alertManager) {
		hashmap_free(alertManager->schedulers);
		hashmap_free(alertManager->activeAlerts);
		alerthandler_free(alertManager->handler);
		
		pthread_mutex_destroy(&alertManager->aMutex);
		free(alertManager);
		alertManager=NULL;
	}
	
	return ;
}
AlertScheduler *  alertmanager_get_activealertscheduler(AlertManager * manager) 
{
	int len ;
	AlertScheduler * scheduler = NULL;
	char *key = NULL;
	map_t activeAlerts = NULL;
	map_t schedulers = NULL;
	activeAlerts = manager->activeAlerts;
	if(activeAlerts == NULL) {
		DEBUG_INFO("activeAlerts == NULL");
		return NULL;
	}
	hashmap_itr_t itr = hashmap_iterator(activeAlerts);
	len = hashmap_length(activeAlerts);
	DEBUG_INFO("activealerts len:%d" ,len);
	while(hashmap_iterator_has_next(itr)) {
			hashmap_iterator_next(itr);
			key = hashmap_iterator_key(itr);
			if(key)
				break;
	}		
	if(itr) 
		hashmap_iterator_free(itr);

	if(key == NULL) {
		DEBUG_INFO("activeAlerts == NULL");
		return NULL;
	}
	
	schedulers = alertmanager_get_schedulers(manager);
	if(schedulers == NULL)
		return NULL;
	int err;
	DEBUG_INFO("alert:%s" ,key);
	err = hashmap_get(schedulers, key, (void **)&scheduler);

	return 	scheduler;
}


