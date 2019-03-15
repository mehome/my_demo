#include <function/common/px_timer.h>
#include <function/log/slog.h>

#include <platform.h>
#include <libcchip.h>
#include <pthread.h>
#include <time.h>
//#define IF311 1
#define ASR_FORMAT		SND_PCM_FORMAT_S16_LE
#define ASR_SPEED		24000
#define ASR_CHANNELS	2

char* get_full_process_name(pid_t pid)
{
    size_t linknamelen;
    char file[256];
    static char cmdline[256] = {0};

    sprintf(file, "/proc/%d/exe", pid);
    linknamelen = readlink(file, cmdline, sizeof(cmdline) / sizeof(*cmdline) - 1);
    cmdline[linknamelen + 1] = 0;
    return cmdline;
}



void * test_pthread1()
{
	int ent = 10;
	//StartRecord();
}



void * test_pthread2()
{

	sleep(1);
	int ent ;
	printf("WAITTING:\n");
	scanf("%d", &ent);
	//StopRecord();
	printf("stop\n");
}

//http://10.0.0.116/music/test.mp3
#if 0
int main(int argc,char *argv[])
{
	int count=0, i = 0;
	apInfList *aplist = NULL;
	while(1){
	count = get_aplist(&aplist);
	for(i = 0; i < count; i++){
		printf("aplist[%d]:%s\n", i, aplist[i].SSID);
	}
	free(aplist);
	sleep(1);
	}
	return 0;
}
#endif


// by mars hu
typedef struct __data_proc_t {
    const char *name;
    int (*exec_proc)(char *cmd[], int parma);
} data_proc_t;
extern char * search_connect_router_ssid(void);

//extern void OnProcessMediaControl(char *pCmd, int iParma);



int device_info() {
	int i = 0;
	DEVICEINFO *aplist = NULL;
	aplist = (DEVICEINFO *)calloc(1, sizeof(DEVICEINFO));
	int count = get_device_info(aplist);
	
	printf("sys_language [%d]:%s\n", i, aplist->sys_language);
	printf("sup_language[%d]:%s\n", i, aplist->sup_language);
	printf("firmware[%d]:%s\n", i, aplist->firmware);
	printf("release[%d]:%s\n", i, aplist->release);
	printf("mac_addr[%d]:%s\n", i, aplist->mac_addr);
	printf("uuid[%d]:%s\n", i, aplist->uuid);
	printf("project[%d]:%s\n", i, aplist->project);
	printf("battery_level[%d]:%s\n", i, aplist->battery_level);
	printf("function_support[%d]:%s\n", i, aplist->function_support);
	printf("bt_ver[%d]:%s\n", i, aplist->bt_ver);
	printf("key_num[%d]:%s\n", i, aplist->key_num);
	printf("charge_plug[%d]:%s\n", i, aplist->charge_plug);
	printf("ssid[%d]:%s\n", i, aplist->audioname);
	printf("project_uuid[%d]:%s\n", i, aplist->project_uuid);

	free(aplist);

	return 0;
}

int get_connectstatuc() {
	printf("current wifi connet status:%d\n", get_wifi_ConnectStatus());
}

static  data_proc_t cmd_tbl[] = {
	//{"MediaPlayerControl", OnProcessMediaControl},
	{"search_wifi_ssid", search_connect_router_ssid},
	{"deviceinfo", device_info},
	{"setdevicessid", set_device_ssid},
	{"getconnectstatus", get_connectstatuc},
	{"playmusic", play_test_music},
};

// by mars hu
int main(int argc,char *argv[]) {
	int i;
	int iRet;
	int iParam = 0;

	/*int pro = get_wifi_bt_download_progress();
	printf("pro = %d",pro);
	if (argc != 1) {
		int iLength = sizeof(cmd_tbl) / sizeof(data_proc_t);
		for (i = 0; i < iLength; i++) {
			if (0 == strncmp(argv[1], cmd_tbl[i].name, strlen(cmd_tbl[i].name))) {
				if (cmd_tbl[i].exec_proc != NULL) {
					if (4 == argc) {
						iParam = atoi(argv[3]);
					}
					iRet = cmd_tbl[i].exec_proc(argv[2], iParam);
				}
				break;
			}
		}
	}*/

	return iRet;
}


