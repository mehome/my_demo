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
	struct uci_context *pcontext = NULL;
	struct uci_ptr ptr;
	char *psearchcopy = NULL;
	
	if(NULL == psearch)
	{
		printf("Error of parameter : NULL == psearch in WIFIAudio_UciConfig_SearchValueString\r\n");
		pretval = NULL;
	}
	else
	{
		pcontext = uci_alloc_context();
	
		uci_set_confdir(pcontext, WIFIAUDIO_CONFIG_PATH);
		
		psearchcopy = strdup(psearch);
		
		//uci_lookup_ptr当中搜索的字符串不能为const的，他会进行修改
		//psearch传进来因为给的是常量字符串，所以会出问题，要自行复制
		if(UCI_OK != uci_lookup_ptr(pcontext, &ptr, psearchcopy, true))
		{
			pretval = NULL;
		}
		else
		{
			if(NULL == ptr.o)
			{
				pretval = NULL;
			}
			else
			{
				if(NULL == ptr.o->v.string)
				{
					pretval = NULL;
				}
				else
				{
					pretval = strdup(ptr.o->v.string);
				}
			}
		}
		
		if(psearchcopy != NULL)
		{
			free(psearchcopy);
			psearchcopy = NULL;
		}
		
		if(pcontext != NULL)
		{
			uci_free_context(pcontext);
			pcontext = NULL;
		}
	}
	
	return pretval;
}

int WIFIAudio_UciConfig_SearchAndSetValueString(char *psearch, char *pstring)
{
	int ret = -1;
	int find = 0;
	struct uci_context *pcontext = NULL;
	struct uci_ptr ptr;
	char *psearchcopy = NULL;//用来查找
	struct uci_package *ppackage = NULL;
	char *ptokpackage = NULL;
	char *ptoksection = NULL;
	char *ptokoption = NULL;
	char path[128];
	
	if((NULL == psearch) || (NULL == pstring))
	{
		printf("Error of parameter : NULL == psearch or NULL == pstring in WIFIAudio_UciConfig_SearchAndSetValueString\r\n");
		ret = -1;
	}
	else
	{
		pcontext = uci_alloc_context();
		
		psearchcopy = strdup(psearch);
		
		ptokpackage = strtok(psearchcopy, ".");
		ptoksection = strtok(NULL, ".");
		ptokoption = strtok(NULL, ".");
		
		memset(path, 0, sizeof(path)/sizeof(char));
		strcpy(path, WIFIAUDIO_CONFIG_PATH);
		strcat(path, ptokpackage);
		
		//加载的时候没写绝对路径，写回去的时候写不回去
		if(UCI_OK == uci_load(pcontext, path, &ppackage))
		{
			struct uci_element *psection_element = NULL;
			
			uci_foreach_element(&ppackage->sections, psection_element)
			{
				if(!strcmp(ptoksection, psection_element->name))
				{
					struct uci_element *poption_element = NULL;
					struct uci_section *psection = uci_to_section(psection_element);
					
					uci_foreach_element(&psection->options, poption_element)
					{
						if(!strcmp(ptokoption, poption_element->name))
						{
							//有找到
							find = 1;
							break;
						}
					}
					//如果有uci_to_section对应的释放函数，需要在这边释放
					break;
				}
			}
			
			if(1 == find)
			{
				memset(&ptr, 0, sizeof(struct uci_ptr));//这边如果不初始化会段错误
				ptr.package = ptokpackage;
				ptr.section = ptoksection;
				ptr.option = ptokoption;
				ptr.value = pstring;
				
				uci_set(pcontext, &ptr);//更新到容器当中
				//cdb set 
				//lib/wdk/commit
				uci_commit(pcontext, &ppackage, true);//更新到文件当中
				
				ret = 0;
			}
			uci_unload(pcontext, ppackage);
		}
		
		if(psearchcopy != NULL)
		{
			free(psearchcopy);
			psearchcopy = NULL;
		}
		
		if(pcontext != NULL)
		{
			uci_free_context(pcontext);
			pcontext = NULL;
		}
	}
	
	return ret;
}

int WIFIAudio_UciConfig_SetValueString(char *psearch, char *pstring)
{
	int ret = -1;
	int find = 0;
	struct uci_context *pcontext = NULL;
	struct uci_ptr ptr;
	char *psearchcopy = NULL;//用来查找
	struct uci_package *ppackage = NULL;
	char *ptokpackage = NULL;
	char *ptoksection = NULL;
	char *ptokoption = NULL;
	char path[128];
	
	if((NULL == psearch) || (NULL == pstring))
	{
		printf("Error of parameter : NULL == psearch or NULL == pstring in WIFIAudio_UciConfig_SetValueString\r\n");
		ret = -1;
	}
	else
	{
		pcontext = uci_alloc_context();
		
		psearchcopy = strdup(psearch);
		
		ptokpackage = strtok(psearchcopy, ".");
		ptoksection = strtok(NULL, ".");
		ptokoption = strtok(NULL, ".");
		
		memset(path, 0, sizeof(path)/sizeof(char));
		strcpy(path, WIFIAUDIO_CONFIG_PATH);
		strcat(path, ptokpackage);
		
		//加载的时候没写绝对路径，写回去的时候写不回去
		if(UCI_OK == uci_load(pcontext, path, &ppackage))
		{
			memset(&ptr, 0, sizeof(struct uci_ptr));//这边如果不初始化会段错误
			ptr.package = ptokpackage;
			ptr.section = ptoksection;
			ptr.option = ptokoption;
			ptr.value = pstring;
			
			uci_set(pcontext, &ptr);//更新到容器当中
			uci_commit(pcontext, &ppackage, true);//更新到文件当中
			
			uci_unload(pcontext, ppackage);
		}
		
		if(psearchcopy != NULL)
		{
			free(psearchcopy);
			psearchcopy = NULL;
		}
		
		if(pcontext != NULL)
		{
			uci_free_context(pcontext);
			pcontext = NULL;
		}
	}
	
	return ret;
}












