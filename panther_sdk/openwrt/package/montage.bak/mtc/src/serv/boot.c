#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"
#include "include/mtdm_misc.h"
#include "misc.h"
#include <wdk/cdb.h>
#include <libcchip/platform.h>
#include <libcchip/function/wav_play/wav_play.h>

#define SuNing_Biu

#if defined SuNing_Biu
#define SSID_PREFIX "Biu_Speaker_"
#define FIRMWARE_PREFIX "Biu_Speaker_V1_"

#elif defined DuerOS
#define SSID_PREFIX "DuerOS_"
#define FIRMWARE_PREFIX "DuerOS_V1_"
#else
#define SSID_PREFIX "CCHIP_"
#define FIRMWARE_PREFIX "CCHIP_8198_"
#endif

int add_zero_str(char *src)
{  
    int i, j;  
  
    char data[20] = {0};  
    char dest[20] = {0};   
    strcpy(data, src);  
    for(i = 0, j = 0; data[i] != '\0'; i++){  
    	 
        if(data[i] == ':' && data[i+2] == ':'){  
        	dest[j++] = data[i];       
          dest[j++] = '0';  
        }else{
        	dest[j++] = data[i];  
        	}
    }  
    dest[j] = '\0';  
    strcpy(src, dest);  
    return 0;  
}  
#if 0
static void update_ssid_name()
{
	char *ssid = NULL;
	char *name = NULL;
	char *factory = NULL;
	char cmd[256] = {0};
	char ra_name[48] = {0};
	char ap_ssid[48] = {0};
	unsigned char val[6] = {0};
	char tmp[BUF_SIZE];
	char bySsidPrefixLength = 0;
	
	// by mars_hu
	char *pssid_prefix = ReadSystemConfigValueString("ssid_prefix");
	if (pssid_prefix != NULL) {
		bySsidPrefixLength = strlen(pssid_prefix);
		if (bySsidPrefixLength <= 48) {
			strcpy(ra_name, pssid_prefix);
		} else {
			bySsidPrefixLength = strlen(DEFAULT_SSID_PREFIX);
			strcpy(ra_name, DEFAULT_SSID_PREFIX);
		}
		free(pssid_prefix);
	} else {
		bySsidPrefixLength = strlen(DEFAULT_SSID_PREFIX);
		strcpy(ra_name, DEFAULT_SSID_PREFIX);
	}

	boot_cdb_get("mac2", tmp);
	output_console("tmp:%s\n", tmp);
	add_zero_str(tmp);
	output_console("add_zero_str tmp:%s\n", tmp);
	cdb_set("$mac_addr", tmp);
	sscanf(tmp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);

	sprintf(&ra_name[bySsidPrefixLength], "%x%2.2x%2.2x", (val[3] & 0x0f), val[4], val[5]);									    					        

	cdb_set("$ra_name", ra_name);
	cdb_set("$wl_bss_ssid1", ra_name);

	//output_console("ra_name:%s", ra_name);
	cdb_set("$audioname", ra_name);
	cdb_set("$ap_ssid1", ra_name);
	cdb_set("$spotify_display_name", ra_name);
}
#endif

// by mars_hu
static void update_firmware() {
	char release_time[64] = {0};
	if ((cdb_get_str("$sw_build", release_time, sizeof(release_time), NULL) != NULL)) {
		char byTmpBuffer[64] = {0};
		char byFirmeareLength = 0;
		char *pFirmware = ReadSystemConfigValueString("firmware");
		if (pFirmware != NULL) {
			byFirmeareLength = strlen(pFirmware);
			if (byFirmeareLength <= 64) {
				strcat(byTmpBuffer, pFirmware);
			} else {
				byFirmeareLength = strlen(FIRMWARE_PREFIX);
				strcat(byTmpBuffer, FIRMWARE_PREFIX);
			}
			free(pFirmware);
		} else {
			byFirmeareLength = strlen(FIRMWARE_PREFIX);
			strcat(byTmpBuffer, FIRMWARE_PREFIX);
		}

		char byTemp[10] = {0};
		char byTemp1[4] = {0};
		memcpy(byTemp, release_time, 10);
		memcpy(byTemp1, &release_time[10], 4);
		sprintf(&byTmpBuffer[byFirmeareLength], "%x.%x", atoi(byTemp), atoi(byTemp1));
		cdb_set("$firmware", byTmpBuffer);
	}
}

static void update_ssid_name()
{
	char *ssid = NULL;
	char *name = NULL;
	char ra_name[48] = {0};
	unsigned char val[6];
	char tmp[BUF_SIZE];
	char *ptr = NULL;
   strcpy(ra_name, SSID_PREFIX);
if ((ptr = strstr(ra_name, SSID_PREFIX))) {
       //ptr =  ptr+strlen("DOSS E-GO_");
							        boot_cdb_get("mac2", tmp);
							         output_console("tmp:%s\n", tmp);
							         cdb_set("$mac_addr", tmp);
							         sscanf(tmp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
							        //sprintf(++ptr, "%2.2x%2.2x%2.2x", val[3], val[4], val[5]);
//									char byMacAddrOne = 0;
//									byMacAddrOne = val[3] & 0x0f;
							        sprintf(&ra_name[strlen(SSID_PREFIX)], "%2.2x%2.2x", val[4], val[5]);									    					        
							       // output_console("ra_name:%s\n", ra_name);
							        cdb_set("$ra_name", ra_name);
							        cdb_set("$wl_bss_ssid1", ra_name);
//				              exec_cmd("cdb commit");
						}
						//output_console("ra_name:%s", ra_name);
							cdb_set("$audioname", ra_name);
	            cdb_set("$ap_ssid1", ra_name);
	            cdb_set("$spotify_display_name", ra_name);
							

}

int file_copy(char *src, char *dst)
{
    FILE *fp1, *fp2;
    int c;
    fp1 = fopen(src, "rb");
    fp2 = fopen(dst, "wb");//¡ä¨°?a???t
    if(fp1 == NULL || fp2 == NULL)//¡ä¨ª?¨®¡ä|¨¤¨ª
    {
        printf("open file failed\n");
        if(fp1) fclose(fp1);
        if(fp2) fclose(fp2);
    }
     
    while((c = fgetc(fp1)) != EOF)//?¨¢¡Á??¨²
        fputc(c, fp2); //D¡ä¨ºy?Y
    fclose(fp1);//1?¡À????t
    fclose(fp2);
     
    return 0;
}

static int IS_File_Null(char *str)  
{  
    int ret = -1;  
    FILE *fp;  
    char ch;  
      
    ret = (str != NULL);       
    int len =0;      
    if(ret)  
    {  
          
        if((fp=fopen(str,"r"))==NULL)  
        {  
            ret = -1;  
              
            return ret;  
        }  
          
      //  ch=fgetc(fp);  
      
      fseek(fp,0L,SEEK_END); 
			len=ftell(fp); 
        output_console("len ==========%d\n",len);   
       /* 
        if(ch == EOF) //EOF = -1, ¨°2?¨ª¨º?0xff;  
            ret = 0;  
  
        else  
            ret = 1;  
            */
    }       
    fclose(fp);  
    return len;   
}  


void set_usb_tf_status(void)
{
    output_console("set usb tf status\n");
	cdb_set_int("$tf_status", 0);
	cdb_set_int("$usb_status", 0);
	 /*  
	     hotplug ä¼šå¯åŠ¨ä¸¤æ¬¡ ï¼Œ
	      çŽ°åœ¨hotplogä¼šå±è”½å¤šæ¬¡æ‰§è¡ŒåŒä¸€USBï¼Œ
	      è¿™é‡Œåœ¨ç¬¬ä¸€æ¬¡ä¹‹å‰
       */
}

static int start(void)
{
    
    FILE *fp = NULL;
    char cmd[512];
    char root[128] = { 0 };
    system("setLanguage.sh init");
 // 	 system("wavplayer /tmp/res/starting_up.wav");
//   eval_nowait("setLanguage.sh init");
//   eval_nowait("wavplayer /tmp/res/starting_up.wav");

    if(f_exists("/proc/jffs2_bbc"))
      f_write_string("/proc/jffs2_bbc", "\"S\"\n" , 0, 0);

    if(f_exists("/proc/net/vlan/config"))
        exec_cmd("vconfig set_name_type DEV_PLUS_VID_NO_PAD");

	system("mkdir -p /tmp/misc;umount /www/misc/ 2>/dev/null;mount /tmp/misc /www/misc");

    mkdir("/var", 0755);
    mkdir("/var/log", 0755);
    mkdir("/var/lock", 0755);
    mkdir("/var/state", 0755);
    mkdir("/var/.uci", 0700);
    f_write_string("/var/log/wtmp", "", 0, 0);
    f_write_string("/var/log/lastlog", "", 0, 0);
    f_write_string("/tmp/resolv.conf.auto", "", 0, 0);
    symlink("/tmp/resolv.conf.auto", "/tmp/resolv.conf");

	set_usb_tf_status();
	cdb_set_int("$wifi_bt_update_flage", 0);

	update_ssid_name();
	update_firmware();
    if(f_exists("/lib/wdk/system")) {
        system_update_hosts();
        exec_cmd("/lib/wdk/system timezone");
    }
    fp = fopen("/proc/cmdline", "r");
    if (fp) {
        while (new_fgets(cmd, sizeof(cmd), fp) != NULL) {
            if (sscanf(cmd, "root=%s ", root) != -1) {
                symlink(root, "/dev/root");
                break;
            }
        }
        fclose(fp);
    }
//    exec_cmd("setLanguage.sh init");	
    if(f_exists("/usr/sbin/reset_btn")) {
        killall("reset_btn", SIGTERM);
        exec_cmd("/usr/sbin/reset_btn");
    }

	cdb_set_int("$network_caller", 0);
    cdb_set_int("$iot_nocheck", 1);
    cdb_set_int("$mpd_nocheck", 0);
    cdb_set_int("$battery_level", 10);
    cdb_set_int("$battery_charge",1);
    
    
	if(f_exists("/root/resources/runtime.json")){
		uartd_fifo_write("duerSceneLedSet007");
		//exec_cmd("myPlay /root/appresources/startup.mp3");
	}
	wav_play_preload("/tmp/res/starting_up.wav");
    
//	cslog("----------------------------");
//	system("echo ["__FILE__":"STR(__LINE__)"]:"__TIME__" done---! >> /dev/console");
    MTDM_LOGGER(this, LOG_INFO, "boot done");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int boot_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

