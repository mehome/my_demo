#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
//#define UNIX_DOMAIN "/tmp/UNIX.domain"
#include <avsclient/libavsclient.h>

#include "WIFIAudio_RetJson.h"
#include "amazon_Json.h"

int main(int argc, char *argv[])
{
	int len = 0;
	int ret = -1;
	char *pLenStr = NULL;
	char *pPostStr = NULL;
	Lenovo_pJsonStr pJsonStr = NULL;
	int semId = 0;

	pLenStr = getenv("CONTENT_LENGTH");
	if (pLenStr != NULL)
	{
		len = atoi(pLenStr);
		
		pPostStr = (char *)calloc(1, len + 1);


		if(pPostStr != NULL)
		{
			ret = fread(pPostStr, sizeof(char), len, stdin);
			if(ret == len)
			{
				pJsonStr = Lenovo_Json_pJsonStringTopJsonStr(LENOVO_JSON_TYPE_COMPANIONINFO, pPostStr);
			}
			free(pPostStr);
			pPostStr = NULL;
		}
	}
	
	if(pJsonStr != NULL)
	{
		ret = OnSendSignInInfo(pJsonStr->JsonStr.pCompanionInfo->pCodeVerifier, \
				 pJsonStr->JsonStr.pCompanionInfo->pAuthorizationCode, \
				 pJsonStr->JsonStr.pCompanionInfo->pRedirectUri, \
				 pJsonStr->JsonStr.pCompanionInfo->pClientId);
		Lenovo_Json_ppJsonStrFree(&pJsonStr);
	}
	
	if (ret == 0)
	{
		WIFIAudio_RetJson_retJSON("OK");
	} else {
		WIFIAudio_RetJson_retJSON("FAIL");
	}
	
	fflush(stdout);

	return 0;
}

