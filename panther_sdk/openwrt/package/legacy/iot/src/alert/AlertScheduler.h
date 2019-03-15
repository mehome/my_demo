#ifndef __ALERT_SCHEDULER_H__
#define __ALERT_SCHEDULER_H__

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "myutils/debug.h"

#include "AlertHead.h"







extern void alertscheduler_free(AlertScheduler * alertScheduler) ;
extern void alertscheduler_cancel(AlertScheduler * alertScheduler, AlertManager *handler);
extern Alert * alertscheduler_get_alert(AlertScheduler * alertScheduler);
extern void alertscheduler_set_active(AlertScheduler * alertScheduler, int active);
extern int alertscheduler_is_active(AlertScheduler * alertScheduler);
extern AlertManager* alertscheduler_get_alertmanager(AlertScheduler * alertScheduler);

extern AlertScheduler *alertscheduler_new(Alert *alert,AlertManager* handler);






#endif





