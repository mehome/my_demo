#include "TuringTTs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <json/json.h>
#include "TtsHttp.h"
#include "myutils/debug.h"
#include "common.h"
#include "TuringParam.h"
#include "request_and_response.h"

static char ttsToken[33] = "0";

#define TTS_URL  "http://smartdevice.ai.tuling123.com/speech/v2/tts"

static char *CreateTuringTtsJson(char *key, char *uid,char *token, char *text, TuringTTsParam *param)
{
	char * json_str = NULL;
	json_object *root=NULL, *parameters = NULL;
	int battery;
	root = json_object_new_object();
	
	parameters  = json_object_new_object();
	
	json_object_object_add(parameters, "ak", json_object_new_string(key));
	json_object_object_add(parameters, "uid", json_object_new_string(uid));
	json_object_object_add(parameters, "token", json_object_new_string(token));
	json_object_object_add(parameters, "text", json_object_new_string(text));
	
	json_object_object_add(parameters, "tts", json_object_new_int(param->tts));
	json_object_object_add(parameters, "tts_lan", json_object_new_int(param->tts_lan));
	json_object_object_add(parameters, "speed", json_object_new_int(param->speed));
	json_object_object_add(parameters, "pitch", json_object_new_int(param->pitch));
	json_object_object_add(parameters, "tone", json_object_new_int(param->tone));
	json_object_object_add(parameters, "volume", json_object_new_int(param->volume));

	json_object_object_add(root, "parameters", parameters);
	json_str = strdup(json_object_to_json_string(root));

	json_object_put(root);

	return json_str;
}



static int ParseTuringTtsJson(char *json ,char *token)
{
	
	json_object *root = NULL, *value = NULL, *payload = NULL;
	int code = -1;
	int ret = 0;
	int len, i;
	info("json:%s",json);
	
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	value = json_object_object_get(root, "token");
	if(value) {
		char *tmp = NULL;
		tmp = json_object_get_string(value);
		memcpy(token, tmp, strlen(tmp));
	}	
	value = json_object_object_get(root, "code");
	if(value) {
		code =  json_object_get_int(value);
	}	
	
	if(code != 200) {
		ret = -1;
		goto exit;
	}

	value = json_object_object_get(root, "url");
	if(value == NULL) {
		ret = -1;
		goto exit;
	}
	len = json_object_array_length(value);
	if(len <= 0) {
		ret = -1;
		goto exit;
	}
	MpdClear();
	MpdRepeat(0);
	MpdRandom(0);
	MpdSingle(0);
	for (i = 0; i < len; i++)
	{
		json_object *index = NULL;
		index = json_object_array_get_idx(value, i);
		if (index != NULL)
		{
			char *url =  NULL;
			url = json_object_get_string(index);
			MpdAdd(url);
		}
	}
	MpdPlay(0);
exit:
	if(root)
		json_object_put(root);
	return ret;
}

static int request_tts(char *url,char *data, mystring *file_content)
{

	 int status_code = post(url,  data, file_content);
	 if (status_code != CURLE_OK) 
     {
        wav_play2("/root/iot/网络超时.wav");
		 return -1;
	 }

	 if (string_empty(file_content)) {
		 //接口返回错误信息
		 return -1;
	 } 
	 return 0;
}

int TuringTTs(char *text)
{
	int ret = 0;
	char *json = NULL;
	char *token = NULL;
	char *apiKey = NULL;
	char *uid = NULL;
	TuringTTsParam param;
	
	mystring *response = NULL;
	
	memset(&param , 0 , sizeof(TuringTTsParam));

	param.tts     = 1;
	param.tts_lan = 0;
	param.speed   = 5;
	param.pitch   = 5;
	param.tone 	  = GetTuringTone();
	param.volume  = 9;
	
	apiKey = GetTuringApiKey();
	uid = GetTuringUserId();
	token = ttsToken;
	
	json = CreateTuringTtsJson(apiKey ,uid, token, text, &param);
	if(json == NULL) {
		ret = -1;
		goto exit;
	}
    
	response = string_new();
	if(response == NULL) {
		ret = -1;
		goto exit;
	}
	info("json:%s", json);
	ret = request_tts(TTS_URL , json, response);
	if(ret != 0)
		goto exit;
	
	int len = 0;
	char *data = NULL;
	data = (char *)string_data(response, &len);
	info("len:%d",len);
	info("data:%s",data);
	ret = ParseTuringTtsJson(data, ttsToken);
exit:
	if(json)
		free(json);
	
	string_free(response);
	
	return ret;
	
}


static char * ParseJson(char *json ,char *token)
{
	
	json_object *root = NULL, *value = NULL, *payload = NULL;
	int code = -1;
	int ret = 0;
	int len, i;
	info("json:%s",json);
	char *purl = NULL;
    
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	value = json_object_object_get(root, "token");
	if(value) {
		char *tmp = NULL;
		tmp = json_object_get_string(value);
		memcpy(token, tmp, strlen(tmp));
	}	
	value = json_object_object_get(root, "code");
	if(value) {
		code =  json_object_get_int(value);
	}	
	
	if(code != 200) {
		ret = -1;
		goto exit;
	}

	value = json_object_object_get(root, "url");
	if(value == NULL) {
		ret = -1;
		goto exit;
	}
	len = json_object_array_length(value);
	if(len <= 0) {
		ret = -1;
		goto exit;
	}
	for (i = 0; i < len; i++)
	{
		json_object *index = NULL;
		index = json_object_array_get_idx(value, i);
		if (index != NULL)
		{
			char *url =  NULL;
			url = json_object_get_string(index);
            purl = strdup(url);
		}
	}
exit:
	if(root)
		json_object_put(root);
    
	return purl;
}


#ifdef USE_CURL
char * TuringGetTTsUrl(char *text)
{
	int ret = 0;
	char *json = NULL;
	char *token = NULL;
	char *apiKey = NULL;
	char *uid = NULL;
	TuringTTsParam param;
    char *purl = NULL;
	
	mystring *response = NULL;
	
	memset(&param , 0 , sizeof(TuringTTsParam));

	param.tts     = 1;
	param.tts_lan = 0;
	param.speed   = 5;
	param.pitch   = 5;
	param.tone 	  = GetTuringTone();
	param.volume  = 9;
	
	apiKey = GetTuringApiKey();
	uid = GetTuringUserId();
	token = ttsToken;
	
	json = CreateTuringTtsJson(apiKey ,uid, token, text, &param);
	if(json == NULL) {
		ret = -1;
		goto exit;
	}
	response = string_new();
	if(response == NULL) {
		ret = -1;
		goto exit;
	}
	info("json:%s", json);
	ret = request_tts(TTS_URL , json, response);
	if(ret != 0)
		goto exit;
	
	int len = 0;
	char *data = NULL;
	data = (char *)string_data(response, &len);
	info("len:%d",len);
	info("data:%s",data);
	purl = ParseJson(data, ttsToken);
exit:
	if(json)
		free(json);
	
	string_free(response);
	
	return purl;
	
}
#else
char * TuringGetTTsUrl(char *text)
{
	int ret = 0;
	char *json = NULL;
	char *token = NULL;
	char *apiKey = NULL;
	char *uid = NULL;
	TuringTTsParam param;
    char *purl = NULL;
    int fd = 0;
	char *host="smartdevice.ai.tuling123.com";
	char *input_text = NULL;
	memset(&param , 0 , sizeof(TuringTTsParam));

	param.tts     = 1;
	param.tts_lan = 0;
	param.speed   = 5;
	param.pitch   = 5;
	param.tone 	  = GetTuringTone();
	param.volume  = 9;
	
	apiKey = GetTuringApiKey();
	uid = GetTuringUserId();
	token = ttsToken;
	
	json = CreateTuringTtsJson(apiKey ,uid, token, text, &param);
	if(json == NULL) {
		ret = -1;
		goto exit;
	}
	debug("json:%s", json);
    fd = get_socket_fd(host);
    if(fd < 0)
        return NULL;
	ret = build_turing_tts_request(fd, host, json);
	if(ret < 0)
		goto exit;
	getResponse(fd, &input_text);
	purl = ParseJson(input_text, ttsToken);
exit:
	if(json)
		free(json);
	
	
	return purl;
	
}

#endif











