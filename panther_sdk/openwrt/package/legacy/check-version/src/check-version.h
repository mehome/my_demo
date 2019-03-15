#ifndef __CHECK_VERSION_H__
#define __CHECK_VERSION_H__

/********************************CONEX start************************************************/
#define TMP_FILE "/tmp/url_tmp_file.txt"
#define FW_FILE "/tmp/firmware"

//#define CONEX_MODE
#define SFS_FILE "/tmp/conex_image.sfs"
#define IFLASH_FILE "/tmp/iflash.bin"
#define CONEX_FW_NAME  "Conexant_firmware_"
#define DOT_CONEX_FW "http://47.88.100.159/DOT_CONEX_FW/"
//#define DOT_CONEX_FW "http://120.77.233.10/test_wifi/"

int flage_conex = 0;
int flage_iflash = 0;
int flage_wifi_fw = 0;
/******************************CONEX end**************************************************/


/********************************WIFI start************************************************/

//#define WIFI_NAME  "F8098_firmware_"
#define WIFI_NAME  "FW_panther_"

#define PROGRESS_DIR "/tmp/wifi_bt_download_progress"
#define BACKUP_QC_WIFI_FW "http://120.77.233.10/BACKUP_QC_WIFI_FW/"
#define DOT_WIFI_FW "http://120.77.233.10/test_wifi/"
#define g_printf_on 1

char fw_ver[100] = { 0 };
char *tr = NULL;
char *url_fw_ver[128] = { 0 };

char conex_fw_ver[100] = { 0 };
char conex_url_fw_ver[128] = { 0 };
/******************************WIFI end**************************************************/



/********************************BT start************************************************/

char bt_url_fw_ver[128] = { 0 };
#define BT_NAME  "bt_firmware_"
#define BACKUP_QC_BT_FW "http://120.77.233.10/BACKUP_QC_BT_FW/"
#define DOT_BT_FW "http://120.77.233.10/test_wifi/"

/******************************BT end**************************************************/


#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})

#define WIFI_BT_UPDATE_FLAGE "$wifi_bt_update_flage"
#define SERVER_WIFI_VER "$server_wifi_ver"
#define SERVER_BT_VER 	 "$server_bt_ver"

/*
#define printf(fmt, args...) \
	do { \
		if (g_printf_on) \
			sysprintf(printf_INFO, fmt, ## args); \
	} while (0)
	*/

#define MAX_COMMAND_LEN		1024
#define MAX_ARGVS    20
/*#define EXIT_FAILURE -1*/

enum WIFI_BT_FLAGE{
	APP_WIFI_BT_DONTUP  ,//   app_wifi_bt_dotup=0,
	APP_BT_UP,	//  app_bt_up,
	APP_WIFI_UP,	//app_wifi_up,
	APP_WIFI_BT_UP,	// app_wifi_bt_up,
};


#define _DEBUG_ 1

#if _DEBUG_
#define INFO(...) printf(__VA_ARGS__)
#else
#define INFO(...)
#endif

#endif


