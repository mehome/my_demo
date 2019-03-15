#ifndef SYSTEM_CFG_H_
#define SYSTEM_CFG_H_ 1

typedef struct apInfList{
//	struct apInfList *next;
	char SSID[16];	//Wifi可见名称
	char bssid[18];	//Mac地址
	char rssi[4];		//信号强度
	char channel;	//Wifi信道
	char auth;		//是否加密
	char encry;		//加密类型
	char extch;
}apInfList;


//设备信息结构体
typedef struct DEVICEINFO{
//	struct apInfList *next;
	char sys_language[8];     //系统语言
	char sup_language[64];	  //支持语言
	char firmware[64];		//固件系列
	char release[32];		//发行版本号
	char mac_addr[32];		//设备mac地址
	char uuid[64];			//设备uuid
	char project[64];		//项目名称
	char battery_level[2];		//电池电量
	char function_support[64];	//功能支持
	char bt_ver[16];			//蓝牙固件版本号
	char key_num[2];				//按键个数
	char charge_plug[8];			//是否充电
	char audioname[32];			//设备ssid
	char project_uuid[128];			//设备ssid

}DEVICEINFO;


typedef struct __cdb_list {
    const char *name;
}cdb_list;
static cdb_list cdb_cmd_tbl[] = {
	{"$sys_language"},		//0
	{"$language_support"},	//1
	{"$firmware"},			//2
	{"$sw_build"},			//3	
	{"$mac_addr"},			//4
	{"$wifiaudio_uuid"},	//5
	{"$model_name"},		//6
	{"$function_support"},	//7
	{"$battery_level"},		//8
	{"$sw_ver"},			//9
	{"$key_num"},			//10
	{"$charge_plug"},		//11
	{"$audioname"},			//12
	{"$project_uuid"},			//13	
};



int get_aplist(apInfList **pAplist);
int connect_wifi(char* ssid,char* psw);
int disconnect_wifi(void);
int set_device_ssid(char *ssid);
int set_device_pwd(char *pwd);
char * search_connect_router_ssid(void);
int get_device_info(DEVICEINFO *aplist );
int get_wifi_ConnectStatus(void);
char* get_lan_ip(void);


#endif
