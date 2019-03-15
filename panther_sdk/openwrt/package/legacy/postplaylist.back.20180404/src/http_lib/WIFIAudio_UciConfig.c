#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>  
#include <string.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "WIFIAudio_UciConfig.h"

#ifdef WIFIAUDIO_DEBUG
int WIFIAudio_UciConfig_FreeValueString(char **ppstring)
{
	int ret = 0;
	if((NULL == ppstring) || (NULL == *ppstring))
	{
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
		ret = -1;
	} else {
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
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
		pretval = NULL;
	} else {
		syslog(LOG_INFO, "%s - %d - %s: psearch = %s\n", __FILE__, __LINE__, __FUNCTION__, psearch);
		pcontext = uci_alloc_context();
	
		uci_set_confdir(pcontext, WIFIAUDIO_CONFIG_PATH);
		
		psearchcopy = strdup(psearch);
		
		if(UCI_OK != uci_lookup_ptr(pcontext, &ptr, psearchcopy, true))
		{
			pretval = NULL;
		} else {
			if(NULL == ptr.o)
			{
				pretval = NULL;
			} else {
				if(NULL == ptr.o->v.string)
				{
					pretval = NULL;
				} else {
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
	char *psearchcopy = NULL;
	struct uci_package *ppackage = NULL;
	char *ptokpackage = NULL;
	char *ptoksection = NULL;
	char *ptokoption = NULL;
	char path[128];
	
	if((NULL == psearch) || (NULL == pstring))
	{
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
		ret = -1;
	} else {
		syslog(LOG_INFO, "%s - %d - %s: psearch = %s, pstring = %s\n", __FILE__, __LINE__, __FUNCTION__, psearch, pstring);
		pcontext = uci_alloc_context();
		
		psearchcopy = strdup(psearch);
		
		ptokpackage = strtok(psearchcopy, ".");
		ptoksection = strtok(NULL, ".");
		ptokoption = strtok(NULL, ".");
		
		memset(path, 0, sizeof(path)/sizeof(char));
		strcpy(path, WIFIAUDIO_CONFIG_PATH);
		strcat(path, ptokpackage);
		
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
							find = 1;
							break;
						}
					}
					break;
				}
			}
			
			if(1 == find)
			{
				memset(&ptr, 0, sizeof(struct uci_ptr));
				ptr.package = ptokpackage;
				ptr.section = ptoksection;
				ptr.option = ptokoption;
				ptr.value = pstring;
				
				uci_set(pcontext, &ptr);
				uci_commit(pcontext, &ppackage, true);
				
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
	char *psearchcopy = NULL;
	struct uci_package *ppackage = NULL;
	char *ptokpackage = NULL;
	char *ptoksection = NULL;
	char *ptokoption = NULL;
	char path[128];
	
	if((NULL == psearch) || (NULL == pstring))
	{
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
		ret = -1;
	} else {
		syslog(LOG_INFO, "%s - %d - %s: psearch = %s, pstring = %s\n", __FILE__, __LINE__, __FUNCTION__, psearch, pstring);
		pcontext = uci_alloc_context();
		
		psearchcopy = strdup(psearch);
		ptokpackage = strtok(psearchcopy, ".");
		ptoksection = strtok(NULL, ".");
		ptokoption = strtok(NULL, ".");
		memset(path, 0, sizeof(path)/sizeof(char));
		strcpy(path, WIFIAUDIO_CONFIG_PATH);
		strcat(path, ptokpackage);
		
		if(UCI_OK == uci_load(pcontext, path, &ppackage))
		{
			memset(&ptr, 0, sizeof(struct uci_ptr));
			ptr.package = ptokpackage;
			ptr.section = ptoksection;
			ptr.option = ptokoption;
			ptr.value = pstring;
			
			uci_set(pcontext, &ptr);
			uci_commit(pcontext, &ppackage, true);
			
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
#else
int WIFIAudio_UciConfig_FreeValueString(char **ppstring)
{
	int ret = -1;
	
	if ((ppstring != NULL) && (*ppstring != NULL))
	{
		free(*ppstring);
		*ppstring = NULL;
	}
	
	return ret;
}

WIFIAudio_UciConfig_Parameter WIFIAudio_UciConfig_GetParameter(char *psearch)
{
	WIFIAudio_UciConfig_Parameter parameter;
	
	if (strcmp(psearch, "xzxwifiaudio.config.audioname") == 0)
	{
		parameter = WIFIAUDIO_UCI_PARAMETER_AUDIONAME;
	} else if (strcmp(psearch, "xzxwifiaudio.config.ap_ssid1") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AP_SSID1;
	} else if (strcmp(psearch, "xzxwifiaudio.config.audiodetectstatus") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AUDIODETECTSTATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.audionamestatus") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AUDIONAMESTATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_loginStatus") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_LOGINSTATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_ProductId") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_PRODUCTID;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_Dsn") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_DSN;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_CodeChallenge") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_CodeChallengeMethod") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGEMETHOD;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_language") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_LANGUAGE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_authorizationCode") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_AUTHORIZATIONCODE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_redirectUri") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_REDIRECTURI;
	} else if (strcmp(psearch, "xzxwifiaudio.config.amazon_clientId") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_AMAZON_CLIENTID;
	} else if (strcmp(psearch, "xzxwifiaudio.config.bt_update_flage") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_BT_UPDATE_FLAGE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.btconnectstatus") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_BTCONNECTSTATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.current_title") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CURRENT_TITLE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.current_artist") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CURRENT_ARTIST;
	} else if (strcmp(psearch, "xzxwifiaudio.config.current_contentURL") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CURRENT_CONTENTURL;
	} else if (strcmp(psearch, "xzxwifiaudio.config.current_coverURL") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CURRENT_COVERURL;
	} else if (strcmp(psearch, "xzxwifiaudio.config.current_album") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CURRENT_ALBUM;
	} else if (strcmp(psearch, "xzxwifiaudio.config.charge") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CHARGE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.channel") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_CHANNEL;
	} else if (strcmp(psearch, "xzxwifiaudio.config.function_support") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_FUNCTION_SUPPORT;
	} else if (strcmp(psearch, "xzxwifiaudio.config.firmware") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_FIRMWARE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.ip") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_IP;
	} else if (strcmp(psearch, "xzxwifiaudio.config.key_num") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_KEY_NUM;
	} else if (strcmp(psearch, "xzxwifiaudio.config.lan_ip") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_LAN_IP;
	} else if (strcmp(psearch, "xzxwifiaudio.config.language") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_LANGUAGE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.language_support") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_LANGUAGE_SUPPORT;
	} else if (strcmp(psearch, "xzxwifiaudio.config.mac_addr") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MAC_ADDR;
	} else if (strcmp(psearch, "xzxwifiaudio.config.manufacturer") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MANUFACTURER;
	} else if (strcmp(psearch, "xzxwifiaudio.config.manufacturer_url") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MANUFACTURER_URL;
	} else if (strcmp(psearch, "xzxwifiaudio.config.model_description") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MODEL_DESCRIPTION;
	} else if (strcmp(psearch, "xzxwifiaudio.config.model_name") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MODEL_NAME;
	} else if (strcmp(psearch, "xzxwifiaudio.config.model_number") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MODEL_NUMBER;
	} else if (strcmp(psearch, "xzxwifiaudio.config.model_url") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_MODEL_URL;
	} else if (strcmp(psearch, "xzxwifiaudio.config.project") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_PROJECT;
	} else if (strcmp(psearch, "xzxwifiaudio.config.power") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_POWER;
	} else if (strcmp(psearch, "xzxwifiaudio.config.playmode") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_PLAYMODE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.powoff_enable") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_POWOFF_ENABLE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.powoff_gettime") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_POWOFF_GETTIME;
	} else if (strcmp(psearch, "xzxwifiaudio.config.wificonnectstatus") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_WIFICONNECTSTATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.release") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_RELEASE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.sw_ver") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_SW_VER;
	} else if (strcmp(psearch, "xzxwifiaudio.config.speech_recognition") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_SPEECH_RECOGNITION;
	} else if (strcmp(psearch, "xzxwifiaudio.config.server_conex_ver") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_SERVER_CONEX_VER;
	} else if (strcmp(psearch, "xzxwifiaudio.config.set_timezone") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_SET_TIMEZONE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.tf_status") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_TF_STATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.testmode") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_TESTMODE;
	} else if (strcmp(psearch, "xzxwifiaudio.config.usb_status") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_USB_STATUS;
	} else if (strcmp(psearch, "xzxwifiaudio.config.volume") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_VOLUME;
	} else if (strcmp(psearch, "xzxwifiaudio.config.wifiaudio_uuid") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_WIFIAUDIO_UUID;
	} else if (strcmp(psearch, "xzxwifiaudio.config.wifi_update_flage") == 0) {
		parameter = WIFIAUDIO_UCI_PARAMETER_WIFI_UPDATE_FLAGE;
	} else {
		parameter = WIFIAUDIO_UCI_PARAMETER_NONE;
	}
	
	return parameter;
}

char * WIFIAudio_UciConfig_GetParameterString(WIFIAudio_UciConfig_Parameter parameter)
{
	char *pStr = NULL;
	
	switch(parameter)
	{
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAME:
			pStr = strdup("$audioname");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AP_SSID1:
			pStr = strdup("$ap_ssid1");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AUDIODETECTSTATUS:
			pStr = strdup("$audiodetectstatus");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAMESTATUS:
			pStr = strdup("$audionamestatus");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LOGINSTATUS:
			pStr = strdup("$amazon_loginstatus");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_PRODUCTID:
			pStr = strdup("$amazon_productid");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_DSN:
			pStr = strdup("$amazon_dsn");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGE:
			pStr = strdup("$amazon_codechallenge");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGEMETHOD:
			pStr = strdup("$amazon_codechallengemethod");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LANGUAGE:
			pStr = strdup("$amazon_language");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_AUTHORIZATIONCODE:
			pStr = strdup("$amazon_authorizationcode");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_REDIRECTURI:
			pStr = strdup("$amazon_redirecturi");
			break;
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CLIENTID:
			pStr = strdup("$amazon_clientid");
			break;
		case WIFIAUDIO_UCI_PARAMETER_BT_UPDATE_FLAGE:
			pStr = strdup("$bt_update_flage");
			break;
		case WIFIAUDIO_UCI_PARAMETER_BTCONNECTSTATUS:
			pStr = strdup("$btconnectstatus");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_TITLE:
			pStr = strdup("$current_title");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ARTIST:
			pStr = strdup("$current_artist");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_CONTENTURL:
			pStr = strdup("$current_contenturl");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_COVERURL:
			pStr = strdup("$current_coverurl");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ALBUM:
			pStr = strdup("$current_album");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CHARGE:
			pStr = strdup("$charge");
			break;
		case WIFIAUDIO_UCI_PARAMETER_CHANNEL:
			pStr = strdup("$channel");
			break;
		case WIFIAUDIO_UCI_PARAMETER_FUNCTION_SUPPORT:
			pStr = strdup("$function_support");
			break;
		case WIFIAUDIO_UCI_PARAMETER_FIRMWARE:
			pStr = strdup("$firmware");
			break;
		case WIFIAUDIO_UCI_PARAMETER_IP:
			pStr = strdup("$ip");
			break;
		case WIFIAUDIO_UCI_PARAMETER_KEY_NUM:
			pStr = strdup("$key_num");
			break;
		case WIFIAUDIO_UCI_PARAMETER_LAN_IP:
			pStr = strdup("$lan_ip");
			break;
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE:
			pStr = strdup("$language");
			break;
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE_SUPPORT:
			pStr = strdup("$language_support");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MAC_ADDR:
			pStr = strdup("$mac_addr");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER:
			pStr = strdup("$manufacturer");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER_URL:
			pStr = strdup("$manufacturer_url");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MODEL_DESCRIPTION:
			pStr = strdup("$model_description");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NAME:
			pStr = strdup("$model_name");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NUMBER:
			pStr = strdup("$model_number");
			break;
		case WIFIAUDIO_UCI_PARAMETER_MODEL_URL:
			pStr = strdup("$model_url");
			break;
		case WIFIAUDIO_UCI_PARAMETER_PROJECT:
			pStr = strdup("$project");
			break;
		case WIFIAUDIO_UCI_PARAMETER_POWER:
			pStr = strdup("$power");
			break;
		case WIFIAUDIO_UCI_PARAMETER_PLAYMODE:
			pStr = strdup("$playmode");
			break;
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_ENABLE:
			pStr = strdup("$powoff_enable");
			break;
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_GETTIME:
			pStr = strdup("$powoff_gettime");
			break;
		case WIFIAUDIO_UCI_PARAMETER_WIFICONNECTSTATUS:
			pStr = strdup("$wificonnectstatus");
			break;
		case WIFIAUDIO_UCI_PARAMETER_RELEASE:
			pStr = strdup("$release");
			break;
		case WIFIAUDIO_UCI_PARAMETER_SW_VER:
			pStr = strdup("$sw_ver");
			break;
		case WIFIAUDIO_UCI_PARAMETER_SPEECH_RECOGNITION:
			pStr = strdup("$speech_recognition");
			break;
		case WIFIAUDIO_UCI_PARAMETER_SERVER_CONEX_VER:
			pStr = strdup("$server_conex_ver");
			break;
		case WIFIAUDIO_UCI_PARAMETER_SET_TIMEZONE:
			pStr = strdup("$set_timezone");
			break;
		case WIFIAUDIO_UCI_PARAMETER_TF_STATUS:
			pStr = strdup("$tf_status");
			break;
		case WIFIAUDIO_UCI_PARAMETER_TESTMODE:
			pStr = strdup("$testmode");
			break;
		case WIFIAUDIO_UCI_PARAMETER_USB_STATUS:
			pStr = strdup("$usb_status");
			break;
		case WIFIAUDIO_UCI_PARAMETER_VOLUME:
			pStr = strdup("$volume");
			break;
		case WIFIAUDIO_UCI_PARAMETER_WIFIAUDIO_UUID:
			pStr = strdup("$wifiaudio_uuid");
			break;
		case WIFIAUDIO_UCI_PARAMETER_WIFI_UPDATE_FLAGE:
			pStr = strdup("$wifi_update_flage");
			break;
		default:
			pStr = NULL;
	}
	
	return pStr;
}

void WIFIAudio_UciConfig_FreeParameterString(char *pParameterString)
{
	if (pParameterString != NULL)
	{
		free(pParameterString);
	}
}

char * WIFIAudio_UciConfig_SearchValueString(char *psearch)
{
	WIFIAudio_UciConfig_Parameter parameter;
	char *pStr = NULL, *pRet = NULL;
	int is_ok = 0;
	int Val;
	
	if (psearch == NULL)
	{
		return pRet;
	}
	
	parameter = WIFIAudio_UciConfig_GetParameter(psearch);
	pStr = WIFIAudio_UciConfig_GetParameterString(parameter);
	pRet = malloc(WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE);
	memset(pRet, 0, WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE);
	
	switch(parameter)
	{
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAME:
		case WIFIAUDIO_UCI_PARAMETER_AP_SSID1:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_PRODUCTID:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_DSN:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGEMETHOD:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_AUTHORIZATIONCODE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_REDIRECTURI:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CLIENTID:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_TITLE:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ARTIST:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_CONTENTURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_COVERURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ALBUM:
		case WIFIAUDIO_UCI_PARAMETER_CHARGE:
		case WIFIAUDIO_UCI_PARAMETER_CHANNEL:
		case WIFIAUDIO_UCI_PARAMETER_FUNCTION_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_FIRMWARE:
		case WIFIAUDIO_UCI_PARAMETER_IP:
		case WIFIAUDIO_UCI_PARAMETER_LAN_IP:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_MAC_ADDR:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER_URL:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_DESCRIPTION:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NAME:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NUMBER:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_URL:
		case WIFIAUDIO_UCI_PARAMETER_PROJECT:
		case WIFIAUDIO_UCI_PARAMETER_RELEASE:
		case WIFIAUDIO_UCI_PARAMETER_SW_VER:
		case WIFIAUDIO_UCI_PARAMETER_SERVER_CONEX_VER:
		case WIFIAUDIO_UCI_PARAMETER_SET_TIMEZONE:
		case WIFIAUDIO_UCI_PARAMETER_WIFIAUDIO_UUID:
			if (cdb_get_str(pStr, pRet, WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE, NULL) != NULL)
			{
				is_ok = 1;
			} else {
				is_ok = 0;
			}
			break;
			
		case WIFIAUDIO_UCI_PARAMETER_AUDIODETECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAMESTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LOGINSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_BT_UPDATE_FLAGE:
		case WIFIAUDIO_UCI_PARAMETER_BTCONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_KEY_NUM:
		case WIFIAUDIO_UCI_PARAMETER_POWER:
		case WIFIAUDIO_UCI_PARAMETER_PLAYMODE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_ENABLE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_GETTIME:
		case WIFIAUDIO_UCI_PARAMETER_WIFICONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_SPEECH_RECOGNITION:
		case WIFIAUDIO_UCI_PARAMETER_TF_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_TESTMODE:
		case WIFIAUDIO_UCI_PARAMETER_USB_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_VOLUME:
		case WIFIAUDIO_UCI_PARAMETER_WIFI_UPDATE_FLAGE:
			Val = cdb_get_int(pStr, -1);
			if (Val != -1)
			{
				sprintf(pRet, "%d", Val);
				is_ok = 1;
			} else {
				is_ok = 0;
			}
			break;
		default:
			is_ok = 0;
	}
	
	if (is_ok == 0)
	{
		free(pRet);
		pRet = NULL;
	}
	
	WIFIAudio_UciConfig_FreeValueString(&pStr);
	pStr = NULL;
	
	return pRet;
}

int WIFIAudio_UciConfig_SearchAndSetValueString(char *psearch, char *pstring)
{
	int ret = -1;
	WIFIAudio_UciConfig_Parameter parameter;
	int Val = 0;
	char *pStr = NULL, *pRet = NULL;
	
	if ((psearch == NULL) || (pstring == NULL))
	{
		return ret;
	}
	
	parameter = WIFIAudio_UciConfig_GetParameter(psearch);
	pStr = WIFIAudio_UciConfig_GetParameterString(parameter);
	pRet = malloc(WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE);
	memset(pRet, 0, WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE);
	
	switch(parameter)
	{
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAME:
		case WIFIAUDIO_UCI_PARAMETER_AP_SSID1:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_PRODUCTID:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_DSN:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGEMETHOD:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_AUTHORIZATIONCODE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_REDIRECTURI:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CLIENTID:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_TITLE:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ARTIST:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_CONTENTURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_COVERURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ALBUM:
		case WIFIAUDIO_UCI_PARAMETER_CHARGE:
		case WIFIAUDIO_UCI_PARAMETER_CHANNEL:
		case WIFIAUDIO_UCI_PARAMETER_FUNCTION_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_FIRMWARE:
		case WIFIAUDIO_UCI_PARAMETER_IP:
		case WIFIAUDIO_UCI_PARAMETER_LAN_IP:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_MAC_ADDR:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER_URL:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_DESCRIPTION:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NAME:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NUMBER:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_URL:
		case WIFIAUDIO_UCI_PARAMETER_PROJECT:
		case WIFIAUDIO_UCI_PARAMETER_RELEASE:
		case WIFIAUDIO_UCI_PARAMETER_SW_VER:
		case WIFIAUDIO_UCI_PARAMETER_SERVER_CONEX_VER:
		case WIFIAUDIO_UCI_PARAMETER_SET_TIMEZONE:
		case WIFIAUDIO_UCI_PARAMETER_WIFIAUDIO_UUID:
			if (cdb_get_str(pStr, pRet, WIFIAUDIO_UCICONFIG_PARAMETER_BUFFSIZE, NULL) != NULL)
			{
				if (cdb_set(pStr, pstring) == CDB_RET_SUCCESS)
				{
					ret = 0;
				}
			}
			break;
			
		case WIFIAUDIO_UCI_PARAMETER_AUDIODETECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAMESTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LOGINSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_BT_UPDATE_FLAGE:
		case WIFIAUDIO_UCI_PARAMETER_BTCONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_KEY_NUM:
		case WIFIAUDIO_UCI_PARAMETER_POWER:
		case WIFIAUDIO_UCI_PARAMETER_PLAYMODE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_ENABLE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_GETTIME:
		case WIFIAUDIO_UCI_PARAMETER_WIFICONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_SPEECH_RECOGNITION:
		case WIFIAUDIO_UCI_PARAMETER_TF_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_TESTMODE:
		case WIFIAUDIO_UCI_PARAMETER_USB_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_VOLUME:
		case WIFIAUDIO_UCI_PARAMETER_WIFI_UPDATE_FLAGE:
			Val = cdb_get_int(pStr, -1);
			if (Val != -1)
			{
				Val = atoi(pstring);
				if (cdb_set_int(pStr, Val) == CDB_RET_SUCCESS)
				{
					ret = 0;
				}
			}
			break;
		default:
			ret = -1;
	}
	
	free(pRet);
	WIFIAudio_UciConfig_FreeValueString(&pStr);
	pRet = NULL;
	pStr = NULL;
	
	return ret;
}

int WIFIAudio_UciConfig_SetValueString(char *psearch, char *pstring)
{
	int ret = -1;
	WIFIAudio_UciConfig_Parameter parameter;
	int Val = 0;
	char *pStr = NULL;
	
	if ((psearch == NULL) || (pstring == NULL))
	{
		ret = -1;
	}
	
	parameter = WIFIAudio_UciConfig_GetParameter(psearch);
	pStr = WIFIAudio_UciConfig_GetParameterString(parameter);
	
	switch(parameter)
	{
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAME:
		case WIFIAUDIO_UCI_PARAMETER_AP_SSID1:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_PRODUCTID:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_DSN:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CODECHALLENGEMETHOD:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_AUTHORIZATIONCODE:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_REDIRECTURI:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_CLIENTID:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_TITLE:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ARTIST:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_CONTENTURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_COVERURL:
		case WIFIAUDIO_UCI_PARAMETER_CURRENT_ALBUM:
		case WIFIAUDIO_UCI_PARAMETER_CHARGE:
		case WIFIAUDIO_UCI_PARAMETER_CHANNEL:
		case WIFIAUDIO_UCI_PARAMETER_FUNCTION_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_FIRMWARE:
		case WIFIAUDIO_UCI_PARAMETER_IP:
		case WIFIAUDIO_UCI_PARAMETER_LAN_IP:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE:
		case WIFIAUDIO_UCI_PARAMETER_LANGUAGE_SUPPORT:
		case WIFIAUDIO_UCI_PARAMETER_MAC_ADDR:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER:
		case WIFIAUDIO_UCI_PARAMETER_MANUFACTURER_URL:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_DESCRIPTION:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NAME:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_NUMBER:
		case WIFIAUDIO_UCI_PARAMETER_MODEL_URL:
		case WIFIAUDIO_UCI_PARAMETER_PROJECT:
		case WIFIAUDIO_UCI_PARAMETER_RELEASE:
		case WIFIAUDIO_UCI_PARAMETER_SW_VER:
		case WIFIAUDIO_UCI_PARAMETER_SERVER_CONEX_VER:
		case WIFIAUDIO_UCI_PARAMETER_SET_TIMEZONE:
		case WIFIAUDIO_UCI_PARAMETER_WIFIAUDIO_UUID:
			if (cdb_set(pStr, pstring) == CDB_RET_SUCCESS)
			{
				ret = 0;
			}
			break;
			
		case WIFIAUDIO_UCI_PARAMETER_AUDIODETECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AUDIONAMESTATUS:
		case WIFIAUDIO_UCI_PARAMETER_AMAZON_LOGINSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_BT_UPDATE_FLAGE:
		case WIFIAUDIO_UCI_PARAMETER_BTCONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_KEY_NUM:
		case WIFIAUDIO_UCI_PARAMETER_POWER:
		case WIFIAUDIO_UCI_PARAMETER_PLAYMODE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_ENABLE:
		case WIFIAUDIO_UCI_PARAMETER_POWOFF_GETTIME:
		case WIFIAUDIO_UCI_PARAMETER_WIFICONNECTSTATUS:
		case WIFIAUDIO_UCI_PARAMETER_SPEECH_RECOGNITION:
		case WIFIAUDIO_UCI_PARAMETER_TF_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_TESTMODE:
		case WIFIAUDIO_UCI_PARAMETER_USB_STATUS:
		case WIFIAUDIO_UCI_PARAMETER_VOLUME:
		case WIFIAUDIO_UCI_PARAMETER_WIFI_UPDATE_FLAGE:
			Val = atoi(pstring);
			if (cdb_set_int(pStr, Val) == CDB_RET_SUCCESS)
			{
				ret = 0;
			}
			break;
		default:
			ret = -1;
	}
	
	WIFIAudio_UciConfig_FreeValueString(&pStr);
	pStr = NULL;
	
	return ret;
}

#endif

