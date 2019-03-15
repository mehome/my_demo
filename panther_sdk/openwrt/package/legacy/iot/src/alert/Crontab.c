#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <json/json.h>
#include <stdbool.h>


#include "Crontab.h"
#include "Alert.h"
#include "list_iterator.h"
#include "AlertManager.h"
#include "myutils/debug.h"
#include "common.h"

//#define USE_FD

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
#ifndef USE_FD
static int file_put_line(const char *src, FILE * stream, int flag)
{		
	size_t len,count;		
	len = strlen(src);				
	char *tmp="#";		
	char *enter="\n";		

	info("len %d ",len);			
	if(flag == 1) {			
		fwrite(tmp, strlen(tmp), 1,stream);		
	}		
	if((count = fwrite(src, 1, len, stream)) != len) {	
		info("count: %d ",count)	;
		info("fwrite %s failed",src);					
						
		return 1;	
	}		
	fwrite(enter, strlen(enter), 1,stream);		
	return 0;
}
#else
static int file_put_line(const char *src, int fd, int flag)
{		
	size_t len,count;		
	len = strlen(src);				
	char *tmp="#";		
	char *enter="\n";		

	info("len %d ",len);			
	if(flag == 1) {			
		write(fd , "#", 1);		
	}		
	if((count = write(fd  , src, len)) != len) {	
		info("count: %d ",count)	;
		info("fwrite %s failed",src);								
		return 1;	
	}		
	//fwrite(enter, strlen(enter), 1,stream);		
	write(fd, "\n", 1);		
	return 0;
}

#endif
#if 0
static int create_path(const char *path)
{
	char *start;
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	if (path[0] == '/')
		start = strchr(path + 1, '/');
	else
		start = strchr(path, '/');

	while (start) {
		char *buffer = strdup(path);
		buffer[start-path] = 0x00;

		if (mkdir(buffer, mode) == -1 && errno != EEXIST) {
			fprintf(stderr, "Problem creating directory %s", buffer);
			perror(" Problem creating directory");
			free(buffer);
			return -1;
		}
		free(buffer);
		start = strchr(start + 1, '/');
	}
	return 0;
}
#endif

static int safe_open(const char *name)
{
	int fd;

	fd = open(name, O_WRONLY | O_CREAT, 0644);
	if (fd == -1) {
		if (errno != ENOENT)
			return -1;
		if (create_path(name) == 0)
			fd = open(name, O_WRONLY | O_CREAT, 0644);
	}
	return fd;
}

static void  file_delete_line(char *token, char *file)
{	
		char cmd[256]={0};		
		snprintf(cmd,256,"sed -i '/\"%s\"/d' %s", token, file);	
		info("cmd:%s",cmd);
		exec_cmd(cmd);

        memset(cmd,0,256);
        snprintf(cmd,256,"sed -i '/\"%s/d' %s", "turing_152", file);
        exec_cmd(cmd);
        
		exec_cmd("/etc/init.d/crond restart");
        
}

void crontab_delete_alert(char *token)
{
	file_delete_line(token, CRONTAB_ALARM_FILE);
}

void crontab_stop_alert(char *token)
{
	char *tmp;
	char *str;
#if 0
	str = replace_str(token, "#", "*");
	if(str != NULL) {
		tmp = str;
	} else {
		tmp = token;
	}
	tmp = token;
#endif
	info("token:%s",token);			
	char cmd[256]={0};
	snprintf(cmd, 256,"sed -i 's/token=\"%s\".*;/token=\"%s\"; active=\"false\" ;/' "CRONTAB_ALARM_FILE, 
				tmp, tmp);	
	info("cmd:%s\n",cmd); 	
	exec_cmd(cmd);

	exec_cmd("/etc/init.d/crond restart");
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

	info("str:%s",tmp);	

	snprintf(cmd,256,"sed -i 's/token=\"%s\".*;/token=\"%s\"; active=\"true\" ;/' "CRONTAB_ALARM_FILE, 
				tmp, tmp);	
	DEBUG_INFO("cmd:%s\n",cmd); 	
	exec_cmd(cmd);
	exec_cmd("/etc/init.d/crond restart");
	
	if(str != NULL) 
		free(str);
}


#if 1
/* return 0 successd ,otherwise failed */
int crontab_wirte_to_disk(list_t *list)
{
	char *tmp;
	FILE *stream=NULL;
	list_iterator_t *it = NULL ;
	
	int min;
	int hour;
	int sec;
	int wday;
	int year;
	int mon;
	int day;

	//char buf[64]={0};
	//snprintf(buf, 64, "touch %s",CRONTAB_ALARM_FILE);
	//system(buf);
	//info("touch %s ok", CRONTAB_ALARM_FILE);
	
	
#ifdef USE_FD
	int fd = -1 ; 
	info("before open");
	fd = safe_open(CRONTAB_ALARM_FILE);		
	if(fd < 0) {	
		error("open failed");
		goto failed;
	}
#else
	info("before fopen");
	stream = fopen(CRONTAB_ALARM_FILE, "w+");
	if (NULL == stream) {
		error("fopen %s failed",CRONTAB_ALARM_FILE);
		goto failed1;
	}
#endif
	
	info("open %s ok", CRONTAB_ALARM_FILE);
	it = list_iterator_new(list,LIST_HEAD);
 
	while(list_iterator_has_next(it)) {
		int err;
		struct tm *time=NULL;
		const char *note = NULL;
		Alert *alert = NULL;
		char *token = NULL ;
		char * repate =NULL;
		char *scheduledTime  = NULL;
		char schedule[512]= {0};
		char cmd[256] = {0};
		alert = list_iterator_next_value(it);
		
		if(alert != NULL) {
				int  type ;
				token 			=  alert_get_token(alert);
				repate 			=  alert_get_repate(alert);
			
				scheduledTime 	=  alert_get_scheduledtime(alert);
				type 			=  alert_get_type(alert); 		
				info("token:%s, repate:%s, scheduledTime:%s, type:%d",token, repate, scheduledTime, type);
				json_object *val = NULL;

				val = json_object_new_object();

				json_object_object_add(val,"token",json_object_new_string(token));

				json_object_object_add(val,"repate", json_object_new_string(repate));

				json_object_object_add(val,"scheduledTime",json_object_new_string(scheduledTime));

				json_object_object_add(val,"type",json_object_new_int(type));

				note = json_object_to_json_string(val);
				info("note:%d :%s",strlen(note), note);
				
#ifdef USE_FD
				err = file_put_line(note , fd, 1);
#else
				err = file_put_line(note , stream, 1);
#endif
				if(err == 1) {
					error("put %s failed", note);
					json_object_put(val);
					goto failed;
				} else {
					info("put %s ok", note);
				}
				
				json_object_put(val);

				//alert_convert_scheduledtime_tm(alert,&time);
				time_t tm_t; 
				char tmp[100]={0};
				tm_t = atol(alert->scheduledTime);
				info("tm_t:%ld",tm_t);   
				time = localtime(&tm_t);
				strftime(tmp, sizeof(tmp), "%Y-%m-%d,%H:%M:%S:%u",time);
	
				sscanf(tmp, "%d-%d-%d,%d:%d:%d:%d", &year, &mon, &day,&hour, &min, &sec, &wday);
        
    			info ("\nTime: %s\n", tmp);
				//char *replace = NULL;
				/*replace = replace_str(token, "#", "*");
				if(replace != NULL)
					tmp = replace; 
				else 
					tmp = token;*/
				//ReplaceStr(token , "#", "*");
				//snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
				//	"echo $token start > ./alarm.test || echo $token cancle > ./alarm.test", token);
				
				if(type == ALERT_TYPE_ALARM) {
					snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
					"/usr/bin/alertfifo.sh \"$token true\" || /usr/bin/alertfifo.sh \"$token false\" ", token);
				} else if (type == ALERT_TYPE_SLEEP) {
					snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
					"/usr/bin/alertfifo.sh \"$token sleep\" || /usr/bin/alertfifo.sh \"$token false\" ", token);
				} else if (type == ALERT_TYPE_WAKEUP) {
					snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
					"/usr/bin/alertfifo.sh \"$token wakeup\" || /usr/bin/alertfifo.sh \"$token false\" ", token);
				} else if (type == ALERT_TYPE_TIMEOFF) {
					snprintf(cmd, 256 ,"token=\"%s\"; active=\"true\" ; [ \"${active}\" = \"true\" ] && "
					"/usr/bin/alertfifo.sh \"$token timeoff\" || /usr/bin/alertfifo.sh \"$token false\" ", token);
				}
				char *pRepate="*";
				if(repate == NULL || strcmp(repate, "") == 0 || strcmp(repate, "null") == 0) {
					snprintf(schedule, 512, "%d %d %d %d %d  %s", min ,hour,day, mon, wday, cmd);
				} else {
					snprintf(schedule, 512, "%d %d %s %s %s  %s", min ,hour,pRepate, pRepate, repate, cmd);
				}
				
				info("schedule:%s ", schedule);
#ifdef USE_FD
				err = file_put_line(schedule , fd, 0);
#else
				err = file_put_line(schedule , stream, 0);
#endif
				if(err == 1) {
					error("put %s failed", schedule);
					goto failed;
				} else {
					info("put %s ok", schedule);
				}

		}
		
	 }//end while(list_iterator_has_next(it))
	exec_cmd("/etc/init.d/crond restart");

	if(it)
		list_iterator_destroy(it);
#ifdef USE_FD
	close(fd);
#else
	fclose(stream);
#endif
	return 0;
failed1:
#ifdef USE_FD
	close(fd);
#else
	fclose(stream);
#endif
failed:
	if(it)
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



#if 1
/* return 0 successd ,otherwise failed */

//int crontab_load_from_disk()
int crontab_load_from_disk(AlertManager *manager)
{ 
			list_iterator_t *it = NULL;
			FILE *stream=NULL;
			char *line;
			char chars[256] = {0} ;
		 	list_t *alerts = NULL;
			list_t *droppedAlerts = NULL;

			if((access(CRONTAB_ALARM_FILE,F_OK))!=-1)  {
				info("%s is  exist" ,CRONTAB_ALARM_FILE);
			} else {
				info("%s is not exist" ,CRONTAB_ALARM_FILE);
				eval("touch","/etc/crontabs/root");
				return 0;
			}
			
			alerts = list_new();
			if(NULL == alerts) {
				error("list_new alerts failed ");
				goto failed;
			}
			droppedAlerts = list_new();
			if(NULL == droppedAlerts) {
				error("list_new droppedAlerts failed ");
				goto failed;
			}
		
			stream = fopen(CRONTAB_ALARM_FILE, "rb");
			if (NULL == stream) {
				error("fopen %s failed",CRONTAB_ALARM_FILE);
				goto failed;
			}

			while(1) {
				 json_object *val = NULL,*tokenObj=NULL ,*repateObj=NULL, *scheduledTimeObj=NULL;
				 json_object *typeObj = NULL,*promptObj=NULL;
				 Alert *alert = NULL;
				 char *token = NULL ;
				 char *repate = NULL;
				 char *scheduledTime = NULL;
				 char *iprompt = NULL;
				 int type;
				line = file_get_line(chars, stream);
				if(line == NULL) {
					break;
				}
				info("line=%s", line);
				
				if(line[0] == '#') {
					char *tmp = NULL;
					tmp = &line[1];
					info("tmp %s ", tmp);
					val = json_tokener_parse(tmp);
					if(val == NULL) {
						error("json_tokener_parse tmp");
						goto failed;
					}
					info("val %s ", json_object_to_json_string(val));

					tokenObj 		 = json_object_object_get(val, "token");
					repateObj 		 = json_object_object_get(val, "repate");
					scheduledTimeObj = json_object_object_get(val, "scheduledTime");
					typeObj 		 = json_object_object_get(val, "type");
					promptObj		 = json_object_object_get(val, "prompt");
					if(tokenObj == NULL || repateObj == NULL || scheduledTimeObj == NULL || typeObj  == NULL ||promptObj==NULL) {
						json_object_put(val);
						error("json_object_object_get token type scheduledTime error");
						goto failed;
					}
				
					token 			 = strdup(json_object_get_string(tokenObj));
					repate			 = strdup(json_object_get_string(repateObj));
					scheduledTime    = strdup(json_object_get_string(scheduledTimeObj));
					type 			 = json_object_get_int(typeObj);
					iprompt			 = strdup(json_object_get_string(promptObj));
					info("token:%s, scheduledTime:%s, repate:%s, type:%d,iprompt:%s", token, scheduledTime,repate, type,iprompt);
					alert = alert_new(token,scheduledTime,repate, type ,iprompt);
					if (NULL == alert) {
						if(token)
							free(token);
						if(repate)
							free(repate);
						if(scheduledTime)
							free(scheduledTime);
						if(iprompt)
							free(iprompt);
						json_object_put(val);
						error("alert_new  failed ");
						goto failed;
					}
					info("list_add");
					list_add(alerts, alert);
					info("after list_add");
					json_object_put(val);
					
				} //end if line[0]=="#"
				else {
					warning("line != #");
				}
				
			}// end while ((line =file_get_line(chars, stream)) != NULL) 
			info("close stream");
			fclose(stream);
			stream = NULL;
			it = list_iterator_new(alerts,LIST_HEAD);
			info("list_iterator_new");
			while(list_iterator_has_next(it)) {
					Alert *alert = NULL;
					char *token = NULL ;
					char *repate = NULL ;
					const char *scheduledTime = NULL;
					time_t scheduled, now;

					alert = list_iterator_next_value(it);

					if(NULL == alert) {
						error("list_iterator_next_value error");
						goto failed;
					}
#if 0
					token 			= alert_get_token(alert);
					scheduledTime 	= alert_get_scheduledtime(alert);
					repate			= alert_get_repate( alert);
					
					//scheduled = alert_convert_scheduledtime(alert);
					scheduled = atol(scheduledTime);
					info("scheduledTime :%ld",scheduled );
		
					now = get_now_time();

					info("now :%ld",now);	

					info("difftime:%f",difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES));
					if(difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES) > 0) {
							alertmanager_add(manager, alert, true);
							info("alertmanager_add token:%s, reapte:%s ,scheduledTime:%s", token, repate, scheduledTime);
					} else {
							info(" file_delete_line token:%s", token);
							//file_delete_line(token, CRONTAB_ALARM_FILE);
							alertmanager_delete(manager, token);
#else
					alertmanager_add(manager, alert, true);
#endif
						
				}// end while(list_iterator_has_next(it))
		
			list_iterator_destroy(it);
			list_destroy(alerts);
			list_destroy(droppedAlerts);
			
			eval("/etc/init.d/crond", "restart");
			return 0;
failed:
			info("failed");
			if( NULL != stream) {
				fclose(stream);
				stream = NULL;
			}

			if(NULL != alerts) {
				list_destroy(alerts);
				alerts = NULL;
			}

			if(NULL !=droppedAlerts) {
				list_destroy(droppedAlerts);
				droppedAlerts = NULL;
			}

			if(NULL != it) {
				list_iterator_destroy(it);
				it = NULL;
			}

			eval("/etc/init.d/crond", "restart");
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
			DEBUG_INFO("allAlerts: %s ",allAlerts)	;
			if(count < 0) {
				DEBUG_ERR("Readn error");
				goto failed3;
			}

			close(fd);

			all =  json_tokener_parse(allAlerts);
			if(NULL == all) {
				DEBUG_ERR("json_tokener_parse failed");
				goto failed3;
			}
			DEBUG_INFO("all:%s ", json_object_to_json_string(all));
			
			length = json_object_array_length(all);
			DEBUG_INFO("length:%d ", length);
			if(length <= 0 ) {
				DEBUG_ERR("json_object_array_length failed");
				goto failed4;
			}
			for ( i = 0; i < length ; i++) {
				Alert *alert = NULL;
				char *token = NULL ;
				AlertType type = NULL; 
				const char *scheduledTime = NULL;
				time_t scheduled, now;
				struct json_object *obj =  NULL ;
				struct json_object *tokenObj=NULL ,*typeObj=NULL, *scheduledTimeObj=NULL;
				obj = json_object_array_get_idx(all, i);  

	
				tokenObj		 = json_object_object_get(obj , "token");
				typeObj 		 = json_object_object_get(obj , "type");
				scheduledTimeObj = json_object_object_get(obj , "scheduledTime");
				
				
				token			 = (char *)json_object_get_string(tokenObj);
				type			 = (char *)json_object_get_string(typeObj);
				scheduledTime	 = (char *)json_object_get_string(scheduledTimeObj);
				DEBUG_INFO("[%d] token:%s time:%s",i , token, scheduledTime);

				now = get_now_time();
				scheduled = alert_convert_scheduledtime_s(scheduledTime);
				alert = alert_new(strdup(token),strdup(type),strdup(scheduledTime));
				
				DEBUG_INFO("now :%ld",now);	
				DEBUG_INFO("scheduled:%ld",scheduled);
				DEBUG_INFO("expire:%ld", now-SECONDS_AFTER_PAST_ALERT_EXPIRES);
				DEBUG_INFO("difftime:%f",difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES));
				if(difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES) > 0) {
						flag = 1;
						alertmanager_add(manager, alert, true);
						DEBUG_INFO("alertmanager_add token:%s, type:%s,scheduledTime:%s,", token, type, scheduledTime);
				} else {
						DEBUG_INFO(" file_delete_line");
						list_add(droppedAlerts, alert);
							//从ALARM_FILE中删除alert;
						alertmanager_drop(alert);
						//file_delete_line(token, CRONTAB_ALARM_FILE);
				}

			}

			if(flag == 0) {
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








