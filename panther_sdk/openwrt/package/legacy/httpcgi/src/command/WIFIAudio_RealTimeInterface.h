#ifndef __WIFIAUDIO_REALTIMEINTERFACE_H__
#define __WIFIAUDIO_REALTIMEINTERFACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <net/ethernet.h>
#include <netinet/ether.h>

#define WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE (~0)

typedef struct _WARTI_GetStatus
{
	char *pLanguage;
	char *pSSID;
	char *pFirmware;
	char *pBuildDate;
	char *pRelease;
	char *pGroup;
	int Expired;
	int Internet;
	char UUID[16];
	int Netstat;
	char *pESSID;
	struct in_addr Apcli0;
	struct in_addr Eth2;
	char *pHardware;
}WARTI_GetStatus,*WARTI_pGetStatus;

typedef struct _WARTI_AccessPoint
{
	char *pSSID;
	struct ether_addr BSSID;
	int RSSI;
	int Channel;
	char *pAuth;
	char *pEncry;
	int Extch;
}WARTI_AccessPoint,*WARTI_pAccessPoint;

typedef struct _WARTI_WLANGetAPlist
{
	char *pRet;
	int Res;
	WARTI_pAccessPoint *pAplist;
}WARTI_WLANGetAPlist,*WARTI_pWLANGetAPlist;

typedef struct _WARTI_GetCurrentWirelessConnect
{
	char *pRet;
	char *pSSID;
	int Connect;
}WARTI_GetCurrentWirelessConnect,*WARTI_pGetCurrentWirelessConnect;

typedef struct _WARTI_GetDeviceInfo
{
	char *pRet;
	int RSSI;
	char *pLanguage;
	char *pSSID;
	char *pFirmware;
	char *pRelease;
	char *pMAC;
	char *pUUID;
	char *pESSID;
	struct in_addr Apcli0;
	char *pProject;
	char *pDeviceName;
	int BatteryPercent;
	char *pLanguageSupport;
	char *pFunctionSupport;	
	int KeyNum;
	char *pBtVersion;
	char *pConexantVersion;	
	char *pPowerSupply;
	char *pProjectUuid;
}WARTI_GetDeviceInfo,*WARTI_pGetDeviceInfo;

typedef struct _WARTI_GetOneTouchConfigure
{
	char *pRet;
	int State;
}WARTI_GetOneTouchConfigure,*WARTI_pGetOneTouchConfigure;

typedef struct _WARTI_GetOneTouchConfigureSetDeviceName
{
	char *pRet;
	int State;
}WARTI_GetOneTouchConfigureSetDeviceName,*WARTI_pGetOneTouchConfigureSetDeviceName;

typedef struct _WARTI_GetLoginAmazonStatus
{
	char *pRet;
	int State;
}WARTI_GetLoginAmazonStatus,*WARTI_pGetLoginAmazonStatus;

typedef struct _WARTI_GetAlexaLanguage
{
	char *pRet;
	char *pLanguage;
}WARTI_GetAlexaLanguage,*WARTI_pGetAlexaLanguage;

typedef struct _WARTI_GetWakeupTimes
{
	char *pRet;
	int times;
}WARTI_GetWakeupTimes,*WARTI_pGetWakeupTimes;

typedef struct _WARTI_GetMicarrayInfo
{
	char *pRet;
	int Mics;
	int Mics_channel;
	int Refs;
	int Refs_channel;
	char *record_dev;
}WARTI_GetMicarrayInfo,*WARTI_pGetMicarrayInfo;

typedef struct _WARTI_MicarrayNormal
{
	char *pRet;
	char *pURL;
}WARTI_MicarrayNormal,*WARTI_pMicarrayNormal;

typedef struct _WARTI_MicarrayUrlList
{
	int Id;
	char *pURL;
}WARTI_MicarrayUrlList,*WARTI_pMicarrayUrlList;

typedef struct _WARTI_MicarrayMic
{
	char *pRet;
	int Num;
	WARTI_pMicarrayUrlList *ppUrlList;
}WARTI_MicarrayMic,*WARTI_pMicarrayMic;

typedef struct _WARTI_MicarrayRef
{
	char *pRet;
	int Num;
	WARTI_pMicarrayUrlList *ppUrlList;
}WARTI_MicarrayRef,*WARTI_pMicarrayRef;

typedef struct _WARTI_LogCaptureEnable
{
	char *pRet;
	char *pURL;
}WARTI_LogCaptureEnable,*WARTI_pLogCaptureEnable;

typedef struct _WARTI_GetPlayerStatus
{
	char *pRet;
	int MainMode;
	int NodeType;
	int Mode;
	int SW;	
	char *pStatus;
	int CurPos;
	int TotLen;
	char *pTitle;
	char *pArtist;
	char *pAlbum;
	int Year;
	int Track;
	char *pGenre;
	int LocalListFlag;
	char *pLocalListFile;
	int PLiCount;
	int PLicurr;
	int Vol;
	int Mute;
	char *pIURI;
	char *pURI;
	char *pCover;
	int SourceMode;
	char *pUUID;
}WARTI_GetPlayerStatus,*WARTI_pGetPlayerStatus;

typedef struct _WARTI_GetPlayerCmdVolume
{
	char *pRet;
	int Volume;
}WARTI_GetPlayerCmdVolume,*WARTI_pGetPlayerCmdVolume;

typedef struct _WARTI_GetPlayerCmdMute
{
	char *pRet;
	int Mute;
}WARTI_GetPlayerCmdMute,*WARTI_pGetPlayerCmdMute;

typedef struct _WARTI_Music
{
	int Index;
	char *pTitle;
	char *pArtist;
	char *pAlbum;
	char *pContentURL;
	char *pCoverURL;
	char *pPlatform;
	int Column;
	int Program;
}WARTI_Music,*WARTI_pMusic;

typedef struct _WARTI_MusicList
{
	char *pRet;
	int Num;
	int Index;
	int NowPlay;
	WARTI_pMusic *pMusicList;
}WARTI_MusicList,*WARTI_pMusicList;

typedef struct _WARTI_GetEqualizer
{
	char *pRet;
	int Mode;
}WARTI_GetEqualizer,*WARTI_pGetEqualizer;

typedef struct _WARTI_GetShutdown
{
	char *pRet;
	int Second;
}WARTI_GetShutdown,*WARTI_pGetShutdown;

typedef struct _WARTI_GetTestMode
{
	char *pRet;
	int test;
}WARTI_GetTestMode,*WARTI_pGetTestMode;

typedef struct _WARTI_GetPlayerCmdChannel
{
	char *pRet;
	int Channel;
}WARTI_GetPlayerCmdChannel,*WARTI_pGetPlayerCmdChannel;

typedef struct _WARTI_GetMvRemoteUpdate
{
	char *pRet;
	int Found;
}WARTI_GetMvRemoteUpdate,*WARTI_pGetMvRemoteUpdate;

typedef struct _WARTI_GetRemoteUpdateProgress
{
	char *pRet;
	int Download;
	int Upgrade;
	int State;
}WARTI_GetRemoteUpdateProgress,*WARTI_pGetRemoteUpdateProgress;

typedef struct _WARTI_SlaveSpeakerBox
{
	char *pName;
	int Mask;
	int Volume;
	int Mute;
	int Channel;
	struct in_addr IP;
	char *pVersion;	
}WARTI_SlaveSpeakerBox,*WARTI_pSlaveSpeakerBox;

typedef struct _WARTI_GetSlaveList
{
	int Slaves;
	WARTI_pSlaveSpeakerBox *pSlaveList;
}WARTI_GetSlaveList,*WARTI_pGetSlaveList;

typedef struct _WARTI_GetDeviceTime
{
	char *pRet;
	int Year;
	int Month;
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Zone;
}WARTI_GetDeviceTime,*WARTI_pGetDeviceTime;

typedef struct _WARTI_GetAlarmClock
{
	char *pRet;
	int N;
	int Enable;
	int Trigger;
	int Operation;
	int Year;
	int Month;
	int Date;
	int WeekDay;
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Volume;
	WARTI_pMusic pMusic;
}WARTI_GetAlarmClock,*WARTI_pGetAlarmClock;

typedef struct _WARTI_GetPlayerCmdSwitchMode
{
	char *pRet;
	int SwitchMode;
}WARTI_GetPlayerCmdSwitchMode,*WARTI_pGetPlayerCmdSwitchMode;

typedef struct _WARTI_GetLanguage
{
	char *pRet;
	int Language;
}WARTI_GetLanguage,*WARTI_pGetLanguage;

typedef struct _WARTI_InsertPlaylistInfo
{
	int Insert;
	WARTI_pMusic pMusic;	
}WARTI_InsertPlaylistInfo,*WARTI_pInsertPlaylistInfo;

typedef struct _WARTI_GetManufacturerInfo
{
	char *pRet;
	char *pManuFacturer;
	char *pManuFacturerURL;
	char *pModelDescription;
	char *pModelName;
	char *pModelNumber;
	char *pModelURL;
}WARTI_GetManufacturerInfo,*WARTI_pGetManufacturerInfo;

typedef struct _WARTI_GetLoginAmazonNeedInfo
{
	char *pRet;
	char *pProductId;
	char *pDsn;
	char *pCodeChallenge;
	char *pCodeChallengeMethod;
}WARTI_GetLoginAmazonNeedInfo,*WARTI_pGetLoginAmazonNeedInfo;

typedef struct _WARTI_PostLoginAmazonInfo
{
	char *pAuthorizationCode;
	char *pRedirectUri;
	char *pClientId;
}WARTI_PostLoginAmazonInfo,*WARTI_pPostLoginAmazonInfo;

typedef struct _WARTI_StorageDeviceOnlineState
{
	char *pRet;
	int TF;
	int USB;
}WARTI_StorageDeviceOnlineState,*WARTI_pStorageDeviceOnlineState;

typedef struct _WARTI_GetTheLatestVersionNumberOfFirmware
{
	char *pRet;
	char *pVersion;
}WARTI_GetTheLatestVersionNumberOfFirmware,*WARTI_pGetTheLatestVersionNumberOfFirmware;

typedef struct _WARTI_PostSynchronousInfoEx
{
	int Slave;
	char *pMaster_SSID;
	char *pMaster_Password;
}WARTI_PostSynchronousInfoEx, *WARTI_pPostSynchronousInfoEx;

typedef struct _WARTI_HostEx
{
	char *pIp;
	char *pMac;
	char *pName;
	char *pVolume;
	char *pOs;
	char *pUuid;
}WARTI_HostEx, *WARTI_pHostEx;

typedef struct _WARTI_ClientEx
{
	bool ActiveStatus;
	bool Connected;
	WARTI_pHostEx pHost;
}WARTI_ClientEx, *WARTI_pClientEx;

typedef struct _WARTI_GetSynchronousInfoEx
{
	char *pRet;
	int ConfigVersion;
	int Num;
	WARTI_pClientEx *pClientList;
}WARTI_GetSynchronousInfoEx, *WARTI_pGetSynchronousInfoEx;

typedef struct _WARTI_GetRecordFileURL
{
	char *pRet;
	char *pURL;
}WARTI_GetRecordFileURL,*WARTI_pGetRecordFileURL;

typedef struct _WARTI_BluetoothDevice
{
	int Connect;
	char *pAddress;
	char *pName;
}WARTI_BluetoothDevice,*WARTI_pBluetoothDevice;

typedef struct _WARTI_GetBluetoothList
{
	char *pRet;
	int Num;
	int Status;
	WARTI_pBluetoothDevice *pBdList;
}WARTI_GetBluetoothList,*WARTI_pGetBluetoothList;

typedef struct _WARTI_GetSpeechRecognition
{
	char *pRet;
	int Platform;
}WARTI_GetSpeechRecognition,*WARTI_pGetSpeechRecognition;

typedef struct _WARTI_LedMatrixData
{
	char *pData;
}WARTI_LedMatrixData,*WARTI_pLedMatrixData;

typedef struct _WARTI_MultiLedMatrix
{
	int Num;
	int Interval;
	WARTI_pLedMatrixData *pScreenlist;
}WARTI_MultiLedMatrix,*WARTI_pMultiLedMatrix;

typedef struct _WARTI_PostLedMatrixScreen
{
	int Animation;
	int Length;
	int Width;
	union 
	{
		WARTI_pLedMatrixData pLedMatrixData;
		WARTI_pMultiLedMatrix pMultiLedMatrix;
	}Content;
}WARTI_PostLedMatrixScreen,*WARTI_pPostLedMatrixScreen;

typedef struct _WARTI_GetChannelCompare
{
	char *pRet;
	double Left;
	double Right;
}WARTI_GetChannelCompare,*WARTI_pGetChannelCompare;

typedef struct _WARTI_RemoteBluetoothUpdate
{
	char *pURL;
}WARTI_RemoteBluetoothUpdate,*WARTI_pRemoteBluetoothUpdate;

typedef struct _WARTI_AlarmClock
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	int Timer;
	WARTI_pMusic pContent;
}WARTI_AlarmClock,*WARTI_pAlarmClock;

typedef struct _WARTI_PlanPlayAlbum
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMusic pContent;
}WARTI_PlanPlayAlbum,*WARTI_pPlanPlayAlbum;

typedef struct _WARTI_PlanPlayMusicList
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMusicList pContent;
}WARTI_PlanPlayMusicList,*WARTI_pPlanPlayMusicList;

typedef struct _WARTI_MixedElement
{
	int Type;
	WARTI_pMusic pContent;
}WARTI_MixedElement,*WARTI_pMixedElement;

typedef struct _WARTI_MixedContent
{
	int Num;
	int NowPlay;
	WARTI_pMixedElement *pMixedList;
}WARTI_MixedContent,*WARTI_pMixedContent;

typedef struct _WARTI_PlanMixed
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMixedContent pContent;
}WARTI_PlanMixed,*WARTI_pPlanMixed;

typedef struct _WARTI_GetUpgradeState
{
	char *pRet;
	int State;
	int Progress;
}WARTI_GetUpgradeState,*WARTI_pGetUpgradeState;

typedef struct _WARTI_ShortcutKeyList
{
	int CollectionNum;
	WARTI_pMixedContent pContent;
}WARTI_ShortcutKeyList,*WARTI_pShortcutKeyList;

enum WARTI_Str_type 
{
	WARTI_STR_TYPE_GETSTATUS			= 1,
	WARTI_STR_TYPE_WLANGETAPLIST,
	WARTI_STR_TYPE_GETCURRENTWIRELESSCONNECT,
	WARTI_STR_TYPE_GETDEVICEINFO,
	WARTI_STR_TYPE_GETONETOUCHCONFIGURE,
	WARTI_STR_TYPE_GETONETOUCHCONFIGURESETDEVICENAME,
	WARTI_STR_TYPE_GETLOGINAMAZONSTATUS,
	WARTI_STR_TYPE_GETLOGINAMAZONNEEDINFO,
	WARTI_STR_TYPE_GETALEXALANGUAGE,
	WARTI_STR_TYPE_GETWAKEUPTIMES,
	WARTI_STR_TYPE_GETMICARRAYINFO,	
	WARTI_STR_TYPE_MICARRAYNORMAL,
	WARTI_STR_TYPE_MICARRAYMIC,
	WARTI_STR_TYPE_MICARRAYREF,
	WARTI_STR_TYPE_LOGCAPTUREENABLE,
	WARTI_STR_TYPE_POSTLOGINAMAZONINFO,
	WARTI_STR_TYPE_GETPLAYERSTATUS,
	WARTI_STR_TYPE_GETCURRENTPLAYLIST,
	WARTI_STR_TYPE_GETPLAYERCMDVOLUME,
	WARTI_STR_TYPE_GETPLAYERCMDMUTE,
	WARTI_STR_TYPE_GETEQUALIZER,
	WARTI_STR_TYPE_GETPLAYERCMDCHANNEL,
	WARTI_STR_TYPE_GETSHUTDOWN,
	WARTI_STR_TYPE_GETMANUFACTURERINFO,
	WARTI_STR_TYPE_GETMVREMOTEUPDATE,
	WARTI_STR_TYPE_GETDOWNLOADPROGRESS,
	WARTI_STR_TYPE_GETSLAVELIST,
	WARTI_STR_TYPE_GETMVREMOTEUPDATEBURNSTATUS,
	WARTI_STR_TYPE_GETDEVICETIME,
	WARTI_STR_TYPE_GETALARMCLOCK,
	WARTI_STR_TYPE_GETPLAYERCMDSWITCHMODE,
	WARTI_STR_TYPE_GETLANGUAGE,
	WARTI_STR_TYPE_INSERTPLAYLISTINFO,
	WARTI_STR_TYPE_STORAGEDEVICEONLINESTATE,
	WARTI_STR_TYPE_GETTHELATESTVERSIONNUMBEROFFIRMWARE,
	WARTI_STR_TYPE_POSTSYNCHRONOUSINFOEX,
	WARTI_STR_TYPE_GETSYNCHRONOUSINFOEX,
	WARTI_STR_TYPE_GETRECORDFILEURL,
	WARTI_STR_TYPE_GETBLUETOOTHLIST,
	WARTI_STR_TYPE_GETSPEECHRECOGNITION,
	WARTI_STR_TYPE_POSTLEDMATRIXSCREEN,
	WARTI_STR_TYPE_GETCHANNELCOMPARE,
	WARTI_STR_TYPE_REMOTEBLUETOOTHUPDATE,
	WARTI_STR_TYPE_GETBLUETOOTHDOWNLOADPROGRESS,
	WARTI_STR_TYPE_CREATEALARMCLOCK,
	WARTI_STR_TYPE_CHANGEALARMCLOCKPLAY,
	WARTI_STR_TYPE_COUNTDOWNALARMCLOCK,
	WARTI_STR_TYPE_PLAYALBUMASPLAN,
	WARTI_STR_TYPE_PLAYMUSICLISTASPLAN,
	WARTI_STR_TYPE_PLAYMIXEDLISTASPLAN,
	WARTI_STR_TYPE_UPGRADEWIFIANDBLUETOOTHSTATE,
	WARTI_STR_TYPE_SHORTCUTKEYLIST,
	WARTI_STR_TYPE_GETTESTMODE,
};

typedef struct _WARTI_Str
{
	enum WARTI_Str_type type;
	union 
	{
		WARTI_pGetStatus pGetStatus;
		WARTI_pWLANGetAPlist pWLANGetAPlist;
		WARTI_pGetCurrentWirelessConnect pGetCurrentWirelessConnect;
		WARTI_pGetDeviceInfo pGetDeviceInfo;
		WARTI_pGetOneTouchConfigure pGetOneTouchConfigure;
		WARTI_pGetOneTouchConfigureSetDeviceName pGetOneTouchConfigureSetDeviceName;
		WARTI_pGetLoginAmazonStatus pGetLoginAmazonStatus;
		WARTI_pGetAlexaLanguage pGetAlexaLanguage;
		WARTI_pGetWakeupTimes pGetWakeupTimes;
		WARTI_pGetTestMode pGetTestMode;
		WARTI_pGetMicarrayInfo pGetMicarrayInfo;
		WARTI_pMicarrayNormal pMicarrayNormal;
		WARTI_pMicarrayMic pMicarrayMic;
		WARTI_pMicarrayRef pMicarrayRef;
		WARTI_pLogCaptureEnable pLogCaptureEnable;
		WARTI_pGetLoginAmazonNeedInfo pGetLoginAmazonNeedInfo;
		WARTI_pPostLoginAmazonInfo pPostLoginAmazonInfo;
		WARTI_pGetPlayerStatus pGetPlayerStatus;
		WARTI_pMusicList pGetCurrentPlaylist;
		WARTI_pGetPlayerCmdVolume pGetPlayerCmdVolume;
		WARTI_pGetPlayerCmdMute pGetPlayerCmdMute;
		WARTI_pGetEqualizer pGetEqualizer;
		WARTI_pGetPlayerCmdChannel pGetPlayerCmdChannel;
		WARTI_pGetShutdown pGetShutdown;
		WARTI_pGetManufacturerInfo pGetManufacturerInfo;
		WARTI_pGetMvRemoteUpdate pGetMvRemoteUpdate;
		WARTI_pGetRemoteUpdateProgress pGetRemoteUpdateProgress;
		WARTI_pGetSlaveList pGetSlaveList;
		WARTI_pGetDeviceTime pGetDeviceTime;
		WARTI_pGetAlarmClock pGetAlarmClock;
		WARTI_pGetPlayerCmdSwitchMode pGetPlayerCmdSwitchMode;
		WARTI_pGetLanguage pGetLanguage;
		WARTI_pInsertPlaylistInfo pInsertPlaylistInfo;
		WARTI_pStorageDeviceOnlineState pStorageDeviceOnlineState;
		WARTI_pGetTheLatestVersionNumberOfFirmware pGetTheLatestVersionNumberOfFirmware;
		WARTI_pPostSynchronousInfoEx pPostSynchronousInfoEx;
		WARTI_pGetSynchronousInfoEx pGetSynchronousInfoEx;
		WARTI_pGetRecordFileURL pGetRecordFileURL;
		WARTI_pGetBluetoothList pGetBluetoothList;
		WARTI_pGetSpeechRecognition pGetSpeechRecognition;
		WARTI_pPostLedMatrixScreen pPostLedMatrixScreen;
		WARTI_pGetChannelCompare pGetChannelCompare;
		WARTI_pRemoteBluetoothUpdate pRemoteBluetoothUpdate;
		WARTI_pAlarmClock pAlarmClock;
		WARTI_pPlanPlayAlbum pPlanPlayAlbum;
		WARTI_pPlanPlayMusicList pPlanPlayMusicList;
		WARTI_pPlanMixed pPlanMixed;
		WARTI_pGetUpgradeState pGetUpgradeState;
		WARTI_pShortcutKeyList pShortcutKeyList;
	}Str;
}WARTI_Str, *WARTI_pStr;

extern int WIFIAudio_RealTimeInterface_pStrFree(WARTI_pStr *pStr);
extern WARTI_pStr WIFIAudio_RealTimeInterface_newStr(enum WARTI_Str_type type, void *pBuf);
extern int WIFIAudio_RealTimeInterface_pBufFree(char **ppBuf);
extern char * WIFIAudio_RealTimeInterface_newBuf(WARTI_pStr pStr);

#ifdef __cplusplus
}
#endif

#endif
