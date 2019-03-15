/*
 * =====================================================================================
 *
 *       Filename:  mtdm_proc.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/17/2016 03:13:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  nady 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _MTDM_PROC_H_
#define _MTDM_PROC_H_
#include "mtdm_types.h"

#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#define FUNCTION_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#define FUNCTION_IS_NOT_USED
#endif

typedef enum 
{
    ProcessBoot = 0,
    ProcessCommit,
    ProcessCommitAll,
    ProcessSave,
    ProcessShutdown,

    ProcessMax
}ProcessCode;

#define BUF_SIZE          512

#define WEB_STATE_DEFAULT 0
#define WEB_STATE_ONGOING 1
#define WEB_STATE_SUCCESS 2
#define WEB_STATE_FAILURE 3
#define WEB_STATE_FWSTART 4

#define OOM_FILE "/etc/config/oom_disable.lst"

#define BOOT     0
#define START    1
#define STOP     2
#define RESTART  (START+STOP)

#define OP_UNKNOWN        -1
#define OP_DB_MODE         0
#define OP_AP_MODE         1
#define OP_WIFIROUTER_MODE 2
#define OP_WISP_MODE       3
#define OP_BR1_MODE        4
#define OP_BR2_MODE        5
#define OP_SMART_MODE      6
#define OP_MB_MODE         7
#define OP_P2P_MODE        8
#define OP_STA_MODE        9
#define OP_SMARTCFG_MODE  99

typedef struct __montage_proc_t {
    const char *mname;
    const int  priority;
    int  boot;
    unsigned long flags;
    void (*exec_proc)(struct __montage_proc_t *proc, int act);
    const char *depsname;
    int selected;
} montage_proc_t;

typedef struct {
    const char *cmd;
    int (*cmd_handler)(void);
} montage_sub_proc_t;



#define PROC_DBG        0x01
#define PROC_ASYNC_EXEC 0x02

#define MTDM_LOGGER(this, level, fmt...) do { \
    if ((this) && (this->flags & PROC_DBG)) { \
        logger (level, fmt);                 \
    }                                        \
} while(0)

static montage_proc_t VARIABLE_IS_NOT_USED *this = NULL;

#define MTDM_INIT_SUB_PROC(proc) do {           \
    if (proc) {                                \
        this = proc;                           \
    }                                          \
} while(0)

#define MTDM_SUB_PROC(sub_proc, cmd) do {       \
    int i;                                     \
    for(i=0; sub_proc[i].cmd != NULL; i++) {   \
        if (strcmp(cmd, sub_proc[i].cmd) == 0) \
            sub_proc[i].cmd_handler();         \
    }                                          \
} while(0)

/* normal proc */
void alsa_proc(montage_proc_t *proc, int act);
void wdk_proc(montage_proc_t *proc, int act);   
void usb_proc(montage_proc_t *proc, int act);   
void lcd_proc(montage_proc_t *proc, int act);   
void boot_proc(montage_proc_t *proc, int act);  
void dbus_proc(montage_proc_t *proc, int act);  
void avahi_proc(montage_proc_t *proc, int act); 
void crond_proc(montage_proc_t *proc, int act); 
void cdb_proc(montage_proc_t *proc, int act);   
void telnet_proc(montage_proc_t *proc, int act);
void uartd_proc(montage_proc_t *proc, int act);
void postplaylist_proc(montage_proc_t *proc, int act);
void alexa_proc(montage_proc_t *proc, int act);
void duer_proc(montage_proc_t *proc, int act);

void ocfg_proc(montage_proc_t *proc, int act);  
void done_proc(montage_proc_t *proc, int act);  
void sysctl_proc(montage_proc_t *proc, int act);
void boot_proc(montage_proc_t *proc, int act);  
void umount_proc(montage_proc_t *proc, int act);

/* cdb dep proc */
void log_proc(montage_proc_t *proc, int act);
void op_proc(montage_proc_t *proc, int act);
void vlan_proc(montage_proc_t *proc, int act);
void lan_proc(montage_proc_t *proc, int act);
void lan6_proc(montage_proc_t *proc, int act);
void wl_proc(montage_proc_t *proc, int act);
void wan_proc(montage_proc_t *proc, int act);
void fw_proc(montage_proc_t *proc, int act);
void nat_proc(montage_proc_t *proc, int act);
void route_proc(montage_proc_t *proc, int act);
void ddns_proc(montage_proc_t *proc, int act);
void dlna_proc(montage_proc_t *proc, int act);
void http_proc(montage_proc_t *proc, int act);
void dns_proc(montage_proc_t *proc, int act);
void smb_proc(montage_proc_t *proc, int act);
void ftp_proc(montage_proc_t *proc, int act);
void prn_proc(montage_proc_t *proc, int act);
void qos_proc(montage_proc_t *proc, int act);
void sys_proc(montage_proc_t *proc, int act);
void antibb_proc(montage_proc_t *proc, int act);
void igmp_proc(montage_proc_t *proc, int act);
void lld2_proc(montage_proc_t *proc, int act);
void upnp_proc(montage_proc_t *proc, int act);
void ra_proc(montage_proc_t *proc, int act);
void omicfg_proc(montage_proc_t *proc, int act);

/* module proc */
int alsa_main(montage_proc_t *proc, char *cmd);
int avahi_main(montage_proc_t *proc, char *cmd);
int boot_main(montage_proc_t *proc, char *cmd);
int checkup_main(int cdbsave, int needreboot);
int crond_main(montage_proc_t *proc, char *cmd);
int dbus_main(montage_proc_t *proc, char *cmd);
int ddns_main(montage_proc_t *proc, char *cmd);  
int dlna_main(montage_proc_t *proc, char *cmd);
int uartd_main(montage_proc_t *proc, char *cmd);
int postplaylist_main(montage_proc_t *proc, char *cmd);
int alexa_main(montage_proc_t *proc, char *cmd);

int dnsmasq_main(montage_proc_t *proc, char *cmd);
int done_main(montage_proc_t *proc, char *cmd);
int ftp_main(montage_proc_t *proc, char *cmd);
int fw_main(montage_proc_t *proc, char *cmd);
int lan_main(montage_proc_t *proc, char *cmd);
int lcd_main(montage_proc_t *proc, char *cmd);
int lld2_main(montage_proc_t *proc, char *cmd);
int mpd_main(montage_proc_t *proc, char *cmd);
int mtdm_ocfg_arg(void);
int mtdm_wan_ip_down(void);
int mtdm_wan_ip_up(BootState boot, montage_proc_t *ptbl, int tblcnt);
int mtdm_sleep(int sec);
#ifdef SPEEDUP_WLAN_CONF
int mtdm_wlhup(void);
#endif
int nat_main(montage_proc_t *proc, char *cmd);
int ocfg_main(montage_proc_t *proc, char *cmd);
int radvd_main(montage_proc_t *proc, char *cmd);
int route_main(montage_proc_t *proc, char *cmd);
int shairport_main(montage_proc_t *proc, char *cmd);
int spotify_main(montage_proc_t *proc, char *cmd);
int sysctl_main(montage_proc_t *proc, char *cmd);
int telnet_main(montage_proc_t *proc, char *cmd);
int uhttpd_main(montage_proc_t *proc, char *cmd);   
int umount_main(montage_proc_t *proc, char *cmd);
int upnpd_main(montage_proc_t *proc, char *cmd);
int upmpdcli_main(montage_proc_t *proc, char *cmd);
int usb_main(montage_proc_t *proc, char *cmd);
int wan_main(montage_proc_t *proc, char *cmd);
int wdk_main(montage_proc_t *proc, char *cmd);
int wlan_main(montage_proc_t *proc, char *cmd);
int workmode_main(montage_proc_t *proc, char *cmd);


/* other proc */
void sendCmd(OpCodeCmd opc, OpCodeCmdArg arg, char *data);
void mtdm_proc_handle(ProcessCode type);
void mtdm_proc_init(void);
#endif
