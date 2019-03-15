#ifndef __CRONTAB_H__
#define __CRONTAB_H__


#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "AlertHead.h"

//#define CRONTAB_ALARM_FILE  "/etc/conf/alarms.json"
#define CRONTAB_ALARM_FILE  "/etc/crontabs/root"
#define SECONDS_AFTER_PAST_ALERT_EXPIRES  1800


#if 1
/* return 0 successd ,otherwise failed */
extern int crontab_wirte_to_disk(list_t *list);
#else
extern int crontab_wirte_to_disk(AlertManager *manager);

#endif

/* return 0 successd ,otherwise failed */
//void alertfiledatastore_load_from_disk(AlertManager *manager)
//extern int crontab_load_from_disk();
extern int crontab_load_from_disk(AlertManager *manager);



extern void crontab_stop_alert(char *token);

extern void crontab_start_alert(char *token);

void crontab_delete_alert(char *token);

#endif





