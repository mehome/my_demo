#include "include/mtdm_client.h"
#include "wdk.h"

int wdk_commit(int argc, char **argv)
{
    return mtdm_client_simply_call(OPC_CMD_COMMIT, 0, NULL);
}

