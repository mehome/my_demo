#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <json/json.h>

#include "list_iterator.h"

#include "AlertManager.h"
#include "Alert.h"





/* return 0 successd ,otherwise failed */
int alertfiledatastore_wirte_to_disk(list_t *list)
{

	struct json_object *array = NULL;
	
	int fd, len; 
	const char *str = NULL;
	list_iterator_t *it = NULL ;

	//DEBUG_INFO("...");
	it = list_iterator_new(list,LIST_HEAD);
 
	array = json_object_new_array();
	if(NULL == array) { 
		json_object_put(array);
		DEBUG_ERR("json_object_new_object failed");
		return 1;
	}
	
	while(list_iterator_has_next(it)) {
		Alert *alert = NULL;
		char *token = NULL ;
		char *type = NULL;
		
		alert =  list_iterator_next_value(it);
		token = alert_get_token(alert);
		type = alert_get_type(alert);
		struct json_object *val;
		val = json_object_new_object();

		json_object_object_add(val,"token",json_object_new_string(token));
		json_object_object_add(val,"type", json_object_new_string(type));
		json_object_object_add(val,"scheduledTime",json_object_new_string("scheduledTime"));
		str = json_object_to_json_string(val) ;
		DEBUG_INFO("val :%s",str);
		json_object_array_add(array,val);
	} //end while(list_iterator_has_next(it))
	str = json_object_to_json_string(array);
	len = strlen(str);
	//DEBUG_INFO("array : %d :%s",len, json_object_to_json_string(val));

	list_iterator_destroy(it);

	fd = open(ALARM_FILE, O_WRONLY|O_CREAT);
	if(fd < 0) {
			DEBUG_INFO("open :%s failed",ALARM_FILE );
			return 1;
	}

	if(write(fd, str, len) != len){
		close(fd);
		return 1;
	}
	json_object_put(array);
	close(fd);
	
	return 0;
	
}

#if 0
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

			//DEBUG_INFO("...");
			alerts = list_new();
			if(NULL == alerts) {
				DEBUG_ERR("list_new alerts failed \n");
				return 1;
			}
			droppedAlerts = list_new();
			if(NULL == droppedAlerts) {
				DEBUG_ERR("list_new droppedAlerts failed \n");
				return 1;
			}
		
			fd = open(ALARM_FILE, O_RDONLY);		
			if(fd < 0) {	
				DEBUG_ERR("open failed \n");
				return 1;
			}
			if(fstat(fd, &sbuf) < 0) {	
				DEBUG_INFO("fstat failed \n");	
				return 1;
			}
			copysz = sbuf.st_size;

			str = calloc(copysz, 1);
			if(str == NULL ){
				DEBUG_INFO("calloc failed \n");	
				close(fd);
				return 1;
			}
			char *tmp = str;
			while((ret = read(fd, buf, 512)) > 0) {
				memcpy(str, buf, ret);
				tmp = tmp + ret;
			}
			DEBUG_INFO("read from file :%s",str);

			array = json_tokener_parse(str);
			if(NULL == array) {
				DEBUG_ERR("json_tokener_parse to array failed");
				return 1;
			}
			len = json_object_array_length(array);

			for (i = 0; i < len; i++) {
				struct json_object *val = NULL,*tokenObj=NULL ,*typeObj=NULL, *scheduledTimeObj=NULL;
				Alert *alert = NULL;
				char *token = NULL ;
				AlertType type = NULL; 
				char *scheduledTime = NULL;
				
				val = json_object_array_get_idx(array, i);
				if (NULL == val) {
					DEBUG_ERR("json_object_array_get_idx %d failed", i);
					return 1;
				}
				
				DEBUG_INFO("val: %s",json_object_to_json_string(val));

				tokenObj 		 = json_object_object_get(val, "token");
				typeObj 		 = json_object_object_get(val, "type");
				scheduledTimeObj = json_object_object_get(val, "scheduledTime");
				
				
				token 			 = strdup(json_object_to_json_string(tokenObj));
				type 			 = strdup(json_object_to_json_string(typeObj));
				scheduledTime    = strdup(json_object_to_json_string(scheduledTimeObj));
				
				alert = alert_new(token,type,scheduledTime, NULL, 0, 0);
				if (NULL == alert) {
					DEBUG_ERR("alert_new  failed ");
					return 1;
				}

				time_t scheduled, now;
				scheduled = alert_convert_scheduledtime(alert);
				now 		  = get_now_time();
				
				if(difftime(scheduled, now-SECONDS_AFTER_PAST_ALERT_EXPIRES)) {
					alertmanager_add(manager, alert,true);
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
			
}
#endif



