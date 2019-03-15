#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>


#include "alipal_https_request.h"
#include "alipal_parse_voice.h"
#include "pal.h"


int AliPal_Parse_Voice(char *asr_cmd)
{
	int fail = 1;

	struct json_object *asr_root_obj = NULL, *asr_method_obj = NULL; 
	struct json_object *array_params_obj = NULL, *array_url_obj = NULL;
	struct json_object *array_title_obj = NULL, *array_artist_obj = NULL;
	struct json_object *array_image_obj = NULL, *array_vendor_obj = NULL;
	
	if(asr_cmd == NULL)
		printf("asr_cmd is null\n");
	
	asr_root_obj = json_tokener_parse(asr_cmd);
	if(is_error(asr_root_obj))
	{
		printf("oh, my god. json_tokener_parse err\n");  
		goto JS_OBJ_ERROR;
	}
	
	asr_method_obj = json_object_object_get(asr_root_obj, "method"); //获取目标对象
	if(asr_method_obj == NULL)
	{
		printf("asr_method_obj is null\n");
		goto JS_OBJ_ERROR;
	}

	array_params_obj = json_object_object_get(asr_root_obj, "params");
	if(array_params_obj == NULL)
	{
		printf("array_params_obj is null\n");
		goto JS_OBJ_ERROR;
	}
	
	array_url_obj = json_object_object_get(array_params_obj, "uri");
	if(array_url_obj == NULL)
	{
		printf("array_url_obj is null\n");
		goto JS_OBJ_ERROR;
	}
	else
	{
		char ttsbuf[512] = {0};
		char *url_string = NULL;

		system("mpc clear");

		url_string = strdup(json_object_get_string(array_url_obj));

		sprintf(ttsbuf, "mpc add %s", url_string);
		
		free(url_string);

		system(ttsbuf);
		
		printf("parse json ok url:%s\n", ttsbuf);

		system("mpc play");
	}

	fail = 0;

JS_OBJ_ERROR:
	json_object_put(asr_root_obj); 	////对总对象释放
		
OTHER_ERROR:
	if(fail)return -1;	

	return 0;
	
}

int AliPal_Parse_tts(struct pal_rec_result *ttsresult)
{
	char playbuf[512] = {0};
	
	system("mpc clear");
	
	sprintf(playbuf, "mpc add \'%s\'", ttsresult->tts);
	
	system(playbuf);
	
	printf("parse json ok url:%s\n", playbuf);

	system("mpc play");
	
	return 0;

}



