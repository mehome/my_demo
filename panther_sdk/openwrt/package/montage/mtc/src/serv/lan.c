#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

static int do_set_arp_lan_bind(void)
{
    char sbuf[USBUF_LEN];
    char entry[MSBUF_LEN];
	char *argv[10];
    char *ip, *mac;
	int num, i, en;

    en = cdb_get_int("$lan_arp_sp", 0);

    if (en != 0) {
        for (i=0;i<20;i++) {
            sprintf(sbuf, "$lan_arp_entry%d", i);
            if (cdb_get_str(sbuf, entry, sizeof(entry), NULL) == NULL)
                break;
            num = str2argv(entry, argv, '&');
            if(num < 4)
                break;
            if(!str_arg(argv, "en=")||!str_arg(argv, "host=")||!str_arg(argv, "ip=")||!str_arg(argv, "mac="))
                continue;
            en = atoi(str_arg(argv, "en="));
            if (en) {
                mac = str_arg(argv, "mac=");
                ip = str_arg(argv, "ip=");
                exec_cmd2("arp -s %s %s > /dev/null 2>&1", ip, mac);
            }
        }
    }

    return 0;
}

static int do_clear_arp_perm(void)
{
    char cmd[MAX_COMMAND_LEN];
    void *myhdr = NULL;
    char *myptr = NULL;
    int myfree = 0;

    sprintf(cmd, "arp -na | awk '/PERM on %s/{gsub(/[()]/,\"\",$2); print $2 }'", mtdm->rule.LAN);
    while (!exec_cmd4(&myhdr, &myptr, myfree, cmd)) {
        if (myptr) {
            exec_cmd2("arp -d %s > /dev/null 2>&1", myptr);
        }
    }

    return 0;
}

static int start(void)
{
    char ipaddr[PBUF_LEN];
    char ntmask[PBUF_LEN];

    cdb_get("$lan_ip", ipaddr);
    cdb_get("$lan_msk", ntmask);

    config_loopback();
    if ((mtdm->rule.OMODE != opMode_p2p) && (mtdm->rule.OMODE != opMode_client)) {
        ifconfig("eth0", IFUP, NULL, NULL);
    }
    ifconfig(mtdm->rule.LAN, IFUP, ipaddr, ntmask);

    do_set_arp_lan_bind();

    return 0;
}

static int stop(void)
{
    do_clear_arp_perm();
    ifconfig("lo", IFDOWN, "0.0.0.0", NULL);
    ifconfig(mtdm->rule.LAN, IFDOWN, "0.0.0.0", NULL);

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int lan_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

