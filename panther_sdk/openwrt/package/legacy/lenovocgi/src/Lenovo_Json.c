#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>

#include "Lenovo_Json.h"
#include "Lenovo_Debug.h"


//统一从JsonObject当中获取Json的String对象，并保存到String类型的指针当中
void Lenovo_Json_GetStringJsonStringFromJsonObject(struct json_object *pObject, char *pKey, char **ppString)
{
	struct json_object *pStringObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		pStringObject = json_object_object_get(pObject, pKey);
		if(NULL == pStringObject)
		{
			*ppString = NULL;
		}
		else
		{
			*ppString = strdup(json_object_get_string(pStringObject));//不引用计数，不分配内存
		}
	}
}

//统一从JsonObject当中获取Json对象，并保存到int类型当中，不论是Json的Int类型还是String类型
void Lenovo_Json_GetIntFromJsonObject(struct json_object *pObject, char *pKey, int *pValue)
{
	struct json_object *pIntObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		pIntObject = json_object_object_get(pObject, pKey);
		if(NULL == pIntObject)
		{
			*pValue = LENOVO_JSON_INTERRORCODE;
		}
		else
		{
			*pValue = json_object_get_int(pIntObject);//不引用计数，不分配内存
		}
	}
}

//统一从JsonObject当中获取Json对象，并保存到double类型当中
void Lenovo_Json_GetDoubleFromJsonObject(struct json_object *pObject, char *pKey, double *pValue)
{
	struct json_object *pDoubleObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		pDoubleObject = json_object_object_get(pObject, pKey);
		if(NULL == pDoubleObject)
		{
			*pValue = LENOVO_JSON_INTERRORCODE;
		}
		else
		{
			*pValue = json_object_get_double(pDoubleObject);//不引用计数，不分配内存
		}
	}
}

//统一压缩int成Json的int对象，并添加挂载到JsonObject
void Lenovo_Json_IntToJsonIntAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pIntObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		}
		else
		{
			pIntObject = json_object_new_int(Value);
			if(NULL != pIntObject)
			{
				json_object_object_add(pObject, pKey, pIntObject);
				//这边虽然手动生成了对象，但是不需要手动释放，因为已经添加挂载到pObject对象上
			}
		}
	}
}

//统一压缩String成Json的string对象，并添加挂载到JsonObject
void Lenovo_Json_StingToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, char *pString)
{
	struct json_object *pStringObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		if(NULL == pString)
		{
			json_object_object_add(pObject, pKey, NULL);
		}
		else
		{
			pStringObject = json_object_new_string(pString);
			if(NULL != pStringObject)
			{
				json_object_object_add(pObject, pKey, pStringObject);
				//这边虽然手动生成了对象，但是不需要手动释放，因为已经添加挂载到pObject对象上
			}
		}
	}
}

//统一压缩int成Json的string对象，并添加挂载到JsonObject
void Lenovo_Json_IntToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pStringObject = NULL;
	char tmp[64];//用来放字符串
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		}
		else
		{
			memset(tmp, 0, sizeof(tmp)/sizeof(char));
			sprintf(tmp, "%d", Value);
			pStringObject = json_object_new_string(tmp);
			if(NULL != pStringObject)
			{
				json_object_object_add(pObject, pKey, pStringObject);
				//这边虽然手动生成了对象，但是不需要手动释放，因为已经添加挂载到pObject对象上
			}
		}
	}
}

//统一压缩double成Json的double对象，并添加挂载到JsonObject
void Lenovo_Json_DoubleToJsonDoubleAddJsonObject(struct json_object *pObject, char *pKey, double Value)
{
	struct json_object *pDoubleObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
		//不做任何动作
	}
	else
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		}
		else
		{
			pDoubleObject = json_object_new_double(Value);
			if(NULL != pDoubleObject)
			{
				json_object_object_add(pObject, pKey, pDoubleObject);
				//这边虽然手动生成了对象，但是不需要手动释放，因为已经添加挂载到pObject对象上
			}
		}
	}
}






//结构体释放部分
int Lenovo_Json_ppJsonStrFree_SsidPwd(Lenovo_pSsidPwd *ppSsidPwd)
{
	int ret = 0;
	
	if((NULL == ppSsidPwd)||(NULL == *ppSsidPwd))
	{
		DEBUG("Error of parameter");
		ret = -1;
	}
	else
	{
		if(NULL != (*ppSsidPwd)->pSsid)
		{
			free((*ppSsidPwd)->pSsid);
			(*ppSsidPwd)->pSsid = NULL;
		}
		
		if(NULL != (*ppSsidPwd)->pPassword)
		{
			free((*ppSsidPwd)->pPassword);
			(*ppSsidPwd)->pPassword = NULL;
		}
		
		free(*ppSsidPwd);
		*ppSsidPwd = NULL;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree_HubMac(Lenovo_pHubMac *ppHubMac)
{
	int ret = 0;
	
	if((NULL == ppHubMac)||(NULL == *ppHubMac))
	{
		DEBUG("Error of parameter");
		ret = -1;
	}
	else
	{
		if(NULL != (*ppHubMac)->pHubType)
		{
			free((*ppHubMac)->pHubType);
			(*ppHubMac)->pHubType = NULL;
		}
		
		if(NULL != (*ppHubMac)->pMacAddr)
		{
			free((*ppHubMac)->pMacAddr);
			(*ppHubMac)->pMacAddr = NULL;
		}
		
		free(*ppHubMac);
		*ppHubMac = NULL;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree_DeviceInfo(Lenovo_pDeviceInfo *ppDeviceInfo)
{
	int ret = 0;
	
	if((NULL == ppDeviceInfo)||(NULL == *ppDeviceInfo))
	{
		DEBUG("Error of parameter");
		ret = -1;
	}
	else
	{
		if(NULL != (*ppDeviceInfo)->pProductId)
		{
			free((*ppDeviceInfo)->pProductId);
			(*ppDeviceInfo)->pProductId = NULL;
		}
		
		if(NULL != (*ppDeviceInfo)->pDsn)
		{
			free((*ppDeviceInfo)->pDsn);
			(*ppDeviceInfo)->pDsn = NULL;
		}
		
		if(NULL != (*ppDeviceInfo)->pCodeChallenge)
		{
			free((*ppDeviceInfo)->pCodeChallenge);
			(*ppDeviceInfo)->pCodeChallenge = NULL;
		}
		
		if(NULL != (*ppDeviceInfo)->pCodeChallengeMethod)
		{
			free((*ppDeviceInfo)->pCodeChallengeMethod);
			(*ppDeviceInfo)->pCodeChallengeMethod = NULL;
		}
		
		free(*ppDeviceInfo);
		*ppDeviceInfo = NULL;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree_CompanionInfo(Lenovo_pCompanionInfo *ppCompanionInfo)
{
	int ret = 0;
	
	if((NULL == ppCompanionInfo)||(NULL == *ppCompanionInfo))
	{
		DEBUG("Error of parameter");
		ret = -1;
	}
	else
	{
		if(NULL != (*ppCompanionInfo)->pAuthCode)
		{
			free((*ppCompanionInfo)->pAuthCode);
			(*ppCompanionInfo)->pAuthCode = NULL;
		}
		
		if(NULL != (*ppCompanionInfo)->pClientId)
		{
			free((*ppCompanionInfo)->pClientId);
			(*ppCompanionInfo)->pClientId = NULL;
		}
		
		if(NULL != (*ppCompanionInfo)->pRedirectUri)
		{
			free((*ppCompanionInfo)->pRedirectUri);
			(*ppCompanionInfo)->pRedirectUri = NULL;
		}
		
		if(NULL != (*ppCompanionInfo)->pSessionId)
		{
			free((*ppCompanionInfo)->pSessionId);
			(*ppCompanionInfo)->pSessionId = NULL;
		}
		
		free(*ppCompanionInfo);
		*ppCompanionInfo = NULL;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree(Lenovo_pJsonStr *ppJsonStr)
{
	int ret = 0;
	
	if((ppJsonStr == NULL) || (*ppJsonStr == NULL))
	{
		ret = -1;
	} 
	else 
	{
		switch ((*ppJsonStr)->Type)
		{
			case LENOVO_JSON_TYPE_SSIDPWD:
				if((*ppJsonStr)->JsonStr.pSsidPwd != NULL)
				{
					ret = Lenovo_Json_ppJsonStrFree_SsidPwd(&((*ppJsonStr)->JsonStr.pSsidPwd));
				}
				break;
			
			case LENOVO_JSON_TYPE_HUBMAC:
				if((*ppJsonStr)->JsonStr.pHubMac != NULL)
				{
					ret = Lenovo_Json_ppJsonStrFree_HubMac(&((*ppJsonStr)->JsonStr.pHubMac));
				}
				break;
			
			case LENOVO_JSON_TYPE_DEVICEINFO:
				if((*ppJsonStr)->JsonStr.pDeviceInfo != NULL)
				{
					ret = Lenovo_Json_ppJsonStrFree_DeviceInfo(&((*ppJsonStr)->JsonStr.pDeviceInfo));
				}
				break;
			
			case LENOVO_JSON_TYPE_COMPANIONINFO:
				if((*ppJsonStr)->JsonStr.pCompanionInfo != NULL)
				{
					ret = Lenovo_Json_ppJsonStrFree_CompanionInfo(&((*ppJsonStr)->JsonStr.pCompanionInfo));
				}
				break;

			default:
				ret = -1;
				break;
		}
		
		free(*ppJsonStr);
		*ppJsonStr = NULL;//就算上面某个没有释放成功，返回前该释放的还是要释放
	}
	
	return ret;
}




//结构体构造部分
Lenovo_pSsidPwd Lenovo_Json_pJsonStringTopJsonStr_SsidPwd(char * pJsonString)
{
	Lenovo_pSsidPwd ret;
	struct json_object *object;
	
	if(pJsonString==NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	ret = (Lenovo_pSsidPwd)calloc(1,sizeof(Lenovo_SsidPwd));
	if(ret==NULL)
	{
		DEBUG("Error of calloc");
		return NULL;
	}
	
	object = json_tokener_parse(pJsonString);//根据标志将字符串转为JSON对象，之前一直不理解这个函数是什么意思
	
	if(!is_error(object))
	{
		//结构体对应的字段进行赋值
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "ssid", &(ret->pSsid));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "pwd", &(ret->pPassword));
		
		json_object_put(object);//转换成结构体之后将JSONobject释放
	}
	
	return ret;
}

Lenovo_pHubMac Lenovo_Json_pJsonStringTopJsonStr_HubMac(char * pJsonString)
{
	Lenovo_pHubMac ret;
	struct json_object *object;
	
	if(pJsonString==NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	ret = (Lenovo_pHubMac)calloc(1,sizeof(Lenovo_HubMac));
	if(ret==NULL)
	{
		DEBUG("Error of calloc");
		return NULL;
	}
	
	object = json_tokener_parse(pJsonString);//根据标志将字符串转为JSON对象，之前一直不理解这个函数是什么意思
	
	if(!is_error(object))
	{
		//结构体对应的字段进行赋值
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "hubtype", &(ret->pHubType));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "macaddr", &(ret->pMacAddr));
		
		json_object_put(object);//转换成结构体之后将JSONobject释放
	}
	
	return ret;
}

Lenovo_pDeviceInfo Lenovo_Json_pJsonStringTopJsonStr_DeviceInfo(char * pJsonString)
{
	Lenovo_pDeviceInfo ret;
	struct json_object *object;
	
	if(pJsonString==NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	ret = (Lenovo_pDeviceInfo)calloc(1,sizeof(Lenovo_DeviceInfo));
	if(ret==NULL)
	{
		DEBUG("Error of calloc");
		return NULL;
	}
	
	object = json_tokener_parse(pJsonString);//根据标志将字符串转为JSON对象，之前一直不理解这个函数是什么意思
	
	if(!is_error(object))
	{
		//结构体对应的字段进行赋值
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "productId", &(ret->pProductId));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "dsn", &(ret->pDsn));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "codeChallenge", &(ret->pCodeChallenge));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "codeChallengeMethod", &(ret->pCodeChallengeMethod));
		
		json_object_put(object);//转换成结构体之后将JSONobject释放
	}
	
	return ret;
}

Lenovo_pCompanionInfo Lenovo_Json_pJsonStringTopJsonStr_CompanionInfo(char *pJsonString)
{
	Lenovo_pCompanionInfo ret;
	struct json_object *object;
	
	if(pJsonString==NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	ret = (Lenovo_pCompanionInfo)calloc(1,sizeof(Lenovo_CompanionInfo));
	if(ret==NULL)
	{
		DEBUG("Error of calloc");
		return NULL;
	}
	
	object = json_tokener_parse(pJsonString);//根据标志将字符串转为JSON对象，之前一直不理解这个函数是什么意思
	
	if(!is_error(object))
	{
		//结构体对应的字段进行赋值
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "authCode", &(ret->pAuthCode));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "clientId", &(ret->pClientId));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "redirectUri", &(ret->pRedirectUri));
		
		Lenovo_Json_GetStringJsonStringFromJsonObject(object, "sessionId", &(ret->pSessionId));
		
		json_object_put(object);//转换成结构体之后将JSONobject释放
	}
	
	return ret;
}

Lenovo_pJsonStr Lenovo_Json_pJsonStringTopJsonStr(enum Lenovo_Json_Type type, void *pJsonString)
{
	Lenovo_pJsonStr pJsonStr = NULL;
	
	if (pJsonString == NULL)
	{
		DEBUG("Error of parameter");
		pJsonStr = NULL;
	}
	else
	{
		pJsonStr = (Lenovo_pJsonStr)calloc(1,sizeof(Lenovo_JsonStr));
		if (pJsonStr == NULL)
		{
			DEBUG("Error of calloc");
		}
		else
		{
			pJsonStr->Type = type;
			
			switch (type)
			{
				case LENOVO_JSON_TYPE_SSIDPWD:
					pJsonStr->JsonStr.pSsidPwd = Lenovo_Json_pJsonStringTopJsonStr_SsidPwd(pJsonString);
					break;
					
				case LENOVO_JSON_TYPE_HUBMAC:
					pJsonStr->JsonStr.pHubMac = Lenovo_Json_pJsonStringTopJsonStr_HubMac(pJsonString);
					break;
					
				case LENOVO_JSON_TYPE_DEVICEINFO:
					pJsonStr->JsonStr.pDeviceInfo = Lenovo_Json_pJsonStringTopJsonStr_DeviceInfo(pJsonString);
					break;
					
				case LENOVO_JSON_TYPE_COMPANIONINFO:
					pJsonStr->JsonStr.pCompanionInfo = Lenovo_Json_pJsonStringTopJsonStr_CompanionInfo(pJsonString);
					break;
					
				default:
					pJsonStr = NULL;
					break;
			}
		}
	}
	
	return pJsonStr;
}





//JSON字符串释放
int Lenovo_Json_ppJsonStringFree(char **ppJsonString)
{
	int ret = 0;
	
	if (ppJsonString == NULL)
	{
		ret = -1;
	} 
	else 
	{
		free(*ppJsonString);
		*ppJsonString = NULL;
	}
	
	return ret;
}




//生成JSON字符串部分
char* Lenovo_Json_pJsonStrTopJsonString_SsidPwd(Lenovo_pSsidPwd pJsonStr)
{
	char *ret;
	struct json_object *object;
	
	if(pJsonStr == NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	object = json_object_new_object();//新建一个空的JSON对相集
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "ssid", pJsonStr->pSsid);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "pwd", pJsonStr->pPassword);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);//转换成字符串之后将JSONobject释放
	
	return ret;
}

char* Lenovo_Json_pJsonStrTopJsonString_HubMac(Lenovo_pHubMac pJsonStr)
{
	char *ret;
	struct json_object *object;
	
	if(pJsonStr == NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	object = json_object_new_object();//新建一个空的JSON对相集
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "hubtype", pJsonStr->pHubType);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "macaddr", pJsonStr->pMacAddr);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);//转换成字符串之后将JSONobject释放
	
	return ret;
}

char* Lenovo_Json_pJsonStrTopJsonString_DeviceInfo(Lenovo_pDeviceInfo pJsonStr)
{
	char *ret;
	struct json_object *object;
	
	if(pJsonStr == NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	object = json_object_new_object();//新建一个空的JSON对相集
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "productId", pJsonStr->pProductId);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "dsn", pJsonStr->pDsn);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeChallenge", pJsonStr->pCodeChallenge);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeChallengeMethod", pJsonStr->pCodeChallengeMethod);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "sessionId", pJsonStr->pSessionId);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);//转换成字符串之后将JSONobject释放
	
	return ret;
}

char* Lenovo_Json_pJsonStrTopJsonString_CompanionInfo(Lenovo_pCompanionInfo pJsonStr)
{
	char *ret;
	struct json_object *object;
	
	if(pJsonStr == NULL)
	{
		DEBUG("Error of parameter");
		return NULL;
	}
	
	object = json_object_new_object();//新建一个空的JSON对相集
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "authCode", pJsonStr->pAuthCode);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "clientId", pJsonStr->pClientId);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "redirectUri", pJsonStr->pRedirectUri);
	
	Lenovo_Json_StingToJsonStringAddJsonObject(object, "sessionId", pJsonStr->pSessionId);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);//转换成字符串之后将JSONobject释放
	
	return ret;
}

char * Lenovo_Json_pJsonStrTopJsonString(Lenovo_pJsonStr pJsonStr)
{
	char *pJsonString = NULL;
	
	switch (pJsonStr->Type)
	{
		case LENOVO_JSON_TYPE_SSIDPWD:
			pJsonString = Lenovo_Json_pJsonStrTopJsonString_SsidPwd(pJsonStr->JsonStr.pSsidPwd);
			break;
			
		case LENOVO_JSON_TYPE_HUBMAC:
			pJsonString = Lenovo_Json_pJsonStrTopJsonString_HubMac(pJsonStr->JsonStr.pHubMac);
			break;
			
		case LENOVO_JSON_TYPE_DEVICEINFO:
			pJsonString = Lenovo_Json_pJsonStrTopJsonString_DeviceInfo(pJsonStr->JsonStr.pDeviceInfo);
			break;
			
		case LENOVO_JSON_TYPE_COMPANIONINFO:
			pJsonString = Lenovo_Json_pJsonStrTopJsonString_CompanionInfo(pJsonStr->JsonStr.pCompanionInfo);
			break;
			
		default:
			pJsonString = NULL;
			break;
	}
	
	return pJsonString;
}
