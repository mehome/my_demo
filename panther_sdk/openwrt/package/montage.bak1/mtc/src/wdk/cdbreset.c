#include "wdk.h"


int wdk_cdbreset(int argc, char **argv)
{
    if (argc != 0)
        return -1;

    if (f_exists("/tmp/dnsmasq.conf")) {
        if (remove("/tmp/dnsmasq.conf") == 0)
            LOG("remove /tmp/dnsmasq.conf success");
        else
            LOG("remove /tmp/dnsmasq.conf fail");
    }

    cdb_reset();

    return 0;
}
