﻿


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <json/json.h>


#include "Crontab.h"
#include "Alert.h"
#include "list_iterator.h"
#include "AlertManager.h"
#include "alert_api.h"

int* substr_check(const char* str, const char* pattern, int* size)  
{  
    if (NULL == str ||  
        NULL == pattern) return NULL;  
  	int i;
    int* p = malloc(100*4);  
    int index = -1;  
  
    int size_str = strlen(str);  
    int size_pattern = strlen(pattern);  
    for ( i = 0 ; i <= size_str - size_pattern;) {  
        int k = 0;
		int j ;
        for (j = i; k < size_pattern;) {  
            if ( str[j] == pattern[k]) {  
                j++;  
                k++;  
            } else {  
                break;  
            }  
        }  
  
        if (k == size_pattern) {  
            p[++index] = i;  
            i += size_pattern; //找到一个匹配后，从这个匹配的下一个字符开始继续检查是否匹配。  
        } else {  
            i++;  
        }  
    }  
  
    *size = index + 1;  
  
    return p;  
}  
  
char* replace_str(const char* source, const char* pattern, const char* replace)  
{  
    char* pres = NULL;  
  	int i;
    int pattern_size = strlen(pattern);  
    int replace_size = strlen(replace);  
  
    int size = 0;  
    int* p = substr_check(source, pattern, &size);  
    int final_size = strlen(source) + size * (replace_size - pattern_size) + 1;  
    pres = malloc(final_size);  
    memset(pres, 0 , final_size);  
  
    int new_index = 0;//计算出原始字符串source中的下标index在替换后的字符串pres中对应的index  
    for(i = 0; i < size; i++) {  
        int old_index = p[i];  
        if (0 == i) {  
            //left side  
            memcpy(pres, source, old_index);  
  
            //replace substr  
            memcpy(pres + old_index, replace, replace_size);   
        } else if ((size - 1) == i) {  
            //left side  
            new_index = p[i - 1] + (i - 1) * (replace_size - pattern_size) + replace_size;  
            memcpy(pres + new_index, source + p[i - 1] + pattern_size, old_index - p[i - 1] - pattern_size);  
  
            //right side  
            new_index = old_index + i * (replace_size - pattern_size) + replace_size;  
            int tmp = strlen(source) - old_index - pattern_size;  
            memcpy(pres + new_index, source + old_index + pattern_size, strlen(source) - old_index - pattern_size);  
  
            //replace substr  
            new_index = old_index + i * (replace_size - pattern_size);  
            memcpy(pres + new_index, replace, replace_size);  
        } else {  
            //left side  
            new_index = p[i - 1] + (i - 1) * (replace_size - pattern_size) + replace_size;  
            memcpy(pres + new_index, source + p[i - 1] + pattern_size, old_index - p[i - 1] - pattern_size);  
  
            //replace substr  
            new_index = old_index + i * (replace_size - pattern_size);  
            memcpy(pres + new_index, replace, replace_size);  
        }  
    }  

	free(p);

    return pres;  
}


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



static char * file_get_line( char *dst, FILE * stream ) 
{
    register int c;
    char * cs;

    cs = dst;
    while ( ( c = fgetc( stream ) ) != EOF ) {
        if ( c == '\n' ) {
			
			break; 
			
		}
        *cs++ = c; 
    }
    *cs = '\0';

    return ( c == EOF && cs == dst ) ? NULL : dst;
}

static int file_put_line(const char *src, FILE * stream, int flag)
{		
	size_t len,count;		
	len = strlen(src);				
	char *tmp="#";		
	char *enter="\n";		

	printf("len %d \n",len);			
	if(flag == 1) {			
		fwrite(tmp, strlen(tmp), 1,stream);		
	}		
	if((count = fwrite(src, 1, len, stream)) != len) {	
		printf("count: %d \n",count)	;
		printf("fwrite %s failed\n",src);					
						
		return 1;	
	}		
	fwrite(enter, strlen(enter), 1,stream);		
	return 0;
}

static void  file_delete_line(char *token, char *file)
{	
		char *tmp;
		char *str;
		str = replace_str(token, "#", "[*#]");
	
		if(str != NULL) {
			tmp = str;
		
		} else {
			tmp = token;
		}
	
		DEBUG_INFO("#########################str:%s\n",tmp);		
		char cmd[256]={0};		
		snprintf(cmd,256,"sed -i '/\"%s\"/d' %s", tmp, file);	
		DEBUG_INFO("cmd:%s\n",cmd);		
		system(cmd);
		system("/etc/init.d/crond restart");
		if(str != NULL) 
			free(str);
}

void crontab_stop_alert(char *token)
{
	char *tmp;
	char *str;
	str = replace_str(token, "#", "*");
	if(str != NULL) {
		tmp = str;
	} else {
		tmp = token;
	}
	tmp = token;

	DEBUG_INFO("#########################str:%s\n",tmp);			
	char cmd[256]={0};
	snprintf(cmd, 256,"sed -i 's/token=\"%s\".*;/token=\"%s\"; active=\"false\" ;/' "CRONTAB_ALARM_FILE, 
				tmp, tmp);	
	DEBUG_INFO("cmd:%s\n",cmd); 	
	system(cmd);
	system("/etc/init.d/crond restart");
	if(str != NULL) 
		free(str);
}

void crontab_start_alert(char *token)
{
	char *tmp;
	char cmd[256]={0};
	char *str;
	str = replace_str(token, "#", "*");
	if(str != NULL) {
		tmp = str;
	} else {
		tmp = token;
	}
	tmp = token;

	DEBUG_INFO("#########################str:%s\n",tmp);	
	//DEBUG_INFO("...");
	snprintf(cmd,256,"sed -i 's/token=\"%s\".*;/token=\"%s\"; active=\"true\" ;/' "CRONTAB_ALARM_FILE, 
				tmp, tmp);	
	DEBUG_INFO("cmd:%s\n",cmd); 	
	system(cmd);
	system("/etc/init.d/crond restart");
	if(str != NULL) 
		free(str);
}


#if 0
/* return 0 successd ,otherwise failed */
int crontab_wirte_to_disk(list_t *list)
{
	char *tmp;
	FILE *stream=NULL;
	list_iterator_t *it = NULL ;

	char buf[64]={0};
	snprintf(buf, 64, "touch %s",CRONTAB_ALARM_FILE);
	system(buf);

	
	//DEBUG_INFO("...");
	stream = fopen(CRONTAB_ALARM_FILE, "wb");
	if (NULL == stream) {
		DEBUG_ERR("fopen %s failed",CRONTAB_ALARM_FILE);
		goto failed1;
	}

	it = list_iterator_new(list,LIST_HEAD);
 
	while(list_iterator_has_next(it)) {
		DEBUG_INFO("list_iterator_has_next");
		int err;
		struct tm *time=NULL;
		const char *note = NULL;
		Alert *alert = NULL;
		char *token = NULL ;
		char *type = NULL;
		char *scheduledTime  = NULL;
		char schedule[512]= {0};
		char cmd[256] = {0};
	
		alert 			=  list_iterator_next_value(it);
		if(alert != NULL) {
				token 			=  alert_get_token(alert);
				type 			=  alert_get_type(alert);
				scheduledTime 	=  alert_get_scheduledtime(alert);
				json_object *val;
				val = json_object_new_object();

				json_object_object_add(val,"token",json_object_new_string(token));
				json_object_object_add(val,"type", json_object_new_string(type));
				json_object_object_add(val,"scheduledTime",json_object_new_string(scheduledTime));
				//DEBUG_INFO("val :%s",json_object_to_json_string(val));
				//printf("val :%s\n",json_object_to_json_string(val));
				
				note = json_object_to_json_string(val);
				//printf("note :%s\n",note);
				DEBUG_INFO("note:%d :%s",strlen(note), note);
				err = file_put_line(note , stream, 1);
				if(err == 1) {
					DEBUG_INFO("put %s failed", note);
					json_object_put(val);
					goto failed;
				} else {
					DEBUG_INFO("put %s ok", note);
				}
				
				json_object_put(val);
				DEBUG_INFO("before alert_convert_scheduledtime_tm");
				alert_convert_scheduledtime_tm(alert,&time);
				DEBUG_INFO("after alert_convert_scheduledtime_tm");
				DEBUG_INFO("token=%s",token);
				char *replace = NULL;
				replace = replace_str(token, "#", "*");
				if(replace != NULL)
					tmp = replace; 
				else 
					tmp = token;
				//ReplaceStr(token , "#", "*");
				//snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
				//	"echo $token start > ./alarm.test || echo $token cancle > ./alarm.test", token);
				snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
					"/usr/bin/alertfifo.sh \"$token true\" || /usr/bin/alertfifo.sh \"$token false\" ", tmp);
				//snprintf(cmd, 128 ,"echo %s starting", token);
				DEBUG_INFO("cmd:%s ", cmd);
				
				snprintf(schedule, 512, "%d %d %d %d %d  %s", time->tm_min ,time->tm_hour,
							time->tm_mday, time->tm_mon+1, time->tm_wday, cmd);
				
				DEBUG_INFO("schedule:%s ", schedule);
				err = file_put_line(schedule , stream, 0);
				if(err == 1) {
					DEBUG_ERR("put %s failed", schedule);
					goto failed;
				} else {
					DEBUG_INFO("put %s ok", schedule);
				}
				if(replace != NULL)
					free(replace);
		}
		
	 }//end while(list_iterator_has_next(it))
	system("/etc/init.d/crond restart");
	fclose(stream);
	return 0;
failed1:
	fclose(stream);
failed:
	list_iterator_destroy(it);
	return 1;
	
}
#else 

/* return 0 successd ,otherwise failed */
int crontab_wirte_to_disk(AlertManager *manager)
{

	size_t len,count;	
	FILE *stream=NULL;
	struct json_object  *all = NULL;
	AlertsStatePayload *payload = NULL;
	
	pthread_mutex_unlock(&manager->aMutex);
	DEBUG_INFO("alertmanager_get_state");
	payload = alertmanager_get_state(manager);
	DEBUG_INFO("alertsstatepayload_to_allalerts");
	all = alertsstatepayload_to_allalerts(payload);

	char *allAlert = NULL;
	DEBUG_INFO("json_object_to_json_string");
	allAlert = json_object_to_json_string(all);
	DEBUG_INFO("allAlert : %s", allAlert);
	
	stream = fopen(CRONTAB_ALARM_FILE, "wb");
	if (NULL == stream) {
		DEBUG_ERR("fopen %s failed",CRONTAB_ALARM_FILE);
		//return 1;
		goto failed;
	}
	len = strlen(allAlert);
	
	if((count = fwrite(allAlert, 1, len, stream)) != len) {	
		DEBUG_ERR("count: %d ",count)	;
		DEBUG_ERR("fwrite %s failed",allAlert);								
		//return 1;	
		goto failed1;
	}	
	alertsstatepayload_free(payload);
	json_object_put(all);
	fclose(stream);
	return 0 ;
failed1:
	fclose(stream);
failed:
	alertsstatepayload_free(payload);
	json_object_put(all);
	
	return 1 ;

	
}

#endif



#if 0
/* return 0 successd ,otherwise failed */

//int crontab_load_from_disk()
int crontab_load_from_disk(AlertManager *manager)
{ 
			list_iterator_t *it = NULL;
			FILE *stream=NULL;
			char *line;
			char chars[256] = {0} ;
		 	list_t *alerts = NULL;
			list_t * droppedAlerts = NULL;

			if((access(CRONTAB_ALARM_FILE,F_OK))!=-1)  {
				DEBUG_ERR("%s is  exist" ,CRONTAB_ALARM_FILE);
			} else {
				DEBUG_ERR("%s is not exist" ,CRONTAB_ALARM_FILE);
				system("touch /etc/crontabs/root ");
			}


			
			DEBUG_INFO("crontab_load_from_disk");
			alerts = list_new();
			if(NULL == alerts) {
				DEBUG_ERR("list_new alerts failed \n");
				//return failed;
				goto failed;
			}
			droppedAlerts = list_new();
			if(NULL == droppedAlerts) {
				DEBUG_ERR("list_new droppedAlerts failed \n");
				goto failed;
			}
		
			stream = fopen(CRONTAB_ALARM_FILE, "rb");
			if (NULL == stream) {
				DEBUG_ERR("fopen %s failed",CRONTAB_ALARM_FILE);
				goto failed;
			}

			while((line = file_get_line(chars, stream)) != NULL) {
				struct json_object *val = NULL,*tokenObj=NULL ,*typeObj=NULL, *scheduledTimeObj=NULL;
				Alert *alert = NULL;
				 char *token = NULL ;
				 AlertType type = NULL; 
				 char *scheduledTime = NULL;
				
				DEBUG_INFO("line=%s", line);
				
				if(line[0] == '#') {
					char *tmp = NULL;
					tmp = &line[1];
					DEBUG_INFO("tmp %s ", tmp);
					val = json_tokener_parse(tmp);
					if(val == NULL) {
						DEBUG_INFO("json_tokener_parse tmp");
						goto failed;
					}
					DEBUG_INFO("val %s ", json_object_to_json_string(val));

					tokenObj 		 = json_object_object_get(val, "token");
					typeObj 		 = json_object_object_get(val, "type");
					scheduledTimeObj = json_object_object_get(val, "scheduledTime");
					if(tokenObj == NULL || typeObj == NULL || scheduledTimeObj == NULL) {
						json_object_put(val);
						DEBUG_INFO("json_object_object_get token type scheduledTime error");
						goto failed;
					}
				
					token 			 = (char *)json_object_get_string(tokenObj);
					type 			 = (char *)json_object_get_string(typeObj);
					scheduledTime    = (char *)json_object_get_string(scheduledTimeObj);
					
					DEBUG_INFO("token:%s, type:%s, scheduledTime:%s", token, type, scheduledTime);
					alert = alert_new(token,type,scheduledTime);
					if (NULL == alert) {
						json_object_put(val);
						DEBUG_ERR("alert_new  failed ");
						goto failed;
					}
					list_add(alerts,alert);
					DEBUG_INFO("after  alert_new  ");
					json_object_put(val);
					
				} //end if line[0]=="#"
				
			}// end while ((line =file_get_line(chars, stream)) != NULL) 

			fclose(stream);
			stream = NULL;
			it = list_iterator_new(alerts,LIST_HEAD);
			while(list_iterator_has_next(it)) {
					Alert *alert = NULL;
					char *token = NULL ;
					AlertType type = NULL; 
					const char *scheduledTime = NULL;
					time_t scheduled, now;

					
					alert = list_iterator_next_value(it);
					if(NULL == alert) {
						DEBUG_ERR("list_iterator_next_value error");
						goto failed;
					}

					token 			= alert_get_token(alert);
					scheduledTime 	= alert_get_scheduledtime(alert);
					type            = alert_get_type( alert);
					
					scheduled = alert_convert_scheduledtime(alert);
					DEBUG_INFO("scheduledTime :%ld",scheduled );
					DEBUG_INFO("after  alert_convert_scheduledtime  ");
					now = get_now_time();

					
					DEBUG_INFO("now :%ld",now);	

					DEBUG_INFO("difftime:%f",difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES));
					if(difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES) > 0) {
							alertmanager_add(manager, alert, true);
							DEBUG_INFO("alertmanager_add token:%s, type:%s,scheduledTime:%s,", token, type, scheduledTime);
					} else {
							DEBUG_INFO(" file_delete_line");
							//list_add(droppedAlerts, alert);
							//从ALARM_FILE中删除alert;
							//alertmanager_drop(alert);
							file_delete_line(token, CRONTAB_ALARM_FILE);
					}
						
				}// end while(list_iterator_has_next(it))
		
					
			// Now that all the valid alerts have been re-added to the alarm manager,
            // go through and explicitly drop all the alerts that were not added
			#if 0
			
			it = list_iterator_new(droppedAlerts,LIST_HEAD);
			while(list_iterator_has_next(it)) {
				Alert *alert = NULL;
				char *token = NULL;
				alert = list_iterator_next_value(it);
				if(NULL != alert) {
					token  = alert_get_token(alert);
					//alertmanager_drop(alert);
					//从ALARM_FILE中删除alert;
					file_delete_line(token, CRONTAB_ALARM_FILE);
				} //  end if(NULL != alert)
				
			} //end while(list_iterator_has_next(it))
			list_iterator_destroy(it);
			#endif
			list_iterator_destroy(it);
			list_destroy(alerts);
			list_destroy(droppedAlerts);
			system("/etc/init.d/crond restart ");
			return 0;
failed:
			if( NULL != stream)
				fclose(stream);
			if(NULL != alerts)
				list_destroy(alerts);
			if(NULL !=droppedAlerts)
				list_destroy(droppedAlerts);
			if(NULL != it)
				list_iterator_destroy(it);
			system("/etc/init.d/crond restart ");
			return 1;
			
}
#else

static unsigned long get_file_size(const char *filename)
{
	unsigned long size;
	FILE *fp ;
	
	fp	= fopen(filename, "rb");
	if(fp == NULL) {
		DEBUG_ERR("open %s failed",filename);
		return 0;
	}

	fseek(fp, SEEK_SET,SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}


ssize_t readn(int fd, void *vptr, size_t n) 
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0) {
		if((nread = read(fd, ptr, nleft)) < 0) {
			if(errno == EINTR)
				nread = 0;
			else 
				return -1;
		}else if(nread == 0) {
			break;
		}
		nleft -= nread;
		ptr += nread;
	}

	return (n-nleft);
}


ssize_t Readn(int fd, void *ptr, size_t nbytes) 
{
	ssize_t n;
	if(( n = readn(fd, ptr, nbytes)) < 0)
		DEBUG_ERR("readn error");

	return n;
}

int crontab_load_from_disk(AlertManager *manager)
{ 
	int flag = 0;
	int fd, length, i;
	FILE *stream=NULL;
	size_t len, count;
	char *allAlerts;
 	list_t *alerts = NULL;
	list_t * droppedAlerts = NULL;
	struct json_object  *all = NULL ;

	DEBUG_INFO("crontab_load_from_disk");
	alerts = list_new();
	if(NULL == alerts) {
		DEBUG_ERR("list_new alerts failed \n");
		goto failed;
	}

	droppedAlerts = list_new();
	if(NULL == droppedAlerts) {
		DEBUG_ERR("list_new droppedAlerts failed \n");
		goto failed1;
	}

	len = get_file_size(CRONTAB_ALARM_FILE);

	fd = open(CRONTAB_ALARM_FILE, O_RDONLY);
	if(fd <= 0) {
		DEBUG_ERR("open failed");
		goto failed2;
	}

	allAlerts = calloc(len, 1);
	if(NULL == allAlerts) {
		DEBUG_ERR("calloc for allAlerts failed");
		goto failed2;
	}

	count = Readn(fd, allAlerts, len);
	if(count < 0) 
	{
		DEBUG_ERR("Readn error");
		goto failed3;
	}
	else
	{
		DEBUG_INFO("allAlerts: %s ",allAlerts);
	}

	close(fd);

	all =  json_tokener_parse(allAlerts);
	if(NULL == all) {
		DEBUG_ERR("json_tokener_parse failed");
		goto failed3;
	}

	length = json_object_array_length(all);
	DEBUG_INFO("length:%d ", length);
	if(length <= 0 ) {
		DEBUG_ERR("json_object_array_length failed");
		goto failed4;
	}

	for ( i = 0; i < length ; i++) 
	{
		int stop = 0;
		Alert temp_alert;
		Alert *alert = NULL;

		time_t scheduled, now;
		struct json_object *obj =  NULL ;
		struct json_object *tokenObj=NULL ,*typeObj=NULL, *scheduledTimeObj=NULL,  *loopCount = NULL;
		struct json_object *stopObj = NULL;
		obj = json_object_array_get_idx(all, i);  
		tokenObj		 = json_object_object_get(obj , "token");
		typeObj 		 = json_object_object_get(obj , "type");
		scheduledTimeObj = json_object_object_get(obj , "scheduledTime");
		stopObj 		 = json_object_object_get(obj , "stop");
		
		memset(&temp_alert, 0, sizeof(Alert));

		temp_alert.token		 = (char *)json_object_get_string(tokenObj);
		temp_alert.type			 = (char *)json_object_get_string(typeObj);
		temp_alert.scheduledTime = (char *)json_object_get_string(scheduledTimeObj);
		temp_alert.stop 		 = json_object_get_int(stopObj);
		stop = temp_alert.stop;
		if ((0 == strcmp(temp_alert.type, TYPE_REMINDER)) || (-1 == json_object_get_int(loopCount)))
		{
			struct json_object *assetPlayOrder = NULL ,*backgroundAlertAsset = NULL;
			struct json_object *loopPauseInMilliseconds = NULL;

			assetPlayOrder          = json_object_object_get(obj , "assetPlayOrder");
			backgroundAlertAsset    = json_object_object_get(obj , "backgroundAlertAsset");
			loopCount               = json_object_object_get(obj , "loopCount");
			loopPauseInMilliseconds = json_object_object_get(obj , "loopPauseInMilliseconds");

			temp_alert.m_reminder.assetPlayOrder          = (char *)json_object_get_string(assetPlayOrder);
			temp_alert.m_reminder.backgroundAlertAsset    = (char *)json_object_get_string(backgroundAlertAsset);
			temp_alert.m_reminder.loopCount               = json_object_get_int(loopCount);
			temp_alert.m_reminder.loopPauseInMilliseconds = json_object_get_int(loopPauseInMilliseconds);
		}

		DEBUG_INFO("[%d] type:%s token:%s time:%s",i , temp_alert.type, temp_alert.token, temp_alert.scheduledTime);

		now = get_now_time_utc();
		//scheduled = alert_convert_scheduledtime_s(temp_alert.scheduledTime);
		scheduled = iso8601_to_UTC(temp_alert.scheduledTime);
		alert = alert_new(&temp_alert);
		alert_set_stop(alert,stop);
		DEBUG_INFO("expire:%ld", now-SECONDS_AFTER_PAST_ALERT_EXPIRES);

		// 只添加了前半个小时和后面的定时器?
		double ddifftime = difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES);
		if(ddifftime > 0) 
		{
			flag = 1;
			//alertmanager_add(manager, alert, true);
			alertmanager_readd(manager, alert, true);
			DEBUG_INFO("alertmanager_readd token:%s, type:%s,scheduledTime:%s,", temp_alert.token, temp_alert.type, temp_alert.scheduledTime);
		} 
		else 
		{
			DEBUG_INFO(" file_delete_line");
			list_add(droppedAlerts, alert);

			// 从ALARM_FILE中删除alert;
			alertmanager_drop(alert);
			//file_delete_line(token, CRONTAB_ALARM_FILE);
		}

	}

	if(flag == 0) 
	{
		remove(CRONTAB_ALARM_FILE);
	}

	return 0;
	
failed4:
json_object_put(all);
failed3:
close(fd);
free(allAlerts);
allAlerts = NULL;
failed2:
list_destroy(droppedAlerts);
droppedAlerts = NULL;
failed1:
list_destroy(alerts);
alerts = NULL;
failed:
return 1;
	
}

#endif







