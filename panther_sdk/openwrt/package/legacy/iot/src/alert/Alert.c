#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "Alert.h"

Alert * alert_new(const char *token, char * scheduledTime ,char *repate, int type ,char *iprompt)
{
	Alert *alert = NULL;
	alert = calloc(1, sizeof(Alert));

	if(NULL == alert) {
		error("malloc alert failed");
		return NULL;
	}
	
	alert->token 	     = token;
	alert->type 	     = type;
	alert->scheduledTime = scheduledTime;
	alert->repate        = repate;
	alert->ptone		 = iprompt;
	info("token:%s, scheduledTime:%s, repate:%s, type:%d,iprompt:%s", token, scheduledTime, repate, type,alert->ptone);
	return alert;
}

char *alert_get_token(Alert *alert)
{
	return alert->token;
}


char *alert_get_repate(Alert *alert)
{
	return alert->repate;
}
char *alert_get_scheduledtime(Alert *alert)
{
	return alert->scheduledTime;
}
int alert_get_type(Alert *alert)
{
	return alert->type;
}

void print_tm( struct tm *p)
{   
	char *wday[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	printf("%d %02d %02d ",1900+p->tm_year,(1+p->tm_mon),p->tm_mday);  
	printf("%s %02d:%02d:%02d\n",wday[p->tm_wday],(p->tm_hour),p->tm_min,p->tm_sec);  
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
	

	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min,&sec ,&zone);

	snprintf(buf,128,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec);	
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
	
	//print_tm(&tm);
	//printf("%s\n", asctime(&tm));


	tm_t = mktime(&tm); 
	
    gettimeofday(&tv, &tz); 
   	info("tz.tz_minuteswest:%d",tz.tz_minuteswest);		

	tm_t -= tz.tz_minuteswest * 60 ;
	
	time = localtime(&tm_t);
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(time);
	
	
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
	info("scheduledTime:%s",scheduledTime);		

	sscanf(scheduledTime,"%d-%02d-%02dT%02d:%02d:%02d%3d", &year,&month,&day, &hour, &min,&sec ,&zone);

	snprintf(buf,64,"%d-%02d-%02d %02d:%02d:%02d", year, month, day,  hour, min, sec);	
	strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
	//print_tm(&tm);

	//tm_t = mktime(&tm); //tm结构体转换成秒数
	
    //gettimeofday(&tv, &tz); //获得当前精确时间（1970年1月1日到现在的时间）

	//tm_t -= tz.tz_minuteswest * 60 ;//获得的是UTC时间，转换成当地时间

	time = localtime(&tm_t);
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(time);
	info("Local time is %s/n",asctime(time)); 
	
	return tm_t;
}

unsigned long  alert_get_time_diff(Alert *alert)
{
	time_t scheduledTime;
	time_t now;
	scheduledTime = alert_convert_scheduledtime(alert);
	now = time(NULL);
	return (unsigned long)difftime(scheduledTime, now);
}



void alert_convert_scheduledtime_tm(Alert *alert, struct tm **time) 
{
	time_t tm_t; 
	struct tm tm; 
	struct timeval tv;
    struct timezone tz;
	char *scheduledTime = NULL;
	
	int zone;
	int year;		
	int month;
	int day;
	int hour;
	int min;
	int sec;
	long iTime;

	scheduledTime = alert->scheduledTime;
	
	tm_t = atol(scheduledTime);

	info("tm_t:%ld",tm_t);   
   // gettimeofday(&tv, &tz); //获得当前精确时间（1970年1月1日到现在的时间）

	//tm_t -= tz.tz_minuteswest * 60 ;//获得的是UTC时间，转换成当地时间
	//tm_t += 8 * 60 * 60;
	
	//tm_t += tz.tz_minuteswest * 60 + 60 * 60;
    static char str_time[100] = {0};
   static char tmp[100] = {0};

	*time = localtime(&tm_t);
   
  
    strftime(tmp, sizeof(tmp), "%Y-%m-%d,%H:%M:%S",*time);
        
    info ("\nTime: %s\n", tmp);
	//info(" \n\n#######################################Beijing  time is %s\n\n",asctime(*time));   
	//*time->tm_year += 1900;
	//*time->tm_mon += 1;
	//print_tm(*time);
	
}
		


time_t get_now_time()
{
	 //struct timeval tv;
   	// struct timezone tz;
	 //gettimeofday(&tv, &tv); 
	 time_t timep;
	 struct tm *timenow; 
	 time(&timep);
	 timenow = localtime(&timep);   
	 info("--------Local time is %s",asctime(timenow));   

	 return timep;
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

	if(alert != NULL)
    {
		if(alert->token) {
			free(alert->token);
		}
		if(alert->scheduledTime) {
			free(alert->scheduledTime);
		}
		if(alert->repate)
        {
            free(alert->repate);
        }      
		free(alert);
		alert = NULL;
	}
}



