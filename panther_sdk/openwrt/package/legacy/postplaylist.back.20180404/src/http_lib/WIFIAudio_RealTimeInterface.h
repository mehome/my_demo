#ifndef __WIFIAUDIO_REALTIMEINTERFACE_H__
#define __WIFIAUDIO_REALTIMEINTERFACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>//IP地址转换所需要的头文件

#include <net/ethernet.h>//ether_addr（mac帧结构）
#include <netinet/ether.h>//mac地址转换的函数定义

#define WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE (~0)

typedef struct _WARTI_GetStatus
{
	char *pLanguage;		//语言
	char *pSSID;			//设备SSID
	char *pFirmware;		//软件版本
	char *pBuildDate;		//编译日期
	char *pRelease;			//版本打包日期
	char *pGroup;			//群组名称
	int Expired;			//软件过期标志，0未过期，1已过期
	int Internet;			//Internet访问，0不能访问，1可以访问
	char UUID[16];			//设备UUID,uuid的动态库里面就是这么定义的
	int Netstat;			//WIFI连接状态，0未连接，1过渡状态，2已连接
	char *pESSID;			//WIFI连接到的路由器
	struct in_addr Apcli0;	//WIFI的IP地址
	struct in_addr Eth2;	//Ethernet的IP地址
	char *pHardware;		//硬件版本
}WARTI_GetStatus,*WARTI_pGetStatus;	//GetStatus结构体

typedef struct _WARTI_AccessPoint
{
	char *pSSID;			//WIFI的可见名称
	struct ether_addr BSSID;//MAC地址
	int RSSI;				//信号强度
	int Channel;			//WIFI的信道
	char *pAuth;			//是否加密
	char *pEncry;			//加密类型
	int Extch;
}WARTI_AccessPoint,*WARTI_pAccessPoint;		//AccessPoint结构体

typedef struct _WARTI_WLANGetAPlist
{
	char *pRet;				//成功标志
	int Res;				//列表数量
	WARTI_pAccessPoint *pAplist;	//ssid列表，这边存指向WARTI_pAccessPoint的指针，即AccessPoint指针的指针
}WARTI_WLANGetAPlist,*WARTI_pWLANGetAPlist;	//WLANGetAPlist结构体

typedef struct _WARTI_GetCurrentWirelessConnect
{
	char *pRet;				//成功标志
	char *pSSID;			//当前连接的SSID
	int Connect;			//联通标志
}WARTI_GetCurrentWirelessConnect,*WARTI_pGetCurrentWirelessConnect;	//GetCurrentWirelessConnect结构体

typedef struct _WARTI_GetDeviceInfo
{
	char *pRet;				//成功标志
	int RSSI;				//信号接收强度指示
	char *pLanguage;		//当前语言
	char *pSSID;			//设备SSID
	char *pFirmware;		//软件版本
	char *pRelease;			//版本打包日期
	char *pMAC;				//MAC地址
	char *pUUID;			//设备UUID
	char *pESSID;			//WIFI连接到的路由器
	struct in_addr Apcli0;	//WIFI的IP地址
	char *pProject;			//工程名
	char *pDeviceName;		//设备的UPNP friendly name
	int BatteryPercent;		//电池当前电量，0表示没有电池
	char *pLanguageSupport;	//语言支持
	char *pFunctionSupport;	//功能支持
	int KeyNum;				//预置按键数量
	char *pBtVersion;		//蓝牙固件版本
	char *pConexantVersion;	//Conexant固件版本
	char *pPowerSupply;		//外部电源标志
}WARTI_GetDeviceInfo,*WARTI_pGetDeviceInfo;//GetDeviceInfo结构体

typedef struct _WARTI_GetOneTouchConfigure
{
	char *pRet;			//成功标志
	int State;			//一键配置状态
}WARTI_GetOneTouchConfigure,*WARTI_pGetOneTouchConfigure;	//GetOneTouchConfigure结构体

typedef struct _WARTI_GetOneTouchConfigureSetDeviceName
{
	char *pRet;			//成功标志
	int State;			//一键配置设备重命名状态
}WARTI_GetOneTouchConfigureSetDeviceName,*WARTI_pGetOneTouchConfigureSetDeviceName;	//GetOneTouchConfigureSetDeviceName结构体

typedef struct _WARTI_GetLoginAmazonStatus
{
	char *pRet;			//成功标志
	int State;			//亚马逊账号登录状态
}WARTI_GetLoginAmazonStatus,*WARTI_pGetLoginAmazonStatus;	//GetLoginAmazonStatus结构体

typedef struct _WARTI_GetPlayerStatus
{
	char *pRet;				//成功标志
	int MainMode;			//0：普通音响或者主音箱 1：子音响
	int NodeType;			//声道：0表示立体声，1表示左声道，2表示右声道
	int Mode;				//播放模式：0表示无模式，1表示第三方软件推送，2表示Airplay，3表示U盘本地播放，4表示wiimu的列表推送模式
	int SW;					//播放列表中的歌曲切换模式：0顺序播放，1循环单曲，2随机播放，3列表循环
	char *pStatus;			//当前状态
	int CurPos;				//当前的位置
	int TotLen;				//总长度
	char *pTitle;			//标题
	char *pArtist;			//艺术家
	char *pAlbum;			//专辑名
	int Year;				//年份
	int Track;				//轨道号
	char *pGenre;			//风格
	int LocalListFlag;		//是否插入U盘
	char *pLocalListFile;	//目前无效
	int PLiCount;			//播放列表的总条数
	int PLicurr;			//当前音轨在播放列表中的index
	int Vol;				//当前音量
	int Mute;				//当前是否静音
	char *pIURI;			//目前无效
	char *pURI;				//当前dlna的uri
	char *pCover;			//封面
	int SourceMode;			//输入源
	char *pUUID;			//设备uuid
}WARTI_GetPlayerStatus,*WARTI_pGetPlayerStatus;//GetPlayerStatus结构体

typedef struct _WARTI_GetPlayerCmdVolume
{
	char *pRet;				//成功标志
	int Volume;				//音量
}WARTI_GetPlayerCmdVolume,*WARTI_pGetPlayerCmdVolume;	//GetPlayerCmdVolume结构体

typedef struct _WARTI_GetPlayerCmdMute
{
	char *pRet;				//成功标志
	int Mute;				//静音标志
}WARTI_GetPlayerCmdMute,*WARTI_pGetPlayerCmdMute;	//GetPlayerCmdMute结构体

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
}WARTI_Music,*WARTI_pMusic;//音乐列表元素的具体信息

typedef struct _WARTI_MusicList
{
	char *pRet;				//成功标志
	int Num;				//歌曲数量
	int Index;				//播放索引
	int NowPlay;
	WARTI_pMusic *pMusicList;
}WARTI_MusicList,*WARTI_pMusicList;//音乐列表的结构体

typedef struct _WARTI_GetEqualizer
{
	char *pRet;				//成功标志
	int Mode;				//均衡器模式
}WARTI_GetEqualizer,*WARTI_pGetEqualizer;

typedef struct _WARTI_GetShutdown
{
	char *pRet;				//成功标志
	int Second;				//秒数
}WARTI_GetShutdown,*WARTI_pGetShutdown;

typedef struct _WARTI_GetPlayerCmdChannel
{
	char *pRet;				//成功标志
	int Channel;			//声道
}WARTI_GetPlayerCmdChannel,*WARTI_pGetPlayerCmdChannel;	//GetPlayerCmdChannel结构体

typedef struct _WARTI_GetMvRemoteUpdate
{
	char *pRet;				//成功标志
	int Found;				//找到升级包的标志
}WARTI_GetMvRemoteUpdate,*WARTI_pGetMvRemoteUpdate;	//GetMvRemoteUpdate结构体

typedef struct _WARTI_GetRemoteUpdateProgress
{
	char *pRet;				//成功标志
	int Download;			//升级包下载进度
	int Upgrade;			//升级包烧写进度
	int State;				//当前状态
}WARTI_GetRemoteUpdateProgress,*WARTI_pGetRemoteUpdateProgress;

typedef struct _WARTI_SlaveSpeakerBox
{
	char *pName;			//设备名称
	int Mask;				//子音箱是否被屏蔽，1屏蔽，0未屏蔽
	int Volume;				//子音箱音量
	int Mute;				//子音箱是否静音，1静音，0非静音
	int Channel;			//子音箱的WIFI信道
	struct in_addr IP;		//子音箱的IP
	char *pVersion;			//子音箱固件版本
}WARTI_SlaveSpeakerBox,*WARTI_pSlaveSpeakerBox;

typedef struct _WARTI_GetSlaveList
{
	int Slaves;				//子音箱个数
	WARTI_pSlaveSpeakerBox *pSlaveList;		//子音箱列表，这边存指向WARTI_pSlaveSpeakerBox的指针，即WARTI_SlaveSpeakerBox指针的指针
}WARTI_GetSlaveList,*WARTI_pGetSlaveList;		//GetSlaveList结构体

typedef struct _WARTI_GetDeviceTime
{
	char *pRet;				//成功标志
	int Year;				//年
	int Month;				//月
	int	Day;				//日
	int Hour;				//时
	int Minute;				//分
	int Second;				//秒
	int Zone;				//时区
}WARTI_GetDeviceTime,*WARTI_pGetDeviceTime;	//GetDeviceTime结构体

typedef struct _WARTI_GetAlarmClock
{
	char *pRet;			//成功标志
	int N;				//闹钟序号
	int Enable;			//使能标志
	int Trigger;		//触发模式
	int Operation;		//执行动作
	int Year;			//年
	int Month;			//月
	int	Date;			//日
	int WeekDay;		//星期几
	int Day;			//每月
	int Hour;			//小时
	int Minute;			//分钟
	int Second;			//秒
	int Volume;			//音量
	WARTI_pMusic pMusic;
}WARTI_GetAlarmClock,*WARTI_pGetAlarmClock;	//GetAlarmClock结构体

typedef struct _WARTI_GetPlayerCmdSwitchMode
{
	char *pRet;			//成功标志
	int SwitchMode;		//选择模式
}WARTI_GetPlayerCmdSwitchMode,*WARTI_pGetPlayerCmdSwitchMode;	//GetPlayerCmdSwitchMode结构体

typedef struct _WARTI_GetLanguage
{
	char *pRet;			//成功标志
	int Language;		//语音语种
}WARTI_GetLanguage,*WARTI_pGetLanguage;	//GetLanguage结构体

typedef struct _WARTI_InsertPlaylistInfo
{
	int Insert;				//插入索引
	WARTI_pMusic pMusic;	
}WARTI_InsertPlaylistInfo,*WARTI_pInsertPlaylistInfo;	//InsertPlaylistInfo结构体

typedef struct _WARTI_GetManufacturerInfo
{
	char *pRet;					//成功标志
	char *pManuFacturer;		//制造商
	char *pManuFacturerURL;		//制造商URL
	char *pModelDescription;	//模型描述
	char *pModelName;			//模型名称
	char *pModelNumber;			//模型版本
	char *pModelURL;			//模型URL
}WARTI_GetManufacturerInfo,*WARTI_pGetManufacturerInfo;	//ManufacturerInfo结构体

typedef struct _WARTI_GetLoginAmazonNeedInfo
{
	char *pRet;					//成功标志
	char *pProductId;			//产品ID
	char *pDsn;					//数据源名
	char *pCodeChallenge;		//
	char *pCodeChallengeMethod;	//
}WARTI_GetLoginAmazonNeedInfo,*WARTI_pGetLoginAmazonNeedInfo;	//ManufacturerInfo结构体

typedef struct _WARTI_PostLoginAmazonInfo
{
	char *pAuthorizationCode;	//授权码
	char *pRedirectUri;			//重定向URI
	char *pClientId;			//客户端ID
}WARTI_PostLoginAmazonInfo,*WARTI_pPostLoginAmazonInfo;	//ManufacturerInfo结构体

typedef struct _WARTI_StorageDeviceOnlineState
{
	char *pRet;			//成功标志
	int TF;				//tf卡
	int USB;			//usb
}WARTI_StorageDeviceOnlineState,*WARTI_pStorageDeviceOnlineState;

typedef struct _WARTI_GetTheLatestVersionNumberOfFirmware
{
	char *pRet;			//成功标志
	char *pVersion;		//版本号
}WARTI_GetTheLatestVersionNumberOfFirmware,*WARTI_pGetTheLatestVersionNumberOfFirmware;

typedef struct _WARTI_PostSynchronousInfoEx
{
	int Slave;				//主从标志
	char *pMaster_SSID;		//主音箱的AP热点SSID
	char *pMaster_Password;	//主音箱的AP热点密码
}WARTI_PostSynchronousInfoEx, *WARTI_pPostSynchronousInfoEx;//同步音箱信息结构体

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
	char *pRet;					//成功标志 
	int ConfigVersion;
	int Num;
	WARTI_pClientEx *pClientList;
}WARTI_GetSynchronousInfoEx, *WARTI_pGetSynchronousInfoEx;//同步配置信息

typedef struct _WARTI_GetRecordFileURL
{
	char *pRet;			//成功标志
	char *pURL;			//文件URL
}WARTI_GetRecordFileURL,*WARTI_pGetRecordFileURL;

typedef struct _WARTI_BluetoothDevice
{
	int Connect;
	char *pAddress;
	char *pName;
}WARTI_BluetoothDevice,*WARTI_pBluetoothDevice;

typedef struct _WARTI_GetBluetoothList
{
	char *pRet;				//成功标志
	int Num;				//列表数量
	int Status;
	WARTI_pBluetoothDevice *pBdList;
}WARTI_GetBluetoothList,*WARTI_pGetBluetoothList;

typedef struct _WARTI_GetSpeechRecognition
{
	char *pRet;				//成功标志
	int Platform;			//语音识别平台
}WARTI_GetSpeechRecognition,*WARTI_pGetSpeechRecognition;

typedef struct _WARTI_LedMatrixData
{
	char *pData;		//显示屏数据
}WARTI_LedMatrixData,*WARTI_pLedMatrixData;

typedef struct _WARTI_MultiLedMatrix
{
	int Num;			//此次多屏个数
	int Interval;		//屏幕间切换时间
	WARTI_pLedMatrixData *pScreenlist;		//显示屏数据数组
}WARTI_MultiLedMatrix,*WARTI_pMultiLedMatrix;

typedef struct _WARTI_PostLedMatrixScreen
{
	int Animation;		//是否连续
	int Length;			//横向灯珠数
	int Width;			//纵向灯珠数
	union 
	{
		WARTI_pLedMatrixData pLedMatrixData;
		WARTI_pMultiLedMatrix pMultiLedMatrix;
	}Content;
}WARTI_PostLedMatrixScreen,*WARTI_pPostLedMatrixScreen;

typedef struct _WARTI_GetAlexaLanguage
{
	char *pRet;			//成功标志
	char *pLanguage;	//Alexa语种
}WARTI_GetAlexaLanguage,*WARTI_pGetAlexaLanguage;

typedef struct _WARTI_GetChannelCompare
{
	char *pRet;			//成功标志
	double Left;		//左声道的值
	double Right;		//右声道的值
}WARTI_GetChannelCompare,*WARTI_pGetChannelCompare;

typedef struct _WARTI_RemoteBluetoothUpdate
{
	char *pURL;			//成功标志
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
}WARTI_AlarmClock,*WARTI_pAlarmClock;//闹钟的都用这个

typedef struct _WARTI_PlanPlayAlbum
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMusic pContent;
}WARTI_PlanPlayAlbum,*WARTI_pPlanPlayAlbum;//定时播放专辑用这个

typedef struct _WARTI_PlanPlayMusicList
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMusicList pContent;
}WARTI_PlanPlayMusicList,*WARTI_pPlanPlayMusicList;//定时播放列表用这个

typedef struct _WARTI_MixedElement
{
	int Type;				//元素类型
	WARTI_pMusic pContent;
}WARTI_MixedElement,*WARTI_pMixedElement;//音乐列表的结构体

typedef struct _WARTI_MixedContent
{
	int Num;				//元素个数
	int NowPlay;
	WARTI_pMixedElement *pMixedList;
}WARTI_MixedContent,*WARTI_pMixedContent;//音乐列表的结构体

typedef struct _WARTI_PlanMixed
{
	int Num;
	int Enable;
	int Week;
	int Hour;
	int Minute;
	WARTI_pMixedContent pContent;
}WARTI_PlanMixed,*WARTI_pPlanMixed;//定时混合播放用这个

typedef struct _WARTI_GetUpgradeState
{
	char *pRet;				//成功标志
	int State;				//当前状态
	int Progress;			//进度
}WARTI_GetUpgradeState,*WARTI_pGetUpgradeState;

typedef struct _WARTI_ShortcutKeyList
{
	int CollectionNum;
	WARTI_pMixedContent pContent;
}WARTI_ShortcutKeyList,*WARTI_pShortcutKeyList;	//快捷键播放列表现在采用这个

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
	WARTI_STR_TYPE_GETALEXALANGUAGE,
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
		WARTI_pGetAlexaLanguage pGetAlexaLanguage;
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


/******************************************
 ** 函数名: WIFIAudio_RealTimeInterface_pStrFree
 ** 功  能: 将*pStr指向WARTI_Str结构体所申请的空间释放
 ** 参  数: *pBuf指向WARTI_Str结构体的指针
 ** 返回值: 成功返回0，失败返回-1
 ******************************************/ 
extern int WIFIAudio_RealTimeInterface_pStrFree(WARTI_pStr *pStr);


/******************************************
 ** 函数名: WIFIAudio_RealTimeInterface_newStr
 ** 功  能: 将pBuf指向JSON格式的字符串转换成WARTI_Str结构体返回
 ** 参  数: type命令类型，pBuf指向JSON格式的字符串的指针
 ** 返回值: 成功返回WARTI_Str结构体，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_RealTimeInterface_newStr(enum WARTI_Str_type type, void *pBuf);


/******************************************
 ** 函数名: WIFIAudio_RealTimeInterface_pBufFree
 ** 功  能: 将*pBuf指向的unsigned char字符串所申请的空间释放
 ** 参  数: *pBuf指向unsigned char字符串的指针
 ** 返回值: 成功返回0，失败返回-1
 ******************************************/ 
extern int WIFIAudio_RealTimeInterface_pBufFree(char **ppBuf);


/******************************************
 ** 函数名: WIFIAudio_RealTimeInterface_newBuf
 ** 功  能: 将pStr指向的WARTI_Str结构体转换成JSON格式字符串返回
 ** 参  数: pStr指向的WARTI_Str结构体的指针
 ** 返回值: 成功返回JSON格式字符串，失败返回NULL
 ******************************************/ 
extern char * WIFIAudio_RealTimeInterface_newBuf(WARTI_pStr pStr);



#ifdef __cplusplus
}
#endif

#endif
