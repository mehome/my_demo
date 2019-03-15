#include <stdio.h>
#include <sys/reboot.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <wdk/cdb.h>

#include "update.h"
#include "../../function/common/misc.h"


int set_wifi_bt_update_flage(int status)
{
	int ret;
	ret = cdb_set_int("$wifi_bt_update_flage",status);
	return ret;
}

int get_wifi_bt_burn_progress(void)
{
	int handle;
    int bytes ;
    char progress_buf[8] = {0};
	char value[8] = "";
	char *ptr = NULL;
	handle=open(BURN_PROGRESS_DIR,O_RDONLY,S_IWRITE|S_IREAD);
//	handle=fopen(BURN_PROGRESS_DIR,"r");
	if(handle < 0){
	//	printf("error opening file\n");
		close(handle);
		return -1;
	}
	bytes=read(handle,progress_buf,6);
//	bytes=fread(progress_buf,6,1,handle);
	if(bytes < 0){		
	//    exit(1);
		close(handle);
		return -1;
	}else {
	//  printf("read:%d bytesread.\n",bytes);
	}
	close(handle);
//	fclose(handle);
	if(strlen(progress_buf) !=0){
		snprintf(value, sizeof(progress_buf), "%s", progress_buf);
    	ptr = value;	
	}else{
		return -1;
	}
	
	int pro;
	pro = atoi(ptr);
	return pro;
}



int get_wifi_bt_download_progress(void)
{
	int handle;
    int bytes ;
    char progress_buf[8] = {0};
	char value[8] = "";
	char *ptr;
  	handle=open(DOWNLOAD_PROGRESS_DIR,O_RDONLY,S_IWRITE|S_IREAD);
	if(handle < 0){
//		printf("error opening file\n");
		close(handle);
		return -1;
	}
	bytes=read(handle,progress_buf,6);
	if(bytes < 0){
		close(handle);
		return -1;
	}else {
	//  printf("read:%d bytesread.\n",bytes);
	}
	close(handle);
	if(strlen(progress_buf) !=0){
		snprintf(value, sizeof(progress_buf), "%s", progress_buf);
    	ptr = value;	
	}else{
		return -1;
	}
	int pro;
	pro = atoi(ptr);
	return pro;
}


int get_wifi_bt_update_flage(void)
{

	return (cdb_get_int("$wifi_bt_update_flage",0));

}

int app_update_noparmeter(char *pPath)
{
	int ret = 0;
	char tempbuff[256];
	if (pPath == NULL){
		ret = system("ota &");
	}else{
		memset(tempbuff, 0, sizeof(tempbuff));
		
		sprintf(tempbuff, "ota %s &", pPath);

		ret = system(tempbuff);
	}
	return ret;

}

int xzxhtml_wifiup(char *pPath)
{
	int ret = 0;
	char tempbuff[256];

	if(NULL == pPath)
	{
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		
		sprintf(tempbuff, "ota html_wifi %s &", pPath);

		ret = system(tempbuff);
	}

	return ret;

}

int xzxhtml_btup(char *pPath)
{
	int ret = 0;
	char tempbuff[256];

	if(NULL == pPath)
	{
		ret = -1;
	} else {
		memset(tempbuff, 0, sizeof(tempbuff));
		
		sprintf(tempbuff, "ota html_bt %s &", pPath);

		ret = system(tempbuff);
	}

	return ret;

}

char *get_conexant_version(void)
{
	char conex[8] = {0};
	char *ptr = NULL;
	char value[8] = "";
	cdb_get_str("$conex_fwver",conex,sizeof(conex),"");
	if(strlen(conex) !=0){
		snprintf(value, sizeof(conex), "%s", conex);
    	ptr = value;
	}else{
		ptr = NULL;
	}	
	return ptr;	
}

int conexant_update(char *sfs,char *bin)
{
	int ret = 0;
	char str[256];

	if (NULL == sfs || NULL == bin){
		return -1;
	}else{
		sprintf(str, "cxdish -D /dev/i2c-1 flash %s %s",sfs, bin);

		ret = system(str);
	}
	system("reboot");
	return ret;
}

int is_charge_plug(void)
{
	char charge_pl[8] = {0};
	char *ptr = NULL;
	char value[8] = "";
	cdb_get_str("$charge_plug",charge_pl,sizeof(charge_pl),"");
	if(strlen(charge_pl) !=0){
		snprintf(value, sizeof(charge_pl), "%s", charge_pl);
    	ptr = value;
	}else{
		return -1;
	}
	int pro;
	pro = atoi(ptr);
	return pro;
}


