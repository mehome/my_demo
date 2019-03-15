#include "include/mtdm_client.h"
#include "wdk.h"

int wdk_save(int argc, char **argv)
{
    return mtdm_client_simply_call(OPC_CMD_SAVE, 0, NULL);
}

