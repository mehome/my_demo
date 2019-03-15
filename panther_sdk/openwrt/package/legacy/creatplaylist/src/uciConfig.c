#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>  
#include <string.h>

#include "uciConfig.h"

int WIFIAudio_UciConfig_FreeValueString(char **ppstring)
{
	int ret = 0;
	if((NULL == ppstring) || (NULL == *ppstring))
	{
		printf("Error of parameter : NULL == ppstring or NULL == *ppstring in WIFIAudio_UciConfig_FreeValueString\r\n");
		ret = -1;
	}
	else
	{
		free(*ppstring);
		*ppstring = NULL;
	}
	
	return ret;
}

char * WIFIAudio_UciConfig_SearchValueString(char *psearch)
{
	char *pretval = NULL;
	char *psearchcopy = NULL;
	char get_cdb_value[4096] = { 0 };
	char search_value[128] = { 0 };
	if(NULL == psearch)
	{
		printf("Error of parameter : NULL == psearch in WIFIAudio_UciConfig_SearchValueString\r\n");
		pretval = NULL;
		return NULL;
	}
	else
	{
		//pcontext = uci_alloc_context();
	
		//uci_set_confdir(pcontext, WIFIAUDIO_CONFIG_PATH);
		
		//psearchcopy = strdup(psearch);
		 memset(search_value, 0, sizeof(search_value)/sizeof(char));
		 sprintf(search_value, "$%s",psearch);
		//uci_lookup_ptr当中搜索的字符串不能为const的，他会进行修改
		//psearch传进来因为给的是常量字符串，所以会出问题，要自行复制
		if ((cdb_get_str(search_value, get_cdb_value, sizeof(get_cdb_value), NULL) == NULL)) 
		//if(UCI_OK != uci_lookup_ptr(pcontext, &ptr, psearchcopy, true))
		{
			printf("can't get str =%s\n",search_value);
			return NULL;
		}		
		 return get_cdb_value;		
	}	
}

int WIFIAudio_UciConfig_SearchAndSetValueString(char *psearch, char *pstring)
{
	int ret = -1;
	char set_cdb_value[4096] = { 0 };
	char search_value[128] = { 0 };
	
	
	if((NULL == psearch) || (NULL == pstring))
	{
		printf("Error of parameter : NULL == psearch or NULL == pstring in WIFIAudio_UciConfig_SearchAndSetValueString\r\n");
		ret = -1;
	}
	else
	{
		memset(search_value, 0, sizeof(search_value)/sizeof(char));
		sprintf(search_value, "$%s",psearch);
		memset(set_cdb_value, 0, sizeof(set_cdb_value)/sizeof(char));
		sprintf(set_cdb_value, "%s",pstring);
		cdb_set(search_value, set_cdb_value);
		ret =1;
	}
	return ret;
}

int WIFIAudio_UciConfig_SetValueString(char *psearch, char *pstring)
{
	
	int ret = -1;
		char set_cdb_value[4096] = { 0 };
		char search_value[128] = { 0 };
		
		
		if((NULL == psearch) || (NULL == pstring))
		{
			printf("Error of parameter : NULL == psearch or NULL == pstring in WIFIAudio_UciConfig_SearchAndSetValueString\r\n");
			ret = -1;
		}
		else
		{
			memset(search_value, 0, sizeof(search_value)/sizeof(char));
			sprintf(search_value, "$%s",psearch);
			memset(set_cdb_value, 0, sizeof(set_cdb_value)/sizeof(char));
			sprintf(set_cdb_value, "%s",pstring);
			cdb_set(search_value, set_cdb_value);
			ret =1;
		}
		return ret;
	}

