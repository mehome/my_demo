#include "huabei.h"

#include "HuabeiEvent.h"
#include "HuabeiUart.h"


#include "debug.h"

static char *g_apiKey        = "of6dohiwn38eifq30jqrzq0rhcsdxolo";
static	char *g_aesKey = "3fcfbea64174456b";

#ifndef USE_IOT_MSG

int HuabeiMonitorControl(char *cmd)
{
	info("cmd:%s",cmd);
	if (0 == strncmp(cmd, HUABEI_STATUS_REPORT, sizeof(HUABEI_STATUS_REPORT) - 1))  
	{
		HuabeiStatusReport(cmd);	
	} 
	else if (0 == strncmp(cmd, HUABEI_TEST, sizeof(HUABEI_TEST) - 1))  
	{
		HuabeiTest(cmd);	
	}
	else if (0 == strncmp(cmd, HUABEI_SELECT_SUBJECT, sizeof(HUABEI_SELECT_SUBJECT) - 1))  
	{
		HuabeiSelectSubject(cmd);	
	}
	else if (0 == strncmp(cmd, HUABEI_LOCAL, sizeof(HUABEI_LOCAL) - 1))  
	{
		HuabeiPlayLocal(cmd);	
	}	
	else if (0 == strncmp(cmd, CAPTURE_END, sizeof(CAPTURE_END) - 1))  
	{
		TuringCaptureEnd();	
	}
	else if (0 == strncmp(cmd, SPEECH_START, sizeof(SPEECH_START) - 1)) 
	{		
		error("SPEECH_START");
		TuringSpeechStart(0);
	} 
	else if (0 == strncmp(cmd, CHAT_START, sizeof(CHAT_START) - 1)) 
	{	
		TuringChatStart();
	}
	else if (0 == strncmp(cmd, CHAT_PLAY, sizeof(CHAT_PLAY) - 1)) 
	{
		TuringChatPlay();
	}
	else if (0 == strncmp(cmd, HUABEI_NEXT, sizeof(HUABEI_NEXT) - 1)) 
	{
		HuabeiPrevNext(1);
	}
	else if (0 == strncmp(cmd, HUABEI_PREV, sizeof(HUABEI_PREV) - 1)) 
	{
		HuabeiPrevNext(0);
	} 
	else if(0 == strncmp(cmd, HUABEI_CONFIRM, sizeof(HUABEI_CONFIRM) - 1))
	{
		
	} 
	else if(0 == strncmp(cmd, HUABEI_PLAY, sizeof(HUABEI_PLAY) - 1))
	{
		HuabeiPlay(cmd);
	}
	else if(0 == strncmp(cmd, "upload", strlen("upload")))
	{
		HuabeiEventUploadFile(HUABEI_FILE_UPLOAD_WECHAT_INTERCOM, "/root/11.png");
	}
	else if (0 == strncmp(cmd, HUABEI_DATA_RE0, sizeof(HUABEI_DATA_RE0) - 1))  
	{
		HuabeiDataRes0(cmd);	
	}	
	else if (0 == strncmp(cmd, HUABEI_DATA_RE1, sizeof(HUABEI_DATA_RE1) - 1))  
	{
		HuabeiDataRes1(cmd);	
	}	
	else if (0 == strncmp(cmd, HUABEI_DATA_RE2, sizeof(HUABEI_DATA_RE2) - 1))  
	{
		HuabeiDataRes2(cmd);	
	}	
	return 0;
}
#endif



int HuabeiInit()
{
	char *deviceId[17]= {0};
	char token[33] = {0};
	
	char *api = NULL;
	char *aes = NULL;
	
	char *apiKeyVal = NULL;
	char *aesKeyVal = NULL;
	api = ConfigParserGetValue("apiKeyHuabei");
	if(!api) 
	{
		apiKeyVal = g_apiKey;
	} else {
		apiKeyVal = api;
	}
	info("apiKeyVal:%s",apiKeyVal);
	aes = ConfigParserGetValue("aesKeyHuabei");
	if(!aes) 
	{
		aesKeyVal = g_aesKey;
	} else {
		aesKeyVal = aes;
	}
	info("aesKeyVal:%s",aesKeyVal);
	GetDeviceID(deviceId);
	HuabeiUserInit(apiKeyVal, aesKeyVal, deviceId);

	info("deviceId:%s",GetHuabeiDeviceId());
	info("apiKey:%s",GetHuabeiApiKey());
	info("aesKey:%s",GetHuabeiAesKey());
	if(api) {
		free(api);
		api = NULL;
	}
	if(aes) {
		free(aes);
		aes = NULL;
	}

	return 0;
}



