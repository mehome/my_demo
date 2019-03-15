#ifndef __OTA_H__
#define __OTA_H__

#define TMP_FILE "/tmp/wifi_tmp.txt"
#define FW_FILE "/tmp/firmware"

//#define CONEX_MODE

/*conexant*/
#define SFS_FILE "/tmp/conex_image.sfs"
#define IFLASH_FILE "/tmp/iflash.bin"
#define CONEX_FW_NAME  "Conexant_firmware_"
#define DOT_CONEX_FW "http://47.88.100.159/DOT_CONEX_FW/"
//#define DOT_CONEX_FW "http://120.77.233.10/test_wifi/"
#define BACKUP_QC_CONEX_FW "http://120.77.233.10/BACKUP_QC_CONEX_FW/"
/*****************************************************************************************/

/*wifi*/

#define WIFI_NAME  "FW_panther_"

#define DOWNLOAD_PROGRESS_DIR "/tmp/wifi_bt_download_progress"
//#define DOT_WIFI_FW_URL "http://47.88.100.159/DOT_WIFI_FW/"
#define DOT_WIFI_FW_URL "http://120.77.233.10/test_wifi/"
//#define DOT_WIFI_FW_URL "http://47.88.100.159/QC_IW700_WIFI/"
#define g_printf_on 1

/*****************************************************************************************/


/*bt*/
#define BT_NAME  "bt_firmware_"
#define DOT_BT_FW_URL "http://120.77.233.10/test_wifi/"
//#define DOT_BT_FW_URL "http://47.88.100.159/QC_IW700_BT/"

#define BT_FW_FILE "/tmp/bt_upgrade.bin"


/*****************************************************************************************/

#define WIFI_BT_UPDATE_FLAGE "$wifi_bt_update_flage"

#if 1
	#define PLAYER "wavplayer"
	#define UPGRADEING " /root/res/cn/upgrading.wav"
	#define BLU_UPGRADE " /root/res/cn/bluetooth_upgrade.wav"
	#define LATEST_VERSION " /root/res/cn/latest_version.wav &"
	#define CONEX_CHECK_VER " /root/res/cn/check_firmware.wav &"
	#define UPGRADED 	" /root/res/cn/upgraded.wav &"
	#define DOWNLOAD_FAIL " /root/res/cn/download_failed.wav &"
	#define INSERT_POWER " /root/res/cn/insert_power_supply.wav &"
	#define LOWER_NOT_UPGRADE " /root/res/cn/lowpower_not_upgrade.wav &"
	#define LOW_POWER " /root/res/cn/lowpower.wav &"
#else
	#define PLAYER "wavplayer"
	#define UPGRADEING " /tmp/res/upgrading.mp3"
	#define BLU_UPGRADE " /tmp/res/bluetooth_upgrade.mp3"
	#define LATEST_VERSION " /tmp/res/latest_version.mp3 &"
	#define LATEST_VERSION " /tmp/res/latest_version.mp3 &"
	#define CONEX_CHECK_VER " /tmp/res/check_firmware.mp3 &"
	#define UPGRADED " /tmp/res/upgraded.mp3 &"
	#define DOWNLOAD_FAIL " /tmp/res/download_failed.mp3 &"
	#define INSERT_POWER " /tmp/res/insert_power_supply.mp3 &"
#endif

#define MAX_COMMAND_LEN		1024
#define MAX_ARGVS    20
/*#define EXIT_FAILURE -1*/


enum WIFI_BT_FLAGE{
	WIFI_UPING=4,//wifi_uping=4,
	BT_UPING,		//bt_uping=5
	WIFI_DOWNING,	//wifi_downing=6
	BT_DOWNING,	//bt_downing
	WEB_WIFI_UPING,					//web_bt_uping
	WEB_BT_UPING,			
};


extern int iUartfd;

#endif


