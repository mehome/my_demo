#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <netinet/in.h>
#include <arpa/inet.h>//IP地址转换所需要的头文件
#include <time.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

//#include "filepathnamesuffix.h"
#include "WIFIAudio_RealTimeInterface.h"
#include "WIFIAudio_FifoCommunication.h"
#include "WIFIAudio_Semaphore.h"
#include "WIFIAudio_UciConfig.h"


#define WADB_PID "/tmp/WADB.pid"
//程序上锁文件，防止多次启动

int start_record;
int abnormal_record;
int second_record;

struct timeval tv_record_start;
struct timeval tv_record_now;

//防止多次启动
int WIFIAudio_DownloadBurn_LockPid(void)
{
	int fd = -1;
	struct flock lock;
	//char buf[32];
	//int len;
	//pid_t pid;
	
	fd = open(WADB_PID, O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		exit(1);
	}
	
	bzero(&lock, sizeof(lock));
	if (fcntl(fd, F_GETLK, &lock) < 0)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		exit(1);
	}
	
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &lock) < 0)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		exit(1);
	}
	
	return 0;
}

int WIFIAudio_DownloadBurn_ConnectAp(char *pssid, int encode, char *ppassword)
{
	int ret = 0;
	char str[128];
	char temp[128];
	
	if((pssid == NULL) || (ppassword == NULL))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));

		if(0 == encode)
		{
			sprintf(str, "elian.sh connect \'%s\' &", pssid);
		} else {
			sprintf(str, "elian.sh connect \'%s\' \'%s\' &", pssid, ppassword);
		}

#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_PlayIndex(int index)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "mpc play %d &", index);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_GetLocalWifiIp(struct in_addr *pip)
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

//设置SSID功能
int WIFIAudio_DownloadBurn_setSSID(char *pssid)
{
	int ret = 0;
	char str[128];
	
	if(NULL == pssid)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "nvram set test1 elian_ssid \"%s\"", pssid);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif

#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "nvram commit");
#else
		ret = ret | system("nvram commit");
#endif
		sleep(2);
		
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "reboot");
#else
		ret = ret | system("reboot");
#endif
	}
	
	return ret;
}

//设置Password功能,0为不设置密码，其他值则地址有效
int WIFIAudio_DownloadBurn_setNetwork(int flag, char *ppassword)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	if(0 == flag)
	{
		sprintf(str, "nvram set test1 elian_passwd \"\"");//为0的时候不设置密码
	} else {
		sprintf(str, "nvram set test1 elian_passwd \"%s\"", ppassword);
	}
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "nvram commit");
#else
	ret = ret | system("nvram commit");
#endif
	sleep(2);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "reboot");
#else
	ret = ret | system("reboot");//这边先这样放着，还没写绝对路径
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_restoreToDefault(void)
{
	int ret = 0;
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "factoryReset.sh &");
#else
	ret = ret | system("factoryReset.sh &");
#endif
	
	sleep(2);
	
	return ret;
}

int WIFIAudio_DownloadBurn_reboot(void)
{
	int ret = 0;
	
	sleep(2);
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "reboot");
#else
	ret = system("reboot");
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_setDeviceName(char *pname)
{
	int ret = 0;
	char str[128];
	
	if(NULL == pname)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "upmpdcliChangeName.sh  '%s' &", pname);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_DownloadBurn(char *purl)
{
	int ret = 0;
	char str[512];
	
	if(NULL == purl)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		
		//正常的URL长度至少都要5个字节吧
		if(strlen(purl) >= 5)
		{
			sprintf(str, "ota '%s' &", purl);
		} else {
			sprintf(str, "ota &");
		}

#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_RemoteBluetoothUpdate(char *purl)
{
	int ret = 0;
	char str[512];
	
	if(NULL == purl)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		
		//正常的URL长度至少都要5个字节吧
		if(strlen(purl) >= 5)
		{
			sprintf(str, "btup '%s' &", purl);
		} else {
			sprintf(str, "btup &");
		}

#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_GetWiFiAndBluetoothUpgradeState(void)
{
	int State = 0;
	char TempBuff[128];
	FILE *fp = NULL;

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "cdb get wifi_bt_update_flage");
#endif
	
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

int WIFIAudio_DownloadBurn_IsWiFiAndBluetoothUpgrade()
{
	int ret = 0;
	int State = 0;
	char TempBuff[128];
	FILE *fp = NULL;
	
	State = WIFIAudio_DownloadBurn_GetWiFiAndBluetoothUpgradeState();
	
	if((State >= 4) && (State <= 9))
	{
		ret = 1;
	} else {
		ret = 0;
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_HtmlUpdate(char *pPath)
{
	int ret = 0;
	char tempbuff[256];
	
	if(NULL == pPath)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		
		sprintf(tempbuff, "ota 3 %s &", pPath);
		
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tempbuff);
#else
		ret = system(tempbuff);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_UpgradeWiFiAndBluetooth(char *purl)
{
	int ret = 0;
	char str[512];
	
	if(NULL == purl)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		
		if(strlen(purl) >= 5)
		{
			sprintf(str, "ota '%s' &", purl);
		} else {
			sprintf(str, "ota &");
		}
		
		if(1 != WIFIAudio_DownloadBurn_IsWiFiAndBluetoothUpgrade())
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
			ret = system(str);
#endif
		}
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_PlayMusicList(void)
{
	int ret = 0;
	char tempbuff[256];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "creatPlayList replacePlaylist %s &", \
	WIFIAUDIO_POSTPLAYLIST_PATHNAME);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tempbuff);
#else
	ret = system(tempbuff);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_PlayMusicListEx(void)
{
	int ret = 0;
	char tempbuff[256];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "creatPlayList loadplaylist %s &", \
	WIFIAUDIO_POSTPLAYLISTEX_PATHNAME);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tempbuff);
#else
	ret = system(tempbuff);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_InsertMusicList(void)
{
	int ret = 0;
	char tempbuff[256];
	
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "creatPlayList loadSongs %s &", \
	WIFIAUDIO_POSTINSERTLIST_PATHNAME);
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tempbuff);
#else
	system(tempbuff);
#endif

	return ret;
}

#define UNIX_DOMAIN "/tmp/UNIX.domain"

int OnWriteMsgToAlexa(char* cmd)
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
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}
	
	flags = fcntl(iConnect_fd, F_GETFL, 0);
	fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);
	
	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}
	
	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	} else {
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

int WIFIAudio_DownloadBurn_LoginAmazon(void)
{
	int ret = 0;
	
	OnWriteMsgToAlexa("GetToken");
	
	return ret;
}

int WIFIAudio_DownloadBurn_SetRecord(int value)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "xzxAction.sh setrt %d &", (value > 0) ? value : 5);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_PlayRecord(void)
{
	int ret = 0;
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, "xzxAction.sh playrec &");
#else
	ret = system("xzxAction.sh playrec &");
#endif	

	return ret;
}

int WIFIAudio_DownloadBurn_PlayTfCard(int index)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "creatPlayList playTF %d &", index);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_PlayUsbDisk(int index)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "creatPlayList playUSB %d &", index);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_StartSynchronizePlayEx(void)
{
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
		ret = -1;
	} else {
		pjsonstring = (char *)calloc(1, (filestat.st_size) * sizeof(char));
		if(NULL == pjsonstring)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
		} else {
			ret =fread(pjsonstring, sizeof(char), filestat.st_size, fp);
		}
		fclose(fp);
		fp = NULL;
	}
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	if(ret > 0)
	{
		pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX, pjsonstring);
		free(pjsonstring);
		pjsonstring = NULL;
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
		} else {
			if(0 == pstr->Str.pPostSynchronousInfoEx->Slave)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, \
				"(/etc/init.d/snapserver start;/etc/init.d/snapclient start 10.10.10.254)&");
#else
				system("(/etc/init.d/snapserver start;/etc/init.d/snapclient start 10.10.10.254)&");
#endif
			} else if(1 == pstr->Str.pPostSynchronousInfoEx->Slave) {
				//避免作从音箱的时候，下次要作为主音箱又启动不了
				memset(tempbuff, 0, sizeof(tempbuff));
				if(NULL == pstr->Str.pPostSynchronousInfoEx->pMaster_Password)
				{
					sprintf(tempbuff, "/etc/init.d/snapclient p2pStart '%s' &", \
					pstr->Str.pPostSynchronousInfoEx->pMaster_SSID);
				} else {
					sprintf(tempbuff, "/etc/init.d/snapclient p2pStart '%s' '%s' &", \
					pstr->Str.pPostSynchronousInfoEx->pMaster_SSID, \
					pstr->Str.pPostSynchronousInfoEx->pMaster_Password);
				}
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tempbuff);
#else
				system(tempbuff);
#endif
			}
			
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		}
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_ShutdownSec(int second)
{
	int ret = 0;
	time_t timep;
	struct tm *p;
	char str[128];
	
	memset(str, 0, sizeof(str));
	
	if(-1 == second)
	{
		sprintf(str, "powoff.sh disable &");
	} else if(0 == second) {
		sprintf(str, "(powoff.sh settime %d;powoff.sh enable) &", second + 1);
	} else {
		sprintf(str, "(powoff.sh settime %d;powoff.sh enable) &", second);
	}

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif

	return ret;
}

int WIFIAudio_DownloadBurn_PlayUrlRecord(char *precurl)
{
	int ret = 0;
	char str[512];
	
	if(NULL == precurl)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "mpg123.sh %s &", precurl);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_StartRecord(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "uartdfifo.sh testtlkon &");

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_FixedRecord(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "uartdfifo.sh timetlk &");

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_VoiceActivityDetection(int value, char *pcardname)
{
	int ret = 0;
	char str[128];
	
	if(NULL == pcardname)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "vadthreshold %s %d &", pcardname, value);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_SetMultiVolume(int value, char *pmac)
{
	int ret = 0;
	char str[128];
	
	if(NULL == pmac)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "snapmulti.sh setVolume \"%s\" \"%d\"", pmac, value);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
		ret = system(str);
#endif
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_SearchBluetooth(void)
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "uartdfifo.sh btsearch &");

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_ConnectBluetooth(char *paddress)
{
	int ret = 0;
	char str[128];
	FILE *fp = NULL;
	char *ptmp = NULL;
	char num[8];
	int index;
	
	if(NULL == paddress)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(str, 0, sizeof(str));
		sprintf(str, "grep %s %s", paddress, WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME);
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#endif
		if(NULL == (fp = popen(str, "r")))
		{
			ret = -1;
		} else {
			memset(str, 0, sizeof(str));
			if(NULL == fgets(str, sizeof(str), fp))
			{
				ret = -1;
			} else {
				ptmp = strtok(str, "+");
				if(NULL == ptmp)
				{
					ret = -1;
				} else {
					strcpy(num, ptmp);
					index = atoi(num)%10;
					
					memset(str, 0, sizeof(str));
					sprintf(str, "uartdfifo.sh \"btin %02d %s\" &", index, paddress);

#ifdef WIFIAUDIO_DEBUG
					syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
					ret = system(str);
#endif
				}
			}
			pclose(fp);
			fp = NULL;
		}
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_UpdateBluetooth(void)
{
	int ret = 0;
	char str[256];
	
	sprintf(str, "btup 3 %s &", WIFIAUDIO_UPDATEBLUETOOTH_PATHNAME);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_LedMatrixScreen(int Animation, int Num, int Interval)
{
	int ret = 0;
	char str[128];
	int sem_id = 0;//信号量ID
	
	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);
	
	memset(str, 0, sizeof(str));
	sprintf(str, "killall WIFIAudio_LightControl_LedMatrixScreen");
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	WIFIAudio_Semaphore_Operation_V(sem_id);
	
	sprintf(str, "WIFIAudio_LightControl_LedMatrixScreen %d %d %d &", Animation, Num, Interval);
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret |= system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_UpdataConexant(void)
{
	int ret = 0;
	char str[256];
	
	sprintf(str, "cxdish -D /dev/i2c-1 flash %s %s &",\
	WIFIAUDIO_CONEXANTSFS_PATHNAME, WIFIAUDIO_CONEXANTBIN_PATHNAME);

#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_NormalRecord(int second)
{
	int ret = 0;
	char str[256];
	
	gettimeofday(&tv_record_start, NULL);
	start_record = 1;
	abnormal_record = 0;
	second_record = second;
	
	memset(str, 0, sizeof(str));
	sprintf(str, "(killall wakeup;arecord -D plughw:1,0 -f s16_le -c 2 -r 16000 %s) &", \
	WIFIAUDIO_NORMALINSYSTEM_PATHNAME);
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_MicRecord(int second)
{
	int ret = 0;
	char str[256];
	
	gettimeofday(&tv_record_start, NULL);
	start_record = 1;
	abnormal_record = 1;
	second_record = second;
	
	memset(str, 0, sizeof(str));
	sprintf(str, "(cxdish set-mode ZSH3;killall wakeup;arecord -D plughw:1,0 -f s16_le -c 2 -r 16000 %s) &", \
	WIFIAUDIO_MICINSYSTEM_PATHNAME);
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_RefRecord(int second)
{
	int ret = 0;
	char str[256];
	
	gettimeofday(&tv_record_start, NULL);
	start_record = 1;
	abnormal_record = 1;
	second_record = second;
	
	memset(str, 0, sizeof(str));
	sprintf(str, "(cxdish set-mode ZSH4;killall wakeup;arecord -D plughw:1,0 -f s16_le -c 2 -r 16000 %s) &", \
	WIFIAUDIO_REFINSYSTEM_PATHNAME);
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_stopModeRecord(void)
{
	int ret = 0;
	
	//如果还在模式录音，下一个100毫秒就直接超时了
	tv_record_start.tv_sec = 0;
	
	return ret;
}

int WIFIAudio_DownloadBurn_DisconnectWiFi()
{
	int ret = 0;
	char str[128];
	
	memset(str, 0, sizeof(str));
	sprintf(str, "sleep 1;elian.sh disconnect");
	
#ifdef WIFIAUDIO_DEBUG
	syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, str);
#else
	ret = system(str);
#endif
	
	return ret;
}

int WIFIAudio_DownloadBurn_CreateShortcutKeyList(void)
{
	int ret = 0;
	int key = 0;
	WARTI_pStr pstr = NULL;
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	struct stat filestat;
	char tmp[256];
	
	stat(WIFIAUDIO_TMPSHORTCUTKEYLIST_PATHNAME, &filestat);
	
	if(NULL == (fp = fopen(WIFIAUDIO_TMPSHORTCUTKEYLIST_PATHNAME, "r")))
	{
		ret = -1;
	} else {
		pjsonstring = (char *)calloc(1, filestat.st_size);
		if(NULL == pjsonstring)
		{
			ret = -1;
		} else {
			ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
		}
		fclose(fp);
		fp = NULL;
	}
	
	if(ret > 0)
	{
		pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_SHORTCUTKEYLIST, pjsonstring);
		free(pjsonstring);
		pjsonstring = NULL;
		if(NULL == pstr)
		{
			ret = -1;
		} else {
			key = pstr->Str.pShortcutKeyList->CollectionNum;
			
			WIFIAudio_RealTimeInterface_pStrFree(&pstr);
			
			if((key >= 1) && (key <= 9))
			{
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "Collection %d \"%s\" &", key, WIFIAUDIO_TMPSHORTCUTKEYLIST_PATHNAME);

#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, tmp);
#else
				ret = system(tmp);
#endif
			}
		}
	}
	
	return ret;
}

int WIFIAudio_DownloadBurn_GetCompleteCommandHandle(int fdfifo, WAFC_pFifoBuff pfifobuff,int len)
{
	int ret = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	
	WIFIAudio_FifoCommunication_GetCompleteCommand(fdfifo, pfifobuff, len);
	
	while(1)
	{
		if(NULL != pcmdparm)
		{
			WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			pcmdparm = NULL;
		}
		
		if(pfifobuff->CurrentLen > (2 + 5 + 2))//0xAA,0xBB,0xCC,0xDD,整个命令长度固定5个字节
		{
			pcmdparm = WIFIAudio_FifoCommunication_FifoBuffToCmdParm(pfifobuff);
		} else {
			break;
		}
		
		if(pcmdparm != NULL)
		{
			//有获取到一个完整指令
			switch(pcmdparm->Command)
			{
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP:
					WIFIAudio_DownloadBurn_ConnectAp(pcmdparm->Parameter.pConnectAp->pSSID, \
					pcmdparm->Parameter.pConnectAp->Encode, pcmdparm->Parameter.pConnectAp->pPassword);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX:
					WIFIAudio_DownloadBurn_PlayIndex(pcmdparm->Parameter.pPlayIndex->Index);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID:
					WIFIAudio_DownloadBurn_setSSID(pcmdparm->Parameter.pSetSSID->pSSID);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK:
					WIFIAudio_DownloadBurn_setNetwork(pcmdparm->Parameter.pSetNetwork->Flag, \
					pcmdparm->Parameter.pSetNetwork->pPassword);
					break;
				
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_RESTORETODEFAULT:
					WIFIAudio_DownloadBurn_restoreToDefault();
					break;
				
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REBOOT:
					WIFIAudio_DownloadBurn_reboot();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME:
					WIFIAudio_DownloadBurn_setDeviceName(pcmdparm->Parameter.pName->pName);
					break;
				
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE:
					WIFIAudio_DownloadBurn_DownloadBurn(pcmdparm->Parameter.pUpdate->pURL);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_HTMLUPDATE:
					WIFIAudio_DownloadBurn_HtmlUpdate(pcmdparm->Parameter.pHtmlUpdate->pPath);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALY:
					WIFIAudio_DownloadBurn_PlayMusicList();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALYEX:
					WIFIAudio_DownloadBurn_PlayMusicListEx();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_INSERTSOMEMUSICLIST:
					WIFIAudio_DownloadBurn_InsertMusicList();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LOGINAMAZON:
					WIFIAudio_DownloadBurn_LoginAmazon();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD:
					WIFIAudio_DownloadBurn_SetRecord(pcmdparm->Parameter.pSetRecord->Value);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYRECORD:
					WIFIAudio_DownloadBurn_PlayRecord();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD:
					WIFIAudio_DownloadBurn_PlayTfCard(pcmdparm->Parameter.pPlayTfCard->Index);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK:
					WIFIAudio_DownloadBurn_PlayUsbDisk(pcmdparm->Parameter.pPlayUsbDisk->Index);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STATRSYNCHRONIZEPLAYEX:
					WIFIAudio_DownloadBurn_StartSynchronizePlayEx();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN:
					WIFIAudio_DownloadBurn_ShutdownSec(pcmdparm->Parameter.pShutdownSec->Second);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD:
					WIFIAudio_DownloadBurn_PlayUrlRecord(pcmdparm->Parameter.pPlayUrlRecord->pUrl);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STARTRECORD:
					WIFIAudio_DownloadBurn_StartRecord();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_FIXEDRECORD:
					WIFIAudio_DownloadBurn_FixedRecord();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION:
					WIFIAudio_DownloadBurn_VoiceActivityDetection(\
					pcmdparm->Parameter.pVoiceActivityDetection->Value, \
					pcmdparm->Parameter.pVoiceActivityDetection->pCardName);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME:
					WIFIAudio_DownloadBurn_SetMultiVolume(\
					pcmdparm->Parameter.pSetMultiVolume->Value, \
					pcmdparm->Parameter.pSetMultiVolume->pMac);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SEARCHBLUETOOTH:
					WIFIAudio_DownloadBurn_SearchBluetooth();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH:
					WIFIAudio_DownloadBurn_ConnectBluetooth(\
					pcmdparm->Parameter.pConnectBluetooth->pAddress);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATEBLUETOOTH:
					WIFIAudio_DownloadBurn_UpdateBluetooth();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LEDMATRIXSCREEN:
					WIFIAudio_DownloadBurn_LedMatrixScreen(\
					pcmdparm->Parameter.pLedMatrixScreen->Animation, \
					pcmdparm->Parameter.pLedMatrixScreen->Num, \
					pcmdparm->Parameter.pLedMatrixScreen->Interval);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATACONEXANT:
					WIFIAudio_DownloadBurn_UpdataConexant();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD:
					WIFIAudio_DownloadBurn_NormalRecord(pcmdparm->Parameter.pSetRecord->Value);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD:
					WIFIAudio_DownloadBurn_MicRecord(pcmdparm->Parameter.pSetRecord->Value);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD:
					WIFIAudio_DownloadBurn_RefRecord(pcmdparm->Parameter.pSetRecord->Value);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STOPMODERECORD:
					WIFIAudio_DownloadBurn_stopModeRecord();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE:
					WIFIAudio_DownloadBurn_RemoteBluetoothUpdate(pcmdparm->Parameter.pUpdate->pURL);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_DISCONNECTWIFI:
					WIFIAudio_DownloadBurn_DisconnectWiFi();
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH:
					WIFIAudio_DownloadBurn_UpgradeWiFiAndBluetooth(pcmdparm->Parameter.pUpdate->pURL);
					break;
					
				case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CREATESHORTCUTKEYLIST:
					WIFIAudio_DownloadBurn_CreateShortcutKeyList();
					break;
					
				default:
					break;
			}
			WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			pcmdparm = NULL;
		}
	}
	
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fdfifo = 0;
	int len = 0;
	fd_set readset;//轮询的可读集合
	struct timeval timeout;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	//防止多次启动
	WIFIAudio_DownloadBurn_LockPid();
	
	//因为这边录音要自己控制时间
	//这边设置一个全局标志
	start_record = 0;
	abnormal_record = 0;
	second_record = 0;
	
	//这边进来的时候先创建管道
	WIFIAudio_FifoCommunication_CreatFifo(WIFIAUDIO_FIFO_PATHNAME);
	
	fdfifo = WIFIAudio_FifoCommunication_OpenReadAndWriteFifo(WIFIAUDIO_FIFO_PATHNAME);
	
	len = 1024;
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(len);
		if(NULL != pfifobuff)
		{
			while(1)
			{
				FD_ZERO(&readset);
				FD_SET(fdfifo, &readset);
				timeout.tv_sec = 0;
				timeout.tv_usec = 100000;//100毫秒
				select(fdfifo + 1, &readset, NULL, NULL, &timeout);//这边不关心写，错误，等待时间
				if(FD_ISSET(fdfifo, &readset))
				{
					//select返回的时候，会将没有等待到的时间对应位清零
					//所以这个时候只要来判断一下一开始加入的文件描述符现在还是否为置位状态就可以知道是否有等待的时间发生
					
					//获取命令并处理
					WIFIAudio_DownloadBurn_GetCompleteCommandHandle(fdfifo, pfifobuff, len);
				} else {
					if(1 == start_record)
					{
						gettimeofday(&tv_record_now, NULL);
						if((tv_record_now.tv_sec - tv_record_start.tv_sec >= second_record))
						{
							system("killall arecord &");
							if(0 == abnormal_record)
							{
								//正常模式录音
							} else {
								//Mic录音和参考信号录音
								//切换回正常模式
								system("cxdish set-mode ZSH2 &");
							}
							start_record = 0;
							abnormal_record = 0;
							second_record = 0;
						}
					}
				}
			}
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		}
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	}
	
	return 0;
}

