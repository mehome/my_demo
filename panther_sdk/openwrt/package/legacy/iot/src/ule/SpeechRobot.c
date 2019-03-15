#include <stdio.h>
#include <curl/curl.h>
#include <json/json.h>
#include <sys/stat.h>  
#include <myutils/mystring.h>

#include "post.h"
#include "param.h"
#include "myutils/debug.h"


static char *INTELLIGENT_ANSWER_SERVER_URL = "http://botapi.maizuan.mobi/forward/api/rest/router/";


#if 0
int SpeechRobot(char *question, char *file) 
{	
	int ret = -1;
	char *body = NULL;
	RobotData *data = NULL;

	
	body = calloc(1, 2048);
	if(body == NULL)
		return -1;
	
	ret = getSign(question, file, body, 2048);
	if(ret != 0)
		goto exit;
	
	data = newRobotData();
	
	ret = SpeechRobotPost(INTELLIGENT_ANSWER_SERVER_URL, body, NULL, data);
	if(ret != 0 || data->code != 0) {
		ret = -1;
		goto exit;
	}

	info("data->buf:%s", data->buf);
#ifdef ENABLE_ULE_IFLYTEK
	StartIflytek(data->buf);
#else
	ret = TuringTTs(data->buf);
#endif

exit:
	if(body)
		free(body);
	freeRobotData(data);
	
	return ret;
}
#else

static char *JsonParser(char *pData)
{
	int ret = 0;
	json_object *root = NULL;
	json_object *val = NULL;
	int successCount = 0;
	int errorCount = 0;
	char *content = NULL;
	char *data = NULL;

	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		error("json_tokener_parse failed");
		return -1;
	}
	info("pData:%s",pData);
	val = json_object_object_get(root, "successCount");
	if(val == NULL)  {
		ret = -1;
		goto exit;
	}
	successCount = json_object_get_int(val);	
	if(successCount <= 0)
		goto exit;

	val = json_object_object_get(root, "content");
	if(val == NULL)  {
		ret = -1;
		goto exit;
	}
	content = json_object_get_string(val);
	if( strstr(content, "这个小助手不清楚") ) {
		ret = -1;
		goto exit;
	}
	data = strdup(content);
	
	val = json_object_object_get(root, "errorCount");
	if(val) {
		errorCount = json_object_get_int(val);
		info("errorCount:%d",errorCount);
	}

	val = json_object_object_get(root, "errInfoList");
	if(val) {
		info("errInfoList:%s",json_object_to_json_string(val));
	}
	
exit:
	json_object_put(root);
	return data;
}

int SpeechRobot(char *question, char *file) 
{	
	int ret = -1;
	char *body = NULL;
	mystring *data = NULL;
	char *json = NULL;
	char *text = NULL;
	int len = 0;
	body = calloc(1, 2048);
	if(body == NULL)
		return -1;
	
	ret = getSign(question, file, body, 2048);
	if(ret != 0) {
		ret = -1;
		goto exit;
	}
	data = string_new();
	
	if(data == NULL) {
		ret = -1;
		goto exit;
	}
	if(file)
		ret = PostFile(INTELLIGENT_ANSWER_SERVER_URL, body, file, data);
	else
		ret = PostQuestion(INTELLIGENT_ANSWER_SERVER_URL, body, NULL, data);
	if (ret != 0) {
		error("PostQuestion error:%d",ret);
		goto exit;
	}
	if (string_empty(data)) {
		error("data is empty");
		ret = -1;
		goto exit;
	} 
	
	json = (char *)string_data(data, &len);
	info("data->buf:%s", json);
	text = JsonParser(json);
	if(text == NULL) {
		error("JsonParser error");
		ret = -1;
		goto exit;
	} 
	
	info("text:%s", text);
#ifdef ENABLE_ULE_IFLYTEK
	StartIflytek(text);
#else
	ret = TuringTTs(text);
#endif

exit:
	string_free(data);
	if(body)
		free(body);
	if(text)
		free(text);
	return ret;
}

#endif

