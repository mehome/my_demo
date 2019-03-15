#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <json/json.h> 
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_SystemCommand.h"

//以可读的方式打开，可读就当做文件存在
//如果不可读那也不要升级了
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


//自己封装一个system的函数,主要是板端直接用system好像会出问题
int WIFIAudio_SystemCommand_SystemShellCommand(char *pstring)
{
	int ret = 0;
	FILE *fp = NULL;
	
	if(NULL == pstring)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fp = popen(pstring, "r");
		if(NULL == fp)
		{
			ret = -1;
		} else {
			pclose(fp);
			fp = NULL;//不关心执行之后显示什么数据
		}
	}
	
	return ret;
}


char* WIFIAudio_SystemCommand_GetOneLineFromFile(char *pPathName, int Len)
{
	FILE *fp = NULL;
	char *pStr = NULL;
	
	if((NULL == pPathName) || (Len < 1))
	{
		pStr = NULL;
	} else {
		fp = fopen(pPathName, "r");
		if(NULL == fp)
		{
			pStr = NULL;
		} else {
			pStr = (char *)calloc(1, Len + 1);
			if(NULL == pStr)
			{
				pStr = NULL;
			} else {
				memset(pStr, 0, Len + 1);
				if(NULL == fgets(pStr, Len, fp))
				{
					free(pStr);
					pStr = NULL;
				}
			}
			
			fclose(fp);
			fp = NULL; 
		}
	}
	
	return pStr;
}



//系统功能实现函数
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
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pret = NULL;
	} else {
		if(NULL != strstr(pstring,"\\x"))
		{
			//含有中文字符
			memset(ssid, 0, sizeof(ssid));
			strcpy(ssid, "0x");
			len = strlen(pstring);//这样下面for循环不用一直strlen
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

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#else
	system("iw dev wlan0 scan | grep -E \"Au|SSID|signal\" > /tmp/ApList.txt");
	//由于要先计算有多少个AP热点，所以先重定向写入文件当中
	//这边没有后台运行，应该是等待执行完才会继续往下走
	
	//利用grep配合正则表达式当中的点.符号排除\r\n之外的字符
	//用wc统计有多少个可见字符的SSID
	fpopen = popen("cat /tmp/ApList.txt | grep \"SSID: .\" | wc -l", "r");
	if(NULL == fpopen)
	{
		pstr = NULL;
	} else {
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
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_WLANGETAPLIST;
			
			pstr->Str.pWLANGetAPlist = (WARTI_pWLANGetAPlist)calloc(1, sizeof(WARTI_WLANGetAPlist));
			if(NULL == pstr->Str.pWLANGetAPlist)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				pstr->Str.pWLANGetAPlist->pRet = strdup("OK");
				
				pstr->Str.pWLANGetAPlist->Res = num;
				pstr->Str.pWLANGetAPlist->pAplist = \
				(WARTI_pAccessPoint*)calloc(1, (pstr->Str.pWLANGetAPlist->Res) \
				* sizeof(WARTI_pAccessPoint));
				//分配几个指向WARTI_pLocalMusicFile的指针
				if(NULL == pstr->Str.pWLANGetAPlist->pAplist)
				{
#ifdef WIFIAUDIO_DEBUG
					syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					fp = fopen("/tmp/ApList.txt", "r");
					if(NULL == fp)
					{
#ifdef WIFIAUDIO_DEBUG
						syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
						WIFIAudio_RealTimeInterface_pStrFree(&pstr);
						pstr = NULL;
					} else {
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
									} else {
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
#ifdef WIFIAUDIO_DEBUG
								syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
								WIFIAudio_RealTimeInterface_pStrFree(&pstr);
								pstr = NULL;
								break;
							} else {
								//先处理信号强度信息
								sscanf(strstr(tempbuff, "signal: "), "signal: %f dBm", &signal);
								//这边取出来的是负值的功率
								//假设-42dbm是100%，-90是0%
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = (int)(signal+142);
								if((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI >= 100)
								{
									//大于-42dbm，都设为强度100
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = 100;
								} else if((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI > 52) {
									//小于-42dbm，将52-100的部分做线性比例
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI = (int)(((pstr->Str.pWLANGetAPlist->pAplist)[i]->RSSI - 52) / 0.48);
								} else {
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
								} else {
									//没有读到加密状态，这个时候已经把下一次的singal读进tempbuff了
									(pstr->Str.pWLANGetAPlist->pAplist)[i]->pAuth = strdup("OPEN");
								}
								
								//MAC地址不管
								//信道不管，直接赋值为1
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->Channel = 1;
								//加密类型不管
								//直接给1
								(pstr->Str.pWLANGetAPlist->pAplist)[i]->Extch = 1;
							}
						}	
						fclose(fp);
						fp = NULL;
					}
				}
			}
		}
	}
#endif

	return pstr;
}

int WIFIAudio_SystemCommand_wlanConnectAp(WAGC_pConnectAp_t pconnectap)
{
	int ret = 0;
	//char str[128];
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;

	if(NULL == pconnectap)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP;
					pcmdparm->Parameter.pConnectAp = (WAFC_pConnectAp)calloc(1, sizeof(WAFC_ConnectAp));
					if(NULL != pcmdparm->Parameter.pConnectAp)
					{
						pcmdparm->Parameter.pConnectAp->pSSID = strdup(pconnectap->pSSID);
						pcmdparm->Parameter.pConnectAp->Encode = pconnectap->Encty;
						pcmdparm->Parameter.pConnectAp->pPassword = strdup(pconnectap->pPassword);
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}

	
	return ret;
}

char * WIFIAudio_SystemCommand_GetConnectWifiName(void)
{
	char status1[128];
	char status2[128];
	FILE *fp = NULL;
	char *psubstr = NULL;
	char *pretname = NULL;
	
	
	return pretname;
}

WARTI_pStr WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char *pucistring = NULL;
	int sem_id = 0;//信号量ID

	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT;
		
		pstr->Str.pGetCurrentWirelessConnect = (WARTI_pGetCurrentWirelessConnect\
		)calloc(1, sizeof(WARTI_GetCurrentWirelessConnect));
		
		if(NULL == pstr->Str.pGetCurrentWirelessConnect)
		{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetCurrentWirelessConnect->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wificonnectstatus");
			if(NULL != pucistring)
			{
				pstr->Str.pGetCurrentWirelessConnect->Connect = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
				
				if(3 == pstr->Str.pGetCurrentWirelessConnect->Connect)
				{
					char *pssid = NULL;
					pssid = WIFIAudio_SystemCommand_GetConnectWifiName();
					if(NULL == pssid)
					{
						pstr->Str.pGetCurrentWirelessConnect->pSSID = NULL;
						//pstr->Str.pGetCurrentWirelessConnect->Connect = 4;
						//这边还是不要改变它的状态了
						//连上就连上没，没查到SSID就算了
					} else {
						pstr->Str.pGetCurrentWirelessConnect->pSSID = pssid;
					}
				}
			}

			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_GetLocalWifiIp(struct in_addr *pip)
{
	int ret = 0;
	FILE *fp = NULL;
	char tempbuff[128];
	char *plocalip = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	if(NULL == pip)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.ip");
		if(NULL != pucistring)
		{
			inet_aton(pucistring, pip);
			
			WIFIAudio_UciConfig_FreeValueString(&pucistring);
			pucistring = NULL;
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(!strcmp(inet_ntoa(*pip), "0.0.0.0"))
		{

			WIFIAudio_Semaphore_Operation_P(sem_id);
		
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.lan_ip");
			if(NULL != pucistring)
			{
				inet_aton(pucistring, pip);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}
	
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
	int i;
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	FILE *fp = NULL;
	
	//开始分配空间
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETDEVICEINFO;
		
		pstr->Str.pGetDeviceInfo = (WARTI_pGetDeviceInfo)calloc(1, sizeof(WARTI_GetDeviceInfo));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetDeviceInfo->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.language");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pLanguage = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.project");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pProject = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.language_support");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pLanguageSupport = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.function_support");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pFunctionSupport = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.key_num");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->KeyNum = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.audioname");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pDeviceName = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.ap_ssid1");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pSSID = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.sw_ver");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pBtVersion = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.firmware");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pFirmware = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.mac_addr");
			if(NULL != pucistring)
			{
				//pstr->Str.pGetDeviceInfo->pMAC = WIFIAudio_SystemCommand_MacChange(pucistring);
				pstr->Str.pGetDeviceInfo->pMAC = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wifiaudio_uuid");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pUUID = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.release");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pRelease = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.power");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->BatteryPercent = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.server_conex_ver");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pConexantVersion = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.charge");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceInfo->pPowerSupply = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
			
			pstr->Str.pGetDeviceInfo->pESSID = WIFIAudio_SystemCommand_GetConnectWifiName();
			
			
			WIFIAudio_SystemCommand_GetLocalWifiIp(&(pstr->Str.pGetDeviceInfo->Apcli0));
			
			pstr->Str.pGetDeviceInfo->RSSI = 98;
		}
	}

	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigure(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//开始分配空间
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETONETOUCHCONFIGURE;
		
		pstr->Str.pGetOneTouchConfigure = (WARTI_pGetOneTouchConfigure)calloc(1, sizeof(WARTI_GetOneTouchConfigure));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetOneTouchConfigure->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);

			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.audiodetectstatus");
			if(NULL != pucistring)
			{
				pstr->Str.pGetOneTouchConfigure->State = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_setOneTouchConfigure(int value)
{
	int ret = 0;
	int sem_id = 0;//信号量ID
	char tempbuff[16];

	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "%d", value);
	ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.audiodetectstatus", tempbuff);
	
	WIFIAudio_Semaphore_Operation_V(sem_id);

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//开始分配空间
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME;
		
		pstr->Str.pGetOneTouchConfigureSetDeviceName = (WARTI_pGetOneTouchConfigureSetDeviceName)calloc(1, sizeof(WARTI_GetOneTouchConfigureSetDeviceName));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetOneTouchConfigureSetDeviceName->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);

			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.audionamestatus");
			if(NULL != pucistring)
			{
				pstr->Str.pGetOneTouchConfigureSetDeviceName->State = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	return pstr;
}

int WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName(int value)
{
	int ret = 0;
	int sem_id = 0;//信号量ID
	char tempbuff[16];

	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "%d", value);
	ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.audionamestatus", tempbuff);
	
	WIFIAudio_Semaphore_Operation_V(sem_id);

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonStatus(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//开始分配空间
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETLOGINAMAZONSTATUS;
		
		pstr->Str.pGetLoginAmazonStatus = (WARTI_pGetLoginAmazonStatus)calloc(1, sizeof(WARTI_GetLoginAmazonStatus));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetLoginAmazonStatus->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_loginStatus");
			if(NULL != pucistring)
			{
				pstr->Str.pGetLoginAmazonStatus->State = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

/*
WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonNeedInfo(void)
{
	WARTI_pStr pstr = NULL;
	char tempbuff[256];
	FILE *fp = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//在/etc/config/xzxwifiaudio当中生成需要的相应参数
	WIFIAudio_SystemCommand_SystemShellCommand("AlexaClient generateParameter");
	
	//system("AlexaClient generateParameter > /dev/null");
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETLOGINAMAZONNEEDINFO;
	
	pstr->Str.pGetLoginAmazonNeedInfo = (WARTI_pGetLoginAmazonNeedInfo)calloc(1, sizeof(WARTI_GetLoginAmazonNeedInfo));
	pstr->Str.pGetLoginAmazonNeedInfo->pRet = strdup("OK");
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_ProductId");
	if(NULL != pucistring)
	{
		pstr->Str.pGetLoginAmazonNeedInfo->pProductId = strdup(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_Dsn");
	if(NULL != pucistring)
	{
		pstr->Str.pGetLoginAmazonNeedInfo->pDsn = strdup(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_CodeChallenge");
	if(NULL != pucistring)
	{
		pstr->Str.pGetLoginAmazonNeedInfo->pCodeChallenge = strdup(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_CodeChallengeMethod");
	if(NULL != pucistring)
	{
		pstr->Str.pGetLoginAmazonNeedInfo->pCodeChallengeMethod = strdup(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	return pstr;
}
*/

int WIFIAudio_SystemCommand_LogoutAmazon(void)
{
	int ret = 0;
	
	WIFIAudio_SystemCommand_SystemShellCommand("amazon_logout.sh");
	
	return ret;
}

int WIFIAudio_SystemCommand_GetMpcVolume(void)
{
	int volume = 0;
	FILE *fp = NULL;
	char tempbuff[128];
	char *ptmp = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.volume");
	if(NULL != pucistring)
	{
		volume = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	return volume;
}

int WIFIAudio_SystemCommand_GetMute(void)
{
	int mute = 0;
	int i = 0;
	FILE *fp = NULL;
	char tempbuff[128];
	
	if(0 != WIFIAudio_SystemCommand_GetMpcVolume())
	{
		mute = 0;
	} else {
		mute = 1;
	}
	
	return mute;
}

//获取输入源
int WIFIAudio_SystemCommand_GetSwitchMode(void)
{
	int mode = 0;
	
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
	if(NULL != pucistring)
	{
		mode = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	return mode;
}

//获取主从音响标志
int WIFIAudio_SystemCommand_GetSlaveflag(void)
{
	int slave = 0;
	int ret = 0;
	int sem_id = 0;//信号量ID
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	char tempbuff[128];
	struct stat filestat;//用来读取文件大小
	
	stat(WIFIAUDIO_POSTSYNCHRONOUSINFOEX_FILENAME, &filestat);
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_POSTSYNCHRONOUSINFOEX_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	if(NULL == (fp = fopen(WIFIAUDIO_POSTSYNCHRONOUSINFOEX_FILENAME, "r")))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		slave = 0;//不存在这个文件，判断为普通音箱
	} else {
		pjsonstring = (char *)calloc(1, (filestat.st_size) * sizeof(char));
		if(NULL == pjsonstring)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
			slave = 0;
		} else {
			ret =fread(pjsonstring, sizeof(char), filestat.st_size, fp);
		}
		fclose(fp);
		fp = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	if(ret > 0)
	{
		//移除旧同步之后，这边使用扩展的解析，不知道会不会出问题，还需要测试一下
		pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX, pjsonstring);
		free(pjsonstring);
		pjsonstring = NULL;
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
			slave = 0;
		} else {
			slave = pstr->Str.pPostSynchronousInfoEx->Slave;
			
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		}
	}
	
	return slave;
}

//获取是否外接设备，现在只要有一个设备都算
int WIFIAudio_SystemCommand_GetLocalListFlag(void)
{
	int flag = 0;
	int tf = 0;
	int usb = 0;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);

	//获取tf
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.tf_status");
	if(NULL != pucistring)
	{
		tf = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	//获取usb
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_status");
	if(NULL != pucistring)
	{
		usb = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	flag = tf | usb;
	
	return flag;
}

WARTI_pStr WIFIAudio_SystemCommand_GetPlayerStatusTopStr(void)
{
	int i;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char status1[512];//原本这边给128字节，遇到URL长一点的，下面状态也会获取错误
	char status2[128];
	char status3[128];
	char tempbuff[128];
	char curpos[10];
	char totlen[10];
	char plicount[10];
	char plicurr[10];
	char *psubstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//先将状态全部获取出来
	memset(status1, 0, sizeof(status1));
	memset(status2, 0, sizeof(status2));
	memset(status3, 0, sizeof(status3));
	
	fp = popen("mpc status", "r");
	if(NULL == fp)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pstr = NULL;
	} else {
		//我们知道获取的状态就是三行，所以获取三次，第一次获取如果为空，那肯定错
		if(NULL == fgets(status1, sizeof(status1), fp))
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pclose(fp);
			fp = NULL;
			pstr = NULL;
		} else {
			if(NULL != fgets(status2, sizeof(status2), fp))
			{
				fgets(status3, sizeof(status3), fp);
			}
			pclose(fp);
			fp = NULL;
			
			//开始分配空间
			pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			} else {
				pstr->type = WARTI_STR_TYPE_GETPLAYERSTATUS;
				
				pstr->Str.pGetPlayerStatus = (WARTI_pGetPlayerStatus)calloc(1, sizeof(WARTI_GetPlayerStatus));
				if(NULL == pstr->Str.pGetPlayerStatus)
				{
#ifdef WIFIAUDIO_DEBUG
					syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pstr->Str.pGetPlayerStatus->pRet = strdup("OK");
					
					//当前播放状态需要优先确认,才能知道是否有在播放,刚才获取的状态有几行
					if(strstr(status2,"[playing]"))
					{
						pstr->Str.pGetPlayerStatus->pStatus = strdup("play");
					} else if(strstr(status2,"[paused]")) {
						pstr->Str.pGetPlayerStatus->pStatus = strdup("pause");
					} else {
						pstr->Str.pGetPlayerStatus->pStatus = strdup("stop");
					}
					
					//因为不同的播放状态所返回的播放状态行数不一样，所以做不一样处理
					if(!strcmp(pstr->Str.pGetPlayerStatus->pStatus, "stop"))
					{
						//获取循环状态
						if(strstr(status1,"repeat: off") && strstr(status1,"random: off") && strstr(status1,"single: off"))
						{
							pstr->Str.pGetPlayerStatus->SW = 0;
						} else if(strstr(status1,"repeat: on") && strstr(status1,"single: on")) {//这边就算随机打开了，也是单曲循环
							pstr->Str.pGetPlayerStatus->SW = 1;
						} else if(strstr(status1,"random: on") && strstr(status1,"single: off")) {
							pstr->Str.pGetPlayerStatus->SW = 2;
						} else if(strstr(status1,"repeat: on") && strstr(status1,"random: off") && strstr(status1,"single: off")) {
							pstr->Str.pGetPlayerStatus->SW = 3;
						}
						
						//获取音量
						pstr->Str.pGetPlayerStatus->Vol = WIFIAudio_SystemCommand_GetMpcVolume();
					} else if((!strcmp(pstr->Str.pGetPlayerStatus->pStatus, "play")) \
					|| (!strcmp(pstr->Str.pGetPlayerStatus->pStatus, "pause"))) {
						//获取循环状态
						if(strstr(status3,"repeat: off") && strstr(status3,"random: off") && strstr(status3,"single: off"))
						{
							pstr->Str.pGetPlayerStatus->SW = 0;
						} else if(strstr(status3,"repeat: on") && strstr(status3,"single: on")) {	//这边就算随机打开了，也是单曲循环
							pstr->Str.pGetPlayerStatus->SW = 1;
						} else if(strstr(status3,"random: on") && strstr(status3,"single: off")) {
							pstr->Str.pGetPlayerStatus->SW = 2;
						} else if(strstr(status3,"repeat: on") && strstr(status3,"random: off") && strstr(status3,"single: off")) {
							pstr->Str.pGetPlayerStatus->SW = 3;
						}
						
						//获取当前位置和总长度
						memset(tempbuff, 0, sizeof(tempbuff));
						memset(curpos, 0,sizeof(curpos));
						memset(totlen, 0,sizeof(totlen));
						strcpy(tempbuff, status2);
						
						strtok(tempbuff, " ");
						strtok(NULL, " ");
						
						psubstr = strtok(NULL, " ");
						strcpy(curpos, strtok(psubstr, "/"));
						strcpy(totlen, strtok(NULL, "/"));
						
						pstr->Str.pGetPlayerStatus->CurPos = atoi(strtok(curpos, ":"));
						while(NULL != (psubstr = strtok(NULL, ":")))
						{
							pstr->Str.pGetPlayerStatus->CurPos *= 60; 
							pstr->Str.pGetPlayerStatus->CurPos += atoi(psubstr);
						}
						
						pstr->Str.pGetPlayerStatus->TotLen = atoi(strtok(totlen, ":"));
						while(NULL != (psubstr = strtok(NULL, ":")))
						{
							pstr->Str.pGetPlayerStatus->TotLen *= 60; 
							pstr->Str.pGetPlayerStatus->TotLen += atoi(psubstr);
						}
						
						
						//获取当前播放列表当中index和播放列表的总条数
						memset(tempbuff, 0, sizeof(tempbuff));
						strcpy(tempbuff, status2);
						strtok(tempbuff, " ");
						
						psubstr = strtok(NULL, " ");
						
						pstr->Str.pGetPlayerStatus->PLicurr = atoi(strtok(psubstr, "#/"));
						pstr->Str.pGetPlayerStatus->PLiCount = atoi(strtok(NULL, "#/"));
						
						sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
						WIFIAudio_Semaphore_Operation_P(sem_id);
						
						//获取标题
						pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_title");
						if(NULL != pucistring)
						{
							pstr->Str.pGetPlayerStatus->pTitle = strdup(pucistring);
							
							WIFIAudio_UciConfig_FreeValueString(&pucistring);
							pucistring = NULL;
						}
						
						//获取艺术家
						pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_artist");
						if(NULL != pucistring)
						{
							pstr->Str.pGetPlayerStatus->pArtist = strdup(pucistring);
							
							WIFIAudio_UciConfig_FreeValueString(&pucistring);
							pucistring = NULL;
						}
						
						//获取URI
						pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_contentURL");
						if(NULL != pucistring)
						{
							pstr->Str.pGetPlayerStatus->pURI = strdup(pucistring);
							
							WIFIAudio_UciConfig_FreeValueString(&pucistring);
							pucistring = NULL;
						}
						
						//获取封面
						pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_coverURL");
						if(NULL != pucistring)
						{
							pstr->Str.pGetPlayerStatus->pCover = strdup(pucistring);
							
							WIFIAudio_UciConfig_FreeValueString(&pucistring);
							pucistring = NULL;
						}
						
						//获取专辑名
						pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_album");
						if(NULL != pucistring)
						{
							pstr->Str.pGetPlayerStatus->pAlbum = strdup(pucistring);
							
							WIFIAudio_UciConfig_FreeValueString(&pucistring);
							pucistring = NULL;
						}
						
						WIFIAudio_Semaphore_Operation_V(sem_id);
						
						//获取年份,如果文件本身没有年份的话就为0
						fp = popen("mpc current -f %date%", "r");
						if(NULL != fp)
						{
							memset(tempbuff, 0, sizeof(tempbuff));
							fgets(tempbuff, sizeof(tempbuff), fp);
							
							pclose(fp);
							fp = NULL;
							
							if(strlen(tempbuff)>0)
							{
								(tempbuff)[strlen(tempbuff) - 1] = 0;
								//打印过信息发现,最后一个是0x0A，将回车替换成\0
							}
							pstr->Str.pGetPlayerStatus->Year = atoi(tempbuff);
						}
						
						
						//获取轨道号，没有就为0
						fp = popen("mpc current -f %track%", "r");
						if(NULL != fp)
						{
							memset(tempbuff, 0, sizeof(tempbuff));
							fgets(tempbuff, sizeof(tempbuff), fp);
							
							pclose(fp);
							fp = NULL;
							
							if(strlen(tempbuff)>0)
							{
								(tempbuff)[strlen(tempbuff) - 1] = 0;
								//打印过信息发现,最后一个是0x0A，将回车替换成\0
							}
							pstr->Str.pGetPlayerStatus->Track = atoi(tempbuff);
						}
						
						//获取风格
						fp = popen("mpc current -f %genre%", "r");
						if(NULL != fp)
						{
							memset(tempbuff, 0, sizeof(tempbuff));
							fgets(tempbuff, sizeof(tempbuff), fp);
							
							pclose(fp);
							fp = NULL;
							
							if(strlen(tempbuff)>0)
							{
								(tempbuff)[strlen(tempbuff) - 1] = 0;
								//打印过信息发现,最后一个是0x0A，将回车替换成\0
							}
							pstr->Str.pGetPlayerStatus->pGenre = strdup(tempbuff);
						}
						
						//获取音量
						pstr->Str.pGetPlayerStatus->Vol = WIFIAudio_SystemCommand_GetMpcVolume();
						
					}
					
					//获取uuid，uuid要放在外头获取
					sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
					WIFIAudio_Semaphore_Operation_P(sem_id);
					pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wifiaudio_uuid");
					if(NULL != pucistring)
					{
						pstr->Str.pGetPlayerStatus->pUUID = strdup(pucistring);
						
						WIFIAudio_UciConfig_FreeValueString(&pucistring);
						pucistring = NULL;
					}
					WIFIAudio_Semaphore_Operation_V(sem_id);
					//获取声道
					pstr->Str.pGetPlayerStatus->NodeType = WIFIAudio_SystemCommand_getPlayerCmd_slave_channel();
					
					//获取静音
					pstr->Str.pGetPlayerStatus->Mute = WIFIAudio_SystemCommand_GetMute();
					
					//获取输入源
					pstr->Str.pGetPlayerStatus->SourceMode = WIFIAudio_SystemCommand_GetSwitchMode();
					
					pstr->Str.pGetPlayerStatus->LocalListFlag = WIFIAudio_SystemCommand_GetLocalListFlag();
					
					//下面这些暂时无法获取，虽然申请内存的时候已经清空了，但是还是要标明一下，免得以后忘了
					pstr->Str.pGetPlayerStatus->MainMode = WIFIAudio_SystemCommand_GetSlaveflag();//主子音箱
					pstr->Str.pGetPlayerStatus->Mode = 0;//播放模式
				}
			}
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_playlist(int num)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX;
				pcmdparm->Parameter.pPlayIndex = (WAFC_pPlayIndex)calloc(1, sizeof(WAFC_PlayIndex));
				if(NULL != pcmdparm->Parameter.pPlayIndex)
				{
					pcmdparm->Parameter.pPlayIndex->Index = num;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_pause(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc pause");

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_play(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc play");

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_prev(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc prev");

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_next(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc next");

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_seek(int num)
{
	int ret = 0;
	char str[32];
	
	memset(str, 0, 32);
	sprintf(str, "mpc seek %d", num);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(str);

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_stop(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc stop");

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_vol(int num)
{
	int ret = 0;
	char str[32];
	
	memset(str, 0, 32);
	sprintf(str, "mpc volume %d", num);
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(str);

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_volTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETPLAYERCMDVOLUME;
	
	pstr->Str.pGetPlayerCmdVolume = (WARTI_pGetPlayerCmdVolume)calloc(1, sizeof(WARTI_GetPlayerCmdVolume));
	pstr->Str.pGetPlayerCmdVolume->pRet = strdup("OK");
	pstr->Str.pGetPlayerCmdVolume->Volume = WIFIAudio_SystemCommand_GetMpcVolume(); 

	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_mute(int flag)
{
	int ret = 0;
	int i = 0;
	FILE *fp = NULL;
	char tempbuff[128];
	
	switch(flag)
	{
		case 0:
			fp = popen("amixer cset numid=41 121,121", "r");
			if(NULL != fp)
			{
				for(i = 0; i < 3; i++)
				{
					memset(tempbuff, 0, sizeof(tempbuff));
					fgets(tempbuff, sizeof(tempbuff), fp);
				}//以为执行完，我们说需要确认的内容在第三行，所以我们这边循环读取三次
				
				if(strstr(tempbuff,"values=121,121"))
				{
					ret = 0;//静音关，执行成功
				} else {
					ret = -1;//静音关执行失败
				}
				
				pclose(fp);
				fp = NULL;
			} else {
				ret = -1;//静音关执行失败
			}
			break;
		
		case 1:
			fp = popen("amixer cset numid=41 0,0", "r");
			if(NULL != fp)
			{
				for(i = 0; i < 3; i++)
				{
					memset(tempbuff, 0, sizeof(tempbuff));
					fgets(tempbuff, sizeof(tempbuff), fp);
				}//以为执行完，我们说需要确认的内容在第三行，所以我们这边循环读取三次
				
				if(strstr(tempbuff,"values=0,0"))
				{
					ret = 0;//静音关，执行成功
				} else {
					ret = -1;//静音关执行失败
				}
				
				pclose(fp);
				fp = NULL;
			} else {
				ret = -1;//静音关执行失败
			}
			break;
		
		default:
			//不做处理原来是什么就是什么
			break;
	}

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETPLAYERCMDMUTE;
	
	pstr->Str.pGetPlayerCmdMute = (WARTI_pGetPlayerCmdMute)calloc(1, sizeof(WARTI_GetPlayerCmdMute));
	pstr->Str.pGetPlayerCmdMute->pRet = strdup("OK");
	pstr->Str.pGetPlayerCmdMute->Mute = WIFIAudio_SystemCommand_GetMute(); 

	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_loopmode(int num)
{
	int ret = 0;
	
	//先把各种模式都关闭掉
	ret = WIFIAudio_SystemCommand_SystemShellCommand("mpc repeat off");
	ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc random off");
	ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc single off");
	ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc consume off");
	switch(num)
	{
		case 0:
			ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc repeat on");
			break;
		
		case 1:
			ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc repeat on");
			ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc single on");
			break;
			
		case 2:
			ret = ret | WIFIAudio_SystemCommand_SystemShellCommand("mpc random on");
			break;
			
		default:
			break;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_setPlayerCmd_equalizer(int mode)
{
	int ret = 0;

	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getEqualizerTopStr(void)
{
	WARTI_pStr pstr = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	pstr->type = WARTI_STR_TYPE_GETEQUALIZER;
	
	pstr->Str.pGetEqualizer = (WARTI_pGetEqualizer)calloc(1, sizeof(WARTI_GetEqualizer));
	pstr->Str.pGetEqualizer->pRet = strdup("OK");
	pstr->Str.pGetEqualizer->Mode = 1; 
	
	return pstr;
}

int WIFIAudio_SystemCommand_setPlayerCmd_slave_channel(int value)
{
	int ret = 0;
	char tempbuff[128];
	
	if((value < 0) || (value > 2))
	{
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		sprintf(tempbuff, "uartdfifo.sh channel %03d &", value);
		
		ret = WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	}	

	
	return ret;
}

int WIFIAudio_SystemCommand_getPlayerCmd_slave_channel(void)
{
	int ret = 3;//0是正常播放，1是只播放左声道，2是只播放右声道，其他值表示错误
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.channel");
	if(NULL != pucistring)
	{
		ret = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	


	return ret;
}

int WIFIAudio_SystemCommand_setSSID(char *pssid)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == pssid)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID;
					pcmdparm->Parameter.pSetSSID = (WAFC_psetSSID)calloc(1, sizeof(WAFC_setSSID));
					if(NULL != pcmdparm->Parameter.pSetSSID)
					{
						pcmdparm->Parameter.pSetSSID->pSSID = strdup(pssid);
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						//写入长度小于等于0
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_setNetwork(WAGC_pNetwork_t pnetwork)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == pnetwork)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK;
					pcmdparm->Parameter.pSetNetwork = (WAFC_psetNetwork)calloc(1, sizeof(WAFC_setNetwork));
					if(NULL != pcmdparm->Parameter.pSetNetwork)
					{
						pcmdparm->Parameter.pSetNetwork->Flag = pnetwork->Auth;
						pcmdparm->Parameter.pSetNetwork->pPassword = strdup(pnetwork->pPassword);
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						//写入长度小于等于0
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_restoreToDefault(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_RESTORETODEFAULT;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_reboot(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REBOOT;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_setDeviceName(char *pname)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == pname)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME;
					pcmdparm->Parameter.pName = (WAFC_pName)calloc(1, sizeof(WAFC_Name));
					if(NULL != pcmdparm->Parameter.pName)
					{
						pcmdparm->Parameter.pName->pName = strdup(pname);
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						//写入长度小于等于0
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_setShutdown(int second)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN;
				pcmdparm->Parameter.pShutdownSec = (WAFC_pShutdownSec)calloc(1, sizeof(WAFC_ShutdownSec));
				if(NULL != pcmdparm->Parameter.pShutdownSec)
				{
					pcmdparm->Parameter.pShutdownSec->Second = second;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getShutdown(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
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
			pstr = NULL;
		} else {
			pstr->Str.pGetShutdown->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.powoff_enable");
			if(NULL != pucistring)
			{
				enable = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
			
			if(0 == enable)
			{
				pstr->Str.pGetShutdown->Second = -1;
			} else {
				WIFIAudio_Semaphore_Operation_P(sem_id);
				pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.powoff_gettime");
				if(NULL != pucistring)
				{
					pstr->Str.pGetShutdown->Second = atoi(pucistring);
					
					WIFIAudio_UciConfig_FreeValueString(&pucistring);
					pucistring = NULL;
				}
				WIFIAudio_Semaphore_Operation_V(sem_id);
			}
		}
	}
	
	return pstr;
}

WARTI_pStr WIFIAudio_SystemCommand_getManuFacturerInfo(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
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
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.manufacturer");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pManuFacturer = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.manufacturer_url");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pManuFacturerURL = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.model_description");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pModelDescription = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.model_name");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pModelName = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.model_number");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pModelNumber = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.model_url");
			if(NULL != pucistring)
			{
				pstr->Str.pGetManufacturerInfo->pModelURL = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
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
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE;
				pcmdparm->Parameter.pUpdate = (WAFC_pUpdate)calloc(1, sizeof(WAFC_Update));
				if(NULL != pcmdparm->Parameter.pUpdate)
				{
					if(NULL == purl)
					{
						//URL为空，传一个空格下去，这样对脚本也不会有影响
						pcmdparm->Parameter.pUpdate->pURL = strdup(" ");
					} else {
						pcmdparm->Parameter.pUpdate->pURL = strdup(purl);
					}
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
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
		
		//设置UTC时间
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
		
		//设置时区
		WIFIAudio_SystemCommand_SystemShellCommand(tempbuff);
	}


	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getDeviceTimeTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char temp[128];
	FILE *fp = NULL;
	char *pTmp = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_GETDEVICETIME;
		pstr->Str.pGetDeviceTime = (WARTI_pGetDeviceTime)calloc(1,sizeof(WARTI_GetDeviceTime));
		
		if(NULL == pstr->Str.pGetDeviceTime)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetDeviceTime->pRet = strdup("OK");
			fp = popen("powoff.sh get_utczone", "r");
			if(NULL == fp)
			{
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				memset(temp, 0, sizeof(temp));
				if(NULL == fgets(temp, sizeof(temp), fp))
				{
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pTmp = strchr(temp, ':');
					if(NULL == pTmp)
					{
						WIFIAudio_RealTimeInterface_pStrFree(&pstr);
						pstr = NULL;
					} else {
						pTmp++;
						sscanf(pTmp, "%04d-%02d-%02d %02d:%02d:%02d", \
						&(pstr->Str.pGetDeviceTime->Year), \
						&(pstr->Str.pGetDeviceTime->Month), \
						&(pstr->Str.pGetDeviceTime->Day), \
						&(pstr->Str.pGetDeviceTime->Hour), \
						&(pstr->Str.pGetDeviceTime->Minute), \
						&(pstr->Str.pGetDeviceTime->Second));
					}
				}
				pclose(fp);
				fp = NULL;
			}
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.set_timezone");
			if(NULL != pucistring)
			{
				pstr->Str.pGetDeviceTime->Zone = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

//统计含有几个1
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

//计算从低位起第一个1在第几位
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
				value = value >> 1;//移除一位低位
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
	char cmd[128];
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	if((num >= 0) && (num <= 1))
	{
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
		} else {
			pstr->type = WARTI_STR_TYPE_GETALARMCLOCK;
			
			pstr->Str.pGetAlarmClock = (WARTI_pGetAlarmClock)calloc(1, sizeof(WARTI_GetAlarmClock));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				pstr->Str.pGetAlarmClock->pRet = strdup("OK");
				pstr->Str.pGetAlarmClock->N = num;
				pstr->Str.pGetAlarmClock->Operation = 0;
				pstr->Str.pGetAlarmClock->Volume = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
				
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "xzxwifiaudio.config.timer%d", num);
				
				sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
				WIFIAudio_Semaphore_Operation_P(sem_id);

				pucistring = WIFIAudio_UciConfig_SearchValueString(cmd);
				if(NULL != pucistring)
				{
					if((NULL == pucistring) || (strlen(pucistring) < 3))
					{
						//这边长度小于3没有特别的意思，只是说明读出来的不是实际内容，因为闹钟的信息肯定不止3个字符
						WIFIAudio_RealTimeInterface_pStrFree(&pstr);
						pstr = NULL;
					} else {
						if(4 != sscanf(pucistring, "%01d%02d%02d%02x", \
						&(pstr->Str.pGetAlarmClock->Enable), &(pstr->Str.pGetAlarmClock->Minute), \
						&(pstr->Str.pGetAlarmClock->Hour), &(pstr->Str.pGetAlarmClock->WeekDay)))
						{
							WIFIAudio_RealTimeInterface_pStrFree(&pstr);
							pstr = NULL;
						} else {
							int onesnum = WIFIAudio_SystemCommand_CountOnesNumber(pstr->Str.pGetAlarmClock->WeekDay);
							
							if(onesnum > 1)
							{
								if(0x7f == pstr->Str.pGetAlarmClock->WeekDay)
								{
									//每天
									pstr->Str.pGetAlarmClock->Trigger = 2;
								} else {
									//每星期扩展模式
									pstr->Str.pGetAlarmClock->Trigger = 4;
								}
							} else if(1 == onesnum) {
								//每星期
								pstr->Str.pGetAlarmClock->WeekDay = WIFIAudio_SystemCommand_WhereIsOnes(pstr->Str.pGetAlarmClock->WeekDay);
								
								pstr->Str.pGetAlarmClock->Trigger = 3;
								
							}
							
						}
					}
					
					WIFIAudio_UciConfig_FreeValueString(&pucistring);
					pucistring = NULL;
				}

				WIFIAudio_Semaphore_Operation_V(sem_id);
				
				if(NULL != pstr)
				{
					//如果上面闹钟信息已经为空了（长度小于3个字符），这边自然不会再去获取url
					//上面没出错
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "xzxwifiaudio.config.timer%d_url", num);
					
					sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
					WIFIAudio_Semaphore_Operation_P(sem_id);
					
					pucistring = WIFIAudio_UciConfig_SearchValueString(cmd);
					if(NULL != pucistring)
					{
						pstr->Str.pGetAlarmClock->pMusic = (WARTI_pMusic)calloc(1, sizeof(WARTI_Music));
						if(NULL == pstr)
						{
#ifdef WIFIAUDIO_DEBUG
							syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
							WIFIAudio_RealTimeInterface_pStrFree(&pstr);
							pstr = NULL;
						} else {
							pstr->Str.pGetAlarmClock->pMusic->pContentURL = strdup(pucistring);
						}
						
						WIFIAudio_UciConfig_FreeValueString(&pucistring);
						pucistring = NULL;
					}
					
					WIFIAudio_Semaphore_Operation_V(sem_id);
				}
			}
		}
	}

	
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
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pstr = NULL;
	} else {
		pstr->type = WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE;
		
		pstr->Str.pGetPlayerCmdSwitchMode = (WARTI_pGetPlayerCmdSwitchMode)calloc(1, sizeof(WARTI_GetPlayerCmdSwitchMode));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
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
			//禁用语音
			
			break;
			
		case 1:
			//汉语
			ret = WIFIAudio_SystemCommand_SystemShellCommand("setLanguage.sh cn");
			break;
			
		case 2:
			//英语
			ret = WIFIAudio_SystemCommand_SystemShellCommand("setLanguage.sh en");
			break;
			
		case 3:
			//法语
			
			break;
			
		case 4:
			//西班牙语
			
			break;
			
		default:
			break;
	}

	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_getLanguageTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETLANGUAGE;
		pstr->Str.pGetLanguage = (WARTI_pGetLanguage)calloc(1, sizeof(WARTI_GetLanguage));
		if(NULL == pstr->Str.pGetLanguage)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		} else {
			pstr->Str.pGetLanguage->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);

			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.language");
			if(NULL != pucistring)
			{
				if(strstr(pucistring,"cn"))
				{
					pstr->Str.pGetLanguage->Language = 1;
				} else if(strstr(pucistring,"en")) {
					pstr->Str.pGetLanguage->Language = 2;
				}
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}

			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

//这边json不使用json库解析
//因为嵌入式这边性能问题
//歌多的话，解析需要挺长时间
//即使不把每个字段解析出来
//只要是使用库解析都需要一定时间
//所以这边特殊处理
char* WIFIAudio_SystemCommand_JsonSringAddRetOk(char *pfilename)
{
	struct stat filestat;//用来读取文件大小
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
			ret = fread(pjsonstring, 1, filestat.st_size, fp);//读取json字符串
			fclose(fp);
			fp = NULL;
			
			if(ret >= 0)
			{
				if(ptmp = strstr(pjsonstring, "\"ret\": null"))
				{
					//读取的东西里面就含有"ret": null
					memcpy(ptmp, "\"ret\": \"OK\"", strlen("\"ret\": \"OK\""));
				} else if(ptmp = strstr(pjsonstring, "\"ret\": \"OK\"")) {
					//原来已经含有"ret": "OK"
					//不再进行添加
				} else {
					ptmp = NULL;
					//读取的东西当中不含有}说明格式有错
					ptmp = strrchr(pjsonstring, '}');
					if(NULL == ptmp)
					{
						free(pjsonstring);
						pjsonstring = NULL;
					} else {
						//这边将ret:OK放到最后不需要移位整个字符串
						//可以最快的完成添加的动作
						//因为json当中没有顺序要求
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
	
	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(\
	WIFIAUDIO_CURRENTPLAYLIST_PATHNAME);

	
	return pjsonstring;
}

//返回当前播放的歌曲数,没在播放返回0
int WIFIAudio_SystemCommand_GetCurrentPlaylistNum(void)
{
	int num = 0;
	FILE *fp = NULL;
	char temp[20];
	
	fp = popen("mpc playlist | wc -l", "r");
	if(NULL == fp)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		num = 0;
	} else {
		memset(temp, 0, sizeof(temp));
		if(NULL == fgets(temp, sizeof(temp), fp))
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
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

int WIFIAudio_SystemCommand_xzxAction_setrt(int value)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD;
				pcmdparm->Parameter.pSetRecord = (WAFC_pSetRecord)calloc(1, sizeof(WAFC_SetRecord));
				if(NULL != pcmdparm->Parameter.pSetRecord)
				{
					pcmdparm->Parameter.pSetRecord->Value = value;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_playrec(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYRECORD;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_playurlrec(char *precurl)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == precurl)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD;
					pcmdparm->Parameter.pPlayUrlRecord = (WAFC_pPlayUrlRecord)calloc(1, sizeof(WAFC_PlayUrlRecord));
					if(NULL != pcmdparm->Parameter.pPlayUrlRecord)
					{
						pcmdparm->Parameter.pPlayUrlRecord->pUrl = strdup(precurl);
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						//写入长度小于等于0
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_StorageDeviceOnlineStateTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//开始分配空间
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_STORAGEDEVICEONLINESTATE;
		
		pstr->Str.pStorageDeviceOnlineState = (WARTI_pStorageDeviceOnlineState)calloc(1, sizeof(WARTI_StorageDeviceOnlineState));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pStorageDeviceOnlineState->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);

			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.tf_status");
			if(NULL != pucistring)
			{
				pstr->Str.pStorageDeviceOnlineState->TF = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_status");
			if(NULL != pucistring)
			{
				pstr->Str.pStorageDeviceOnlineState->USB = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

char* WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(\
	WIFIAUDIO_TFPLAYLISTINFO_PATHNAME);

	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_PlayTfCard(int index)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD;
				pcmdparm->Parameter.pPlayTfCard = (WAFC_pPlayTfCard)calloc(1, sizeof(WAFC_PlayTfCard));
				if(NULL != pcmdparm->Parameter.pPlayTfCard)
				{
					pcmdparm->Parameter.pPlayTfCard->Index = index;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

char* WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString(void)
{
	char *pjsonstring = NULL;
	
	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(\
	WIFIAUDIO_USBPLAYLISTINFO_PATHNAME);
	
	return pjsonstring;
}

int WIFIAudio_SystemCommand_PlayUsbDisk(int index)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK;
				pcmdparm->Parameter.pPlayUsbDisk = (WAFC_pPlayUsbDisk)calloc(1, sizeof(WAFC_PlayUsbDisk));
				if(NULL != pcmdparm->Parameter.pPlayUsbDisk)
				{
					pcmdparm->Parameter.pPlayUsbDisk->Index = index;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr(void)
{
	int i = 0;
	int nullnum = 0;
	int ret = -1;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	struct stat filestat;//用来读取文件大小
	
	if(NULL == (fp = fopen(WIFIAUDIO_SNAPCASTSERVER_PATHNAME, "r")))
	{
		pstr = NULL;
	} else {
		stat(WIFIAUDIO_SNAPCASTSERVER_PATHNAME, &filestat);
		pjsonstring = (char *)calloc(1, (filestat.st_size) * sizeof(char));
		ret = fread(pjsonstring, 1, filestat.st_size, fp);//读取json字符串
		fclose(fp);
		fp = NULL;
		
		if(ret >= 0)
		{
			pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_GETSYNCHRONOUSINFOEX, pjsonstring);
			
			free(pjsonstring);
			pjsonstring = NULL;
			
			if(NULL != pstr)
			{
				pstr->Str.pGetCurrentPlaylist->pRet = strdup("OK");
			}
		}
	}

	
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
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == psetsynchronousex)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME;
					pcmdparm->Parameter.pSetMultiVolume = (WAFC_pSetMultiVolume)calloc(1, sizeof(WAFC_SetMultiVolume));
					if(NULL != pcmdparm->Parameter.pSetMultiVolume)
					{
						pcmdparm->Parameter.pSetMultiVolume->Value = psetsynchronousex->Value;
						pcmdparm->Parameter.pSetMultiVolume->pMac = strdup(psetsynchronousex->pMac);
						
						WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
						//写入长度小于等于0
						if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
						{
							ret = -1;
						}
					} else {
						ret = -1;
					}
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_startrecord(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STARTRECORD;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_stoprecord(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, 128);
	sprintf(str, "uartdfifo.sh testtlkoff &");
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand(str);
	
	return ret;
}

int WIFIAudio_SystemCommand_xzxAction_fixedrecord(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_FIXEDRECORD;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr(void)
{
	int i = 0;
	int nullnum = 0;
	int ret = -1;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char filename[128];
	char tmp[256];
	struct in_addr ip;
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	
	//这边先只读打开music下录音文件试一下，免得不存在还返回成功就jiānjiè了
	if(NULL == (fp = fopen(filename, "r")))
	{
		pstr = NULL;
	} else {
		fclose(fp);
		fp = NULL;
		
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;
			
			pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				WIFIAudio_SystemCommand_GetLocalWifiIp(&ip);
				if(!strcmp(inet_ntoa(ip), "0.0.0.0"))
				{
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
					
					sprintf(tmp, "http://%s/%s%s%s", inet_ntoa(ip), \
					WIFIAUDIO_RECORDINSERVER_PATH2, \
					WIFIAUDIO_RECORDINSERVER_NAME, \
					WIFIAUDIO_RECORDINSERVER_SUFFIX);
					
					pstr->Str.pGetRecordFileURL->pURL = strdup(tmp);
				}
			}
		}
	}
	
	return pstr;
}


int WIFIAudio_SystemCommand_xzxAction_vad(WAGC_pxzxAction_t pxzxaction)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(NULL == pxzxaction)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if((NULL == pxzxaction->pCardName) || (pxzxaction->Value < 500000) || (pxzxaction->Value > 2000000))
		{
			ret = -1;
		} else {
			//几个允许的声卡名称
			if(strcmp(pxzxaction->pCardName, "default") \
			&& strcmp(pxzxaction->pCardName, "plughw:1") \
			&& strcmp(pxzxaction->pCardName, "plughw:0"))
			{
				ret = -1;
			} else {
				fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
				if(fdfifo > 0)
				{
					pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
					if(NULL != pfifobuff)
					{
						pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
						if(NULL != pcmdparm)
						{
							pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION;
							pcmdparm->Parameter.pVoiceActivityDetection = (\
							WAFC_pVoiceActivityDetection)calloc(1, sizeof(WAFC_VoiceActivityDetection));
							if(NULL != pcmdparm->Parameter.pVoiceActivityDetection)
							{
								pcmdparm->Parameter.pVoiceActivityDetection->Value = pxzxaction->Value;
								pcmdparm->Parameter.pVoiceActivityDetection->pCardName = strdup(pxzxaction->pCardName);
								
								WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
								
								//写入长度小于等于0
								if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
								{
									ret = -1;
								}
							} else {
								ret = -1;
							}
							WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
						} else {
							ret = -1;
						}
						
						WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
						pfifobuff = NULL;
					} else {
						ret = -1;
					}
					
					close(fdfifo);//将管道关闭
					fdfifo = 0;
				} else {
					ret = -1;
				}
			}
		}
	}


	
	return ret;
}

int WIFIAudio_SystemCommand_GetBluetoothStatusInFile(void)
{
	int status = 0;
	char str[16];
	FILE *fp = NULL;
	
	memset(str, 0, sizeof(str));
	
	if(NULL != (fp = fopen(WIFIAUDIO_BULETOOTHSTATUS_PATHNAME, "r")))
	{
		if(NULL == fgets(str, sizeof(str), fp))
		{
			status = 0;
		} else {
			status = atoi(strtok(str, "\r\n"));
		}
		
		fclose(fp);
		fp = NULL;
	}
	
	return status;
}

int WIFIAudio_SystemCommand_Bluetooth_Search(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if(3 == WIFIAudio_SystemCommand_GetBluetoothStatusInFile())
	{
		//正在搜索中，不再进行搜索，直接返回OK
		ret = 0;
	} else {
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SEARCHBLUETOOTH;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
					
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

//一个文件有多少行
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
		//这边就不使用cat %s | wc -l的方式了
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
				//去掉文件名
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
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	//获取连接状态
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.btconnectstatus");
	if(NULL != pucistring)
	{
		status = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	return status;
}

char* WIFIAudio_SystemCommand_GetBluetoothAddeInFile(void)
{
	char *paddr = NULL;
	char str[32];
	FILE *fp = NULL;
	
	memset(str, 0, sizeof(str));
	
	if(NULL != (fp = fopen(WIFIAUDIO_BULETOOTHADDR_PATHNAME, "r")))
	{
		if(NULL == fgets(str, sizeof(str), fp))
		{
			paddr = NULL;
		} else {
			paddr = strdup(strtok(str, "\r\n"));
		}
		
		fclose(fp);
		fp = NULL;
	}
	
	return paddr;
}

int WIFIAudio_SystemCommand_GetBluetoothStatusInInfo(void)
{
	int status = 0;
	char str[64];
	FILE *fp = NULL;
	
	memset(str, 0, sizeof(str));
	
	if(NULL != (fp = fopen(WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME, "r")))
	{
		while(NULL != fgets(str, sizeof(str), fp))
		{
			if('1' == str[0])
			{
				status = 1;
				break;
			}
		}
		
		fclose(fp);
		fp = NULL;
	}
	
	return status;
}

char* WIFIAudio_SystemCommand_GetBluetoothAddrInInfo(void)
{
	char str[64];
	FILE *fp = NULL;
	char *pAddr = NULL;
	
	memset(str, 0, sizeof(str));
	
	if(NULL != (fp = fopen(WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME, "r")))
	{
		while(NULL != fgets(str, sizeof(str), fp))
		{
			if('1' == str[0])
			{
				strtok(str, "+&\r\n");
				pAddr = strdup(strtok(NULL, "+&\r\n"));
				break;
			}
		}
		
		fclose(fp);
		fp = NULL;
	}
	
	return pAddr;
}

WARTI_pStr WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr(void)
{
	int i = 0;
	int ret = -1;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char str[64];
	int num;
	char *ptmp = NULL;
	int status_file = 0;
	//int status_conf = 0;
	int status_info = 0;
	char *paddr_file = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETBLUETOOTHLIST;
		
		pstr->Str.pGetBluetoothList = (WARTI_pGetBluetoothList)calloc(1, sizeof(WARTI_GetBluetoothList));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetBluetoothList->pRet = strdup("OK");
			
			status_file = WIFIAudio_SystemCommand_GetBluetoothStatusInFile();
			//status_conf = WIFIAudio_SystemCommand_GetBluetoothStatusInConf();
			status_info = WIFIAudio_SystemCommand_GetBluetoothStatusInInfo();
			if(3 == status_file)
			{
				//还在搜索
				pstr->Str.pGetBluetoothList->Status = 3;
			} else {
				//if((0 == status_conf) && ((0 == status_file) || ((4 == status_file))))
				if((0 == status_file) || ((4 == status_file)))
				{
					//配置文件当中为未连接，且为连接失败或者搜索完成，设置为未连接
					pstr->Str.pGetBluetoothList->Status = 0;
				} else {
					if(1 == status_info)
					{
						//连接成功
						pstr->Str.pGetBluetoothList->Status = 1;
						paddr_file = WIFIAudio_SystemCommand_GetBluetoothAddrInInfo();
					} else if(2 == status_file) {
					// } else if((2 == status_file) && (0 == status_conf)) {
						//未连接，且正在连接
						pstr->Str.pGetBluetoothList->Status = 2;
						paddr_file = WIFIAudio_SystemCommand_GetBluetoothAddeInFile();
					}
				}
				
				//只要不是正在搜索状态都需要返回列表
				if(NULL != (fp = fopen(WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME, "r")))
				{
					//获取数量
					pstr->Str.pGetBluetoothList->Num = \
					WIFIAudio_SystemCommand_LinesOfBtFile(WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME);
					
					if(pstr->Str.pGetBluetoothList->Num > 0)
					{
						pstr->Str.pGetBluetoothList->pBdList = \
						(WARTI_pBluetoothDevice *)calloc(1, pstr->Str.pGetBluetoothList->Num * sizeof(WARTI_pBluetoothDevice));
						if(NULL == pstr)
						{
							WIFIAudio_RealTimeInterface_pStrFree(&pstr);
							pstr = NULL;
						} else {
							for(i = 0; i < pstr->Str.pGetBluetoothList->Num; i++)
							{
								memset(str, 0, sizeof(str));
								if(NULL == fgets(str, sizeof(str), fp))
								{
									WIFIAudio_RealTimeInterface_pStrFree(&pstr);
									pstr = NULL;
									break;
								} else {
									if(NULL == strstr(str, "&"))
									{
										i--;
										//continue跳出循环的时候会继续走增量的那一部分
										//所以这边要做一下自减，抵消此次无用的循环
										continue;
									}
									pstr->Str.pGetBluetoothList->pBdList[i] = \
									(WARTI_pBluetoothDevice)calloc(1, sizeof(WARTI_BluetoothDevice));
									if(NULL == pstr->Str.pGetBluetoothList->pBdList[i])
									{
										WIFIAudio_RealTimeInterface_pStrFree(&pstr);
										pstr = NULL;
										break;
									} else {
										ptmp = strtok(str, "+&\r\n");
										if(NULL == ptmp)
										{
											pstr->Str.pGetBluetoothList->pBdList[i]->Connect = 0;
										} else {
											pstr->Str.pGetBluetoothList->pBdList[i]->Connect = atoi(ptmp)/10;
										}
										pstr->Str.pGetBluetoothList->pBdList[i]->Connect = 0;
										
										ptmp = strtok(NULL, "+&\r\n");
										if(NULL == ptmp)
										{
											pstr->Str.pGetBluetoothList->pBdList[i]->pAddress = NULL;
										} else {
											pstr->Str.pGetBluetoothList->pBdList[i]->pAddress = strdup(ptmp);
											
											//当前是连接中或者已连接的状态下
											if((1 == pstr->Str.pGetBluetoothList->Status) || \
											(2 == pstr->Str.pGetBluetoothList->Status))
											{
												if(NULL != paddr_file)
												{
													//与当前连接的地址匹配
													if(!strcmp(paddr_file, ptmp))
													{
														pstr->Str.pGetBluetoothList->pBdList[i]->Connect = \
														pstr->Str.pGetBluetoothList->Status;
													}
												}
											}
										}
										ptmp = strtok(NULL, "+&\r\n");
										
										if(NULL == ptmp)
										{
											pstr->Str.pGetBluetoothList->pBdList[i]->pName = NULL;
										} else {
											pstr->Str.pGetBluetoothList->pBdList[i]->pName = strdup(ptmp);
										}
									}
								}
							}
						}
					}
					
					fclose(fp);
					fp = NULL;
				}
				
				if(NULL != paddr_file)
				{
					free(paddr_file);
					paddr_file = NULL;
				}
			}
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_Bluetooth_Connect(char *paddress)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH;
				pcmdparm->Parameter.pConnectBluetooth = (WAFC_pConnectBluetooth)calloc(1, sizeof(WAFC_ConnectBluetooth));
				if(NULL != pcmdparm->Parameter.pConnectBluetooth)
				{
					pcmdparm->Parameter.pConnectBluetooth->pAddress = strdup(paddress);
						
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
						
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
				
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
			
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_Bluetooth_Disconnect(void)
{
	int ret = 0;
	
	ret = WIFIAudio_SystemCommand_SystemShellCommand("uartdfifo.sh btdiscon");
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETSPEECHRECOGNITION;
		
		pstr->Str.pGetSpeechRecognition = (WARTI_pGetSpeechRecognition)calloc(1, sizeof(WARTI_GetSpeechRecognition));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetSpeechRecognition->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.speech_recognition");
			if(NULL != pucistring)
			{
				pstr->Str.pGetSpeechRecognition->Platform = atoi(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
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
	int i = 0;
	LedUart_pDataContent pDataContent = NULL;
	WFLCF_pBuff pBuff = NULL;
	WFLCU_pUartBuff pUartBuff = NULL;
	int sem_id = 0;//信号量ID
	int fd = 0;
	int buffLen = 0;
	
	if(NULL == pSetSingleLight)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pDataContent = (LedUart_pDataContent)calloc(1, sizeof(LedUart_DataContent));
		
		if(NULL == pDataContent)
		{
			ret = -1;
		} else {
			pDataContent->Type = WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_SINGLELIGHT;
			
			pDataContent->Data.pSingleLight = (LedUart_pSingleLight)calloc(1, sizeof(LedUart_SingleLight));
			if(NULL == pDataContent->Data.pSingleLight)
			{
				ret = -1;
			} else {
				pDataContent->Data.pSingleLight->Enable = pSetSingleLight->Enable;
				pDataContent->Data.pSingleLight->Brightness = pSetSingleLight->Brightness;
				pDataContent->Data.pSingleLight->Red = pSetSingleLight->Red;
				pDataContent->Data.pSingleLight->Green = pSetSingleLight->Green;
				pDataContent->Data.pSingleLight->Blue = pSetSingleLight->Blue;
				
				pBuff = WIFIAudio_LightControlFormat_pDataContentTopBuff(pDataContent);
				if(NULL == pBuff)
				{
					ret = -1;
				} else {
					buffLen = WIFIAudio_LightControl_GetCommandBuffLength(pBuff);
					
					if(0 == access(WIFIAUDIO_LEDFIFO_PATHNAME, F_OK))
					{
						
					} else {
						mkfifo(WIFIAUDIO_LEDFIFO_PATHNAME, 0666);
					}
					
					fd = open(WIFIAUDIO_LEDFIFO_PATHNAME, O_WRONLY);
					
					if(fd > 0)
					{
						sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
						WIFIAudio_Semaphore_Operation_P(sem_id);
						if(write(fd, pBuff, buffLen) > 0)
						{
							
						} else {
							ret = -1;
						}
						WIFIAudio_Semaphore_Operation_V(sem_id);
						close(fd);
						fd = -1;
					}
				}
			}
			
			WIFIAudio_LightControlFormat_FreeppDataContent(&pDataContent);
		}
	}

	
	return ret;
}


WARTI_pStr WIFIAudio_SystemCommand_GetAlexaLanguageTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	char *pucistring = NULL;
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETALEXALANGUAGE;
		
		pstr->Str.pGetAlexaLanguage = (WARTI_pGetAlexaLanguage)calloc(1, sizeof(WARTI_GetAlexaLanguage));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			pstr = NULL;
		} else {
			pstr->Str.pGetAlexaLanguage->pRet = strdup("OK");
			
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_language");
			if(NULL != pucistring)
			{
				pstr->Str.pGetAlexaLanguage->pLanguage = strdup(pucistring);
				
				WIFIAudio_UciConfig_FreeValueString(&pucistring);
				pucistring = NULL;
			}
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
		}
	}

	
	return pstr;
}

#define UNIX_DOMAIN "/tmp/UNIX.domain"

static int OnWriteMsgToAlexa(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		//printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		//printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		//printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	} else {
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

int WIFIAudio_SystemCommand_SetAlexaLanguage(char* pLanguage)
{
	int ret = 0;
	char tmp[128];
	
	if(NULL == pLanguage)
	{
		ret = -1;
	} else {
		if((!strcmp(pLanguage, "en-AU")) || \
		   (!strcmp(pLanguage, "en-CA")) || \
		   (!strcmp(pLanguage, "en-GB")) || \
		   (!strcmp(pLanguage, "en-IN")) || \
		   (!strcmp(pLanguage, "en-US")) || \
		   (!strcmp(pLanguage, "de-DE")) || \
		   (!strcmp(pLanguage, "ja-JP")))
		{
			sprintf(tmp, "SetAlexaLanguage.sh %s", pLanguage);
			ret = WIFIAudio_SystemCommand_SystemShellCommand(tmp);
			OnWriteMsgToAlexa("SetAlexaLanguage");
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_UpdataConexant(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	if((1 == WIFIAudio_SystemCommand_FileExists(WIFIAUDIO_CONEXANTSFS_PATHNAME)) \
	&& (1 == WIFIAudio_SystemCommand_FileExists(WIFIAUDIO_CONEXANTBIN_PATHNAME)))
	{
		//升级需要的两个文件都存在
		fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
		if(fdfifo > 0)
		{
			pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
			if(NULL != pfifobuff)
			{
				pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
				if(NULL != pcmdparm)
				{
					pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATACONEXANT;
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
					
					WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
				pfifobuff = NULL;
			} else {
				ret = -1;
			}
			
			close(fdfifo);//将管道关闭
			fdfifo = 0;
		} else {
			ret = -1;
		}
	} else {
		//升级所需要的sfs和bin文件有缺少
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_SetTestMode(int mode)
{
	int ret = 0;
	int sem_id = 0;//信号量ID
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	if(0 == mode)
	{
		WIFIAudio_UciConfig_SetValueString("xzxwifiaudio.config.testmode", "0");
	} else {
		WIFIAudio_UciConfig_SetValueString("xzxwifiaudio.config.testmode", "1");
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr(void)
{
	WARTI_pStr pstr = NULL;
	float left = 0;
	float right = 0;
	char filename[256];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	
	if(1 == WIFIAudio_SystemCommand_FileExists(filename))
	{
		//先判断录音文件存在
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_GETCHANNELCOMPARE;
			
			pstr->Str.pGetChannelCompare = (WARTI_pGetChannelCompare)calloc(1, sizeof(WARTI_GetChannelCompare));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				pstr->Str.pGetChannelCompare->pRet = strdup("OK");
				
				WIFIAudio_ChannelCompare_channel_compare(filename, &left, &right);
				
				pstr->Str.pGetChannelCompare->Left = left;
				
				pstr->Str.pGetChannelCompare->Right = right;
			}
		}
	}
	
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
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD;
				pcmdparm->Parameter.pSetRecord = (WAFC_pSetRecord)calloc(1, sizeof(WAFC_SetRecord));
				if(NULL == pcmdparm->Parameter.pSetRecord)
				{
					ret = -1;
				} else {
					pcmdparm->Parameter.pSetRecord->Value = second;
					
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char tmp[256];
	struct in_addr ip;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);
	//后台录音要让他停下来
	if(1 != WIFIAudio_SystemCommand_FileExists(WIFIAUDIO_NORMALINSYSTEM_PATHNAME))
	{
		pstr = NULL;
	} else {
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;
			
			pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				WIFIAudio_SystemCommand_GetLocalWifiIp(&ip);
				if(!strcmp(inet_ntoa(ip), "0.0.0.0"))
				{
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
					
					sprintf(tmp, "http://%s%s", inet_ntoa(ip), \
					WIFIAUDIO_NORMALINSERVER_PATHNAME);
					
					pstr->Str.pGetRecordFileURL->pURL = strdup(tmp);
				}
			}
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_micrecord(int second)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD;
				pcmdparm->Parameter.pSetRecord = (WAFC_pSetRecord)calloc(1, sizeof(WAFC_SetRecord));
				if(NULL == pcmdparm->Parameter.pSetRecord)
				{
					ret = -1;
				} else {
					pcmdparm->Parameter.pSetRecord->Value = second;
					
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char tmp[256];
	struct in_addr ip;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);
	//后台录音要让他停下来
	if(1 != WIFIAudio_SystemCommand_FileExists(WIFIAUDIO_MICINSYSTEM_PATHNAME))
	{
		pstr = NULL;
	} else {
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;
			
			pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				WIFIAudio_SystemCommand_GetLocalWifiIp(&ip);
				if(!strcmp(inet_ntoa(ip), "0.0.0.0"))
				{
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
					
					sprintf(tmp, "http://%s%s", inet_ntoa(ip), \
					WIFIAUDIO_MICINSERVER_PATHNAME);
					
					pstr->Str.pGetRecordFileURL->pURL = strdup(tmp);
				}
			}
		}
	}

	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_refrecord(int second)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD;
				pcmdparm->Parameter.pSetRecord = (WAFC_pSetRecord)calloc(1, sizeof(WAFC_SetRecord));
				if(NULL == pcmdparm->Parameter.pSetRecord)
				{
					ret = -1;
				} else {
					pcmdparm->Parameter.pSetRecord->Value = second;
					
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr(void)
{
	WARTI_pStr pstr = NULL;
	char tmp[256];
	struct in_addr ip;
	
	WIFIAudio_SystemCommand_xzxAction_stopModeRecord();
	usleep(700000);
	//后台录音要让他停下来
	if(1 != WIFIAudio_SystemCommand_FileExists(WIFIAUDIO_REFINSYSTEM_PATHNAME))
	{
		pstr = NULL;
	} else {
		pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			pstr->type = WARTI_STR_TYPE_GETRECORDFILEURL;
			
			pstr->Str.pGetRecordFileURL = (WARTI_pGetRecordFileURL)calloc(1, sizeof(WARTI_GetRecordFileURL));
			if(NULL == pstr)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				WIFIAudio_RealTimeInterface_pStrFree(&pstr);
				pstr = NULL;
			} else {
				WIFIAudio_SystemCommand_GetLocalWifiIp(&ip);
				if(!strcmp(inet_ntoa(ip), "0.0.0.0"))
				{
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				} else {
					pstr->Str.pGetRecordFileURL->pRet = strdup("OK");
					
					sprintf(tmp, "http://%s%s", inet_ntoa(ip), \
					WIFIAUDIO_REFINSERVER_PATHNAME);
					
					pstr->Str.pGetRecordFileURL->pURL = strdup(tmp);
				}
			}
		}
	}
	
	return pstr;
}

int WIFIAudio_SystemCommand_xzxAction_stopModeRecord(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STOPMODERECORD;
					
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_GetWifiUpgradeState(void)
{
	int State = 0;
	int sem_id = 0;
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wifi_update_flage");
	if(NULL != pucistring)
	{
		State = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	//逻辑判断
	if(State == 1)
	{
		//uci当中正在升级中是1
		//http约定正在升级中是2
		State = 2;
	} else if(State == 2) {
		//uci当中正在升级中是2
		//http约定正在升级中是1
		State = 1;
		
		//这个时候手动恢复成默认值0
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		WIFIAudio_UciConfig_SetValueString("xzxwifiaudio.config.wifi_update_flage", "0");
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
	}
	
	return State;
}

int WIFIAudio_SystemCommand_GetWifiUpgradeDownloadProgress(void)
{
	int Progress = 0;
	int sem_id = 0;
	char *pTmp_Download = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		//打开文件获取前，先进行信号量加锁
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_WIFIDOWNLOADPROGRESS_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
			
		if(NULL != (pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIDOWNLOADPROGRESS_PATHNAME, 10)))
		{
			Progress = (pTmp_Download[0] > 100)?100:pTmp_Download[0];
			free(pTmp_Download);
			pTmp_Download = NULL;
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetWifiUpgradeBurnProgress(void)
{
	int Progress = 0;
	char *pTmp_Upgrade = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		if(NULL != (pTmp_Upgrade = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIUPGRADEPROGRESS_PATHNAME, 10)))
		{
			Progress = atoi(strtok(pTmp_Upgrade, " .%%"));
			
			free(pTmp_Upgrade);
			pTmp_Upgrade = NULL;
		}
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_GetDownloadProgressTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int State = 0;
	int Download = 0;
	int Upgrade = 0;
	
	/*
	State = WIFIAudio_SystemCommand_GetWifiUpgradeState();
	
	if(State == 2)
	{
		//升级中才会有进度
		Download = WIFIAudio_SystemCommand_GetWifiUpgradeDownloadProgress();
		Upgrade = WIFIAudio_SystemCommand_GetWifiUpgradeBurnProgress();
	}
	*/
	Download = WIFIAudio_SystemCommand_GetWifiUpgradeDownloadProgress();
	Upgrade = WIFIAudio_SystemCommand_GetWifiUpgradeBurnProgress();
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETDOWNLOADPROGRESS;
		
		pstr->Str.pGetRemoteUpdateProgress = (WARTI_pGetRemoteUpdateProgress)calloc(1, sizeof(WARTI_GetRemoteUpdateProgress));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
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

int WIFIAudio_SystemCommand_GetBluetoothUpgradeState(void)
{
	int State = 0;
	int sem_id = 0;
	char *pucistring = NULL;
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	pucistring = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.bt_update_flage");
	if(NULL != pucistring)
	{
		State = atoi(pucistring);
		
		WIFIAudio_UciConfig_FreeValueString(&pucistring);
		pucistring = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	//逻辑判断
	if(State == 1)
	{
		//uci当中正在升级中是1
		//http约定正在升级中是2
		State = 2;
	} else if(State == 2) {
		//uci当中正在升级中是2
		//http约定正在升级中是1
		State = 1;
		
		//这个时候手动恢复成默认值0
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_UCICONFIG_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		WIFIAudio_UciConfig_SetValueString("xzxwifiaudio.config.bt_update_flage", "0");
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
	}
	
	return State;
}

int WIFIAudio_SystemCommand_GetBluetoothUpgradeDownloadProgress(void)
{
	int Progress = 0;
	int sem_id = 0;
	char *pTmp_Download = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		//打开文件获取前，先进行信号量加锁
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		if(NULL != (pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_PATHNAME, 10)))
		{
			Progress = (pTmp_Download[0] > 100)?100:pTmp_Download[0];
			free(pTmp_Download);
			pTmp_Download = NULL;
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothUpgradeBurnProgress(void)
{
	int Progress = 0;
	char *pTmp_Upgrade = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		if(NULL != (pTmp_Upgrade = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_BLUETOOTHUPGRADEPROGRESS_PATHNAME, 10)))
		{
			Progress = atoi(strtok(pTmp_Upgrade, " .%%"));
			
			free(pTmp_Upgrade);
			pTmp_Upgrade = NULL;
		}
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int Download = 0;
	int Upgrade = 0;
	int State = 0;
	
	/*
	State = WIFIAudio_SystemCommand_GetBluetoothUpgradeState();
	
	if(State == 2)
	{
		//升级中才会有进度
		Download = WIFIAudio_SystemCommand_GetBluetoothUpgradeDownloadProgress();
		Upgrade = WIFIAudio_SystemCommand_GetBluetoothUpgradeBurnProgress();
	}
	*/
	
	Download = WIFIAudio_SystemCommand_GetBluetoothUpgradeDownloadProgress();
	Upgrade = WIFIAudio_SystemCommand_GetBluetoothUpgradeBurnProgress();
	
	//构建结构体
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_GETDOWNLOADPROGRESS;
		
		pstr->Str.pGetRemoteUpdateProgress = (WARTI_pGetRemoteUpdateProgress)calloc(1, sizeof(WARTI_GetRemoteUpdateProgress));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
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
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_DISCONNECTWIFI;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}

	
	return ret;
}

int WIFIAudio_SystemCommand_PowerOnAutoPlay(int enable)
{
	int ret = 0;
	
	if(enable == 0)
	{
		ret = WIFIAudio_SystemCommand_SystemShellCommand("uartdfifo.sh autoplay_close &");
	} else {
		ret = WIFIAudio_SystemCommand_SystemShellCommand("uartdfifo.sh autoplay_open &");
	}
	
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
	
	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(\
	WIFIAUDIO_ALARMCLOCKANDPLAYPLAN_PATHNAME);
	
	return pjsonstring;
}

char* WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan)
{
	char *pjsonstring = NULL;
	char tmp[256];
	
	sprintf(tmp, WIFIAUDIO_PLAYLISTASPLAN_PATHNAME, pAlarmAndPlan->Num);

	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(tmp);
	
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
	char tmp[256];
	
	sprintf(tmp, WIFIAUDIO_PLAYMIXEDLISTASPLAN_PATHNAME, pAlarmAndPlan->Num);

	pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(tmp);
	
	return pjsonstring;
}


int WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth(char *purl)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH;
				pcmdparm->Parameter.pUpdate = (WAFC_pUpdate)calloc(1, sizeof(WAFC_Update));
				if(NULL != pcmdparm->Parameter.pUpdate)
				{
					if(NULL == purl)
					{
						//URL为空，传一个空格下去，这样对脚本也不会有影响
						pcmdparm->Parameter.pUpdate->pURL = strdup(" ");
					} else {
						pcmdparm->Parameter.pUpdate->pURL = strdup(purl);
					}
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				} else {
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			} else {
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		} else {
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	} else {
		ret = -1;
	}
	
	return ret;
}

int WIFIAudio_SystemCommand_GetWiFiAndBluetoothUpgradeState(void)
{
	int State = 0;
	char TempBuff[128];
	FILE *fp = NULL;
	
	fp = popen("cdb get wifi_bt_update_flage", "r");
	if(NULL == fp)
	{
		State = -1;
	} else {
		memset(TempBuff, 0, sizeof(TempBuff));
		if(NULL != fgets(TempBuff, sizeof(TempBuff), fp))
		{
			State = atoi(strtok(TempBuff, "\r\n"));
		}
		pclose(fp);
		fp = NULL;
	}
	
	return State;
}

int WIFIAudio_SystemCommand_GetWiFiDownloadProgress(void)
{
	int Progress = 0;
	int sem_id = 0;
	char *pTmp_Download = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		//打开文件获取前，先进行信号量加锁
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_WIFIDOWNLOADPROGRESS_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
			
		if(NULL != (pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIANDBLUETOOTHDOWNLOADPROGRESS_PATHNAME, 10)))
		{
			Progress = (pTmp_Download[0] > 100)?100:pTmp_Download[0];
			free(pTmp_Download);
			pTmp_Download = NULL;
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetWiFiBurnProgress(void)
{
	int Progress = 0;
	char *pTmp_Upgrade = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		if(NULL != (pTmp_Upgrade = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIANDBLUETOOTHUPGRADEPROGRESS_PATHNAME, 10)))
		{
			Progress = atoi(strtok(pTmp_Upgrade, " .%%"));
			
			free(pTmp_Upgrade);
			pTmp_Upgrade = NULL;
		}
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothDownloadProgress(void)
{
	int Progress = 0;
	int sem_id = 0;
	char *pTmp_Download = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		//打开文件获取前，先进行信号量加锁
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		if(NULL != (pTmp_Download = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIANDBLUETOOTHDOWNLOADPROGRESS_PATHNAME, 10)))
		{
			Progress = (pTmp_Download[0] > 100)?100:pTmp_Download[0];
			free(pTmp_Download);
			pTmp_Download = NULL;
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

int WIFIAudio_SystemCommand_GetBluetoothBurnProgress(void)
{
	int Progress = 0;
	char *pTmp_Upgrade = NULL;
	int i = 0;
	
	//规避读的过程中遇到写
	for(i = 0; i < 5; i++)
	{
		if(NULL != (pTmp_Upgrade = WIFIAudio_SystemCommand_GetOneLineFromFile(WIFIAUDIO_WIFIANDBLUETOOTHUPGRADEPROGRESS_PATHNAME, 10)))
		{
			Progress = atoi(strtok(pTmp_Upgrade, " .%%"));
			
			free(pTmp_Upgrade);
			pTmp_Upgrade = NULL;
		}
		
		if(Progress != 0)
		{
			//下载进度获取到值，直接弹出
			break;
		} else {
			usleep(50000);
		}
	}
	
	return Progress;
}

WARTI_pStr WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr(void)
{
	WARTI_pStr pstr = NULL;
	int State = 0;
	int Progress = 0;
	
	State = WIFIAudio_SystemCommand_GetWiFiAndBluetoothUpgradeState();
	
	switch(State)
	{
		//升级中才会有进度
		case 4:
			//WiFi升级中
			Progress = WIFIAudio_SystemCommand_GetWiFiBurnProgress();
			break;
			
		case 5:
			//蓝牙升级中
			Progress = WIFIAudio_SystemCommand_GetBluetoothBurnProgress();
			break;
			
		case 6:
			//WiFi固件下载中
			Progress = WIFIAudio_SystemCommand_GetWiFiDownloadProgress();
			break;
			
		case 7:
			//蓝牙固件下载中
			Progress = WIFIAudio_SystemCommand_GetBluetoothDownloadProgress();
			break;
			
		default:
			break;
	}
	
	pstr = (WARTI_pStr)calloc(1, sizeof(WARTI_Str));
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	} else {
		pstr->type = WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE;
		
		pstr->Str.pGetUpgradeState = (WARTI_pGetUpgradeState)calloc(1, sizeof(WARTI_GetUpgradeState));
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
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

int WIFIAudio_SystemCommand_RemoveShortcutKeyList(int key)
{
	int ret = 0;
	char tmp[32];
	
	if((key < 1) || (key > 9))
	{
		ret = -1;
	} else {
		memset(tmp, 0, sizeof(tmp));
	
		sprintf(tmp, "Collection %d &", key);
		
		WIFIAudio_SystemCommand_SystemShellCommand(tmp);
	}
	
	return ret;
}

char* WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString(int key)
{
	char *pjsonstring = NULL;
	char tmp[256];
	
	if((key < 1) || (key > 9))
	{
		pjsonstring = NULL;
	} else {
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, WIFIAUDIO_SHORTCUTKEYLIST_PATHNAME, key);
		
		pjsonstring = WIFIAudio_SystemCommand_JsonSringAddRetOk(tmp);
	}
	
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

