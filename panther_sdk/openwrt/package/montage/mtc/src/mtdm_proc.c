/*
 * =====================================================================================
 *
 *       Filename:  mtdm_proc.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/17/2016 03:12:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  nady 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/
typedef struct
{
    struct list_head list;
    montage_proc_t *proc;
}procListNode;

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/
LIST_HEAD(bootProc);
LIST_HEAD(shutdownProc);
LIST_HEAD(commitProc);

static montage_proc_t boot_proc_tbl[] = {
    { "alsa",   1,1,0,alsa_proc,  0                           },
    { "wdk",    5,1,0,wdk_proc,   0                           },
#ifdef CONFIG_PACKAGE_st7565p_lcd
    { "lcd",    7,1,0,lcd_proc,   0                           },
#endif
	{ "uartd",  8,1,0,uartd_proc, 0 						  },
    { "boot",  10,1,0,boot_proc,  0                           },
#ifdef CONFIG_PACKAGE_libdbus
    { "dbus",  11,1,0,dbus_proc,  0                           },
#endif
#ifdef CONFIG_PACKAGE_libavahi
    { "avahi", 12,1,0,avahi_proc, 0                           },
#endif
    { "usb",   13,1,0,usb_proc,   0                           },
    { "cdb",   14,1,0,cdb_proc,   0                           },
    { "crond", 15,1,0,crond_proc, 0                           },
    { "telnet",50,1,0,telnet_proc,0                           },
//    { "uartd", 52,1,0,uartd_proc, 0                           },
#ifdef CONFIG_PACKAGE_alexa
    { "alexa", 53,1,0,alexa_proc, 0                           },
#endif
#ifdef CONFIG_PACKAGE_duer_linux
	{ "duer", 54,1,0,duer_proc, 0 					 		  },
#endif
    { "postplaylist", 92,1,0,postplaylist_proc,0              },
    { "ocfg",  94,1,0,ocfg_proc,  0                           },
    { "done",  95,1,0,done_proc,  0                           },
    { "sysctl",99,1,0,sysctl_proc,0                           },
};
static montage_proc_t shutdown_proc_tbl[] = {
    { "boot",  98,0,0,boot_proc,  0                           },
    { "umount",99,0,0,umount_proc,0                           },
};
static montage_proc_t cdb_proc_tbl[] = {
    { "log",   10,1,0,log_proc,   0                           },
    { "op",    10,1,0,op_proc,    0                           },
    { "vlan",  40,1,0,vlan_proc,  0                           },
    { "lan",   41,1,0,lan_proc,   "op"                        },
#ifdef CONFIG_IPV6
    { "lan6",  41,1,0,lan6_proc,  "op"                        },
#endif
    { "wl",    42,1,0,wl_proc,    "op"                        },
    { "wan",   43,0,0,wan_proc,   "poe pptp l2tp dns vlan"    },
#ifdef CONFIG_FW
    { "fw",    45,1,0,fw_proc,    "wan"                       },
#endif
    { "nat",   46,1,0,nat_proc,   "wan vlan igmp"             },
    { "route", 49,1,0,route_proc, "lan wan"                   },
    { "ddns",  50,1,0,ddns_proc,  "wan"                       },
    { "dlna",  50,1,0,dlna_proc,  0                           },
    { "http",  50,1,0,http_proc,  "op lan"                    },
    { "dns",   60,1,0,dns_proc,   "lan dhcps"                 },
#ifdef CONFIG_PACKAGE_samba3
    { "smb",   62,1,0,smb_proc,   0                           },
#endif
    { "ftp",   63,1,0,ftp_proc,   0                           },
    { "prn",   63,1,0,prn_proc,   0                           },
#ifdef CONFIG_QOS
    { "qos",   70,1,0,qos_proc,   "wan vlan"                  },
#endif
    { "sys",   75,1,0,sys_proc,   0                           },
    { "upnp",  78,1,0,upnp_proc,  "wan"                       },
    { "ra",    79,1,0,ra_proc,    "op lan"                    },
    { "igmp",  80,1,0,igmp_proc,  "wan vlan"                  },
    { "antibb",81,1,0,antibb_proc,"vlan"                      },
    { "lld2",  82,1,0,lld2_proc,  "lan"                       },
    { "omicfg",97,0,0,omicfg_proc,0                           }
};

static montage_proc_t *proc_tbl[] = {
    boot_proc_tbl,
    shutdown_proc_tbl,
    cdb_proc_tbl,
    NULL,
};
static int proc_tbl_size[] = {
    sizeof(boot_proc_tbl) / sizeof(montage_proc_t),
    sizeof(shutdown_proc_tbl) / sizeof(montage_proc_t),
    sizeof(cdb_proc_tbl) / sizeof(montage_proc_t),
    0,
};

static const char *opc_cmd_name[] = {
    [OPC_CMD_IDX(OPC_CMD_ALSA)]       = "alsa",
    [OPC_CMD_IDX(OPC_CMD_WDK)]        = "wdk",
    [OPC_CMD_IDX(OPC_CMD_USB)]        = "usb",
    [OPC_CMD_IDX(OPC_CMD_LCD)]        = "lcd",
    [OPC_CMD_IDX(OPC_CMD_BOOT)]       = "boot",
#ifdef CONFIG_PACKAGE_libdbus
    [OPC_CMD_IDX(OPC_CMD_DBUS)]       = "dbus",
#endif
    [OPC_CMD_IDX(OPC_CMD_AVAHI)]      = "avahi",
    [OPC_CMD_IDX(OPC_CMD_CROND)]      = "crond",
    [OPC_CMD_IDX(OPC_CMD_CDB)]        = "cdb",
    [OPC_CMD_IDX(OPC_CMD_TELNET)]     = "telnet",
    [OPC_CMD_IDX(OPC_CMD_OCFG)]       = "ocfg",
    [OPC_CMD_IDX(OPC_CMD_DONE)]       = "done",
    [OPC_CMD_IDX(OPC_CMD_SYSCTL)]     = "sysctl",
    [OPC_CMD_IDX(OPC_CMD_UMOUNT)]     = "umount",
    [OPC_CMD_IDX(OPC_CMD_LOG)]        = "log",
    [OPC_CMD_IDX(OPC_CMD_OP)]         = "op",
    [OPC_CMD_IDX(OPC_CMD_VLAN)]       = "vlan",
    [OPC_CMD_IDX(OPC_CMD_LAN)]        = "lan",
    [OPC_CMD_IDX(OPC_CMD_LAN6)]       = "lan6",
    [OPC_CMD_IDX(OPC_CMD_WL)]         = "wl",
    [OPC_CMD_IDX(OPC_CMD_WAN)]        = "wan",
    [OPC_CMD_IDX(OPC_CMD_FW)]         = "fw",
    [OPC_CMD_IDX(OPC_CMD_NAT)]        = "nat",
    [OPC_CMD_IDX(OPC_CMD_ROUTE)]      = "route",
    [OPC_CMD_IDX(OPC_CMD_DDNS)]       = "ddns",
    [OPC_CMD_IDX(OPC_CMD_DLNA)]       = "dlna",
    [OPC_CMD_IDX(OPC_CMD_HTTP)]       = "http",
    [OPC_CMD_IDX(OPC_CMD_DNS)]        = "dns",
    [OPC_CMD_IDX(OPC_CMD_SMB)]        = "smb",
    [OPC_CMD_IDX(OPC_CMD_FTP)]        = "ftp",
    [OPC_CMD_IDX(OPC_CMD_PRN)]        = "prn",
    [OPC_CMD_IDX(OPC_CMD_QOS)]        = "qos",
    [OPC_CMD_IDX(OPC_CMD_SYS)]        = "sys",
    [OPC_CMD_IDX(OPC_CMD_ANTIBB)]     = "antibb",
    [OPC_CMD_IDX(OPC_CMD_IGMP)]       = "igmp",
    [OPC_CMD_IDX(OPC_CMD_LLD2)]       = "lld2",
    [OPC_CMD_IDX(OPC_CMD_UPNP)]       = "upnp",
    [OPC_CMD_IDX(OPC_CMD_RA)]         = "ra",
    [OPC_CMD_IDX(OPC_CMD_OMICFG)]     = "omicfg",
    [OPC_CMD_IDX(OPC_CMD_COMMIT)]     = "commit",
    [OPC_CMD_IDX(OPC_CMD_SAVE)]       = "save",
    [OPC_CMD_IDX(OPC_CMD_SHUTDOWN)]   = "shutdown",
    [OPC_CMD_IDX(OPC_CMD_WANIPUP)]    = "wanipup",
    [OPC_CMD_IDX(OPC_CMD_WANIPDOWN)]  = "wanipdown",
    [OPC_CMD_IDX(OPC_CMD_OCFGARG)]    = "ocfgarg",
    [OPC_CMD_IDX(OPC_CMD_SLEEP)]      = "sleep",
#ifdef SPEEDUP_WLAN_CONF
    [OPC_CMD_IDX(OPC_CMD_WLHUP)]      = "wlhup",
#endif
    [OPC_CMD_IDX(OPC_CMD_MAX)]        = NULL
};

static const char *opc_cmd_arg_name[] = {
    [OPC_CMD_ARG_UNKNOWN]             = "unknown",
    [OPC_CMD_ARG_BOOT]                = "boot",
    [OPC_CMD_ARG_START]               = "start",
    [OPC_CMD_ARG_STOP]                = "stop",
    [OPC_CMD_ARG_RESTART]             = "restart",
    [OPC_CMD_ARG_DBG]                 = "dbg",
    [OPC_CMD_ARG_UNDBG]               = "undbg",
    [OPC_CMD_ARG_MAX]                 = "unknown",
};

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/
void mtdm_proc_handle(ProcessCode type);

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/
extern MtdmData *mtdm;
extern const char *applet_name;

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/
static void mtc_delay_commit(int sData)
{
    if (mtdm->bootState != bootState_done) {
        TimerMod(timerMtdmDelayCommit, 1);
    }
    else {
        exec_cmd("mtc commit");
    }
}

void mtc_delay_commit_timer_enable(void)
{
    TimerDel(timerMtdmDelayCommit);
    TimerAdd(timerMtdmDelayCommit, mtc_delay_commit, 0, 1);
}

void cdb_select_module(const char *cdb)
{
    montage_proc_t *ptbl = cdb_proc_tbl;
    int tblcnt = sizeof(cdb_proc_tbl) / sizeof(montage_proc_t);
    for (--tblcnt;tblcnt >= 0; tblcnt--) {
        if (cdb && !strcmp(ptbl[tblcnt].mname, cdb) && (ptbl[tblcnt].selected == 0)) {
            ptbl[tblcnt].selected = 1;
            logger (LOG_INFO, "%s [%s]", __func__, cdb);
            break;
        }
    }
}

void cdb_select_all_modules(int selected)
{
    montage_proc_t *ptbl = cdb_proc_tbl;
    int tblcnt = sizeof(cdb_proc_tbl) / sizeof(montage_proc_t);
    selected = (selected) ? 1 : 0;
    for (--tblcnt;tblcnt >= 0; tblcnt--) {
        ptbl[tblcnt].selected = selected;
    }
}

void cdb_find_dep_cdb(char *cdbs)
{
    montage_proc_t *ptbl = cdb_proc_tbl;
    int tblcnt = sizeof(cdb_proc_tbl) / sizeof(montage_proc_t);
    char *ptr;
    int i;

    if (cdbs == 0) return;
    while ( *cdbs == ' ' ) cdbs++;
    if (*cdbs == 0) return;

    while ( cdbs && (*cdbs != '\0') ) {
        if (*cdbs == ' ') {
            cdbs++;
            continue;
        }
        ptr = strchr(cdbs, ' ');
        if (ptr) *ptr++ = '\0';
        for (i = 0; i < tblcnt; i++) {
            if (!strcmp(ptbl[i].mname, cdbs) || (ptbl[i].depsname && strstr(ptbl[i].depsname, cdbs))) {
                cdb_select_module(ptbl[i].mname);
                if (strcmp(ptbl[i].mname, cdbs)) {
                    cdb_find_dep_cdb((char *)ptbl[i].mname);
                }
            }
        }
        cdbs = ptr;
    }
}

int handle_cdb(struct list_head *lh)
{
    struct list_head *pos, *n;
    char cdbs[CDB_DATA_MAX_LENGTH];

    checkup_main(0, 0);
    memset(cdbs, 0, CDB_DATA_MAX_LENGTH);

    cdb_service_commit_change(cdbs);

    cdb_select_all_modules(0);
    cdb_find_dep_cdb(cdbs);

    list_for_each_safe(pos, n, lh) {
        procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
        if (!node->proc->selected) {
            list_del(&node->list);
            free(node);
        }
    }
    if (!list_empty(lh)) {
        cdb_save();
    }

    return 0;
}

int handle_cdb_save(void)
{
    checkup_main(1, 0);
    cdb_save();
    return 0;
}

int handle_oom(void)
{
    char cmd[BUF_SIZE];
    char buf[BUF_SIZE];
    int  oom_fd;

    if (f_exists(OOM_FILE)) {
        oom_fd = open(OOM_FILE, O_RDWR|O_SYNC);
        if (oom_fd < 0)
            return -1;
        while ( safe_read_line(oom_fd, buf, sizeof(buf) ) > 0 ) {
            if (buf[0] != '\0') {
                sprintf(cmd, "echo -17 > /proc/%d/oom_adj", get_pid_by_name(buf));
                exec_cmd(cmd);
            }
        }
        close(oom_fd);
    }
    return 0;
}

void trace_arg(char *file,char *func,int line){
	char buf[1024]="";
	cdb_get_str("$lan_ip",buf,sizeof(buf),"");
	cslog("[%s:%s:%d]:%s",file,func,line,buf);
}

/************* normal *************/
void alsa_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        alsa_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}
void wdk_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        wdk_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}
void usb_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        usb_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}
void lcd_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        lcd_main(proc, "boot");
    }
    else if (act == STOP) {
        lcd_main(proc, "stop");
    }
}
void boot_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        boot_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}
void dbus_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_dbus)
    if ((act == BOOT) || (act == START)) {
        dbus_main(proc, "boot");
    }
    else if (act == STOP) {
        ;
    }
#endif
}
void avahi_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_avahi_daemon)
    if ((act == BOOT) || (act == START)) {
        avahi_main(proc, "boot");
    }
    else if (act == STOP) {
        ;
    }
#endif
}
void crond_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        crond_main(proc, "boot");
    }
    else if (act == STOP) {
        ;
    }
}
void cdb_proc(montage_proc_t *proc, int act)
{
    if (act == BOOT) {
        mtdm_proc_handle(ProcessCommitAll);
    }
}
void telnet_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        telnet_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}

void uartd_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        uartd_main(proc, "boot");
    }
    else if (act == STOP){
		uartd_main(proc, "stop");
	}

}

void postplaylist_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        postplaylist_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}

void alexa_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        alexa_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}

void duer_proc(montage_proc_t *proc, int act){
    if ((act == BOOT) || (act == START)) {
        duer_main(proc, "boot");
    }
    else if (act == STOP) {
        duer_main(proc, "stop");
    }
}

void ocfg_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        ocfg_main(proc, "boot");
    }
    else if (act == STOP) {
        ocfg_main(proc, "stop");
    }
}
void done_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        done_main(proc, "boot");
    }
    else if (act == STOP)
        ;
}
void sysctl_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        sysctl_main(proc, "boot");
        handle_oom();
    }
    else if (act == STOP)
        ;
}
void umount_proc(montage_proc_t *proc, int act)
{
    if ((act == BOOT) || (act == START)) {
        ;
    }
    else if (act == STOP)
        umount_main(proc, "stop");
}

/************* cdb dep *************/
void log_proc(montage_proc_t *proc, int act)
{
    if (cdb_get_int("$log_rm_enable", 0) == 0) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/lib/wdk/logconf"))
            exec_cmd("/lib/wdk/logconf");
    }
    else if (act == STOP)
        ;
}
void op_proc(montage_proc_t *proc, int act)
{
    if (act == BOOT) {
        workmode_main(proc, "boot");
    }
    else if (act == START) {
        workmode_main(proc, "start");
    }
    else if (act == STOP) {
        workmode_main(proc, "stop");
    }
}
void vlan_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/etc/init.d/vlan"))
            exec_cmd("/etc/init.d/vlan start");
    }
    else if (act == STOP) {
        if (f_exists("/etc/init.d/vlan"))
            exec_cmd("/etc/init.d/vlan stop");
    }
}
void lan_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        lan_main(proc, "start");
    }
    else if (act == STOP) {
        lan_main(proc, "stop");
    }
}
void lan6_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_radvd)
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        radvd_main(proc, "start");
    }
    else if (act == STOP) {
        radvd_main(proc, "stop");
    }
#endif
}
void wl_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        wlan_main(proc, "start");
    }
    else if (act == STOP) {
        wlan_main(proc, "stop");
    }
}
void wan_proc(montage_proc_t *proc, int act)
{
    //return;
    if (act == BOOT) {
        wan_main(proc, "boot");
    }
    else if (act == START) {
        wan_main(proc, "start");
    }
    else if (act == STOP) {
        wan_main(proc, "stop");
    }
}
void fw_proc(montage_proc_t *proc, int act)
{
#ifdef CONFIG_FW
    if (mtdm->rule.OMODE == OP_STA_MODE) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        fw_main(proc, "start");
    }
    else if (act == STOP) {
        fw_main(proc, "stop");
    }
#endif
}
void nat_proc(montage_proc_t *proc, int act)
{
    if (mtdm->rule.OMODE == OP_STA_MODE) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        nat_main(proc, "start");
    }
    else if (act == STOP) {
        nat_main(proc, "stop");
    }
}
void route_proc(montage_proc_t *proc, int act)
{
    if (mtdm->rule.OMODE == OP_STA_MODE) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        route_main(proc, "start");
    }
    else if (act == STOP) {
        route_main(proc, "stop");
    }
}
void ddns_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_inadyn) || defined(CONFIG_PACKAGE_ez_ipupdate)
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        ddns_main(proc, "start");
    }
    else if (act == STOP) {
        ddns_main(proc, "stop");
    }
#endif
}
void dlna_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_minidlna)
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        dlna_main(proc, "start");
    }
    else if (act == STOP) {
        dlna_main(proc, "stop");
    }
#endif
}
void http_proc(montage_proc_t *proc, int act)
{
#if defined(CONFIG_PACKAGE_uhttpd)
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        uhttpd_main(proc, "start");
    }
    else if (act == STOP) {
        uhttpd_main(proc, "stop");
    }
#endif
}
void dns_proc(montage_proc_t *proc, int act)
{
#ifndef SPEEDUP_WLAN_CONF
    if (mtdm->rule.OMODE == OP_STA_MODE) {
        return;
    }
#endif
#ifdef CONFIG_PACKAGE_dnsmasq
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        dnsmasq_main(proc, "start");
    }
    else if (act == STOP) {
        dnsmasq_main(proc, "stop");
    }
#endif
}
void smb_proc(montage_proc_t *proc, int act)
{
#ifdef CONFIG_PACKAGE_samba3
    if (cdb_get_int("$smb_enable", 0) == 0) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/lib/wdk/smb"))
            exec_cmd("/lib/wdk/smb run");
    }
    else if (act == STOP)
        ;
#endif
}
void ftp_proc(montage_proc_t *proc, int act)
{
#ifdef CONFIG_PACKAGE_stupid_ftpd
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        ftp_main(proc, "start");
    }
    else if (act == STOP) {
        ftp_main(proc, "stop");
    }
#endif
}
void prn_proc(montage_proc_t *proc, int act)
{
    if (cdb_get_int("$prn_enable", 0) == 0) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/lib/wdk/prn"))
            exec_cmd("/lib/wdk/prn run");
    }
    else if (act == STOP)
        ;
}
void qos_proc(montage_proc_t *proc, int act)
{
#if 1
    // currently, we don't support qos
    return;
#else
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/etc/init.d/qos"))
            exec_cmd("/etc/init.d/qos start");
    }
    else if (act == STOP) {
        if (f_exists("/etc/init.d/qos"))
            exec_cmd("/etc/init.d/qos stop");
    }
#endif
}
void sys_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/lib/wdk/system"))
            exec_cmd("/lib/wdk/system");
        if (f_exists("/lib/wdk/mangment"))
            exec_cmd("/lib/wdk/mangment");
    }
    else if (act == STOP)
        ;
}
void antibb_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/etc/init.d/bstorm"))
            exec_cmd("/etc/init.d/bstorm start");
    }
    else if (act == STOP) {
        if (f_exists("/etc/init.d/bstorm"))
            exec_cmd("/etc/init.d/bstorm stop");
    }
}
void igmp_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/etc/init.d/igmpproxy"))
            exec_cmd("/etc/init.d/igmpproxy start");
    }
    else if (act == STOP) {
        if (f_exists("/etc/init.d/igmpproxy"))
            exec_cmd("/etc/init.d/igmpproxy stop");
    }
}
void lld2_proc(montage_proc_t *proc, int act)
{
#ifdef CONFIG_PACKAGE_lld2
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        lld2_main(proc, "start");
    }
    else if (act == STOP) {
        lld2_main(proc, "stop");
    }
#endif
}
void upnp_proc(montage_proc_t *proc, int act)
{
#ifdef CONFIG_PACKAGE_miniupnpd
    if ((mtdm->rule.OMODE != OP_WIFIROUTER_MODE) && (mtdm->rule.OMODE != OP_WISP_MODE)) {
        return;
    }
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        upnpd_main(proc, "start");
    }
    else if (act == STOP) {
        upnpd_main(proc, "stop");
    }
#endif
}
void ra_proc(montage_proc_t *proc, int act)
{
    int ra_func = cdb_get_int("$ra_func", 0);
    act = ((act == BOOT) ? START : act);
    if (act == START) {
#ifdef CONFIG_PACKAGE_mpd_mini		
        if ((ra_func != raFunc_ALL_DISABLE) && (get_pid_by_name("mpdaemon") < 0))
            mpd_main(proc, "start");
#endif		
#if defined(CONFIG_PACKAGE_shairport)||defined(CONFIG_PACKAGE_shairport_sync_openssl)
        shairport_main(proc, "start");
#endif
#ifdef CONFIG_PACKAGE_upmpdcli
		upmpdcli_main(proc, "start");
#endif
#ifdef CONFIG_PACKAGE_spotify
        spotify_main(proc, "start");
#endif
    }
    else if (act == STOP) {
#ifdef CONFIG_PACKAGE_mpd_mini			
        if (ra_func == raFunc_ALL_DISABLE) {
            mpd_main(proc, "stop");
        }
#endif		
#ifdef CONFIG_PACKAGE_mcc
        exec_cmd2("mcc_key switch %d", ra_func);
#endif
#ifdef CONFIG_PACKAGE_shairport
        shairport_main(proc, "stop");
#endif
#ifdef CONFIG_PACKAGE_upmpdcli
		upmpdcli_main(proc, "stop");
#endif
#ifdef CONFIG_PACKAGE_spotify
        spotify_main(proc, "stop");
#endif
#if defined(CONFIG_PACKAGE_mpd_mini)
        killall(applet_name, SIGUSR1);
#endif
    }
}
void omicfg_proc(montage_proc_t *proc, int act)
{
    act = ((act == BOOT) ? START : act);
    if (act == START) {
        if (f_exists("/lib/wdk/omnicfg"))
            exec_cmd("/lib/wdk/omnicfg reload");
    }
    else if (act == STOP) {
        if (f_exists("/lib/wdk/omnicfg"))
            exec_cmd("/lib/wdk/omnicfg stop");
    }
}

int init_proc_list(ProcessCode type, struct list_head **mylh, montage_proc_t **myptbl, int *mytblcnt)
{
    struct list_head *lh;
    montage_proc_t *ptbl;
    int tblcnt;
    int i;
    procListNode *node;

    switch(type) {
        case ProcessBoot:
        {
            lh = &bootProc;
            ptbl = boot_proc_tbl;
            tblcnt = sizeof(boot_proc_tbl) / sizeof(montage_proc_t);
            break;
        }
        case ProcessShutdown:
        {
            lh = &shutdownProc;
            ptbl = shutdown_proc_tbl;
            tblcnt = sizeof(shutdown_proc_tbl) / sizeof(montage_proc_t);
            break;
        }
        case ProcessCommit:
        case ProcessCommitAll:
        {
            lh = &commitProc;
            ptbl = cdb_proc_tbl;
            tblcnt = sizeof(cdb_proc_tbl) / sizeof(montage_proc_t);
            break;
        }
        case ProcessSave:
            return 0;
        default:
            return -1;
    }

    for (i = 0;i < tblcnt; i++) {
        if ((type == ProcessBoot) && !ptbl[i].boot)
            continue;
        if ((node = (procListNode *)calloc(sizeof(procListNode), 1)) != NULL) {
            node->list.next = node->list.prev = &node->list;
            node->proc = &ptbl[i];

            if (!list_empty(lh)) {
                struct list_head *prev = lh;
                struct list_head *next;
                list_for_each(next, lh) {
                    procListNode *node2 = (procListNode *)list_entry(next, procListNode, list);
                    if (node->proc->priority < node2->proc->priority)
                        break;
                    prev = next;
                }
                list_add(&node->list, prev, next);
            }
            else {
                list_add_tail(lh, &node->list);
            }
        }
    }
    
    *mylh = lh;
    *myptbl = ptbl;
    *mytblcnt = tblcnt;

    return 0;
}

static void exec_proc(procListNode *node, int ctrl)
{
    int async_exec;
    pid_t child;

    async_exec = (node->proc->flags & PROC_ASYNC_EXEC);
    if((async_exec) && ((ctrl==BOOT) || (ctrl==START)))
    {
        child = fork();
        if(child==0)
        {
            mtdm_ipc_lock();
            node->proc->exec_proc(node->proc, ctrl);
            mtdm_ipc_unlock();
            exit(0);
        }
        else if(child < 0)
        {
            mtdm_ipc_lock();
            node->proc->exec_proc(node->proc, ctrl);
            mtdm_ipc_unlock();
        }
    }
    else
    {
        mtdm_ipc_lock();
        node->proc->exec_proc(node->proc, ctrl);
        mtdm_ipc_unlock();
    }
}

void mtdm_proc_handle(ProcessCode type)
{
    struct timeval sAlltv, stv;
    struct list_head *lh = NULL;
    montage_proc_t *ptbl = NULL;
    int tblcnt = 0;

    if (init_proc_list(type, &lh, &ptbl, &tblcnt)) {
        logger (LOG_ERR, "init_proc_list failed!!");
        return;
    }

    if (lh && !list_empty(lh) && (type == ProcessCommit)) {
        /* do checkup and find cdb dependence */
        handle_cdb(lh);
    }
    else if (type == ProcessSave) {
        handle_cdb_save();
    }

    if (lh && !list_empty(lh)) {
        struct list_head *pos, *n;
        if (type == ProcessCommit) {
            list_for_each_i_safe(pos, n, lh) {
                procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
                gettimeofbegin(&stv);
                exec_proc(node, STOP);
                logger (LOG_INFO, "STOP module [%s] used time: %ld", node->proc->mname, gettimevaldiff(&stv));
            }
            list_for_each_safe(pos, n, lh) {
                procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
                gettimeofbegin(&stv);
                exec_proc(node, START);
                logger (LOG_INFO, "START module [%s] used time: %ld", node->proc->mname, gettimevaldiff(&stv));
            }
        }
        else if (type == ProcessCommitAll) {
            list_for_each_safe(pos, n, lh) {
                procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
                if (!node->proc->boot) {
                    continue;
                }
                gettimeofbegin(&stv);
                exec_proc(node, BOOT);
                logger (LOG_INFO, "BOOT module [%s] used time: %ld", node->proc->mname, gettimevaldiff(&stv));

                if (mtdm->bootState != bootState_start) {
                    if (mtdm->bootState == bootState_wanup) {
                        wan_main(NULL, "start");
                    }
                    if (mtdm->bootState == bootState_ipup) {
                        mtdm_wan_ip_up(mtdm->bootState, cdb_proc_tbl, (sizeof(cdb_proc_tbl) / sizeof(montage_proc_t)));
                    }
                    mtdm->bootState = bootState_start;
                }
            }
        }
        else if (type == ProcessShutdown) {
            gettimeofbegin(&sAlltv);
            list_for_each_safe(pos, n, lh) {
                procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
                gettimeofbegin(&stv);
                exec_proc(node, STOP);
                logger (LOG_INFO, "STOP module [%s] used time: %ld", node->proc->mname, gettimevaldiff(&stv));
            }
            logger (LOG_INFO, "All done: %ld", gettimevaldiff(&sAlltv));
        }
        else if (type == ProcessBoot) {
            gettimeofbegin(&sAlltv);
            list_for_each_safe(pos, n, lh) {
                procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
                gettimeofbegin(&stv);
                exec_proc(node, BOOT);
                logger (LOG_INFO, "BOOT module [%s] used time: %ld", node->proc->mname, gettimevaldiff(&stv));

                if (mtdm->bootState != bootState_start) {
                    if (mtdm->bootState == bootState_wanup) {
                        wan_main(NULL, "start");
                    }
                    if (mtdm->bootState == bootState_ipup) {
                        mtdm_wan_ip_up(mtdm->bootState, NULL, 0);
                    }
                    mtdm->bootState = bootState_start;
                }
            }
            logger (LOG_INFO, "All done: %ld", gettimevaldiff(&sAlltv));

            exec_cmd("/etc/init.d/reboot_test &");

            /* clear boot */
            mtdm->bootState = bootState_done;
        }

        list_for_each_safe(pos, n, lh) {
            procListNode *node = (procListNode *)list_entry(pos, procListNode, list);
            list_del(&node->list);
            free(node);
        }
    }
    else {
        logger (LOG_INFO, "Need to do Nothing!!");
    }
}

void mtdm_proc_init(void)
{
    montage_proc_t *ptbl;
    int tblcnt;
    int i, j;
    for (i=0; proc_tbl[i]!=NULL; i++) {
        ptbl = proc_tbl[i];
        tblcnt = proc_tbl_size[i];
        for (j=0; j<tblcnt; j++) {
            if (mtdm->boot.flags & PROC_DBG) {
                ptbl[j].flags |= PROC_DBG;
            }
        }
    }
}

void sendCmd(OpCodeCmd opc, OpCodeCmdArg arg, char *data)
{
    montage_proc_t *ptbl;
    int tblcnt;
    int i, j;

    if (mtdm->bootState != bootState_done) {
        if ((opc == OPC_CMD_WAN) && (arg == OPC_CMD_ARG_START)) {
            mtdm->bootState = bootState_wanup;
            logger (LOG_INFO, "Execute Cmd:[%s](0x%x) when boot", opc_cmd_name[OPC_CMD_IDX(opc)], opc);
        }
        else if (opc == OPC_CMD_WANIPUP) {
            mtdm->bootState = bootState_ipup;
            logger (LOG_INFO, "Execute Cmd:[%s](0x%x) when boot", opc_cmd_name[OPC_CMD_IDX(opc)], opc);
        }
        else {
            if (opc <= OPC_CMD_MAX && opc >= OPC_CMD_MIN) {
                logger (LOG_ERR, "Execute Cmd:[%s](0x%x) when boot, drop it", opc_cmd_name[OPC_CMD_IDX(opc)], opc);
            }
            else {
                logger (LOG_ERR, "Execute Cmd(0x%x) when boot, drop it", opc);
            }
        }
    }
    else if (opc > OPC_CMD_MAX || opc < OPC_CMD_MIN) {
        logger (LOG_ERR, "Execute Cmd(0x%x) out of range", opc);
    }
    else if (opc < OPC_CMD_COMMIT) {
        if (arg >= OPC_CMD_ARG_MAX || arg <= OPC_CMD_ARG_MIN) {
            logger (LOG_ERR, "Execute Cmd Arg(0x%x) out of range", arg);
        }
        else {
            logger (LOG_INFO, "Execute Cmd:[%s](0x%x) [%s](0x%x)", opc_cmd_name[OPC_CMD_IDX(opc)], opc, 
                                                                   opc_cmd_arg_name[arg], arg);
            for (i=0; proc_tbl[i]!=NULL; i++) {
                ptbl = proc_tbl[i];
                tblcnt = proc_tbl_size[i];
                for (j=0; j<tblcnt; j++) {
                    if (strcmp(ptbl[j].mname, opc_cmd_name[OPC_CMD_IDX(opc)]) == RET_OK) {
                        logger (LOG_INFO, "Cmd:[%s] [%s] executing", ptbl[j].mname, opc_cmd_arg_name[arg]);
                        if (arg == OPC_CMD_ARG_RESTART) {
                            ptbl[j].exec_proc(&ptbl[j], STOP);
                            ptbl[j].exec_proc(&ptbl[j], START);
                        }
                        else if (arg == OPC_CMD_ARG_STOP) {
                            ptbl[j].exec_proc(&ptbl[j], STOP);
                        }
                        else if (arg == OPC_CMD_ARG_START) {
                            ptbl[j].exec_proc(&ptbl[j], START);
                        }
                        else if (arg == OPC_CMD_ARG_BOOT) {
                            ptbl[j].exec_proc(&ptbl[j], BOOT);
                        }
                        else if (arg == OPC_CMD_ARG_DBG) {
                            ptbl[j].flags |= PROC_DBG;
                        }
                        else if (arg == OPC_CMD_ARG_UNDBG) {
                            ptbl[j].flags &= ~PROC_DBG;
                        }
                        else {
                            logger (LOG_INFO, "Cmd:[%s] [%s] undefined", 
                                    ptbl[j].mname, opc_cmd_arg_name[OPC_CMD_ARG_UNKNOWN]);
                        }
                            
                        logger (LOG_INFO, "Cmd:[%s] [%s] done", ptbl[j].mname, opc_cmd_arg_name[arg]);
                        return;
                    }
                }
            }
        }
    }
    else {
        logger (LOG_INFO, "Execute Cmd:[%s](0x%x)", opc_cmd_name[OPC_CMD_IDX(opc)], opc);
        switch(opc) {
            case OPC_CMD_COMMIT:
                mtdm_proc_handle(ProcessCommit);
                break;
            case OPC_CMD_SAVE:
                mtdm_proc_handle(ProcessSave);
                break;
            case OPC_CMD_SHUTDOWN:
                mtdm_proc_handle(ProcessShutdown);
                break;
            case OPC_CMD_WANIPUP:
                mtdm_wan_ip_up(bootState_done, NULL, 0);
                break;
            case OPC_CMD_WANIPDOWN:
                mtdm_wan_ip_down();
                break;
            case OPC_CMD_OCFGARG:
                mtdm_ocfg_arg();
                break;
            case OPC_CMD_SLEEP:
                    mtdm_sleep(atoi(data));
                break;
#ifdef SPEEDUP_WLAN_CONF
            case OPC_CMD_WLHUP:
                mtdm_wlhup();
                break;
#endif
            default:
                logger (LOG_ERR, "Execute Cmd(0x%x) unknown", opc);
                break;
        }
        //logger (LOG_INFO, "Cmd:[%s](0x%x) done", opc_cmd_name[OPC_CMD_IDX(opc)], opc);
    }
}


