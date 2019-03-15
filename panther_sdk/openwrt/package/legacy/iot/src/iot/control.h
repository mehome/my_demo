#ifndef __CONTROL_H__
#define __CONTROL_H__

#include <json/json.h>
#include "myutils/debug.h"

enum {
	CONTROL_VOLUME_DOWN = 0 , //chat ,
	CONTROL_VOLUME_UP , //audio ,
	CONTROL_BLN_OFF,
	CONTROL_BLN_ON,
	CONTROL_LOW_POWER_ON,
	CONTROL_LOW_POWER_OFF,
	CONTROL_FORMAT_STORAGE,
	CONTROL_FACTORY_RESET,
	CONTROL_ALARM_SET,
	CONTROL_TELEPHONE_SET,
	CONTROL_SLEEP_TIME_SET,
	CONTROL_WAKEUP_TIME_SET,
	CONTROL_TIMED_OFF_SET,
};


int ParseControlData(json_object *message);





#endif

