/*
 * =====================================================================================
 *
 *       Filename:  checkup.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/22/2016 05:33:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <time.h>
#include <string.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

typedef struct {
    const char *cname;
    const char *cname_org;
    void (*exec_proc)(char *new_value, char *org_value, int clear);
} montage_checkup_t;

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

#if !defined(__PANTHER__)
void lan_ip_proc(char *new_value, char *org_value, int clear);
#endif
void remote_ctrl_proc(char *new_value, char *org_value, int clear);
void wan_clone_mac_proc(char *new_value, char *org_value, int clear);
void op_work_proc(char *new_value, char *org_value, int clear);

montage_checkup_t checkup_tbl[] = {
#if !defined(__PANTHER__)
    { "$lan_ip",               "$org_lan_ip",               lan_ip_proc        },
#endif
    { "$sys_remote_enable",    "$org_sys_remote_enable",    remote_ctrl_proc   },
    { "$sys_remote_ip",        "$org_sys_remote_ip",        remote_ctrl_proc   },
    { "$sys_remote_port",      "$org_sys_remote_port",      remote_ctrl_proc   },
    { "$sys_remote_port",      "$org_sys_remote_port",      wan_clone_mac_proc },
    { "$wan_clone_mac_enable", "$org_clone_mac_enable",     wan_clone_mac_proc },
    { "$op_work_mode",         "$org_op_work_mode",         op_work_proc       },
};

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

#if !defined(__PANTHER__)
void lan_ip_proc(char *new_value, char *org_value, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        char lanip[64];
        char dhcps_start[64];
        char dhcps_end[64];
        char dhcps_ip[64];
        char *p1 = lanip, *p2 = dhcps_start, *p3 = dhcps_end;
        cdb_get("$lan_ip", lanip);
        cdb_get("$dhcps_start", dhcps_start);
        cdb_get("$dhcps_end", dhcps_end);
        if ((p1 = strrchr(lanip, '.'))) {
            *p1 = '\0';
        }
        if ((p2 = strrchr(dhcps_start, '.'))) {
            *p2 = '\0';
        }
        if ((p3 = strrchr(dhcps_end, '.'))) {
            *p3 = '\0';
        }
        if (!p1 || !p2 || !p3) {
            return; 
        }
        if (strcmp(lanip, dhcps_start)) {
            sprintf(dhcps_ip, "%s.%s", lanip, p2+1);
            cdb_set("$dhcps_start", dhcps_ip);
            sprintf(dhcps_ip, "%s.%s", lanip, p3+1);
            cdb_set("$dhcps_end", dhcps_ip);
            isneedreboot = 1;
        }
        done = 1;
    }
}
#endif

void remote_ctrl_proc(char *new_value, char *org_value, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        char tmp[CDB_DATA_MAX_LENGTH];
        cdb_get("$nat_enable", tmp);
        cdb_set("$nat_enable", tmp);
        done = 1;
    }
}

void wan_clone_mac_proc(char *new_value, char *org_value, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
#if !defined(__PANTHER__)
        isneedreboot = 1;
#endif
        done = 1;
    }
}

void op_work_proc(char *new_value, char *org_value, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        int kop_work = atoi(new_value);
        int fop_work = atoi(org_value);
        switch(kop_work) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 7:
            case 9:
                {
                    switch(fop_work) {
                        case 4:
                        case 5:
                        case 8:
                            cdb_set("$nat_enable", "1");
                            cdb_set("$fw_enable", "1");
                            cdb_set("$dhcps_enable", "1");
                            cdb_set("$wl_forward_mode", "0");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 4:
            case 5:
            case 8:
                {
                    switch(fop_work) {
                        case 3:
                        case 4:
                        case 5:
                        case 9:
                            cdb_set("$nat_enable", "0");
                            cdb_set("$fw_enable", "0");
                            cdb_set("$dhcps_enable", "0");
                            cdb_set("$wl_forward_mode", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }

        switch(kop_work) {
            case 0:
            case 1:
            case 2:
            case 6:
            case 7:
                {
                    switch(fop_work) {
                        case 3:
                        case 4:
                        case 5:
                        case 9:
                            // cdb_ap
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_enable1", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 3:
            case 9:
                {
                    switch(fop_work) {
                        case 0:
                        case 1:
                        case 2:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            // cdb_ap_sta 1
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_role2", "1");
                            cdb_set("$wl_bss_enable1", "1");
                            cdb_set("$wl_bss_enable2", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 4:
            case 5:
                {
                    switch(fop_work) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 6:
                        case 7:
                            // cdb_ap_sta 257
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_role2", "257");
                            cdb_set("$wl_bss_enable1", "1");
                            cdb_set("$wl_bss_enable2", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
        done = 1;
    }
}

void clear_checkup_state(void)
{
    montage_checkup_t *ptbl = checkup_tbl;
    int tblcnt = sizeof(checkup_tbl) / sizeof(montage_checkup_t);
    for (--tblcnt;tblcnt >= 0; tblcnt--) {
        ptbl[tblcnt].exec_proc(0, 0, 1);
    }
}

#if defined(__PANTHER__)
int checkup_main(int cdbsave, int needreboot)
{
    montage_checkup_t *ptbl = checkup_tbl;
    char buf[CDB_DATA_MAX_LENGTH];
    char buf_org[CDB_DATA_MAX_LENGTH];
    int tblcnt = sizeof(checkup_tbl) / sizeof(montage_checkup_t);
    int i;

    int iscdbsave = 0;
    int isneedreboot = 0;

    /*  get and compare old value & new value in checkup_tbl
              call the handling function if they are different
     */
    for (i = 0; i < tblcnt; i++)
    {
        if ((cdb_get((char *)ptbl[i].cname, buf) < 0) || !strlen(buf))
            continue;

        if ((cdb_get((char *)ptbl[i].cname_org, buf_org) < 0) || !strlen(buf_org))
        {
            cdb_set((char *)ptbl[i].cname_org, buf);
            continue;
        }

        if (strcmp(buf, buf_org))
            ptbl[i].exec_proc(buf, buf_org, 0);

        cdb_set((char *)ptbl[i].cname_org, buf);
    }

    clear_checkup_state();

    if (!iscdbsave && isneedreboot)
    {
        cdb_save();
        reboot(RB_AUTOBOOT);
    }

    return 0;
}
#else
int checkup_main(int cdbsave, int needreboot)
{
    montage_checkup_t *ptbl = checkup_tbl;
    int tblcnt = sizeof(checkup_tbl) / sizeof(montage_checkup_t);
    char kcdb[BUF_SIZE] = { 0 };
    char fcdb[BUF_SIZE] = { 0 };
    int i;

    int iscdbsave = 0;
    int isneedreboot = 0;

    iscdbsave = cdbsave;
    if (isneedreboot == 0) {
        isneedreboot = needreboot;
    }

    for (i = 0; i < tblcnt; i++) {
        if (cdb_get((char *)ptbl[i].cname, kcdb) < 0) {
            continue;
        }
        if (fcdb_get(ptbl[i].cname, fcdb) < 0) {
            continue;
        }
        if (strcmp(kcdb, fcdb)) {
            ptbl[i].exec_proc(kcdb, fcdb, 0);
        }
    }
    clear_checkup_state();
    if (!iscdbsave && isneedreboot) {
        cdb_save();
        reboot(RB_AUTOBOOT);
    }

    return 0;
}
#endif

