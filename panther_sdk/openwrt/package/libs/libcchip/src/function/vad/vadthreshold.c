#include <stdio.h>
#include "capture.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include <sys/wait.h>  

char g_byWakeUpFlag = 0;

int g_iVadThreshold = 1000000;

#define VAD_THRESHOLD_ON 		"vadthreshold_on"
#define VAD_THRESHOLD_OFF 		"vadthreshold_off"

int myexec_cmd(const char *cmd)
{
    char buf[128];
    FILE *pfile;
    int status = -2;

    if ((pfile = popen(cmd, "r"))) {
        fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
        while(!feof(pfile)) {
            fgets(buf, sizeof buf, pfile);
        }
        status = pclose(pfile);
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

//int vadthreshold(int argc, char* argv[])
int vadthreshold(int value, char *pcardname)
{
		if (0 == strcmp("default", pcardname))
		{
			g_byWakeUpFlag = 0;
		}
		else if (0 == strcmp("plughw:1,0", pcardname))
		{
			g_byWakeUpFlag = 1;
		}
		else
		{
			g_byWakeUpFlag = 0;
		}

		char vstring[16] = {0};
		g_iVadThreshold = value;

		DEBUG_PRINTF("g_byWakeUpFlag:%d, g_iVadThreshold:%d", g_byWakeUpFlag, g_iVadThreshold);

		myexec_cmd("uartdfifo.sh " VAD_THRESHOLD_ON);

		sprintf(vstring, "%d", value);
		printf("value: %s\n", vstring);
		capture_voice(vstring);

		myexec_cmd("uartdfifo.sh " VAD_THRESHOLD_OFF);
}
