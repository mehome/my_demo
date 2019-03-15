#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"
#include "misc.h"

extern MtdmData *mtdm;

int system_update_hosts(void)
{
    char host_name[SSBUF_LEN] = {0};
    char sys_name[SSBUF_LEN];
    char ip[30];

    if (cdb_get_str("$sys_name", sys_name, sizeof(sys_name), NULL) != NULL) {
        if (gethostname(host_name, sizeof(host_name) - 1) == 0) {
            if (strcmp(host_name, sys_name)) {
                sethostname(sys_name, strlen(sys_name));
                if (strcmp(host_name, "(none)")) {
                    // it is not in boot process now
                    // hostname is changed, need to update /etc/hosts
                    f_write_string("/etc/hosts", "127.0.0.1 localhost", FW_NEWLINE, 0);
                    get_ifname_ether_ip(mtdm->rule.LAN, ip, sizeof(ip));
                    f_write_sprintf("/etc/hosts", FW_APPEND | FW_NEWLINE, 0, "%s %s", ip, sys_name);
                }
            }
        }
    }

    return 0;
}


