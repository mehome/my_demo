#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

#define OCFG_NAME       "omnicfg_bc"
#define OCFG_CMDS       "/usr/bin/"OCFG_NAME
#define OCFG_PIDFILE    "/var/run/"OCFG_NAME".pid"

#define OCFG_ELEM_SIZE 31

struct ocfg_var_t {
    char args[1024];
    char mac[18];
    char ip_addr[OCFG_ELEM_SIZE + 1];
    char sw_vid[OCFG_ELEM_SIZE + 1];
    char sw_pid[OCFG_ELEM_SIZE + 1];
	char wan_ip[OCFG_ELEM_SIZE + 1];
	char audio_name[64];
	char extend[64];
	char uuid[64];
    long ver_time;
    int result;
};

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

extern MtdmData *mtdm;

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

static char* replace(char* p)  
{  
    char* ret = p;  
    int num = 0;  
    int oldlen = 0;  
    int newlen = 0;  
    char* q = p;  
    char* r;  
    assert(p != NULL);  
    while (*p != '\0')  
    {  
        if (*p == ' ')  
        {  
            num++;  
        }  
        oldlen++;  
        p++;  
    }  
    p = q;  
    newlen = oldlen + 2 * num;  
    q = p + oldlen - 1;  
    r = p + newlen - 1;  
    while (q != r)  
    {  
        if (*q == ' ')  
        {  
            *r-- = '0';  
            *r-- = '2';  
            *r-- = '%';  
        }  
        else  
        {  
            *r = *q;  
            r--;  
        }  
        q--;  
    }  
    return ret;  
}  

static struct ocfg_var_t * get_ocfg_parm(void)
{
    struct ocfg_var_t *var = calloc(1, sizeof(struct ocfg_var_t));
    unsigned char mac[6];

    if (var) {
        my_mac2val(mac, mtdm->boot.MAC2);
        my_val2mac(var->mac, mac);
        if((mtdm->rule.OMODE != 8) && (mtdm->rule.OMODE != 9)) {
            cdb_get_str("$lan_ip", var->ip_addr, OCFG_ELEM_SIZE, NULL);
        }
		cdb_get_str("$wanif_ip", var->wan_ip, OCFG_ELEM_SIZE, NULL);
		if(var->wan_ip==NULL || strlen(var->wan_ip)<7 || strcmp(var->wan_ip,"0.0.0.0")==0){
			memset(var->wan_ip,0,sizeof(var->wan_ip));
			memcpy(var->wan_ip,var->ip_addr,sizeof(var->ip_addr));
			}
		cdb_get_str("$ra_name", var->audio_name, 64, NULL);
        cdb_get_str("$sw_vid", var->sw_vid, OCFG_ELEM_SIZE, NULL);
        cdb_get_str("$sw_pid", var->sw_pid, OCFG_ELEM_SIZE, NULL);
        var->result = cdb_get_int("$omi_result", 0);
        var->ver_time = uptime();
		
		sprintf(var->uuid, "xzxaudio-wifi-1511-0023-%02x%02x%02x%02x%02x%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		cdb_set("$wifiaudio_uuid",var->uuid);
	}

    return var;
}
//#define OMNICFG_APP_SUPPORT_DYNAMIC_PS_MODE
#define POWERCTL_FLAG_WIFI_DYNAMIC        0x200000
static int stop(void)
{
#if 1
    killall(OCFG_NAME, SIGHUP);
#else
    int found = 0;

    if( f_exists(OCFG_PIDFILE) ) {
        int pid = read_pid_file(OCFG_PIDFILE);
        if (pid != UNKNOWN) {
            found = 1;
            kill(pid, SIGHUP);
        }
        unlink(OCFG_PIDFILE);
    }
    if (!found) {
        killall(OCFG_NAME, SIGHUP);
    }
#endif

    return 0;
}

int mtdm_ocfg_conf(int new_conf)
{
    struct ocfg_var_t *var = get_ocfg_parm();
    char *ptr;
	
#if defined(OMNICFG_APP_SUPPORT_DYNAMIC_PS_MODE)	
    char tmp[128] = {0};
    unsigned long powercfg;
    int wifi_ps_mode = 0;

    if(CDB_RET_SUCCESS==boot_cdb_get("powercfg", tmp))
    {
        powercfg = strtoul(tmp, NULL, 0);
        if(powercfg & POWERCTL_FLAG_WIFI_DYNAMIC)
            wifi_ps_mode = 1;		
    }
#endif
    char args[LSBUF_LEN] = "{\
\"ver\":\"1\",\
\"name\":\"OMNICFG@%s\",\
\"omi_result\":\"%d\",\
\"sw_vid\":\"%s\",\
\"sw_pid\":\"%s\",\
\"lan_ip\":\"%s\",\
\"version\":\"%lu\",\
\"new_conf\":\"%d\"\,\
\"wan_ip\":\"%s\"\,\
\"audio_name\":\"%s\"\,\
\"extend\":\"%s\"\,\
\"uuid\":\"%s\"\
} ";
	
    if (var) {
        ptr = var->args;
		
        ptr += sprintf(ptr, args, var->mac, var->result, var->sw_vid, var->sw_pid, 
                       var->ip_addr, var->ver_time, new_conf, var->wan_ip, replace(var->audio_name),replace(var->extend), var->uuid);
        ptr += sprintf(ptr, args, var->mac, var->result, var->sw_vid, var->sw_pid, 
                       var->ip_addr, var->ver_time, 0, var->wan_ip, replace(var->audio_name),replace(var->extend), var->uuid);
        ptr += sprintf(ptr, "60");
#if defined(OMNICFG_APP_SUPPORT_DYNAMIC_PS_MODE)
        /*
            omnicfg_bc do not broadcast system info. if ps mode is enabled
            if "-u" flag is added to command line, omnicfg_bc will reponse query using unicast
         */
        if(wifi_ps_mode)
            ptr += sprintf(ptr, " -u");
#endif		
	    stop();
		start_stop_daemon_args_with_space(OCFG_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, OCFG_PIDFILE, 
            "%s", var->args);
        free(var);
    }
//#endif

    return 0;
}

int mtdm_ocfg_arg(void)
{
    return mtdm_ocfg_conf(1);
}

static int start(void)
{
    return mtdm_ocfg_conf(0);
}




static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int ocfg_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

