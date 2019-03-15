#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>
#include <avsclient/libavsclient.h>

#include "WIFIAudio_RetJson.h"
#include "amazon_Json.h"

int main(int argc,char *argv[])
{
	Lenovo_pJsonStr pJsonStr = NULL;
	char *pJsonStrring = NULL;
	FILE *fp = NULL;
	LOGIN_INFO_STRUCT* loginfo = NULL;
	int ret = -1;

	pJsonStr = (Lenovo_pJsonStr)calloc(1, sizeof(Lenovo_JsonStr));
	if(pJsonStr != NULL)
	{
		pJsonStr->Type = LENOVO_JSON_TYPE_DEVICEINFO;
		
		pJsonStr->JsonStr.pDeviceInfo = (Lenovo_pDeviceInfo)calloc(1, sizeof(Lenovo_DeviceInfo));
		
		if (pJsonStr->JsonStr.pDeviceInfo != NULL)
		{
			loginfo = logInInfo_new();
			if (loginfo != NULL) {
				if ((loginfo->pProductId != NULL) && \
				    (loginfo->pDeviceSerialNumber != NULL) && \
				    (loginfo->pCodeChallenge != NULL) && \
				    (loginfo->pCodeVerifier != NULL) && \
				    (loginfo->pCodeChallengeMethod != NULL))
				{
					pJsonStr->JsonStr.pDeviceInfo->pProductId = strdup(loginfo->pProductId);
					pJsonStr->JsonStr.pDeviceInfo->pDsn = strdup(loginfo->pDeviceSerialNumber);
					pJsonStr->JsonStr.pDeviceInfo->pCodeChallenge = strdup(loginfo->pCodeChallenge);
					pJsonStr->JsonStr.pDeviceInfo->pCodeVerifier = strdup(loginfo->pCodeVerifier);
					pJsonStr->JsonStr.pDeviceInfo->pCodeChallengeMethod = strdup(loginfo->pCodeChallengeMethod);
					
					pJsonStrring = Lenovo_Json_pJsonStrTopJsonString(pJsonStr);
					if(pJsonStrring != NULL)
					{
						ret = 0;
					}
				}
				
				logInInfo_free(loginfo);
			}
			
			if (ret == 0)
			{
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

