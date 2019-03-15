#ifndef __ALERT_MANAGER_H__
#define __ALERT_MANAGER_H__

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "debug.h"
//#undef  __ALERT_SCHEDULER_H__
#include "AlertHead.h"



#define BUFF_LENGTH 256
#define ALERT_FIFO_FILE "/tmp/AlertFIFO"
#if 0

typedef struct alert_manager_s {
	map_t schedulers; //<alert token, AlterScheduler>  一个token对应一个AlterScheduler，用于定时
	hashset_t activeAlerts;  //active alerts token set
	pthread_t tid;
	pthread_mutex_t aMutex; //互斥量

}AlertManager;
#endif
 /* create a AlertManager * , used to manager all alters */
extern AlertManager *alertmanager_new(void);

 /* check if existence of the alert`s token is alertToken 
  *
  * returns non-zero if the alert exists and zero otherwise
  */
extern int alertmanager_has_alert(AlertManager *alertManager, char *alertToken);

 /* check if existence of active alerts
  *
  * returns non-zero if  active alerts exists and zero otherwise
  */
extern int alertmanager_has_activealerts(AlertManager *alertManager);


/* alertmanageradd alerts */
extern void alertmanager_add(AlertManager * alertManager, Alert *alert, int suppressEvent);

/* get all  active alerts
 *
 * returns NULL  if  active alerts not exists and hashset otherwise
 */
//extern hashset_t alertmanager_get_activealerts(AlertManager *alertManager);
extern map_t alertmanager_get_activealerts(AlertManager *alertManager);


/* get all alertToken  AlertScheduler
 *
 * returns NULL  if  alertToken not exists and  a AlertScheduler  otherwise
 */
extern AlertScheduler *alertmanager_get_alertscheduler(AlertManager *alertManager, char *alertToken);

/* get all  alerts
 *
 * returns NULL  if  alerts not exists and  a list otherwise
 */
extern list_t * alertmanager_get_allalerts(AlertManager *alertManager);


/* stop alertToken alerts */
extern void alertmanager_stop_alert(AlertManager *alertManager,char * alertToken);

/* start alertToken alerts */
//extern void alertmanager_start_alert(AlertManager *alertManager,char * alertToken);
extern void alertmanager_start_alert(AlertScheduler *scheduler, char * alertToken);

/* remove item from the  alertmanager */
extern void alertmanager_free(AlertManager *alertManager);

/*  write current alerts to disk */

extern int alertmanager_write_current_alerts_to_disk(AlertManager * alertManager);

extern void alertmanager_drop(Alert *alert) ;

extern map_t alertmanager_get_schedulers(AlertManager *alertManager);

extern void alertmanager_delete(AlertManager * alertManager, char *alertToken);


extern pthread_t alertmanager_schedulers(AlertManager *alertManager);

extern AlertsStatePayload * alertmanager_get_state(AlertManager * alertManager);

extern AlertScheduler *  alertmanager_get_activealertscheduler(AlertManager * manager) ;
extern  int alertmanager_set_alert_stop(AlertManager *alertManager,char *alertToken, int stop);

#endif
