#include <json/json.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "WIFIAudio_RetJson.h"

int WIFIAudio_RetJson_retJSON(char *pstring)
{
	int ret = 0;
	char *pjsonstring = NULL;
	struct json_object *object;
	
	if(NULL == pstring)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		object = json_object_new_object();
		json_object_object_add(object, "ret", json_object_new_string(pstring));
		
		pjsonstring = (char *)calloc(1, strlen(json_object_to_json_string(object)) + 1);
		if(pjsonstring==NULL)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			json_object_put(object);
			ret = -1;
		} else {
			strcpy(pjsonstring,json_object_to_json_string(object));
			json_object_put(object);

#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pjsonstring);
#endif
			
			printf("%s", pjsonstring);
			
			free(pjsonstring);
			pjsonstring = NULL;
		}
	}
	
	return ret;
}
