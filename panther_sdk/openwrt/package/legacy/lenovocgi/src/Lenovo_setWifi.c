#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>

#include "Lenovo_Debug.h"
#include "Lenovo_FileName.h"

#include "Lenovo_UciConfig.h"
#include "Lenovo_Json.h"


char* Lenovo_setWifi_MacChange(char *pMacString)
{
	char *pMacNew = NULL;
	char *pTmp = NULL;
	
	if(NULL == pMacString)
	{
		
	}
	else
	{
		pMacNew = (char *)calloc(1, 13);
		pTmp = strtok(pMacString, ":");
		if(NULL != pTmp)
		{
			memset(pMacNew, 0, 13);
			strcat(pMacNew, pTmp);
		}
		while(NULL != (pTmp = strtok(NULL, ":")))
		{
			strcat(pMacNew, pTmp);
		}
	}
	
	return pMacNew;
}

int main(void)
{
	int len = 0;
	int ret = 0;
	FILE *fp = NULL;
	char *pLenStr = NULL;
	char *pPostStr = NULL;
	Lenovo_pJsonStr pJsonStr = NULL;
	char cmd[128];
	char *pUciStr = NULL;
	int semId = 0;//信号量ID
	char *pJsonStrring = NULL;
	
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
				pJsonStr = Lenovo_Json_pJsonStringTopJsonStr(LENOVO_JSON_TYPE_SSIDPWD, pPostStr);
				
				if(NULL == pJsonStr)
				{
					
				}
			}
			free(pPostStr);
			pPostStr = NULL;
		}
	}
	
	//上面已经解析出JSON信息
	if(NULL != pJsonStr)
	{
		memset(cmd, 0, sizeof(cmd)/sizeof(char));
		if((pJsonStr->JsonStr.pSsidPwd->pPassword == NULL) || !(strcmp(pJsonStr->JsonStr.pSsidPwd->pPassword, "")))
		{
			sprintf(cmd, "elian.sh connect \'%s\' &", pJsonStr->JsonStr.pSsidPwd->pSsid);
		}
		else
		{
			sprintf(cmd, "elian.sh connect \'%s\' \'%s\' &", pJsonStr->JsonStr.pSsidPwd->pSsid, pJsonStr->JsonStr.pSsidPwd->pPassword);
		}
//printf("%s\r\n", cmd);

		fp = popen(cmd, "r");
		if(NULL == fp)
		{
			
		}
		else
		{
			pclose(fp);
			fp = NULL;//不关心执行之后显示什么数据
		}
		
		Lenovo_Json_ppJsonStrFree(&pJsonStr);
		
		pJsonStr = (Lenovo_pJsonStr)calloc(1, sizeof(Lenovo_JsonStr));
		if(NULL == pJsonStr)
		{
			DEBUG("Error of calloc");
		}
		else
		{
			pJsonStr->Type = LENOVO_JSON_TYPE_HUBMAC;
			
			pJsonStr->JsonStr.pHubMac = (Lenovo_pHubMac)calloc(1, sizeof(Lenovo_HubMac));
			if(NULL == pJsonStr)
			{
				DEBUG("Error of calloc");
				Lenovo_Json_ppJsonStrFree(&pJsonStr);
			}
			else
			{
				pJsonStr->JsonStr.pHubMac->pHubType = strdup("1000040");
				
				semId = Lenovo_Semaphore_CreateOrGetSem((key_t)LENOVO_UCICONFIG_SEMKEY, 1);
				Lenovo_Semaphore_Operation_P(semId);
				
				pUciStr = Lenovo_UciConfig_SearchValueString("xzxwifiaudio.config.mac_addr");
				if(NULL != pUciStr)
				{
					pJsonStr->JsonStr.pHubMac->pMacAddr = Lenovo_setWifi_MacChange(pUciStr);
					//pJsonStr->JsonStr.pHubMac->pMacAddr = strdup(pUciStr);
					
					Lenovo_UciConfig_FreeValueString(&pUciStr);
					pUciStr = NULL;
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
	}

	fflush(stdout);

	return 0;
}

