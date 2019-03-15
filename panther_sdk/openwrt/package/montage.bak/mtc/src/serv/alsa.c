#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"
#include <json/json.h>

#define DEFAULT_VOL 80

#define get_vol_set(vol) ((vol*255/100)%256)

static int get_config_volume(char *pPrefix){
	int err=0;
//	cslog(pPrefix);
	if(!(pPrefix&&pPrefix[0])){
		return -1;
	}
	unsigned prefixSize=strlen(pPrefix);
	char *pName=calloc(1,prefixSize+sizeof("_enable"));
	if(!pName){
		return -2;
	}
	sprintf(pName,"%s_enable",pPrefix);
//	cslog(pName);
	char *pEn = ReadSystemConfigValueString(pName);
	if(!pEn) {
		err=-3;
		goto err0;
	}
//	cslog(pName);
	if('0'==pEn[0]){
		err=-4;
		goto err1;
	}
//	cslog(pName);
	pName=realloc(pName,prefixSize+sizeof("_value"));
	if(!pName){
		return -5;
	}
//	cslog(pName);
	memset(pName,0,prefixSize+sizeof("_value"));
	sprintf(pName,"%s_value",pPrefix);
//	cslog(pName);
	char *pVol= ReadSystemConfigValueString(pName);
	if(!pVol){
		err=-6;
		goto err1;
	}
//	cslog(pName);
	int vol=atoi(pVol)	;
	free(pVol);
	return vol;
err1:
	free(pEn);
err0:
	free(pName);
	return err;
}

static int get_duercfg(char *pName,int *pVol){
	char *json_str=NULL;
	int err=fp_read_file(&json_str,"/root/appresources/dueros_config.json","r");
	if(err<0){
		err=-1;
		goto Err1;
	}	
	struct json_object *obj=json_tokener_parse(json_str);
	if(is_error(obj)){
		err=-2;
		goto Err2;
	}
	char *attr=NULL;
	struct json_object *attr_obj=NULL;
	attr_obj=json_object_object_get(obj,pName);
	if(is_error(attr_obj)){
		err=-3;
		goto Err2;
	}
	attr=json_object_get_string(attr_obj);
	if(!attr){
		err=-4;
		goto Err2;

	}
	*pVol=atoi(attr);	
Err2:
	free(json_str);
Err1:	
	return err;	
}

static int start(void){
    exec_cmd("alsactl init external");
	int vol=DEFAULT_VOL;
	int err=get_duercfg("com_volume",&vol);
	if(err<0){
		cslog("err=%d",err);
		exec_cmd2("amixer set ALERTS %d > /dev/null",get_vol_set(DEFAULT_VOL));
	}else{
		exec_cmd2("amixer set ALERTS %d > /dev/null",get_vol_set(vol));
	}
	err=get_duercfg("alert_volume",&vol);
	if(err<0){
		cslog("err=%d",err);
		vol= cdb_get_int("$ra_vol", DEFAULT_VOL);
		exec_cmd2("amixer set COMMON %d > /dev/null",get_vol_set(vol));
	}else{
		exec_cmd2("amixer set COMMON %d > /dev/null",get_vol_set(vol));
		cdb_set_int("$ra_vol", vol);
		exec_cmd2("cdb save &");
	}

//  use aplay to play wav with a big buffer and 6 second start delay
//  aplay -Dplug:bigdmixer -R 6000000 /root/res/en/connected.wav
/*
    if(f_exists("/PowerOn.wav")) {
        MTC_LOGGER(this, LOG_INFO, "Playing PowerOn.wav");
        eval_nowait("wavplayer", "/PowerOn.wav");
    }
    else {
        MTC_LOGGER(this, LOG_INFO, "There is no PowerOn.wav");
    }
*/
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int alsa_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

