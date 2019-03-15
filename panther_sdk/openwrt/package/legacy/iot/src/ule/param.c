
#include "param.h"

#include <stdio.h>
#include <curl/curl.h>
#include <json/json.h>
#include <sys/stat.h>  

#include <fcntl.h>
#include "myutils/debug.h"
#include "common.h"

static char *INTELLIGENT_ANSWER_SERVER_URL = "http://botapi.maizuan.mobi/forward/api/rest/router/";

static char* CLIENT_ID_NAME = "ClientId";
static char* CLIENT_ID_VALUE = "sumenet_54d1c89ee30a29bfcd36cd36dd34rrs";


static char* SECRET_KEY = "sdsdKJFiw7821rhgIGTYJkfg34ee";

static char* APP_KEY_NAME  = "appKey";
static char* APP_KEY_VALUE = "buyplus_sdHDfewfHNJHFW3124235";

static char* APP_VER_NAME  = "app_ver";
static char* APP_VER_VALUE = "2.1.3";

static char* DEVICEID_NAME = "deviceId";


static char* FILEITEM_NAME = "fileItem";


static char* FORMAT_NAME = "format";
static char* FORMAT_VALUE = "json";

static char* METHOD_NAME = "method";
static char* METHOD_GET_ANSWER = "robot.intelligence.customer";


static char* QUESTION_NAME = "question";


static char* SESSION_KEY_NAME = "sessionKey";


static char* SIGN_NAME = "sign";

static char* SMAC_NAME = "smac";

static char* TIME_NAME  ="timestamp";


static char* VER_NAME = "ver";
static char* VER_VALUE = "1.0";

static char*  CONTENT_TYPE_NAME = "ContentType";
static char* CONTENT_TYPE_VALUE = "application/x-www-form-urlencoded;charset=utf-8";


static int getTime(char *data) 
{
	time_t now;
	struct tm *tm_now;

	time(&now);
	tm_now = localtime(&now);
	sprintf(data, "%d-%02d-%02d%02d:%02d:%02d", 1900+tm_now->tm_year, tm_now->tm_mon, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
}

RobotData *newRobotData()
{	
	RobotData *data = calloc(1, sizeof(RobotData));
	if(data == NULL)
		return NULL;
	data->buf 	=  NULL;
	data->code 	=  -1;
	return data;
}
void freeRobotData(RobotData *data)
{
	if(data) {
		
		if(data->buf) 
			free(data->buf);
		
		free(data);
	}
}

void setRobotDataCode(RobotData *data, int code)
{
	data->code = code;
}
void setRobotDataBuf(RobotData *data, char *buf)
{
	data->buf = buf;
}

int   getSign(char *question, char *file, char *body, int len) 
{
	//char sign[1024] = {0};
	//char body[1024] = {0};
	char *sign = NULL;
	char deviceIdValue[64] = {0};	
	char md5[64] = {0};
	char time[64] = {0};
	sign = malloc(1024);
	if(sign == NULL)
		return -1;
	
	memset(sign, 0, 1024);
	memset(body, 0, 1024);
	memset(deviceIdValue, 0, 64);
	memset(md5, 0, 64);
	GetDeviceID(deviceIdValue);
	
	memcpy(sign,SECRET_KEY , strlen(SECRET_KEY));


	strcat(sign, APP_KEY_NAME);//'appKey'
	strcat(sign, APP_KEY_VALUE);
	
	strcat(sign, APP_VER_NAME);//'app_ver'
	strcat(sign, APP_VER_VALUE);
	
	strcat(sign, DEVICEID_NAME);//'deviceId'
	strcat(sign, deviceIdValue);
	
	if(file) {
		info("file not null ");
		strcat(sign, FILEITEM_NAME);//'fileItem'
		strcat(sign, file);
		//strcat(sign, "wav");
	}
	strcat(sign, FORMAT_NAME); //'format'
	strcat(sign, FORMAT_VALUE);
	
	strcat(sign, METHOD_NAME); //'method'
	strcat(sign, METHOD_GET_ANSWER);
	
	if(question) {
		info("question not null ");
		strcat(sign, QUESTION_NAME); //'question'
		strcat(sign, question);
	}

	getTime(time);
	strcat(sign, TIME_NAME); 
	strcat(sign, time); 
	info("time:%d",strlen(time));
	info("time:%s",time);

	
	strcat(sign, VER_NAME);//'app_ver'
	strcat(sign, VER_VALUE);
	
	strcat(sign, SECRET_KEY);
	info("sign:%d",strlen(sign));
	info("sign:%s",sign);
	getMd5(md5, sign);
	info("md5:%s",md5);
	if(file) {
		info("file not null ");
		snprintf(body, len,"appKey=%s&app_ver=%s&deviceId=%s&fileItem=%s&format=%s&method=%s&sign=%s&timestamp=%s&ver=%s",
			APP_KEY_VALUE,APP_VER_VALUE,  deviceIdValue, file, FORMAT_VALUE, METHOD_GET_ANSWER, md5, time, VER_VALUE);
	}
	if(question) {
		info("question not null ");
		snprintf(body, len,"appKey=%s&app_ver=%s&deviceId=%s&format=%s&method=%s&question=%s&sign=%s&timestamp=%s&ver=%s",
			APP_KEY_VALUE,APP_VER_VALUE,  deviceIdValue, FORMAT_VALUE,METHOD_GET_ANSWER, question, md5, time, VER_VALUE);
	}
	info("body:%s",body);
	//SpeechRobotPost(INTELLIGENT_ANSWER_SERVER_URL,body,NULL);
	if(sign)
		free(sign);
	return 0;
}

//int SpeechRobot(char *question, char *file) 
//{	
//	char body[1024] = {0};
//	RobotData *data = NULL;
//	getSign(question, file, body);
	
//	data = newRobotData();
//	
//	SpeechRobotPost(INTELLIGENT_ANSWER_SERVER_URL,body,NULL, data);
//}













