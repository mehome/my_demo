#include <stdio.h>
#include <wdk/cdb.h>
#include <sys/reboot.h>
#include "system_cfg.h"
#include "../../function/common/misc.h"

//3.1.1	获取当前环境下wifi的SSID				测试OK
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padding_char = '=';


#define APLIST_LEN 2048
int get_aplist(apInfList **pAplist)
{
	char *aplist = NULL, *ssid = NULL, *bssid = NULL, *signal = NULL, *channel = NULL;
	struct json_object *root = NULL;
	char *psignal = NULL, *tpsignal = NULL;
	char value[CDB_DATA_MAX_LENGTH]="";
	int cur_size=0, aplist_len = 0;
	int i=0, ap_count = 0;
	apInfList *pst_ap_list = NULL;
	char *ap[48] = {0};
	
	aplist=realloc(aplist,APLIST_LEN);
	if(!aplist){
		err("realloc %d failure",i);
		return -2;			
	}
	my_popen_get(aplist, APLIST_LEN, "iw dev sta0 scan | grep -E \"Au|SSID|signal|BSS|DS Parameter set\"");
	aplist_len = strlen(aplist);
	
	printf("===========\n%s\n============\n", aplist);
	for(; i < aplist_len; i++){
		psignal = strstr(&aplist[i], "BSS");
		if(psignal != NULL){
			ap[ap_count++] = psignal;
			i = psignal - aplist;
			if(i > 0)
				aplist[i - 1] = '\0';
		}else{
			break;
		}
	}
	
	pst_ap_list = (apInfList *)calloc(ap_count, sizeof(apInfList));
	if(pst_ap_list == NULL){
		err("melloc failure");
		return -2;
	}
	memset(pst_ap_list, 0, sizeof(apInfList) * ap_count);
	int j = 0;
	printf("ap count: %d\n", ap_count);
	for(i = 0; i < ap_count; i++){
		printf("\nap[%d]: %s\n", i, ap[i]);
		bssid = strstr(ap[i], "BSS");
		if(bssid != NULL){
			strncpy(pst_ap_list[i].bssid, &bssid[4], 17);
			printf("bssid: %s\n", pst_ap_list[i].bssid);
		}
		signal = strstr(ap[i], "signal");
		if(signal != NULL){
			strncpy(pst_ap_list[i].rssi, &signal[8], 4);
			printf("rssi: %s\n", pst_ap_list[i].rssi);
		}
		channel = strstr(ap[i], "channel");
		if(channel != NULL){
			pst_ap_list[i].channel = channel[8];
			printf("channel: %c\n", pst_ap_list[i].channel);
		}
		ssid = strstr(ap[i], "SSID");	
		if(ssid != NULL){
			printf("ssid[%d]:%s\n", i, ssid);
			for(j = 0; j < 16; j++){				
				if(ssid[j + 6] != '\n' ){
					printf("[%x]", ssid[j + 6]);
					pst_ap_list[i].SSID[j] = ssid[j + 6];
				}else{
					break;
				}
			}
		}
		pst_ap_list[i].auth = 0;
		pst_ap_list[i].encry = 0;
		pst_ap_list[i].extch = 0;
		
	}
	free(aplist);
/*	
	if(psignal){
		ap_count++;
		for(; i < aplist_len; i++ ){
			tpsignal = strstr(++psignal, "signal");
			if(tpsignal){
				ap_count++;
				i = tpsignal - aplist;
				
//				aplist[i-2] = ';';
//				aplist[i-1] = '\n';
			}else{
				break;
			}			
		}
		if(aplist_len < APLIST_LEN){
			aplist[aplist_len] = ';';
		}else{
			aplist[APLIST_LEN - 1] = ';';
		}
	}
*/
//	printf("len: %d\naplist: %s\n", aplist_len, aplist);
	*pAplist=pst_ap_list;
	return ap_count;
}

//3.1.2	连接wifi		测试OK
int connect_wifi(char* ssid,char* psw){
	int ret=my_popen("elian.sh connect %s %s &",ssid,psw);
	if(!ret){
		err("elian.sh failure,ret=%d",ret);
		return -1;
	}
	return 0;
}

//3.1.16	断开wifi		测试OK
int disconnect_wifi(void){
	int ret=my_popen("elian.sh disconnect &");
	if(ret<0){
		err("elian.sh disconnect failure,ret=%d",ret);
		return -1;
	}
	return 0;
}

//3.4.1	修改设备ssid名称			测试OK
int set_device_ssid(char *ssid){
	int ret=cdb_set("$ap_ssid1",ssid);
	if(ret<0){
		err("cdb_set failure,ret=%d",ret);
		return -1;
	}
	ret=cdb_save();
	if(ret<0){
		return -2;
	}
	sleep(3);
	printf("set device ssid\n");
	reboot(RB_AUTOBOOT);

	return 0;	
}

//3.4.2	设置设备wifi密码			功能未实现
int set_device_pwd(char *pwd){
	int ret=cdb_set("$ap_passphase",pwd);
	if(ret<0){
		err("cdb_set failure,ret=%d",ret);
		return -1;
	}
	ret=cdb_save();
	if(ret<0){
		return -2;
	}
	ret=my_popen("mtc commit &");
	if(ret<0){
		err("elian.sh disconnect failure,ret=%d",ret);
		return -1;
	}
	return 0;		
}

/*查询当前连接的路由器*/
char * search_connect_router_ssid(void)
{
	char aplistinfo[128] = "";
	char * ptr = NULL;
	char ssid[32] = "";
	if (2 == cdb_get_int("$wanif_state",0)){
		cdb_get_str("$wl_aplist_info1",aplistinfo,sizeof(aplistinfo),"");
		if(strlen(aplistinfo) != 0){
			ptr = strstr(aplistinfo,"&ssid");
			strncpy(ssid, &ptr[6], 32);
		
			return ssid;
		}else{
			return NULL;
		}
	}else{
		return NULL;
	}

}



/*编码代码
* const unsigned char * sourcedata， 源数组
* char * base64 ，码字保存
*/
int base64_encode(const unsigned char * sourcedata, char * base64)
{
    int i=0, j=0;
    unsigned char trans_index=0;    // 索引是8位，但是高两位都为0
    const int datalength = strlen((const char*)sourcedata);
    for (; i < datalength; i += 3){
        // 每三个一组，进行编码
        // 要编码的数字的第一个
        trans_index = ((sourcedata[i] >> 2) & 0x3f);
        base64[j++] = base64char[(int)trans_index];
        // 第二个
        trans_index = ((sourcedata[i] << 4) & 0x30);
        if (i + 1 < datalength){
            trans_index |= ((sourcedata[i + 1] >> 4) & 0x0f);
            base64[j++] = base64char[(int)trans_index];
        }else{
            base64[j++] = base64char[(int)trans_index];

            base64[j++] = padding_char;

            base64[j++] = padding_char;

            break;   // 超出总长度，可以直接break
        }
        // 第三个
        trans_index = ((sourcedata[i + 1] << 2) & 0x3c);
        if (i + 2 < datalength){ // 有的话需要编码2个
            trans_index |= ((sourcedata[i + 2] >> 6) & 0x03);
            base64[j++] = base64char[(int)trans_index];

            trans_index = sourcedata[i + 2] & 0x3f;
            base64[j++] = base64char[(int)trans_index];
        }
        else{
            base64[j++] = base64char[(int)trans_index];

            base64[j++] = padding_char;

            break;
        }
    }

    base64[j] = '\0'; 

    return 0;
}

inline int num_strchr(const char *str, char c) // 
{
    const char *pindex = strchr(str, c);
    if (NULL == pindex){
        return -1;
    }
    return pindex - str;
}
/* 解码
* const char * base64 码字
* unsigned char * dedata， 解码恢复的数据
*/
int base64_decode(const char * base64, unsigned char * dedata)
{
    int i = 0, j=0;
    int trans[4] = {0,0,0,0};
    for (;base64[i]!='\0';i+=4){
        // 每四个一组，译码成三个字符
        trans[0] = num_strchr(base64char, base64[i]);
        trans[1] = num_strchr(base64char, base64[i+1]);
        // 1/3
        dedata[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1]>>4) & 0x03);

        if (base64[i+2] == '='){
            continue;
        }
        else{
            trans[2] = num_strchr(base64char, base64[i + 2]);
        }
        // 2/3
        dedata[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);

        if (base64[i + 3] == '='){
            continue;
        }
        else{
            trans[3] = num_strchr(base64char, base64[i + 3]);
        }

        // 3/3
        dedata[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
    }

    dedata[j] = '\0';

    return 0;
}


int get_device_info(DEVICEINFO *aplist )
{
	char listinfo[128] = "";
	char mode_name[32] = "";
	char out_p[128] = "";
	int i;

	cdb_get_str("$model_name",mode_name,sizeof(mode_name),"");	//DOSS1212_DS_1639
    base64_encode(mode_name, aplist->project_uuid);
	cdb_set("$project_uuid",aplist->project_uuid);
	printf("aplist->project_uuid:%s",aplist->project_uuid);
	int iLength = sizeof(cdb_cmd_tbl) / sizeof(cdb_list);
	for (i = 0; i < iLength; i++) {
		cdb_get_str(cdb_cmd_tbl[i].name,listinfo,sizeof(listinfo),"");
			//	printf("[i] = %d->listinfo:%s",i,listinfo);

		switch(i){
			case 0:
				strncpy(aplist->sys_language,listinfo,strlen(listinfo));
				break;
			case 1:
				strncpy(aplist->sup_language,listinfo,strlen(listinfo));
				break;				
			case 2:
				strncpy(aplist->firmware,listinfo,strlen(listinfo));
				break;				
			case 3:
				strncpy(aplist->release,listinfo,strlen(listinfo));
				break;				
			case 4:
				strncpy(aplist->mac_addr,listinfo,strlen(listinfo));
				break;				
			case 5:
				strncpy(aplist->uuid,listinfo,strlen(listinfo));
				break;					
			case 6:
				strncpy(aplist->project,listinfo,strlen(listinfo));
				break;					
			case 7:
				strncpy(aplist->function_support,listinfo,strlen(listinfo));
				break;					
			case 8:
				strncpy(aplist->battery_level,listinfo,strlen(listinfo));
				break;					
			case 9:
				strncpy(aplist->bt_ver,listinfo,strlen(listinfo));
				break;
			case 10:
				strncpy(aplist->key_num,listinfo,strlen(listinfo));
				break;
			case 11:
				strncpy(aplist->charge_plug,listinfo,strlen(listinfo));
				break;
			case 12:
				strncpy(aplist->audioname,listinfo,strlen(listinfo));
				break;	
			case 13:
				strncpy(aplist->project_uuid,listinfo,strlen(listinfo));
				break;
			default:
				break;

		}
		
	}
	return 0;
}


int set_language(int language)
{
	int ret = 0;
	switch(language)
	{
		case 0:
			//禁用语音
			break;
		case 1:
			//汉语
			ret = my_popen("setLanguage.sh cn");
			break;
		case 2:
			//英语
			ret = my_popen("setLanguage.sh en");
			break;
		case 3:
			//法语
			break;
		case 4:
			//西班牙语
			break;
		default:
			printf("not supported language\n");
			break;
	}
	return ret;
}

int get_wakeup_times(void)
{
	int times = 0;
	char *buf;
	if (fd_read_file(&buf,"/tmp/wakeloop") < 0)
	{
		return 0;
	}
	times = atoi(buf);
	free(buf);
	return times;		
}

int get_battery_level(void)
{
	return cdb_get_int("$battery_level", 0);
}

/*
0：未连接
1：连接中
2：连接成功
3：正在断开连接
*/
int get_wifi_ConnectStatus(void)
{
	return (char)cdb_get_int("$wanif_state", 0);
}

/*获取lan端ip*/
char* get_lan_ip(void)
{
	static char lan_ip[16] = {0};
	cdb_get_str("$lanif_ip",lan_ip,sizeof(lan_ip),"");
	if(strlen(lan_ip) != 0){
		return lan_ip;
	}
	else{
		return NULL;
	}
}

