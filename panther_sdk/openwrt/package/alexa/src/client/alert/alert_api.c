#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include <pthread.h>

#include <stdbool.h>
#include <assert.h>
#include <curl/curl.h>
#include <sys/wait.h>  
#include <sys/types.h>


#include "list.h"
#include "alert_api.h"
#include "Alert.h"
#include "Crontab.h"
#include "AlertManager.h"
#include "AlertScheduler.h"
#include "hashmap_itr.h"
#include "AlertsStatePayload.h"
#include "../alert/AlertHead.h"

extern char g_byDeleteAlertFlag;


static pthread_mutex_t DownMusicMtx = PTHREAD_MUTEX_INITIALIZER;  

static void  alert_add(AlertManager *manager, char *token, Alert *pAlear)
{
	// 判断当前定时器是否存zai，如果存zai则判断是否是否相等，如果相等的话则直接return
	if (alertmanager_has_alert(manager,token)) 
	{
		DEBUG_INFO("have %s ", token);
		AlertScheduler *scheduler;
		Alert *alert;
		char *time = NULL;

		scheduler = alertmanager_get_alertscheduler(manager, token);
		DEBUG_INFO("after alertmanager_get_alertscheduler  ");

		alert = alertscheduler_get_alert(scheduler);
		DEBUG_INFO("after alertscheduler_get_alert  ");

		time = alert_get_scheduledtime(alert);
		DEBUG_INFO("after alert_get_scheduledtime  ");
		if (strcmp(time, pAlear->scheduledTime) == 0)
		{
			DEBUG_INFO("%s equals %s",pAlear->scheduledTime,time);
			return ;
		}
		else
		{
			alertscheduler_cancel(scheduler,manager);
		}
	}

	Alert *alert = pAlear;
	alertmanager_add(manager, alert, false);
}

#if 0
int handle_alerts_directive(AlertManager *manager, char *name , char *token, char *type, char *scheduledTime) 
{
	DEBUG_INFO("######### name = %s",name);
	DEBUG_INFO("######### token = %s",token);
	DEBUG_INFO("######### type = %s",type);
	DEBUG_INFO("######### scheduledTime = %s",scheduledTime);

	if(	strcmp (name ,ALERTS_DIRECTIVES_SETALERT) == 0 ) {
		DEBUG_INFO("######### ALERTS_DIRECTIVES_SETALERT");
		alert_add(manager, token, type, scheduledTime) ;
	} else if(	strcmp (name ,ALERTS_DIRECTIVES_DELETEALERT) == 0 ) {
		DEBUG_INFO("######### ALERTS_DIRECTIVES_DELETEALERT");
		alertmanager_delete(manager,token);
	} else {
		
	}
	free(name);
	name = NULL;
	return 0;
}
#endif

#if 0
static int DownLoaderMusicAsset(char *pURL, char *pAssetName)
{
	int iRet = -1;
	char *pTemp = malloc(512);
	DEBUG_INFO("aaaaaaaaaaaaaaaaaaaaaaa");
	if (pTemp != NULL)
	{
		DEBUG_INFO("bbbbbbbbbbbbbbbbbbb");
		// 查找当前目录是否有这个文件，有的话则不要下载了，这里好像查找的时间比下载要花时间?
		memset(pTemp, 0, 512);
		DEBUG_INFO("cccccccccccccccc");
		sprintf(pTemp, "curl -k \"%s\" > /media/%s ", pURL, pAssetName);
		DEBUG_INFO("Set Remind, pTemp:%s", pTemp);
		//free(pURL);
		//free(pAssetName);
		//myexec_cmd(pTemp);
		system(pTemp);
		DEBUG_INFO("dddddddddddddddddd");

		free(pTemp);
		pTemp = NULL;
		iRet = 0;
	}

	return iRet;
}
#endif

typedef struct MUSIC_INFO
{
	list_t *Music_InfoList;
	list_t *Assets_List;
}MUSIC_INFO_STRU;


size_t DownLoaderMusic_callback(void *buffer,size_t size, size_t count,void *response)  
{
	size_t writeSize = fwrite(buffer, size, count, (FILE *)response );

	DEBUG_INFO("begin fwritefunc ... size:%d, nitems:%d, readSize:%d", size, count, writeSize);

	return writeSize;
}


void *DownLoaderMusicPthread(MUSIC_INFO_STRU *param)
{

	MUSIC_INFO_STRU *pMusicInfo = param;

	int i;
	char *pURL = NULL;
	char *pFileName = NULL;
	char byBuffer[128] = {0};
	char byCurlBuffer[256] = {0};

	
	list_iterator_t *music = NULL;
	list_iterator_t *assetid = NULL;
	music = list_iterator_new(pMusicInfo->Music_InfoList, LIST_HEAD);
	assetid = list_iterator_new(pMusicInfo->Assets_List, LIST_HEAD);

	while((list_iterator_has_next(music)) && (list_iterator_has_next(assetid)))
	{
		pURL = (char *)list_iterator_next_value(music);
		pFileName = (char *)list_iterator_next_value(assetid);
		sprintf(byBuffer, "/media/%s", pFileName);

		if(access(byBuffer,0) == 0)
		{
			DEBUG_INFO("have a files, continue");
			continue;
		}
		printf("\033[1;32m""pURL:%s\n""\033[0m", pURL);
		printf("\033[1;32m""pFileName:%s\n""\033[0m", byBuffer);

		/*sprintf(byCurlBuffer, "curl -v -k \"%s\" > %s", pURL, byBuffer);
		system(byCurlBuffer);*/

		FILE *fp = fopen(byBuffer, "wb");

		CURL *curl = curl_easy_init();
		if (curl == NULL) 
		{
			DEBUG_INFO("curl_easy_init() failed\n");
			return false;
		}

		curl_easy_setopt(curl, CURLOPT_URL, pURL);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		
		//if (1 == g_byDebugFlag)
		{
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		}
	
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownLoaderMusic_callback);
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45); 

		int mcode = curl_easy_perform(curl);
		DEBUG_INFO("curl_easy_perform = %d", mcode); 
		if(CURLE_OK == mcode)
		{  
			DEBUG_INFO("curl_easy_perform ok..\n");
		}
		else
		{
			DEBUG_INFO("curl_easy_perform faild..\n");
		}
		curl_easy_cleanup(curl);

		close(fp);
	}

	list_iterator_destroy(music);
	list_iterator_destroy(assetid);

	pthread_exit("DownLoaderMusicPthread is done");
}

static int DownLoaderMusicAsset(MUSIC_INFO_STRU *pMusicInfo)
{
	int iRet;
	pthread_mutex_lock(&DownMusicMtx);

	pthread_t DownLoader_Pthread;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);

	iRet = pthread_create(&DownLoader_Pthread, &a_thread_attr, DownLoaderMusicPthread, pMusicInfo);
	//iRet = pthread_create(&StatusMonitor_Pthread, NULL, StatusMonitorPthread, NULL);
	if(iRet != 0)
	{
		DEBUG_INFO("pthread_create error:%s\n", strerror(iRet));
		return iRet;
	}

	pthread_detach(DownLoader_Pthread);

	pthread_mutex_unlock(&DownMusicMtx);

	return iRet;
}

int handle_alerts_directive(AlertManager *manager, char *name , char *payload) 
{
	Alert *alert;
	struct json_object *root =NULL;
	struct json_object *tokenObj = NULL;
	int i = 0;
	char * token = NULL, *token1 = NULL;
	
	DEBUG_INFO("######### payload = %s",payload);
	root = json_tokener_parse(payload);

	if(is_error(root)) {
		DEBUG_INFO("json_tokener_parse payload failed");
		goto failed;
	}

	
	tokenObj  = json_object_object_get(root, "token");
	if(tokenObj == NULL) {
		DEBUG_INFO("json_object_object_get(payload, token) failed");
		goto failed;
	}

	token1 = json_object_get_string(tokenObj);
	if (strcmp(name, ALERTS_DIRECTIVES_SETALERT) == 0) 
	{
		DEBUG_INFO("######### ALERTS_DIRECTIVES_SETALERT");
		alert = calloc(1, sizeof(Alert));
	
		alert->token = strdup(token1);
	
		struct json_object  *typeObj = NULL, *scheduledObj=NULL;
		char *type = NULL, *scheduledTime = NULL;
		typeObj 		= json_object_object_get(root, "type");
		if(tokenObj == NULL) {
			DEBUG_INFO("json_object_object_get(payload, token) failed");
			goto failed1;
		}
		scheduledObj 	= json_object_object_get(root, "scheduledTime");
		if(tokenObj == NULL) {
			DEBUG_INFO("json_object_object_get(payload, token) failed");
			goto failed1;
		}

		alert->type 			= strdup(json_object_get_string(typeObj));
		alert->scheduledTime = strdup(json_object_get_string(scheduledObj));

		printf("\033[1;31m""alert.type:%s\n""\033[0m", alert->type);
		printf("\033[1;31m""alert.scheduledTime:%s\n""\033[0m", alert->scheduledTime);

		// 判断是否为REMINDER或者是命名定时器
		if ((0 == strcmp(TYPE_REMINDER, alert->type)) || strstr(payload, "assets"))
		{
			struct json_object *assets = NULL, *assetId = NULL, *url = NULL, *value = NULL; 
			struct json_object *assetPlayOrder = NULL, *backgroundAlertAsset = NULL, *loopCount = NULL, *loopPauseInMilliSeconds = NULL;

			// 只有reminder才有循环次数
			if ((0 == strcmp(TYPE_REMINDER, alert->type)))
			{
				// 需要循环的次数
				loopCount = json_object_object_get(root, "loopCount");
				if (NULL == loopCount)
				{
					DEBUG_INFO("json_object_object_get directive failed");
					goto failed1;
				}
				alert->m_reminder.loopCount = json_object_get_int(loopCount);
			}
			else
			{
				alert->m_reminder.loopCount = -1;
			}

			// 每个循环中间的停顿时间
			loopPauseInMilliSeconds = json_object_object_get(root, "loopPauseInMilliSeconds");
			if (NULL == loopPauseInMilliSeconds)
			{
				DEBUG_INFO("json_object_object_get directive failed");
				goto failed1;
			}
			alert->m_reminder.loopPauseInMilliseconds = json_object_get_int(loopPauseInMilliSeconds);

			// 必须播放的音频文件，如果列表中没有，播放默认闹钟的音频文件
			backgroundAlertAsset  = json_object_object_get(root, "backgroundAlertAsset");
			if (NULL == backgroundAlertAsset)
			{
				DEBUG_INFO("json_object_object_get directive failed");
				goto failed1;
			}
			alert->m_reminder.backgroundAlertAsset = strdup(json_object_get_string(backgroundAlertAsset));

			// 获取并下载音频文件
			assets = json_object_object_get(root, "assets");
			if (NULL == assets) 
			{
				DEBUG_INFO("json_object_object_get directive failed");
				goto failed1;
			}
			else
			{
				MUSIC_INFO_STRU *pMusic_Info = malloc(sizeof(MUSIC_INFO_STRU));
				int dateLength =  json_object_array_length(assets);
				int iDateLength = 64;

				char *pTempURL = NULL;
				int iTempURLLength = 1;
				int iTempAssetIDLength = 1;
				pMusic_Info->Music_InfoList = list_new();
				pMusic_Info->Assets_List = list_new();

				alert->m_reminder.assetPlayOrder = (char *)malloc(iDateLength);
				strcpy(alert->m_reminder.assetPlayOrder, "mpg123 -q");

				char *pURL = NULL;
				char *pAssetId = NULL;

				for (i = 0; i < dateLength; i++)
				{
					// 按下标取assetId和url
					value = json_object_array_get_idx(assets, i);
					assetId = json_object_object_get(value, "assetId");
					url = json_object_object_get(value, "url");
					pURL	 = strdup(json_object_get_string(url));
					pAssetId = strdup(json_object_get_string(assetId));

					// 保存音频列表的文件名
					iDateLength += strlen(json_object_get_string(assetId));
					alert->m_reminder.assetPlayOrder = (char *)realloc(alert->m_reminder.assetPlayOrder, iDateLength);
					strcat(alert->m_reminder.assetPlayOrder, " /media/");
					strcat(alert->m_reminder.assetPlayOrder, (char *)json_object_get_string(assetId));
					strcat(alert->m_reminder.assetPlayOrder, " < /dev/null");

					// 将需要下载的URL和名字保存下来，之后创建下载线程下载内容
					list_add(pMusic_Info->Music_InfoList, pURL);
					list_add(pMusic_Info->Assets_List,   pAssetId);
				}
				printf("\033[1;32m""loopCount:%d, loopPauseInMilliseconds:%d assetPlayOrder:%s\n""\033[0m", alert->m_reminder.loopCount, alert->m_reminder.loopPauseInMilliseconds, alert->m_reminder.assetPlayOrder);

				if (0 != DownLoaderMusicAsset(pMusic_Info))
				{
					goto failed1;
				}
			}
		}

		//DEBUG_INFO("type:%s, scheduledTime:%s"alert.type, alert.scheduledTime);
		alert_add(manager, alert->token, alert) ;
	} 
	else if(strcmp (name ,ALERTS_DIRECTIVES_DELETEALERT) == 0 ) 
	{
		DEBUG_INFO("######### ALERTS_DIRECTIVES_DELETEALERT");
		g_byDeleteAlertFlag = 1;
		DEBUG_INFO("alert->token:%s", token1);
		alertmanager_delete(manager,token1);
		system("uartdfifo.sh AlexaAlerEnd");
	//	OnWriteMsgToUartd("AlexaAlerEnd");

		//MpdInit();
		MpdVolume(200);
		//MpdDeinit();
	} 
	else {
		
	}

	json_object_put(root);
	if(NULL != payload) {
		free(payload);
		payload = NULL;
	}
	if(NULL != name) {
		free(name);
		name = NULL;
	}
	return 0;
failed1:
	if(NULL != token) {
		free(token);
		token = NULL;
	}
failed:
	json_object_put(root);
	if(NULL != payload) {
		free(payload);
		payload = NULL;
	}
	if(NULL != name) {
		free(name);
		name = NULL;
	}
	return 1;
	
}






