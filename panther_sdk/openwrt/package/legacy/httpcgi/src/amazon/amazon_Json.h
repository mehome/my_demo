#ifndef __LENOVO_JSON_H__
#define __LENOVO_JSON_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define LENOVO_JSON_INTERRORCODE (~0)

#include <json/json.h> 

typedef struct _Lenovo_DeviceInfo
{
	char *pProductId;
	char *pDsn;
	char *pCodeChallenge;
	char *pCodeVerifier;
	char *pCodeChallengeMethod;
}Lenovo_DeviceInfo,*Lenovo_pDeviceInfo;

typedef struct _Lenovo_CompanionInfo
{
	char *pAuthorizationCode;
	char *pRedirectUri;
	char *pClientId;
	char *pCodeVerifier;
}Lenovo_CompanionInfo,*Lenovo_pCompanionInfo;

enum Lenovo_Json_Type
{
	LENOVO_JSON_TYPE_DEVICEINFO,
	LENOVO_JSON_TYPE_COMPANIONINFO,
};

typedef struct _Lenovo_JsonStr
{
	enum Lenovo_Json_Type Type;
	union 
	{
		Lenovo_pDeviceInfo pDeviceInfo;
		Lenovo_pCompanionInfo pCompanionInfo;
	}JsonStr;
}Lenovo_JsonStr, *Lenovo_pJsonStr;

extern int Lenovo_Json_ppJsonStrFree(Lenovo_pJsonStr *ppJsonStr);
extern Lenovo_pJsonStr Lenovo_Json_pJsonStringTopJsonStr(enum Lenovo_Json_Type type, void *pJsonString);
extern int Lenovo_Json_ppJsonStringFree(char **ppJsonString);
extern char* Lenovo_Json_pJsonStrTopJsonString(Lenovo_pJsonStr pJsonStr);

#ifdef __cplusplus
}
#endif

#endif	// __LENOVO_JSON_H__

