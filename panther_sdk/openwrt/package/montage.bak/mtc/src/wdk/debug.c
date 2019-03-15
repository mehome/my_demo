
#include "wdk.h"




int wdk_debug(int argc, char **argv)
{
    int i = 0;
    char cmd[100] = {0};
    for (i = 0; i < argc; i++) {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    STDOUT("cmd=%s\n", cmd);
    if (strlen(cmd) > 0)
        print_shell_result(cmd);

    return 0;
}

