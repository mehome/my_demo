#include "control.h"

#include <stdio.h>


#include "myutils/debug.h"
#include "common.h"
#include "AlertHead.h"


static int blnSwitch = 0;
static int lowPowerSwitch = 0;

void SwitchInit()
{
	int bln = 0;
	int lbi = 0;
	int battery = 0;
	char * power = NULL;
	power = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.bln");

	if(power) {
		warning("bln:%s",power);
		bln = atoi(power);
		if( bln != 0 && bln != 1)
			bln = 0;
		WIFIAudio_UciConfig_FreeValueString(&power);
	}

	power = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.lbi");
	
	if(power) {
		warning("lbi:%s",power);
		lbi = atoi(power);
		if( bln != 0 && bln != 1)
			lbi = 0;
		WIFIAudio_UciConfig_FreeValueString(&power);
	}
	blnSwitch = bln;
	lowPowerSwitch = lbi;
	
}

int GetBlnSwitch()
{
	return blnSwitch ;
}

int GetLowPowerSwitch()
{
	return lowPowerSwitch ;
}

/* 处理音量消息. */
static int ControlVolume(json_object *arg)
{
	char *cmd[24]= {0};
	int volume ;
	volume = json_object_get_int(arg);
	snprintf(cmd,24, "mpc volume %d", volume);

	info("cmd:%s",cmd);
	system(cmd);
	return 0;
}
/* 处理呼吸灯消息. */
static int ControlBln(int on)
{
	info("ControlBln:%d", on);
	blnSwitch = on;
	if(on) {
		exec_cmd("uartdfifo.sh bln002");
		WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.bln", "1");
	} else {
		exec_cmd("uartdfifo.sh bln000");
		WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.bln", "0");
	}
	return 0;
}
/* 处理低电量消息. */
static int ControlLowPower(int on)
{
	info("ControlLowPower:%d", on);
	lowPowerSwitch = on;
	if(on) {
		WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.lbi", "1");
	} else {
		WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.lbi", "0");
	}
	return 0;
}

static int ControlFormatStorage(void)
{
	
	return 0;
}

/* 处理恢复出场设置消息. */
static int ControlFactoryReset(void)
{
#if defined(CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_1MIC_V1)	
	exec_cmd("uartdfifo.sh StartKeyFactory");
//#elif defined(CONFIG_PROJECT_1MIC_V1)
//	exec_cmd("uartdfifo.sh StartKeyFactory");
#endif
	exec_cmd("factoryReset.sh");
	exec_cmd("reboot");
	return 0;
}


static char * strtrim_ch(char *s, char ch) 
{
    char *p = s;
    char *q = s;
    while ( *p == ch ) ++p;
    while ( *q++ = *p++)
        ;
    q -= 2;
    while (*q == ch) --q;
    *(q+1) ='\0';
    return s;
}

/* 处理闹钟消息. */
static int ControlAlarmSet(json_object *arg)
{
	json_object *id = NULL, *title = NULL, *time=NULL, *status=NULL;
	json_object *repeat = NULL,*prompt = NULL;
	char *pTime=NULL, *pTitle = NULL, *pRpeat=NULL;
	int turingId;
	int iStatus;
	char token[24]={0};
	char *iprompt=NULL;
	//char tmp[28]={0};
	id =  json_object_object_get(arg, "id");
	if(id == NULL)
		return 0;
	if(id) {
		turingId = json_object_get_int(id);
		info("id:%d", turingId);
	}
	status =  json_object_object_get(arg, "status");
	if(status) {
		iStatus = json_object_get_int(status);
		info("status:%d", iStatus);
	}

	time =  json_object_object_get(arg, "time");
	if(time) {
		
		long iTime ;

		char *tmp;
		int len;

		iTime  =(long)json_object_get_double(time);
		error("iTime:%ld", iTime);
			
		tmp = json_object_get_string(time);
		len = strlen(tmp);
		info("time:%s len:%d", tmp, len);
		len = len - 2;
		pTime = malloc(len);
		pTime[len-1]='\0';
		int i; 
		for(i =0; i < len-1; i++)
			pTime[i] = tmp[i];
		info("pTime:%s",pTime);
		iTime = atol(pTime);
		info("iTime:%ld", iTime);

		struct tm *time = NULL ;
		
		time = localtime(&iTime);
 
		info("Local time is %s",asctime(time)); 
	}
	title =  json_object_object_get(arg, "title");
	if(title) {
		pTitle = json_object_get_string(title);
		info("title:%s", pTitle);
	}
	prompt = json_object_object_get(arg,"prompt");
	if(prompt)
	{
		iprompt = json_object_get_string(prompt);
		info("iprompt :%s",iprompt);
	}
	repeat =  json_object_object_get(arg, "repeat");
	if(repeat) {
		char *value = json_object_get_string(repeat);
		warning("reapte:\"%s\"[%p] len:%d", value, value, strlen(value));
		pRpeat =  strtrim_ch(value, ',');
		warning("pRpeat:\"%s\" [%p] len:%d", pRpeat, pRpeat, strlen(pRpeat));
	}

	snprintf(token, 24,  "turing_%d", turingId);
	warning("token:%s",token);
	warning("pTime:%s",pTime);
	warning("pRpeat:%s",pRpeat);
	
	if(iStatus == 1) {
		/* 添加闹钟 */
		AlertAdd(strdup(token), pTime, strdup(pRpeat),ALERT_TYPE_ALARM,strdup(iprompt));
	}else if(iStatus == 0){
		error("AlertRemove:%s",token);
		/* 关闭闹钟 */
		AlertRemove(token);
	}
	 
	return 0;
}

/* 处理休眠和唤醒消息. */
static int ControlSleepWakeup(int type, json_object *arg)
{
	int len;
	char *pBedTime = NULL, *pWakeTime = NULL;
	char *tmp;
#if 0
	tmp = json_object_get_string(arg);

	len = strlen(tmp);
	info("time:%s len:%d", tmp, len);
	len = len - 2;
	pTime = malloc(len);
	pTime[len-1]='\0';
	int i; 
	for(i =0; i < len-1; i++)
		pTime[i] = tmp[i];
	info("pTime:%s",pTime);
	iTime = atol(pTime);
	info("iTime:%ld", iTime);

	struct tm *time = NULL ;
	
	time = localtime(&iTime);

	info("Local time is %s",asctime(time)); 
#endif
	json_object *onObj = NULL,  *bedObj = NULL, *wakeObj = NULL;
	long bedTime, wakeTime ;

	int on = 0;
	
	onObj = json_object_object_get(arg, "on");
	if(onObj) {
		on = json_object_get_int(onObj);
	}
	
	bedObj = json_object_object_get(arg, "bed");
	if(bedObj) {
		tmp = json_object_get_string(bedObj);
		bedTime  =(long)json_object_get_double(bedObj);
		error("bedTime:%ld", bedTime);
		len = strlen(tmp);
		info("time:%s len:%d", tmp, len);
		len = len - 2;
		pBedTime = malloc(len);
		pBedTime[len-1]='\0';
		int i; 
		for(i =0; i < len-1; i++)
			pBedTime[i] = tmp[i];
		info("pTime:%s",pBedTime);
		bedTime = atol(pBedTime);
		info("iTime:%ld", bedTime);

		struct tm *time = NULL ;
		
		time = localtime(&bedTime);

		info("Local time is %s",asctime(time)); 
	}
	
	wakeObj = json_object_object_get(arg, "wake");
	if(wakeObj) {
		tmp = json_object_get_string(wakeObj);
		len = strlen(tmp);
		info("time:%s len:%d", tmp, len);
		wakeTime  =(long)json_object_get_double(wakeObj);
		error("wakeTime:%ld", wakeTime);
		len = len - 2;
		pWakeTime = malloc(len);
		pWakeTime[len-1]='\0';
		int i; 
		for(i =0; i < len-1; i++)
			pWakeTime[i] = tmp[i];
		info("pTime:%s",pWakeTime);
		wakeTime = atol(pWakeTime);
		info("iTime:%ld", wakeTime);

		struct tm *time = NULL ;
		
		time = localtime(&wakeTime);

		info("Local time is %s",asctime(time)); 
	}
	
	if(on == 1) 
	{
		if(pWakeTime) {
			/* 添加闹钟 类别ALERT_TYPE_WAKEUP */
			AlertAdd(strdup("turing_wake"), pWakeTime,strdup("null"), ALERT_TYPE_WAKEUP,strdup("null"));
		}
		if(pBedTime) {
			/* 添加闹钟 类别ALERT_TYPE_SLEEP */
			AlertAdd(strdup("turing_bed"), pBedTime,strdup("null"), ALERT_TYPE_SLEEP,strdup("null"));
		}
	}

	return 0;

}

/* 处理定时停止播放消息. */
static int ControlTimeOff(int type, json_object *arg)
{
	
	int len;
	char *pTime = NULL;
	long iTime ;
	int tmp ;
	char buf[128] = {0};
	
	tmp = json_object_get_int(arg);
	info("tmp:%d",tmp); 

	if(tmp == 0) {
		AlertRemove("turing_timeoff");
		return 0;
	} else if(tmp == -1) {
		AlertRemove("turing_timeoff");
		eval("mpc", "repeat", "off");
		eval("mpc", "single", "on");
		return 0;
	}	
	unsigned int now = time(NULL);
	
	now += tmp*60;
	
	info("now:%d",now); 
	
	struct tm *time = NULL;
	
	time = localtime(&now);
	
	info("Local time is %s",asctime(time)); 
	snprintf(buf, 128, "%d", now);
	info("buf:%s",buf);
	/* 添加闹钟 类别ALERT_TYPE_TIMEOFF */
	AlertAdd(strdup("turing_timeoff"), strdup(buf),strdup("null"), ALERT_TYPE_TIMEOFF,strdup("null"));


	
	return 0;
}

/* 处理图灵IOT云端向设备端推送的控制消息. */
int ParseControlData(json_object *message)
{
	struct json_object *operate = NULL, *arg = NULL;
	warning("message:%s",json_object_to_json_string(message));
	operate =  json_object_object_get(message, "operate");
	
	if(operate) {
		int iOperate; 
		iOperate = json_object_get_int(operate);
		arg = json_object_object_get(message, "arg");
		if(arg) 
        {
			switch(iOperate) 
            {
				case CONTROL_VOLUME_DOWN:
				case CONTROL_VOLUME_UP:
					info("get CONTROL_VOLUME message");
					ControlVolume(arg);
					break;
				case CONTROL_BLN_OFF:
					info("get CONTROL_BLN_OFF message");
					//ControlBln(0);
					break;
				case CONTROL_BLN_ON:
					info("get CONTROL_BLN_ON message");
					//ControlBln(1);
					break;
				case CONTROL_LOW_POWER_OFF:
					info("get CONTROL_LOW_POWER_OFF message");
					//ControlLowPower(0);
					break;
				case CONTROL_LOW_POWER_ON:
					info("get CONTROL_LOW_POWER_ON message");
					//ControlLowPower(1);
					break;
				case CONTROL_FORMAT_STORAGE:
					info("get CONTROL_FORMAT_STORAGE message");
					ControlFormatStorage();
					break;
				case CONTROL_FACTORY_RESET:
					info("get CONTROL_FACTORY_RESET message");
					ControlFactoryReset();
					break;
				case CONTROL_ALARM_SET:
					info("get CONTROL_ALARM_SET message");
					ControlAlarmSet(arg);
					break;
				case CONTROL_TELEPHONE_SET:
					info("get CONTROL_TELEPHONE_SET message");
					break;
				case CONTROL_SLEEP_TIME_SET:
					info("get CONTROL_SLEEP_TIME_SET message");
					ControlSleepWakeup(0,arg);
					break;
				case CONTROL_WAKEUP_TIME_SET:
					ControlSleepWakeup(1,arg);
					info("get CONTROL_WAKEUP_TIME_SET message");
					break;
				case CONTROL_TIMED_OFF_SET:
					ControlTimeOff(1,arg);
					info("get CONTROL_TIMED_OFF_SET message");
					break;
			}
		}
		
	}
	return 0;
}






