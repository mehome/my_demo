
#include "wdk.h"

static void setenv_keep_alive(char *file)
{
    FILE *fp;
    char buf[512];

    fp = fopen(file, "r");
    if (fp) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if (sscanf(buf, "export WDKUPGRADE_KEEP_ALIVE=%s", buf) != -1) {
                LOG("export WDKUPGRADE_KEEP_ALIVE=%s", buf);
                setenv("WDKUPGRADE_KEEP_ALIVE", buf, 1);
                break;
            }
        }
        fclose(fp);
    }
}

static int do_largepost()
{
    char alive_list[200] = { 0 };
    char *WDKUPGRADE_KEEP_ALIVE = NULL;

    if (f_exists("/lib/wdk/sysupgrade_before")) {
        setenv_keep_alive("/lib/wdk/sysupgrade_before");
    }

    /*
    	Get the list of program that we dont want to kill to free memory
    	WDKUPGRADE_KEEP_ALIVE environment variable is also needed

    	also we need to put http(myself) alive to drop cache
    */

    strcat(alive_list, "procd\\|ubusd\\|askfirst\\|uhttpd\\|wpa_supplicant\\|hostapd\\|ash\\|watchdog\\|omnicfg_bc\\|ota\\|telnetd\\|http\\|ps\\|$$");

    if ((WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE")) != NULL) {
        char *argv[10];
        int num = str2argv(WDKUPGRADE_KEEP_ALIVE, argv, ' ');
        while (num-- > 0) {
            strcat(alive_list, "\\|");
            strcat(alive_list, argv[num]);
        }
    }

    LOG("Alive list = %s", alive_list);
    exec_cmd2("ps | grep -v '%s' | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill > /dev/null 2>&1", alive_list);

    /*
    root@router:/lib/wdk# free
                  total         used         free       shared      buffers
    Mem:          28292        17500        10792            0         1276
    Swap:             0            0            0
    Total:        28292        17500        10792
    root@router:/lib/wdk# echo 3 > /proc/sys/vm/drop_caches
    root@router:/lib/wdk# free
    			  total         used         free       shared      buffers
    Mem:          28292        14764        13528            0          312
    Swap:             0            0            0
    Total:        28292        14764        13528
    */
    writeline("/proc/sys/vm/drop_caches", "1");
    sync();

    return 0;
}

int wdk_http(int argc, char **argv)
{
    if (argc != 1)
        return -1;

    if (strcmp(argv[0], "largepost") == 0) {
        do_largepost();
    } else if (strcmp(argv[0], "peerip") == 0) {
        /* echo peerip, the env value is from uhttpd */
        char *value = getenv("REMOTE_ADDR");
        if (NULL != value) {
            LOG("REMOTE IP=%s", value);
            STDOUT("%s\n", value);
        }
    } else if (strcmp(argv[0], "peermac") == 0) {
        char *value = getenv("REMOTE_ADDR");
        char mac[100] = {0};
        if (NULL != value) {
            LOG("REMOTE IP=%s", value);
            exec_cmd3(mac, sizeof(mac), "cat /proc/net/arp | grep \"%s\" | awk '{print $4}'", value);
            if (strlen(mac) > 0)
                STDOUT("%s\n", mac);
        }
    } else {
        return -1;
    }
    return 0;
}


