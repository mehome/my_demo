#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>

#include "Lenovo_Debug.h"
#include "Lenovo_FileName.h"

#include "Lenovo_UciConfig.h"
#include "Lenovo_Json.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <fcntl.h>

//#define UNIX_DOMAIN "/tmp/UNIX.domain"
#include <avsclient/libavsclient.h>
#include <wdk/cdb.h>
#if 0
int OnWriteMsgToAlexa(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}
#endif

int main(void)
{
	int len = 0;
	int ret = 0;
	char *pLenStr = NULL;
	char *pPostStr = NULL;
	Lenovo_pJsonStr pJsonStr = NULL;
	int semId = 0;//信号量ID
	
	//接受数据
	pLenStr = getenv("CONTENT_LENGTH");
	if (NULL == pLenStr)
	{

	}
	else
	{
		len = atoi(pLenStr);
		
		pPostStr = (char *)calloc(1, len + 1);


		if(NULL == pPostStr)
		{

		}
		else
		{
			ret = fread(pPostStr, sizeof(char), len, stdin);
			if(ret < len)
			{
				
			}
			else
			{
				pJsonStr = Lenovo_Json_pJsonStringTopJsonStr(LENOVO_JSON_TYPE_COMPANIONINFO, pPostStr);
			}
			free(pPostStr);
			pPostStr = NULL;
		}
	}
	
	//上面已经解析出JSON信息
	if(NULL != pJsonStr)
	{
		semId = Lenovo_Semaphore_CreateOrGetSem((key_t)LENOVO_UCICONFIG_SEMKEY, 1);
		Lenovo_Semaphore_Operation_P(semId);

		char byCodeVerifier[64] = {0};
		cdb_get("$amazon_code_verifier", byCodeVerifier);

		OnSendSignInInfo(byCodeVerifier, pJsonStr->JsonStr.pCompanionInfo->pAuthCode, pJsonStr->JsonStr.pCompanionInfo->pRedirectUri, pJsonStr->JsonStr.pCompanionInfo->pClientId);

		/*if(NULL != pJsonStr->JsonStr.pCompanionInfo->pAuthCode)
		{
			Lenovo_UciConfig_SetValueString("xzxwifiaudio.config.amazon_authorizationCode", \
			pJsonStr->JsonStr.pCompanionInfo->pAuthCode);
		}

		if(NULL != pJsonStr->JsonStr.pCompanionInfo->pClientId)
		{
			Lenovo_UciConfig_SetValueString("xzxwifiaudio.config.amazon_clientId", \
			pJsonStr->JsonStr.pCompanionInfo->pClientId);
		}

		if(NULL != pJsonStr->JsonStr.pCompanionInfo->pRedirectUri)
		{
			Lenovo_UciConfig_SetValueString("xzxwifiaudio.config.amazon_redirectUri", \
			pJsonStr->JsonStr.pCompanionInfo->pRedirectUri);
		}*/

		/*if(NULL != pJsonStr->JsonStr.pCompanionInfo->pSessionId)
		{
			Lenovo_UciConfig_SetValueString("xzxwifiaudio.config.00000000", \
			pJsonStr->JsonStr.pCompanionInfo->pSessionId);
		}*/

		Lenovo_Semaphore_Operation_V(semId);
		
		Lenovo_Json_ppJsonStrFree(&pJsonStr);
		
		//OnWriteMsgToAlexa("GetToken");
	}

	fflush(stdout);

	return 0;
}

