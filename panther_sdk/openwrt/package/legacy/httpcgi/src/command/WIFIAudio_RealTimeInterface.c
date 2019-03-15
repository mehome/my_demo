#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>  
#include <sys/socket.h>
#include <json/json.h> 

#include "WIFIAudio_RealTimeInterface.h"

void WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(struct json_object *pObject, char *pKey, char **ppString)
{
	struct json_object *pStringObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		pStringObject = json_object_object_get(pObject, pKey);
		if(NULL == pStringObject)
		{
			*ppString = NULL;
		} else {
			*ppString = strdup(json_object_get_string(pStringObject));
		}
	}
}

void WIFIAudio_RealTimeInterface_GetIntFromJsonObject(struct json_object *pObject, char *pKey, int *pValue)
{
	struct json_object *pIntObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		pIntObject = json_object_object_get(pObject, pKey);
		if(NULL == pIntObject)
		{
			*pValue = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
		} else {
			*pValue = json_object_get_int(pIntObject);
		}
	}
}

void WIFIAudio_RealTimeInterface_GetDoubleFromJsonObject(struct json_object *pObject, char *pKey, double *pValue)
{
	struct json_object *pDoubleObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		pDoubleObject = json_object_object_get(pObject, pKey);
		if(NULL == pDoubleObject)
		{
			*pValue = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
		} else {
			*pValue = json_object_get_double(pDoubleObject);
		}
	}
}

void WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pIntObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		if(WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE == Value)
		{
		} else {
			pIntObject = json_object_new_int(Value);
			if(NULL != pIntObject)
			{
				json_object_object_add(pObject, pKey, pIntObject);
			}
		}
	}
}

void WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject_WithoutError(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pIntObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		pIntObject = json_object_new_int(Value);
		if(NULL != pIntObject)
		{
			json_object_object_add(pObject, pKey, pIntObject);
		}
	}
}

void WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, char *pString)
{
	struct json_object *pStringObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		if(NULL == pString)
		{
		} else {
			pStringObject = json_object_new_string(pString);
			if(NULL != pStringObject)
			{
				json_object_object_add(pObject, pKey, pStringObject);
			}
		}
	}
}

void WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(struct json_object *pObject, char *pKey, int Value)
{
	struct json_object *pStringObject = NULL;
	char tmp[64];
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		if(WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE == Value)
		{
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

void WIFIAudio_RealTimeInterface_DoubleToJsonDoubleAddJsonObject(struct json_object *pObject, char *pKey, double Value)
{
	struct json_object *pDoubleObject = NULL;
	
	if((NULL == pObject) || (NULL == pKey))
	{
	} else {
		if(WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE == Value)
		{
		} else {
			pDoubleObject = json_object_new_double(Value);
			if(NULL != pDoubleObject)
			{
				json_object_object_add(pObject, pKey, pDoubleObject);
			}
		}
	}
}

int WIFIAudio_RealTimeInterface_pGetStatusFree(WARTI_pGetStatus *ppgetstatus)
{
	int ret = 0;
	
	if((NULL == ppgetstatus)||(NULL == *ppgetstatus))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetstatus)->pLanguage)
		{
			free((*ppgetstatus)->pLanguage);
			(*ppgetstatus)->pLanguage = NULL;
		}
		if(NULL != (*ppgetstatus)->pSSID)
		{
			free((*ppgetstatus)->pSSID);
			(*ppgetstatus)->pSSID = NULL;
		}
		if(NULL != (*ppgetstatus)->pFirmware)
		{
			free((*ppgetstatus)->pFirmware);
			(*ppgetstatus)->pFirmware = NULL;
		}
		if(NULL != (*ppgetstatus)->pBuildDate)
		{
			free((*ppgetstatus)->pBuildDate);
			(*ppgetstatus)->pBuildDate = NULL;
		}
		if(NULL != (*ppgetstatus)->pRelease)
		{
			free((*ppgetstatus)->pRelease);
			(*ppgetstatus)->pRelease = NULL;
		}
		if(NULL != (*ppgetstatus)->pGroup)
		{
			free((*ppgetstatus)->pGroup);
			(*ppgetstatus)->pGroup = NULL;
		}
		if(NULL != (*ppgetstatus)->pESSID)
		{
			free((*ppgetstatus)->pESSID);
			(*ppgetstatus)->pESSID = NULL;
		}
		if(NULL != (*ppgetstatus)->pHardware)
		{
			free((*ppgetstatus)->pHardware);
			(*ppgetstatus)->pHardware = NULL;
		}
		free(*ppgetstatus);
		*ppgetstatus = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pAccessPointFree(WARTI_pAccessPoint *ppaccespoint)
{
	int ret = 0;
	
	if((NULL == ppaccespoint)||(NULL == *ppaccespoint))
	{
		ret = -1;
	} else {
		if(NULL != (*ppaccespoint)->pSSID)
		{
			free((*ppaccespoint)->pSSID);
			(*ppaccespoint)->pSSID = NULL;
		}
		
		if(NULL != (*ppaccespoint)->pAuth)
		{
			free((*ppaccespoint)->pAuth);
			(*ppaccespoint)->pAuth = NULL;
		}
		
		if(NULL != (*ppaccespoint)->pEncry)
		{
			free((*ppaccespoint)->pEncry);
			(*ppaccespoint)->pEncry = NULL;
		}
		
		free(*ppaccespoint);
		*ppaccespoint = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pWLANGetAPlistFree(WARTI_pWLANGetAPlist *ppwlangetaplist)
{
	int i;
	int ret = 0;
	if((NULL == ppwlangetaplist)||(NULL == *ppwlangetaplist))
	{
		ret = -1;
	} else {
		if(NULL != (*ppwlangetaplist)->pRet)
		{
			free((*ppwlangetaplist)->pRet);
			(*ppwlangetaplist)->pRet = NULL;
		}
		if(NULL != ((*ppwlangetaplist)->pAplist))
		{
			for(i = 0; i<((*ppwlangetaplist)->Res); i++)
			{
				if(NULL != (((*ppwlangetaplist)->pAplist)[i]))
				{
					WIFIAudio_RealTimeInterface_pAccessPointFree(&(((*ppwlangetaplist)->pAplist)[i]));
				}
			}
			free((*ppwlangetaplist)->pAplist);
			(*ppwlangetaplist)->pAplist = NULL;
		}
		free(*ppwlangetaplist);
		*ppwlangetaplist = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetCurrentWirelessConnectFree(WARTI_pGetCurrentWirelessConnect *ppgetcurrentwirelessconnect)
{
	int ret = 0;
	if((NULL == ppgetcurrentwirelessconnect)||(NULL == *ppgetcurrentwirelessconnect))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetcurrentwirelessconnect)->pRet)
		{
			free((*ppgetcurrentwirelessconnect)->pRet);
			(*ppgetcurrentwirelessconnect)->pRet = NULL;
		}
		if(NULL != (*ppgetcurrentwirelessconnect)->pSSID)
		{
			free((*ppgetcurrentwirelessconnect)->pSSID);
			(*ppgetcurrentwirelessconnect)->pSSID = NULL;
		}
		
		free(*ppgetcurrentwirelessconnect);
		*ppgetcurrentwirelessconnect = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetDeviceInfoFree(WARTI_pGetDeviceInfo *ppgetdeviceinfo)
{
	int ret = 0;
	
	if((NULL == ppgetdeviceinfo)||(NULL == *ppgetdeviceinfo))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetdeviceinfo)->pRet)
		{
			free((*ppgetdeviceinfo)->pRet);
			(*ppgetdeviceinfo)->pRet = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pLanguage)
		{
			free((*ppgetdeviceinfo)->pLanguage);
			(*ppgetdeviceinfo)->pLanguage = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pSSID)
		{
			free((*ppgetdeviceinfo)->pSSID);
			(*ppgetdeviceinfo)->pSSID = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pFirmware)
		{
			free((*ppgetdeviceinfo)->pFirmware);
			(*ppgetdeviceinfo)->pFirmware = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pRelease)
		{
			free((*ppgetdeviceinfo)->pRelease);
			(*ppgetdeviceinfo)->pRelease = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pESSID)
		{
			free((*ppgetdeviceinfo)->pESSID);
			(*ppgetdeviceinfo)->pESSID = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pProject)
		{
			free((*ppgetdeviceinfo)->pProject);
			(*ppgetdeviceinfo)->pProject = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pDeviceName)
		{
			free((*ppgetdeviceinfo)->pDeviceName);
			(*ppgetdeviceinfo)->pDeviceName = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pLanguageSupport)
		{
			free((*ppgetdeviceinfo)->pLanguageSupport);
			(*ppgetdeviceinfo)->pLanguageSupport = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pFunctionSupport)
		{
			free((*ppgetdeviceinfo)->pFunctionSupport);
			(*ppgetdeviceinfo)->pFunctionSupport = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pMAC)
		{
			free((*ppgetdeviceinfo)->pMAC);
			(*ppgetdeviceinfo)->pMAC = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pUUID)
		{
			free((*ppgetdeviceinfo)->pUUID);
			(*ppgetdeviceinfo)->pUUID = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pBtVersion)
		{
			free((*ppgetdeviceinfo)->pBtVersion);
			(*ppgetdeviceinfo)->pBtVersion = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pConexantVersion)
		{
			free((*ppgetdeviceinfo)->pConexantVersion);
			(*ppgetdeviceinfo)->pConexantVersion = NULL;
		}
		if(NULL != (*ppgetdeviceinfo)->pPowerSupply)
		{
			free((*ppgetdeviceinfo)->pPowerSupply);
			(*ppgetdeviceinfo)->pPowerSupply = NULL;
		}
		free(*ppgetdeviceinfo);
		*ppgetdeviceinfo = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetOneTouchConfigureFree(WARTI_pGetOneTouchConfigure *ppgetonetouchconfigure)
{
	int ret = 0;
	
	if((NULL == ppgetonetouchconfigure)||(NULL == *ppgetonetouchconfigure))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetonetouchconfigure)->pRet)
		{
			free((*ppgetonetouchconfigure)->pRet);
			(*ppgetonetouchconfigure)->pRet = NULL;
		}
		
		free(*ppgetonetouchconfigure);
		*ppgetonetouchconfigure = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetOneTouchConfigureSetDeviceNameFree(WARTI_pGetOneTouchConfigureSetDeviceName *ppgetonetouchconfiguresetdevicename)
{
	int ret = 0;
	
	if((NULL == ppgetonetouchconfiguresetdevicename)||(NULL == *ppgetonetouchconfiguresetdevicename))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetonetouchconfiguresetdevicename)->pRet)
		{
			free((*ppgetonetouchconfiguresetdevicename)->pRet);
			(*ppgetonetouchconfiguresetdevicename)->pRet = NULL;
		}
		
		free(*ppgetonetouchconfiguresetdevicename);
		*ppgetonetouchconfiguresetdevicename = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetLoginAmazonStatusFree(WARTI_pGetLoginAmazonStatus *ppgetloginamazonstatus)
{
	int ret = 0;
	
	if((NULL == ppgetloginamazonstatus)||(NULL == *ppgetloginamazonstatus))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetloginamazonstatus)->pRet)
		{
			free((*ppgetloginamazonstatus)->pRet);
			(*ppgetloginamazonstatus)->pRet = NULL;
		}
		
		free(*ppgetloginamazonstatus);
		*ppgetloginamazonstatus = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetLoginAmazonNeedInfoFree(WARTI_pGetLoginAmazonNeedInfo *ppgetloginamazonneedinfo)
{
	int ret = 0;
	
	if((NULL == ppgetloginamazonneedinfo)||(NULL == *ppgetloginamazonneedinfo))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetloginamazonneedinfo)->pRet)
		{
			free((*ppgetloginamazonneedinfo)->pRet);
			(*ppgetloginamazonneedinfo)->pRet = NULL;
		}
		if(NULL != (*ppgetloginamazonneedinfo)->pProductId)
		{
			free((*ppgetloginamazonneedinfo)->pProductId);
			(*ppgetloginamazonneedinfo)->pProductId = NULL;
		}
		if(NULL != (*ppgetloginamazonneedinfo)->pDsn)
		{
			free((*ppgetloginamazonneedinfo)->pDsn);
			(*ppgetloginamazonneedinfo)->pDsn = NULL;
		}
		if(NULL != (*ppgetloginamazonneedinfo)->pCodeChallenge)
		{
			free((*ppgetloginamazonneedinfo)->pCodeChallenge);
			(*ppgetloginamazonneedinfo)->pCodeChallenge = NULL;
		}
		if(NULL != (*ppgetloginamazonneedinfo)->pCodeChallengeMethod)
		{
			free((*ppgetloginamazonneedinfo)->pCodeChallengeMethod);
			(*ppgetloginamazonneedinfo)->pCodeChallengeMethod = NULL;
		}
		
		free(*ppgetloginamazonneedinfo);
		*ppgetloginamazonneedinfo = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pPostLoginAmazonInfoFree(WARTI_pPostLoginAmazonInfo *pppostloginamazoninfo)
{
	int ret = 0;
	
	if((NULL == pppostloginamazoninfo)||(NULL == *pppostloginamazoninfo))
	{
		ret = -1;
	} else {
		if(NULL != (*pppostloginamazoninfo)->pAuthorizationCode)
		{
			free((*pppostloginamazoninfo)->pAuthorizationCode);
			(*pppostloginamazoninfo)->pAuthorizationCode = NULL;
		}
		if(NULL != (*pppostloginamazoninfo)->pRedirectUri)
		{
			free((*pppostloginamazoninfo)->pRedirectUri);
			(*pppostloginamazoninfo)->pRedirectUri = NULL;
		}
		if(NULL != (*pppostloginamazoninfo)->pClientId)
		{
			free((*pppostloginamazoninfo)->pClientId);
			(*pppostloginamazoninfo)->pClientId = NULL;
		}
		
		free(*pppostloginamazoninfo);
		*pppostloginamazoninfo = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetPlayerStatusFree(WARTI_pGetPlayerStatus *ppgetplayerstatus)
{
	int ret = 0;
	
	if((NULL == ppgetplayerstatus)||(NULL == *ppgetplayerstatus))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetplayerstatus)->pRet)
		{
			free((*ppgetplayerstatus)->pRet);
			(*ppgetplayerstatus)->pRet = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pStatus)
		{
			free((*ppgetplayerstatus)->pStatus);
			(*ppgetplayerstatus)->pStatus = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pTitle)
		{
			free((*ppgetplayerstatus)->pTitle);
			(*ppgetplayerstatus)->pTitle = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pArtist)
		{
			free((*ppgetplayerstatus)->pArtist);
			(*ppgetplayerstatus)->pArtist = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pAlbum)
		{
			free((*ppgetplayerstatus)->pAlbum);
			(*ppgetplayerstatus)->pAlbum = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pGenre)
		{
			free((*ppgetplayerstatus)->pGenre);
			(*ppgetplayerstatus)->pGenre = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pLocalListFile)
		{
			free((*ppgetplayerstatus)->pLocalListFile);
			(*ppgetplayerstatus)->pLocalListFile = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pIURI)
		{
			free((*ppgetplayerstatus)->pIURI);
			(*ppgetplayerstatus)->pIURI = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pURI)
		{
			free((*ppgetplayerstatus)->pURI);
			(*ppgetplayerstatus)->pURI = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pCover)
		{
			free((*ppgetplayerstatus)->pCover);
			(*ppgetplayerstatus)->pCover = NULL;
		}
		if(NULL != (*ppgetplayerstatus)->pUUID)
		{
			free((*ppgetplayerstatus)->pUUID);
			(*ppgetplayerstatus)->pUUID = NULL;
		}
		
		free(*ppgetplayerstatus);
		*ppgetplayerstatus = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMusicFree(WARTI_pMusic *ppmusic)
{
	int ret = 0;
	
	if((NULL == ppmusic) || (NULL == *ppmusic))
	{
		ret = -1;
	} else {
		if(NULL != (*ppmusic)->pTitle)
		{
			free((*ppmusic)->pTitle);
			(*ppmusic)->pTitle = NULL;
		}
		if(NULL != (*ppmusic)->pArtist)
		{
			free((*ppmusic)->pArtist);
			(*ppmusic)->pArtist = NULL;
		}
		if(NULL != (*ppmusic)->pAlbum)
		{
			free((*ppmusic)->pAlbum);
			(*ppmusic)->pAlbum = NULL;
		}
		if(NULL != (*ppmusic)->pContentURL)
		{
			free((*ppmusic)->pContentURL);
			(*ppmusic)->pContentURL = NULL;
		}
		if(NULL != (*ppmusic)->pCoverURL)
		{
			free((*ppmusic)->pCoverURL);
			(*ppmusic)->pCoverURL = NULL;
		}
		if(NULL != (*ppmusic)->pPlatform)
		{
			free((*ppmusic)->pPlatform);
			(*ppmusic)->pPlatform = NULL;
		}
		free(*ppmusic);
		*ppmusic = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(WARTI_pMusicList *ppmusiclist)
{
	int i;
	int ret = 0;
	
	if((NULL == ppmusiclist)||(NULL == *ppmusiclist))
	{
		ret = -1;
	} else {
		if(NULL != (*ppmusiclist)->pRet)
		{
			free((*ppmusiclist)->pRet);
			(*ppmusiclist)->pRet = NULL;
		}
		if(NULL != (*ppmusiclist)->pMusicList)
		{
			for(i = 0; i<((*ppmusiclist)->Num); i++)
			{
				if(NULL != (((*ppmusiclist)->pMusicList)[i]))
				{
					WIFIAudio_RealTimeInterface_pMusicFree(&(((*ppmusiclist)->pMusicList)[i]));
				}
			}
			free((*ppmusiclist)->pMusicList);
			(*ppmusiclist)->pMusicList = NULL;
		}
		free(*ppmusiclist);
		*ppmusiclist = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetPlayerCmdVolumeFree(WARTI_pGetPlayerCmdVolume *ppgetplayercmdvolume)
{
	int ret = 0;
	
	if((NULL == ppgetplayercmdvolume)||(NULL == *ppgetplayercmdvolume))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetplayercmdvolume)->pRet)
		{
			free((*ppgetplayercmdvolume)->pRet);
			(*ppgetplayercmdvolume)->pRet = NULL;
		}
		
		free(*ppgetplayercmdvolume);
		*ppgetplayercmdvolume = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetPlayerCmdMuteFree(WARTI_pGetPlayerCmdMute *ppgetplayercmdmute)
{
	int ret = 0;
	
	if((NULL == ppgetplayercmdmute)||(NULL == *ppgetplayercmdmute))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetplayercmdmute)->pRet)
		{
			free((*ppgetplayercmdmute)->pRet);
			(*ppgetplayercmdmute)->pRet = NULL;
		}
		
		free(*ppgetplayercmdmute);
		*ppgetplayercmdmute = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetEqualizerFree(WARTI_pGetEqualizer *ppgetequalizer)
{
	int ret = 0;
	
	if((NULL == ppgetequalizer)||(NULL == *ppgetequalizer))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetequalizer)->pRet)
		{
			free((*ppgetequalizer)->pRet);
			(*ppgetequalizer)->pRet = NULL;
		}
		
		free(*ppgetequalizer);
		*ppgetequalizer = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetPlayerCmdChannelFree(WARTI_pGetPlayerCmdChannel *ppgetplayercmdchannel)
{
	int ret = 0;
	
	if((NULL == ppgetplayercmdchannel)||(NULL == *ppgetplayercmdchannel))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetplayercmdchannel)->pRet)
		{
			free((*ppgetplayercmdchannel)->pRet);
			(*ppgetplayercmdchannel)->pRet = NULL;
		}
		
		free(*ppgetplayercmdchannel);
		*ppgetplayercmdchannel = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetShutdownFree(WARTI_pGetShutdown *ppgetshutdown)
{
	int ret = 0;
	
	if((NULL == ppgetshutdown)||(NULL == *ppgetshutdown))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetshutdown)->pRet)
		{
			free((*ppgetshutdown)->pRet);
			(*ppgetshutdown)->pRet = NULL;
		}
		
		free(*ppgetshutdown);
		*ppgetshutdown = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pManufacturerInfoFree(WARTI_pGetManufacturerInfo *ppmanufacturerinfo)
{
	int ret = 0;
	
	if((NULL == ppmanufacturerinfo)||(NULL == *ppmanufacturerinfo))
	{
		ret = -1;
	} else {
		if(NULL != (*ppmanufacturerinfo)->pRet)
		{
			free((*ppmanufacturerinfo)->pRet);
			(*ppmanufacturerinfo)->pRet = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pManuFacturer)
		{
			free((*ppmanufacturerinfo)->pManuFacturer);
			(*ppmanufacturerinfo)->pManuFacturer = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pManuFacturerURL)
		{
			free((*ppmanufacturerinfo)->pManuFacturerURL);
			(*ppmanufacturerinfo)->pManuFacturerURL = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pModelDescription)
		{
			free((*ppmanufacturerinfo)->pModelDescription);
			(*ppmanufacturerinfo)->pModelDescription = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pModelName)
		{
			free((*ppmanufacturerinfo)->pModelName);
			(*ppmanufacturerinfo)->pModelName = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pModelNumber)
		{
			free((*ppmanufacturerinfo)->pModelNumber);
			(*ppmanufacturerinfo)->pModelNumber = NULL;
		}
		if(NULL != (*ppmanufacturerinfo)->pModelURL)
		{
			free((*ppmanufacturerinfo)->pModelURL);
			(*ppmanufacturerinfo)->pModelURL = NULL;
		}
		
		free(*ppmanufacturerinfo);
		*ppmanufacturerinfo = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateFree(WARTI_pGetMvRemoteUpdate *ppgetmvremoteupdate)
{
	int ret = 0;
	
	if((NULL == ppgetmvremoteupdate)||(NULL == *ppgetmvremoteupdate))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetmvremoteupdate)->pRet)
		{
			free((*ppgetmvremoteupdate)->pRet);
			(*ppgetmvremoteupdate)->pRet = NULL;
		}
		
		free(*ppgetmvremoteupdate);
		*ppgetmvremoteupdate = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateStatusFree(WARTI_pGetRemoteUpdateProgress *ppgetremoteupdateprogress)
{
	int ret = 0;
	
	if((NULL == ppgetremoteupdateprogress)||(NULL == *ppgetremoteupdateprogress))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetremoteupdateprogress)->pRet)
		{
			free((*ppgetremoteupdateprogress)->pRet);
			(*ppgetremoteupdateprogress)->pRet = NULL;
		}
		
		free(*ppgetremoteupdateprogress);
		*ppgetremoteupdateprogress = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pSlaveSpeakerBoxFree(WARTI_pSlaveSpeakerBox *ppslavespeakerbox)
{
	int ret = 0;
	
	if((NULL == ppslavespeakerbox)||(NULL == *ppslavespeakerbox))
	{
		ret = -1;
	} else {
		if(NULL != (*ppslavespeakerbox)->pName)
		{
			free((*ppslavespeakerbox)->pName);
			(*ppslavespeakerbox)->pName = NULL;
		}
		if(NULL != (*ppslavespeakerbox)->pVersion)
		{
			free((*ppslavespeakerbox)->pVersion);
			(*ppslavespeakerbox)->pVersion = NULL;
		}
		free(*ppslavespeakerbox);
		*ppslavespeakerbox = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetSlaveListFree(WARTI_pGetSlaveList *ppgetslavelist)
{
	int i;
	int ret = 0;
	
	if((NULL == ppgetslavelist)||(NULL == *ppgetslavelist))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetslavelist)->pSlaveList)
		{
			for(i = 0; i<((*ppgetslavelist)->Slaves); i++)
			{
				if(NULL != (((*ppgetslavelist)->pSlaveList)[i]))
				{
					WIFIAudio_RealTimeInterface_pSlaveSpeakerBoxFree(&(((*ppgetslavelist)->pSlaveList)[i]));
				}
			}
			free((*ppgetslavelist)->pSlaveList);
			(*ppgetslavelist)->pSlaveList = NULL;
		}
		free(*ppgetslavelist);
		*ppgetslavelist = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetDeviceTimeFree(WARTI_pGetDeviceTime *ppgetdevicetime)
{
	int ret = 0;
	
	if((NULL == ppgetdevicetime)||(NULL == *ppgetdevicetime))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetdevicetime)->pRet)
		{
			free((*ppgetdevicetime)->pRet);
			(*ppgetdevicetime)->pRet = NULL;
		}
		
		free(*ppgetdevicetime);
		*ppgetdevicetime = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetAlarmClockFree(WARTI_pGetAlarmClock *ppgetalarmclock)
{
	int ret = 0;
	
	if((NULL == ppgetalarmclock)||(NULL == *ppgetalarmclock))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetalarmclock)->pRet)
		{
			free((*ppgetalarmclock)->pRet);
			(*ppgetalarmclock)->pRet = NULL;
		}
		
		if(NULL != (*ppgetalarmclock)->pMusic)
		{
			WIFIAudio_RealTimeInterface_pMusicFree(&((*ppgetalarmclock)->pMusic));
			(*ppgetalarmclock)->pMusic = NULL;
		}
		
		free(*ppgetalarmclock);
		*ppgetalarmclock = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetPlayerCmdSwitchModeFree(WARTI_pGetPlayerCmdSwitchMode *ppgetplayercmdswitchmode)
{
	int ret = 0;
	
	if((NULL == ppgetplayercmdswitchmode)||(NULL == *ppgetplayercmdswitchmode))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetplayercmdswitchmode)->pRet)
		{
			free((*ppgetplayercmdswitchmode)->pRet);
			(*ppgetplayercmdswitchmode)->pRet = NULL;
		}
		
		free(*ppgetplayercmdswitchmode);
		*ppgetplayercmdswitchmode = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetLanguageFree(WARTI_pGetLanguage *ppgetlanguage)
{
	int ret = 0;
	
	if((NULL == ppgetlanguage)||(NULL == *ppgetlanguage))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetlanguage)->pRet)
		{
			free((*ppgetlanguage)->pRet);
			(*ppgetlanguage)->pRet = NULL;
		}
		
		free(*ppgetlanguage);
		*ppgetlanguage = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pInsertPlaylistInfoFree(WARTI_pInsertPlaylistInfo *ppinsertplaylistinfo)
{
	int ret = 0;
	
	if((NULL == ppinsertplaylistinfo)||(NULL == *ppinsertplaylistinfo))
	{
		ret = -1;
	} else {
		if(NULL != (*ppinsertplaylistinfo)->pMusic)
		{
			WIFIAudio_RealTimeInterface_pMusicFree(&((*ppinsertplaylistinfo)->pMusic));
			(*ppinsertplaylistinfo)->pMusic = NULL;
		}
		
		free(*ppinsertplaylistinfo);
		*ppinsertplaylistinfo = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pStorageDeviceOnlineStateFree(WARTI_pStorageDeviceOnlineState *ppstoragedeviceonlinestate)
{
	int ret = 0;
	
	if((NULL == ppstoragedeviceonlinestate)||(NULL == *ppstoragedeviceonlinestate))
	{
		ret = -1;
	} else {
		if(NULL != (*ppstoragedeviceonlinestate)->pRet)
		{
			free((*ppstoragedeviceonlinestate)->pRet);
			(*ppstoragedeviceonlinestate)->pRet = NULL;
		}
		
		free(*ppstoragedeviceonlinestate);
		*ppstoragedeviceonlinestate = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetTheLatestVersionNumberOfFirmwareFree(WARTI_pGetTheLatestVersionNumberOfFirmware *ppgetthelatestversionnumberoffirmware)
{
	int ret = 0;
	
	if((NULL == ppgetthelatestversionnumberoffirmware)||(NULL == *ppgetthelatestversionnumberoffirmware))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetthelatestversionnumberoffirmware)->pRet)
		{
			free((*ppgetthelatestversionnumberoffirmware)->pRet);
			(*ppgetthelatestversionnumberoffirmware)->pRet = NULL;
		}
		
		if(NULL != (*ppgetthelatestversionnumberoffirmware)->pVersion)
		{
			free((*ppgetthelatestversionnumberoffirmware)->pVersion);
			(*ppgetthelatestversionnumberoffirmware)->pVersion = NULL;
		}
		
		free(*ppgetthelatestversionnumberoffirmware);
		*ppgetthelatestversionnumberoffirmware = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pPostSynchronousInfoExFree(WARTI_pPostSynchronousInfoEx *pppostsynchronousinfoex)
{
	int ret = 0;
	
	if((NULL == pppostsynchronousinfoex)||(NULL == *pppostsynchronousinfoex))
	{
		ret = -1;
	} else {
		if(NULL != (*pppostsynchronousinfoex)->pMaster_SSID)
		{
			free((*pppostsynchronousinfoex)->pMaster_SSID);
			(*pppostsynchronousinfoex)->pMaster_SSID = NULL;
		}
		
		if(NULL != (*pppostsynchronousinfoex)->pMaster_Password)
		{
			free((*pppostsynchronousinfoex)->pMaster_Password);
			(*pppostsynchronousinfoex)->pMaster_Password = NULL;
		}
		
		free(*pppostsynchronousinfoex);
		*pppostsynchronousinfoex = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pHostExFree(WARTI_pHostEx *pphostex)
{
	int ret = 0;
	
	if((NULL == pphostex)||(NULL == *pphostex))
	{
		ret = -1;
	} else {
		if(NULL != (*pphostex)->pIp)
		{
			free((*pphostex)->pIp);
			(*pphostex)->pIp = NULL;
		}
		
		if(NULL != (*pphostex)->pMac)
		{
			free((*pphostex)->pMac);
			(*pphostex)->pMac = NULL;
		}
		
		if(NULL != (*pphostex)->pName)
		{
			free((*pphostex)->pName);
			(*pphostex)->pName = NULL;
		}
		
		if(NULL != (*pphostex)->pVolume)
		{
			free((*pphostex)->pVolume);
			(*pphostex)->pVolume = NULL;
		}
		
		if(NULL != (*pphostex)->pOs)
		{
			free((*pphostex)->pOs);
			(*pphostex)->pOs = NULL;
		}
		
		if(NULL != (*pphostex)->pUuid)
		{
			free((*pphostex)->pUuid);
			(*pphostex)->pUuid = NULL;
		}
		
		free(*pphostex);
		*pphostex = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pClientExFree(WARTI_pClientEx *ppclientex)
{
	int ret = 0;
	
	if((NULL == ppclientex)||(NULL == *ppclientex))
	{
		ret = -1;
	} else {
		if(NULL != (*ppclientex)->pHost)
		{
			WIFIAudio_RealTimeInterface_pHostExFree(&((*ppclientex)->pHost));
			(*ppclientex)->pHost = NULL;
		}
		
		free(*ppclientex);
		*ppclientex = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetSynchronousInfoExFree(WARTI_pGetSynchronousInfoEx *ppgetsynchronousinfoex)
{
	int ret = 0;
	int i = 0;
	
	if((NULL == ppgetsynchronousinfoex)||(NULL == *ppgetsynchronousinfoex))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetsynchronousinfoex)->pClientList)
		{
			for(i = 0; i < (*ppgetsynchronousinfoex)->Num; i++)
			{
				WIFIAudio_RealTimeInterface_pClientExFree(&((*ppgetsynchronousinfoex)->pClientList)[i]);
				((*ppgetsynchronousinfoex)->pClientList)[i] = NULL;
			}
			
			free((*ppgetsynchronousinfoex)->pClientList);
			(*ppgetsynchronousinfoex)->pClientList = NULL;
		}
		
		free(*ppgetsynchronousinfoex);
		*ppgetsynchronousinfoex = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetRecordFileURLFree(WARTI_pGetRecordFileURL *ppgetrecordfileurl)
{
	int ret = 0;
	
	if((NULL == ppgetrecordfileurl)||(NULL == *ppgetrecordfileurl))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetrecordfileurl)->pRet)
		{
			free((*ppgetrecordfileurl)->pRet);
			(*ppgetrecordfileurl)->pRet = NULL;
		}
		
		if(NULL != (*ppgetrecordfileurl)->pURL)
		{
			free((*ppgetrecordfileurl)->pURL);
			(*ppgetrecordfileurl)->pURL = NULL;
		}
		
		free(*ppgetrecordfileurl);
		*ppgetrecordfileurl = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pBluetoothDeviceFree(WARTI_pBluetoothDevice *ppbluetoothdevice)
{
	int ret = 0;
	
	if((NULL == ppbluetoothdevice)||(NULL == *ppbluetoothdevice))
	{
		ret = -1;
	} else {
		if(NULL != (*ppbluetoothdevice)->pAddress)
		{
			free((*ppbluetoothdevice)->pAddress);
			(*ppbluetoothdevice)->pAddress = NULL;
		}
		if(NULL != (*ppbluetoothdevice)->pName)
		{
			free((*ppbluetoothdevice)->pName);
			(*ppbluetoothdevice)->pName = NULL;
		}
		free(*ppbluetoothdevice);
		*ppbluetoothdevice = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetBluetoothListFree(WARTI_pGetBluetoothList *ppgetbluetoothlist)
{
	int i;
	int ret = 0;
	if((NULL == ppgetbluetoothlist)||(NULL == *ppgetbluetoothlist))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetbluetoothlist)->pRet)
		{
			free((*ppgetbluetoothlist)->pRet);
			(*ppgetbluetoothlist)->pRet = NULL;
		}
		if(NULL != ((*ppgetbluetoothlist)->pBdList))
		{
			for(i = 0; i<((*ppgetbluetoothlist)->Num); i++)
			{
				if(NULL != (((*ppgetbluetoothlist)->pBdList)[i]))
				{
					WIFIAudio_RealTimeInterface_pBluetoothDeviceFree(&(((*ppgetbluetoothlist)->pBdList)[i]));
				}
			}
			free((*ppgetbluetoothlist)->pBdList);
			(*ppgetbluetoothlist)->pBdList = NULL;
		}
		free(*ppgetbluetoothlist);
		*ppgetbluetoothlist = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetSpeechRecognitionFree(WARTI_pGetSpeechRecognition *ppgetspeechrecognition)
{
	int ret = 0;
	if((NULL == ppgetspeechrecognition)||(NULL == *ppgetspeechrecognition))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetspeechrecognition)->pRet)
		{
			free((*ppgetspeechrecognition)->pRet);
			(*ppgetspeechrecognition)->pRet = NULL;
		}
		
		free(*ppgetspeechrecognition);
		*ppgetspeechrecognition = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pLedMatrixDataFree(WARTI_pLedMatrixData *ppledmatrixdata)
{
	int ret = 0;
	if((NULL == ppledmatrixdata)||(NULL == *ppledmatrixdata))
	{
		ret = -1;
	} else {
		if(NULL != (*ppledmatrixdata)->pData)
		{
			free((*ppledmatrixdata)->pData);
			(*ppledmatrixdata)->pData = NULL;
		}
		
		free(*ppledmatrixdata);
		*ppledmatrixdata = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pMultiLedMatrixFree(WARTI_pMultiLedMatrix *ppmultiledmatrix)
{
	int i;
	int ret = 0;
	if((NULL == ppmultiledmatrix)||(NULL == *ppmultiledmatrix))
	{
		ret = -1;
	} else {
		if(NULL != (*ppmultiledmatrix)->pScreenlist)
		{
			for(i = 0; i < (*ppmultiledmatrix)->Num; i++)
			{
				if(NULL != (*ppmultiledmatrix)->pScreenlist[i])
				{
					WIFIAudio_RealTimeInterface_pLedMatrixDataFree(&((*ppmultiledmatrix)->pScreenlist[i]));
					(*ppmultiledmatrix)->pScreenlist[i] = NULL;
				}
			}
			
			free((*ppmultiledmatrix)->pScreenlist);
			(*ppmultiledmatrix)->pScreenlist = NULL;
		}
		
		free(*ppmultiledmatrix);
		*ppmultiledmatrix = NULL;
	}
		
	return ret;
}


int WIFIAudio_RealTimeInterface_pPostLedMatrixScreenFree(WARTI_pPostLedMatrixScreen *pppostledmatrixscreen)
{
	int ret = 0;
	if((NULL == pppostledmatrixscreen)||(NULL == *pppostledmatrixscreen))
	{
		ret = -1;
	} else {
		switch((*pppostledmatrixscreen)->Animation)
		{
			case 0:
				if((*pppostledmatrixscreen)->Content.pLedMatrixData != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pLedMatrixDataFree(&((*pppostledmatrixscreen)->Content.pLedMatrixData));
					(*pppostledmatrixscreen)->Content.pLedMatrixData = NULL;
				}
				break;
				
			case 1:
				if((*pppostledmatrixscreen)->Content.pMultiLedMatrix != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pMultiLedMatrixFree(&((*pppostledmatrixscreen)->Content.pMultiLedMatrix));
					(*pppostledmatrixscreen)->Content.pMultiLedMatrix = NULL;
				}
				break;
				
			default:
				ret = -1;
				break;
		}
		
		free(*pppostledmatrixscreen);
		*pppostledmatrixscreen = NULL;
	}
		
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetAlexaLanguageFree(WARTI_pGetAlexaLanguage *ppgetalexalanguage)
{
	int ret = 0;
	
	if((NULL == ppgetalexalanguage)||(NULL == *ppgetalexalanguage))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetalexalanguage)->pRet)
		{
			free((*ppgetalexalanguage)->pRet);
			(*ppgetalexalanguage)->pRet = NULL;
		}
		
		if(NULL != (*ppgetalexalanguage)->pLanguage)
		{
			free((*ppgetalexalanguage)->pLanguage);
			(*ppgetalexalanguage)->pLanguage = NULL;
		}
		
		free(*ppgetalexalanguage);
		*ppgetalexalanguage = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetChannelCompareFree(WARTI_pGetChannelCompare *ppgetchannelcompare)
{
	int ret = 0;
	
	if((NULL == ppgetchannelcompare)||(NULL == *ppgetchannelcompare))
	{
		ret = -1;
	} else {
		if(NULL != (*ppgetchannelcompare)->pRet)
		{
			free((*ppgetchannelcompare)->pRet);
			(*ppgetchannelcompare)->pRet = NULL;
		}
		
		free(*ppgetchannelcompare);
		*ppgetchannelcompare = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pRemoteBluetoothUpdateFree(WARTI_pRemoteBluetoothUpdate *ppremotebluetoothupdate)
{
	int ret = 0;
	
	if((NULL == ppremotebluetoothupdate)||(NULL == *ppremotebluetoothupdate))
	{
		ret = -1;
	} else {
		if(NULL != (*ppremotebluetoothupdate)->pURL)
		{
			free((*ppremotebluetoothupdate)->pURL);
			(*ppremotebluetoothupdate)->pURL = NULL;
		}
		
		free(*ppremotebluetoothupdate);
		*ppremotebluetoothupdate = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pAlarmClockFree(WARTI_pAlarmClock *ppalarmclock)
{
	int ret = 0;
	
	if((NULL == ppalarmclock)||(NULL == *ppalarmclock))
	{
		ret = -1;
	} else {
		if(NULL != (*ppalarmclock)->pContent)
		{
			WIFIAudio_RealTimeInterface_pMusicFree(&(*ppalarmclock)->pContent);
			(*ppalarmclock)->pContent = NULL;
		}
		
		free(*ppalarmclock);
		*ppalarmclock = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pPlanPlayAlbumFree(WARTI_pPlanPlayAlbum *ppPlanPlayAlbum)
{
	int ret = 0;
	
	if((NULL == ppPlanPlayAlbum)||(NULL == *ppPlanPlayAlbum))
	{
		ret = -1;
	} else {
		if(NULL != (*ppPlanPlayAlbum)->pContent)
		{
			WIFIAudio_RealTimeInterface_pMusicFree(&(*ppPlanPlayAlbum)->pContent);
			(*ppPlanPlayAlbum)->pContent = NULL;
		}
		
		free(*ppPlanPlayAlbum);
		*ppPlanPlayAlbum = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pPlanPlayMusicListFree(WARTI_pPlanPlayMusicList *ppPlanPlayMusicList)
{
	int ret = 0;
	
	if((NULL == ppPlanPlayMusicList)||(NULL == *ppPlanPlayMusicList))
	{
		ret = -1;
	} else {
		if(NULL != (*ppPlanPlayMusicList)->pContent)
		{
			WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(&(*ppPlanPlayMusicList)->pContent);
			(*ppPlanPlayMusicList)->pContent = NULL;
		}
		
		free(*ppPlanPlayMusicList);
		*ppPlanPlayMusicList = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMixedElementFree(WARTI_pMixedElement *ppMixedElement)
{
	int ret = 0;
	
	if((NULL == ppMixedElement)||(NULL == *ppMixedElement))
	{
		ret = -1;
	} else {
		WIFIAudio_RealTimeInterface_pMusicFree(&((*ppMixedElement)->pContent));
		
		free(*ppMixedElement);
		*ppMixedElement = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMixedContentFree(WARTI_pMixedContent *ppMixedContent)
{
	int ret = 0;
	int i = 0;
	
	if((NULL == ppMixedContent)||(NULL == *ppMixedContent))
	{
		ret = -1;
	} else {
		if(NULL != ((*ppMixedContent)->pMixedList))
		{
			for(i = 0; i<((*ppMixedContent)->Num); i++)
			{
				if(NULL != (((*ppMixedContent)->pMixedList)[i]))
				{
					WIFIAudio_RealTimeInterface_pMixedElementFree(&(((*ppMixedContent)->pMixedList)[i]));
				}
			}
			free((*ppMixedContent)->pMixedList);
			(*ppMixedContent)->pMixedList = NULL;
		}
		
		free(*ppMixedContent);
		*ppMixedContent = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pPlanMixedFree(WARTI_pPlanMixed *ppPlanMixed)
{
	int ret = 0;
	
	if((NULL == ppPlanMixed)||(NULL == *ppPlanMixed))
	{
		ret = -1;
	} else {
		if(NULL != (*ppPlanMixed)->pContent)
		{
			WIFIAudio_RealTimeInterface_pMixedContentFree(&(*ppPlanMixed)->pContent);
			(*ppPlanMixed)->pContent = NULL;
		}
		
		free(*ppPlanMixed);
		*ppPlanMixed = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetUpgradeStateFree(WARTI_pGetUpgradeState *ppGetUpgradeState)
{
	int ret = 0;
	
	if((NULL == ppGetUpgradeState)||(NULL == *ppGetUpgradeState))
	{
		ret = -1;
	} else {
		if(NULL != (*ppGetUpgradeState)->pRet)
		{
			free((*ppGetUpgradeState)->pRet);
			(*ppGetUpgradeState)->pRet = NULL;
		}
		
		free(*ppGetUpgradeState);
		*ppGetUpgradeState = NULL;
	}
	return ret;
}

int WIFIAudio_RealTimeInterface_pShortcutKeyListFree(WARTI_pShortcutKeyList *ppShortcutKeyList)
{
	int ret = 0;
	
	if((NULL == ppShortcutKeyList)||(NULL == *ppShortcutKeyList))
	{
		ret = -1;
	} else {
		if(NULL != (*ppShortcutKeyList)->pContent)
		{
			WIFIAudio_RealTimeInterface_pMixedContentFree(&(*ppShortcutKeyList)->pContent);
			(*ppShortcutKeyList)->pContent = NULL;
		}
		
		free(*ppShortcutKeyList);
		*ppShortcutKeyList = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pGetMicarrayInfoFree(WARTI_pGetMicarrayInfo *ppGetMicarrayInfo)
{
	int ret = 0;
	
	if ((ppGetMicarrayInfo == NULL) || (*ppGetMicarrayInfo == NULL))
	{
		ret = -1;
	} else {
		if ((*ppGetMicarrayInfo)->pRet != NULL)
		{
			free((*ppGetMicarrayInfo)->pRet);
			(*ppGetMicarrayInfo)->pRet = NULL;
		}
		
		free(*ppGetMicarrayInfo);
		*ppGetMicarrayInfo = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMicarrayNormalFree(WARTI_pMicarrayNormal *ppMicarrayNormal)
{
	int ret = 0;
	
	if ((ppMicarrayNormal == NULL) || (*ppMicarrayNormal == NULL))
	{
		ret = -1;
	} else {
		if ((*ppMicarrayNormal)->pRet != NULL)
		{
			free((*ppMicarrayNormal)->pRet);
			(*ppMicarrayNormal)->pRet = NULL;
		}
		
		if ((*ppMicarrayNormal)->pURL != NULL)
		{
			free((*ppMicarrayNormal)->pURL);
			(*ppMicarrayNormal)->pURL = NULL;
		}
		
		free(*ppMicarrayNormal);
		*ppMicarrayNormal = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMicarrayUrlListFree(WARTI_pMicarrayUrlList *ppMicarrayUrlList)
{
	int ret = 0;
	
	if ((ppMicarrayUrlList == NULL) || (*ppMicarrayUrlList == NULL))
	{
		ret = -1;
	} else {
		if ((*ppMicarrayUrlList)->pURL != NULL)
		{
			free((*ppMicarrayUrlList)->pURL);
			(*ppMicarrayUrlList)->pURL = NULL;
		}
		
		free(*ppMicarrayUrlList);
		*ppMicarrayUrlList = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMicarrayMicFree(WARTI_pMicarrayMic *ppMicarrayMic)
{
	int ret = 0;
	
	if ((ppMicarrayMic == NULL) || (*ppMicarrayMic == NULL))
	{
		ret = -1;
	} else {
		if ((*ppMicarrayMic)->pRet != NULL)
		{
			free((*ppMicarrayMic)->pRet);
			(*ppMicarrayMic)->pRet = NULL;
		}
		
		if ((*ppMicarrayMic)->ppUrlList != NULL)
		{
			WIFIAudio_RealTimeInterface_pMicarrayUrlListFree((*ppMicarrayMic)->ppUrlList);
			(*ppMicarrayMic)->ppUrlList = NULL;
		}
		
		free(*ppMicarrayMic);
		*ppMicarrayMic = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMicarrayRefFree(WARTI_pMicarrayRef *ppMicarrayRef)
{
	int ret = 0;
	
	if ((ppMicarrayRef == NULL) || (*ppMicarrayRef == NULL))
	{
		ret = -1;
	} else {
		if ((*ppMicarrayRef)->pRet != NULL)
		{
			free((*ppMicarrayRef)->pRet);
			(*ppMicarrayRef)->pRet = NULL;
		}
		
		if ((*ppMicarrayRef)->ppUrlList != NULL)
		{
			WIFIAudio_RealTimeInterface_pMicarrayUrlListFree((*ppMicarrayRef)->ppUrlList);
			(*ppMicarrayRef)->ppUrlList = NULL;
		}
		
		free(*ppMicarrayRef);
		*ppMicarrayRef = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pStrFree(WARTI_pStr *pStr)
{
	int ret = 0;
	
	if((pStr == NULL)||(*pStr == NULL))
	{
		ret = -1;
	} else {
		switch ((*pStr)->type)
		{
			case WARTI_STR_TYPE_GETSTATUS:
				if((*pStr)->Str.pGetStatus != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetStatusFree(&((*pStr)->Str.pGetStatus));
					(*pStr)->Str.pGetStatus = NULL;
				}
				break;

			case WARTI_STR_TYPE_WLANGETAPLIST:
				if((*pStr)->Str.pWLANGetAPlist != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pWLANGetAPlistFree(&((*pStr)->Str.pWLANGetAPlist));
					(*pStr)->Str.pWLANGetAPlist = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT:
				if((*pStr)->Str.pGetCurrentWirelessConnect != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetCurrentWirelessConnectFree(&((*pStr)->Str.pGetCurrentWirelessConnect));
					(*pStr)->Str.pGetCurrentWirelessConnect = NULL;
				}
				break;

			case WARTI_STR_TYPE_GETDEVICEINFO:
				if((*pStr)->Str.pGetDeviceInfo != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetDeviceInfoFree(&((*pStr)->Str.pGetDeviceInfo));
					(*pStr)->Str.pGetDeviceInfo = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETONETOUCHCONFIGURE:
				if((*pStr)->Str.pGetOneTouchConfigure != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetOneTouchConfigureFree(&((*pStr)->Str.pGetOneTouchConfigure));
					(*pStr)->Str.pGetOneTouchConfigure = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME:
				if((*pStr)->Str.pGetOneTouchConfigureSetDeviceName != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetOneTouchConfigureSetDeviceNameFree(&((*pStr)->Str.pGetOneTouchConfigureSetDeviceName));
					(*pStr)->Str.pGetOneTouchConfigureSetDeviceName = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETLOGINAMAZONSTATUS:
				if((*pStr)->Str.pGetLoginAmazonStatus != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetLoginAmazonStatusFree(&((*pStr)->Str.pGetLoginAmazonStatus));
					(*pStr)->Str.pGetLoginAmazonStatus = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETLOGINAMAZONNEEDINFO:
				if((*pStr)->Str.pGetLoginAmazonNeedInfo != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetLoginAmazonNeedInfoFree(&((*pStr)->Str.pGetLoginAmazonNeedInfo));
					(*pStr)->Str.pGetLoginAmazonNeedInfo = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_POSTLOGINAMAZONINFO:
				if((*pStr)->Str.pPostLoginAmazonInfo != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPostLoginAmazonInfoFree(&((*pStr)->Str.pPostLoginAmazonInfo));
					(*pStr)->Str.pPostLoginAmazonInfo = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETPLAYERSTATUS:
				if((*pStr)->Str.pGetPlayerStatus != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetPlayerStatusFree(&((*pStr)->Str.pGetPlayerStatus));
					(*pStr)->Str.pGetPlayerStatus = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETCURRENTPLAYLIST:
				if((*pStr)->Str.pGetCurrentPlaylist != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(&((*pStr)->Str.pGetCurrentPlaylist));
					(*pStr)->Str.pGetCurrentPlaylist = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETPLAYERCMDVOLUME:
				if((*pStr)->Str.pGetPlayerCmdVolume != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetPlayerCmdVolumeFree(&((*pStr)->Str.pGetPlayerCmdVolume));
					(*pStr)->Str.pGetPlayerCmdVolume = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETPLAYERCMDMUTE:
				if((*pStr)->Str.pGetPlayerCmdMute != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetPlayerCmdMuteFree(&((*pStr)->Str.pGetPlayerCmdMute));
					(*pStr)->Str.pGetPlayerCmdMute = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETEQUALIZER:
				if((*pStr)->Str.pGetEqualizer != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetEqualizerFree(&((*pStr)->Str.pGetEqualizer));
					(*pStr)->Str.pGetEqualizer = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETPLAYERCMDCHANNEL:
				if((*pStr)->Str.pGetPlayerCmdChannel != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetPlayerCmdChannelFree(&((*pStr)->Str.pGetPlayerCmdChannel));
					(*pStr)->Str.pGetPlayerCmdChannel = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETSHUTDOWN:
				if((*pStr)->Str.pGetShutdown != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetShutdownFree(&((*pStr)->Str.pGetShutdown));
					(*pStr)->Str.pGetShutdown = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETMANUFACTURERINFO:
				if((*pStr)->Str.pGetManufacturerInfo != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pManufacturerInfoFree(&((*pStr)->Str.pGetManufacturerInfo));
					(*pStr)->Str.pGetManufacturerInfo = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETMVREMOTEUPDATE:
				if((*pStr)->Str.pGetMvRemoteUpdate != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateFree(&((*pStr)->Str.pGetMvRemoteUpdate));
					(*pStr)->Str.pGetMvRemoteUpdate = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETDOWNLOADPROGRESS:
			case WARTI_STR_TYPE_GETBLUETOOTHDOWNLOADPROGRESS:
				if((*pStr)->Str.pGetRemoteUpdateProgress != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateStatusFree(&((*pStr)->Str.pGetRemoteUpdateProgress));
					(*pStr)->Str.pGetRemoteUpdateProgress = NULL;
				}
				break;

			case WARTI_STR_TYPE_GETSLAVELIST:
				if((*pStr)->Str.pGetSlaveList != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetSlaveListFree(&((*pStr)->Str.pGetSlaveList));
					(*pStr)->Str.pGetSlaveList = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETDEVICETIME:
				if((*pStr)->Str.pGetDeviceTime != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetDeviceTimeFree(&((*pStr)->Str.pGetDeviceTime));
					(*pStr)->Str.pGetDeviceTime = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETALARMCLOCK:
				if((*pStr)->Str.pGetAlarmClock != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetAlarmClockFree(&((*pStr)->Str.pGetAlarmClock));
					(*pStr)->Str.pGetAlarmClock = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE:
				if((*pStr)->Str.pGetPlayerCmdSwitchMode != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetPlayerCmdSwitchModeFree(&((*pStr)->Str.pGetPlayerCmdSwitchMode));
					(*pStr)->Str.pGetPlayerCmdSwitchMode = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETLANGUAGE:
				if((*pStr)->Str.pGetLanguage != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetLanguageFree(&((*pStr)->Str.pGetLanguage));
					(*pStr)->Str.pGetLanguage = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_INSERTPLAYLISTINFO:
				if((*pStr)->Str.pInsertPlaylistInfo != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pInsertPlaylistInfoFree(&((*pStr)->Str.pInsertPlaylistInfo));
					(*pStr)->Str.pInsertPlaylistInfo = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_STORAGEDEVICEONLINESTATE:
				if((*pStr)->Str.pStorageDeviceOnlineState != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pStorageDeviceOnlineStateFree(&((*pStr)->Str.pStorageDeviceOnlineState));
					(*pStr)->Str.pStorageDeviceOnlineState = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETTHELATESTVERSIONNUMBEROFFIRMWARE:
				if((*pStr)->Str.pGetTheLatestVersionNumberOfFirmware != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetTheLatestVersionNumberOfFirmwareFree(&((*pStr)->Str.pGetTheLatestVersionNumberOfFirmware));
					(*pStr)->Str.pGetTheLatestVersionNumberOfFirmware = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX:
				if((*pStr)->Str.pPostSynchronousInfoEx != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPostSynchronousInfoExFree(&((*pStr)->Str.pPostSynchronousInfoEx));
					(*pStr)->Str.pPostSynchronousInfoEx = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETSYNCHRONOUSINFOEX:
				if((*pStr)->Str.pGetSynchronousInfoEx != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetSynchronousInfoExFree(&((*pStr)->Str.pGetSynchronousInfoEx));
					(*pStr)->Str.pGetSynchronousInfoEx = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETRECORDFILEURL:
				if((*pStr)->Str.pGetRecordFileURL != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetRecordFileURLFree(&((*pStr)->Str.pGetRecordFileURL));
					(*pStr)->Str.pGetRecordFileURL = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETBLUETOOTHLIST:
				if((*pStr)->Str.pGetBluetoothList != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetBluetoothListFree(&((*pStr)->Str.pGetBluetoothList));
					(*pStr)->Str.pGetBluetoothList = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETSPEECHRECOGNITION:
				if((*pStr)->Str.pGetSpeechRecognition != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetSpeechRecognitionFree(&((*pStr)->Str.pGetSpeechRecognition));
					(*pStr)->Str.pGetSpeechRecognition = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_POSTLEDMATRIXSCREEN:
				if((*pStr)->Str.pPostLedMatrixScreen != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPostLedMatrixScreenFree(&((*pStr)->Str.pPostLedMatrixScreen));
					(*pStr)->Str.pPostLedMatrixScreen = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETALEXALANGUAGE:
				if((*pStr)->Str.pGetAlexaLanguage != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetAlexaLanguageFree(&((*pStr)->Str.pGetAlexaLanguage));
					(*pStr)->Str.pGetAlexaLanguage = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_GETCHANNELCOMPARE:
				if((*pStr)->Str.pGetChannelCompare != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetChannelCompareFree(&((*pStr)->Str.pGetChannelCompare));
					(*pStr)->Str.pGetChannelCompare = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_REMOTEBLUETOOTHUPDATE:
				if((*pStr)->Str.pRemoteBluetoothUpdate != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pRemoteBluetoothUpdateFree(&((*pStr)->Str.pRemoteBluetoothUpdate));
					(*pStr)->Str.pRemoteBluetoothUpdate = NULL;
				}
				break;
			case WARTI_STR_TYPE_CREATEALARMCLOCK:
			case WARTI_STR_TYPE_CHANGEALARMCLOCKPLAY:
			case WARTI_STR_TYPE_COUNTDOWNALARMCLOCK:
				if((*pStr)->Str.pAlarmClock != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pAlarmClockFree(&((*pStr)->Str.pAlarmClock));
					(*pStr)->Str.pAlarmClock = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_PLAYALBUMASPLAN:
				if((*pStr)->Str.pPlanPlayAlbum != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPlanPlayAlbumFree(&((*pStr)->Str.pPlanPlayAlbum));
					(*pStr)->Str.pPlanPlayAlbum = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_PLAYMUSICLISTASPLAN:
				if((*pStr)->Str.pPlanPlayMusicList != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPlanPlayMusicListFree(&((*pStr)->Str.pPlanPlayMusicList));
					(*pStr)->Str.pPlanPlayMusicList = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_PLAYMIXEDLISTASPLAN:
				if((*pStr)->Str.pPlanMixed != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pPlanMixedFree(&((*pStr)->Str.pPlanMixed));
					(*pStr)->Str.pPlanMixed = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE:
				if((*pStr)->Str.pGetUpgradeState != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pGetUpgradeStateFree(&((*pStr)->Str.pGetUpgradeState));
					(*pStr)->Str.pGetUpgradeState = NULL;
				}
				break;
				
			case WARTI_STR_TYPE_SHORTCUTKEYLIST:
				if((*pStr)->Str.pShortcutKeyList != NULL)
				{
					ret = WIFIAudio_RealTimeInterface_pShortcutKeyListFree(&((*pStr)->Str.pShortcutKeyList));
					(*pStr)->Str.pShortcutKeyList = NULL;
				}
				break;
	
			case WARTI_STR_TYPE_GETMICARRAYINFO:
				break;
				
			case WARTI_STR_TYPE_MICARRAYNORMAL:
				break;
				
			case WARTI_STR_TYPE_MICARRAYMIC:
				break;
				
			case WARTI_STR_TYPE_MICARRAYREF:
				break;

			default:
				ret = -1;
				break;
		}
		
		free(*pStr);
		*pStr = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_uuid_parse(const char *in, unsigned char * uuid)
{
	int ret = 0;
	int i,j;
	char strsub[3];
	
	if((NULL == uuid) || (NULL == in))
	{
		ret = -1;
	} else {
		for(i = 0, j = 0;j < 16; i = i + 2, j++)
		{
			if((8 == i)||(13 == i)||(18 == i)||(23 == i))
			{
				i++;
			}
			memset(strsub, 0, 3);
			memcpy(strsub, &(in[i]), 2);
			uuid[j] = (unsigned char)(strtol(strsub, NULL, 16));
		}
	}
	
	return ret;
}

WARTI_pGetStatus WIFIAudio_RealTimeInterface_pStringTopGetStatus(char * p)
{
	WARTI_pGetStatus ret;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetStatus)calloc(1,sizeof(WARTI_GetStatus));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "language", &(ret->pLanguage));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ssid", &(ret->pSSID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "firmware", &(ret->pFirmware));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "builddate", &(ret->pBuildDate));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Release", &(ret->pRelease));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "group", &(ret->pGroup));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "essid", &(ret->pESSID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "hardware", &(ret->pHardware));
		
		getobject = json_object_object_get(object,"uuid");
		if(getobject==NULL)
		{
			
		} else {
			WIFIAudio_RealTimeInterface_uuid_parse(json_object_get_string(getobject),(unsigned char *)ret->UUID);
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "expired", &(ret->Expired));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "internet", &(ret->Internet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "netstat", &(ret->Netstat));

		getobject = json_object_object_get(object,"apcli0");
		if(getobject==NULL)
		{
		} else {
			inet_aton(json_object_get_string(getobject),&(ret->Apcli0));
		}
		
		getobject = json_object_object_get(object,"eth2");
		if(getobject==NULL)
		{
		} else {
			inet_aton(json_object_get_string(getobject),&(ret->Eth2));
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pAccessPoint WIFIAudio_RealTimeInterface_pJsonObjectTopAccessPoint(struct json_object *p)
{
	WARTI_pAccessPoint ret;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pAccessPoint)calloc(1,sizeof(WARTI_AccessPoint));
	if(ret==NULL)
	{
		return NULL;
	}
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "ssid", &(ret->pSSID));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "auth", &(ret->pAuth));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "encry", &(ret->pEncry));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "rssi", &(ret->RSSI));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "channel", &(ret->Channel));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "extch", &(ret->Extch));
	
	getobject = json_object_object_get(p,"bssid");
	if(getobject==NULL)
	{
	} else {
		memcpy(&(ret->BSSID),ether_aton(json_object_get_string(getobject)),\
		sizeof(struct ether_addr));
	}
	
	return ret;
}

WARTI_pWLANGetAPlist WIFIAudio_RealTimeInterface_pStringTopWLANGetAPlist(char *p)
{
	WARTI_pWLANGetAPlist ret;
	int i;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pWLANGetAPlist)calloc(1,sizeof(WARTI_WLANGetAPlist));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "res", &(ret->Res));
		ret->pAplist = (WARTI_pAccessPoint*)calloc(1,(ret->Res)*sizeof(WARTI_pAccessPoint));
		
		getobject = json_object_object_get(object,"aplist");
		if(getobject==NULL)
		{
		} else {
			for(i = 0;i<(ret->Res);i++)
			{
				(ret->pAplist)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopAccessPoint(json_object_array_get_idx(getobject,i));
			}
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetCurrentWirelessConnect WIFIAudio_RealTimeInterface_pStringTopGetCurrentWirelessConnect(char *p)
{
	WARTI_pGetCurrentWirelessConnect ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetCurrentWirelessConnect)calloc(1,sizeof(WARTI_GetCurrentWirelessConnect));
	if(NULL == ret)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ssid", &(ret->pSSID));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "connect", &(ret->Connect));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetDeviceInfo WIFIAudio_RealTimeInterface_pStringTopGetDeviceInfo(char * p)
{
	WARTI_pGetDeviceInfo ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetDeviceInfo)calloc(1,sizeof(WARTI_GetDeviceInfo));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "language", &(ret->pLanguage));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ssid", &(ret->pSSID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "firmware", &(ret->pFirmware));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Release", &(ret->pRelease));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "essid", &(ret->pESSID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "project", &(ret->pProject));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "DeviceName", &(ret->pDeviceName));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "language_support", &(ret->pLanguageSupport));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "function_support", &(ret->pFunctionSupport));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "uuid", &(ret->pUUID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "MAC", &(ret->pMAC));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "bt_ver", &(ret->pBtVersion));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "conexant_ver", &(ret->pConexantVersion));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "power_supply", &(ret->pPowerSupply));

		getobject = json_object_object_get(object,"apcli0");
		if(getobject==NULL)
		{
		} else {
			inet_aton(json_object_get_string(getobject),&(ret->Apcli0));
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "rssi", &(ret->RSSI));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "battery_percent", &(ret->BatteryPercent));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "key_num", &(ret->KeyNum));
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetOneTouchConfigure WIFIAudio_RealTimeInterface_pStringTopGetOneTouchConfigure(char *p)
{
	WARTI_pGetOneTouchConfigure ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetOneTouchConfigure)calloc(1,sizeof(WARTI_GetOneTouchConfigure));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "state", &(ret->State));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetOneTouchConfigureSetDeviceName WIFIAudio_RealTimeInterface_pStringTopGetOneTouchConfigureSetDeviceName(char *p)
{
	WARTI_pGetOneTouchConfigureSetDeviceName ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetOneTouchConfigureSetDeviceName)calloc(1,sizeof(WARTI_GetOneTouchConfigureSetDeviceName));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "state", &(ret->State));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetLoginAmazonStatus WIFIAudio_RealTimeInterface_pStringTopGetLoginAmazonStatus(char *p)
{
	WARTI_pGetLoginAmazonStatus ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetLoginAmazonStatus)calloc(1,sizeof(WARTI_GetLoginAmazonStatus));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "state", &(ret->State));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetLoginAmazonNeedInfo WIFIAudio_RealTimeInterface_pStringTopGetLoginAmazonNeedInfo(char *p)
{
	WARTI_pGetLoginAmazonNeedInfo ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetLoginAmazonNeedInfo)calloc(1,sizeof(WARTI_GetLoginAmazonNeedInfo));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ProductId", &(ret->pProductId));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Dsn", &(ret->pDsn));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "CodeChallenge", &(ret->pCodeChallenge));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "CodeChallengeMethod", &(ret->pCodeChallengeMethod));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pPostLoginAmazonInfo WIFIAudio_RealTimeInterface_pStringTopPostLoginAmazonInfo(char *p)
{
	WARTI_pPostLoginAmazonInfo ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pPostLoginAmazonInfo)calloc(1,sizeof(WARTI_PostLoginAmazonInfo));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "authorizationCode", &(ret->pAuthorizationCode));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "redirectUri", &(ret->pRedirectUri));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "clientId", &(ret->pClientId));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetPlayerStatus WIFIAudio_RealTimeInterface_pStringTopGetPlayerStatus(char *p)
{
	WARTI_pGetPlayerStatus ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetPlayerStatus)calloc(1,sizeof(WARTI_GetPlayerStatus));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "status", &(ret->pStatus));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Title", &(ret->pTitle));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Artist", &(ret->pArtist));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Album", &(ret->pAlbum));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Genre", &(ret->pGenre));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "locallistfile", &(ret->pLocalListFile));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "iuri", &(ret->pIURI));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "uri", &(ret->pURI));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "cover", &(ret->pCover));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "uuid", &(ret->pUUID));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "mainmode", &(ret->MainMode));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "nodetype", &(ret->NodeType));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "mode", &(ret->Mode));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "sw", &(ret->SW));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "curpos", &(ret->CurPos));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "totlen", &(ret->TotLen));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "Year", &(ret->Year));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "Track", &(ret->Track));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "locallistflag", &(ret->LocalListFlag));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "plicount", &(ret->PLiCount));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "plicurr", &(ret->PLicurr));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "vol", &(ret->Vol));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "mute", &(ret->Mute));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "sourcemode", &(ret->SourceMode));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pMusic WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(struct json_object *p)
{
	WARTI_pMusic ret;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "title", &(ret->pTitle));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "artist", &(ret->pArtist));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "album", &(ret->pAlbum));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "content_url", &(ret->pContentURL));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "cover_url", &(ret->pCoverURL));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "platform", &(ret->pPlatform));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "column", &(ret->Column));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "program", &(ret->Program));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "index", &(ret->Index));
	
	return ret;
}

WARTI_pMusicList WIFIAudio_RealTimeInterface_pJsonObjectTopMusicList(struct json_object *p)
{
	WARTI_pMusicList ret;
	struct json_object *getobject;
	int i = 0;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pMusicList)calloc(1,sizeof(WARTI_MusicList));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "ret", &(ret->pRet));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "num", &(ret->Num));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "index", &(ret->Index));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "nowplay", &(ret->NowPlay));
	
	ret->pMusicList = (WARTI_pMusic*)calloc(1,(ret->Num)*sizeof(WARTI_pMusic));
	if(NULL == ret->pMusicList)
	{
		WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(&ret);
		ret = NULL;
	}
	
	getobject = json_object_object_get(p,"musiclist");
	for(i = 0;i<(ret->Num);i++)
	{
		(ret->pMusicList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(json_object_array_get_idx(getobject,i));
	}
	
	return ret;
}

WARTI_pMusicList WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist(char *p)
{
	WARTI_pMusicList ret = NULL;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = WIFIAudio_RealTimeInterface_pJsonObjectTopMusicList(object);
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetPlayerCmdVolume WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdVolume(char *p)
{
	WARTI_pGetPlayerCmdVolume ret = NULL;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetPlayerCmdVolume)calloc(1,sizeof(WARTI_GetPlayerCmdVolume));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "volume", &(ret->Volume));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetPlayerCmdMute WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdMute(char *p)
{
	WARTI_pGetPlayerCmdMute ret = NULL;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetPlayerCmdMute)calloc(1,sizeof(WARTI_GetPlayerCmdMute));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "mute", &(ret->Mute));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetEqualizer WIFIAudio_RealTimeInterface_pStringTopGetEqualizer(char *p)
{
	WARTI_pGetEqualizer ret = NULL;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetEqualizer)calloc(1,sizeof(WARTI_GetEqualizer));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "mode", &(ret->Mode));
		json_object_put(object);

	}
	
	return ret;
}

WARTI_pGetPlayerCmdChannel WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdChannel(char *p)
{
	WARTI_pGetPlayerCmdChannel ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetPlayerCmdChannel)calloc(1,sizeof(WARTI_GetPlayerCmdChannel));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "channel", &(ret->Channel));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetShutdown WIFIAudio_RealTimeInterface_pStringTopGetShutdown(char *p)
{
	WARTI_pGetShutdown ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetShutdown)calloc(1,sizeof(WARTI_GetShutdown));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "sec", &(ret->Second));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetManufacturerInfo WIFIAudio_RealTimeInterface_pStringTopManufacturerInfo(char *p)
{
	WARTI_pGetManufacturerInfo ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetManufacturerInfo)calloc(1,sizeof(WARTI_GetManufacturerInfo));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Manufacturer", &(ret->pManuFacturer));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ManufacturerURL", &(ret->pManuFacturerURL));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Modeldescription", &(ret->pModelDescription));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Modelname", &(ret->pModelName));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "Modelnumber", &(ret->pModelNumber));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ModelURL", &(ret->pModelURL));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetMvRemoteUpdate WIFIAudio_RealTimeInterface_pStringTopGetMvRemoteUpdate(char *p)
{
	WARTI_pGetMvRemoteUpdate ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetMvRemoteUpdate)calloc(1,sizeof(WARTI_GetMvRemoteUpdate));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "found", &(ret->Found));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetRemoteUpdateProgress WIFIAudio_RealTimeInterface_pStringTopGetRemoteUpdateProgress(char *p)
{
	WARTI_pGetRemoteUpdateProgress ret;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetRemoteUpdateProgress)calloc(1,sizeof(WARTI_GetRemoteUpdateProgress));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "state", &(ret->State));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "download", &(ret->Download));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "upgrade", &(ret->Upgrade));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pSlaveSpeakerBox WIFIAudio_RealTimeInterface_pJsonObjectTopSlaveSpeakerBox(struct json_object *p)
{
	WARTI_pSlaveSpeakerBox ret;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pSlaveSpeakerBox)calloc(1,sizeof(WARTI_SlaveSpeakerBox));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "name", &(ret->pName));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "version", &(ret->pVersion));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "mask", &(ret->Mask));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "volume", &(ret->Volume));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "mute", &(ret->Mute));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "channel", &(ret->Channel));
	
	getobject = json_object_object_get(p,"ip");
	if(getobject==NULL)
	{
	} else {
		inet_aton(json_object_get_string(getobject),&(ret->IP));
	}
	
	return ret;
}

WARTI_pGetSlaveList WIFIAudio_RealTimeInterface_pStringTopGetSlaveList(char *p)
{
	WARTI_pGetSlaveList ret;
	int i;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetSlaveList)calloc(1,sizeof(WARTI_GetSlaveList));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "slaves", &(ret->Slaves));
		ret->pSlaveList = (WARTI_pSlaveSpeakerBox*)calloc(1,(ret->Slaves)*sizeof(WARTI_pSlaveSpeakerBox));
		getobject = json_object_object_get(object,"slave_list");
		
		for(i = 0;i<(ret->Slaves);i++)
		{
			(ret->pSlaveList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopSlaveSpeakerBox(json_object_array_get_idx(getobject,i));
		}
		
		json_object_put(object);

	}
	
	return ret;
}

WARTI_pGetDeviceTime WIFIAudio_RealTimeInterface_pStringTopGetDeviceTime(char *p)
{
	WARTI_pGetDeviceTime ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetDeviceTime)calloc(1,sizeof(WARTI_GetDeviceTime));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "year", &(ret->Year));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "month", &(ret->Month));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "day", &(ret->Day));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "hour", &(ret->Hour));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "minute", &(ret->Minute));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "second", &(ret->Second));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "zone", &(ret->Zone));

		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetAlarmClock WIFIAudio_RealTimeInterface_pStringTopGetAlarmClock(char *p)
{
	WARTI_pGetAlarmClock ret;
	struct json_object *getobject;
	struct json_object *object;
	char temp[20];
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetAlarmClock)calloc(1,sizeof(WARTI_GetAlarmClock));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "n", &(ret->N));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "enable", &(ret->Enable));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "trigger", &(ret->Trigger));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "operation", &(ret->Operation));
		
		getobject = json_object_object_get(object,"date");
		if(getobject==NULL)
		{
			ret->Year = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
			ret->Month = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
			ret->Date = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
		} else {
			strcpy(temp,json_object_get_string(getobject));
			sscanf(temp,"%04d%02d%02d",&(ret->Year),&(ret->Month),&(ret->Date));
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "week_day", &(ret->WeekDay));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "day", &(ret->Day));
		
		getobject = json_object_object_get(object,"time");
		if(getobject==NULL)
		{
			ret->Hour = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
			ret->Minute = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
			ret->Second = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
		} else {
			strcpy(temp,json_object_get_string(getobject));
			sscanf(temp,"%02d%02d%02d",&(ret->Hour),&(ret->Minute),&(ret->Second));
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "volume", &(ret->Volume));
		
		getobject = json_object_object_get(object,"content");
		if(getobject==NULL)
		{
			ret->pMusic = NULL;
		} else {
			ret->pMusic = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetPlayerCmdSwitchMode WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdSwitchMode(char *p)
{
	WARTI_pGetPlayerCmdSwitchMode ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetPlayerCmdSwitchMode)calloc(1,sizeof(WARTI_GetPlayerCmdSwitchMode));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "switchmode", &(ret->SwitchMode));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetLanguage WIFIAudio_RealTimeInterface_pStringTopGetLanguage(char *p)
{
	WARTI_pGetLanguage ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetLanguage)calloc(1,sizeof(WARTI_GetLanguage));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "language", &(ret->Language));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pInsertPlaylistInfo WIFIAudio_RealTimeInterface_pStringTopInsertPlaylistInfo(char *p)
{
	WARTI_pInsertPlaylistInfo ret;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pInsertPlaylistInfo)calloc(1,sizeof(WARTI_InsertPlaylistInfo));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "insert", &(ret->Insert));
		
		getobject = json_object_object_get(object,"content");
		if(getobject==NULL)
		{
			ret->pMusic = NULL;
		} else {
			ret->pMusic = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pStorageDeviceOnlineState WIFIAudio_RealTimeInterface_pStringTopStorageDeviceOnlineState(char *p)
{
	WARTI_pStorageDeviceOnlineState ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pStorageDeviceOnlineState)calloc(1,sizeof(WARTI_StorageDeviceOnlineState));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "tf", &(ret->TF));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "usb", &(ret->USB));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetTheLatestVersionNumberOfFirmware WIFIAudio_RealTimeInterface_pStringTopGetTheLatestVersionNumberOfFirmware(char *p)
{
	WARTI_pGetTheLatestVersionNumberOfFirmware ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pGetTheLatestVersionNumberOfFirmware)calloc(1,sizeof(WARTI_GetTheLatestVersionNumberOfFirmware));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "version", &(ret->pVersion));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pPostSynchronousInfoEx WIFIAudio_RealTimeInterface_pStringTopPostSynchronousInfoEx(char *p)
{
	WARTI_pPostSynchronousInfoEx ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pPostSynchronousInfoEx)calloc(1,sizeof(WARTI_PostSynchronousInfoEx));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "slave", &(ret->Slave));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "master_ssid", &(ret->pMaster_SSID));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "master_pwd", &(ret->pMaster_Password));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pHostEx WIFIAudio_RealTimeInterface_pJsonObjectTopHostEx(struct json_object *p)
{
	WARTI_pHostEx ret;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pHostEx)calloc(1,sizeof(WARTI_HostEx));
	if(ret==NULL)
	{
		return NULL;
	}
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "ip", &(ret->pIp));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "mac", &(ret->pMac));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "mvolume", &(ret->pVolume));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "name", &(ret->pName));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "os", &(ret->pOs));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "uuid", &(ret->pUuid));
	
	return ret;
}

WARTI_pClientEx WIFIAudio_RealTimeInterface_pJsonObjectTopClientEx(struct json_object *p)
{
	WARTI_pClientEx ret;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pClientEx)calloc(1,sizeof(WARTI_ClientEx));
	if(ret==NULL)
	{
		return NULL;
	}
	
	getobject = json_object_object_get(p,"active_status");
	if(getobject==NULL)
	{
		ret->ActiveStatus = false;
	} else {
		ret->ActiveStatus = json_object_get_boolean(getobject);
	}
	
	getobject = json_object_object_get(p,"connected");
	if(getobject==NULL)
	{
		ret->Connected = false;
	} else {
		ret->Connected = json_object_get_boolean(getobject);
	}
	
	getobject = json_object_object_get(p,"host");
	if(getobject==NULL)
	{
		ret->pHost = NULL;
	} else {
		ret->pHost = WIFIAudio_RealTimeInterface_pJsonObjectTopHostEx(getobject);
	}
	
	return ret;
}

WARTI_pGetSynchronousInfoEx WIFIAudio_RealTimeInterface_pStringTopGetSynchronousInfoEx(char *p)
{
	WARTI_pGetSynchronousInfoEx ret;
	struct json_object *getobject;
	struct json_object *object;
	int i = 0;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetSynchronousInfoEx)calloc(1,sizeof(WARTI_GetSynchronousInfoEx));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "ConfigVersion", &(ret->ConfigVersion));

		getobject = json_object_object_get(object,"Client");
		if(getobject==NULL)
		{
			ret->Num = 0;
			ret->pClientList = NULL;
		}
		else
		{
			ret->Num = json_object_array_length(getobject);
			ret->pClientList = (WARTI_pClientEx *)calloc(1,(ret->Num)*sizeof(WARTI_pClientEx));
			if(ret->pClientList==NULL)
			{
				WIFIAudio_RealTimeInterface_pGetSynchronousInfoExFree(&ret);
				return NULL;
			}
			else
			{
				for(i = 0;i<(ret->Num);i++)
				{
					(ret->pClientList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopClientEx(json_object_array_get_idx(getobject,i));
				}
			}
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetRecordFileURL WIFIAudio_RealTimeInterface_pStringTopGetRecordFileURL(char *p)
{
	WARTI_pGetRecordFileURL ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		ret = (WARTI_pGetRecordFileURL)calloc(1,sizeof(WARTI_GetRecordFileURL));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "url", &(ret->pURL));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pBluetoothDevice WIFIAudio_RealTimeInterface_pJsonObjectTopBluetoothDevice(struct json_object *p)
{
	WARTI_pBluetoothDevice ret;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pBluetoothDevice)calloc(1,sizeof(WARTI_BluetoothDevice));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "connect", &(ret->Connect));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "address", &(ret->pAddress));
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "name", &(ret->pName));
	
	return ret;
}

WARTI_pGetBluetoothList WIFIAudio_RealTimeInterface_pStringTopGetBluetoothList(char *p)
{
	WARTI_pGetBluetoothList ret;
	int i;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetBluetoothList)calloc(1,sizeof(WARTI_GetBluetoothList));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "num", &(ret->Num));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "status", &(ret->Status));
		ret->pBdList = (WARTI_pBluetoothDevice*)calloc(1,(ret->Num)*sizeof(WARTI_pBluetoothDevice));
		
		getobject = json_object_object_get(object,"bdlist");
		if(getobject==NULL)
		{
		} else {
			for(i = 0;i<(ret->Num);i++)
			{
				(ret->pBdList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopBluetoothDevice(json_object_array_get_idx(getobject,i));
			}
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetSpeechRecognition WIFIAudio_RealTimeInterface_pStringTopGetSpeechRecognition(char *p)
{
	WARTI_pGetSpeechRecognition ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetSpeechRecognition)calloc(1,sizeof(WARTI_GetSpeechRecognition));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "platform", &(ret->Platform));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pLedMatrixData WIFIAudio_RealTimeInterface_pJsonObjectTopLedMatrixData(struct json_object *p)
{
	WARTI_pLedMatrixData ret = NULL;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pLedMatrixData)calloc(1,sizeof(WARTI_LedMatrixData));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(p, "data", &(ret->pData));
	
	return ret;
}

WARTI_pMultiLedMatrix WIFIAudio_RealTimeInterface_pJsonObjectTopMultiLedMatrix(struct json_object *p)
{
	WARTI_pMultiLedMatrix ret = NULL;
	int i = 0;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pMultiLedMatrix)calloc(1,sizeof(WARTI_MultiLedMatrix));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "num", &(ret->Num));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "interval", &(ret->Interval));
	
	ret->pScreenlist = (WARTI_pLedMatrixData *)calloc(1, (ret->Num)*sizeof(WARTI_pLedMatrixData));
	if(ret->pScreenlist==NULL)
	{
		WIFIAudio_RealTimeInterface_pMultiLedMatrixFree(&ret);
		return NULL;
	} else {
		getobject = json_object_object_get(p,"screenlist");
		for(i = 0;i<(ret->Num);i++)
		{
			(ret->pScreenlist)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopLedMatrixData(json_object_array_get_idx(getobject,i));
		}
	}
	
	return ret;
}

WARTI_pPostLedMatrixScreen WIFIAudio_RealTimeInterface_pStringTopPostLedMatrixScreen(char *p)
{
	WARTI_pPostLedMatrixScreen ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pPostLedMatrixScreen)calloc(1,sizeof(WARTI_PostLedMatrixScreen));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "animation", &(ret->Animation));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "length", &(ret->Length));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "width", &(ret->Width));
		
		getobject = json_object_object_get(object, "content");
		if(getobject==NULL)
		{
		} else {
			switch(ret->Animation)
			{
				case 0:
					ret->Content.pLedMatrixData = WIFIAudio_RealTimeInterface_pJsonObjectTopLedMatrixData(getobject);
					break;
					
				case 1:
					ret->Content.pMultiLedMatrix = WIFIAudio_RealTimeInterface_pJsonObjectTopMultiLedMatrix(getobject);
					break;
					
				default:
					break;
			}
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetAlexaLanguage WIFIAudio_RealTimeInterface_pStringTopGetAlexaLanguage(char *p)
{
	WARTI_pGetAlexaLanguage ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pGetAlexaLanguage)calloc(1,sizeof(WARTI_GetAlexaLanguage));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "language", &(ret->pLanguage));
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetChannelCompare WIFIAudio_RealTimeInterface_pStringTopGetChannelCompare(char *p)
{
	WARTI_pGetChannelCompare ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pGetChannelCompare)calloc(1,sizeof(WARTI_GetChannelCompare));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetDoubleFromJsonObject(object, "left", &(ret->Left));
		WIFIAudio_RealTimeInterface_GetDoubleFromJsonObject(object, "right", &(ret->Right));
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pRemoteBluetoothUpdate WIFIAudio_RealTimeInterface_pStringTopRemoteBluetoothUpdate(char *p)
{
	WARTI_pRemoteBluetoothUpdate ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pRemoteBluetoothUpdate)calloc(1,sizeof(WARTI_RemoteBluetoothUpdate));
		if(NULL == ret)
		{
			return NULL;
		}
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "url", &(ret->pURL));
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pAlarmClock WIFIAudio_RealTimeInterface_pStringTopAlarmClock(char *p)
{
	WARTI_pAlarmClock ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pAlarmClock)calloc(1,sizeof(WARTI_AlarmClock));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "num", &(ret->Num));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "enable", &(ret->Enable));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "week", &(ret->Week));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "hour", &(ret->Hour));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "minute", &(ret->Minute));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "timer", &(ret->Timer));
		
		getobject = json_object_object_get(object,"content");
		if(getobject == NULL)
		{
			ret->pContent = NULL;
		} else {
			ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pPlanPlayAlbum WIFIAudio_RealTimeInterface_pStringTopPlanPlayAlbum(char *p)
{
	WARTI_pPlanPlayAlbum ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pPlanPlayAlbum)calloc(1,sizeof(WARTI_PlanPlayAlbum));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "num", &(ret->Num));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "enable", &(ret->Enable));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "week", &(ret->Week));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "hour", &(ret->Hour));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "minute", &(ret->Minute));
		
		getobject = json_object_object_get(object,"content");
		if(getobject == NULL)
		{
			ret->pContent = NULL;
		} else {
			ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pPlanPlayMusicList WIFIAudio_RealTimeInterface_pStringTopPlanPlayMusicList(char *p)
{
	WARTI_pPlanPlayMusicList ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pPlanPlayMusicList)calloc(1,sizeof(WARTI_PlanPlayMusicList));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "num", &(ret->Num));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "enable", &(ret->Enable));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "week", &(ret->Week));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "hour", &(ret->Hour));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "minute", &(ret->Minute));
		
		getobject = json_object_object_get(object,"content");
		if(getobject == NULL)
		{
			ret->pContent = NULL;
		} else {
			ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMusicList(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pMixedElement WIFIAudio_RealTimeInterface_pJsonObjectTopMixedElement(struct json_object *p)
{
	WARTI_pMixedElement ret;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pMixedElement)calloc(1,sizeof(WARTI_MixedElement));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "type", &(ret->Type));
	
	getobject = json_object_object_get(p,"content");
	if(getobject == NULL)
	{
	} else {
		ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(getobject);
	}
	
	return ret;
}

WARTI_pMixedContent WIFIAudio_RealTimeInterface_pJsonObjectTopMixedContent(struct json_object *p)
{
	WARTI_pMixedContent ret;
	struct json_object *getobject;
	int i = 0;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pMixedContent)calloc(1,sizeof(WARTI_MixedContent));
	if(ret==NULL)
	{
		return NULL;
	}
	
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "num", &(ret->Num));
	WIFIAudio_RealTimeInterface_GetIntFromJsonObject(p, "nowplay", &(ret->NowPlay));
	
	ret->pMixedList = (WARTI_pMixedElement*)calloc(1,(ret->Num)*sizeof(WARTI_pMixedElement));
	if(NULL == ret->pMixedList)
	{
		WIFIAudio_RealTimeInterface_pMixedContentFree(&ret);
		ret = NULL;
	}
	
	getobject = json_object_object_get(p,"mixedlist");
	for(i = 0;i<(ret->Num);i++)
	{
		(ret->pMixedList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopMixedElement(json_object_array_get_idx(getobject,i));
	}
	
	return ret;
}

WARTI_pPlanMixed WIFIAudio_RealTimeInterface_pStringTopPlanMixed(char *p)
{
	WARTI_pPlanMixed ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		ret = (WARTI_pPlanMixed)calloc(1,sizeof(WARTI_PlanMixed));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "num", &(ret->Num));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "enable", &(ret->Enable));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "week", &(ret->Week));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "hour", &(ret->Hour));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "minute", &(ret->Minute));
		
		getobject = json_object_object_get(object,"content");
		if(getobject == NULL)
		{
			ret->pContent = NULL;
		} else {
			ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMixedContent(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pGetUpgradeState WIFIAudio_RealTimeInterface_pStringTopGetUpgradeState(char *p)
{
	WARTI_pGetUpgradeState ret;
	struct json_object *getobject;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	ret = (WARTI_pGetUpgradeState)calloc(1,sizeof(WARTI_GetUpgradeState));
	if(ret==NULL)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	if(!is_error(object))
	{
		WIFIAudio_RealTimeInterface_GetStringJsonStringFromJsonObject(object, "ret", &(ret->pRet));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "state", &(ret->State));
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "progress", &(ret->Progress));

		json_object_put(object);

	}
	
	return ret;
}

WARTI_pShortcutKeyList WIFIAudio_RealTimeInterface_pStringTopShortcutKeyList(char *p)
{
	WARTI_pShortcutKeyList ret;
	struct json_object *object;
	struct json_object *getobject;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_tokener_parse(p);
	
	if(!is_error(object))
	{
		ret = (WARTI_pShortcutKeyList)calloc(1,sizeof(WARTI_ShortcutKeyList));
		if(NULL == ret)
		{
			return NULL;
		}
		
		WIFIAudio_RealTimeInterface_GetIntFromJsonObject(object, "CollectionNum", &(ret->CollectionNum));
		
		getobject = json_object_object_get(object,"content");
		if(getobject == NULL)
		{
			ret->pContent = NULL;
		} else {
			ret->pContent = WIFIAudio_RealTimeInterface_pJsonObjectTopMixedContent(getobject);
		}
		
		json_object_put(object);
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_RealTimeInterface_newStr(enum WARTI_Str_type type, void *pBuf)
{
	WARTI_pStr pStr = NULL;
	
	if (pBuf == NULL)
	{
		pStr = NULL;
	} else {
		pStr = (WARTI_pStr)calloc(1,sizeof(WARTI_Str));
		if (pStr == NULL)
		{
		} else {
			pStr->type = type;
			
			switch (type)
			{
				case WARTI_STR_TYPE_GETSTATUS:
					pStr->Str.pGetStatus = WIFIAudio_RealTimeInterface_pStringTopGetStatus(pBuf);
					break;
					
				case WARTI_STR_TYPE_WLANGETAPLIST:
					pStr->Str.pWLANGetAPlist = WIFIAudio_RealTimeInterface_pStringTopWLANGetAPlist(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT:
					pStr->Str.pGetCurrentWirelessConnect = WIFIAudio_RealTimeInterface_pStringTopGetCurrentWirelessConnect(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETDEVICEINFO:
					pStr->Str.pGetDeviceInfo = WIFIAudio_RealTimeInterface_pStringTopGetDeviceInfo(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETONETOUCHCONFIGURE:
					pStr->Str.pGetOneTouchConfigure = WIFIAudio_RealTimeInterface_pStringTopGetOneTouchConfigure(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME:
					pStr->Str.pGetOneTouchConfigureSetDeviceName = WIFIAudio_RealTimeInterface_pStringTopGetOneTouchConfigureSetDeviceName(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETLOGINAMAZONSTATUS:
					pStr->Str.pGetLoginAmazonStatus = WIFIAudio_RealTimeInterface_pStringTopGetLoginAmazonStatus(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETLOGINAMAZONNEEDINFO:
					pStr->Str.pGetLoginAmazonNeedInfo = WIFIAudio_RealTimeInterface_pStringTopGetLoginAmazonNeedInfo(pBuf);
					break;
					
				case WARTI_STR_TYPE_POSTLOGINAMAZONINFO:
					pStr->Str.pPostLoginAmazonInfo = WIFIAudio_RealTimeInterface_pStringTopPostLoginAmazonInfo(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETPLAYERSTATUS:
					pStr->Str.pGetPlayerStatus = WIFIAudio_RealTimeInterface_pStringTopGetPlayerStatus(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETCURRENTPLAYLIST:
					pStr->Str.pGetCurrentPlaylist = WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETPLAYERCMDVOLUME:
					pStr->Str.pGetPlayerCmdVolume = WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdVolume(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETPLAYERCMDMUTE:
					pStr->Str.pGetPlayerCmdMute = WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdMute(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETEQUALIZER:
					pStr->Str.pGetEqualizer = WIFIAudio_RealTimeInterface_pStringTopGetEqualizer(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETPLAYERCMDCHANNEL:
					pStr->Str.pGetPlayerCmdChannel = WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdChannel(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETSHUTDOWN:
					pStr->Str.pGetShutdown = WIFIAudio_RealTimeInterface_pStringTopGetShutdown(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETMANUFACTURERINFO:
					pStr->Str.pGetManufacturerInfo = WIFIAudio_RealTimeInterface_pStringTopManufacturerInfo(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETMVREMOTEUPDATE:
					pStr->Str.pGetMvRemoteUpdate = WIFIAudio_RealTimeInterface_pStringTopGetMvRemoteUpdate(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETDOWNLOADPROGRESS:
				case WARTI_STR_TYPE_GETBLUETOOTHDOWNLOADPROGRESS:
					pStr->Str.pGetRemoteUpdateProgress = WIFIAudio_RealTimeInterface_pStringTopGetRemoteUpdateProgress(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETSLAVELIST:
					pStr->Str.pGetSlaveList = WIFIAudio_RealTimeInterface_pStringTopGetSlaveList(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETDEVICETIME:
					pStr->Str.pGetDeviceTime = WIFIAudio_RealTimeInterface_pStringTopGetDeviceTime(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETALARMCLOCK:
					pStr->Str.pGetAlarmClock = WIFIAudio_RealTimeInterface_pStringTopGetAlarmClock(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE:
					pStr->Str.pGetPlayerCmdSwitchMode = WIFIAudio_RealTimeInterface_pStringTopGetPlayerCmdSwitchMode(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETLANGUAGE:
					pStr->Str.pGetLanguage = WIFIAudio_RealTimeInterface_pStringTopGetLanguage(pBuf);
					break;
					
				case WARTI_STR_TYPE_INSERTPLAYLISTINFO:
					pStr->Str.pInsertPlaylistInfo = WIFIAudio_RealTimeInterface_pStringTopInsertPlaylistInfo(pBuf);
					break;
					
				case WARTI_STR_TYPE_STORAGEDEVICEONLINESTATE:
					pStr->Str.pStorageDeviceOnlineState = WIFIAudio_RealTimeInterface_pStringTopStorageDeviceOnlineState(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETTHELATESTVERSIONNUMBEROFFIRMWARE:
					pStr->Str.pGetTheLatestVersionNumberOfFirmware = WIFIAudio_RealTimeInterface_pStringTopGetTheLatestVersionNumberOfFirmware(pBuf);
					break;
					
				case WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX:
					pStr->Str.pPostSynchronousInfoEx = WIFIAudio_RealTimeInterface_pStringTopPostSynchronousInfoEx(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETSYNCHRONOUSINFOEX:
					pStr->Str.pGetSynchronousInfoEx = WIFIAudio_RealTimeInterface_pStringTopGetSynchronousInfoEx(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETRECORDFILEURL:
					pStr->Str.pGetRecordFileURL = WIFIAudio_RealTimeInterface_pStringTopGetRecordFileURL(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETBLUETOOTHLIST:
					pStr->Str.pGetBluetoothList = WIFIAudio_RealTimeInterface_pStringTopGetBluetoothList(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETSPEECHRECOGNITION:
					pStr->Str.pGetSpeechRecognition = WIFIAudio_RealTimeInterface_pStringTopGetSpeechRecognition(pBuf);
					break;
					
				case WARTI_STR_TYPE_POSTLEDMATRIXSCREEN:
					pStr->Str.pPostLedMatrixScreen = WIFIAudio_RealTimeInterface_pStringTopPostLedMatrixScreen(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETALEXALANGUAGE:
					pStr->Str.pGetAlexaLanguage = WIFIAudio_RealTimeInterface_pStringTopGetAlexaLanguage(pBuf);
					break;
					
				case WARTI_STR_TYPE_GETCHANNELCOMPARE:
					pStr->Str.pGetChannelCompare = WIFIAudio_RealTimeInterface_pStringTopGetChannelCompare(pBuf);
					break;
					
				case WARTI_STR_TYPE_REMOTEBLUETOOTHUPDATE:
					pStr->Str.pRemoteBluetoothUpdate = WIFIAudio_RealTimeInterface_pStringTopRemoteBluetoothUpdate(pBuf);
					break;
					
				case WARTI_STR_TYPE_CREATEALARMCLOCK:
				case WARTI_STR_TYPE_CHANGEALARMCLOCKPLAY:
				case WARTI_STR_TYPE_COUNTDOWNALARMCLOCK:
					pStr->Str.pAlarmClock = WIFIAudio_RealTimeInterface_pStringTopAlarmClock(pBuf);
					break;
					
				case WARTI_STR_TYPE_PLAYALBUMASPLAN:
					pStr->Str.pPlanPlayAlbum = WIFIAudio_RealTimeInterface_pStringTopPlanPlayAlbum(pBuf);
					break;
					
				case WARTI_STR_TYPE_PLAYMUSICLISTASPLAN:
					pStr->Str.pPlanPlayMusicList = WIFIAudio_RealTimeInterface_pStringTopPlanPlayMusicList(pBuf);
					break;
					
				case WARTI_STR_TYPE_PLAYMIXEDLISTASPLAN:
					pStr->Str.pPlanMixed = WIFIAudio_RealTimeInterface_pStringTopPlanMixed(pBuf);
					break;
					
				case WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE:
					pStr->Str.pGetUpgradeState = WIFIAudio_RealTimeInterface_pStringTopGetUpgradeState(pBuf);
					break;
					
				case WARTI_STR_TYPE_SHORTCUTKEYLIST:
					pStr->Str.pShortcutKeyList = WIFIAudio_RealTimeInterface_pStringTopShortcutKeyList(pBuf);
					break;
					
				default:
					pStr = NULL;
					break;
			}
		}
	}
	
	return pStr;
}

int WIFIAudio_RealTimeInterface_pBufFree(char **ppBuf)
{
	int ret = 0;
	
	if (ppBuf == NULL)
	{
		ret = -1;
	} 
	else 
	{
		free(*ppBuf);
		*ppBuf = NULL;
	}
	
	return ret;
}

int WIFIAudio_RealTimeInterface_uuid_unparse(const unsigned char * uuid, char *out)
{
	int ret = 0;
	int i;
	char temp[20];
	char str[37];
	char strsub[3];
	
	if((NULL == uuid) || (NULL == out))
	{
		ret = -1;
	} else {
		memcpy(temp, uuid, 16);
		memset(str, 0, 37);
		for(i = 0;i < 16;i++)
		{
			memset(strsub, 0, 3);
			sprintf(strsub, "%x%x", \
			(unsigned char)((0x0f)&(temp[i]>>4)), (unsigned char)((0x0f)&(temp[i])));
			strcat(str,strsub);
			if((3 == i)||(5 == i)||(7 == i)||(9 == i))
			{
				strcat(str,"-");
			}
		}
		strcpy(out, str);
	}
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetStatusTopString(WARTI_pGetStatus p)
{
	char *ret;
	char temp[37];
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "language", p->pLanguage);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ssid", p->pSSID);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "firmware", p->pFirmware);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "builddate", p->pBuildDate);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Release", p->pRelease);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "group", p->pGroup);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "expired", p->Expired);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "internet", p->Internet);
	WIFIAudio_RealTimeInterface_uuid_unparse((unsigned char *)p->UUID,temp);
	json_object_object_add(object,"uuid", json_object_new_string(temp));
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "netstat", p->Netstat);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "essid", p->pESSID);
	json_object_object_add(object,"apcli0", json_object_new_string(inet_ntoa(p->Apcli0)));
	json_object_object_add(object,"eth2", json_object_new_string(inet_ntoa(p->Eth2)));
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "hardware", p->pHardware);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

int WIFIAudio_RealTimeInterface_pMacAddressFillZero(char *p)
{
	char ret[18];
	char temp[18];
	int i;
	
	if(p == NULL)
	{
		return -1;
	}
	
	strcpy(ret,p);
	strcpy(temp,p);
	for(i = 2;i<=14;i = i+3)
	{
		if((ret[i])!=':')
		{
			ret[i-2] = '0';
			strcpy(&(ret[i-1]),&(temp[i-2]));
			strcpy(temp,ret);
		}
	}
	
	if('\0'==ret[16])
	{
		ret[15] = '0';
		strcpy(&(ret[16]),&(temp[15]));
	}
	
	strcpy(p,ret);
	
	return 0;
}

struct json_object* WIFIAudio_RealTimeInterface_pAccessPointTopJsonObject(WARTI_pAccessPoint p)
{
	char temp[18];
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ssid", p->pSSID);
	strcpy(temp,ether_ntoa(&(p->BSSID)));
	json_object_object_add(object,"bssid", json_object_new_string(temp));
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "rssi", p->RSSI);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "channel", p->Channel);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "auth", p->pAuth);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "encry", p->pEncry);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "extch", p->Extch);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pWLANGetAPlistTopString(WARTI_pWLANGetAPlist p)
{
	char *ret;
	int i;
	struct json_object *object;
	struct json_object *array;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "res", p->Res);
	array = json_object_new_array();
	if(p->pAplist==NULL)
	{
	} else {
		for(i = 0;i<(p->Res);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pAccessPointTopJsonObject((p->pAplist)[i]));
		}
	}
	json_object_object_add(object,"aplist",array);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetCurrentWirelessConnectTopString(WARTI_pGetCurrentWirelessConnect p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ssid", p->pSSID);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "connect", p->Connect);

	ret = strdup(json_object_to_json_string(object));

	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetDeviceInfoTopString(WARTI_pGetDeviceInfo p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "rssi", p->RSSI);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "language", p->pLanguage);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ssid", p->pSSID);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "firmware", p->pFirmware);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Release", p->pRelease);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "MAC", p->pMAC);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "uuid", p->pUUID);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "essid", p->pESSID);
	json_object_object_add(object,"apcli0", json_object_new_string(inet_ntoa(p->Apcli0)));
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "project", p->pProject);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "DeviceName", p->pDeviceName);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "battery_percent", p->BatteryPercent);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "language_support", p->pLanguageSupport);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "function_support", p->pFunctionSupport);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "key_num", p->KeyNum);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "bt_ver", p->pBtVersion);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "conexant_ver", p->pConexantVersion);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "power_supply", p->pPowerSupply);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "project_uuid", p->pProjectUuid);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetOneTouchConfigureTopString(WARTI_pGetOneTouchConfigure p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "state", p->State);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetOneTouchConfigureSetDeviceNameTopString(WARTI_pGetOneTouchConfigureSetDeviceName p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "state", p->State);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetLoginAmazonStatusTopString(WARTI_pGetLoginAmazonStatus p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "state", p->State);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPostLoginAmazonInfoTopString(WARTI_pPostLoginAmazonInfo p)
{
	char *ret;
	struct json_object *object;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "authorizationCode", p->pAuthorizationCode);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "redirectUri", p->pRedirectUri);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "clientId", p->pClientId);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetPlayerStatusTopString(WARTI_pGetPlayerStatus p)
{
	char *ret;
	struct json_object *object = NULL;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "mainmode", p->MainMode);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "nodetype", p->NodeType);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "mode", p->Mode);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "sw", p->SW);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "status", p->pStatus);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "curpos", p->CurPos);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "totlen", p->TotLen);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Title", p->pTitle);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Artist", p->pArtist);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Album", p->pAlbum);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "Year", p->Year);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "Track", p->Track);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Genre", p->pGenre);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "locallistflag", p->LocalListFlag);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "locallistfile", p->pLocalListFile);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "plicount", p->PLiCount);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "plicurr", p->PLicurr);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "vol", p->Vol);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "mute", p->Mute);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "iuri", p->pIURI);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "uri", p->pURI);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "cover", p->pCover);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "sourcemode", p->SourceMode);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "uuid", p->pUUID);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pMusicTopJsonObject(WARTI_pMusic p)
{
	struct json_object *object;
	
	if(p == NULL)
	{
		return NULL;
	}
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "index", p->Index);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "title", p->pTitle);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "artist", p->pArtist);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "album", p->pAlbum);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "content_url", p->pContentURL);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "cover_url", p->pCoverURL);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "platform", p->pPlatform);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "column", p->Column);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "program", p->Program);
	
	return object;
}

struct json_object* WIFIAudio_RealTimeInterface_pMusicListTopJsonObject(WARTI_pMusicList p)
{
	int i;
	struct json_object *object;
	struct json_object *array;
	
	if(p==NULL)
	{
		return NULL;
	}
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "index", p->Index);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "nowplay", p->NowPlay);
	
	array = json_object_new_array();
	if(p->pMusicList==NULL)
	{
		json_object_object_add(object,"musiclist", NULL);
	} else {
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
		}
	}
	json_object_object_add(object,"musiclist",array);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pGetCurrentPlaylistTopString(WARTI_pMusicList p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = WIFIAudio_RealTimeInterface_pMusicListTopJsonObject(p);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetPlayerCmdVolumeTopString(WARTI_pGetPlayerCmdVolume p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "volume", p->Volume);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetPlayerCmdMuteTopString(WARTI_pGetPlayerCmdMute p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "mute", p->Mute);	
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetEqualizerTopString(WARTI_pGetEqualizer p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "mode", p->Mode);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetPlayerCmdChannelTopString(WARTI_pGetPlayerCmdChannel p)
{
	char *ret;
	struct json_object *object;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "channel", p->Channel);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetShutdownTopString(WARTI_pGetShutdown p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	json_object_object_add(object, "sec", json_object_new_int(p->Second));
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pManufacturerInfoTopString(WARTI_pGetManufacturerInfo p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Manufacturer", p->pManuFacturer);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ManufacturerURL", p->pManuFacturerURL);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Modeldescription", p->pModelDescription);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Modelname", p->pModelName);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "Modelnumber", p->pModelNumber);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ModelURL", p->pModelURL);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateTopString(WARTI_pGetMvRemoteUpdate p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "found", p->Found);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetRemoteUpdateProgressTopString(WARTI_pGetRemoteUpdateProgress p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "state", p->State);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "upgrade", p->Upgrade);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "download", p->Download);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pSlaveSpeakerBoxTopJsonObject(WARTI_pSlaveSpeakerBox p)
{
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "name", p->pName);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "mask", p->Mask);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "volume", p->Volume);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "mute", p->Mute);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "channel", p->Channel);
	json_object_object_add(object,"ip", json_object_new_string(inet_ntoa(p->IP)));
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "version", p->pVersion);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pGetSlaveListTopString(WARTI_pGetSlaveList p)
{
	char *ret;
	int i;
	struct json_object *object;
	struct json_object *array;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "slaves", p->Slaves);
	
	array = json_object_new_array();
	if(p->pSlaveList==NULL)
	{
		json_object_object_add(object,"slave_list",NULL);
	} else {
		for(i = 0;i<(p->Slaves);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pSlaveSpeakerBoxTopJsonObject((p->pSlaveList)[i]));
		}
	}
	json_object_object_add(object,"slave_list",array);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetDeviceTimeTopString(WARTI_pGetDeviceTime p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "year", p->Year);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "month", p->Month);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "day", p->Day);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "hour", p->Hour);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "minute", p->Minute);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "second", p->Second);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject_WithoutError(object, "zone", p->Zone);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetAlarmClockTopString(WARTI_pGetAlarmClock p)
{
	char *ret;
	char temp[20];
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "n", p->N);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "enable", p->Enable);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "trigger", p->Trigger);
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "operation", p->Operation);
	
	switch(p->Trigger)
	{
		case 1:
			if((p->Year==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE) \
			|| (p->Month==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE) \
			|| (p->Date==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE))
			{
				json_object_object_add(object,"date", NULL);
			} else {
				sprintf(temp,"%04d%02d%02d",p->Year,p->Month,p->Date);
				json_object_object_add(object,"date", json_object_new_string(temp));
			}
			break;
			
		case 3:
		case 4:
			WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "week_day", p->WeekDay);
			break;
			
		case 5:
			WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "day", p->Day);
			break;
			
		default:
			break;
	}
	
	if((p->Hour==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE) \
	|| (p->Minute==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE) \
	|| (p->Second==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE))
	{
		json_object_object_add(object,"time", NULL);
	} else {
		sprintf(temp,"%02d%02d%02d",p->Hour,p->Minute,p->Second);
		json_object_object_add(object,"time", json_object_new_string(temp));
	}
	
	WIFIAudio_RealTimeInterface_IntToJsonStringAddJsonObject(object, "volume", p->Volume);
	
	if(p->pMusic==NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", \
		WIFIAudio_RealTimeInterface_pMusicTopJsonObject(p->pMusic));
	}
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetPlayerCmdSwitchModeTopString(WARTI_pGetPlayerCmdSwitchMode p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "switchmode", p->SwitchMode);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetLanguageTopString(WARTI_pGetLanguage p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "language", p->Language);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pInsertPlaylistInfoTopString(WARTI_pInsertPlaylistInfo p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "insert", p->Insert);
	if(p->pMusic==NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", \
		WIFIAudio_RealTimeInterface_pMusicTopJsonObject(p->pMusic));
	}
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pStorageDeviceOnlineStateTopString(WARTI_pStorageDeviceOnlineState p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "tf", p->TF);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "usb", p->USB);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetTheLatestVersionNumberOfFirmwareTopString(WARTI_pGetTheLatestVersionNumberOfFirmware p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "version", p->pVersion);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPostSynchronousInfoExTopString(WARTI_pPostSynchronousInfoEx p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "slave", p->Slave);
	switch(p->Slave)
	{
		case 0:
			break;
			
		case 1:
			WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "master_ssid", p->pMaster_SSID);
			WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "master_pwd", p->pMaster_Password);
			break;
			
		default:
			break;
	}
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pHostExTopJsonObject(WARTI_pHostEx p)
{
	struct json_object *object;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ip", p->pIp);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "mac", p->pMac);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "mvolume", p->pVolume);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "name", p->pName);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "os", p->pOs);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "uuid", p->pUuid);
	
	return object;
}

struct json_object* WIFIAudio_RealTimeInterface_pClientExTopJsonObject(WARTI_pClientEx p)
{
	struct json_object *object;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	json_object_object_add(object,"active_status", json_object_new_boolean(p->ActiveStatus));
	json_object_object_add(object,"connected", json_object_new_boolean(p->Connected));
	if(p->pHost==NULL)
	{
		json_object_object_add(object,"host", NULL);
	} else {
		json_object_object_add(object,"host", WIFIAudio_RealTimeInterface_pHostExTopJsonObject(p->pHost));
	}
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pGetSynchronousInfoExTopString(WARTI_pGetSynchronousInfoEx p)
{
	char *ret;
	struct json_object *object;
	struct json_object *array;
	int i = 0;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "ConfigVersion", p->ConfigVersion);
	array = json_object_new_array();
	if(p->pClientList==NULL)
	{
		json_object_object_add(object,"Client",NULL);
	} else {
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pClientExTopJsonObject((p->pClientList)[i]));
		}
	}
	json_object_object_add(object,"Client",array);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetRecordFileURLTopString(WARTI_pGetRecordFileURL p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "url", p->pURL);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pBluetoothDeviceTopJsonObject(WARTI_pBluetoothDevice p)
{
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "connect", p->Connect);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "address", p->pAddress);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "name", p->pName);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pGetBluetoothListTopString(WARTI_pGetBluetoothList p)
{
	char *ret;
	int i;
	struct json_object *object;
	struct json_object *array;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "status", p->Status);
	
	array = json_object_new_array();
	if(p->pBdList==NULL)
	{
		json_object_object_add(object,"bdlist", NULL);
	} else {
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pBluetoothDeviceTopJsonObject((p->pBdList)[i]));
		}
	}
	json_object_object_add(object,"bdlist",array);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetSpeechRecognitionTopString(WARTI_pGetSpeechRecognition p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "platform", p->Platform);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pLedMatrixDataTopJsonObject(WARTI_pLedMatrixData p)
{
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "data", p->pData);
	
	return object;
}

struct json_object* WIFIAudio_RealTimeInterface_pMultiLedMatrixTopJsonObject(WARTI_pMultiLedMatrix p)
{
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	
	if(p==NULL)
	{
		return NULL;
	}
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "interval", p->Interval);
	array = json_object_new_array();
	if(p->pScreenlist==NULL)
	{
		json_object_object_add(object,"screenlist", NULL);
	} else {
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pLedMatrixDataTopJsonObject((p->pScreenlist)[i]));
		}
	}
	json_object_object_add(object,"screenlist",array);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pGetAlexaLanguageTopString(WARTI_pGetAlexaLanguage p)
{
	char *ret = NULL;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	if((!strcmp(p->pLanguage, "en-AU")) || \
	   (!strcmp(p->pLanguage, "en-CA")) || \
	   (!strcmp(p->pLanguage, "en-GB")) || \
	   (!strcmp(p->pLanguage, "en-IN")) || \
	   (!strcmp(p->pLanguage, "en-US")) || \
	   (!strcmp(p->pLanguage, "de-DE")) || \
	   (!strcmp(p->pLanguage, "ja-JP")))
	{
		object = json_object_new_object();
		
		WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
		WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "language", p->pLanguage);
		ret = strdup(json_object_to_json_string(object));
	
		json_object_put(object);
	}
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetWakeupTimesTopString(WARTI_pGetWakeupTimes p)
{
	char *ret;
	struct json_object *object;
	
	if (p == NULL)
	{
                return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "times", p->times);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetMicarrayInfo(WARTI_pGetMicarrayInfo p)
{
	char *ret;
	struct json_object *object;
	
	if (p == NULL)
        {
                return NULL;
        }
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "mics", p->Mics);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "mics channel", p->Mics_channel);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "refs", p->Refs);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "refs channel", p->Refs_channel);
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "record dev", p->record_dev);

        ret = strdup(json_object_to_json_string(object));

        json_object_put(object);

        return ret;
}

char * WIFIAudio_RealTimeInterface_pMicarrayNormal(WARTI_pMicarrayNormal p)
{
	char *ret;
	struct json_object *object;
	
	if (p == NULL)
        {
                return NULL;
        }
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
        WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "url", p->pURL);

        ret = strdup(json_object_to_json_string(object));

        json_object_put(object);

        return ret;
}

char * WIFIAudio_RealTimeInterface_pMicarrayMic(WARTI_pMicarrayMic p)
{
	char *ret;
	int i;
	struct json_object *object = NULL;
	struct json_object *miclist_object = NULL;
	struct json_object *pTmp_object = NULL;
	
	if (p == NULL)
        {
                return NULL;
        }
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	
	miclist_object = json_object_new_array();
	if (p->ppUrlList != NULL)
	{
		for (i = 0; i < (p->Num); i++)
		{	
			pTmp_object = json_object_new_object();
			
			WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(pTmp_object, "mic", (p->ppUrlList)[i]->Id);
			WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(pTmp_object, "url", (p->ppUrlList)[i]->pURL);
		
			json_object_array_add(miclist_object, pTmp_object);
		}
	}
	json_object_object_add(object, "miclist", miclist_object);
	
        ret = strdup(json_object_to_json_string(object));
	
        json_object_put(object);
	
        return ret;
}

char * WIFIAudio_RealTimeInterface_pMicarrayRef(WARTI_pMicarrayRef p)
{
	char *ret;
	int i;
	struct json_object *object;
	struct json_object *reflist_object = NULL;
	struct json_object *pTmp_object = NULL;
	
	if (p == NULL)
        {
                return NULL;
        }
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	
	reflist_object = json_object_new_array();
	if (p->ppUrlList != NULL)
	{
		for (i = 0; i < (p->Num); i++)
		{
			pTmp_object = json_object_new_object();
			
			WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(pTmp_object, "refs", (p->ppUrlList)[i]->Id);
			WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(pTmp_object, "url", (p->ppUrlList)[i]->pURL);
		
			json_object_array_add(reflist_object, pTmp_object);
		}
	}
	
	json_object_object_add(object, "reflist", reflist_object);
	
        ret = strdup(json_object_to_json_string(object));

        json_object_put(object);

        return ret;
}

char * WIFIAudio_RealTimeInterface_pLogCaptureEnable(WARTI_pLogCaptureEnable p)
{
	char *ret;
	struct json_object *object;
	
	if (p == NULL)
	{
                return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
        WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "url", p->pURL);

        ret = strdup(json_object_to_json_string(object));

        json_object_put(object);

        return ret;
}

char * WIFIAudio_RealTimeInterface_pGetChannelCompareTopString(WARTI_pGetChannelCompare p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_DoubleToJsonDoubleAddJsonObject(object, "left", p->Left);
	WIFIAudio_RealTimeInterface_DoubleToJsonDoubleAddJsonObject(object, "right", p->Right);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pRemoteBluetoothUpdateTopString(WARTI_pRemoteBluetoothUpdate p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "url", p->pURL);

	ret = strdup(json_object_to_json_string(object));

	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPostLedMatrixScreenTopString(WARTI_pPostLedMatrixScreen p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "animation", p->Animation);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "length", p->Length);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "width", p->Width);
	switch(p->Animation)
	{
		case 0:
			if(p->Content.pLedMatrixData==NULL)
			{
				json_object_object_add(object,"content", NULL);
			} else {
				json_object_object_add(object,"content", \
				WIFIAudio_RealTimeInterface_pLedMatrixDataTopJsonObject(\
				p->Content.pLedMatrixData));
			}
			break;
		case 1:
			if(p->Content.pMultiLedMatrix==NULL)
			{
				json_object_object_add(object,"content", NULL);
			} else {
				json_object_object_add(object,"content", \
				WIFIAudio_RealTimeInterface_pMultiLedMatrixTopJsonObject(\
				p->Content.pMultiLedMatrix));
			}
			break;
		default:
			break;
	}

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pAlarmClockTopString(WARTI_pAlarmClock p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "enable", p->Enable);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "week", p->Week);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "hour", p->Hour);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "minute", p->Minute);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "timer", p->Timer);
	
	if(p->pContent == NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", \
		WIFIAudio_RealTimeInterface_pMusicTopJsonObject(p->pContent));
	}

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPlanPlayAlbumTopString(WARTI_pPlanPlayAlbum p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "enable", p->Enable);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "week", p->Week);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "hour", p->Hour);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "minute", p->Minute);
	
	if(p->pContent == NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", \
		WIFIAudio_RealTimeInterface_pMusicTopJsonObject(p->pContent));
	}

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPlanPlayMusicListTopString(WARTI_pPlanPlayMusicList p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "enable", p->Enable);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "week", p->Week);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "hour", p->Hour);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "minute", p->Minute);
	
	if(p->pContent == NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", WIFIAudio_RealTimeInterface_pMusicListTopJsonObject(p->pContent));
	}

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

struct json_object* WIFIAudio_RealTimeInterface_pMixedElementTopJsonObject(WARTI_pMixedElement p)
{
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "type", p->Type);
	
	if(p->pContent==NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", WIFIAudio_RealTimeInterface_pMusicTopJsonObject(p->pContent));
	}
	
	return object;
}

struct json_object* WIFIAudio_RealTimeInterface_pMixedContentTopJsonObject(WARTI_pMixedContent p)
{
	int i;
	struct json_object *object;
	struct json_object *array;
	
	if(p == NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "nowplay", p->NowPlay);
	array = json_object_new_array();
	if(p->pMixedList==NULL)
	{
		json_object_object_add(object,"mixedlist", NULL);
	} else {
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pMixedElementTopJsonObject((p->pMixedList)[i]));
		}
	}
	json_object_object_add(object,"mixedlist",array);
	
	return object;
}

char * WIFIAudio_RealTimeInterface_pMixedContentTopString(WARTI_pMixedContent p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = WIFIAudio_RealTimeInterface_pMixedContentTopJsonObject(p);

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pPlanMixedTopString(WARTI_pPlanMixed p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "num", p->Num);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "enable", p->Enable);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "week", p->Week);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "hour", p->Hour);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "minute", p->Minute);
	if(p->pContent == NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", WIFIAudio_RealTimeInterface_pMixedContentTopJsonObject(p->pContent));
	}

	ret = strdup(json_object_to_json_string(object));

	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetUpgradeStateTopString(WARTI_pGetUpgradeState p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "state", p->State);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "progress", p->Progress);

	ret = strdup(json_object_to_json_string(object));

	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pShortcutKeyListTopString(WARTI_pShortcutKeyList p)
{
	char *ret;
	struct json_object *object;
	
	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "CollectionNum", p->CollectionNum);
	
	if(p->pContent == NULL)
	{
		json_object_object_add(object,"content", NULL);
	} else {
		json_object_object_add(object,"content", WIFIAudio_RealTimeInterface_pMixedContentTopJsonObject(p->pContent));
	}

	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_pGetTestModeTopString(WARTI_pGetTestMode p)
{
	char *ret;
	struct json_object *object;

	if(NULL == p)
	{
		return NULL;
	}
	
	object = json_object_new_object();
	WIFIAudio_RealTimeInterface_StingToJsonStringAddJsonObject(object, "ret", p->pRet);
	WIFIAudio_RealTimeInterface_IntToJsonIntAddJsonObject(object, "test", p->test);
	
	ret = strdup(json_object_to_json_string(object));
	
	json_object_put(object);
	
	return ret;
}

char * WIFIAudio_RealTimeInterface_newBuf(WARTI_pStr pStr)
{
	char *pBuf = NULL;
	
	switch (pStr->type)
	{
		case WARTI_STR_TYPE_GETSTATUS:
			pBuf = WIFIAudio_RealTimeInterface_pGetStatusTopString(pStr->Str.pGetStatus);
			break;
			
		case WARTI_STR_TYPE_WLANGETAPLIST:
			pBuf = WIFIAudio_RealTimeInterface_pWLANGetAPlistTopString(pStr->Str.pWLANGetAPlist);
			break;
			
		case WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT:
			pBuf = WIFIAudio_RealTimeInterface_pGetCurrentWirelessConnectTopString(pStr->Str.pGetCurrentWirelessConnect);
			break;
			
		case WARTI_STR_TYPE_GETDEVICEINFO:
			pBuf = WIFIAudio_RealTimeInterface_pGetDeviceInfoTopString(pStr->Str.pGetDeviceInfo);
			break;
			
		case WARTI_STR_TYPE_GETONETOUCHCONFIGURE:
			pBuf = WIFIAudio_RealTimeInterface_pGetOneTouchConfigureTopString(pStr->Str.pGetOneTouchConfigure);
			break;
			
		case WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME:
			pBuf = WIFIAudio_RealTimeInterface_pGetOneTouchConfigureSetDeviceNameTopString(pStr->Str.pGetOneTouchConfigureSetDeviceName);
			break;
			
		case WARTI_STR_TYPE_GETLOGINAMAZONSTATUS:
			pBuf = WIFIAudio_RealTimeInterface_pGetLoginAmazonStatusTopString(pStr->Str.pGetLoginAmazonStatus);
			break;
		
		case WARTI_STR_TYPE_GETALEXALANGUAGE:
			pBuf = WIFIAudio_RealTimeInterface_pGetAlexaLanguageTopString(pStr->Str.pGetAlexaLanguage);
			break;
		
		case WARTI_STR_TYPE_GETWAKEUPTIMES:
			pBuf = WIFIAudio_RealTimeInterface_pGetWakeupTimesTopString(pStr->Str.pGetWakeupTimes);
			break;
		
		case WARTI_STR_TYPE_GETMICARRAYINFO:
			pBuf = WIFIAudio_RealTimeInterface_pGetMicarrayInfo(pStr->Str.pGetMicarrayInfo);
			break;

		case WARTI_STR_TYPE_MICARRAYNORMAL:
			pBuf = WIFIAudio_RealTimeInterface_pMicarrayNormal(pStr->Str.pMicarrayNormal);
			break;
			
		case WARTI_STR_TYPE_MICARRAYMIC:
			pBuf = WIFIAudio_RealTimeInterface_pMicarrayMic(pStr->Str.pMicarrayMic);
			break;
			
		case WARTI_STR_TYPE_MICARRAYREF:
			pBuf = WIFIAudio_RealTimeInterface_pMicarrayRef(pStr->Str.pMicarrayRef);
			break;
			
		case WARTI_STR_TYPE_LOGCAPTUREENABLE:
			pBuf = WIFIAudio_RealTimeInterface_pLogCaptureEnable(pStr->Str.pLogCaptureEnable);
			break;
		
		case WARTI_STR_TYPE_POSTLOGINAMAZONINFO:
			pBuf = WIFIAudio_RealTimeInterface_pPostLoginAmazonInfoTopString(pStr->Str.pPostLoginAmazonInfo);
			break;
			
		case WARTI_STR_TYPE_GETPLAYERSTATUS:
			pBuf = WIFIAudio_RealTimeInterface_pGetPlayerStatusTopString(pStr->Str.pGetPlayerStatus);
			break;
			
		case WARTI_STR_TYPE_GETCURRENTPLAYLIST:
			pBuf = WIFIAudio_RealTimeInterface_pGetCurrentPlaylistTopString(pStr->Str.pGetCurrentPlaylist);
			break;
			
		case WARTI_STR_TYPE_GETPLAYERCMDVOLUME:
			pBuf = WIFIAudio_RealTimeInterface_pGetPlayerCmdVolumeTopString(pStr->Str.pGetPlayerCmdVolume);
			break;
			
		case WARTI_STR_TYPE_GETPLAYERCMDMUTE:
			pBuf = WIFIAudio_RealTimeInterface_pGetPlayerCmdMuteTopString(pStr->Str.pGetPlayerCmdMute);
			break;
			
		case WARTI_STR_TYPE_GETEQUALIZER:
			pBuf = WIFIAudio_RealTimeInterface_pGetEqualizerTopString(pStr->Str.pGetEqualizer);
			break;
			
		case WARTI_STR_TYPE_GETPLAYERCMDCHANNEL:
			pBuf = WIFIAudio_RealTimeInterface_pGetPlayerCmdChannelTopString(pStr->Str.pGetPlayerCmdChannel);
			break;
			
		case WARTI_STR_TYPE_GETSHUTDOWN:
			pBuf = WIFIAudio_RealTimeInterface_pGetShutdownTopString(pStr->Str.pGetShutdown);
			break;
			
		case WARTI_STR_TYPE_GETMANUFACTURERINFO:
			pBuf = WIFIAudio_RealTimeInterface_pManufacturerInfoTopString(pStr->Str.pGetManufacturerInfo);
			break;
			
		case WARTI_STR_TYPE_GETMVREMOTEUPDATE:
			pBuf = WIFIAudio_RealTimeInterface_pGetMvRemoteUpdateTopString(pStr->Str.pGetMvRemoteUpdate);
			break;
			
		case WARTI_STR_TYPE_GETDOWNLOADPROGRESS:
		case WARTI_STR_TYPE_GETBLUETOOTHDOWNLOADPROGRESS:
			pBuf = WIFIAudio_RealTimeInterface_pGetRemoteUpdateProgressTopString(pStr->Str.pGetRemoteUpdateProgress);
			break;
			
		case WARTI_STR_TYPE_GETSLAVELIST:
			pBuf = WIFIAudio_RealTimeInterface_pGetSlaveListTopString(pStr->Str.pGetSlaveList);
			break;
			
		case WARTI_STR_TYPE_GETDEVICETIME:
			pBuf = WIFIAudio_RealTimeInterface_pGetDeviceTimeTopString(pStr->Str.pGetDeviceTime);
			break;
			
		case WARTI_STR_TYPE_GETALARMCLOCK:
			pBuf = WIFIAudio_RealTimeInterface_pGetAlarmClockTopString(pStr->Str.pGetAlarmClock);
			break;
			
		case WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE:
			pBuf = WIFIAudio_RealTimeInterface_pGetPlayerCmdSwitchModeTopString(pStr->Str.pGetPlayerCmdSwitchMode);
			break;
			
		case WARTI_STR_TYPE_GETLANGUAGE:
			pBuf = WIFIAudio_RealTimeInterface_pGetLanguageTopString(pStr->Str.pGetLanguage);
			break;
			
		case WARTI_STR_TYPE_INSERTPLAYLISTINFO:
			pBuf = WIFIAudio_RealTimeInterface_pInsertPlaylistInfoTopString(pStr->Str.pInsertPlaylistInfo);
			break;
			
		case WARTI_STR_TYPE_STORAGEDEVICEONLINESTATE:
			pBuf = WIFIAudio_RealTimeInterface_pStorageDeviceOnlineStateTopString(pStr->Str.pStorageDeviceOnlineState);
			break;
			
		case WARTI_STR_TYPE_GETTHELATESTVERSIONNUMBEROFFIRMWARE:
			pBuf = WIFIAudio_RealTimeInterface_pGetTheLatestVersionNumberOfFirmwareTopString(pStr->Str.pGetTheLatestVersionNumberOfFirmware);
			break;
			
		case WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX:
			pBuf = WIFIAudio_RealTimeInterface_pPostSynchronousInfoExTopString(pStr->Str.pPostSynchronousInfoEx);
			break;
			
		case WARTI_STR_TYPE_GETSYNCHRONOUSINFOEX:
			pBuf = WIFIAudio_RealTimeInterface_pGetSynchronousInfoExTopString(pStr->Str.pGetSynchronousInfoEx);
			break;
			
		case WARTI_STR_TYPE_GETRECORDFILEURL:
			pBuf = WIFIAudio_RealTimeInterface_pGetRecordFileURLTopString(pStr->Str.pGetRecordFileURL);
			break;
			
		case WARTI_STR_TYPE_GETBLUETOOTHLIST:
			pBuf = WIFIAudio_RealTimeInterface_pGetBluetoothListTopString(pStr->Str.pGetBluetoothList);
			break;
			
		case WARTI_STR_TYPE_GETSPEECHRECOGNITION:
			pBuf = WIFIAudio_RealTimeInterface_pGetSpeechRecognitionTopString(pStr->Str.pGetSpeechRecognition);
			break;
			
		case WARTI_STR_TYPE_POSTLEDMATRIXSCREEN:
			pBuf = WIFIAudio_RealTimeInterface_pPostLedMatrixScreenTopString(pStr->Str.pPostLedMatrixScreen);
			break;
			
		case WARTI_STR_TYPE_GETCHANNELCOMPARE:
			pBuf = WIFIAudio_RealTimeInterface_pGetChannelCompareTopString(pStr->Str.pGetChannelCompare);
			break;
			
		case WARTI_STR_TYPE_REMOTEBLUETOOTHUPDATE:
			pBuf = WIFIAudio_RealTimeInterface_pRemoteBluetoothUpdateTopString(pStr->Str.pRemoteBluetoothUpdate);
			break;
			
		case WARTI_STR_TYPE_CREATEALARMCLOCK:
		case WARTI_STR_TYPE_CHANGEALARMCLOCKPLAY:
		case WARTI_STR_TYPE_COUNTDOWNALARMCLOCK:
			pBuf = WIFIAudio_RealTimeInterface_pAlarmClockTopString(pStr->Str.pAlarmClock);
			break;
			
		case WARTI_STR_TYPE_PLAYALBUMASPLAN:
			pBuf = WIFIAudio_RealTimeInterface_pPlanPlayAlbumTopString(pStr->Str.pPlanPlayAlbum);
			break;
		case WARTI_STR_TYPE_PLAYMUSICLISTASPLAN:
			pBuf = WIFIAudio_RealTimeInterface_pPlanPlayMusicListTopString(pStr->Str.pPlanPlayMusicList);
			break;
			
		case WARTI_STR_TYPE_PLAYMIXEDLISTASPLAN:
			pBuf = WIFIAudio_RealTimeInterface_pPlanMixedTopString(pStr->Str.pPlanMixed);
			break;
			
		case WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE:
			pBuf = WIFIAudio_RealTimeInterface_pGetUpgradeStateTopString(pStr->Str.pGetUpgradeState);
			break;
			
		case WARTI_STR_TYPE_SHORTCUTKEYLIST:
			pBuf = WIFIAudio_RealTimeInterface_pShortcutKeyListTopString(pStr->Str.pShortcutKeyList);
			break;
			
		case WARTI_STR_TYPE_GETTESTMODE:
			pBuf = WIFIAudio_RealTimeInterface_pGetTestModeTopString(pStr->Str.pGetTestMode);
			break;
			
		default:
			pBuf = NULL;
			break;
	}
	
	return pBuf;
}

