#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "Alert.h"
#include "alert_api.h"

Alert * alert_new(Alert *pAlert)
{
	Alert *alert = NULL;
	alert = calloc(1, sizeof(Alert));

	if(NULL == alert) {
		DEBUG_ERR("malloc alert failed");
		return NULL;
	}

	//alert->token 	     = strdup(token);
	//alert->type 		 = strdup(type);
	//alert->scheduledTime = strdup(scheduledTime);
	alert->token 	               = strdup(pAlert->token);
	alert->type 		           = strdup(pAlert->type);
	alert->scheduledTime           = strdup(pAlert->scheduledTime);

	if ((0 == strcmp(pAlert->type, TYPE_REMINDER)) || (-1 == pAlert->m_reminder.loopCount))
	{
		alert->m_reminder.assetPlayOrder          = strdup(pAlert->m_reminder.assetPlayOrder);
		alert->m_reminder.backgroundAlertAsset    = strdup(pAlert->m_reminder.backgroundAlertAsset);
		alert->m_reminder.loopCount               = pAlert->m_reminder.loopCount;
		alert->m_reminder.loopPauseInMilliseconds = pAlert->m_reminder.loopPauseInMilliseconds;
	}
	else
	{
		alert->m_reminder.assetPlayOrder          = NULL;
		alert->m_reminder.backgroundAlertAsset    = NULL;
		alert->m_reminder.loopCount               = -2;
		alert->m_reminder.loopPauseInMilliseconds = -1;
	}
	alert->stop = 0;
	return alert;
}

char *alert_get_token(Alert *alert)
{
	return alert->token;
}


char *alert_get_type(Alert *alert)
{
	return alert->type;
}

char *alert_get_scheduledtime(Alert *alert)
{
	return alert->scheduledTime;
}

char *alert_get_assetPlayOrder(Alert *alert)
{
	return alert->m_reminder.assetPlayOrder;
}

char *alert_get_backgroundAlertAsset(Alert *alert)
{
	return alert->m_reminder.backgroundAlertAsset;
}

int alert_get_loopCount(Alert *alert)
{
	return alert->m_reminder.loopCount;
}

int alert_get_loopPauseInMilliseconds(Alert *alert)
{
	return alert->m_reminder.loopPauseInMilliseconds;
}

int alert_get_stop(Alert *alert)
{
	return alert->stop;
}
void   alert_set_stop(Alert *alert, int stop)
{
	alert->stop = stop;
}

void print_tm( struct tm *p)
{   
	char *wday[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	//printf("%d %02d %02d ",1900+p->tm_year,(1+p->tm_mon),p->tm_mday);  
	//printf("%s %02d:%02d:%02d\n",wday[p->tm_wday],(p->tm_hour),p->tm_min,p->tm_sec);  
}


/*  convert UTC time to local zone time  */
time_t alert_convert_scheduledtime(Alert *alert)
{
	
	time_t tm_t;
	struct tm tm; 
	 struct tm *time = NULL ;
	struct timeval tv;
    struct timezone tz;
	
	char *scheduledTime = NULL;
	char buf[128]={0};
	
	int zone;
	int year;		
	int month;
	int day;
	int hour;
	int min;
	int sec;
	
	scheduledTime = alert->scheduledTime;
	//printf("scheduledTime:%s\n",scheduledTime);	
	DEBUG_INFO("scheduledTime:%s",scheduledTime);		

	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min,&sec ,&zone);

	snprintf(buf,128,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec);	
	DEBUG_INFO("buf:%s\n",buf);		
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
	DEBUG_INFO("strptime");		
	//print_tm(&tm);
	//printf("%s\n", asctime(&tm));
	DEBUG_INFO("print_tm");		

	tm_t = mktime(&tm); //tm结构体转换成秒数
	
    gettimeofday(&tv, &tz); //获得当前精确时间（1970年1月1日到现在的时间）
   	DEBUG_INFO("****************tz.tz_minuteswest:%d\n",tz.tz_minuteswest);		

	tm_t -= tz.tz_minuteswest * 60 ;//获得的是UTC时间，转换成当地时间
	
	time = localtime(&tm_t);
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(time);
	
	
	return tm_t;
}


time_t iso8601_to_UTC(char *scheduledTime)
{
	char buf[64]={0};

	int zone = 0;
	int year = 0;		
	int month= 0;
	int day  = 0;
	int hour = 0;
	int min  = 0;
	int sec  = 0;
	
	struct tm tm; 
	time_t tm_t;

	//scheduledTime = alert->scheduledTime;
	//printf("scheduledTime:%s\n",scheduledTime);	
	DEBUG_INFO("scheduledTime:%s\n",scheduledTime);		

	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min, &sec ,&zone);
	
	snprintf(buf,64,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec);	
	
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);

	tm_t = mktime(&tm); //tm结构体转换成秒数

	return tm_t;
}


/*  convert UTC time to local zone time  */
time_t alert_convert_scheduledtime_s(char *scheduledTime)
{
	
	time_t tm_t;
	struct tm tm; 
	struct tm *time = NULL ;
	struct timeval tv;
    struct timezone tz;
	
	//char *scheduledTime = NULL;
	char buf[64]={0};
	
	int zone;
	int year;		
	int month;
	int day;
	int hour;
	int min;
	int sec;
	
	//scheduledTime = alert->scheduledTime;
	//printf("scheduledTime:%s\n",scheduledTime);	
	DEBUG_INFO("scheduledTime:%s",scheduledTime);		

	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min,&sec ,&zone);

	snprintf(buf,64,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec);	
	//DEBUG_INFO("buf:%s\n",buf);		
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
	//print_tm(&tm);

	tm_t = mktime(&tm); //tm结构体转换成秒数
	
    gettimeofday(&tv, &tz); //获得当前精确时间（1970年1月1日到现在的时间）
    DEBUG_INFO("****************tz.tz_minuteswest:%d\n",tz.tz_minuteswest);		

	tm_t -= tz.tz_minuteswest * 60 ;//获得的是UTC时间，转换成当地时间

	time = localtime(&tm_t);
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(time);
	DEBUG_INFO("Local time is %s",asctime(time)); 
	
	return tm_t;
}

// 得到本地时间和定时器时间的差值
unsigned long  alert_get_time_diff(Alert *alert)
{
	time_t scheduledTime;
	time_t now;
	//scheduledTime = alert_convert_scheduledtime(alert);
	scheduledTime = iso8601_to_UTC(alert->scheduledTime);
	//now = time(NULL);
	now = get_now_time_utc();
	DEBUG_INFO("now:%ld scheduledTime:%ld", now, scheduledTime);
	return (unsigned long)difftime(scheduledTime, now);
}

void alert_convert_scheduledtime_tm(Alert *alert, struct tm **time) 
{
	time_t tm_t; 
	struct tm tm; 
	struct timeval tv;
    struct timezone tz;
	
	char *scheduledTime = NULL;
	char buf[64]={0};
	
	int zone;
	int year;		
	int month;
	int day;
	int hour;
	int min;
	int sec;

	scheduledTime = alert->scheduledTime;
	
	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min,&sec ,&zone);
	
	snprintf(buf,64,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec );	
	DEBUG_INFO("buf:%s",buf);		
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);

	tm_t = mktime(&tm); //tm结构体转换成秒数
	
    gettimeofday(&tv, &tz); //获得当前精确时间（1970年1月1日到现在的时间）

	tm_t -= tz.tz_minuteswest * 60 ;//获得的是UTC时间，转换成当地时间
	//tm_t += 8 * 60 * 60;
	
	//tm_t += tz.tz_minuteswest * 60 + 60 * 60;

	*time = localtime(&tm_t);
	 DEBUG_INFO("############Beijing  time is %s/n",asctime(*time));   
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(*time);
	
}
		

// 返回UTC时间
time_t get_now_time()
{
	 //struct timeval tv;
	// struct timezone tz;
	 //gettimeofday(&tv, &tv); //��õ�ǰ��ȷʱ�䣨1970��1��1�յ����ڵ�ʱ�䣩
	 time_t timep;
	 struct tm *timenow; //ʵ����tm�ṹָ��  
	 time(&timep);
	 timenow = localtime(&timep);	
	 DEBUG_INFO("--------Local time is %s/n",asctime(timenow));   
	 //p=gmtime(s);

	 return timep;
}


time_t get_now_time_utc()
{
	time_t tm_t;
	struct tm utc_tm;;
	time_t cur_time = time(NULL);
	if( cur_time < 0 )
	{
		perror("time");
		return -1;
	}

	if( NULL == gmtime_r( &cur_time, &utc_tm ) )
	{
		perror("gmtime" );
		return -1;
	}

	tm_t = mktime(&utc_tm);

	return tm_t;
}



int alert_equals(Alert *alert, Alert *otherAlert)
{
	if(NULL != alert && alert == otherAlert)
		return 1;
	if(NULL != alert->token && NULL != otherAlert->token) {
		if(strcmp(alert->token, otherAlert->token) == 0)
			return 1;
	}
	return 0;
}


void alert_free(Alert *alert)
{

	if(alert != NULL) {
		if(alert->token) {
			free(alert->token);
		}
		if(alert->type) {
			free(alert->type);
		}
		if(alert->scheduledTime) {
			free(alert->scheduledTime);
		}
		if(alert->m_reminder.assetPlayOrder) {
			free(alert->m_reminder.assetPlayOrder);
		}
		if(alert->m_reminder.backgroundAlertAsset) {
			free(alert->m_reminder.backgroundAlertAsset);
		}
		free(alert);
		alert = NULL;
	}
}



