#ifndef __DUEROS_OTA_H__
#define __DUEROS_OTA_H__

#define TMP_FILE "/tmp/wifi_tmp.txt"
#define VOICE_FW_FILE "/tmp/firmware"
#define BT_FW_FILE "/tmp/bt_upgrade.bin"

//#define CONEX_MODE
#define DEBUG_ERROR(fmt, args...) do { printf("\033[1;31m""[%s:%d]"#fmt"\r\n""\033[0m", __func__, __LINE__, ##args); } while(0) 

#define DOWNLOAD_PROGRESS_DIR "/tmp/ota_down_pro"

/*#define EXIT_FAILURE -1*/
#define NONE "\033[0m"   			//正常终端颜色
#define RED "\033[0;32;31m"    		//红色
#define LIGHT_RED "\033[1;31m"  	//粗体红色
#define GREEN "\033[0;32;32m"    	//绿色
#define LIGHT_GREEN "\033[1;32m"  
#define BLUE "\033[0;32;34m"     	//蓝色
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"   	//暗灰色
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"  	//淡紫色
#define YELLOW "\033[1;33m"      	//黄色
#define WHITE "\033[1;37m"    		//粗体白色


#endif


