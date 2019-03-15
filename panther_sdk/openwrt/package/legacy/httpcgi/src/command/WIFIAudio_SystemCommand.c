#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h> 
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h> 

#include "libcchip/libcchip.h"
#include <avsclient/libavsclient.h>
#include "WIFIAudio_SystemCommand.h"
//#include <libcchip.h>

int WIFIAudio_SystemCommand_FileExists(char *pfilename)
{
	int ret = 0;
	
	if(NULL == pfilename)
	{
		ret = -1;
	} else {
		if(NULL == fopen(pfilename, "r"))
		{
			ret = 0;
		} else {
			ret = 1;
		}
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_SystemShellCommand(char *pstring)
{
	int ret = 0;
	FILE *fp = NULL;
	
	if(NULL == pstring)
	{
		ret = -1;
	} else {
		fp = popen(pstring, "r");
		if(NULL == fp)
		{
			ret = -1;
		} else {
			pclose(fp);
			fp = NULL;
		}
	}
	
	return ret;
}


char* WIFIAudio_SystemCommand_GetOneLineFromFile(char *pPathName, int Len)
{
	FILE *fp = NULL;
	char *pStr = NULL;
	
	if((pPathName != NULL) && (Len >= 1))
	{
		fp = fopen(pPathName, "r");
		if(fp != NULL)
		{
			flock(fileno(fp), LOCK_EX);
			
			pStr = (char *)calloc(1, Len + 1);
			if(pStr != NULL)
			{
				memset(pStr, 0, Len + 1);
				if(fgets(pStr, Len, fp) == NULL)
				{
					free(pStr);
					pStr = NULL;
				}
			}
			
			flock(fileno(fp), LOCK_UN);
			fclose(fp);
			fp = NULL; 
		}
	}
	
	return pStr;
}

char * WIFIAudio_SystemCommand_SsidOfMotageToMtk(char *pstring)
{
	char *pret = pstring;
	char tmp[5];
	char ssid[256];
	int i = 0;
	int len = 0;
	int hex;
	
	if(NULL == pstring)
	{
		pret = NULL;
	} else {
		if(NULL != strstr(pstring,"\\x"))
		{
			memset(ssid, 0, sizeof(ssid));
			strcpy(ssid, "0x");
			len = strlen(pstring);
			for(i = 0; i < len; i++)
			{
				if(0 == memcmp(pstring + i, "\\x", 2))
				{
					memset(tmp, 0, sizeof(tmp));
					memcpy(tmp, pstring + i, 4);
					sscanf(tmp, "\\x%02x", &hex);
					
					memset(tmp, 0, sizeof(tmp));
					sprintf(tmp, "%02X", (unsigned char)hex);
					strcat(ssid, tmp);
					i += 3;
				} else {
					sprintf(tmp, "%02X", (unsigned char)pstring[i]);
					strcat(ssid, tmp);
				}
			}
			
			pret = strdup(ssid);
			free(pstring);
			pstring = NULL;
		}
	}
	
	return pret;
}

WARTI_pStr WIFIAudio_SystemCommand_WlanGetApListTopStr(void)
{
	int i;
	int j;//调试计数变量
	int num = 0;
	float signal = 0;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	FILE *fpopen = NULL;
	char tempbuff[256];
	char ssid[64];

	system("iw dev sta0 scan | grep -E \"Au|SSID|signal\" > /tmp/ApList.txt");
	//由于要先计算有多少个AP热点，所以先重定向写入文件当中
	//这边没有后台运行，应该是等待执行完才会继续往下走
	
	//利用grep配合正则表达式当中的点.符号排除\r\n之外的字符
	//用wc统计有多少个可见字符的SSID
	fpopen = popen("cat /tmp/ApList.txt | grep \"SSID: .\" | wc -l", "r");
	if(NULL == fpopen)
	{
		pstr = NULL;
	}
	else
	{
		memset(tempbuff, 0, sizeof(tempbuff));
		if(NULL != fgets(tempbuff, sizeof(tempbuff), fpopen))
		{
			num = atoi(strtok(tempbuff, "\r\n"));
		}
		pclose(fpopen);
		fpopen = NULL;
		
		//申请空间
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
			//PRINTF("Error of calloc : NULL == pstr in WIFIAudio_SystemCommand_WlanGetApListTopStr\r\n");
		}
		else
		{
			pstr->type = WARTI_STR_TYPE_WLANGETAPLIST;
			
			pstr->Str.pWLANGetAPlist = (WARTI_pWLANGetAPlist)calloc(1, sizeof(WARTI_WLANGetAPlist));
			if(NULL == pstr->Str.pWLANGetAPlist)
			{
				//PRINTF("Error of calloc : NULL == pstr->Str.pWLANGetAPlist in WIFIAudio_SystemCommand_WlanGetApListTopStr\r\n");
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			}
			else
			{
				pstr->Str.pWLANGetAPlist->pRet = strdup("OK");
				
				pstr->Str.pWLANGetAPlist->Res = num;
				pstr->Str.pWLANGetAPlist->pAplist = \
				(WARTI_pAccessPoint*)calloc(1, (pstr->Str.pWLANGetAPlist->Res) \
				* sizeof(WARTI_pAccessPoint));
				//分配几个指向WARTI_pLocalMusicFile的指针
				if(NULL == pstr->Str.pWLANGetAPlist->pAplist)
				{
					//PRINTF("Error of calloc : NULL == pstr->Str.pWLANGetAPlist->pAplist in WIFIAudio_SystemCommand_WlanGetApListTopStr\r\n");
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				}
				else
				{
					fp = fopen("/tmp/ApList.txt", "r");
					if(NULL == fp)
					{
						//PRINTF("Error of popen : NULL == fp in WIFIAudio_SystemCommand_WlanGetApListTopStr\r\n");
						WIFIAudio_RealTimeInterface_pStrFree(&pstr);
						pstr = NULL;
					}
					else
					{
						for(i = 0; i< pstr->Str.pWLANGetAPlist->Res; i++)
						{
							//for循环当中，判断一下之前一个循环有没有预读到下一个SSID的信息
							if(NULL == strstr(tempbuff, "signal:"))
							{
								//不含信号强度信息，就先获取信号强度所在信息
								while(1)
								{
									memset(tempbuff, 0, sizeof(tempbuff));
									fgets(tempbuff, sizeof(tempbuff), fp);
									if(NULL != strstr(tempbuff, "signal: "))
									{
										//信号强度先不处理
										break;
									}
									else
									{
										//一直读，直到遇到signal才是一个信号所有信息的开始
										continue;
									}
								}
							}
							
							//signal之后就是ssid，先把ssid读出来，看下是否为空
							memset(ssid, 0, sizeof(ssid));
							fgets(ssid, sizeof(ssid), fp);
							//将后面的\n去掉
							strtok(ssid, "\r\n");
							
							//去掉一个水平制表符\9
							memmove(ssid, &(ssid[1]), strlen(ssid) - 1);
							//移动之后，最后一个没有清零
							ssid[strlen(ssid) - 1] = 0;
							if((0 == strcmp("SSID: ", ssid)) || (0 == strcmp("SSID:  ", ssid)))
							{
								//相当于如果SSID字符串为空
								//重新开启一次循环的时候先把此次循环的扣掉
								//并把刚才读取的signal清空
								i--;
								memset(tempbuff, 0, sizeof(tempbuff));
								continue;
							}
							
							(pstr->Str.pWLANGetAPlist->pAplist)[i] = (WARTI_pAccessPoint)calloc(1,sizeof(WARTI_AccessPoint));
							if(NULL == (pstr->Str.pWLANGetAPlist->pAplist)[i])
							{
								//PRINTF("Error of calloc : NULL == (pstr->Str.pWLANGetAPlist->pAplist)[%d] in WIFIAudio_SystemCommand_WlanGetApListTopStr\r\n", i);
								WIFIAudio_RealTimeInterface_pStrFree(&pstr);
								pstr = NULL;
								break;
							}
							else
							{
								//先处理信号强度信息
								sscanf(strstr(tempbuff, "signal: "), "signal: %f dBm", &signal);
								//这边取出来的是负值的功率
								//假设-42dbm是100%，-90是0%
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = (int)(signal+142);
								if((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI >= 100)
								{
									//大于-42dbm，都设为强度100
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = 100;
								}
								else if((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI > 52)
								{
									//小于-42dbm，将52-100的部分做线性比例
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = (int)(((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI - 52) / 0.48);
								}
								else
								{
									//小于-90dbm，都设置为强度0
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = 0;
								}
								
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->pSSID = strdup(&(ssid[strlen("SSID: ")]));				
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->pSSID = \
								WIFIAudio_SystemCommand_SsidOfMotageToMtk((pstr->Str.pWLANGetAPlist->pAplist)[i]->pSSID);
								
								memset(tempbuff, 0, sizeof(tempbuff));
								fgets(tempbuff, sizeof(tempbuff), fp);
								if(NULL != strstr(tempbuff, "Authentication suites:"))
								{
									//有读到加密状态
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->pAuth = strdup("WPA2PSK");
								}
								else
								{
									//没有读到加密状态，这个时候已经把下一次的singal读进tempbuff了
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->pAuth = strdup("OPEN");
								}
								
								//MAC地址不管
								//信道不管，直接赋值为1
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->Channel = 1;
								//加密类型不管
								//直接给1
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->Extch = 1;
/*
for(j = 0; j < strlen(tempbuff); j++)
{
	printf(" 0x%2X,", tempbuff[j]);
}
printf("\r\n");
*/
//break;
//为了测试方便
							}
						}	
						fclose(fp);
						fp = NULL;
					}
				}
			}
		}
	}

	return pstr;
}

int WIFIAudio_SystemCommand_wlanConnectAp(WAGC_pConnectAp_t pconnectap)
{
	int ret = 0;
	
	if(pconnectap == NULL)
	{
		ret = -1;
	} else {	
		ret = connect_wifi(pconnectap->pSSID, pconnectap->pPassword);
	}
	
	return ret;
}

char * WIFIAudio_SystemCommand_GetConnectWifiName(void)
{
	char *pretname = NULL;
	
	return pretname;
}

WARTI_pStr WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pSSID = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr == NULL)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT;
		
		pstr->Str.pGetCurrentWirelessConnect = (WARTI_pGetCurrentWirelessConnect)calloc(1, sizeof(WARTI_GetCurrentWirelessConnect));
		
		if(pstr->Str.pGetCurrentWirelessConnect == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pSSID = search_connect_router_ssid();
			if (pSSID != NULL)
			{
				pstr->Str.pGetCurrentWirelessConnect->pRet = strdup("OK");
				pstr->Str.pGetCurrentWirelessConnect->pSSID = strdup(pSSID);
				pstr->Str.pGetCurrentWirelessConnect->Connect = get_wifi_ConnectStatus();
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}

	return pstr;
}

int WIFIAudio_SystemCommand_GetLocalWifiIp(struct in_addr *pip)
{
	int ret = 0;
	
	return ret;
}

char* WIFIAudio_SystemCommand_MacChange(char *pMacString)
{
	char *pMacNew = NULL;
	int i = 0;
	int j = 0;
	
	if(NULL == pMacString)
	{
		
	} else {
		pMacNew = (char *)calloc(1, 6*2+5+1);
		
		for(i = 0, j = 0; j < 12;i=i+2, j=j+2)
		{
			pMacNew[i] = pMacString[j];
			pMacNew[i+1] = pMacString[j+1];
			if(j < 10)
			{
				pMacNew[i+2] = ':';
				i++;
			}
		}
		pMacNew[i] = '\0';
	}
	
	return pMacNew;
}

WARTI_pStr WIFIAudio_SystemCommand_GetDeviceInfo(void)
{
	WARTI_pStr pstr = NULL;
	DEVICEINFO *pDeviceInfo = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETDEVICEINFO;
		
		pstr->Str.pGetDeviceInfo = (WARTI_pGetDeviceInfo)calloc(1, sizeof(WARTI_GetDeviceInfo));
		if(pstr->Str.pGetDeviceInfo == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pDeviceInfo = (DEVICEINFO *)calloc(1, sizeof(DEVICEINFO));
			if (pDeviceInfo != NULL)
			{
				if (get_device_info(pDeviceInfo) == 0)
				{
					pstr->Str.pGetDeviceInfo->pRet = strdup("OK");
					pstr->Str.pGetDeviceInfo->pLanguage = strdup(pDeviceInfo->sys_language);
					pstr->Str.pGetDeviceInfo->pLanguageSupport = strdup(pDeviceInfo->sup_language);
					pstr->Str.pGetDeviceInfo->pFirmware = strdup(pDeviceInfo->firmware);
					pstr->Str.pGetDeviceInfo->pRelease = strdup(pDeviceInfo->release);
					pstr->Str.pGetDeviceInfo->pMAC = strdup(pDeviceInfo->mac_addr);
					pstr->Str.pGetDeviceInfo->pUUID = strdup(pDeviceInfo->uuid);
					pstr->Str.pGetDeviceInfo->pProject = strdup(pDeviceInfo->project);
					pstr->Str.pGetDeviceInfo->BatteryPercent = atoi(pDeviceInfo->battery_level);
					pstr->Str.pGetDeviceInfo->pFunctionSupport = strdup(pDeviceInfo->function_support);
					pstr->Str.pGetDeviceInfo->pBtVersion = strdup(pDeviceInfo->bt_ver);
					pstr->Str.pGetDeviceInfo->KeyNum = atoi(pDeviceInfo->key_num);
					pstr->Str.pGetDeviceInfo->pPowerSupply = strdup(pDeviceInfo->charge_plug);
					pstr->Str.pGetDeviceInfo->pSSID = strdup(pDeviceInfo->audioname);
					pstr->Str.pGetDeviceInfo->pDeviceName = strdup(pDeviceInfo->audioname);
					pstr->Str.pGetDeviceInfo->pProjectUuid = strdup(pDeviceInfo->project_uuid);
				}
				
				free(pDeviceInfo);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigure(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETONETOUCHCONFIGURE;
		
		pstr->Str.pGetOneTouchConfigure = (WARTI_pGetOneTouchConfigure)calloc(1, sizeof(WARTI_GetOneTouchConfigure));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetOneTouchConfigure->pRet = strdup("OK");
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_setOneTouchConfigure(int value)
{
	int ret = 0;
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME;
		
		pstr->Str.pGetOneTouchConfigureSetDeviceName = (WARTI_pGetOneTouchConfigureSetDeviceName)calloc(1, sizeof(WARTI_GetOneTouchConfigureSetDeviceName));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetOneTouchConfigureSetDeviceName->pRet = strdup("OK");
		}
	}

	return pstr;
}

int WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName(int value)
{
	int ret = 0;
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonStatus(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETLOGINAMAZONSTATUS;
		
		pstr->Str.pGetLoginAmazonStatus = (WARTI_pGetLoginAmazonStatus)calloc(1, sizeof(WARTI_GetLoginAmazonStatus));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pstr->Str.pGetLoginAmazonStatus->pRet = strdup("OK");
			pstr->Str.pGetLoginAmazonStatus->State = OnGetAVSSigninStatus();
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_LogoutAmazon(void)
{
	int ret = 0;
	
	ret = OnSignUPAmazonAlexa();
	
	return ret;
}

int WIFIAudio_SystemCommand_GetSwitchMode(void)
{
	int mode = 0;
	
	return mode;
}

int WIFIAudio_SystemCommand_GetSlaveflag(void)
{
	int slave = 0;
	
	return slave;
}

int WIFIAudio_SystemCommand_GetLocalListFlag(void)
{
	int flag = 0;
	
	return flag;
}

WARTI_pStr WIFIAudio_SystemCommand_GetWakeupTimesTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETWAKEUPTIMES;
		pstr->Str.pGetWakeupTimes = (WARTI_pGetWakeupTimes)calloc(1, sizeof(WARTI_GetWakeupTimes));
		if (pstr->Str.pGetWakeupTimes == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pstr->Str.pGetWakeupTimes->pRet = strdup("OK");
			pstr->Str.pGetWakeupTimes->times = get_wakeup_times();
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_GetPlayerStatusTopStr(void)
{
	WARTI_pStr pstr = NULL;
/*	PLAYER_STATUS *pStatus = NULL;
	
	pStatus = MediaPlayerAllStatus();
	if (pStatus == NULL)
	{
		return pstr;
	}
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETPLAYERSTATUS;
		pstr->Str.pGetPlayerStatus = (WARTI_pGetPlayerStatus)calloc(1, sizeof(WARTI_GetPlayerStatus));
		if(pstr->Str.pGetPlayerStatus == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pstr->Str.pGetPlayerStatus->pRet = strdup("OK");
			pstr->Str.pGetPlayerStatus->MainMode = pStatus->mainmode;
			pstr->Str.pGetPlayerStatus->Mode = 0;
			pstr->Str.pGetPlayerStatus->CurPos = pStatus->curpos;
			pstr->Str.pGetPlayerStatus->TotLen = pStatus->totlen;
			pstr->Str.pGetPlayerStatus->pTitle = strdup(pStatus->Title);
			pstr->Str.pGetPlayerStatus->pArtist = strdup(pStatus->Artist);
			pstr->Str.pGetPlayerStatus->pAlbum = strdup(pStatus->Album);
			pstr->Str.pGetPlayerStatus->Year = pStatus->Year;
			pstr->Str.pGetPlayerStatus->Track = pStatus->Track;
			pstr->Str.pGetPlayerStatus->pGenre = strdup(pStatus->Genre);
			pstr->Str.pGetPlayerStatus->LocalListFlag = pStatus->TFflag;
			pstr->Str.pGetPlayerStatus->PLiCount = pStatus->plicount;
			pstr->Str.pGetPlayerStatus->PLicurr = pStatus->plicurr;	
			pstr->Str.pGetPlayerStatus->Vol = pStatus->vol;	
			pstr->Str.pGetPlayerStatus->Mute = pStatus->mute;
			pstr->Str.pGetPlayerStatus->pURI = strdup(pStatus->uri);
			pstr->Str.pGetPlayerStatus->pCover = strdup(pStatus->cover);
			pstr->Str.pGetPlayerStatus->SourceMode = pStatus->sourcemode;
			pstr->Str.pGetPlayerStatus->pUUID = strdup(pStatus->uuid);
		}
	}*/
	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_playlist(int num)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "mpc play %d", num);
	
	system(str);
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_pause(void)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerPause();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_play(void)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerPlay();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_prev(void)
{
	int ret = 0;
	/*bool ret_val;

	ret_val = MediaPlayerPrevious();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_next(void)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerNext();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_seek(int num)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerSeek();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_stop(void)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerStop();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_vol(int num)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerSetVolume();
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_volTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	/*pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETPLAYERCMDVOLUME;
	
	pstr->Str.pGetPlayerCmdVolume = (WARTI_pGetPlayerCmdVolume)calloc(1, sizeof(WARTI_GetPlayerCmdVolume));
	pstr->Str.pGetPlayerCmdVolume->pRet = strdup("OK");
	//pstr->Str.pGetPlayerCmdVolume->Volume = MediaPlayerGetVolume();*/
	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_mute(int flag)
{
	int ret = 0;
	/*bool mute_val;
	bool ret_val;
	
	if (flag == 1)
	{
		mute_val = true;
	} else {
		mute_val = false;
	}
	
	ret_val = MediaPlayerSetMute(mute_val);
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr(void)
{
	WARTI_pStr pstr = NULL;
	/*bool Mute_val;
	int Mute = 1;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETPLAYERCMDMUTE;
	
	pstr->Str.pGetPlayerCmdMute = (WARTI_pGetPlayerCmdMute)calloc(1, sizeof(WARTI_GetPlayerCmdMute));
	pstr->Str.pGetPlayerCmdMute->pRet = strdup("OK");
	Mute_val = MediaPlayerMuteStatus();
	if (Mute_val == true)
	{
		Mute = 1;
	} else {
		Mute = 0;
	}
	
	pstr->Str.pGetPlayerCmdMute->Mute = Mute;*/
	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_loopmode(int num)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerSetPlayMode(num);
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_equalizer(int mode)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerSetEqualizerMode(mode);
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getEqualizerTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int EqualizerMode = 0;
	
	/*pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	pstr->type = WARTI_STR_TYPE_GETEQUALIZER;
	
	pstr->Str.pGetEqualizer = (WARTI_pGetEqualizer)calloc(1, sizeof(WARTI_GetEqualizer));
	pstr->Str.pGetEqualizer->pRet = strdup("OK");
	
	EqualizerMode = MediaPlayerGetEqualizerMode();
	pstr->Str.pGetEqualizer->Mode = EqualizerMode;*/
	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_slave_channel(int value)
{
	int ret = 0;
	/*bool ret_val;
	
	ret_val = MediaPlayerSetChannel(value);
	if (ret_val != true)
	{
		ret = -1;
	}*/
	
	return ret;
}

int WIFIAudio_SystemCommand_getPlayerCmd_slave_channel(void)
{
	int ret = 3;
	
	return ret;
}

int WIFIAudio_SystemCommand_setSSID(char *pssid)
{
	int ret = 0;
	
	if(pssid == NULL)
	{
		ret = -1;
	} else {	
		ret = set_device_ssid(pssid);
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_setNetwork(WAGC_pNetwork_t pnetwork)
{
	int ret = 0;
	
	if(pnetwork == NULL)
	{
		ret = -1;
	} else {
		if (pnetwork->Auth == 1)
		{
			if (pnetwork->pPassword != NULL)
			{
				ret = set_device_pwd(pnetwork->pPassword);
				if (ret == 0)
				{
					ret = 1;
				} else {
					ret = -1;
				}
			} else {
				ret = -1;
			}
		}
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_restoreToDefault(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "factoryReset.sh");
	
	system(str);
	
	return ret;
}

int WIFIAudio_SystemCommand_reboot(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "reboot");
	
	system(str);
	
	return ret;
}

int WIFIAudio_SystemCommand_setDeviceName(char *pname)
{
	int ret = 0;
	char str[128];
	
	if(pname == NULL)
	{
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "upmpdcliChangeName.sh '%s'", pname);
	
		system(str);
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_setShutdown(int second)
{
	int ret = 0;
	
	ret = enable_shutdown_set(second);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getShutdown(void)
{
	WARTI_pStr pstr = NULL;
	char *pucistring = NULL;
	int enable = 0;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL != pstr)
	{
		pstr->type = WARTI_STR_TYPE_GETSHUTDOWN;
		
		pstr->Str.pGetShutdown = (WARTI_pGetShutdown)calloc(1, sizeof(WARTI_GetShutdown));
		if(NULL == pstr->Str.pGetShutdown)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pstr->Str.pGetShutdown->Second = get_shutdown_remain_time();
			if (pstr->Str.pGetShutdown->Second == -1)
			{
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			} else {
				pstr->Str.pGetShutdown->pRet = strdup("OK");
			}
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_getManuFacturerInfo(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	if(NULL != pstr)
	{
		pstr->type = WARTI_STR_TYPE_GETMANUFACTURERINFO;
		
		pstr->Str.pGetManufacturerInfo = (WARTI_pGetManufacturerInfo)calloc(1, sizeof(WARTI_GetManufacturerInfo));
		if(NULL == pstr->Str.pGetManufacturerInfo)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetManufacturerInfo->pRet = strdup("OK");
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_setPowerWifiDown(void)
{
	int ret = 0;
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetTheLatestVersionNumberOfFirmwareTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETTHELATESTVERSIONNUMBEROFFIRMWARE;
	
	pstr->Str.pGetTheLatestVersionNumberOfFirmware = (WARTI_pGetTheLatestVersionNumberOfFirmware)calloc(1, sizeof(WARTI_GetTheLatestVersionNumberOfFirmware));
	pstr->Str.pGetTheLatestVersionNumberOfFirmware->pRet = strdup("OK");
	pstr->Str.pGetTheLatestVersionNumberOfFirmware->pVersion = strdup("20161115");

	
	return pstr;
}


int WIFIAudio_SystemCommand_getMvRemoteUpdate(char *purl)
{
	int ret = 0;
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_switchmode(int mode)
{
	int ret = 0;
	char tempbuff[128];
	
	if((mode < 0) || (mode > 2))
	{
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		sprintf(tempbuff, "uartdfifo.sh setplm %03d &", mode);
		ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	}

	return ret;
}

int WIFIAudio_SystemCommand_setTimeSync(WAGC_pDateTime_t pdatetime)
{
	int ret = 0;
	char tempbuff[128];
	FILE *fp = NULL;
	
	if((pdatetime->Year < 0) || (pdatetime->Year > 3000) 
	|| (pdatetime->Month < 1) || (pdatetime->Month > 12)
	|| (pdatetime->Day < 1) || (pdatetime->Day > 31)
	|| (pdatetime->Hour < 0) || (pdatetime->Hour > 24)
	|| (pdatetime->Minute < 0) || (pdatetime->Minute > 60)
	|| (pdatetime->Second < 0) || (pdatetime->Second > 60)
	|| (pdatetime->Zone < -12) || (pdatetime->Zone > 12))
	{
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		sprintf(tempbuff, "powoff.sh set_utctime \"%04d-%02d-%02d %02d:%02d:%02d\"", \
		pdatetime->Year, pdatetime->Month, pdatetime->Day, \
		pdatetime->Hour, pdatetime->Minute, pdatetime->Second);
		
		WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
		
		memset(tempbuff, 0, sizeof(tempbuff));
		if(pdatetime->Zone > 0)
		{
			sprintf(tempbuff, "powoff.sh set_timezone \"+%d\"", \
			pdatetime->Zone);
		} else {
			sprintf(tempbuff, "powoff.sh set_timezone \"%d\"", \
			pdatetime->Zone);
		}
		
		WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	}


	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getDeviceTimeTopStr(void)
{
	WARTI_pStr pstr = NULL;

	return pstr;
}

int WIFIAudio_SystemCommand_CountOnesNumber(int value)
{
	int num = 0;
	
	while(value)
	{
		value = value & (value - 1);
		num++;
	}
	
	return num;
}

int WIFIAudio_SystemCommand_WhereIsOnes(int value)
{
	int index = 0;
	
	if(0 == value)
	{
		index = -1;
	} else {
		while(1)
		{
			if(1 != (value & 0x01))
			{
				index++;
				value = value >> 1;
			} else {
				break;
			}
		}
	}
	
	return index;
}

WARTI_pStr WIFIAudio_SystemCommand_getAlarmClockTopStr(int num)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

int WIFIAudio_SystemCommand_alarmStop(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc stop");
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_switchmodeTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE;
		
		pstr->Str.pGetPlayerCmdSwitchMode = (WARTI_pGetPlayerCmdSwitchMode)calloc(1, sizeof(WARTI_GetPlayerCmdSwitchMode));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetPlayerCmdSwitchMode->pRet = strdup("OK");
			
			pstr->Str.pGetPlayerCmdSwitchMode->SwitchMode = WIFIAudio_SystemCommand_GetSwitchMode();
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_SelectLanguage(int language)
{
	int ret = 0;
	
	switch(language)
	{
		case 0:
			break;
		case 1:
			ret = WIFIAudio_SystemCommand_SystemShellCommand("setLanguage.sh cn");
			break;
		case 2:
			ret = WIFIAudio_SystemCommand_SystemShellCommand("setLanguage.sh en");
			break;
		case 3:
			break;
			
		case 4:
			break;
			
		default:
			break;
	}

	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getLanguageTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

char* WIFIAudio_SystemCommand_JsonSringAddRetOk(char *pfilename)
{
	struct stat filestat;
	char *pjsonstring = NULL;
	FILE *fp = NULL;
	int ret = -1;
	char *ptmp = NULL;
	
	if(NULL == pfilename)
	{
		
	} else {
		if(NULL == (fp = fopen(pfilename, "r")))
		{
			
		} else {
			stat(pfilename, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + strlen(",\"ret\": \"OK\"")));
			ret = fread(pjsonstring, 1, filestat.st_size, fp);
			fclose(fp);
			fp = NULL;
			
			if(ret >= 0)
			{
				if(ptmp = strstr(pjsonstring, "\"ret\": null"))
				{
					memcpy(ptmp, "\"ret\": \"OK\"", strlen("\"ret\": \"OK\""));
				} else if(ptmp = strstr(pjsonstring, "\"ret\": \"OK\"")) {
				} else {
					ptmp = NULL;
					ptmp = strrchr(pjsonstring, '}');
					if(NULL == ptmp)
					{
						free(pjsonstring);
						pjsonstring = NULL;
					} else {
						strcpy(ptmp, ",\"ret\": \"OK\"}");
					}
				}
			}
		}
	}
	
	return pjsonstring;
}

char* WIFIAudio_SystemCommand_GetCurrentPlaylistTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_GetCurrentPlaylistNum(void)
{
	int num = 0;
	FILE *fp = NULL;
	char temp[20];
	
	fp = popen("mpc playlist | wc -l", "r");
	if(NULL == fp)
	{
		num = 0;
	} else {
		memset(temp, 0, sizeof(temp));
		if(NULL == fgets(temp, sizeof(temp), fp))
		{
			num = 0;
		} else {
			num = atoi(temp);
		}
		pclose(fp);
		fp = NULL;
	}

	return num;
}

int WIFIAudio_SystemCommand_DeletePlaylistMusic(int index)
{
	int ret = 0;
	char temp[128];
	
	if((index < 0) || (index > 30))
	{
		ret = -1;
	} else {
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "creatPlayList deleteonesong %d", index);
		
		ret = WIFIAudio_SystemCommand_SystemShellCommand(temp);
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_RemoveShortcutKeyList(int key)
{
	int ret = 0;
	
	ret = del_collectionSong(key);
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_setrt(int value)
{
	int ret = 0;
	
	ret = set_record_time(value);
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_playrec(void)
{
	int ret = 0;
	
	ret = play_record_audio();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_playurlrec(char *precurl)
{
	int ret = 0;
	
	ret = play_url_audio(precurl);
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetMicarrayInfoTopStr(void)
{
	WARTI_pStr pstr = NULL;
	micarray_Info *pInfo = NULL;
	int ret = 0;

	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_GETMICARRAYINFO;
		pstr->Str.pGetMicarrayInfo = (WARTI_pGetMicarrayInfo)calloc(1, sizeof(WARTI_GetMicarrayInfo));
		if (pstr->Str.pGetMicarrayInfo == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			ret = get_micarray_info(&pInfo);	//配置mic的参数
			
			pstr->Str.pGetMicarrayInfo->pRet = strdup("OK");
			pstr->Str.pGetMicarrayInfo->Mics = pInfo->mics;
			pstr->Str.pGetMicarrayInfo->Mics_channel = pInfo->mics_channel;
			pstr->Str.pGetMicarrayInfo->Refs = pInfo->refs;
			pstr->Str.pGetMicarrayInfo->Refs_channel = pInfo->refs_channel;
			pstr->Str.pGetMicarrayInfo->record_dev = pInfo->record_dev;
			free(pInfo);
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_MicarrayRecord(int value)
{
	int ret = 0;
	
	ret = micarray_record(value);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_MicarrayNormalTopStr(void)
{
	WARTI_pStr pstr = NULL;
	micarray_Info *pInfo = NULL;
	char *pPath;
	char *pIp;
	char URL[300];
	char *pRaw = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_MICARRAYNORMAL;
		pstr->Str.pMicarrayNormal = (WARTI_pMicarrayNormal)calloc(1, sizeof(WARTI_MicarrayNormal));
		if (pstr->Str.pMicarrayNormal == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pPath = micarray_normal_record_path();
			pstr->Str.pMicarrayNormal->pRet = strdup("OK");
			memset(URL, 0, 300);
			pRaw = strdup(strrchr(pPath, '/') + 1);
			sprintf(URL, "misc/%s", pRaw);
			pstr->Str.pMicarrayNormal->pURL = strdup(URL);
			
			free(pRaw);
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_MicarrayMicTopStr(void)
{
	WARTI_pStr pstr = NULL;
	micarray_Info *pInfo = NULL;
	char *pPath;
	char *pIp;
	char URL[300];
	char *pRaw = NULL;
	char *pRaw_head = NULL;
	char *pRaw_tail = NULL;
	int ret;
	int i;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_MICARRAYMIC;
		pstr->Str.pMicarrayMic = (WARTI_pMicarrayMic)calloc(1, sizeof(WARTI_MicarrayMic));
		if (pstr->Str.pMicarrayMic != NULL)
		{
			ret = get_micarray_info(&pInfo);
			if (ret == 0)
			{
				pstr->Str.pMicarrayMic->pRet = strdup("OK");
				pstr->Str.pMicarrayMic->Num = pInfo->mics;
				pstr->Str.pMicarrayMic->ppUrlList = (WARTI_pMicarrayUrlList *)calloc(1, pstr->Str.pMicarrayMic->Num * sizeof(WARTI_pMicarrayUrlList));
				pPath= micarray_mic_record_path();
				
				for (i = 0; i < pstr->Str.pMicarrayMic->Num; i++)
				{
					pstr->Str.pMicarrayMic->ppUrlList[i] = (WARTI_pMicarrayUrlList)calloc(1, sizeof(WARTI_MicarrayUrlList));
					if (pstr->Str.pMicarrayMic->ppUrlList[i] != NULL)
					{
						pstr->Str.pMicarrayMic->ppUrlList[i]->Id = i + 1;
						memset(URL, 0, 300);
						pRaw = strdup(strrchr(pPath, '/') + 1);
						pRaw_tail = strrchr(pRaw, '.');
						*(pRaw_tail - 1) += i;
						sprintf(URL, "misc/%s", pRaw);
						pstr->Str.pMicarrayMic->ppUrlList[i]->pURL = strdup(URL);
						free(pRaw);
					}
				}
			}
		} else {
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_MicarrayRefTopStr(void)
{
	WARTI_pStr pstr = NULL;
	micarray_Info *pInfo = NULL;
	char *pPath;
	char *pIp;
	char URL[300];
	char *pRaw = NULL;
	char *pRaw_head = NULL;
	char *pRaw_tail = NULL;
	int ret;
	int i;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr == NULL)
	{
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_MICARRAYREF;
		pstr->Str.pMicarrayRef = (WARTI_pMicarrayRef)calloc(1, sizeof(WARTI_MicarrayRef));
		if (pstr->Str.pMicarrayRef != NULL)
		{
			ret = get_micarray_info(&pInfo);
			if (ret == 0)
			{
				pstr->Str.pMicarrayRef->pRet = strdup("OK");
				pstr->Str.pMicarrayRef->Num = pInfo->refs;
				pstr->Str.pMicarrayRef->ppUrlList = (WARTI_pMicarrayUrlList *)calloc(1, pstr->Str.pMicarrayRef->Num * sizeof(WARTI_pMicarrayUrlList));
				pPath= micarray_ref_record_path();
				
				for (i = 0; i < pstr->Str.pMicarrayRef->Num; i++)
				{
					pstr->Str.pMicarrayRef->ppUrlList[i] = (WARTI_pMicarrayUrlList)calloc(1, sizeof(WARTI_MicarrayUrlList));
					if (pstr->Str.pMicarrayRef->ppUrlList[i] != NULL)
					{
						pstr->Str.pMicarrayRef->ppUrlList[i]->Id = i + 1;
						memset(URL, 0, 300);
						pRaw = strdup(strrchr(pPath, '/') + 1);
						pRaw_tail = strrchr(pRaw, '.');
						*(pRaw_tail - 1) += i;
						sprintf(URL, "misc/%s", pRaw);
						pstr->Str.pMicarrayRef->ppUrlList[i]->pURL = strdup(URL);
						free(pRaw);
					}
				}
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		} else {
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_MicarrayStop(void)
{
	int ret = 0;
	
	ret = micarray_stop_record();
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_StorageDeviceOnlineStateTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

char* WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_PlayTfCard(int index)
{
	int ret = 0;
	
	return ret;
}

char* WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_PlayUsbDisk(int index)
{
	int ret = 0;

	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr(void)
{
	WARTI_pStr pstr = NULL;

	return pstr;
}

int WIFIAudio_SystemCommand_RemoveSynchronousEx(char *pmac)
{
	int ret = 0;
	char str[128];
	
	if(NULL == pmac)
	{
		ret = -1;
	} else {
		memset(str, 0, 128);
		sprintf(str, "snapmulti.sh disconnect \"%s\" &", pmac);
		
		ret = WIFIAudio_SystemCommand_SystemShellCommand(str);
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_SetSynchronousEx_vol(WAGC_pSetSynchronousEx_t psetsynchronousex)
{
	int ret = 0;
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_startrecord(void)
{
	int ret = 0;

	ret = start_record();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_stoprecord(void)
{
	int ret = 0;
	
	ret = stop_record();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_fixedrecord(void)
{
	int ret = 0;
	
	ret = start_fixed_record();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pURL = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;	
		pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
		if(pstr->Str.pGetRecordFileURL == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pURL = (char *)get_record_file_URL();
			if (pURL != NULL)
			{
				pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
				pstr->Str.pGetRecordFileURL->pURL = strdup(pURL);
				free(pURL);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_vad(WAGC_pxzxAction_t pxzxaction)
{
	int ret = 0;
	
	ret = voice_activity_detection(pxzxaction->Value, pxzxaction->pCardName);
	
	return ret;
}

int WIFIAudio_SystemCommand_GetBluetoothStatusInFile(void)
{
	int status = 0;
	
	return status;
}

int WIFIAudio_SystemCommand_Bluetooth_Search(void)
{
	int ret = 0;
	
	ret = bt_search();
	
	return ret;
}

int WIFIAudio_SystemCommand_LinesOfBtFile(char *pfilename)
{
	int num = 0;
	FILE *fp = NULL;
	char str[128];
	char *ptmp = NULL;
	
	if(NULL == pfilename)
	{
		num = 0;
	} else {
		snprintf(str, sizeof(str), "grep \'&\' %s | wc -l", pfilename);
		fp = popen(str, "r");
		if(NULL == fp)
		{
			num = 0;
		} else {
			memset(str, 0, sizeof(str));
			if(NULL == fgets(str, sizeof(str), fp))
			{
				num = 0;
			} else {
				ptmp = strtok(str, "\r\n");
				if(NULL == ptmp)
				{
					num = 0;
				} else {
					num = atoi(ptmp);
				}
			}
			pclose(fp);
			fp = NULL;
		}
	}
	
	return num;
}

int WIFIAudio_SystemCommand_GetBluetoothStatusInConf(void)
{
	int status = 0;
	
	return status;
}

char* WIFIAudio_SystemCommand_GetBluetoothAddeInFile(void)
{
	char *paddr = NULL;
	
	return paddr;
}

int WIFIAudio_SystemCommand_GetBluetoothStatusInInfo(void)
{
	int status = 0;

	return status;
}

char* WIFIAudio_SystemCommand_GetBluetoothAddrInInfo(void)
{
	char *pAddr = NULL;
	
	return pAddr;
}

WARTI_pStr WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

int WIFIAudio_SystemCommand_Bluetooth_Connect(char *paddress)
{
	int ret = 0;
	
	if(paddress == NULL)
	{
		ret = -1;
	}
	
	ret = bt_connect(paddress);
	
	return ret;
}

int WIFIAudio_SystemCommand_Bluetooth_Disconnect(void)
{
	int ret = 0;
	
	ret = bt_disconnect();
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

int WIFIAudio_SystemCommand_SetSpeechRecognition(int platform)
{
	int ret = 0;
	
	switch(platform)
	{
		case 1:
			ret = WIFIAudio_SystemCommand_SystemShellCommand("/usr/bin/Alexa_iflytek.sh speech");
			break;
			
		case 2:
			ret = WIFIAudio_SystemCommand_SystemShellCommand("/usr/bin/Alexa_iflytek.sh iflytek");
			break;
			
		case 3:
			ret = WIFIAudio_SystemCommand_SystemShellCommand("/usr/bin/Alexa_iflytek.sh Alexa");
			break;
			
		default:
			ret = -1;
			break;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_SetSingleLight(WAGC_pSetSingleLight_t pSetSingleLight)
{
	int ret = 0;
	
	return ret;
}


WARTI_pStr WIFIAudio_SystemCommand_GetAlexaLanguageTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_GETALEXALANGUAGE;
		
		pstr->Str.pGetAlexaLanguage = (WARTI_pGetAlexaLanguage)calloc(1, sizeof(WARTI_GetAlexaLanguage));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetAlexaLanguage->pRet = strdup("OK");
			
			pucistring = OnGetAlexaLanguage();
			pstr->Str.pGetAlexaLanguage->pLanguage = strdup(pucistring);
			free(pucistring);
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_SetAlexaLanguage(char* pLanguage)
{
	int ret = 0;
	char tmp[128];
	
	if(pLanguage == NULL)
	{
		ret = -1;
	} else {
		ret = OnSetAlexaLanguage(pLanguage);
		if (ret != 0)
		{
			ret = -1;
		}
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_UpdataConexant(void)
{
	int ret = 0;

	ret = conexant_update(WIFIAUDIO_CONEXANTSFS_PATHNAME, WIFIAUDIO_CONEXANTBIN_PATHNAME);
	
	return ret;
}

int WIFIAudio_SystemCommand_SetTestMode(int mode)
{
	int ret = 0;
	
	ret = set_test_mode(mode);
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetTestModeTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETTESTMODE;
		pstr->Str.pGetTestMode = (WARTI_pGetTestMode)calloc(1, sizeof(WARTI_GetTestMode));
		if (pstr->Str.pGetTestMode == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetTestMode->pRet = strdup("OK");
			pstr->Str.pGetTestMode->test = get_test_mode();
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_PlayTestMusic(char *purl)
{
	int ret = 0;
	
	ret = play_test_music(purl);
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_SdIsExist(void)
{
	int ret = 0;
	ret = Sd_Is_Exist();
	if(ret != 0)
	{
		ret = -1;
	}
	return ret;
}

int WIFIAudio_SystemCommand_USBIsExist(void)
{
	int ret = 0;
	ret = Usb_Is_Exist();
	if(ret != 0)
	{
		ret = -1;
	}
	return ret;
}

int WIFIAudio_SystemCommand_StopTestMusic(void)
{
	int ret = 0;
	
	ret = stop_test_music();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_LogCaptureEnableTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pURL = NULL;
	char *pPath = NULL;
	char URL[300];
	char *pRaw = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_LOGCAPTUREENABLE;
		pstr->Str.pLogCaptureEnable = (WARTI_pLogCaptureEnable)calloc(1, sizeof(WARTI_LogCaptureEnable));
		if (pstr->Str.pLogCaptureEnable == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pPath = interaction_log_capture_enable();
			if (pPath != NULL)
			{
				pstr->Str.pLogCaptureEnable->pRet = strdup("OK");
				
				memset(URL, 0, 300);
				pRaw = strdup(strrchr(pPath, '/') + 1);
				sprintf(URL, "misc/%s", pRaw);
				pstr->Str.pLogCaptureEnable->pURL = strdup(URL);
				free(pRaw);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_LogCaptureDisable(void)
{
	int ret = 0;
	
	ret = interaction_log_capture_disable();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	return pstr;
}

int WIFIAudio_SystemCommand_ChangeNetworkConnect(void)
{
	int ret = 0;
	
	WIFIAudio_SystemCommand_SystemShellCommand("changenc &");
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_normalrecord(int second)
{
	int ret = 0;
	
	ret = dot_normal_record(second);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pURL;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);

	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if (pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;
		pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
		if(pstr == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pURL = dot_normal_record_web_path();
			if (pURL != NULL)
			{
				pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
				pstr->Str.pGetRecordFileURL->pURL = strdup(pURL);
				free(pURL);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_micrecord(int second)
{
	int ret = 0;

	ret = dot_mic_record(second);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pURL;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;	
		pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
		if(pstr->Str.pGetRecordFileURL == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pURL = dot_mic_record_web_path();
			if (pURL != NULL)
			{
				pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
				pstr->Str.pGetRecordFileURL->pURL = strdup(pURL);
				free(pURL);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_refrecord(int second)
{
	int ret = 0;
	
	ret = dot_ref_record(second);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pURL;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;	
		pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
		if(pstr->Str.pGetRecordFileURL == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pURL = dot_ref_record_web_path();
			if (pURL != NULL)
			{
				pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
				pstr->Str.pGetRecordFileURL->pURL = strdup(pURL);
				free(pURL);
			} else {
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_stopModeRecord(void)
{
	int ret = 0;
	
	ret = dot_stop_record();
	if (ret != 0)
	{
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_GetWifiUpgradeDownloadProgress(void)
{
	int Progress = 0;
	char *pTmp_Download = NULL;
	
	if (access(WIFIAUDIO_WIFI_UP_PROGRESS, F_OK) != 0)
	{
		creat(WIFIAUDIO_WIFI_UP_PROGRESS, S_IRWXU);
	}
	
	pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFI_UP_PROGRESS, 10);
	if (pTmp_Download != NULL)
	{
			Progress = atoi(pTmp_Download);
			if (Progress > 100)
			{
				Progress = 100;
			}
			
			free(pTmp_Download);
			pTmp_Download = NULL;
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetWifiUpgradeBurnProgress(void)
{
	int Progress = 0;
	
	Progress = get_wifi_bt_burn_progress();
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_GetDownloadProgressTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int State = 0;
	int Download = 0;
	int Upgrade = 0;
	
	Upgrade = WIFIAudio_SystemCommand_GetWifiUpgradeBurnProgress();
	if (Upgrade == -1)
	{
		Upgrade = 0;
	}
	Download = WIFIAudio_SystemCommand_GetWifiUpgradeDownloadProgress();
	State = get_wifi_bt_update_flage();
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETDOWNLOADPROGRESS;
			
		pstr->Str.pGetRemoteUpdateProgress = (WARTI_pGetRemoteUpdateProgress)calloc(1, sizeof(WARTI_GetRemoteUpdateProgress));
		if(pstr->Str.pGetRemoteUpdateProgress == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetRemoteUpdateProgress->pRet = strdup("OK");
			pstr->Str.pGetRemoteUpdateProgress->State = State;
			pstr->Str.pGetRemoteUpdateProgress->Download = Download;
			pstr->Str.pGetRemoteUpdateProgress->Upgrade = Upgrade;
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_GetBluetoothUpgradeDownloadProgress(void)
{
	int Progress = 0;
	char *pTmp_Download = NULL;
	
	if (access(WIFIAUDIO_BT_UP_PROGRESS, F_OK) != 0)
	{
		creat(WIFIAUDIO_BT_UP_PROGRESS, S_IRWXU);
	}
	
	pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_BT_UP_PROGRESS, 10);
	if (pTmp_Download != NULL)
	{
			Progress = atoi(pTmp_Download);
			if (Progress > 100)
			{
				Progress = 100;
			}
			
			free(pTmp_Download);
			pTmp_Download = NULL;
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothUpgradeBurnProgress(void)
{
	int Progress = 0;
	
	Progress = get_wifi_bt_burn_progress();
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int Download = 0;
	int Upgrade = 0;
	int State = 0;
	
	Upgrade = WIFIAudio_SystemCommand_GetBluetoothUpgradeBurnProgress();
	if (Upgrade == -1)
	{
		Upgrade = 0;
	}
	Download = WIFIAudio_SystemCommand_GetBluetoothUpgradeDownloadProgress();
	State = get_wifi_bt_update_flage();
		
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(pstr != NULL)
	{
		pstr->type = WARTI_STR_TYPE_GETDOWNLOADPROGRESS;
		
		pstr->Str.pGetRemoteUpdateProgress = (WARTI_pGetRemoteUpdateProgress)calloc(1, sizeof(WARTI_GetRemoteUpdateProgress));
		if(pstr->Str.pGetRemoteUpdateProgress == NULL)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetRemoteUpdateProgress->pRet = strdup("OK");
			pstr->Str.pGetRemoteUpdateProgress->State = State;
			pstr->Str.pGetRemoteUpdateProgress->Download = Download;
			pstr->Str.pGetRemoteUpdateProgress->Upgrade = Upgrade;
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_DisconnectWiFi(void)
{
	int ret = 0;
	
	ret = disconnect_wifi();
	
	return ret;
}

int WIFIAudio_SystemCommand_PowerOnAutoPlay(int enable)
{
	int ret = 0;

	ret = startingup_play_state(enable);
	
	return ret;
}

int WIFIAudio_SystemCommand_RemoveAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	int ret = 0;
	char tempbuff[32];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "alarm rm %d &", pAlarmAndPlan->Num);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	
	return ret;
}

int WIFIAudio_SystemCommand_EnableAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	int ret = 0;
	char tempbuff[32];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "alarm create %d %d &", pAlarmAndPlan->Num, (pAlarmAndPlan->Enable > 1)?1:pAlarmAndPlan->Enable);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	
	return ret;
}

int WIFIAudio_SystemCommand_RemovePlayPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	int ret = 0;
	char tempbuff[32];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "alarm rmmusic %d &", pAlarmAndPlan->Num);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	
	return ret;
}

char* WIFIAudio_SystemCommand_GetAlarmClockAndPlayPlanTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

char* WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_EnablePlayMusicAsPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	int ret = 0;
	char tempbuff[32];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "alarm markmusic %d %d &", pAlarmAndPlan->Num, (pAlarmAndPlan->Enable > 1)?1:pAlarmAndPlan->Enable);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	
	return ret;
}

char* WIFIAudio_SystemCommand_GetPlanMixedListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth(char *purl)
{
	int ret = 0;
	
	ret = app_update_noparmeter(purl);
	
	return ret;
}

int WIFIAudio_SystemCommand_GetWiFiDownloadProgress(void)
{
	int Progress = 0;
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetWiFiBurnProgress(void)
{
	int Progress = 0;
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothDownloadProgress(void)
{
	int Progress = 0;
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothBurnProgress(void)
{
	int Progress = 0;
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int State = 0;
	int Progress = 0;
	
	State = get_wifi_bt_update_flage();
	
	switch(State)
	{
		case 4:
		case 5:
			Progress = get_wifi_bt_burn_progress();
			if (Progress == -1)
			{
				Progress = 0;
			}
			break;
			
		case 6:
		case 7:
			Progress = get_wifi_bt_download_progress();
			if (Progress == -1)
			{
				Progress = 0;
			}
			break;
			
		default:
			Progress = 0;
			break;
	}
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
	} else {
		pstr->type = WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE;
		
		pstr->Str.pGetUpgradeState = (WARTI_pGetUpgradeState)calloc(1, sizeof(WARTI_GetUpgradeState));
		if(NULL == pstr)
		{
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetUpgradeState->pRet = strdup("OK");
			pstr->Str.pGetUpgradeState->State = State;
			pstr->Str.pGetUpgradeState->Progress = Progress;
		}
	}
	
	return pstr;
}


char* WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString(int key)
{
	char *pjsonstring = NULL;
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_EnableAutoTimeSync(int enable)
{
	int ret = 0;
	
	if(enable == 0)
	{
		ret = WIFIAudio_SystemCommand_SystemShellCommand("powoff.sh setutc disable &");
	} else {
		ret = WIFIAudio_SystemCommand_SystemShellCommand("powoff.sh setutc enable &");
	}
	
	return ret;
}

