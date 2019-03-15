#ifndef __ALERT_APT_H__
#define __ALERT_APT_H__ 
#include <stdlib.h>

#include "AlertManager.h"


#define TYPE_REMINDER					"REMINDER"
#define TYPE_TIMER						"TIMER"
#define TYPE_ALARM						"ALARM"


#define ALERTS_DIRECTIVES_SETALERT 		"SetAlert"
#define ALERTS_DIRECTIVES_DELETEALERT 	"DeleteAlert"



extern int handle_alerts_directive(AlertManager *manager, char *name , char *payload);
#endif



