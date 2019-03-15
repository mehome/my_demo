#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define UHTTPD_NAME     "uhttpd"
#define UHTTPD_CMDS     "/usr/sbin/"UHTTPD_NAME
#if defined(__PANTHER__)
#define UHTTPD_KEY      "/etc/uhttpd.key"
#define UHTTPD_CERT     "/etc/uhttpd.crt"
#else
#define UHTTPD_KEY      "/var/uhttpd.key"
#define UHTTPD_CERT     "/var/uhttpd.crt"
#endif
#define UHTTPD_CONFILE  "/tmp/httpd.conf"
#define UHTTPD_PIDFILE  "/var/run/"UHTTPD_NAME".pid"
#define UHTTPD_ARGS     "-f -c "UHTTPD_CONFILE" -h /www -p 0.0.0.0:80"
#define UHTTPSD_ARGS    UHTTPD_ARGS" -K "UHTTPD_KEY" -C "UHTTPD_CERT" -s 0.0.0.0:8443"

#define UHTTPD_TLSFILE  "/usr/lib/uhttpd_tls.so"
#define PX5G_BIN        "/usr/sbin/px5g"

static int FUNCTION_IS_NOT_USED uhttpd_generate_keys(void)
{
/*
    local days=730
    local bits=1024
    local country="DE"
    local state="Berlin"
    local location="Berlin"
    local commonname="OpenWrt"

    [ -x "$PX5G_BIN" ] && {
        $PX5G_BIN selfsigned -der \
            -days ${days:-730} -newkey rsa:${bits:-1024} -keyout "$UHTTPD_KEY" -out "$UHTTPD_CERT" \
            -subj /C=${country:-DE}/ST=${state:-Saxony}/L=${location:-Leipzig}/CN=${commonname:-OpenWrt}
    } || {
        echo "WARNING: the specified certificate and key" \
            "files do not exist and the px5g generator" \
            "is not available, skipping SSL setup."
    }
 */

    if( f_exists(PX5G_BIN) ) {
#if defined(__PANTHER__)
        exec_cmd2(PX5G_BIN" selfsigned -der -days 3660 -newkey rsa:1024 -keyout "UHTTPD_KEY" -out "UHTTPD_CERT
                  " -subj /C=CN/ST=Shanghai/L=Shanghai/CN=PantherSDK");
#else
        exec_cmd2(PX5G_BIN" selfsigned -der -days 730 -newkey rsa:1024 -keyout "UHTTPD_KEY" -out "UHTTPD_CERT
                  " -subj /C=DE/ST=Berlin/L=Berlin/CN=OpenWrt");
#endif
    }
    else {
        MTDM_LOGGER(this, LOG_ERR, "WARNING: the specified certificate and key");
        MTDM_LOGGER(this, LOG_ERR, "files do not exist and the px5g generator");
        MTDM_LOGGER(this, LOG_ERR, "is not available, skipping SSL setup.");
    }

    return 0;
}

static int start(void)
{
    FILE *fp;
    char args[MSBUF_LEN] = UHTTPD_ARGS;
    char sysuser[MSBUF_LEN];
    char *argv[10];
    char *user, *pass;

    if( f_exists(UHTTPD_TLSFILE) ) {
#if defined(__PANTHER__)
        if(f_exists_and_not_zero_byte(UHTTPD_KEY)
            && f_exists_and_not_zero_byte(UHTTPD_CERT))
        {
            /* use existing keys */
        }
        else
        {
            unlink(UHTTPD_KEY);
            unlink(UHTTPD_CERT);
            uhttpd_generate_keys();
        }
#else
        if( !f_exists(UHTTPD_KEY) && !f_exists(UHTTPD_CERT) ) {
            uhttpd_generate_keys();
        }
#endif
        if( f_exists(UHTTPD_KEY) && f_exists(UHTTPD_CERT) ) {
            snprintf(args, sizeof(args), "%s", UHTTPSD_ARGS);
        }
    }

    cdb_get_str("$sys_user1", sysuser, sizeof(sysuser), NULL);
    str2argv(sysuser, argv, '&');
    user = str_arg(argv, "user=");
    pass = str_arg(argv, "pass=");
    if (!user || !pass) {
        MTDM_LOGGER(this, LOG_ERR, "invalid $sys_user1=[%s]", sysuser);
        return 0;
    }
    fp = fopen(UHTTPD_CONFILE, "w");
    if (fp) {
        fprintf(fp, "/:%s:%s\n", user, pass);
        fclose(fp);
    }

    start_stop_daemon(UHTTPD_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, UHTTPD_PIDFILE,
        "%s", args);

    return 0;
}

static int stop(void)
{
    if( f_exists(UHTTPD_PIDFILE) ) {
        start_stop_daemon(UHTTPD_CMDS, SSDF_STOP_DAEMON, 0, SIGTERM, UHTTPD_PIDFILE, NULL);
        unlink(UHTTPD_PIDFILE);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int uhttpd_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

