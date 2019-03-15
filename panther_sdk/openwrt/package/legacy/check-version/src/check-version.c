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
#include <libcchip/platform.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <wdk/cdb.h>
#include "mon_config.h"
#include <time.h>
#include <signal.h>

#include "check-version.h"

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
	printf("stream---------:%s\n",stream);
	printf("ptr--------:%s\n",(char *)ptr);
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


int progress_func(char *progress_data,  
                     double dltotal, double dlnow,
                     double ultotal, double ulnow)  
{  

	 printf("222222222222222222222\n");
	  printf("\r(%g %%)", dlnow*100.0/dltotal);  
	  unsigned char  a;
 		a = (unsigned char)(dlnow*100.0/dltotal);

		FILE *filp = NULL;  
 
    unsigned char dataPtr[1] = {0};
    dataPtr[0] = a;

    filp = fopen(PROGRESS_DIR,"w+");  /* 可读可写，不存在则创建 */  
    int writeCnt = fwrite(dataPtr,sizeof(dataPtr),1,filp);  /* 返回值为1 */  
  
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
	unlink(TMP_FILE);
	unlink(FW_FILE);
	curl = curl_easy_init();
  	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
			
		if (!loadfw){	//不下载
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, TMP_FILE);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		}else{			//下载
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, FW_FILE);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  
		    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  
		    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data); 
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
			printf("length:%ld\n",length);
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
          	  printf("fread failed read:%d\n",read);
          	  fclose(fp);
          	  return NULL;
          	}
          	fclose(fp);
         	// printf("fread ok read:%d\n",read);
      } else {
      	printf("fopen failed");
      }  
		}
	}
	
	return response;
}

int compare_fw_sw(const char *Fw_ver,const char *sw_ver)
{
		
	char fw_buf[9] = "";
	char sw_buf[9] = "";
	if (0 == strcmp(Fw_ver,sw_ver))
		return 1;
	strlcpy(fw_buf,Fw_ver,sizeof(fw_buf));
	strlcpy(sw_buf,sw_ver,sizeof(sw_buf));
	if (atoi(fw_buf) == atoi(sw_buf)){
		strncpy(fw_buf,&Fw_ver[8],8);
		strncpy(sw_buf,&sw_ver[8],8);
//		printf("\n server_wifi_ver = %d local_sw_build = %d\n",atoi(fw_buf),atoi(sw_buf));
		if(atoi(fw_buf) == atoi(sw_buf) || atoi(fw_buf) < atoi(sw_buf))
			return -1;
		else
			return 0;
	}else if(atoi(fw_buf) > atoi(sw_buf)){
	//	strcpy(url_fw_ver,fw_buf);
		return 0;
	}else
		return -1;
		
}


int bt_compare_fw_sw(const char *fw_ver,const char *sw_ver)
{
		
	char fw_buf[9] = "";
	char sw_buf[9] = "";
	if (0 == strcmp(fw_ver,sw_ver))
		return 1;
	strlcpy(fw_buf,fw_ver,sizeof(fw_buf));
	strlcpy(sw_buf,sw_ver,sizeof(sw_buf));
//	INFO("\n server_bt_ver = %d sw_ver = %d \n",atoi(fw_buf),atoi(sw_buf));
	if(atoi(fw_buf) == atoi(sw_buf) || atoi(fw_buf) < atoi(sw_buf))
	{
		return -1;
	}else if(atoi(fw_buf) > atoi(sw_buf)){
		strcpy(bt_url_fw_ver,fw_buf);
//		INFO("\n bt_url_fw_ver = %s \n",bt_url_fw_ver);
		return 0;
	}
		
}

static int bt_check_firmware_version(char * url)
{
	char buf[256]={0};
	char fw_ver[128] = { 0 };
    int ret;    
   	char *pjon = NULL;
   	int i = 0;
   	char bin[5] = {0};
   	char *_bin = ".bin";
   	char pretval[16] = {0};
   	int sw_ver_int;
    char tmp_ver[32] = {0};
  	int url_len;

	cdb_get("$sw_ver",pretval);
 	pjon = curl_get(url,0);	
	err("pjon :bt>>>>>>>>[%s]",pjon);
	if (NULL == pjon){
	//	perror("pjon");
		return 	ENOENT;
	}

	tr	= strstr(pjon,BT_NAME);
	if (NULL == tr){
		printf("BT No valid the  firmware\n");
		return ENOENT;
	}
	while(1)
	{
		strncpy(fw_ver, &tr[sizeof(BT_NAME)-1], 8);
		strncpy(bin, &tr[sizeof(BT_NAME)-1 + 8], 4);

		tr = tr + 5; //tr
		
	    if (0 == strcmp(bin,_bin)){
	 			ret = bt_compare_fw_sw(fw_ver,pretval);	 			
	 			if (0 == ret){
				//	INFO("fw_ver = %s bt_url_fw_ver = %s\n",fw_ver,bt_url_fw_ver);
	 				strcpy(bt_url_fw_ver,fw_ver);
					while(1){						
						tr	= strstr(tr,BT_NAME);
						if ((NULL == tr && 0 == ret) || (NULL == tr && 1 == ret)){
							return 0;
						}
						strncpy(fw_ver, &tr[sizeof(BT_NAME)-1], 8);
						strncpy(bin, &tr[sizeof(BT_NAME)-1 + 8], 4);					
						tr = tr + 5;	
						
	//					printf(" fw_ver = %s tmp_ver = %s img = %s ret = %d\n",fw_ver,tmp_ver,bin//,ret);
						if (0 == strcmp(bin,_bin)){
						//	strcpy(tmp_ver,fw_ver);
							ret = bt_compare_fw_sw(fw_ver,bt_url_fw_ver);
							if (ret == 0 ){
								strcpy(bt_url_fw_ver, fw_ver);
							}else if (-1 == ret){
									return 0;
							
							}else if(1 == ret){
								tr = strstr(tr,BT_NAME);
								if(NULL == tr)
									return 0;
								break;
							}	
							tr	= strstr(tr,BT_NAME);
					 		if (NULL == tr && 0 == ret)
					 			return ret;		
				 	
						}
					}
				}else if (-1 == ret){
				tr	= strstr(tr,BT_NAME);
				if(NULL == tr && -1 == ret)
				return ret;				
			//	continue;
			}
		}
		tr	= strstr(tr,BT_NAME);
		if (NULL == tr)
			return -1;
		if(-1 == ret)
			continue;
		else if (0 == ret)
			break;
	}
    return ret;
}

//http://120.77.233.10/WIFI_FW_TIANTAO/wifi_test/
static int wifi_check_firmware_version(char * url)
{
    char sw_ver[100] = { 0 };
 	char sw_build[100] = { 0 };
    int ret = -1;    
   	char *pjon = NULL;
   	int i = 0;
   	char img[5] = {0};
   	char *_img = ".img";
   	char tmp_ver[32] = {0};
   	 	
   	cdb_get("$sw_build",sw_build);
	err("sw_build:%s",sw_build);
 	pjon = curl_get(url,0);
	err("pjon :wifi>>>>>>>>[%s]",pjon);
 	if (NULL == pjon){
		perror("pjon");
		return 	ENOENT;//返回2
	}
 		 		
	tr	= strstr(pjon,WIFI_NAME);	//找到对应的版本信息，如:FW_panther_20181126160628.img
	if (NULL == tr){
		printf("wifi No valid the  firmware\n");
		return ENOENT;
	}
	while(1)
	{
		strncpy(fw_ver, &tr[sizeof(WIFI_NAME)-1], 14);	//取出日期:如fw_ver=20181126160628
		strncpy(img, &tr[sizeof(WIFI_NAME)-1 + 14], 4);//取出后缀:img=img
		tr = tr + 5; //tr
	    if (0 == strcmp(img,_img)){		//判断是否为wifi升级文件
 			ret = compare_fw_sw(fw_ver,sw_build);	//比较日期达小
 			
 			if (0 == ret){		//有新版本的情况
 				strcpy(url_fw_ver,fw_ver);
				while(1){	
					tr	= strstr(tr,WIFI_NAME);
					if ((NULL == tr && 0 == ret) || (NULL == tr && 1 == ret))
				 		return 0;	
					strncpy(fw_ver, &tr[sizeof(WIFI_NAME)-1], 14);
					strncpy(img, &tr[sizeof(WIFI_NAME)-1 + 14], 4);					
					tr = tr + 5;
					if (0 == strcmp(img,_img)){
					//	strcpy(tmp_ver,fw_ver);
					//	printf("%d: fw_ver = %s url_fw_ver = %s\n",__LINE__,fw_ver,url_fw_ver);
						ret = compare_fw_sw(fw_ver,url_fw_ver);
						if (ret == 0 ){
							strcpy(url_fw_ver, fw_ver);
						}else if (-1 == ret){
							return 0;
						}else if(1 == ret){
								tr	= strstr(tr,WIFI_NAME);
								if(NULL == tr)
									return 0;
							//	break;
						}			
								
					//	tr	= strstr(tr,WIFI_NAME);
				 	//	if (NULL == tr && 0 == ret)
				 	//		return ret;		
				 	
					}
				}
			}else if (-1 == ret){
					tr	= strstr(tr,WIFI_NAME);
					if(NULL == tr && -1 == ret)
					return ret;				
				//	continue;
			}
		}
 		tr	= strstr(tr,WIFI_NAME);
 		if (NULL == tr)
 			return -1;
 		if(-1 == ret)
 			continue;
 		else if (0 == ret)
 			break;
 	}
    return ret;
}

void get_conex_ver(void)
{
	int handle;
    char sw_buf[16] = {0};	
#ifdef CONEX_MODE    
    system("cxdish fw-version");
    handle=open("/tmp/conex_fw_ver",O_RDONLY,S_IWRITE|S_IREAD);
	if(handle < 0)
	{
		printf("error opening file\n");
		//  exit(1);
	}else {
		bytes=read(handle,sw_buf,10);
		if(bytes==-1)
		{		
		//    exit(1);
		}else {
			cdb_set("$conex_fwver",sw_buf);
		}
	}
	close(handle);
//	printf("cxdish version = %d\n",atoi(sw_buf));

#endif	

}
int main(int argc, char *argv[])
{

	char sw_build[16] = {0};
	int wifi_ret = 0,bt_ret = 0;
	int backup_wifi_ret,backup_bt_ret;
    int bytes ;
 	char ota_wifi_path[128] = {0};
	char ota_bt_path[128] = {0};
 	char backup_ota_wifi_path[128] = {0};
	char backup_ota_bt_path[128] = {0};	
	char back_tmp[128] = {0};

	cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_BT_DONTUP);//设置0
    exec_cmd("crontab /usr/bin/root &");
	
	cdb_get("$sw_build",sw_build);
	cdb_get("$ota_wifi_path",ota_wifi_path);				//http://120.77.233.10/WIFI_FW_TIANTAO/wifi_test/
	cdb_get("$ota_bt_path",ota_bt_path);					//http://120.77.233.10/WIFI_FW_TIANTAO/bt_test/
	cdb_get("$backup_ota_wifi_path",backup_ota_wifi_path);	//http://120.77.233.10/BACKUP_QC_WIFI_FW/
	cdb_get("$backup_ota_bt_path",backup_ota_bt_path);		//http://120.77.233.10/BACKUP_QC_BT_FW/

	err("$sw_build =%s",sw_build);
	err("$ota_wifi_path =%s ,ota_bt_path =%s",ota_wifi_path,ota_bt_path);
	err("$backup_ota_wifi_path =%s ,$backup_ota_bt_path =%s",backup_ota_wifi_path,backup_ota_bt_path);
	get_conex_ver();
#if !defined(CONFIG_PROJECT_BEDLAMP_V1)  
	exec_cmd("uartdfifo.sh getbat");			//获取电池的插拔状态/电池电量
#endif	
	if (1 == argc){		//done
		wifi_ret = wifi_check_firmware_version(ota_wifi_path);
		bt_ret = bt_check_firmware_version(ota_bt_path);
	}else if (2 == argc){
		if( argv[1] != NULL){
			wifi_ret = wifi_check_firmware_version(argv[1]);
			bt_ret = bt_check_firmware_version(argv[1]);
		}
	}
	backup_wifi_ret = wifi_check_firmware_version(backup_ota_wifi_path);
	backup_bt_ret = bt_check_firmware_version(backup_ota_bt_path);
	
	err("wifi_ret  =%d,bt_ret =%d",wifi_ret,bt_ret);
	err("backup_wifi_ret = %d backup_bt_ret = %d\n",backup_wifi_ret,backup_bt_ret);
	

	if (backup_wifi_ret != 0) {
		if (backup_bt_ret != 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_BT_DONTUP);//0,done
		}else if(backup_bt_ret == 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_BT_UP);
			cdb_set(SERVER_BT_VER,bt_url_fw_ver);			
			memset(back_tmp,0,sizeof(back_tmp));
			sprintf(back_tmp,"ota %s &",backup_ota_bt_path);
			system(back_tmp);
			return 0;
		}		
	}else if (backup_wifi_ret == 0){
		cdb_set(SERVER_WIFI_VER,url_fw_ver);
		if (backup_bt_ret != 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_UP);		//2,done
			memset(back_tmp,0,sizeof(back_tmp));
			sprintf(back_tmp,"ota %s &",backup_ota_wifi_path);
			system(back_tmp);
			return 0;
		}else if(backup_bt_ret == 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_BT_UP);	
			cdb_set(SERVER_BT_VER,bt_url_fw_ver);
			memset(back_tmp,0,sizeof(back_tmp));
			sprintf(back_tmp,"ota %s &",backup_ota_wifi_path);
			system(back_tmp);
			return 0;
		}
	}else{
		printf("NOT ACTIVATED  BACKUP SERVER\n");	
	}			
	/*返回-1是不需要升级，0是需要升级			*/
	if (wifi_ret != 0) { 		//是否升级蓝牙
		if (bt_ret != 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_BT_DONTUP);//0,done
			INFO("Wifi and bt need't up\n");
		}else if(bt_ret == 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_BT_UP);	//1,not update
			cdb_set(SERVER_BT_VER,bt_url_fw_ver);
			INFO("Bt have new version:%s\n",bt_url_fw_ver);	//update bt,将获取到的wifi版本号设置到cdb
		}		
	}else if (wifi_ret == 0){	//是否升级wifi
		cdb_set(SERVER_WIFI_VER,url_fw_ver);				//将获取到的wifi版本号设置到cdb
		INFO("Wifi have new version:%s\n",url_fw_ver);		
		if (bt_ret != 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_UP);	//2,update wifi
		}else if(bt_ret == 0){
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_BT_UP);
			cdb_set(SERVER_BT_VER,bt_url_fw_ver);
			INFO("Bt have new version:%s\n",bt_url_fw_ver);	//update wifi
		}else{
			cdb_set_int(WIFI_BT_UPDATE_FLAGE,APP_WIFI_UP);
		}
	}

	return 0;		
	
}
