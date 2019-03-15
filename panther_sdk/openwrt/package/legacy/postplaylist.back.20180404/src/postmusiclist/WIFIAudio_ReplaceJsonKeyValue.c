#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>  

#include "json/json.h"
//读文件大小
#include <sys/stat.h>
#include <unistd.h>

#include "WIFIAudio_ReplaceJsonKeyValue.h"

char *readFileString(char *pFileName)
{
	struct stat filestat;//用来读取文件大小
	char *pString = NULL;
	FILE *fp = NULL;
	int ret = 0;

	if(NULL != pFileName)
	{
		stat(pFileName, &filestat);
		if(NULL != (fp = fopen(pFileName, "r")))
		{
			pString = (char *)calloc(1, filestat.st_size + 1);
			if(NULL != pString)
			{
				ret = fread(pString, 1, filestat.st_size, fp);
				if(ret <= 0)
				{
					free(pString);
					pString = NULL;
				}
			}
		}
		fclose(fp);
		fp = NULL;
	}
	return pString;
}

int writeFileString(char *pFileName, char *pString)
{
	FILE *fp = NULL;
	int ret = -1;

	if((NULL != pFileName) && (NULL != pString))
	{
		if(NULL != (fp = fopen(pFileName, "w+")))
		{
			ret = fwrite(pString, 1, strlen(pString), fp);
			if(ret <= 0)
			{
				ret = -1;
			}
			else
			{
				ret = 0;
			}
		}
		fclose(fp);
		fp = NULL;
	}
	return ret;
}

void replaceJsonKeyValueString_string(struct json_object *pObject, char *pKey, char *pValueString)
{
	struct json_object *pStringObject = NULL;
	if((pObject != NULL) && (pKey != NULL) && (pValueString != NULL))
	{
		//同样的key添加到对象上，会自动替换
		//无需json_object_put或json_object_object_del进行释放
		pStringObject = json_object_new_string(pValueString);
		if(NULL != pStringObject)
		{
			json_object_object_add(pObject, pKey, pStringObject);
		}
	}
}

//只考虑改了json_string对象
//而且也没考虑json_array嵌套其中
char *replaceJsonKeyValueString(char *pJsonString, char *pKey, char *pValue)
{
	char *pDestString = NULL;
	struct json_object *pObject = NULL;
	struct json_object *pTmpObject = NULL;
	struct json_object *pBaseObject = NULL;
	char *pTmpKey = NULL;
	char *pKeyCopy = NULL;
	char *keyName[10];
	int i = 0;
	int baseNum = 0;

	if((pJsonString != NULL) && (pKey != NULL) && (pValue != NULL))
	{
		pObject = json_tokener_parse(pJsonString);

		if(!is_error(pObject))
		{
			//外部传入的可能是个常量字符串
			//strtok会改变字符串地址
			//这边复制一下
			pKeyCopy = strdup(pKey);
			for(i = 0; i < 10; i++)
			{
				keyName[i] = NULL;
			}
			
			pTmpKey = strtok(pKeyCopy, ".");
			//只要这个字符串有内容
			//第一次怎么样都不会为空
			keyName[0] = strdup(pTmpKey);
			
			while(NULL != (pTmpKey = strtok(NULL, ".")))
			{
				//计算有几级字典
				baseNum++;
				keyName[baseNum] = strdup(pTmpKey);
			}
			
			pTmpObject = pObject;
			
			//取得要修改对象的最后一级父对象
			for(i = 0; i < baseNum; i++)
			{
				//记录获取对象之前
				pBaseObject = pTmpObject;
				pTmpObject = json_object_object_get(pBaseObject, keyName[i]);
				if(NULL == pTmpObject)
				{
					//中间层对象没获取到
					//增加中间层字典对象
					pTmpObject = json_object_new_object();
					json_object_object_add(pBaseObject, keyName[i], pTmpObject);
				}
			}
			
			pBaseObject = pTmpObject;
			
			replaceJsonKeyValueString_string(pBaseObject, keyName[i], pValue);
			
			
			pDestString = strdup(json_object_to_json_string(pObject));
			json_object_put(pObject);
			
			//回收资源
			for(i = 0; i < 10; i++)
			{
				if(keyName[i] != NULL)
				{
					free(keyName[i]);
					keyName[i] = NULL;
				}
				else
				{
					//因为都是按顺序存放的
					//如果其中有一个为空
					//说明后面都都为空
					break;
				}
			}
		}
	}

	return pDestString;
}


int setJsonFileKeyValueString(char *pFileName, char *pKey, char *pValueString)
{
	int ret = -1;
	char *pDestString = NULL;
	char *pString = NULL;

	if((pFileName != NULL) && (pKey != NULL) && (pValueString != NULL))
	{
		pString = readFileString(pFileName);
		if(NULL != pString)
		{
			pDestString = replaceJsonKeyValueString(pString, pKey, pValueString);
			if(NULL != pDestString)
			{
				ret = writeFileString(pFileName, pDestString);
				free(pDestString);
				pDestString = NULL;
			}
			free(pString);
			pString = NULL;
		}
	}

	return ret;
}