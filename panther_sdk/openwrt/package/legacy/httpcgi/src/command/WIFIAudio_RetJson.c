#include <json/json.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WIFIAudio_RetJson.h"

int WIFIAudio_RetJson_retJSON(char *pstring)
{
	int ret = 0;
	char *pjsonstring = NULL;
	struct json_object *object;
	
	if(NULL == pstring)
	{
		ret = -1;
	} else {
		object = json_object_new_object();
		json_object_object_add(object, "ret", json_object_new_string(pstring));
		
		pjsonstring = (char *)calloc(1, strlen(json_object_to_json_string(object)) + 1);
		if(pjsonstring==NULL)
		{
			json_object_put(object);
			ret = -1;
		} else {
			strcpy(pjsonstring,json_object_to_json_string(object));
			json_object_put(object);
			printf("%s", pjsonstring);
			free(pjsonstring);
			pjsonstring = NULL;
		}
	}
	
	return ret;
}
