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

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "WIFIAudio_RetJson.h"
#include "Lenovo_Json.h"

int main(int argc, char *argv[])
{
	int len = 0;
	int ret = -1;
	char *pLenStr = NULL;
	char *pPostStr = NULL;
	Lenovo_pJsonStr pJsonStr = NULL;
	int semId = 0;

#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
	openlog(argv[0], LOG_PID, LOG_USER);
#endif
	
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
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: Post: %s\n", __FILE__, __LINE__, __FUNCTION__, pPostStr);
#endif
				pJsonStr = Lenovo_Json_pJsonStringTopJsonStr(LENOVO_JSON_TYPE_COMPANIONINFO, pPostStr);
			}
			free(pPostStr);
			pPostStr = NULL;
		}
	}
	
	if(pJsonStr != NULL)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: AuthorizationCode: %s\n", __FILE__, __LINE__, __FUNCTION__, pJsonStr->JsonStr.pCompanionInfo->pAuthorizationCode);
		syslog(LOG_INFO, "%s - %d - %s: RedirectUri: %s\n", __FILE__, __LINE__, __FUNCTION__, pJsonStr->JsonStr.pCompanionInfo->pRedirectUri);
		syslog(LOG_INFO, "%s - %d - %s: ClientId: %s\n", __FILE__, __LINE__, __FUNCTION__, pJsonStr->JsonStr.pCompanionInfo->pClientId);
		syslog(LOG_INFO, "%s - %d - %s: CodeVerifier: %s\n", __FILE__, __LINE__, __FUNCTION__, pJsonStr->JsonStr.pCompanionInfo->pCodeVerifier);
		ret = 0;
#else
		ret = OnSendSignInInfo(pJsonStr->JsonStr.pCompanionInfo->pCodeVerifier, \
				 pJsonStr->JsonStr.pCompanionInfo->pAuthorizationCode, \
				 pJsonStr->JsonStr.pCompanionInfo->pRedirectUri, \
				 pJsonStr->JsonStr.pCompanionInfo->pClientId);
#endif
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

