#include <ctype.h>
#include "include/mtdm_client.h"
#include "wdk.h"

#include <libutils/ipc.h>

#define OMNICFG_NAME    "omnicfg"
#define OMNICFG_BIN     "/lib/wdk/"OMNICFG_NAME

#define OMNICFG_APPLY_NAME    "omnicfg_apply"
#define OMNICFG_APPLY_BIN     "/lib/wdk/"OMNICFG_APPLY_NAME
#define OMNICFG_APPLY_PIDFILE "/var/run/"OMNICFG_APPLY_NAME".pid"

#define OMNICFG_COUNT_FILE    "/tmp/run/omni_count"

#define MYLOG(fmt, args...)  do { \
    logger(LOG_INFO, "[omnicfg_apply]:[" fmt "]", ## args); \
    LOG("[omnicfg_apply]:[" fmt "]", ## args); \
} while(0)

static int omnicfg_terminate_apply(void)
{
    MYLOG("cgi_apply to terminate");
    cdb_set_int("$omi_time_to_kill_ap", 0);
    return 0;
}

static int omnicfg_apply(char *mode, char *ssid, char *pass, int from_cgi)
{
    int test = cdb_get_int("$omi_result", OMNICFG_R_SUCCESS);
    int omicfg_sniffer_enable = cdb_get_int("$omicfg_sniffer_enable", 0);

    char *addr = getenv("REMOTE_ADDR");

#if !defined(OMNICFG_IPC_LOCK)
    if (test == OMNICFG_R_CONNECTING) {
        MYLOG("IGNORE: omnicfg apply is ongoing!!");
        return 0;
    }
#endif

    MYLOG("apply to MODE(%s) SSID(%s) PASS(%s)", mode, ssid, pass);

    if (sscanf(mode, "%d", &test) != 1) {
        MYLOG("IGNORE: Please assign Work Mode!!");
        return 0;
    }
    else if ((test != WORK_MODE_WISP) && (test != WORK_MODE_RP) &&
             (test != WORK_MODE_BR) && (test != WORK_MODE_CLIENT)) {
        MYLOG("IGNORE: Please assign correct Work Mode!!");
        return 0;
    }

    if (strlen(ssid) <= 0) {
        MYLOG("IGNORE: Please assign which AP(SSID) to connect!!");
        return 0;
    }

    if(omnicfg_shall_exit())
        return 0;

    if (omicfg_sniffer_enable)
    {
        exec_cmd2("wd smrtcfg stop");
    }

    if(omnicfg_shall_exit())
        return 0;

    omnicfg_cdb_save();

    cdb_set_int("$omi_result", OMNICFG_R_CONNECTING);

    /* build environment */
    clearenv();

    /* common information */
    setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin:/lib/wdk", 1);
    setenv("MODE", mode, 1);
    setenv("SSID", ssid, 1);
    if(pass) {
        setenv("PASS", pass, 1);
    }
    setenv("NAME", "", 1);
    setenv("BSSID", "", 1);
    if (from_cgi && addr) {
        setenv("REMOTE_ADDR", addr, 1);
        setenv("FROM_CGI", "1", 1);
    }
    else {
        setenv("FROM_CGI", "0", 1);
    }

    if(omnicfg_shall_exit())
        return 0;

    start_stop_daemon(OMNICFG_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, "done");

    return 0;
}

/*
 * cat /tmp/ios_apply_encode 
 * begin-base64 644 /tmp/ios_apply_decode
 * c3NpZD1tZWV0aW5nXzMKcGFzcz0xMjM0NTY3MApwaGFzZT1NT05UQUdF
 * ====
 *
 * cat /tmp/ios_apply_decode      
 * ssid=meeting_3
 * pass=12345670
 *
 */
static int omnicfg_cgi_apply(char *mode, char *endata)
{
    FILE *fp = NULL;
    char buf[MSBUF_LEN];
    char *ssid = NULL, *pass = NULL, *phase = NULL;
    char *ptr;
    int ret = -1;

    if (endata && (strlen(endata) > 0)) {
        MYLOG("cgi_apply to MODE(%s) BASE64(%s)", mode, endata);
    }
    else {
        omnicfg_terminate_apply();
        goto err;
    }

    if ((fp = fopen("/tmp/ios_apply_encode", "w")) != NULL) {
        fprintf(fp, "begin-base64 644 /tmp/ios_apply_decode\n");
        fprintf(fp, "%s\n", endata);
        fprintf(fp, "====\n");
        fclose(fp);
    }
    else {
        omnicfg_terminate_apply();
        goto err;
    }

    if(omnicfg_shall_exit())
        goto err;

    exec_cmd2("uudecode /tmp/ios_apply_encode");

    fp = fopen("/tmp/ios_apply_decode", "r");
    if (fp) {
        while ((ptr = new_fgets(buf, sizeof(buf), fp)) != NULL) {
            if (!strncmp(ptr, "ssid=", sizeof("ssid=")-1)) {
                ptr += (sizeof("ssid=")-1);
                ssid = strdup(ptr);
            }
            else if (!strncmp(ptr, "pass=", sizeof("pass=")-1)) {
                ptr += (sizeof("pass=")-1);
                pass = strdup(ptr);
            }
            else if (!strncmp(ptr, "phase=", sizeof("phase=")-1)) {
                ptr += (sizeof("phase=")-1);
                phase = strdup(ptr);
            }
        }
        fclose(fp);
    }
    else {
        omnicfg_terminate_apply();
        goto err;
    }
    unlink("/tmp/ios_apply_encode");
    unlink("/tmp/ios_apply_decode");
    if (!phase) {
        goto err;
    }
    MYLOG("check vendor phase (%s)", phase);
    if ((cdb_get_str("$sw_omnicfg_vendor_key", buf, sizeof(buf), NULL) != NULL) &&
        (strcmp(buf, ""))) {
        if (strcmp(phase, buf)) {
            goto err;
        }
    }
    else {
        if (strcmp(phase, "MONTAGE")) {
            goto err;
        }
    }

    cdb_set_int("$omi_time_to_kill_ap", 60);

    if(omnicfg_shall_exit())
        goto err;

    ret = omnicfg_apply(mode, ssid, pass, 1);

err:
    if (ssid) {
        free(ssid);
    }
    if (pass) {
        free(pass);
    }
    if (phase) {
        free(phase);
    }
    if (ret != 0) {
        omnicfg_terminate_apply();
    }
    return ret;
}

#if !defined(OMNICFG_IPC_LOCK)
int omnicfg_apply_save_pid(void)
{
    FILE *pidfile;

    mkdir("/var/run", 0755);
    if (f_exists(OMNICFG_APPLY_PIDFILE)) {
        if ((get_pid_by_pidfile(OMNICFG_APPLY_PIDFILE, OMNICFG_APPLY_NAME) != -1) || 
            (get_pid_by_pidfile(OMNICFG_APPLY_PIDFILE, OMNICFG_APPLY_BIN) != -1)) {
            return -1;
        }
    }

    if ((pidfile = fopen(OMNICFG_APPLY_PIDFILE, "w")) != NULL) {
        fprintf(pidfile, "%d\n", getpid());
        (void) fclose(pidfile);
    }

    return 0;
}

int omnicfg_apply_remove_pid(void)
{
    if (f_exists(OMNICFG_APPLY_PIDFILE)) {
        unlink(OMNICFG_APPLY_PIDFILE);
    }

    return 0;
}
#endif

int wdk_omnicfg_apply(int argc, char **argv)
{
    int ret = -1;
    if ((argc == 2) || (argc == 3)) {
#if defined(OMNICFG_IPC_LOCK)
        if(0 > omnicfg_lock())
            return -1;
#else
        if (omnicfg_apply_save_pid()) {
            MYLOG("Skip, %s is running!", OMNICFG_APPLY_BIN);
            return ret;
        }
#endif

        MYLOG("ARG0=%s.ARG1=%s.ARG2=%s", argv[0], argv[1], argv[2]);
        if (!strcmp(argv[0], "base64")) {
            ret = omnicfg_cgi_apply(argv[1], argv[2]);
        }
        else {
            ret = omnicfg_apply(argv[0], argv[1], argv[2], 0);
        }
#if defined(OMNICFG_IPC_LOCK)
        omnicfg_unlock();
#else
        omnicfg_apply_remove_pid();
#endif
    }

    return ret;
}

