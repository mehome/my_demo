#ifndef __WIFIAUDIO_GETCOMMAND_H__
#define __WIFIAUDIO_GETCOMMAND_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>//IP地址转换所需要的头文件

enum WAGC_Command_Auth_Attr 
{
	WAGC_COMMAND_AUTH_TYPE_OPEN 		= 0,
	WAGC_COMMAND_AUTH_TYPE_WPA_PSK,
	WAGC_COMMAND_AUTH_TYPE_WPA2_PSK,
	WAGC_COMMAND_AUTH_TYPE_WPA,
	WAGC_COMMAND_AUTH_TYPE_WPA2,
	WAGC_COMMAND_AUTH_TYPE_WEP,
};

enum WAGC_Command_Encty_Attr 
{
	WAGC_COMMAND_ENCTY_TYPE_NONE 		= 0,
	WAGC_COMMAND_ENCTY_TYPE_TKIP,
	WAGC_COMMAND_ENCTY_TYPE_AES,
};

typedef struct _WAGC_ConectAp_t 
{
	char *pSSID;
	int Channel;
	enum WAGC_Command_Auth_Attr Auth;
	enum WAGC_Command_Encty_Attr Encty;
	char *pPassword;
	int Extch;
}WAGC_ConectAp_t, *WAGC_pConectAp_t, WAGC_ConnectAp_t, *WAGC_pConnectAp_t,\
 WAGC_SubConnectAp_t, *WAGC_pSubConnectAp_t, WAGC_ConnectApEx_t, *WAGC_pConnectApEx_t;
 
 
typedef struct _WAGC_OneTouchState_t
{
	int Value;
}WAGC_OneTouchState_t, *WAGC_pOneTouchState_t;

typedef struct _WAGC_OneTouchNameState_t
{
	int Value;
}WAGC_OneTouchNameState_t, *WAGC_pOneTouchNameState_t;

 enum WAGC_Command_SetPlayerAttr 
{
	WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAYLIST 		= 1,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_PAUSE,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAY,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_PREV,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_NEXT,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_SEEK,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_STOP,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_VOL,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_MUTE,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_LOOPMODE,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_EQUALIZER,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_VOL,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_MUTE,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_CHANNEL,
	WAGC_COMMAND_SETPLAYERATTR_TYPE_SWITCHMODE,
};
 
typedef struct _WAGC_SetPlayerCmd_t
{
	enum WAGC_Command_SetPlayerAttr Attr; 
	char *pURI;
	int Value;
} WAGC_SetPlayerCmd_t, *WAGC_pSetPlayerCmd_t;

enum WAGC_Command_GetPlayerAttr 
{
	WAGC_COMMAND_GETPLAYERATTR_TYPE_VOLUME 			= 1,
	WAGC_COMMAND_GETPLAYERATTR_TYPE_MUTE,
	WAGC_COMMAND_GETPLAYERATTR_TYPE_SLAVE_CHANNEL,
	WAGC_COMMAND_GETPLAYERATTR_TYPE_SWITCHMODE,
};

typedef struct _WAGC_GetPlayerCmd_t
{
	enum WAGC_Command_GetPlayerAttr Attr; 
} WAGC_GetPlayerCmd_t, *WAGC_pGetPlayerCmd_t;

enum WAGC_Command_MultiroomAttr
{
	WAGC_COMMAND_MULTIROOMATTR_TYPE_GETSLAVELIST	=1,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEKICKOUT,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMASK,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEUNMASK,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEVOLUME,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMUTE,
	WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVECHANNEL,
};

typedef struct _WAGC_Multiroom_t
{
	enum WAGC_Command_MultiroomAttr Attr;
	struct in_addr IP;
	int Value;
} WAGC_Multiroom_t, *WAGC_pMultiroom_t;

typedef struct _WAGC_SSID_t
{
	char *pSSID;
} WAGC_SSID_t, *WAGC_pSSID_t;

typedef struct _WAGC_Network_t
{
	int Auth;
	char *pPassword;
} WAGC_Network_t, *WAGC_pNetwork_t;

typedef struct _WAGC_Name_t
{
	char *pName;
} WAGC_Name_t, *WAGC_pName_t;

typedef struct _WAGC_Shutdown_t
{
	int Second;
}WAGC_Shutdown_t, *WAGC_pShutdown_t;

typedef struct _WAGC_UpdateURL_t
{
	char *pURL;
}WAGC_UpdateURL_t, *WAGC_pUpdateURL_t;

typedef struct _WAGC_DateTime_t
{
	int Year;
	int Month;
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Zone;
}WAGC_DateTime_t, *WAGC_pDateTime_t;

typedef struct _WAGC_ClockNum_t
{
	int Num;
}WAGC_ClockNum_t, *WAGC_pClockNum_t;

typedef struct _WAGC_SelectLanguage_t
{
	int Language;
}WAGC_SelectLanguage_t, *WAGC_pSelectLanguage_t;

typedef struct _WAGC_PlaylistIndex_t
{
	int Index;
}WAGC_PlaylistIndex_t, *WAGC_pPlaylistIndex_t;

enum WAGC_Command_xzxActionAttr
{
	WAGC_COMMAND_XZXACTIONATTR_TYPE_SETRT	=1,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYREC,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYURLREC,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_STARTRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_FIXEDRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_GETRECORDFILEURL,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_VOICEACTIVITYDETECTION,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_CHANNELCOMPARE,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_NORMALRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_GETNORMALURL,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_MICRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_GETMICURL,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_REFRECORD,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_GETREFURL,
	WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPMODERECORD,
};

typedef struct _WAGC_xzxAction_t
{
	enum WAGC_Command_xzxActionAttr Attr;
	int Value;
	char *pUrl;
	char *pCardName;
}WAGC_xzxAction_t, *WAGC_pxzxAction_t;

typedef struct _WAGC_PlayTfCard_t
{
	int Index;
}WAGC_PlayTfCard_t, *WAGC_pPlayTfCard_t;

typedef struct _WAGC_PlayUsbDisk_t
{
	int Index;
}WAGC_PlayUsbDisk_t, *WAGC_pPlayUsbDisk_t;

typedef struct _WAGC_RemoveSynchronousEx_t
{
	char *pMac;
}WAGC_RemoveSynchronousEx_t, *WAGC_pRemoveSynchronousEx_t;

enum WAGC_Command_SetSynchronousExAttr
{
	WAGC_COMMAND_SETSYNCHRONOUSATTREX_TYPE_VOLUME	=1,
};

typedef struct _WAGC_SetSynchronousEx_t
{
	enum WAGC_Command_SetSynchronousExAttr Attr; 
	int Value;
	char *pMac;
} WAGC_SetSynchronousEx_t, *WAGC_pSetSynchronousEx_t;

enum WAGC_Command_BluetoothAttr
{
	WAGC_COMMAND_BLUETOOTHATTR_TYPE_SEARCH	=1,
	WAGC_COMMAND_BLUETOOTHATTR_TYPE_GETDEVICELIST,
	WAGC_COMMAND_BLUETOOTHATTR_TYPE_CONNECT,
	WAGC_COMMAND_BLUETOOTHATTR_TYPE_DISCONNECT,
};

typedef struct _WAGC_Bluetooth_t
{
	enum WAGC_Command_BluetoothAttr Attr;
	char *pAddress;
}WAGC_Bluetooth_t, *WAGC_pBluetooth_t;

typedef struct _WAGC_SetSpeechRecognition_t
{
	int Platform;
}WAGC_SetSpeechRecognition_t, *WAGC_pSetSpeechRecognition_t;

typedef struct _WAGC_SetSingleLight_t
{
	int Enable;
	int Brightness;
	int Red;
	int Green;
	int Blue;
}WAGC_SetSingleLight_t, *WAGC_pSetSingleLight_t;

typedef struct _WAGC_SetAlexaLanguage_t
{
	char *pLanguage;
}WAGC_SetAlexaLanguage_t, *WAGC_pSetAlexaLanguage_t;

typedef struct _WAGC_SetTestMode_t
{
	int Mode;
}WAGC_SetTestMode_t, *WAGC_pSetTestMode_t;

typedef struct _WAGC_SetEnable_t
{
	int Enable;
}WAGC_SetEnable_t, *WAGC_pSetEnable_t;

typedef struct _WAGC_AlarmAndPlan_t
{
	int Num;
	int Enable;
}WAGC_AlarmAndPlan_t, *WAGC_pAlarmAndPlan_t;

typedef struct _WAGC_ShortcutKey_t
{
	int Num;
}WAGC_ShortcutKey_t, *WAGC_pShortcutKey_t;

enum WAGC_Command
{
	WIFIAUDIO_GETCOMMAND_GETSTATUS						= 1,
	WIFIAUDIO_GETCOMMAND_GETSTATUSEX,
	WIFIAUDIO_GETCOMMAND_WLANGETAPLIST,
	WIFIAUDIO_GETCOMMAND_WLANCONECTAP,
	WIFIAUDIO_GETCOMMAND_WLANCONNECTAP,
	WIFIAUDIO_GETCOMMAND_WLANSUBCONNECTAP,
	WIFIAUDIO_GETCOMMAND_WLANGETCONNECTSTATE,
	WIFIAUDIO_GETCOMMAND_GETCURRENTWIRELESSCONNECT,
	WIFIAUDIO_GETCOMMAND_GETCURRENTWIRELESSCONNECTEX,
	WIFIAUDIO_GETCOMMAND_GETDEVICEINFO,
	WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURE,
	WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURE,
	WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURESETDEVICENAME,
	WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURESETDEVICENAME,
	WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONSTATUS,
	WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONNEEDINFO,
	WIFIAUDIO_GETCOMMAND_LOGOUTAMAZON,
	WIFIAUDIO_GETCOMMAND_WLANGETAPLISTEX,
	WIFIAUDIO_GETCOMMAND_WLANCONNECTAPEX,
	WIFIAUDIO_GETCOMMAND_GETPLAYERSTATUS,
	WIFIAUDIO_GETCOMMAND_SETPLAYERCMD,
	WIFIAUDIO_GETCOMMAND_GETEQUALIZER,
	WIFIAUDIO_GETCOMMAND_GETCURRENTPLAYLIST,
	WIFIAUDIO_GETCOMMAND_MULTIROOM,
	WIFIAUDIO_GETCOMMAND_WPSSERVERMODE,
	WIFIAUDIO_GETCOMMAND_WPSCANCEL,
	WIFIAUDIO_GETCOMMAND_GETPLAYERCMD,
	WIFIAUDIO_GETCOMMAND_SETSSID,
	WIFIAUDIO_GETCOMMAND_SETNETWORK,
	WIFIAUDIO_GETCOMMAND_RESTORETODEFAULT,
	WIFIAUDIO_GETCOMMAND_REBOOT,
	WIFIAUDIO_GETCOMMAND_SETDEVICENAME,
	WIFIAUDIO_GETCOMMAND_SETSHUTDOWN,
	WIFIAUDIO_GETCOMMAND_GETSHUTDOWN,
	WIFIAUDIO_GETCOMMAND_GETMANUFACTURERINFO,
	WIFIAUDIO_GETCOMMAND_SETPOWERWIFIDOWN,
	WIFIAUDIO_GETCOMMAND_GETTHELATESTVERSIONNUMBEROFFIRMWARE,
	WIFIAUDIO_GETCOMMAND_GETMVREMOTEUPDATE,
	WIFIAUDIO_GETCOMMAND_SETTIMESYNC,
	WIFIAUDIO_GETCOMMAND_GETDEVICETIME,
	WIFIAUDIO_GETCOMMAND_GETALARMCLOCK,
	WIFIAUDIO_GETCOMMAND_ALARMSTOP,
	WIFIAUDIO_GETCOMMAND_SELECTLANGUAGE,
	WIFIAUDIO_GETCOMMAND_GETLANGUAGE,
	WIFIAUDIO_GETCOMMAND_DELETEPLAYLISTMUSIC,
	WIFIAUDIO_GETCOMMAND_XZXACTION,
	WIFIAUDIO_GETCOMMAND_STORAGEDEVICEONLINESTATE,
	WIFIAUDIO_GETCOMMAND_GETTFCARDPLAYLISTINFO,
	WIFIAUDIO_GETCOMMAND_PLAYTFCARD,
	WIFIAUDIO_GETCOMMAND_GETUSBDISKPLAYLISTINFO,
	WIFIAUDIO_GETCOMMAND_PLAYUSBDISK,
	WIFIAUDIO_GETCOMMAND_GETSYNCHRONOUSINFOEX,
	WIFIAUDIO_GETCOMMAND_REMOVESYNCHRONOUSEX,
	WIFIAUDIO_GETCOMMAND_SETSYNCHRONOUSEX,
	WIFIAUDIO_GETCOMMAND_BLUETOOTH,
	WIFIAUDIO_GETCOMMAND_GETSPEECHRECOGNITION,
	WIFIAUDIO_GETCOMMAND_SETSPEECHRECOGNITION,
	WIFIAUDIO_GETCOMMAND_SETSINGLELIGHT,
	WIFIAUDIO_GETCOMMAND_GETALEXALANGUAGE,
	WIFIAUDIO_GETCOMMAND_SETALEXALANGUAGE,
	WIFIAUDIO_GETCOMMAND_UPDATACONEXANT,
	WIFIAUDIO_GETCOMMAND_SETTESTMODE,
	WIFIAUDIO_GETCOMMAND_CHANGENETWORKCONNECT,
	WIFIAUDIO_GETCOMMAND_GETDOWNLOADPROGRESS,
	WIFIAUDIO_GETCOMMAND_GETBLUETOOTHDOWNLOADPROGRESS,
	WIFIAUDIO_GETCOMMAND_DISCONNCTWIFI,
	WIFIAUDIO_GETCOMMAND_POWERONAUTOPLAY,
	WIFIAUDIO_GETCOMMAND_REMOVEALARMCLOCK,
	WIFIAUDIO_GETCOMMAND_ENABLEALARMCLOCK,
	WIFIAUDIO_GETCOMMAND_REMOVEPLAYPLAN,
	WIFIAUDIO_GETCOMMAND_GETALARMCLOCKANDPLAYPLAN,
	WIFIAUDIO_GETCOMMAND_GETPLANMUSICLIST,
	WIFIAUDIO_GETCOMMAND_ENABLEPLAYMUSICASPLAN,
	WIFIAUDIO_GETCOMMAND_GETPLANMIXEDLIST,
	WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTH,
	WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTHSTATE,
	WIFIAUDIO_GETCOMMAND_REMOVESHORTCUTKEYLIST,
	WIFIAUDIO_GETCOMMAND_GETSHORTCUTKEYLIST,
	WIFIAUDIO_GETCOMMAND_ENABLEAUTOTIMESYNC,
};

typedef struct _WAGC_ComParm
{
	enum WAGC_Command Command;				//枚举类型的命令
	union 
	{
		WAGC_pConectAp_t pConectAp;
		WAGC_pConnectAp_t pConnectAp;
		WAGC_pSubConnectAp_t pSubConnectAp;
		WAGC_pConnectApEx_t pConnectApEx;
		WAGC_pOneTouchState_t pOneTouchState;
		WAGC_pOneTouchNameState_t pOneTouchNameState;
		WAGC_pSetPlayerCmd_t pSetPlayerCmd;
		WAGC_pGetPlayerCmd_t pGetPlayerCmd;
		WAGC_pMultiroom_t pMultiroom;
		WAGC_pSSID_t pSSID;
		WAGC_pNetwork_t pNetwork;
		WAGC_pName_t pName;
		WAGC_pShutdown_t pShutdown;
		WAGC_pUpdateURL_t pUpdateURL;
		WAGC_pDateTime_t pDateTime;
		WAGC_pClockNum_t pClockNum;
		WAGC_pSelectLanguage_t pSelectLanguage;
		WAGC_pPlaylistIndex_t pPlaylistIndex;
		WAGC_pxzxAction_t pxzxAction;
		WAGC_pPlayTfCard_t pPlayTfCard;
		WAGC_pPlayUsbDisk_t pPlayUsbDisk;
		WAGC_pRemoveSynchronousEx_t pRemoveSynchronousEx;
		WAGC_pSetSynchronousEx_t pSetSynchronousEx;
		WAGC_pBluetooth_t pBluetooth;
		WAGC_pSetSpeechRecognition_t pSetSpeechRecognition;
		WAGC_pSetSingleLight_t pSetSingleLight;
		WAGC_pSetAlexaLanguage_t pSetAlexaLanguage;
		WAGC_pSetTestMode_t pSetTestMode;
		WAGC_pSetEnable_t pSetEnable;
		WAGC_pAlarmAndPlan_t pAlarmAndPlan;
		WAGC_pShortcutKey_t pShortcutKey;
	}Parameter;
}WAGC_ComParm,*WAGC_pComParm;//命令参数结构体

/******************************************
 ** 函数名: WIFIAudio_GetCommand_newComParm
 ** 功  能: 将pBuf指向命令字符串转换成WAGC_ComParm命令参数结构体返回
 ** 参  数: pBuf指向命令字符串的指针
 ** 返回值: 成功返回WAGC_ComParm命令参数结构体，失败返回NULL
 ******************************************/ 
extern WAGC_pComParm WIFIAudio_GetCommand_newComParm(const char *pBuf);


/******************************************
 ** 函数名: WIFIAudio_GetCommand_freeComParm
 ** 功  能: 将pComParm指向WAGC_ComParm命令参数结构体所申请的空间进行释放
 ** 参  数: pComParm指向WAGC_ComParm命令参数结构体的指针
 ** 返回值: 成功返回0，失败返回-1
 ******************************************/ 
extern int WIFIAudio_GetCommand_freeComParm(WAGC_pComParm *pComParm);

#ifdef __cplusplus
}
#endif

#endif		// __WIFIAUDIO_GETCOMMAND_H__