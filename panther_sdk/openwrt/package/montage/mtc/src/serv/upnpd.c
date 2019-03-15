#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define UPNPD_NAME       "miniupnpd"
#define UPNPD_CMDS       "/usr/bin/"UPNPD_NAME
#define UPNPD_ARGS       "-l"
#define UPNPD_PIDFILE    "/var/run/"UPNPD_NAME".pid"

#define IPTABLES_BIN     "iptables"

static int FUNCTION_IS_NOT_USED upnpd_setup(void)
{
    exec_cmd2(IPTABLES_BIN" -t nat -N MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -t nat -A PREROUTING -j MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -N MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -A FORWARD -j MINIUPNPD");
    return 0;
}

static int FUNCTION_IS_NOT_USED upnpd_clear(void)
{
	exec_cmd2(IPTABLES_BIN" -t nat -F MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -t nat -D PREROUTING -j MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -t nat -X MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -F MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -D FORWARD -j MINIUPNPD");
	exec_cmd2(IPTABLES_BIN" -X MINIUPNPD");
    return 0;
}

static int FUNCTION_IS_NOT_USED upnpd_start(void)
{
    char args[SSBUF_LEN];
    int upnp_enable = cdb_get_int("$upnp_enable", 0);
    int upnp_adv_time = cdb_get_int("$upnp_adv_time", 0);
    char lan_ip[PBUF_LEN];

    if (upnp_enable && (cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL) != NULL)) {
        upnpd_setup();

        snprintf(args, sizeof(args), "-i %s -a %s -t %d %s", mtdm->rule.WAN, lan_ip, upnp_adv_time, UPNPD_ARGS);
        start_stop_daemon(UPNPD_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, UPNPD_PIDFILE, 
            "%s", args);
    }

    return 0;
}

static int FUNCTION_IS_NOT_USED upnpd_stop(void)
{
    if( f_exists(UPNPD_PIDFILE) ) {
        start_stop_daemon(UPNPD_CMDS, SSDF_STOP_DAEMON, 0, SIGTERM, UPNPD_PIDFILE, NULL);
        unlink(UPNPD_PIDFILE);

        upnpd_clear();
    }
    return 0;
}

static int start(void)
{
    upnpd_stop();
    return upnpd_start();
}

static int stop(void)
{
    return upnpd_stop();
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int upnpd_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

