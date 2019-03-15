#ifndef __WIFIAUDIO_FIFOCOMMUNICATION_H__
#define __WIFIAUDIO_FIFOCOMMUNICATION_H__

#ifdef __cplusplus
extern "C"
{
#endif


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>//IP地址转换所需要的头文件

#include "filepathnamesuffix.h"

#define WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1 (0xAA)
#define WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2 (0xBB)

#define WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR ('\n')
//每个参数的间隔符

#define WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG1 (0xCC)
#define WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG2 (0xDD)


typedef struct _WAFC_pFifoBuff
{
	char *pBuff;
	int BuffLen;
	int CurrentLen;
}WAFC_FifoBuff, *WAFC_pFifoBuff;//管道读取缓存

typedef struct _WAFC_ConnectAp
{
	char *pSSID;
	int Encode;
	char *pPassword;
}WAFC_ConnectAp, *WAFC_pConnectAp;

typedef struct _WAFC_PlayIndex
{
	int Index;
}WAFC_PlayIndex, *WAFC_pPlayIndex;

typedef struct _WAFC_setSSID
{
	char *pSSID;
}WAFC_setSSID, *WAFC_psetSSID;

typedef struct _WAFC_setNetwork
{
	int Flag;
	char *pPassword;
}WAFC_setNetwork, *WAFC_psetNetwork;

typedef struct _WAFC_Name
{
	char *pName;
}WAFC_Name, *WAFC_pName;

typedef struct _WAFC_Update
{
	char *pURL;
}WAFC_Update, *WAFC_pUpdate;

typedef struct _WAFC_HtmlUpdate
{
	char *pPath;
}WAFC_HtmlUpdate, *WAFC_pHtmlUpdate;

typedef struct _WAFC_SetRecord
{
	int Value;
}WAFC_SetRecord, *WAFC_pSetRecord;

typedef struct _WAFC_PlayTfCard
{
	int Index;
}WAFC_PlayTfCard, *WAFC_pPlayTfCard;

typedef struct _WAFC_PlayUsbDisk
{
	int Index;
}WAFC_PlayUsbDisk, *WAFC_pPlayUsbDisk;

typedef struct _WAFC_ShutdownSec
{
	int Second;
}WAFC_ShutdownSec, *WAFC_pShutdownSec;

typedef struct _WAFC_PlayUrlRecord
{
	char *pUrl;
}WAFC_PlayUrlRecord, *WAFC_pPlayUrlRecord;

typedef struct _WAFC_VoiceActivityDetection
{
	int Value;
	char *pCardName;
}WAFC_VoiceActivityDetection, *WAFC_pVoiceActivityDetection;

typedef struct _WAFC_SetMultiVolume
{
	int Value;
	char *pMac;
}WAFC_SetMultiVolume, *WAFC_pSetMultiVolume;

typedef struct _WAFC_ConnectBluetooth
{
	char *pAddress;
}WAFC_ConnectBluetooth, *WAFC_pConnectBluetooth;

typedef struct _WAFC_LedMatrixScreen
{
	int Animation;
	int Num;
	int Interval;
}WAFC_LedMatrixScreen, *WAFC_pLedMatrixScreen;

enum WIFIAUDIO_FIFOCOMMUNICATION_COMMAND
{
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP = 1,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_RESTORETODEFAULT, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REBOOT, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_HTMLUPDATE,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALY,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALYEX,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_INSERTSOMEMUSICLIST,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LOGINAMAZON,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STATRSYNCHRONIZEPLAYEX,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STARTRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_FIXEDRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SEARCHBLUETOOTH,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATEBLUETOOTH,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LEDMATRIXSCREEN,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATACONEXANT,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STOPMODERECORD,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE, 
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_DISCONNECTWIFI,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH,
	WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CREATESHORTCUTKEYLIST,
	//每次新增还是放最底下吧，免得到时候乱了
};

typedef struct _WAFC_CmdParm
{
	enum WIFIAUDIO_FIFOCOMMUNICATION_COMMAND Command;
	union 
	{
		WAFC_pConnectAp pConnectAp;
		WAFC_pPlayIndex pPlayIndex;
		WAFC_psetSSID pSetSSID;
		WAFC_psetNetwork pSetNetwork;
		WAFC_pName pName;
		WAFC_pUpdate pUpdate;
		WAFC_pHtmlUpdate pHtmlUpdate;
		WAFC_pSetRecord pSetRecord;
		WAFC_pPlayTfCard pPlayTfCard;
		WAFC_pPlayUsbDisk pPlayUsbDisk;
		WAFC_pShutdownSec pShutdownSec;
		WAFC_pPlayUrlRecord pPlayUrlRecord;
		WAFC_pVoiceActivityDetection pVoiceActivityDetection;
		WAFC_pSetMultiVolume pSetMultiVolume;
		WAFC_pConnectBluetooth pConnectBluetooth;
		WAFC_pLedMatrixScreen pLedMatrixScreen;
	}Parameter;
}WAFC_CmdParm, *WAFC_pCmdParm;


 
extern int WIFIAudio_FifoCommunication_CreatFifo(char *ppath);

extern int WIFIAudio_FifoCommunication_OpenReadAndWriteFifo(char *ppath);

extern int WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(char *ppath);

extern WAFC_pFifoBuff WIFIAudio_FifoCommunication_NewBlankFifoBuff(int bufflen);

extern int WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(int fdfifo, WAFC_pFifoBuff pfifobuff, int writelen);

extern int WIFIAudio_FifoCommunication_FreeFifoBuff(WAFC_pFifoBuff *ppfifobuff);

//释放命令结构体
extern int WIFIAudio_FifoCommunication_FreeCmdParm(WAFC_pCmdParm *ppcmdparm);

//从管道当中获取一个完整命令buff
extern int WIFIAudio_FifoCommunication_GetCompleteCommand(int fdfifo, WAFC_pFifoBuff pfifobuff, int readlen);

//从管道内存结构体当中，获取一个命令结构体
extern WAFC_pCmdParm WIFIAudio_FifoCommunication_FifoBuffToCmdParm(WAFC_pFifoBuff pfifobuff);

//将命令结构体压缩进去管道内存结构体
extern int WIFIAudio_FifoCommunication_CmdParmToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm);


#ifdef __cplusplus
}
#endif

#endif
