#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define UPMPDCLI_BIN "/usr/bin/upmpdcli"
#define UPMPDCLI_NAME "upmpdcli"
#define BR0_PID_FILE "/var/run/upmpdcli.br0"
#define BR1_PID_FILE "/var/run/upmpdcli.br1"


int chk_wanip(char *name)
{
    int fd;
    struct ifreq ifr;
	
	if(!name)
		return 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof ifr);
    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr == 0) {
        return 0;
    }
    else {
        return 1;
    }
}


static int upmpdcli_start(void)
{
    char ra_name[BUF_SIZE];
    unsigned char val[6];
    char tmp[BUF_SIZE];
    char *ptr = NULL;
    int  ra_func = cdb_get_int("$ra_func", 0);
    
    if ((ra_func != raFunc_DLNA_AIRPLAY) && (ra_func != raFunc_ALL_ENABLE)) {
        MTDM_LOGGER(this, LOG_INFO, "upmpdcli: ignore -- ra_func=[%d]", ra_func);
        return 0;
    }
    
    cdb_get_str("$ra_name", ra_name, sizeof(ra_name), "");
    
    if ((ptr = strstr(ra_name, "_%6x"))) {
        boot_cdb_get("mac0", tmp);
        sscanf(tmp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
        sprintf(++ptr, "%2.2x%2.2x%2.2x", val[3], val[4], val[5]);
        cdb_set("$ra_name", ra_name);
    }
    if (strlen(ra_name) <= 0) {
        strcpy(ra_name, "AirPort");
    }
    
    if (f_exists(UPMPDCLI_BIN)) {
        if ((get_pid_by_name(UPMPDCLI_NAME) > 0)) {
            killall(UPMPDCLI_NAME, SIGKILL);
        }
/*
        if ((mtdm->rule.OMODE != opMode_client) && (mtdm->rule.OMODE != opMode_rp) && (mtdm->rule.OMODE != opMode_br)) {
            start_stop_daemon(UPMPDCLI_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                "-D -h 127.0.0.1 -f %s -i br0", ra_name);
						cdb_set("$dlnar_br0_nocheck", "0");
				}
*/
		
		   if (chk_wanip(mtdm->rule.WANB)) {
				/*
		            start_stop_daemon(UPMPDCLI_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
		                "-D -h 127.0.0.1 -f \"%s\" -i \"%s\"", ra_name, mtdm->rule.WANB);
						*/
					exec_cmd2(UPMPDCLI_BIN " -h 127.0.0.1 -f \"%s\" -i %s -D", ra_name, mtdm->rule.WANB);
					if(!strcmp(mtdm->rule.WANB, "br0"))
						cdb_set("$dlnar_br0_nocheck", "0");
					else
						cdb_set("$dlnar_br1_nocheck", "0");
				}
				else  if ((mtdm->rule.OMODE != opMode_client) && (mtdm->rule.OMODE != opMode_rp) && (mtdm->rule.OMODE != opMode_br)) {
				    /*
					start_stop_daemon(UPMPDCLI_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
		                "-D -h 127.0.0.1 -f \"%s\" -i br0", ra_name);				
						*/
					exec_cmd2(UPMPDCLI_BIN " -h 127.0.0.1 -f \"%s\" -i br0 -D", ra_name);
					cdb_set("$dlnar_br0_nocheck", "0");
				}
		}
		
    return 0;
}

static int upmpdcli_stop(void)
{
    killall("upmpdcli", SIGKILL);
    if( f_exists(BR0_PID_FILE) ) {
        start_stop_daemon(UPMPDCLI_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, BR0_PID_FILE, NULL);
        unlink(BR0_PID_FILE);
        cdb_set("$dlnar_br0_nocheck", "1");
    }
    if( f_exists(BR1_PID_FILE) ) {
        start_stop_daemon(UPMPDCLI_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, BR1_PID_FILE, NULL);
        unlink(BR1_PID_FILE);
        cdb_set("$dlnar_br1_nocheck", "1");
    }
    return 0;
}

static int start(void)
{
    upmpdcli_stop();
    return upmpdcli_start();
}

static int stop(void)
{
    return upmpdcli_stop();
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int upmpdcli_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

