#include "include/mtdm_client.h"
#include "wdk.h"


int wdk_wan(int argc, char **argv)
{
    if (argc == 0) {
        LOG("usage:/lib/wdk/wan [connect/release]\n");
        return 0;
    } else if(argc == 1) {

        putenv("manual_onoff=1");

        if (strcmp("connect", argv[0]) == 0) {
            mtdm_client_simply_call(OPC_CMD_WAN, OPC_CMD_ARG_STOP, NULL);
            mtdm_client_simply_call(OPC_CMD_WAN, OPC_CMD_ARG_START, NULL);
        } else if (strcmp("release", argv[0]) == 0)  {
            mtdm_client_simply_call(OPC_CMD_WAN, OPC_CMD_ARG_STOP, NULL);
        } else {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}





