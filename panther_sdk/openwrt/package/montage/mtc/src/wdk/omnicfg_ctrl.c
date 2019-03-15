#include <ctype.h>
#include "wdk.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_client.h"

#include <libutils/ipc.h>

#define OMNICFG_NAME    "omnicfg"
#define OMNICFG_BIN     "/lib/wdk/"OMNICFG_NAME

#define OMNICFG_APPLY_NAME    "omnicfg_apply"
#define OMNICFG_APPLY_BIN     "/lib/wdk/"OMNICFG_APPLY_NAME

#define OMNICFG_COUNT_FILE    "/tmp/run/omni_count"

#define MYLOG(fmt, args...)  do { \
    logger(LOG_INFO, "[omnicfg_ctrl]:[" fmt "]", ## args); \
    LOG("[omnicfg_ctrl]:[" fmt "]", ## args); \
} while(0)

#ifdef SPEEDUP_WLAN_CONF
#define UNKNOWN (-1)
#define HOSTAPD_PIDFILE        "/var/run/hostapd.pid"
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
#endif

static int omnicfg_trigger(void)
{
    int omicfg_sniffer_enable = cdb_get_int("$omicfg_sniffer_enable", 0);
    int omi_config_mode       = cdb_get_int("$omi_config_mode", 0);
    int omicfg_log_mode       = cdb_get_int("$omicfg_log_mode", 0);
    int omicfg_group_cmd_en   = cdb_get_int("$omicfg_group_cmd_en", 0);
    int omicfg_count = 0;
    FILE *fp = NULL;

    exec_cmd2(OMNICFG_BIN" voice trigger");

    if (omicfg_sniffer_enable) {
        exec_cmd2("wd smrtcfg stop");
    }

#if !defined(OMNICFG_IPC_LOCK)
    killall(OMNICFG_NAME, SIGTERM);
    killall(OMNICFG_APPLY_NAME, SIGTERM);
#endif

    if (omicfg_log_mode) {
        killall("syslogd", SIGTERM);
    }

    if(omnicfg_shall_exit())
        return 0;

    if ((omicfg_sniffer_enable == 0) && (omi_config_mode == OMNICFG_MODE_DISABLE)) {
        omnicfg_cdb_save();
    }

    cdb_set_int("$op_work_mode", 1);
    cdb_set_int("$omicfg_config_enable", 1);
    cdb_set_int("$omi_result", OMNICFG_R_UNKNOWN);
    cdb_set_int("$omi_restore_apply", 0);

#ifdef SPEEDUP_WLAN_CONF
    exec_cmd("mtc wlhup");
#endif
    /*
     * cdb: omicfg_group_cmd_en to determine「一鍵配置的DUT」or 「較舊版本的DUT」
     */
    if (omicfg_group_cmd_en > 0) {
        cdb_set_int("$omi_config_mode", OMNICFG_MODE_SNIFFER);
        cdb_set_int("$omicfg_sniffer_enable", 1);
    }

    if(omnicfg_shall_exit())
        return 0;

    start_stop_daemon(OMNICFG_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, "start_log");

    if (f_exists(OMNICFG_COUNT_FILE)) {
        if ((fp = fopen(OMNICFG_COUNT_FILE, "r")) != NULL) {
            if (fscanf(fp, "%d", &omicfg_count) != 1) {
                omicfg_count = 0;
            }
            fclose(fp);
        }
    }
    if ((fp = fopen(OMNICFG_COUNT_FILE, "w")) != NULL) {
        fprintf(fp, "%d\n", ++omicfg_count);
        fclose(fp);
    }
    logger (LOG_INFO, "[omnicfg_ctrl]trigger OMNI_COUNT=%d", omicfg_count);

    if(omnicfg_shall_exit())
        return 0;

#ifndef SPEEDUP_WLAN_CONF
    omnicfg_simply_commit();
#else
    mtdm_client_simply_call(OPC_CMD_OCFG, OPC_CMD_ARG_STOP, NULL);
#if defined(__PANTHER__)

    if(omnicfg_shall_exit())
        return 0;

    exec_cmd2("wd smrtcfg start");
#else
    omnicfg_hostapd_reload();
#endif
#endif

    return 0;
}

static int omnicfg_query(void)
{
    int omi_result      = cdb_get_int("$omi_result", 0);
    int omicfg_enable   = cdb_get_int("$omicfg_enable", 0);
    int omi_config_mode = cdb_get_int("$omi_config_mode", 0);
    int op_work_mode    = cdb_get_int("$op_work_mode", 0);
    int omi_work_mode   = cdb_get_int("$omi_work_mode", 0);
    FILE *fp = NULL;

    if ((fp = fopen("/tmp/omnicfg_status", "w")) != NULL) {
        fprintf(fp, "%d %d %d %d %d\n", 
            omi_result, omicfg_enable, omi_config_mode, op_work_mode, omi_work_mode);
        fclose(fp);
    }
    printf("%d %d %d %d %d\n", omi_result, omicfg_enable, omi_config_mode, op_work_mode, omi_work_mode);

    return 0;
}

int wdk_omnicfg_ctrl(int argc, char **argv)
{
    int ret = -1;

    if (argc == 1) {
        MYLOG("ARG0=%s", argv[0]);
        if (!strcmp(argv[0], "trigger")) {
#if defined(OMNICFG_IPC_LOCK)
            if(0 > omnicfg_lock())
                return -1;

            ret = omnicfg_trigger();

            omnicfg_unlock();
#else
            ret = omnicfg_trigger();
#endif

        } else if (!strcmp(argv[0], "query")) {
            ret = omnicfg_query();
        }
    }

    return ret;
}

