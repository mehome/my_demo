#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <json/json.h>
#include <stdbool.h>

#include "list_iterator.h"

#include "AlertManager.h"
#include "Alert.h"





/* return 0 successd ,otherwise failed */
int alertfiledatastore_wirte_to_disk(list_t *list)
{

	struct json_object *array = NULL;
	
	int fd = -1, len; 
	const char *str = NULL;
	list_iterator_t *it = NULL ;

	it = list_iterator_new(list,LIST_HEAD);
 
	array = json_object_new_array();
	if(NULL == array) { 
		error("json_object_new_object failed");
		goto failed;
	}
	
	while(list_iterator_has_next(it)) {
		Alert *alert = NULL;
		char *token = NULL ;
		char *repate = NULL;
		char *scheduledtime = NULL;
		alert =  list_iterator_next_value(it);
		token = alert_get_token(alert);
		repate = alert_get_repate(alert);
		scheduledtime = alert_get_scheduledtime(alert);
		struct json_object *val;
		val = json_object_new_object();

		json_object_object_add(val,"token",json_object_new_string(token));
		json_object_object_add(val,"repate", json_object_new_string(repate));
		json_object_object_add(val,"scheduledTime",json_object_new_string(scheduledtime));
		str = json_object_to_json_string(val) ;
		info("val :%s",str);
		json_object_array_add(array,val);
	} 
	str = json_object_to_json_string(array);
	len = strlen(str);
	
	fd = open(ALARM_FILE, O_WRONLY|O_CREAT);
	if(fd < 0) {
			error("open :%s failed",ALARM_FILE );
			goto failed;
	}

	if(write(fd, str, len) != len){
		error("write :%s failed",ALARM_FILE );
		goto failed;
	}
	json_object_put(array);
	list_iterator_destroy(it);
	close(fd);
	return 0;
failed:
	if(array)
		json_object_put(array);
	if(fd > 0)
		close(fd);
	if(it)
		list_iterator_destroy(it);
	return 1;
	
}


/* return 0 successd ,otherwise failed */
int alertfiledatastore_load_from_disk(AlertManager *manager)
{
			int fd,ret,len, i;	
			size_t copysz ;		
			struct stat sbuf;
			char *str = NULL;
			char buf[512]={0};
			struct json_object *array = NULL;
			list_t *alerts = NULL;
			list_t * droppedAlerts = NULL;
			
			alerts = list_new();
			if(NULL == alerts) {
				error("list_new alerts failed ");
				goto failed;
			}
			droppedAlerts = list_new();
			if(NULL == droppedAlerts) {
				error("list_new droppedAlerts failed");
				goto failed;
			}
		
			fd = open(ALARM_FILE, O_RDONLY);		
			if(fd < 0) {	
				error("open failed");
				goto failed;
			}
			if(fstat(fd, &sbuf) < 0) {	
				error("fstat failed");	
				goto failed;
			}
			copysz = sbuf.st_size;

			str = calloc(copysz, 1);
			if(str == NULL ){
				error("calloc failed");	
				goto failed;
			}
			char *tmp = str;
			while((ret = read(fd, buf, 512)) > 0) {
				memcpy(str, buf, ret);
				tmp = tmp + ret;
			}
			info("read from file :%s",str);

			array = json_tokener_parse(str);
			if(NULL == array) {
				error("json_tokener_parse to array failed");
				goto failed;
			}
			len = json_object_array_length(array);

			for (i = 0; i < len; i++) {
				json_object *val = NULL,*tokenObj=NULL ,*repateObj=NULL, *scheduledTimeObj=NULL;
				json_object *typeObj = NULL;
				Alert *alert = NULL;
				char *token = NULL ;
				char * repate = NULL; 
				char *scheduledTime = NULL;
				int type; 
				val = json_object_array_get_idx(array, i);
				if (NULL == val) {
					error("json_object_array_get_idx %d failed", i);
					goto failed;
				}
				
				error("val: %s",json_object_to_json_string(val));

				tokenObj 		 = json_object_object_get(val, "token");
				repateObj 		 = json_object_object_get(val, "repate");
				scheduledTimeObj = json_object_object_get(val, "scheduledTime");
				typeObj 		 = json_object_object_get(val, "type");
				
				token 			 = strdup(json_object_get_string(tokenObj));
				repate 			 = strdup(json_object_get_string(repateObj));
				scheduledTime    = strdup(json_object_get_string(scheduledTimeObj));
				type 		     = json_object_get_int(typeObj);
		
				alert = alert_new(token,scheduledTime ,repate,type ,strdup("null"));
				if (NULL == alert) {
					if(token)
						free(token);
					if(repate)
						free(repate);
					if(scheduledTime)
						free(scheduledTime);
					error("alert_new  failed ");
					goto failed;
				}

				time_t scheduled, now;
				scheduled = alert_convert_scheduledtime(alert);
				now 	  = get_now_time();
				
				if(difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES)) {
					alertmanager_add(manager, alert, true);
				} else {
					list_add(droppedAlerts, alert);
				}

			}//end for (i = 0; i < len; i++) 

			// Now that all the valid alerts have been re-added to the alarm manager,
            // go through and explicitly drop all the alerts that were not added

			list_iterator_t *it = NULL;
			it = list_iterator_new(droppedAlerts,LIST_HEAD);
			while(list_iterator_has_next(it)) {
				Alert *alert = NULL;
				alert = list_iterator_next_value(it);
				if(NULL != alert)
					alertmanager_drop(alert);
				
			} //end while(list_iterator_has_next(it))

			json_object_put(array);
			close(fd);
			free(str);
			str = NULL;

			list_destroy(alerts);
			list_destroy(droppedAlerts);
			return 0;
	failed:
			info("failed");
			if(fd > 0) {
				close(fd);
				fd = -1;
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

			//eval("/etc/init.d/crond", "restart");
			return 1;
			
			
}




