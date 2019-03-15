#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>
#include <syslog.h>
#include <avsclient/libavsclient.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "WIFIAudio_RetJson.h"
#include "Lenovo_Json.h"
#include "uuid.h"

int main(int argc,char *argv[])
{
	Lenovo_pJsonStr pJsonStr = NULL;
	char *pJsonStrring = NULL;
	FILE *fp = NULL;
	LOGIN_INFO_STRUCT* loginfo = NULL;
	int ret = -1;

#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
	openlog(argv[0], LOG_PID, LOG_USER);
#endif
	
	pJsonStr = (Lenovo_pJsonStr)calloc(1, sizeof(Lenovo_JsonStr));
	if(pJsonStr != NULL)
	{
		pJsonStr->Type = LENOVO_JSON_TYPE_DEVICEINFO;
		
		pJsonStr->JsonStr.pDeviceInfo = (Lenovo_pDeviceInfo)calloc(1, sizeof(Lenovo_DeviceInfo));
		
		if (pJsonStr->JsonStr.pDeviceInfo != NULL)
		{
#ifdef WIFIAUDIO_DEBUG
			pJsonStr->JsonStr.pDeviceInfo->pProductId = strdup("doss_smart_speaker_01");
			pJsonStr->JsonStr.pDeviceInfo->pDsn = strdup("123456");
			pJsonStr->JsonStr.pDeviceInfo->pCodeChallenge = strdup("codeChallenge");
			pJsonStr->JsonStr.pDeviceInfo->pCodeVerifier = strdup("codeVerifier");
			pJsonStr->JsonStr.pDeviceInfo->pCodeChallengeMethod = strdup("codeChallengeMethod");
				
			pJsonStrring = Lenovo_Json_pJsonStrTopJsonString(pJsonStr);
			if(pJsonStrring != NULL)
			{
				ret = 0;
			}
#else
			loginfo = logInInfo_new();
			if (loginfo != NULL) {
				pJsonStr->JsonStr.pDeviceInfo->pProductId = strdup(loginfo->pProductId);
				pJsonStr->JsonStr.pDeviceInfo->pDsn = strdup(loginfo->pDeviceSerialNumber);
				pJsonStr->JsonStr.pDeviceInfo->pCodeChallenge = strdup(loginfo->pCodeChallenge);
				pJsonStr->JsonStr.pDeviceInfo->pCodeVerifier = strdup(loginfo->pCodeVerifier);
				pJsonStr->JsonStr.pDeviceInfo->pCodeChallengeMethod = strdup(loginfo->pCodeChallengeMethod);
				
				logInInfo_free(loginfo);
				
				pJsonStrring = Lenovo_Json_pJsonStrTopJsonString(pJsonStr);
				if(pJsonStrring != NULL)
				{
					ret = 0;
				}
			}
#endif
			
			if (ret == 0)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: String: %s\n", __FILE__, __LINE__, __FUNCTION__, pJsonStrring);
#endif
				printf("%s", pJsonStrring);
				Lenovo_Json_ppJsonStringFree(&pJsonStrring);
			} else {
				WIFIAudio_RetJson_retJSON("FAIL");
			}
		}
		
		Lenovo_Json_ppJsonStrFree(&pJsonStr);
	}
	fflush(stdout);

	return 0;
}

