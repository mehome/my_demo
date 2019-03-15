#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>

#include "amazon_Json.h"

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
				Lenovo_Json_GetStringJsonStringFromJsonObject(object, "authCode", &(ret->pAuthorizationCode));
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

char* Lenovo_Json_pJsonStrTopJsonString_DeviceInfo(Lenovo_pDeviceInfo pJsonStr)
{
	char *ret = NULL;
	struct json_object *object = NULL;
	
	if(pJsonStr != NULL)
	{
		object = json_object_new_object();
		
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "ret", "OK");
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "productId", pJsonStr->pProductId);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "dsn", pJsonStr->pDsn);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeChallenge", pJsonStr->pCodeChallenge);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeVerifier", pJsonStr->pCodeVerifier);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeChallengeMethod", pJsonStr->pCodeChallengeMethod);
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
		
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "authCode", pJsonStr->pAuthorizationCode);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "redirectUri", pJsonStr->pRedirectUri);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "clientId", pJsonStr->pClientId);
		Lenovo_Json_StingToJsonStringAddJsonObject(object, "codeVerifier", pJsonStr->pCodeVerifier);
		
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

