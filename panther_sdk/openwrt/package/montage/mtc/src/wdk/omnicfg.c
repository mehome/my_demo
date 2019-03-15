#define _GNU_SOURCE
#include <ctype.h>
#include <libutils/wlutils.h>
#include "include/mtdm_client.h"
#include "wdk.h"

#include <libutils/ipc.h>

#undef STDOUT
#include "include/mtdm_misc.h"

#define OMNICFG_NAME    "omnicfg"
#define OMNICFG_BIN     "/lib/wdk/"OMNICFG_NAME
#define OMNICFG_PIDFILE "/var/run/"OMNICFG_NAME".pid"
#define OMNICFG_LOG     "/www/"OMNICFG_NAME"_log"

#define SYSLOG_NAME     "syslogd"
#define SYSLOG_BIN      "/sbin/"SYSLOG_NAME
#define SYSLOG_FILE     "/var/log/messages"
#define SYSLOG_ZIP_FILE "/var/log/messages.gz"

#define HOSTAPD_PIDFILE "/var/run/hostapd.pid"

#define UNKNOWN (-1)
#define MYLOG(fmt, args...)  do { \
    logger(LOG_INFO, "[omnicfg]:[" fmt "]", ## args); \
    LOG("[omnicfg]:[" fmt "]", ## args); \
} while(0)

#ifdef SPEEDUP_WLAN_CONF
void set_ssid_pwd(char *ssid, char *pass)
{
    if (strlen(ssid) > 32)
    {
        printf("critical error, ssid two long : %s\n", ssid);
        return;
    }

    if (pass)
    {
        if (strlen(pass) > 64)
        {
            printf("critical error, pass two long : pass:%s\n", pass);
            return;
        }
    }

    cdb_set("$wl_bss_ssid2", ssid);
    cdb_set("$wl_bss_bssid2", "");

    if (!pass)
    {
        cdb_set("$wl_bss_sec_type2", "0");
        cdb_set("$wl_bss_cipher2", "0");
        cdb_set("$wl_bss_wpa_psk2", "");
        cdb_set("$wl_bss_wep_1key2", "");
        cdb_set("$wl_bss_wep_2key2", "");
        cdb_set("$wl_bss_wep_3key2", "");
        cdb_set("$wl_bss_wep_4key2", "");
        cdb_set("$wl_passphase2", "");
        return;
    }

    cdb_set("$wl_bss_wep_1key2", pass);
    cdb_set("$wl_bss_wep_2key2", pass);
    cdb_set("$wl_bss_wep_3key2", pass);
    cdb_set("$wl_bss_wep_4key2", pass);
    cdb_set("$wl_passphase2", pass);
    cdb_set("$wl_bss_sec_type2", "4");
}
#endif

int omnicfg_cdb_save(void)
{
    char buf[MSBUF_LEN];

    if (cdb_get_int("$op_work_mode", 0) == WORK_MODE_AP) {
        cdb_set_int("$omi_work_mode", WORK_MODE_AP);
        return 0;
    }

    if (cdb_get_int("$omicfg_rollback", 0))
    {
        /* rollback enable, backup current op_work_mode */
        cdb_set_int("$omi_work_mode", cdb_get_int("$op_work_mode", 0));
    }
    else
    {
        /* rollback disabled (force rollback to AP mode) */
        cdb_set_int("$omi_work_mode", WORK_MODE_AP);
    }

    cdb_set("$omi_bss_ssid2", cdb_get_array_str("$wl_bss_ssid", 2, buf, sizeof(buf), ""));
    cdb_set("$omi_bss_bssid2", cdb_get_array_str("$wl_bss_bssid", 2, buf, sizeof(buf), ""));
    cdb_set_int("$omi_bss_cipher2", cdb_get_array_int("$wl_bss_cipher", 2, 0));
    cdb_set_int("$omi_bss_sec_type2", cdb_get_array_int("$wl_bss_sec_type", 2, 0));
    cdb_set("$omi_bss_wpa_psk2", cdb_get_array_str("$wl_bss_wpa_psk", 2, buf, sizeof(buf), ""));
    cdb_set("$omi_bss_wep_1key2", cdb_get_array_str("$wl_bss_wep_1key", 2, buf, sizeof(buf), ""));
    cdb_set("$omi_bss_wep_2key2", cdb_get_array_str("$wl_bss_wep_2key", 2, buf, sizeof(buf), ""));
    cdb_set("$omi_bss_wep_3key2", cdb_get_array_str("$wl_bss_wep_3key", 2, buf, sizeof(buf), ""));
    cdb_set("$omi_bss_wep_4key2", cdb_get_array_str("$wl_bss_wep_4key", 2, buf, sizeof(buf), ""));

    return 0;
}

int omnicfg_simply_commit(void)
{
    mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_STOP, NULL);
    mtdm_client_simply_call(OPC_CMD_DNS, OPC_CMD_ARG_STOP, NULL);
    mtdm_client_simply_call(OPC_CMD_WL, OPC_CMD_ARG_STOP, NULL);
    mtdm_client_simply_call(OPC_CMD_OP, OPC_CMD_ARG_RESTART, NULL);
    mtdm_client_simply_call(OPC_CMD_LAN, OPC_CMD_ARG_START, NULL);
    mtdm_client_simply_call(OPC_CMD_WL, OPC_CMD_ARG_START, NULL);
    mtdm_client_simply_call(OPC_CMD_DNS, OPC_CMD_ARG_START, NULL);
    mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_START, NULL);

    return 0;
}

static char WAN_IF[16] = { 0 };
static char *WAN = WAN_IF;

static int omnicfg_callback(char *rbuf, int rlen)
{
    char *wan = strstr(rbuf, "WAN=");
    int i = 0, j = 4;

    memset(WAN_IF, 0, sizeof(WAN_IF));
    if (wan && ((rbuf == wan) || (*(wan-1) == '\n'))) {
        while (i < (sizeof(WAN_IF)-1)) {
            WAN_IF[i] = *(wan + j);
            if ((WAN_IF[i] == '\n') || (WAN_IF[i] == 0)) {
                WAN_IF[i] = 0;
                break;
            }
            i++;
            j++;
        }
    }

    return 0;
}

static int __attribute__ ((unused)) omnicfg_get_mtdm_info(void)
{
    MtdmPkt *packet;
    int ret = RET_ERR;

    if((packet = calloc(1, sizeof(MtdmPkt)))) {
        packet->head.ifc = INFC_DAT;
        packet->head.opc = OPC_DAT_INFO;
        ret = mtdm_client_call(packet, omnicfg_callback);
        free(packet);
    }

    return ret;
}

static int omicfg_set_directmode_route(int set)
{
    char *addr;

    /*
     * Direct Mode add/remove route for App
     */
    if ((addr = getenv("REMOTE_ADDR")) != NULL) {
        if (set) {
            route_add("br0", 0, addr, NULL, "255.255.255.255");
        }
        else {
            route_del("br0", 0, addr, NULL, "255.255.255.255");
        }
        MYLOG("%s directmode config by REMOTE_ADDR(%s)", (set) ? "add" : "remove", addr);
    }

    return 0;
}

static int omicfg_wait_directmode_timeout(void)
{
    int loop = cdb_get_int("$omi_time_to_kill_ap", 0);
    int new;

    while (loop-- > 0) {
        sleep(1);
        new = cdb_get_int("$omi_time_to_kill_ap", 0);
        if (loop > new) {
            loop = new;
        }
        MYLOG("wait directmode timeout new[%d], loop[%d]", new, loop);
    }

    return 0;
}

#if !defined(__PANTHER__)
static int omnicfg_hostapd_reload(void)
{
    int pid;

    if (f_exists(HOSTAPD_PIDFILE)) {
        pid = read_pid_file(HOSTAPD_PIDFILE);
        if (pid != UNKNOWN) {
            kill(pid, SIGUSR2);
        }
    }

    return 0;
}
#endif
#ifndef SPEEDUP_WLAN_CONF
// to scan AP and check AP security
// AP_NOT_FOUND   0
// AP_FOUND       1
// AP_PASS_ERROR  2
// AP_SEC_ERROR   3
static int check_ap_scan(char *ssid, char *pass)
{
    int cnt;
    int ret;

    for (cnt=0;cnt < SCAN_RETRY_CNT; cnt++) {
        ret = wifi_set_ssid_pwd(ssid, pass);
        if (ret != SCAN_AP_NOT_FOUND) {
            break;
        }
    }

    return ret;
}
#endif
static int omnicfg_cdb_restore(void)
{
    char buf[MSBUF_LEN];

    cdb_set_int("$op_work_mode", cdb_get_int("$omi_work_mode", 0));
    cdb_set("$wl_bss_ssid2", cdb_get_array_str("$omi_bss_ssid", 2, buf, sizeof(buf), ""));
    cdb_set("$wl_bss_bssid2", cdb_get_array_str("$omi_bss_bssid", 2, buf, sizeof(buf), ""));
    cdb_set_int("$wl_bss_cipher2", cdb_get_array_int("$omi_bss_cipher", 2, 0));
    cdb_set_int("$wl_bss_sec_type2", cdb_get_array_int("$omi_bss_sec_type", 2, 0));
    cdb_set("$wl_bss_wpa_psk2", cdb_get_array_str("$omi_bss_wpa_psk", 2, buf, sizeof(buf), ""));
    cdb_set("$wl_bss_wep_1key2", cdb_get_array_str("$omi_bss_wep_1key", 2, buf, sizeof(buf), ""));
    cdb_set("$wl_bss_wep_2key2", cdb_get_array_str("$omi_bss_wep_2key", 2, buf, sizeof(buf), ""));
    cdb_set("$wl_bss_wep_3key2", cdb_get_array_str("$omi_bss_wep_3key", 2, buf, sizeof(buf), ""));
    cdb_set("$wl_bss_wep_4key2", cdb_get_array_str("$omi_bss_wep_4key", 2, buf, sizeof(buf), ""));

    return 0;
}

#if !defined(OMNICFG_IPC_LOCK)
static int omnicfg_save_pid(void)
{
    FILE *pidfile;

    mkdir("/var/run", 0755);
    if (f_exists(OMNICFG_PIDFILE)) {
        if ((get_pid_by_pidfile(OMNICFG_PIDFILE, OMNICFG_NAME) != -1) || 
            (get_pid_by_pidfile(OMNICFG_PIDFILE, OMNICFG_BIN) != -1)) {
            return -1;
        }
    }

    if ((pidfile = fopen(OMNICFG_PIDFILE, "w")) != NULL) {
        fprintf(pidfile, "%d\n", getpid());
        (void) fclose(pidfile);
    }

    return 0;
}

static int omnicfg_remove_pid(void)
{
    if (f_exists(OMNICFG_PIDFILE)) {
        unlink(OMNICFG_PIDFILE);
    }

    return 0;
}
#endif

static int omnicfg_syslogd(int act)
{
    char buf[USBUF_LEN];
    int omicfg_log_mode = cdb_get_int("$omicfg_log_mode", 0);
    int i;

    switch (act) {
        case OMNICFG_LOG_CLEAN:
            exec_cmd("rm -f "OMNICFG_LOG"*");
            break;
        case OMNICFG_LOG_START:
            if (omicfg_log_mode > 0) {
                killall(SYSLOG_NAME, SIGTERM);
                if (f_exists(SYSLOG_FILE)) {
                    unlink(SYSLOG_FILE);
                }
                start_stop_daemon(SYSLOG_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, NULL);
            }
            break;
        case OMNICFG_LOG_STOP:
            killall(SYSLOG_NAME, SIGTERM);
            break;
        case OMNICFG_LOG_BACKUP:
            killall(SYSLOG_NAME, SIGTERM);
            if (omicfg_log_mode > 1) {
                for (i=0;i<100;i++) {
                    sprintf(buf, OMNICFG_LOG"%2.2d.gz", i);
                    if (!f_exists(buf)) {
                        break;
                    }
                }
            }
            else {
                sprintf(buf, OMNICFG_LOG"00.gz");
            }
            exec_cmd("gzip "SYSLOG_FILE);
            cp(SYSLOG_ZIP_FILE, buf);
            unlink(SYSLOG_ZIP_FILE);
            break;
        default:
            break;
    }

    return 0;
}

static int omnicfg_reload(int conf)
{
    cdb_set_int("$omi_iw_channel", 0);

#if !defined(__PANTHER__)
    omnicfg_hostapd_reload();
#endif

    if (conf) {
        mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_STOP, NULL);
        mtdm_client_simply_call(OPC_CMD_OCFGARG, 0, NULL);
    }
    else {
        mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_RESTART, NULL);
    }

    return 0;
}

static int omnicfg_stop(void)
{
    mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_STOP, NULL);
    exec_cmd2("wd smrtcfg stop");

    cdb_set_int("$omicfg_sniffer_enable", 0);
    cdb_set_int("$omicfg_config_enable", 0);
    cdb_set_int("$omi_config_mode", OMNICFG_MODE_DISABLE);
    cdb_set_int("$omi_target_timeout", 0);

    return 0;
}

static void omnicfg_json_str(int append, char *name, char *value)
{
    printf("%s\"%s\":\"%s\"", (append) ? "," : "", name, value);
}

static void omnicfg_json_num(int append, char *name, int value)
{
    printf("%s\"%s\":%d", (append) ? "," : "", name, value);
}

static void omnicfg_json_num_to_str(int append, char *name, int value)
{
    char buf[USBUF_LEN];
    snprintf(buf, sizeof(buf), "%d", value);
    omnicfg_json_str(append, name, buf);
}

/*
{"op_work_mode":"1","SupportTimeout":"enable","RemainTime":0,"WifiState":3,"omi_work_mode":0,"omi_restore_apply":0}
 */
static int omnicfg_state(void)
{
    int i, j, k;

    printf("{");

    i = cdb_get_int("$op_work_mode", 0);
    omnicfg_json_num_to_str(0, "op_work_mode", i);

    i = cdb_get_int("$omi_config_mode", 0);
    if (i == 1) {
        k = (cdb_get_int("$omicfg_sniffer_timeout", 0) <= 0) ? 0 : 1;
    }
    else {
        k = (cdb_get_int("$omicfg_timeout_period", -1) == 0) ? 0 : 1;
    }
    if (k > 0) {
        omnicfg_json_str(1, "SupportTimeout", "enable");
    }
    else {
        omnicfg_json_str(1, "SupportTimeout", "disable");
    }

    j = cdb_get_int("$omicfg_config_enable", 0);
    k = 0;
    if (j > 0) {
        k = cdb_get_int("$omi_target_timeout", 0) - uptime();
        k = (k < 0) ? 0 : k;
    }
    else if ((i == 2) && (j == 0)) {
        k = cdb_get_int("$omi_target_timeout", 0) - uptime();
        k = (k < 0) ? 0 : k;
    }
    omnicfg_json_num(1, "RemainTime", k);

    omnicfg_json_num(1, "WifiState", cdb_get_int("$omi_result", 0));

    omnicfg_json_num(1, "omi_work_mode", cdb_get_int("$omi_work_mode", 0));

    omnicfg_json_num(1, "omi_restore_apply", cdb_get_int("$omi_restore_apply", 0));

    printf("}\n");

    return 0;
}

static int omnicfg_voice(char *act)
{
#if 1
	struct omnicfg_voice_mapping {
		char *name;
		int fail_type;
	};
    struct omnicfg_voice_mapping tbls[] = {
        { "pass_error",          1},
        { "no_ip_got",           2},
    };
    const int cnt = ARRAY_SIZE(tbls);
    int i;
	struct stat buf;

    for (i=0;i<cnt;i++) {
        if (!strcmp(tbls[i].name, act))
        {
        	send_button_direction_to_dueros(2, 1, tbls[i].fail_type);
            break;
        }
    }
#else
#ifdef CONFIG_PACKAGE_mcc
    struct mcc_dbus_act {
        char *name;
        char *cmd;
    };
    struct mcc_dbus_act tbls[] = {
        { "trigger",             "omf_trigger"             }, 
        { "cfg",                 "omf_cfg"                 }, 
        { "cfg_time_out",        "omf_cfg_time_out"        }, 
        { "cfg_done",            "omf_cfg_done"            }, 
        { "cfg_direct",          "omf_cfg_direct"          }, 
        { "cfg_direct_time_out", "omf_cfg_direct_time_out" }, 
        { "cfg_fail",            "omf_cfg_fail"            }, 
        { "connected",           "omf_connected"           }, 
        { "ap_not_found",        "omf_ap_not_found"        }, 
        { "pass_error",          "omf_pass_error"          }, 
        { "no_ip_got",           "omf_no_ip_got"           }, 
        { "ip_connect_fail",     "omf_ip_connect_fail"     }, 
        { "connect_time_out",    "omf_connect_time_out"    }, 
    };
    const int cnt = ARRAY_SIZE(tbls);
    int i;

    if (f_exists("/usr/bin/mcc_dbus")) {
        killall("wavplayer", SIGKILL);

        for (i=0;i<cnt;i++) {
            if (!strcmp(tbls[i].name, act)) {
                exec_cmd2("mcc_dbus %s", tbls[i].cmd);
                break;
            }
        }
    }
#else
    struct omnicfg_voice_mapping {
        char *name;
        char *filename;
    };
    struct omnicfg_voice_mapping tbls[] = {
        { "trigger",             "/tmp/res/connecting_to_network.wav"     },
        { "cfg",                 NULL           },
        { "cfg_time_out",        NULL           },
        { "cfg_done",            NULL           },
        { "cfg_direct",          NULL           },
        { "cfg_direct_time_out", NULL           },
        { "cfg_fail",            NULL           },
        { "connecting",          "/tmp/res/connecting.wav"                },
        { "connected",           NULL           },
        { "not_connected",       "/tmp/res/speaker_not_connected.wav"     },
        { "ap_not_found",        "/tmp/res/connection_unsuccessful.wav"   },
        { "pass_error",          "/tmp/res/invalid_password.wav"          },
        { "no_ip_got",           "/tmp/res/speaker_not_connected.wav"     },
        { "ip_connect_fail",     "/tmp/res/speaker_not_connected.wav"     },
        { "connect_time_out",    "/tmp/res/connection_unsuccessful.wav"   },
    };
    const int cnt = ARRAY_SIZE(tbls);
    int i;
	struct stat buf;

    for (i=0;i<cnt;i++) {
        if (!strcmp(tbls[i].name, act))
        {
            if(tbls[i].filename)
            {
                if(stat(tbls[i].filename, &buf) == 0)
                {
                    //killall("/usr/bin/wavplayer", SIGKILL);
                    exec_cmd2("/usr/bin/wavplayer %s &", tbls[i].filename);
                }
                break;
            }
        }
    }
#endif
#endif

    return 0;
}

#if 0
static int omnicfg_fail_voice(void)
{
    switch (cdb_get_int("$omi_result", 3)) {
        case OMNICFG_R_UNKNOWN:
        case OMNICFG_R_CONNECTING:
        case OMNICFG_R_SUCCESS:
            break;
        case OMNICFG_R_AP_NOT_FOUND:
            omnicfg_voice("ap_not_found");
            break;
        case OMNICFG_R_PASS_ERROR:
            omnicfg_voice("pass_error");
            break;
        case OMNICFG_R_IP_NOT_AVAILABLE:
            omnicfg_voice("no_ip_got");
            break;
        case OMNICFG_R_IP_NOT_USABLE:
            omnicfg_voice("ip_connect_fail");
            break;
        case OMNICFG_R_CONNECT_TIMEOUT:
            omnicfg_voice("connect_time_out");
            break;
        default:
            break;
    }

    return 0;
}
#endif

static int omnicfg_restore(void)
{
    //omnicfg_fail_voice();

    MYLOG("restore to last success config");

    cdb_set_int("$omicfg_sniffer_enable", 0);
    cdb_set_int("$omicfg_config_enable", 0);
    cdb_set_int("$omi_config_mode", OMNICFG_MODE_DISABLE);
    cdb_set_int("$omi_restore_apply", 1);

    omnicfg_cdb_restore();

    mtdm_client_simply_call(OPC_CMD_COMMIT, 0, NULL);
    mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_RESTART, NULL);

    omnicfg_syslogd(OMNICFG_LOG_BACKUP);

    return 0;
}

static int omnicfg_fail_case(int restart)
{
    int omicfg_group_cmd_en = cdb_get_int("$omicfg_group_cmd_en", 0);

    if (is_directmode()) {
        if (restart) {
            omicfg_wait_directmode_timeout();
        }
        omicfg_set_directmode_route(0);
    }

    /*
     * 一鍵配置的DUT會結束配置，回到trigger前的狀態。
     * 舊DUT會將omicfg_config_enable設為0，並重啟 hostapd 重新配置 Direct Mode
     */
    if (omicfg_group_cmd_en > 0) {
        omnicfg_restore();
    }
    else {
        cdb_set_int("$op_work_mode", WORK_MODE_AP);
        cdb_set_int("$omicfg_config_enable", 0);
        cdb_set_int("$omi_config_mode", OMNICFG_MODE_DIRECT);
        omnicfg_reload(0);
        //omnicfg_fail_voice();
        omnicfg_syslogd(OMNICFG_LOG_BACKUP);
        if (restart) {
            omnicfg_simply_commit();
            MYLOG("restart omnicfg in restart function");

            mtdm_client_simply_call(OPC_CMD_SAVE, 0, NULL);
        }
        else {
            MYLOG("restart omnicfg in redo function");
        }
    }

    // return < 0, it's failed
    return -1;
}

static int omnicfg_redo(void)
{
    return omnicfg_fail_case(0);
}

static int omnicfg_restart(void)
{
    return omnicfg_fail_case(1);
}

static int omnicfg_check(void)
{
    struct route_info rtInfo;
    char wanif_ip[30];
    int omicfg_time;
    int ret;

    if ((omicfg_time = cdb_get_int("$omicfg_wifi_conn_time", 0)) > 0) {

        omicfg_time *= 10;
        MYLOG("check connect");
        while (omicfg_time > 0) {

            if(omnicfg_shall_exit())
                return -1;

            ret = cdb_get_int("$omi_wpa_supp_result", 0);
            if (ret == 2) {
                MYLOG("authenticate fail");
                omnicfg_restart();
                cdb_set_int("$omi_result", OMNICFG_R_PASS_ERROR);
                omnicfg_voice("pass_error");
                return -1;
            }
            else if (ret == 3) {
                MYLOG("wpa_supplicant cannot found AP");
                omnicfg_restart();
                cdb_set_int("$omi_result", OMNICFG_R_AP_NOT_FOUND);
                omnicfg_voice("ap_not_found");
                return -1;
            }
            else if (ret == 1) {
                MYLOG("Connect complete");
                break;
            }

            usleep(100 * 1000);
            omicfg_time--;
        }

        if(omnicfg_shall_exit())
            return -1;

        if (omicfg_time <= 0) {
            MYLOG("Connect timeout");
            omnicfg_restart();
            cdb_set_int("$omi_result", OMNICFG_R_CONNECT_TIMEOUT);
            return -1;
        }
    }

    if ((omicfg_time = cdb_get_int("$omicfg_ip_conn_time", 0)) > 0) {

        omicfg_time *= 10;
        MYLOG("check ip address");
        while (omicfg_time > 0) {

            if(omnicfg_shall_exit())
                return -1;

            omnicfg_get_mtdm_info();
            if (*WAN != 0) {
                if ((get_ifname_ether_ip(WAN, wanif_ip, sizeof(wanif_ip)) == EXIT_SUCCESS) && 
                    (route_gw_info(&rtInfo) == EXIT_SUCCESS)) {
                    // 1.there is IP in wanif, but not sure IFUP
                    // 2.there is default gw
                    break;
                }
            }

            usleep(100 * 1000);
            omicfg_time--;
        }

        if(omnicfg_shall_exit())
            return -1;

        if (omicfg_time <= 0) {
            MYLOG("ip not found");
            omnicfg_restart();
            cdb_set_int("$omi_result", OMNICFG_R_IP_NOT_AVAILABLE);
            omnicfg_voice("no_ip_got");
            return -1;
        }

        MYLOG("get ip(%s) gateway(%s)", wanif_ip, inet_ntoa(rtInfo.gateWay));
    }

    MYLOG("check done success");

    return 0;
}

static int omnicfg_config(void)
{
    char *ssid = getenv("SSID");
    char *pass = getenv("PASS");
    char *bssid = getenv("BSSID");
    char *mmode = getenv("MODE");
    int mode = ((mmode) ? atoi(mmode) : WORK_MODE_WISP);
#ifdef SPEEDUP_WLAN_CONF
	char *ch_tmp = getenv("CH");
    int ch = ((ch_tmp) ? atoi(ch_tmp) : 0);
    MYLOG("config CH(%d) MODE(%d) SSID(%s) PASS(%s)", ch, mode, ssid, pass);
#else
    int ret;

    MYLOG("config MODE(%d) SSID(%s) PASS(%s)", mode, ssid, pass);
#endif
    if ((ssid == NULL) || (strlen(ssid) == 0)) {
        // AP_NOT_FOUND
        MYLOG("Can not found AP (SSID is empty)");
        omnicfg_redo();
        cdb_set_int("$omi_result", OMNICFG_R_AP_NOT_FOUND);
        return 0;
    }
    if ((pass != NULL) && (strlen(pass) == 0)) {
        pass = NULL;
    }

    cdb_set_int("$omicfg_sniffer_enable", 0);
    cdb_set_int("$omicfg_config_enable", 0);
    cdb_set_int("$omi_wpa_supp_state", 0);
    cdb_set_int("$omi_wpa_supp_result", 0);
    cdb_set_int("$omi_wpa_supp_retry", cdb_get_int("$omicfg_wifi_try_cnt", 0));
    cdb_set_int("$omi_restore_apply", 0);
#ifdef SPEEDUP_WLAN_CONF
    if (ch > 0)
    {
        cdb_set_int("$omi_scan_freq", (2412 + (ch - 1) * 5));
    }
#endif
    cdb_set_int("$op_work_mode", mode);

    if (is_directmode()) {
        omicfg_set_directmode_route(1);
    }
#ifdef SPEEDUP_WLAN_CONF
    set_ssid_pwd(ssid, pass);
#else
    ret = check_ap_scan(ssid, pass);
    if (ret != SCAN_AP_FOUND) {
        omnicfg_redo();

        if ((ret == SCAN_AP_PASS_ERROR) || (ret == SCAN_AP_SEC_ERROR)) {
            // AP_PASS_ERROR or AP_SEC_ERROR
            cdb_set_int("$omi_result", OMNICFG_R_PASS_ERROR);
            MYLOG("password check error");
        }
        else if (ret != SCAN_AP_FOUND) {
            // AP_NOT_FOUND
            cdb_set_int("$omi_result", OMNICFG_R_AP_NOT_FOUND);
            MYLOG("Can not found AP");
        }

        return 0;
    }

#endif
    MYLOG("Found AP");

    if(omnicfg_shall_exit())
        return 0;

    if (bssid) {
        cdb_set("$wl_bss_bssid2", bssid);
    }

#ifdef SPEEDUP_WLAN_CONF
    cdb_set_int("$omi_result", OMNICFG_R_CONNECTING);
    omnicfg_voice("connecting");
    cdb_set_int("$omicfg_sniffer_enable", 0);
#if !defined(__PANTHER__)
    omnicfg_hostapd_reload();
#endif
    exec_cmd("mtc wlhup");
#else
    omnicfg_simply_commit();
#endif

    if (omnicfg_check() == 0) {
        MYLOG("config done success");
        omnicfg_syslogd(OMNICFG_LOG_STOP);

        if(omnicfg_shall_exit())
        {
            if(cdb_get_int("$wl_aplist_en", 0) == 1)
                exec_cmd("aplist r 1");
            return -1;
        }

        mtdm_client_simply_call(OPC_CMD_SAVE, 0, NULL);

        if (is_directmode()) {
            omicfg_wait_directmode_timeout();
            omicfg_set_directmode_route(0);
            #ifndef SPEEDUP_WLAN_CONF
            if ((mode == WORK_MODE_P2P) || (mode == WORK_MODE_CLIENT)) {
                MYLOG("disable br0");
                killall("hostapd", SIGTERM);
                brctl_delif("br0", "eth0.3");
                ifconfig("br0", 0, NULL, NULL);
                brctl_delbr("br0");
            }
            #endif
        }

        cdb_set_int("$omi_config_mode", OMNICFG_MODE_DISABLE);
        omnicfg_reload(1);
        cdb_set_int("$omi_result", OMNICFG_R_SUCCESS);
        //omnicfg_voice("connected");
    }

    if(omnicfg_shall_exit())
    {
        if(cdb_get_int("$wl_aplist_en", 0) == 1)
            exec_cmd("aplist r 1");
        return -1;
    }

    return 0;
}

int wdk_omnicfg(int argc, char **argv)
{
    int ret = -1;

    if ((argc == 1) || (argc == 2)) {
        MYLOG("ARG0=%s.ARG1=%s", argv[0], argv[1]);
        if (!strcmp(argv[0], "done")) {
#if defined(OMNICFG_IPC_LOCK)
            if(0 > omnicfg_lock())
                return -1;
            ret = omnicfg_config();
            omnicfg_unlock();
#else
            if (omnicfg_save_pid()) {
                MYLOG("Skip, %s is running!\n", OMNICFG_BIN);
                return ret;
            }
            ret = omnicfg_config();
            omnicfg_remove_pid();
#endif
        }
        else if (!strcmp(argv[0], "sniffer_timeout")) {
            ret = omnicfg_syslogd(OMNICFG_LOG_BACKUP);
            ret = omnicfg_syslogd(OMNICFG_LOG_START);
        }
        else if (!strcmp(argv[0], "timeout")) {
            ret = omnicfg_restore();
        }
        else if (!strcmp(argv[0], "remove_log")) {
            ret = omnicfg_syslogd(OMNICFG_LOG_CLEAN);
        }
        else if (!strcmp(argv[0], "start_log")) {
            ret = omnicfg_syslogd(OMNICFG_LOG_START);
        }
        else if (!strcmp(argv[0], "stop_log")) {
            ret = omnicfg_syslogd(OMNICFG_LOG_STOP);
        }
        else if (!strcmp(argv[0], "reload")) {
            ret = omnicfg_reload(0);
        }
        else if (!strcmp(argv[0], "stop")) {
            ret = omnicfg_stop();
        }
        else if (!strcmp(argv[0], "state")) {
            ret = omnicfg_state();
        }
        else if (!strcmp(argv[0], "zero_counter")) {
            ret = cdb_set_int("$omi_time_to_kill_ap", 0);
            // send signal to notify omnicfg don't wait 
        }
        else if (!strcmp(argv[0], "voice") && (argc == 2)) {
            ret = omnicfg_voice(argv[1]);
        }
        else {
            MYLOG("ARG0 unknown!!");
        }
    }

    return ret;
}

