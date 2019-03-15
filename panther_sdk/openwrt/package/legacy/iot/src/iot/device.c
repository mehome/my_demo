#include <stdio.h>
#include <sys/vfs.h>
#include <sys/socket.h>  
#include <sys/ioctl.h>  
#include <netinet/if_ether.h>  
#include <net/if.h>  
#include <pthread.h>
#include <cdb.h>
#include <strings.h>


#include "aes.h"
#include "myutils/debug.h"
#include "common.h"

#include "DeviceStatus.h"
#include "AlertHead.h"
#include "AlertHead.h"

static pthread_mutex_t deviceStatusMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t  deviceStatusCond = PTHREAD_COND_INITIALIZER; 

DeviceStatus deviceStatus;

#if defined	ENABLE_YIYA 
#define DEVICE_NAME "br0"
#elif defined ENABLE_LVMENG
#define DEVICE_NAME "br0"
#elif defined ENABLE_HUABEI
#define DEVICE_NAME "br0"
#elif defined ENABLE_COMMSAT
#define DEVICE_NAME "br0"
#else
#define DEVICE_NAME "br0"
#endif



int GetPlayState()
{	int play = 0;
	int stat = 0;
	stat = MpdGetPlayState();
	debug("stat:%d",stat);
	if(stat == 3)
		play = 5;
	else if (stat == 2) {
		int local = MpdCliPlayLocalOrUri();
		debug("local:%d",local);
		if(local == 1) {
			play = 1;
		} else if (local == 2){
			play = 4;
		}	
	}

	
	return play;
}


int GetDeviceVolume()
{	int vol=50;
	char *volume = NULL;

	//volume = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.volume");
	//if(volume) {
	//	vol = atoi(volume);
	//}
	//MpdUpdateStatus();
	vol = MpdGetVolume();
	debug("vol:%d",vol);
	return vol;
}

int GetDeviceBattery()
{
	int battery = 5;
	char * power = NULL;
	//power = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.battery");
	battery = cdb_get_int("$battery_level",10);
	return battery*10;
}
int GetDeviceCharging()
{
	int charging = 0;
	//pCharging = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.charge");
    charging = cdb_get_int("$charge_plug",0);
	return charging;
	
}
int GetDeviceLowPower()
{
	int low = 0;
	int battery = 0;
	//power = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.battery");
	battery = cdb_get_int("$battery_level",10);

    if(battery <= 2)
        low = 1;

	return low;
	
}

int GetDeviceShake()
{
	int shake = 0;
	return shake;
}

int GetDeviceBln()
{
	int blnValue = 0;
	int battery = 0;
	char * bln = NULL;
	//bln = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.bln");
    blnValue = cdb_get_int("$bln",10);

    if( blnValue != 0 )
        blnValue = 1;

    return blnValue;
}


int GetDeviceSFree(void)
{
	unsigned long long freeDisk;
	size_t mbFreedisk;
	freeDisk = GetDeviceFreeSize("/overlay");
	mbFreedisk = freeDisk>>10;  
    debug (" free=%dKB\n", mbFreedisk);  
	
	return mbFreedisk;
}

int GetDeviceSTotal()
{
	size_t mbTotalsize ;
	unsigned long totalBlocks;
	unsigned long totalSize;

	totalSize = GetDeviceTotalSize("/overlay");  
    mbTotalsize = totalSize>>10;  
  
    debug(" total=%dKB", mbTotalsize);  
	return mbTotalsize;
}

unsigned char * GetUserID(char *apiKey, char *aesKey)
{
	char *device = DEVICE_NAME; //teh0是网卡设备名  
	char temp_id[128] = {0};
	char aes_iv[17] = {0};
	unsigned char macaddr[ETH_ALEN]; //ETH_ALEN（6）是MAC地址长度  
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0); //建立套接口  
	struct ifreq req;  
	int err,rc;  
	rc  = strcpy(req.ifr_name, device); //将设备名作为输入参数传入  
	err = ioctl(s, SIOCGIFHWADDR, &req); //执行取MAC地址操作  
	close(s);  
	memcpy(aes_iv, apiKey,16);
#ifndef ENABLE_HUABEI
	sprintf(temp_id, "%s", "aiAA");
#else
	sprintf(temp_id, "%s", "draw");
#endif
	if (err != -1 )  
	{  
		memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); //取输出的MAC地址  
		for(i = 0; i < ETH_ALEN; i++)  
		{
			sprintf(&temp_id[(2+i) * 2], "%02x", macaddr[i]);
		}
	}

	debug("temp_id:%s", temp_id);


	unsigned char out[64] = {'0'};
	unsigned char *pData = malloc(128);
	memset(pData, 0, 128);	
	debug("apiKey:%s",apiKey);
	debug("temp_id:%s",temp_id);
	debug("aes_key:%s",aesKey);
	debug("aes_iv:%s",aes_iv);
	AES128_CBC_encrypt_buffer(out, temp_id, 16, aesKey, aes_iv);
	debug("out:%d",strlen(out));
    debug("Encrypt: ");
	 //for(i = 0;out[i] != '\0';i++)
    for(i = 0; i<16; i++)
	{
		sprintf(&pData[i*2], "%02x", out[i]);
    }
	debug("pData:%s",pData);
	return pData;
}


int GetDeviceID(char *id)
{

	char *device = DEVICE_NAME; 
	unsigned char macaddr[ETH_ALEN]; 
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0); 
	struct ifreq req;  
	int err,rc;  
	rc  = strcpy(req.ifr_name, device); 
	err = ioctl(s, SIOCGIFHWADDR, &req); 
	close(s);  
#ifndef ENABLE_HUABEI
	sprintf(id, "%s", "aiAA");
#else
	sprintf(id, "%s", "draw");
#endif
	if (err != -1 )  
	{  
		memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); 
		for(i = 0; i < ETH_ALEN; i++)  
		{
			sprintf(&id[(2+i) * 2], "%02x", macaddr[i]);
		}
	}
	return 0;
}

int GetDeviceLbi()
{
	//char *lbi = NULL;
	int lbiValue = 0;
	//lbi = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.lbi");
    lbiValue = cdb_get_int("$lbi",1);

    if( lbiValue != 0)
        lbiValue = 1;

	return lbiValue;
}

int GetDeviceTcard()
{
	int status; 
	status = cdb_get_int("$tf_status", 0);
	return status;
}

int GetDeviceSleepStatus(SleepStatus *status)
{
	Alert *alertBed = NULL;
	Alert *alertWake = NULL;

	alertBed = GetAlert("turing_bed");
	alertWake = GetAlert("turing_wake");

	if(alertBed && alertWake) 
	{
		status->bed = atol(alert_get_scheduledtime(alertBed));
		status->wake = atol(alert_get_scheduledtime(alertWake));
	} 
	
	return 0;
}

void   NewDeviceStatus(DeviceStatus *status)
{
	memset(status, 0, sizeof(DeviceStatus));
}

void GetDeviceStatus(DeviceStatus *status)
{	
	memset(status, 0, sizeof(DeviceStatus));
	status->vol     	= GetDeviceVolume();	//音量
	status->battery		= GetDeviceBattery();	//电池电量
	status->sfree   	= GetDeviceSFree();		//当前内存存储情况
	status->stotal		= GetDeviceSTotal();	//总内存容量情况
	status->play   	 	= GetPlayState();		//歌曲状态
	status->bln			= GetDeviceBln();		//呼吸灯开关
	status->shake 		= GetDeviceShake();
	status->power 		= GetDeviceLowPower();	//低电状态
	status->charging 	= GetDeviceCharging();	//是否充电状态
	status->lbi 		= GetDeviceLbi();
	status->tcard 		= GetDeviceTcard();		//tf卡插拔状态
	GetDeviceSleepStatus(&(status->sleepStatus));
}

