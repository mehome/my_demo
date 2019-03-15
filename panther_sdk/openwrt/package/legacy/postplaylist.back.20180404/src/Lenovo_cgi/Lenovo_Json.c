#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>

#include "Lenovo_Json.h"
//#include "Lenovo_Debug.h"

//统一从JsonObject当中获取Json的String对象，并保存到String类型的指针当中
void Lenovo_Json_GetStringJsonStringFromJsonObject(struct json_object *pObject, char *pKey, char **ppString)
{
	struct json_object *pStringObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		pStringObject = json_object_object_get(pObject, pKey);
		if(pStringObject == NULL)
		{
			*ppString = NULL;
		} else {
			*ppString = strdup(json_object_get_string(pStringObject));
		}
	}
}

//统一从JsonObject当中获取Json对象，并保存到int类型当中，不论是Json的Int类型还是String类型
void Lenovo_Json_GetIntFromJsonObject(struct json_object *pObject, char *pKey, int *pValue)
{
	struct json_object *pIntObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		pIntObject = json_object_object_get(pObject, pKey);
		if(pIntObject == NULL)
		{
			*pValue = LENOVO_JSON_INTERRORCODE;
		} else {
			*pValue = json_object_get_int(pIntObject);
		}
	}
}

//统一从JsonObject当中获取Json对象，并保存到double类型当中
void Lenovo_Json_GetDoubleFromJsonObject(struct json_object *pObject, char *pKey, double *pValue)
{
	struct json_object *pDoubleObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		pDoubleObject = json_object_object_get(pObject, pKey);
		if(pDoubleObject == NULL)
		{
			*pValue = LENOVO_JSON_INTERRORCODE;
		} else {
			*pValue = json_object_get_double(pDoubleObject);
		}
	}
}

//统一压缩int成Json的int对象，并添加挂载到JsonObject
void Lenovo_Json_IntToJsonIntAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pIntObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		} else {
			pIntObject = json_object_new_int(Value);
			if(pIntObject != NULL)
			{
				json_object_object_add(pObject, pKey, pIntObject);
			}
		}
	}
}

//统一压缩String成Json的string对象，并添加挂载到JsonObject
void Lenovo_Json_StingToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, char *pString)
{
	struct json_object *pStringObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		if(NULL == pString)
		{
			json_object_object_add(pObject, pKey, NULL);
		} else {
			pStringObject = json_object_new_string(pString);
			if(pStringObject != NULL)
			{
				json_object_object_add(pObject, pKey, pStringObject);
			}
		}
	}
}

//统一压缩int成Json的string对象，并添加挂载到JsonObject
void Lenovo_Json_IntToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pStringObject = NULL;
	char tmp[64];
	
	if((pObject != NULL) && (pKey != NULL))
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		} else {
			memset(tmp, 0, sizeof(tmp)/sizeof(char));
			sprintf(tmp, "%d", Value);
			pStringObject = json_object_new_string(tmp);
			if(NULL != pStringObject)
			{
				json_object_object_add(pObject, pKey, pStringObject);
			}
		}
	}
}

//统一压缩double成Json的double对象，并添加挂载到JsonObject
void Lenovo_Json_DoubleToJsonDoubleAddJsonObject(struct json_object *pObject, char *pKey, double Value)
{
	struct json_object *pDoubleObject = NULL;
	
	if((pObject != NULL) && (pKey != NULL))
	{
		if(LENOVO_JSON_INTERRORCODE == Value)
		{
			json_object_object_add(pObject, pKey, NULL);
		} else {
			pDoubleObject = json_object_new_double(Value);
			if(pDoubleObject != NULL)
			{
				json_object_object_add(pObject, pKey, pDoubleObject);
			}
		}
	}
}

//结构体释放部分
int Lenovo_Json_ppJsonStrFree_DeviceInfo(Lenovo_pDeviceInfo *ppDeviceInfo)
{
	int ret = -1;
	
	if((ppDeviceInfo != NULL) && (*ppDeviceInfo != NULL))
	{
		if((*ppDeviceInfo)->pProductId != NULL)
		{
			free((*ppDeviceInfo)->pProductId);
			(*ppDeviceInfo)->pProductId = NULL;
		}
		
		if((*ppDeviceInfo)->pDsn != NULL)
		{
			free((*ppDeviceInfo)->pDsn);
			(*ppDeviceInfo)->pDsn = NULL;
		}
		
		if((*ppDeviceInfo)->pCodeChallenge != NULL)
		{
			free((*ppDeviceInfo)->pCodeChallenge);
			(*ppDeviceInfo)->pCodeChallenge = NULL;
		}
		
		if((*ppDeviceInfo)->pCodeChallengeMethod != NULL)
		{
			free((*ppDeviceInfo)->pCodeChallengeMethod);
			(*ppDeviceInfo)->pCodeChallengeMethod = NULL;
		}
		
		free(*ppDeviceInfo);
		*ppDeviceInfo = NULL;
		
		ret = 0;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree_CompanionInfo(Lenovo_pCompanionInfo *ppCompanionInfo)
{
	int ret = -1;
	
	if((ppCompanionInfo != NULL) && (*ppCompanionInfo != NULL))
	{
		if((*ppCompanionInfo)->pAuthorizationCode != NULL)
		{
			free((*ppCompanionInfo)->pAuthorizationCode);
			(*ppCompanionInfo)->pAuthorizationCode = NULL;
		}
		
		if((*ppCompanionInfo)->pClientId != NULL)
		{
			free((*ppCompanionInfo)->pClientId);
			(*ppCompanionInfo)->pClientId = NULL;
		}
		
		if((*ppCompanionInfo)->pRedirectUri != NULL)
		{
			free((*ppCompanionInfo)->pRedirectUri);
			(*ppCompanionInfo)->pRedirectUri = NULL;
		}
		
		// 预留，暂时不用
		//if((*ppCompanionInfo)->pSessionId != NULL)
		//{
		//	free((*ppCompanionInfo)->pSessionId);
		//	(*ppCompanionInfo)->pSessionId = NULL;
		//}
		
		free(*ppCompanionInfo);
		*ppCompanionInfo = NULL;
		
		ret = 0;
	}
	
	return ret;
}

int Lenovo_Json_ppJsonStrFree(Lenovo_pJsonStr *ppJsonStr)
{
	int ret = -1;
	
	if((ppJsonStr != NULL) && (*ppJsonStr != NULL))
	{
		switch ((*ppJsonStr)->Type)
		{
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
				return ret;
				break;
		}
		
		free(*ppJsonStr);
		*ppJsonStr = NULL;
		
		ret = 0;
	}
	
	return ret;
}

//结构体构造部分
Lenovo_pDeviceInfo Lenovo_Json_pJsonStringTopJsonStr_DeviceInfo(char *pJsonString)
{
	Lenovo_pDeviceInfo ret = NULL;
	struct json_object *object = NULL;
	
	if(pJsonString!=NULL)
	{
		ret = (Lenovo_pDeviceInfo)calloc(1, sizeof(Lenovo_DeviceInfo));
		if(ret != NULL)
		{
			object = json_tokener_parse(pJsonString);
			if(!is_error(object))
			{
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "productId", &(ret->pProductId));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "dsn", &(ret->pDsn));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "codeChallenge", &(ret->pCodeChallenge));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "codeChallengeMethod", &(ret->pCodeChallengeMethod));
				
				json_object_put(object);
			} else {
			
			}
		}
	}
		
	return ret;
}

Lenovo_pCompanionInfo Lenovo_Json_pJsonStringTopJsonStr_CompanionInfo(char *pJsonString)
{
	Lenovo_pCompanionInfo ret = NULL;
	struct json_object *object = NULL;
	
	if(pJsonString != NULL)
	{
		ret = (Lenovo_pCompanionInfo)calloc(1, sizeof(Lenovo_CompanionInfo));
		if(ret != NULL)
		{
			object = json_tokener_parse(pJsonString);
			
			if(!is_error(object))
			{
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "authorizationCode", &(ret->pAuthorizationCode));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "redirectUri", &(ret->pRedirectUri));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "clientId", &(ret->pClientId));
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "codeVerifier", &(ret->pCodeVerifier));
				
				json_object_put(object);
			}
		}
	}
	
	return ret;
}

Lenovo_pJsonStr Lenovo_Json_pJsonStringTopJsonStr(enum Lenovo_Json_Type type, void *pJsonString)
{
	Lenovo_pJsonStr pJsonStr = NULL;
	
	if (pJsonString != NULL)
	{
		pJsonStr = (Lenovo_pJsonStr)calloc(1, sizeof(Lenovo_JsonStr));
		if (pJsonStr != NULL)
		{
			pJsonStr->Type = type;
			
			switch (type)
			{
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
	int ret = -1;
	
	if (ppJsonString != NULL)
	{
		free(*ppJsonString);
		*ppJsonString = NULL;
		
		ret = 0;
	}
	
	return ret;
}

//生成JSON字符串部分
char* Lenovo_Json_pJsonStrTopJsonString_DeviceInfo(Lenovo_pDeviceInfo pJsonStr)
{
	char *ret = NULL;
	struct json_object *object = NULL;
	
	if(pJsonStr != NULL)
	{
		object = json_object_new_object();
		
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "ret", "OK");
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "ProductId", pJsonStr->pProductId);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "Dsn", pJsonStr->pDsn);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "CodeChallenge", pJsonStr->pCodeChallenge);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "CodeVerifier", pJsonStr->pCodeVerifier);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "CodeChallengeMethod", pJsonStr->pCodeChallengeMethod);
		ret = strdup(json_object_to_json_string(object));
		
		json_object_put(object);
	}
	
	return ret;
}

char* Lenovo_Json_pJsonStrTopJsonString_CompanionInfo(Lenovo_pCompanionInfo pJsonStr)
{
	char *ret = NULL;
	struct json_object *object = NULL;
	
	if(pJsonStr != NULL)
	{
		object = json_object_new_object();
		
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "authorizationCode", pJsonStr->pAuthorizationCode);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "redirectUri", pJsonStr->pRedirectUri);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "clientId", pJsonStr->pClientId);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeVerifier", pJsonStr->pCodeVerifier);
//		Lenovo_Json_StingToJsonStringAddJsonObject(object, "sessionId", pJsonStr->pSessionId);
		
		ret = strdup(json_object_to_json_string(object));
		
		json_object_put(object);
	}
	
	return ret;
}

char* Lenovo_Json_pJsonStrTopJsonString(Lenovo_pJsonStr pJsonStr)
{
	char *pJsonString = NULL;
	
	switch (pJsonStr->Type)
	{
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

