#include <libcchip/platform.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include "mon_config.h"

#include <curl/curl.h>
#include <curl/easy.h> 
#include <wdk/cdb.h>
#include <time.h>
#include <signal.h>

#include "ota.h"
#include "app_btup.h"
int iUartfd;



int flage_conex = 0;
int flage_iflash = 0;
int flage_wifi_fw = 0;
int flage_bt_fw = 0;


/*conexant*/
char fw_ver[100] = { 0 };
char *tr = NULL;
char url_fw_ver[128] = { 0 };

/*wifi*/
char conex_fw_ver[100] = { 0 };
char conex_url_fw_ver[128] = { 0 };

/*bt*/
char bt_url_fw_ver[128] = { 0 };


int get_wifi_bt_flage = 0;
char bt_get_fw[1024] = {0};


#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})

/*
#define printf(fmt, args...) \
	do { \
		if (g_printf_on) \
			sysprintf(printf_INFO, fmt, ## args); \
	} while (0)
	*/


int writeline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "w");
    if (fp == NULL) {
        printf("open file %s failed", path);
        return -1;
    }
    if((fputs(buffer, fp)) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        printf("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}
#if 1

int str2args (const char *str, char *argv[], char *delim, int max)
{
    char *p;
    int n;
    p = (char *) str;
    for (n=0; n < max; n++)
    {
        if (0==(argv[n]=strtok_r(p, delim, &p)))
            break;
    }
    return n;
}

int str2argv(char *buf, char **v_list, char c)
{
    int n;
    char del[4];

    del[0] = c;
    del[1] = 0;
    n=str2args(buf, v_list, del, MAX_ARGVS);
    v_list[n] = 0;
    return n;
}

int exec_cmd2(const char *cmd, ...)
{
	char buf[MAX_COMMAND_LEN];
	va_list args;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	return exec_cmd(buf);
}

static void free_memory(int all)
{
    char alive_list[200] = { 0 };
    char *WDKUPGRADE_KEEP_ALIVE = NULL;

    if (all) {
        strcat(alive_list, "init\\|ash\\|watchdog\\|ota\\|telnetd\\|ps\\|$$");
    }
    else {
        strcat(alive_list, "init\\|uhttpd\\|omnicfg_bc\\|hostapd\\|ash\\|watchdog\\|ota\\|telnetd\\|http\\|ps\\|WIFIAudio_DownloadBurn\\|httpapi.cgi\\|posthtmlupdate.cgi\\|$$");
    }

    if ((WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE")) != NULL) {
        char *argv[10];
        int num = str2argv(WDKUPGRADE_KEEP_ALIVE, argv, ' ');
        while (num-- > 0) {
            strcat(alive_list, "\\|");
            strcat(alive_list, argv[num]);
        }
    }

    printf("Free memory now, all: [%d]", all);
    printf("Kill programs to free memory without: [%s]", alive_list);
    exec_cmd2("ps | grep -v '%s' | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9 > /dev/null 2>&1", alive_list);

    writeline("/proc/sys/vm/drop_caches", "3");
    sleep(1);
    writeline("/proc/sys/vm/drop_caches", "2");
    sync();
}
#endif
/*
	Check the firmware versions of ([cdb version] and [firmware.img version]) are same.

	same 		=> return 1
	not same	=> return 0

*/

size_t getFileSize(char * file)    
{   size_t size = -1;
    FILE * fp = fopen(file, "r");   
    if(fp == NULL) {
    	return -1;
    }
    fseek(fp, 0L, SEEK_END);   
    size = ftell(fp);   
    fclose(fp);   
    return size;   
} 

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    int len = size * nmemb;
    int written = len;
    FILE *fp = NULL;
    if (written == 0) 
    	return 0;
	
	printf("size:%d,nmemb:%d\n",size,nmemb);
	printf("stream---------:%s\n",stream);	// /tmp/firmware
	printf("ptr--------:%s\n",(char *)ptr);
		//DEBUG_INFO("ptr:%s",(char *)ptr);
    if (access((char*) stream, 0) == -1) {
        fp = fopen((char*) stream, "wb");
    } else {
        fp = fopen((char*) stream, "ab");
    }
    if (fp) {
        fwrite(ptr, size, nmemb, fp);
    }

    fclose(fp);
    return written;
}


/*用于打印下载的进度*/
int progress_func(char *progress_data,  
                     double dltotal, double dlnow,
                     double ultotal, double ulnow)  
{  
	printf("333333333333333333\n");
	fprintf(stderr,"\r%g%%", dlnow*100.0/dltotal);
	char string[8] = "";
	int length;
//	fprintf(stderr,"\r%d.0%%",dlnow*100.0/dltotal);
	sprintf(string,"\r%g%%",dlnow*100.0/dltotal);
	length = strlen(string);

	FILE *filp = NULL;  


	filp = fopen(DOWNLOAD_PROGRESS_DIR,"w+");  /* 可读可写，不存在则创建 */  
	int writeCnt = fwrite(string,length,1,filp);  /* 返回值为1 */  

	fclose(filp);  
	
  return 0;  
} 

static char *curl_get(char* url,int loadfw)
{
	CURL *curl;
	CURLcode res;
	 long length = 0;
	
	char * response = NULL;
	char *progress_data = "* ";  

	curl = curl_easy_init();
  	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);//下载ftp上的文件(如wifi文件)
			
		if (!loadfw){
			unlink(TMP_FILE);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, TMP_FILE);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		}else{
			if (1 == flage_conex){
				unlink(SFS_FILE);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, SFS_FILE);
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			}
			if(1 == flage_iflash){
				unlink(IFLASH_FILE);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, IFLASH_FILE);
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			}
			if (1 == flage_wifi_fw){
				unlink(FW_FILE);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, FW_FILE);	//下载后放入/tmp/firmware中的文件
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  
				curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  
				curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data); 
			  }
			if (1 == flage_bt_fw){
				unlink(BT_FW_FILE);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, BT_FW_FILE);// 下载后放入/tmp/bt_upgrade.bin文件中
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  
				curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  
				curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data); 
			  }
		}
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		//curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10l);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	 	//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
 		//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (!loadfw)
    	if (CURLE_OK == res) {
			length = getFileSize(TMP_FILE);
			printf("length:%ld",length);
			response = malloc(length);
			if(response == NULL) {
				printf("calloc failed");
				return NULL;
			}
			FILE *fp = NULL;
      		fp = fopen(TMP_FILE, "r");
      		if (fp) {
        		int read=0;
          	if( (read=fread(response, 1, length, fp)) != length){
          	  printf("fread failed read:%d",read);
          	  fclose(fp);
          	  return NULL;
          	}
          	fclose(fp);
         	 printf("fread ok read:%d",read);
      } else {
      	printf("fopen failed");
      }  
		}
	}
	
	return response;
}

int conex_compare_fw_sw(const char *fw_ver,const char *sw_ver)
{
		
	char fw_buf[9] = "";
	char sw_buf[9] = "";
	if (0 == strcmp(fw_ver,sw_ver))
		return -1;
	strlcpy(fw_buf,fw_ver,sizeof(fw_buf));
	strlcpy(sw_buf,sw_ver,sizeof(sw_buf));
	printf("atoi(fw_buf = %d  atoi(sw_buf) = %d \n",atoi(fw_buf),atoi(sw_buf));
	if(atoi(fw_buf) == atoi(sw_buf) || atoi(fw_buf) < atoi(sw_buf))
			return -1;
	else if(atoi(fw_buf) > atoi(sw_buf)){
			strcpy(conex_url_fw_ver,fw_buf);
			return 0;
		}
		
}

static int conex_check_fw_version(char * url)
{
    int handle;
    int bytes ;
    char sw_buf[10] = "";
    system("rm /tmp/iflash.bin");
		system("rm /tmp/conex*");
    system("cxdish fw-version");
    handle=open("/tmp/conex_fw_ver",O_RDONLY,S_IWRITE|S_IREAD);
    if(handle==-1)
    	{
        printf("error opening file\n");
      //  exit(1);
   		}
    bytes=read(handle,sw_buf,10);
    if(bytes==-1)
    	{
        printf("read failed.\n");
    //    exit(1);
    	}else {
        printf("read:%d bytesread.\n",bytes);
    	}
		close(handle);
		printf("cxdish version = %d\n",atoi(sw_buf));
		
		char sw_ver[100] = { 0 };
 		char sw_build[100] = { 0 };
    int ret = -1;    
   	char *pjon = NULL;
   	tr = NULL;
   	int i = 0;
   	char img[5] = {0};
   	char *_img = ".sfs";
   	char tmp_ver[32] = {0};
//   	flage_conex = 1;
	 	pjon = curl_get(url,0);	 		
		tr	= strstr(pjon,CONEX_FW_NAME);
		if (NULL == tr){
				printf("No valid the  firmware\n");
				return -1;
			}
//		flage_conex = 0;
		while(1)
		{
			strncpy(conex_fw_ver, &tr[sizeof(CONEX_FW_NAME)-1], 7);
			strncpy(img, &tr[sizeof(CONEX_FW_NAME)-1 + 7], 4);
			tr = tr + 15; //tr +
			
	    if (0 == strcmp(img,_img)){
	 			ret = conex_compare_fw_sw(conex_fw_ver,sw_buf);
	 			
	 			if  (0 == ret){
	 				strcpy(conex_url_fw_ver,conex_fw_ver);
					while(1){
						
						tr	= strstr(tr,CONEX_FW_NAME);						
						strncpy(conex_fw_ver, &tr[sizeof(CONEX_FW_NAME)-1], 7);
						strncpy(img, &tr[sizeof(CONEX_FW_NAME)-1 + 7], 4);					
						tr = tr + 15;	
						
					//	printf(" conex_fw_ver = %s tmp_ver = %s img = %s ret = %d\n",conex_fw_ver,tmp_ver,img,ret);
						if (0 == strcmp(img,_img)){
						//	strcpy(tmp_ver,conex_fw_ver);
						ret =	conex_compare_fw_sw(conex_fw_ver,conex_url_fw_ver);
						if (ret == 0 )
							strcpy(conex_url_fw_ver, conex_fw_ver);
						else if (-1 == ret){
								return 0;
							}		
						}		
								
						tr	= strstr(tr,CONEX_FW_NAME);
				 		if (NULL == tr && 0 == ret)
				 			return ret;		
				 	
					}
				}else if (-1 == ret){
				tr	= strstr(tr,CONEX_FW_NAME);
				if(NULL == tr && -1 == ret)
				return ret;				
			//	continue;
			}
		}
	 		tr	= strstr(tr,CONEX_FW_NAME);
	 		if (NULL == tr)
	 			return -1;
	 		if(-1 == ret)
	 			continue;
	 		else if (0 == ret)
	 			break;
 		}
    return ret;
		
}

void dance_func()
{
	system(PLAYER DOWNLOAD_FAIL);	
	printf("reboot\n");
	system("reboot");	

}

void init_sigaction()
{
	struct sigaction act;
		  
	act.sa_handler = dance_func; //设置处理信号的函数
	act.sa_flags  = 0;

	sigemptyset(&act.sa_mask);
	sigaction(SIGPROF, &act, NULL);//时间到发送SIGROF信号
}

void init_time()
 {
     struct itimerval val;
          
     val.it_value.tv_sec = 5; //1秒后启用定时器
     val.it_value.tv_usec = 0;
 
     val.it_interval = val.it_value; //定时器间隔为1s

     setitimer(ITIMER_PROF, &val, NULL);
}

 
void sigalrm_fn(int sig)  
{  
	int normal_state = 0;

	system(PLAYER DOWNLOAD_FAIL);	//固件下载失败
	cdb_set_int(WIFI_BT_UPDATE_FLAGE,normal_state);
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		exec_cmd("uartdfifo.sh StartConnectedFAL");
#endif
	printf("reboot\n");
	system("reboot");		//系统重启	  
	return;  
}  

void conex_up(char * url)
{		
	char byTmpBuffer[1024] = {0};
	flage_conex = 1;
	char *ptr;
	strcpy(byTmpBuffer, url);
	strcat(byTmpBuffer, CONEX_FW_NAME);
	strcat(byTmpBuffer,conex_url_fw_ver);
	strcat(byTmpBuffer,".sfs");
	printf("bytmpbuffer  =%s && download firmware\n",byTmpBuffer);
	//http://120.77.233.10/WIFI_FW_TIANTAO/wifi_test/FW_panther_20190105101821.img && download firmware flage_wifi_fw = 1
	ptr = curl_get(byTmpBuffer,1);
	flage_conex = 0;
}

void iflash_down(char * url)
{		
	char byTmpBuffer[1024] = {0};
	flage_iflash = 1;
	char *ptr;
	strcpy(byTmpBuffer, url);
	strcat(byTmpBuffer, "iflash.bin");
	printf("bytmpbuffer  =%s && download firmware\n",byTmpBuffer);
	ptr = curl_get(byTmpBuffer,1);
	flage_iflash = 0;
} 
 
void bt_download()
{
	
	strcpy(bt_get_fw, DOT_BT_FW_URL);
	strcat(bt_get_fw, BT_NAME);
	strcat(bt_get_fw, bt_url_fw_ver);
	strcat(bt_get_fw,".bin");
	printf("\n bt_url_fw_ver = %s bt_get_fw = %s && download firmware\n",bt_url_fw_ver,bt_get_fw);
	flage_bt_fw = 1;
	curl_get(bt_get_fw,1);
	flage_bt_fw = 0;
	
} 

void conexant_up()
{
#ifdef CONEX_MODE
	if (get_wifi_bt_flage != 1)		
	if(conex_check_fw_version(DOT_CONEX_FW) == 0){
		system(PLAYER CONEX_CHECK_VER);
		iflash_down(DOT_CONEX_FW);	
		conex_up(DOT_CONEX_FW);
		system("cxdish -D /dev/i2c-1 flash /tmp/conex_image.sfs /tmp/iflash.bin");
		if (get_wifi_bt_flage == 0){
			system(PLAYER UPGRADED);
			sleep(1);				
			system("reboot");
		}
	}
#endif
}

 
void wifi_up(char * url,int app_flage)
{
	signal(SIGALRM, sigalrm_fn);  //后面的函数必须是带int参数的
	alarm(540); 
	char *ptr;
	char byTmpBuffer[1024] = {0};
//	int wifi_uping = 4;
//	int wifi_downing = 6;

//	eval("uartdfifo.sh", "upwifi");		
//	free_memory(0);

	system(PLAYER UPGRADEING);	
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartWifiUp");	
#endif
	strcpy(byTmpBuffer, url);
	strcat(byTmpBuffer, WIFI_NAME);
	strcat(byTmpBuffer,url_fw_ver);
	strcat(byTmpBuffer,".img");
	printf("\n bytmpbuffer  =%s && download firmware flage_wifi_fw = 1\n",byTmpBuffer);
	flage_wifi_fw = 1;
	cdb_set_int(WIFI_BT_UPDATE_FLAGE,WIFI_DOWNING);
	ptr = curl_get(byTmpBuffer,1);
	flage_wifi_fw = 0;
	cdb_set_int(WIFI_BT_UPDATE_FLAGE,WIFI_UPING);
	cdb_set_int("$amazon_loginStatus", 0);
	cdb_save();
	eval("/sbin/sysupgrade", "/tmp/firmware");	//升级程序		
}

void bt_up(char * url)
{
//	int bt_uping = 5;
//	int bt_downing = 7;
	signal(SIGALRM, sigalrm_fn);  //后面的函数必须是带int参数的
	alarm(360); 
	
//	free_memory(0);
	system(PLAYER BLU_UPGRADE);

	system("killall -9 uartd");
	strcpy(bt_get_fw, url);
	strcat(bt_get_fw, BT_NAME);
	strcat(bt_get_fw, bt_url_fw_ver);
	strcat(bt_get_fw,".bin");
	printf("\n bt_get_fw  =%s && download firmware\n",bt_get_fw);

	cdb_set_int(WIFI_BT_UPDATE_FLAGE,BT_DOWNING);
	flage_bt_fw = 1;
	curl_get(bt_get_fw,1);
	flage_bt_fw = 0;
	cdb_set_int(WIFI_BT_UPDATE_FLAGE,BT_UPING);
	system("/usr/bin/btup");
	
//	set_opt(115200,8,0,1,BT_FW_FILE);
}

void html_wifiup(char * url)
{
	char byTmpBuffer[1024] = {0};
	char upbuf[1024];
//	int web_uping = 8;
	//free_memory(0);

	strcpy(byTmpBuffer, url);
    system(PLAYER UPGRADEING);
    cdb_set_int(WIFI_BT_UPDATE_FLAGE,WEB_WIFI_UPING);
    printf("byTmpBuffer = %s\n",byTmpBuffer);
	cdb_set_int("$amazon_loginStatus", 0);
	cdb_save();
   	eval("/sbin/sysupgrade", byTmpBuffer);
}

void html_btup(char * url)
{
	char byTmpBuffer[1024] = {0};
	char upbuf[1024];
//	int web_uping = 9;
	//free_memory(0);

	strcpy(byTmpBuffer, url);
    system(PLAYER UPGRADEING);
    cdb_set_int(WIFI_BT_UPDATE_FLAGE,WEB_BT_UPING);
    printf("byTmpBuffer = %s\n",byTmpBuffer);

	system("killall -9 uartd");
	system("/usr/bin/btup");
  // 	set_opt(115200,8,0,1,url);
}

void update_parmeter(char *argv)
{
	conexant_up();
//	err("get_wifi_bt_flage test2222222 >>>>>>%d",get_wifi_bt_flage);
	switch(get_wifi_bt_flage){
		case 0:
			eval(PLAYER,LATEST_VERSION);
			
			break;
		case 1:
			bt_up(argv);
			break;
		case 2:
			wifi_up(argv,0);
			break;
		case 3:
			wifi_up(argv,1);				
			break;
		default:
			break;				
	}
}

void update_noparmeter()
{

 	char ota_wifi_path[128] = {0};
	char ota_bt_path[128] = {0};

	cdb_get("$ota_wifi_path",ota_wifi_path);
	cdb_get("$ota_bt_path",ota_bt_path);
	err("get_wifi_bt_flage test :>>%d",get_wifi_bt_flage);
	switch(get_wifi_bt_flage){
		case 0:
//			system(PLAYER LATEST_VERSION);
			break;
		case 1:
			bt_up(ota_bt_path);
			break;
		case 2:
			wifi_up(ota_wifi_path,0);
			break;
		case 3:
			wifi_up(ota_wifi_path,1);						
			break;
		default:
			break;				
	}
}
char get_charge_status[4] = {0};
char  BatteryValue[3] = {0};
int  wifi_get_charge_status =0;
int BatteryValueLevel = 0;
void get_ver_flage(void)
{
	cdb_get_str("$charge_plug",get_charge_status,4,"");
	cdb_get_str("$server_wifi_ver",url_fw_ver,128,"");
	cdb_get_str("$server_bt_ver",bt_url_fw_ver,128,"");
	
	get_wifi_bt_flage = cdb_get_int(WIFI_BT_UPDATE_FLAGE,get_wifi_bt_flage);
	
	printf("get_wifi_bt_flage = %d \n",get_wifi_bt_flage);

}

int main(int argc, char *argv[])
{
	char noparmeterbuf[128] = {0};
 	char ota_wifi_path[128] = {0};
	char ota_bt_path[128] = {0};
	int BatteryLevel = 0;
    
	cdb_get("$ota_wifi_path",ota_wifi_path);
	cdb_get("$ota_bt_path",ota_bt_path);
    
#if defined(CONFIG_PROJECT_BEDLAMP_V1) || defined(CONFIG_PROJECT_HUABEI_V1)
    BatteryLevel = 10;
    strcpy(get_charge_status,"001");
#elif defined(CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
	cdb_set_int("$battery_first_state",0);
    BatteryLevel = cdb_get_int("$battery_level",0);
	cdb_get_str("$charge_plug",get_charge_status,4,"");
    
#endif

	err("charge_plug =%s,battery_level =%d",get_charge_status,BatteryLevel);
	/*网页升级调用*/
	if (argv[1] != NULL && argv[2] != NULL)
    { 
		if (0 == strcmp(argv[1],"html_wifi"))
        {
			html_wifiup(argv[2]);
		}
        else if (0 == strcmp(argv[1],"html_bt"))
        {
			html_btup(argv[2]);
		}
	}
	/*APP升级调用*/
	if (0 == strcmp(get_charge_status,"001"))	//当前正充电
	{		
		if (argc == 1)
        {
		//	system("/usr/bin/check-version");
			get_ver_flage();
#ifdef CONEX_MODE		
			conexant_up();
#endif
			update_noparmeter();
		}
        else if (argc == 2)
        {
			if (argv[1] != NULL)
            {
				sprintf(noparmeterbuf,"/usr/bin/check-version %s",argv[1]);
				system(noparmeterbuf);
				get_ver_flage();
				update_parmeter(argv[1]);
			}
		}
        else
		{
			printf("Please usag 'ota  html_wifi /tmp/xxx.img' or 'ota'\n");
		}
	}
	else
    {
		if(BatteryLevel < 4)		//没有充电，当前电量小于3	
		{
			get_wifi_bt_flage = cdb_get_int(WIFI_BT_UPDATE_FLAGE,get_wifi_bt_flage);
			if((get_wifi_bt_flage == 1)||(get_wifi_bt_flage == 2)||(get_wifi_bt_flage == 3))
			{
				printf("Please insert power\n");
				system(PLAYER LOWER_NOT_UPGRADE);
			}
            else
            {
                system(PLAYER LOW_POWER);
            }
            return 0;
        }
        else
        {                                       //没有充电，当前电量>3
            if (argc == 1)
            {
                get_ver_flage();
#ifdef CONEX_MODE		
                conexant_up();
#endif
                update_noparmeter();
            }
            else if (argc == 2)
            {
                if (argv[1] != NULL)
                {
                    sprintf(noparmeterbuf,"/usr/bin/check-version %s",argv[1]);
                    system(noparmeterbuf);
                    get_ver_flage();
                    update_parmeter(argv[1]);
                }
            }
            else
            {
                printf("Please usag 'ota  html_wifi /tmp/xxx.img' or 'ota'\n");
            }
        }
    }
	return 0;		
}
