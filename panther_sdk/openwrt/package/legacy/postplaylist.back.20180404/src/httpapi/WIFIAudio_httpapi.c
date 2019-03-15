#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_GetCommand.h"
#include "WIFIAudio_RealTimeInterface.h"
#include "WIFIAudio_SystemCommand.h"

//关于调用返回，具体功能实现全在这里面
int WIFIAudio_httpapi_retGetStatus(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retWlanGetApList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_WlanGetApListTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retwlanConnectAp(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_wlanConnectAp(pcomparm->Parameter.pConnectAp);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retwlanGetConnectState(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetCurrentWirelessConnect(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetDeviceInfo(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetDeviceInfo();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}

int WIFIAudio_httpapi_retGetOneTouchConfigure(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getOneTouchConfigure();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}

int WIFIAudio_httpapi_retSetOneTouchConfigure(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setOneTouchConfigure(\
		pcomparm->Parameter.pOneTouchState->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetOneTouchConfigureSetDeviceName(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}

int WIFIAudio_httpapi_retSetOneTouchConfigureSetDeviceName(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName(\
		pcomparm->Parameter.pOneTouchNameState->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetLoginAmazonStatus(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getLoginAmazonStatus();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}

/*
int WIFIAudio_httpapi_retGetLoginAmazonNeedInfo(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getLoginAmazonNeedInfo();
		
		
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}
*/

int WIFIAudio_httpapi_retLogoutAmazon(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_LogoutAmazon();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetPlayerStatus(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	int i = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetPlayerStatusTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_playlist(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边只是调试用，用过之后要记得删除
		ret = WIFIAudio_SystemCommand_setPlayerCmd_playlist(\
		pcomparm->Parameter.pSetPlayerCmd->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_pause(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_pause();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_play(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_play();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_prev(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_prev();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_next(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_next();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_seek(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_seek(pcomparm->Parameter.pSetPlayerCmd->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_stop(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_stop();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_vol(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_vol(pcomparm->Parameter.pSetPlayerCmd->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}

	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_mute(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_mute(pcomparm->Parameter.pSetPlayerCmd->Value);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}

	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_loopmode(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_loopmode(\
		pcomparm->Parameter.pSetPlayerCmd->Value);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_equalizer(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_equalizer(\
		pcomparm->Parameter.pSetPlayerCmd->Value);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_slave_vol(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_slave_mute(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_slave_channel(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_slave_channel(\
		pcomparm->Parameter.pSetPlayerCmd->Value);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd_switchmode(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setPlayerCmd_switchmode(\
		pcomparm->Parameter.pSetPlayerCmd->Value);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPlayerCmd(WAGC_pComParm pcomparm)
{
	int ret = 0;
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pSetPlayerCmd->Attr)
		{	
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAYLIST:
				WIFIAudio_httpapi_retsetPlayerCmd_playlist(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_PAUSE:
				WIFIAudio_httpapi_retsetPlayerCmd_pause(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAY:
				WIFIAudio_httpapi_retsetPlayerCmd_play(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_PREV:
				WIFIAudio_httpapi_retsetPlayerCmd_prev(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_NEXT:
				WIFIAudio_httpapi_retsetPlayerCmd_next(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_SEEK:
				WIFIAudio_httpapi_retsetPlayerCmd_seek(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_STOP:
				WIFIAudio_httpapi_retsetPlayerCmd_stop(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_VOL:
				WIFIAudio_httpapi_retsetPlayerCmd_vol(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_MUTE:
				WIFIAudio_httpapi_retsetPlayerCmd_mute(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_LOOPMODE:
				WIFIAudio_httpapi_retsetPlayerCmd_loopmode(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_EQUALIZER:
				WIFIAudio_httpapi_retsetPlayerCmd_equalizer(pcomparm);
				break;
			
			/*case WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_VOL:
				WIFIAudio_httpapi_retsetPlayerCmd_slave_vol(pcomparm);
				break;
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_MUTE:
				WIFIAudio_httpapi_retsetPlayerCmd_slave_mute(pcomparm);
				break;*///这几个接口在v0.6版本的时候去掉了
			
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_CHANNEL:
				WIFIAudio_httpapi_retsetPlayerCmd_slave_channel(pcomparm);
				break;
				
			case WAGC_COMMAND_SETPLAYERATTR_TYPE_SWITCHMODE:
				WIFIAudio_httpapi_retsetPlayerCmd_switchmode(pcomparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetEqualizer(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getEqualizerTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetCurrentPlaylist(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetCurrentPlaylistTopJsonString();
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_getSlaveList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveKickout(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveMask(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveUnMask(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveVolume(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveMute(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom_SlaveChannel(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retMultiroom(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pMultiroom->Attr)
		{
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_GETSLAVELIST:
				WIFIAudio_httpapi_retMultiroom_getSlaveList(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEKICKOUT:
				WIFIAudio_httpapi_retMultiroom_SlaveKickout(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMASK:
				WIFIAudio_httpapi_retMultiroom_SlaveMask(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEUNMASK:
				WIFIAudio_httpapi_retMultiroom_SlaveUnMask(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEVOLUME:
				WIFIAudio_httpapi_retMultiroom_SlaveVolume(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMUTE:
				WIFIAudio_httpapi_retMultiroom_SlaveMute(pcomparm);
				break;
				
			case WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVECHANNEL:
				WIFIAudio_httpapi_retMultiroom_SlaveChannel(pcomparm);
				break;
			
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retwlanSubConnectAp(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retwpsservermode(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retwpscancel(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetPlayerCmd_vol(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getPlayerCmd_volTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetPlayerCmd_mute(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetPlayerCmd_slave_channel(WAGC_pComParm pcomparm)
{
	int ret = 0;
	int retsystem = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		retsystem = WIFIAudio_SystemCommand_getPlayerCmd_slave_channel();
		
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %d\n", __FILE__, __LINE__, __FUNCTION__, retsystem);
#endif
		printf("%d", retsystem);
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetPlayerCmd_switchmode(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getPlayerCmd_switchmodeTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetPlayerCmd(WAGC_pComParm pcomparm)
{
	int ret = 0;
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pGetPlayerCmd->Attr)
		{
			case WAGC_COMMAND_GETPLAYERATTR_TYPE_VOLUME:
				WIFIAudio_httpapi_retgetPlayerCmd_vol(pcomparm);
				break;
				
			case WAGC_COMMAND_GETPLAYERATTR_TYPE_MUTE:
				WIFIAudio_httpapi_retgetPlayerCmd_mute(pcomparm);
				break;
				
			case WAGC_COMMAND_GETPLAYERATTR_TYPE_SLAVE_CHANNEL:
				WIFIAudio_httpapi_retgetPlayerCmd_slave_channel(pcomparm);
				break;
				
			case WAGC_COMMAND_GETPLAYERATTR_TYPE_SWITCHMODE:
				WIFIAudio_httpapi_retgetPlayerCmd_switchmode(pcomparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetSSID(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if((NULL == pcomparm) || (NULL == pcomparm->Parameter.pSSID) \
	|| (NULL == pcomparm->Parameter.pSSID->pSSID))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setSSID((char *)(pcomparm->Parameter.pSSID->pSSID));
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetNetwork(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if((NULL == pcomparm) || (NULL == pcomparm->Parameter.pNetwork) \
	|| (NULL == pcomparm->Parameter.pNetwork->pPassword))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setNetwork(pcomparm->Parameter.pNetwork);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retrestoreToDefault(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_restoreToDefault();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retreboot(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_reboot();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetDeviceName(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setDeviceName(pcomparm->Parameter.pName->pName);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetShutdown(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_RetJson_retJSON("OK");//直接在这边先打印返回给上层的信息
		WIFIAudio_SystemCommand_setShutdown(pcomparm->Parameter.pShutdown->Second);
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetShutdown(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getShutdown();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetManuFacturerInfo(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getManuFacturerInfo();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			
			//两个结构体结构相同，但是还是需要将指针类型转换一下
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetPowerWifiDown(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_RetJson_retJSON("OK");//直接在这边先打印返回给上层的信息
		WIFIAudio_SystemCommand_setPowerWifiDown();
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetTheLatestVersionNumberOfFirmware(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetTheLatestVersionNumberOfFirmwareTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetMvRemoteUpdate(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_getMvRemoteUpdate(pcomparm->Parameter.pUpdateURL->pURL);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retsetTimeSync(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_setTimeSync(pcomparm->Parameter.pDateTime);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetDeviceTime(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getDeviceTimeTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetAlarmClock(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getAlarmClockTopStr(pcomparm->Parameter.pClockNum->Num);
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retalarmStop(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_alarmStop();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSelectLanguage(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SelectLanguage(pcomparm->Parameter.pSelectLanguage->Language);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retgetLanguage(WAGC_pComParm pcomparm)
{
	int ret = 0;
	WARTI_pStr pstr = NULL;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_getLanguageTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retDeletePlaylistMusic(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_DeletePlaylistMusic(\
		pcomparm->Parameter.pPlaylistIndex->Index);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_setrt(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_setrt(\
		pcomparm->Parameter.pxzxAction->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_playrec(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_playrec();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_playurlrec(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_playurlrec(pcomparm->Parameter.pxzxAction->pUrl);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_startrecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_startrecord();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_stoprecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_stoprecord();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_fixedrecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_fixedrecord();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_getrecordfileurl(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_vad(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_vad(pcomparm->Parameter.pxzxAction);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_compare(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_normalrecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_normalrecord(\
		pcomparm->Parameter.pxzxAction->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_getnormalurl(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_micrecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_micrecord(\
		pcomparm->Parameter.pxzxAction->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_getmicurl(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_refrecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_refrecord(\
		pcomparm->Parameter.pxzxAction->Value);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_getrefurl(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction_stopModeRecord(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retxzxAction(WAGC_pComParm pcomparm)
{
	int ret = 0;
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pxzxAction->Attr)
		{	
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_SETRT:
				WIFIAudio_httpapi_retxzxAction_setrt(pcomparm);
				break;
			
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYREC:
				WIFIAudio_httpapi_retxzxAction_playrec(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYURLREC:
				WIFIAudio_httpapi_retxzxAction_playurlrec(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_STARTRECORD:
				WIFIAudio_httpapi_retxzxAction_startrecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPRECORD:
				WIFIAudio_httpapi_retxzxAction_stoprecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_FIXEDRECORD:
				WIFIAudio_httpapi_retxzxAction_fixedrecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_GETRECORDFILEURL:
				WIFIAudio_httpapi_retxzxAction_getrecordfileurl(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_VOICEACTIVITYDETECTION:
				WIFIAudio_httpapi_retxzxAction_vad(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_CHANNELCOMPARE:
				WIFIAudio_httpapi_retxzxAction_compare(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_NORMALRECORD:
				WIFIAudio_httpapi_retxzxAction_normalrecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_GETNORMALURL:
				WIFIAudio_httpapi_retxzxAction_getnormalurl(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_MICRECORD:
				WIFIAudio_httpapi_retxzxAction_micrecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_GETMICURL:
				WIFIAudio_httpapi_retxzxAction_getmicurl(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_REFRECORD:
				WIFIAudio_httpapi_retxzxAction_refrecord(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_GETREFURL:
				WIFIAudio_httpapi_retxzxAction_getrefurl(pcomparm);
				break;
				
			case WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPMODERECORD:
				WIFIAudio_httpapi_retxzxAction_stopModeRecord(pcomparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retStorageDeviceOnlineState(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_StorageDeviceOnlineStateTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetTfCardPlaylistInfo(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString();
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retPlayTfCard(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_PlayTfCard(\
		pcomparm->Parameter.pPlayTfCard->Index);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetUsbDiskPlaylistInfo(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString();
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retPlayUsbDisk(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_PlayUsbDisk(\
		pcomparm->Parameter.pPlayUsbDisk->Index);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetSynchronousInfoEx(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retRemoveSynchronousEx(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_RemoveSynchronousEx(\
		pcomparm->Parameter.pRemoveSynchronousEx->pMac);
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetSynchronousEx_vol(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SetSynchronousEx_vol(\
		pcomparm->Parameter.pSetSynchronousEx);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetSynchronousEx(WAGC_pComParm pcomparm)
{
	int ret = 0;
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pSetSynchronousEx->Attr)
		{	
			case WAGC_COMMAND_SETSYNCHRONOUSATTREX_TYPE_VOLUME:
				WIFIAudio_httpapi_retSetSynchronousEx_vol(pcomparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retBluetooth_Search(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_Bluetooth_Search();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retBluetooth_GetDeviceList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retBluetooth_Connect(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_Bluetooth_Connect(\
		pcomparm->Parameter.pBluetooth->pAddress);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retBluetooth_Disconnect(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_Bluetooth_Disconnect();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retBluetooth(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcomparm->Parameter.pBluetooth->Attr)
		{	
			case WAGC_COMMAND_BLUETOOTHATTR_TYPE_SEARCH:
				WIFIAudio_httpapi_retBluetooth_Search(pcomparm);
				break;
			
			case WAGC_COMMAND_BLUETOOTHATTR_TYPE_GETDEVICELIST:
				WIFIAudio_httpapi_retBluetooth_GetDeviceList(pcomparm);
				break;
				
			case WAGC_COMMAND_BLUETOOTHATTR_TYPE_CONNECT:
				WIFIAudio_httpapi_retBluetooth_Connect(pcomparm);
				break;
				
			case WAGC_COMMAND_BLUETOOTHATTR_TYPE_DISCONNECT:
				WIFIAudio_httpapi_retBluetooth_Disconnect(pcomparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetSpeechRecognition(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetSpeechRecognition(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SetSpeechRecognition(pcomparm->Parameter.pSetSpeechRecognition->Platform);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetSingleLight(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SetSingleLight(pcomparm->Parameter.pSetSingleLight);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetAlexaLanguage(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetAlexaLanguageTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetAlexaLanguage(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SetAlexaLanguage(pcomparm->Parameter.pSetAlexaLanguage->pLanguage);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retUpdataConexant(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_UpdataConexant();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retSetTestMode(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_SetTestMode(pcomparm->Parameter.pSetTestMode->Mode);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retChangeNetworkConnect(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_ChangeNetworkConnect();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetDownloadProgress(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetDownloadProgressTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetBluetoothDownloadProgress(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				printf("%s",pstring);
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retDisconnectWiFi(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_DisconnectWiFi();
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retPowerOnAutoPlay(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_PowerOnAutoPlay(pcomparm->Parameter.pSetEnable->Enable);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retRemoveAlarmClock(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_RemoveAlarmClock(pcomparm->Parameter.pAlarmAndPlan);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retEnableAlarmClock(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_EnableAlarmClock(pcomparm->Parameter.pAlarmAndPlan);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retRemovePlayPlan(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_RemovePlayPlan(pcomparm->Parameter.pAlarmAndPlan);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetAlarmClockAndPlayPlan(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetAlarmClockAndPlayPlanTopJsonString();
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetPlanMusicList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString(pcomparm->Parameter.pAlarmAndPlan);
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retEnablePlayMusicAsPlan(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_EnablePlayMusicAsPlan(pcomparm->Parameter.pAlarmAndPlan);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetPlanMixedList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetPlanMixedListTopJsonString(pcomparm->Parameter.pAlarmAndPlan);
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retUpgradeWiFiAndBluetooth(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth(pcomparm->Parameter.pUpdateURL->pURL);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retUpgradeWiFiAndBluetoothState(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	WARTI_pStr pstr = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		pstr = WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr();
		if(NULL == pstr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
			//JSON字符串打印
			pstring = WIFIAudio_RealTimeInterface_newBuf(pstr);
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			if(NULL == pstring)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
				ret = -1;
			} else {
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
				WIFIAudio_RealTimeInterface_pBufFree(&pstring);
			}
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retRemoveShortcutKeyList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_RemoveShortcutKeyList(pcomparm->Parameter.pShortcutKey->Num);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retGetShortcutKeyList(WAGC_pComParm pcomparm)
{
	int ret = 0;
	char *pstring = NULL;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边没有进行json解析，特殊处理，直接返回字符串
		pstring = WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString(pcomparm->Parameter.pShortcutKey->Num);
		if(NULL == pstring)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			ret = -1;
		} else {
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, pstring);
#endif
			printf("%s",pstring);
			WIFIAudio_RealTimeInterface_pBufFree(&pstring);
		}
	}
	
	return ret;
}

int WIFIAudio_httpapi_retEnableAutoTimeSync(WAGC_pComParm pcomparm)
{
	int ret = 0;
	
	if(NULL == pcomparm)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		ret = WIFIAudio_SystemCommand_EnableAutoTimeSync(pcomparm->Parameter.pSetEnable->Enable);
		
		if(0 == ret)
		{
			WIFIAudio_RetJson_retJSON("OK");
		} else {
			WIFIAudio_RetJson_retJSON("FAIL");
		}
	}
	
	return ret;
}

int main(int argc,char *argv[])
{
	char Comment[512];
	WAGC_pComParm pComParm;
//	char *data;

#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
	openlog(argv[0], LOG_PID, LOG_USER);
#endif
//	data = getenv("QUERY_STRING");
	char data[] = "command=GetDeviceInfo";

	memset(Comment, 0, 512);

	if (sscanf(data, "command=%s", Comment) != 1)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, Comment);
#endif

		pComParm = WIFIAudio_GetCommand_newComParm(Comment);
		
		if (pComParm != NULL)
		{
			switch (pComParm->Command)
			{
				case WIFIAUDIO_GETCOMMAND_WLANGETAPLIST:
					WIFIAudio_httpapi_retWlanGetApList(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_WLANCONNECTAP:
					WIFIAudio_httpapi_retwlanConnectAp(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETCURRENTWIRELESSCONNECT:
					WIFIAudio_httpapi_retGetCurrentWirelessConnect(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETDEVICEINFO:
					WIFIAudio_httpapi_retGetDeviceInfo(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURE:
					WIFIAudio_httpapi_retGetOneTouchConfigure(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURE:
					WIFIAudio_httpapi_retSetOneTouchConfigure(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURESETDEVICENAME:
					WIFIAudio_httpapi_retGetOneTouchConfigureSetDeviceName(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURESETDEVICENAME:
					WIFIAudio_httpapi_retSetOneTouchConfigureSetDeviceName(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONSTATUS:
					WIFIAudio_httpapi_retGetLoginAmazonStatus(pComParm);
					break;
					
	//			case WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONNEEDINFO:
	//				WIFIAudio_httpapi_retGetLoginAmazonNeedInfo(pComParm);
	//				break;
					
				case WIFIAUDIO_GETCOMMAND_LOGOUTAMAZON:
					WIFIAudio_httpapi_retLogoutAmazon(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETPLAYERSTATUS:
					WIFIAudio_httpapi_retGetPlayerStatus(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETPLAYERCMD:
					WIFIAudio_httpapi_retsetPlayerCmd(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETEQUALIZER:
					WIFIAudio_httpapi_retgetEqualizer(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETCURRENTPLAYLIST:
					WIFIAudio_httpapi_retGetCurrentPlaylist(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETPLAYERCMD:
					WIFIAudio_httpapi_retgetPlayerCmd(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETSSID:
					WIFIAudio_httpapi_retsetSSID(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETNETWORK:
					WIFIAudio_httpapi_retsetNetwork(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_RESTORETODEFAULT:
					WIFIAudio_httpapi_retrestoreToDefault(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_REBOOT:
					WIFIAudio_httpapi_retreboot(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETDEVICENAME:
					WIFIAudio_httpapi_retsetDeviceName(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETSHUTDOWN:
					WIFIAudio_httpapi_retsetShutdown(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETSHUTDOWN:
					WIFIAudio_httpapi_retgetShutdown(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETMANUFACTURERINFO:
					WIFIAudio_httpapi_retgetManuFacturerInfo(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETPOWERWIFIDOWN:
					WIFIAudio_httpapi_retsetPowerWifiDown(pComParm);
					break;
				
				case WIFIAUDIO_GETCOMMAND_GETTHELATESTVERSIONNUMBEROFFIRMWARE:
					WIFIAudio_httpapi_retGetTheLatestVersionNumberOfFirmware(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETMVREMOTEUPDATE:
					WIFIAudio_httpapi_retgetMvRemoteUpdate(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETTIMESYNC:
					WIFIAudio_httpapi_retsetTimeSync(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETDEVICETIME:
					WIFIAudio_httpapi_retgetDeviceTime(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETALARMCLOCK:
					WIFIAudio_httpapi_retgetAlarmClock(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_ALARMSTOP:
					WIFIAudio_httpapi_retalarmStop(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SELECTLANGUAGE:
					WIFIAudio_httpapi_retSelectLanguage(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETLANGUAGE:
					WIFIAudio_httpapi_retgetLanguage(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_DELETEPLAYLISTMUSIC:
					WIFIAudio_httpapi_retDeletePlaylistMusic(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_XZXACTION:
					WIFIAudio_httpapi_retxzxAction(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_STORAGEDEVICEONLINESTATE:
					WIFIAudio_httpapi_retStorageDeviceOnlineState(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETTFCARDPLAYLISTINFO:
					WIFIAudio_httpapi_retGetTfCardPlaylistInfo(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_PLAYTFCARD:
					WIFIAudio_httpapi_retPlayTfCard(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETUSBDISKPLAYLISTINFO:
					WIFIAudio_httpapi_retGetUsbDiskPlaylistInfo(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_PLAYUSBDISK:
					WIFIAudio_httpapi_retPlayUsbDisk(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETSYNCHRONOUSINFOEX:
					WIFIAudio_httpapi_retGetSynchronousInfoEx(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_REMOVESYNCHRONOUSEX:
					WIFIAudio_httpapi_retRemoveSynchronousEx(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETSYNCHRONOUSEX:
					WIFIAudio_httpapi_retSetSynchronousEx(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_BLUETOOTH:
					WIFIAudio_httpapi_retBluetooth(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETSPEECHRECOGNITION:
					WIFIAudio_httpapi_retGetSpeechRecognition(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETSPEECHRECOGNITION:
					WIFIAudio_httpapi_retSetSpeechRecognition(pComParm);
					break;
				case WIFIAUDIO_GETCOMMAND_SETSINGLELIGHT:
					WIFIAudio_httpapi_retSetSingleLight(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETALEXALANGUAGE:
					WIFIAudio_httpapi_retGetAlexaLanguage(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETALEXALANGUAGE:
					WIFIAudio_httpapi_retSetAlexaLanguage(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_UPDATACONEXANT:
					WIFIAudio_httpapi_retUpdataConexant(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_SETTESTMODE:
					WIFIAudio_httpapi_retSetTestMode(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_CHANGENETWORKCONNECT:
					WIFIAudio_httpapi_retChangeNetworkConnect(pComParm);
					break;
				
				case WIFIAUDIO_GETCOMMAND_GETDOWNLOADPROGRESS:
					WIFIAudio_httpapi_retGetDownloadProgress(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETBLUETOOTHDOWNLOADPROGRESS:
					WIFIAudio_httpapi_retGetBluetoothDownloadProgress(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_DISCONNCTWIFI:
					WIFIAudio_httpapi_retDisconnectWiFi(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_POWERONAUTOPLAY:
					WIFIAudio_httpapi_retPowerOnAutoPlay(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_REMOVEALARMCLOCK:
					WIFIAudio_httpapi_retRemoveAlarmClock(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_ENABLEALARMCLOCK:
					WIFIAudio_httpapi_retEnableAlarmClock(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_REMOVEPLAYPLAN:
					WIFIAudio_httpapi_retRemovePlayPlan(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETALARMCLOCKANDPLAYPLAN:
					WIFIAudio_httpapi_retGetAlarmClockAndPlayPlan(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETPLANMUSICLIST:
					WIFIAudio_httpapi_retGetPlanMusicList(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_ENABLEPLAYMUSICASPLAN:
					WIFIAudio_httpapi_retEnablePlayMusicAsPlan(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETPLANMIXEDLIST:
					WIFIAudio_httpapi_retGetPlanMixedList(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTH:
					WIFIAudio_httpapi_retUpgradeWiFiAndBluetooth(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTHSTATE:
					WIFIAudio_httpapi_retUpgradeWiFiAndBluetoothState(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_REMOVESHORTCUTKEYLIST:
					WIFIAudio_httpapi_retRemoveShortcutKeyList(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_GETSHORTCUTKEYLIST:
					WIFIAudio_httpapi_retGetShortcutKeyList(pComParm);
					break;
					
				case WIFIAUDIO_GETCOMMAND_ENABLEAUTOTIMESYNC:
					WIFIAudio_httpapi_retEnableAutoTimeSync(pComParm);
					break;
				default:
#ifdef WIFIAUDIO_DEBUG
					syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
					break;
			}
			WIFIAudio_GetCommand_freeComParm(&pComParm);
		}
	}
	
	fflush(stdout);
		
	return 0;
}


