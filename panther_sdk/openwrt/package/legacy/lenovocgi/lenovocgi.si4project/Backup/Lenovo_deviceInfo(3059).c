#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>
#include <avsclient/libavsclient.h>
#include <wdk/cdb.h>

#include "Lenovo_Debug.h"
#include "Lenovo_FileName.h"

#include "Lenovo_UciConfig.h"
#include "Lenovo_Json.h"
#include "uuid.h"
int main(int argc,char *argv[])
{
	Lenovo_pJsonStr pJsonStr = NULL;
	char *pUciStr = NULL;
	int semId = 0;//信号量ID
	char *pJsonStrring = NULL;
	FILE *fp = NULL;
	
	pJsonStr = (Lenovo_pJsonStr)calloc(1, sizeof(Lenovo_JsonStr));
	if(NULL == pJsonStr)
	{
		DEBUG("Error of calloc");
	}
	else
	{
		pJsonStr->Type = LENOVO_JSON_TYPE_DEVICEINFO;
		
		pJsonStr->JsonStr.pDeviceInfo = (Lenovo_pDeviceInfo)calloc(1, sizeof(Lenovo_DeviceInfo));
		if(NULL == pJsonStr)
		{
			DEBUG("Error of calloc");
			Lenovo_Json_ppJsonStrFree(&pJsonStr);
		}
		else
		{
			/*
			fp = popen("AlexaClient generateParameter", "r");
			if(NULL == fp)
			{
				
			}
			else
			{
				pclose(fp);
				fp = NULL;//不关心执行之后显示什么数据
			}*/
			
			semId = Lenovo_Semaphore_CreateOrGetSem((key_t)LENOVO_UCICONFIG_SEMKEY, 1);
			Lenovo_Semaphore_Operation_P(semId);
			
			LOGIN_INFO_STRUCT* loginfo;
			loginfo = logInInfo_new();
			if (loginfo != NULL) {

				pJsonStr->JsonStr.pDeviceInfo->pCodeChallengeMethod = strdup(loginfo->pCodeChallengeMethod);
				//printf("pCodeChallengeMethod:%s\n", loginfo->pCodeChallengeMethod);

				pJsonStr->JsonStr.pDeviceInfo->pCodeChallenge = strdup(loginfo->pCodeChallenge);
				//printf("pCodeChallenge:%s\n", loginfo->pCodeChallenge);

				pJsonStr->JsonStr.pDeviceInfo->pProductId = strdup(loginfo->pProductId);
				//printf("pProductId:%s\n", loginfo->pProductId);

				pJsonStr->JsonStr.pDeviceInfo->pDsn = strdup(loginfo->pDeviceSerialNumber);
				//printf("pDeviceSerialNumber:%s\n", loginfo->pDeviceSerialNumber);

				//printf("pCodeVerifier:%s\n", loginfo->pCodeVerifier);

				// 调试时先设置到cdb变量中，后面直接传给APP APP再下发过来给我们
				cdb_set("$amazon_code_verifier", loginfo->pCodeVerifier);
				
				logInInfo_free(loginfo);
			}


			/*pUciStr = Lenovo_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_ProductId");
			if(NULL != pUciStr)
			{
				pJsonStr->JsonStr.pDeviceInfo->pProductId = strdup(pUciStr);

				Lenovo_UciConfig_FreeValueString(&pUciStr);
				pUciStr = NULL;
			}
			
			pUciStr = Lenovo_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_Dsn");
			if(NULL != pUciStr)
			{
				pJsonStr->JsonStr.pDeviceInfo->pDsn = strdup(pUciStr);

				Lenovo_UciConfig_FreeValueString(&pUciStr);
				pUciStr = NULL;
			}
			
			pUciStr = Lenovo_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_CodeChallenge");
			if(NULL != pUciStr)
			{
				pJsonStr->JsonStr.pDeviceInfo->pCodeChallenge = strdup(pUciStr);

				Lenovo_UciConfig_FreeValueString(&pUciStr);
				pUciStr = NULL;
			}
			
			pUciStr = Lenovo_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_CodeChallengeMethod");
			if(NULL != pUciStr)
			{
				pJsonStr->JsonStr.pDeviceInfo->pCodeChallengeMethod = strdup(pUciStr);

				Lenovo_UciConfig_FreeValueString(&pUciStr);
				pUciStr = NULL;
			}*/
			
			char messageId[64] = {0};

      		getuuidString(messageId);
			if(NULL != messageId)
			{
				pJsonStr->JsonStr.pDeviceInfo->pSessionId = messageId;

				//Lenovo_UciConfig_FreeValueString(&pUciStr);
				//pUciStr = NULL;
			}

			Lenovo_Semaphore_Operation_V(semId);

			pJsonStrring = Lenovo_Json_pJsonStrTopJsonString(pJsonStr);
			Lenovo_Json_ppJsonStrFree(&pJsonStr);

			if(pJsonStrring != NULL)
			{
				printf("%s", pJsonStrring);
			}
			Lenovo_Json_ppJsonStringFree(&pJsonStrring);
		}
	}
	fflush(stdout);

	return 0;
}
