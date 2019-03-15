#include "lock.h"

#include <stdio.h>
#include <cdb.h>



static int type = 0; 

static char *speech[] = {
	"抱歉，地球操作暂无权限哦",
	"阿尔法星已锁定按键，无法操作",
	"按键权限已锁定，请听当前内容吧",
	"请先解锁，获取地球操作权限",
}; 

static int speechLength = sizeof(speech)/sizeof(speech[0]);

static int powerState = 0;
/* 童锁功能 */
void	ChildLock()
{
	int turingPowerState;
	turingPowerState =  cdb_get_int("$turing_power_state",0);
	warning("turingPowerState:%d",turingPowerState);
	if(turingPowerState == 4) {
		cdb_set_int("$turing_power_state",0);
	} else {
		powerState = turingPowerState;
		cdb_set_int("$turing_power_state",4);
	}
}

void ChildLocked()
{
	
	text_to_sound(speech[type]);

	type = (type+1)%speechLength;
}











