#include <limits.h>
#include "wdk.h"

int g_log_on;



int find_pid_by_name( char* proc_name, int* foundpid)
{
    DIR             *dir = NULL;
    struct dirent   *d = NULL;
    int             pid = 0;
    int i = 0;
    char  *s = NULL;
    int pnlen;

    foundpid[0] = 0;
    pnlen = strlen(proc_name);

    /* Open the /proc directory. */
    dir = opendir("/proc");
    if (!dir)   {
        LOG("find_pid_by_name: cannot open /proc");
        return -1;
    }

    /* Walk through the directory. */
    while ((d = readdir(dir)) != NULL) {

        char exe [PATH_MAX+1];
        char path[PATH_MAX+1];
        int len;
        int namelen;

        /* See if this is a process */
        if ((pid = atoi(d->d_name)) == 0)
            continue;

        snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
        if ((len = readlink(exe, path, PATH_MAX)) < 0)
            continue;
        path[len] = '\0';

        /* Find ProcName */
        s = strrchr(path, '/');
        if(s == NULL)
            continue;
        s++;

        /* we don't need small name len */
        namelen = strlen(s);
        if(namelen < pnlen)     continue;

        if(!strncmp(proc_name, s, pnlen)) {
            /* to avoid subname like search proc tao but proc taolinke matched */
            if(s[pnlen] == ' ' || s[pnlen] == '\0') {
                foundpid[i] = pid;
                i++;
            }
        }
    }

    foundpid[i] = 0;
    closedir(dir);

    return  0;

}





/*
	Everytime read a line and print to the stdout
*/
void inline dump_file(char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    char buf[200] = {0};
    while (fgets(buf, sizeof(buf), fp) != NULL ) {
        STDOUT("%s",  buf);
    }
}



int inline str_equal(char *str1, char *str2)
{
    return (strcmp(str1,str2) == 0);
}


void update_upload_state(int state)
{
    FILE *fp = NULL;
    char *remote_addr = NULL;
    char *file_id = NULL;

    remote_addr = getenv("REMOTE_ADDR");
    if (NULL == remote_addr)
        return;

    file_id = getenv("FILE_ID");
    if (NULL == file_id)
        return;

    if( (fp = fopen(WEB_UPLOAD_STATE, "a+")) ) {
        fprintf(fp, "%s %s %u %u\n", remote_addr, file_id, state, (unsigned int)time(NULL));
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
        LOG("%s: state=%d ip=%s id=%s\n", __func__, state, remote_addr, file_id);
    }
}



struct wdk_program programs[] = {

    {
        .name = "sleep",
        .description = \
        "sleep for a while. This function will block\n"
        "the caller for sometime, default for 1 second\n"
        "\nOptions:\n\n"
        "/lib/wdk/sleep                       sleep for 1 s\n"
        "/lib/wdk/sleep [second]              sleep for [second]/1000 s",
        .function = wdk_sleep,
    },
    {
        .name = "ap",
        .description = \
        "Scan,  get scan result or apply , the result is formatted to make it\n"
        "parsed easily.\n"
	"\nOptions:\n\n"
        "/lib/wdk/ap scan                  issue scan command to driver\n"
        "/lib/wdk/ap result                get formatted ap scan result output\n"
        "/lib/wdk/ap apply ssid [passwd]   scan for the ap parameter and set cdb\n",
        .function = wdk_ap,
    },
    {
        .name = "cdbreset",
        .description = "Erase mtd2, you need to reboot after this operation\n",
        .function = wdk_cdbreset,

    },
    {
        .name = "debug",
        .description = "Debug the command, print the cmd and print the output\n",
        .function = wdk_debug,

    },
    {
        .name = "dhcps",
        .description = "get/set parameters of dhcp server, in this board\n"
        "dnsmasq takes over the dhcp server's work\n\n"
	"\nOptions:\n\n"
        "/lib/wdk/dhcps leases       show dhcp server leases\n"
        "/lib/wdk/dhcps num          show how many devices have be given the ip address\n"
        "/lib/wdk/dhcps set          set a static lease to the dhcp server",
        .function = wdk_dhcps,

    },
    {
        .name = "get_channel",
        .description = "get current wifi channel",
        .function = wdk_get_channel,

    },
    {
        .name = "logout",
        .description = "flush the web auth buffer to force web clients\n"
        "login again, usually triggered by a logout button pressed by the user.",
     
        .function = wdk_logout,

    },
    {
        .name = "ping",
        .description = "ping some address and get result\n"
		"\nOptions:\n\n"
		"/lib/wdk/ping addr  ping the addr and output\n",
        .function = wdk_ping,
    },
    {
        .name = "reboot",
        .description = "stop wifi, http server then reboot linux system",
        .function = wdk_reboot,
    },
    {
        .name = "route",
        .description = "route table setting\n"
        "\nOptions:\n\n"
	"/lib/wdk/route raw            translate the output of /proc/net/route\n"
	"/lib/wdk/route rip            mtc route restart\n",
        .function = wdk_route,
    },
    {
        .name = "wan",
        .description = "connect or release wan connection\n"
	"\nOptions:\n\n"
	"/lib/wdk/wan  connect		restart wan service \n"
	"/lib/wdk/wan  release 	        stop  wan service \n",
        .function = wdk_wan,
    },
    {

        .name = "mangment",
        .description = "override /tmp/httpd.conf with $sys_user1 in cdb  value",
        .function = wdk_mangment,
    },

    {
        .name = "sta",
        .description = "dump current STA(s) connected to this AP with pretty format",
        .function = wdk_sta,
    },

    {
        .name = "stor",
        .description = "show or delete files in Disk Drive, used by the web front ui"
	"\nOptions:\n\n"
	"/lib/wdk/stor ls    [PATH]           list the dir of SD/USBDisk root\n"
	"/lib/wdk/stor rm    [PATH]           delete some file with specific dir of SD/USBDisk root\n",
        .function = wdk_stor,
    },

    {
        .name = "stapoll",
        .description = "repair the channel when as a repeater or wisp. normally, we connect\n"
        "to an AP using sta0 and act as an AP using wlan0. If the channel of sta0\n"
        "changes, wlan0 channel need also be changed to be fixed by\n"
        "set the $wl_channel parameter in cdb.",
        .function =wdk_stapoll,
    },

    {
        .name = "status",
        .description = "get the status of wifi signal or battery power\n"
	"\nOptions:\n\n"
	"/lib/wdk/status wifi_signal		 get the signal strength of the sta0\n"
	"/lib/wdk/status batt_power          get the battery power via ADC(currently unsupported)",

        .function =wdk_status,
    },

    {
        .name = "system",
        .description = "set  ntp,hostname,timezone and watchdog\n"
	"\nOptions:\n\n"
	"/lib/wdk/system ntp                restart ntp client with cdb parameters\n"
	"/lib/wdk/system hostname           setting kernel hostname with $sys_name\n"
	"/lib/wdk/system timezone           setting /etc/TZ with $sys_day_info\n"
	"/lib/wdk/system watchdog           restart watchdog  cdb parameters\n",
        .function =wdk_system,
    },


    {
        .name = "http",
        .description = "clear memory, get client ip or mac\n"
	"\nOptions:\n\n"
	"/lib/wdk/http  largepost        clear memory expect some key process\n"
	"/lib/wdk/http  peerip           getting the http client ip controlling this device\n"
	"/lib/wdk/http  peermac          getting the http client mac controlling this device\n",
        .function =wdk_http,
    },


    {
        .name = "sysupgrade",
        .description = "sysupgrade in /lib/wdk dir\n"
	"\nOptions:\n\n"
	"/lib/wdk/sysupgrade auto	    detect the usb/sd storage and update firmware.img\n"
	"/lib/wdk/sysupgrade 			upgrate the firmware placed in /tmp/firmware.img",
        .function =wdk_sysupgrade,
    },


    {
        .name = "wdkupgrade",
        .description = "sysupgrade in tmp dir, used by sysupgrade .\n"
        "don't call this function explicitly",
        .function =wdk_sysupgrade,
    },

    {
        .name = "time",
        .description = "Get/Set some information of time "
	"\nOptions:\n\n"
	"/lib/wdk/time                        print seconds since linux started\n"
	"/lib/wdk/time  ntp	                  print seconds since 1970\n"
	"/lib/wdk/time  ntpstr                print current time string\n"
	"/lib/wdk/time  sync  [timevalue]     set the time value to system clock\n",
        .function =wdk_time,
    },
    {
        .name = "upnp",
        .description = "get some information of miniupnpd lease\n"
        "it runs as an internet gateway service\n"
		"\nOptions:\n\n"
		"/lib/wdk/upnp   map        print formatted /var/lease",
        .function =wdk_upnp,
    },

    {
        .name = "cdbupload",
        .description = "write user cdb config files to internal cdb. \n"
        "it can be paintext, gzip, bzip2 format. You must place the\n"
        "config.cdb in /tmp directory",
        .function =wdk_cdbupload,
    },

    {
        .name = "log",
        .description = "send/dump/clean mail\n"
	"/lib/wdk/log -s                 send the /var/log/messages*\n"
	"/lib/wdk/log -xymta             dump /var/log/messages*\n"
	"/lib/wdk/log clean              clear /var/log/messages*\n",
        .function =wdk_log,
    },
    {
        .name = "logconf",
        .description = "start syslogd,  to the network ip $log_sys_ip",
        .function =wdk_logconf,
    },
    {
        .name = "upload",
        .description = "query upload status.  with the last line of /tmp/upload_state\n"
        "update_upload_state will write current state to this file. States are:\n\n"
	"WEB_STATE_DEFAULT=0,\nWEB_STATE_ONGOING=1,\nWEB_STATE_SUCCESS=2,\n"
	"WEB_STATE_FAILURE=3,\nWEB_STATE_FWSTART=4\n\n\n"
	"\nOptions:\n\n"
	"/lib/wdk/upload state		 show the state of the current upload status*",
        .function =wdk_upload,
    },

    {
        .name = "wps",
        .description = "wps 0/1 status/button/pin/cancel/genpin"
	"\nOptions:\n\n"
	"/lib/wdk/wps button              wps-pbc\n"
	"/lib/wdk/wps cancel              cancel wps progress\n"
	"/lib/wdk/wps genpin		      generate pin code\n"
	"/lib/wdk/wps status	          show current wps status\n"
	"/lib/wdk/wps pin [pincode]	      do wps with the pin code\n",
        .function = wdk_wps,

    },

    {
        .name = "smbc",
        .description = "upload/query local media files to samba server"
	"\nOptions:\n\n"
	"/lib/wdk/smbc [file_dir] [server_dir] [usrname] [password] upload files to server\n"
	"/lib/wdk/smbc upload_status         show the status of the upload progress\n",
        .function = wdk_smbc,
    },
    {
        .name = "commit",
        .description = "call mtdm commit, restart service depend on\n"
        "the cdb.dep&cdb.define&cdb changes",
        .function = wdk_commit,
    },
    {
        .name = "save",
        .description = "call mtdm save, save the cdb content to flash",
        .function = wdk_save,
    },

    {
        .name = "rakey",
        .description = "simple wifi audio aplication interface"
	"\nOptions:\n\n"
	"/lib/wdk/rakey [1-6]                   play the playlist int /playlist/ with prefix of [1-6]\n"
	"/lib/wdk/rakey volup/voldown           increase/decrease volume by step 5\n"
	"/lib/wdk/rakey forward/backward	    play the next/previous playlist int /playlist/ \n"
	"/lib/wdk/rakey switch	[rafunc]        switch the rafunc \n"
	,
      .function = wdk_rakey,
    },

	{
		.name = "mpc",
		.description = "wdk mpc control, directly call mpc\n",
		.function = wdk_mpc,
	},

    {
        .name = "pandora",
        .description = "wdk pandora control\n"
                       "\nOptions:\n\n"
                       "/lib/wdk/pandora conf\n"
                       "/lib/wdk/pandora shutdown\n"
                       "/lib/wdk/pandora play [index]\n"
                       "/lib/wdk/pandora next\n"
                       "/lib/wdk/pandora stop\n"
                       "/lib/wdk/pandora volume\n"
                       "/lib/wdk/pandora get_login_status\n"
                       "/lib/wdk/pandora get_play_song\n"
                       "/lib/wdk/pandora get_station_list\n"
                       "/lib/wdk/pandora get_station_index\n",
        .function = wdk_pandora,
    },

	{
		.name = "omnicfg",
        .description = "wdk omnicfg\n"
                       "\nOptions:\n\n"
                       "/lib/wdk/omnicfg done\n"
                       "/lib/wdk/omnicfg sniffer_timeout\n"
                       "/lib/wdk/omnicfg timeout\n"
                       "/lib/wdk/omnicfg remove_log\n"
                       "/lib/wdk/omnicfg start_log\n"
                       "/lib/wdk/omnicfg stop_log\n"
                       "/lib/wdk/omnicfg reload\n"
                       "/lib/wdk/omnicfg stop\n"
                       "/lib/wdk/omnicfg state\n"
                       "/lib/wdk/omnicfg voice num\n"
                       "/lib/wdk/omnicfg zero_counter\n",
		.function = wdk_omnicfg,
	},

	{
		.name = "omnicfg_ctrl",
        .description = "wdk omnicfg control\n"
                       "\nOptions:\n\n"
                       "/lib/wdk/omnicfg_ctrl trigger\n"
                       "/lib/wdk/omnicfg_ctrl query\n",
		.function = wdk_omnicfg_ctrl,
	},

	{
		.name = "omnicfg_apply",
        .description = "wdk omnicfg apply\n"
                       "\nOptions:\n\n"
                       "/lib/wdk/omnicfg_apply base64 mode encode\n"
                       "/lib/wdk/omnicfg_apply mode ssid psk\n",
		.function = wdk_omnicfg_apply,
	},

	{
		.name = "hotplug-call",
        .description = "hotplug-call for kernel\n",
		.function = wdk_hotplug,
	},

};


int get_self_path(char *buf, int len)
{
    char szTmp[32] = {0};
    sprintf(szTmp, "/proc/%d/exe", getpid());
    int bytes = readlink(szTmp, buf, len);
    return bytes;
}

int readline2(char *base_path, char *ext_path, char* buffer, int buffer_size)
{
    FILE * fp = NULL;
    char path[200] = {0};
    strcat(path, base_path);
    strcat(path, ext_path);
    fp = fopen(path, "r");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }

    if((fgets(buffer, buffer_size, fp))!=NULL)   {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}




int readline(char *path, char* buffer, int buffer_size)
{
    FILE * fp = NULL;
    fp = fopen(path, "r");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }

    if((fgets(buffer, buffer_size, fp))!=NULL)   {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}

int writeline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "w");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }
    if((fputs(buffer, fp)) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}

int appendline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "a");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }
    if(fputs(buffer, fp) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}


void enable_debug(int enable)
{
    if (enable) {
        putenv("WDK_DBG=1");
    } else {
        putenv("WDK_DBG=0");
    }
}


int check_debug()
{
    int enable = 0;

    char *value = getenv("WDK_DBG");
    if (NULL != value) {
        if (strcmp(value, "1") == 0)
            enable = 1;
    }
    return enable;
}


void debug_on()
{
    g_log_on = 1;
}


#if 0
/*
	check program exists in /lib/wdk dir
*/
int program_exists(char *program_name)
{
    DIR *dir = NULL;
    struct dirent *dir_entry;
    int exist = 0;

    dir = opendir("/lib/wdk");
    if (NULL != dir) {
        while ((dir_entry = readdir(dir)) != NULL) {
            printf("\t%s\n", dir_entry->d_name );
            if (strcmp(dir_entry->d_name, program_name) == 0)
                exist = 1;
        }
        (void) closedir(dir);
    }

    return exist;
}
#endif

void print_available_program()
{
    int i = 0;
    for (i = 0; i < sizeof(programs)/sizeof(programs[0]); i++) {
        printf("\n=========>");
        printf(programs[i].name);
        printf(":\n");
        printf(programs[i].description);
        printf("\n\n");
    }
}


void print_help()
{
    printf("\nwdk multi-call binary\nAvailable programs:\n\n");

    print_available_program();
}


/*
	change to single file format

	example:
	/lib/wdk/commit -> commit
	./commit 		-> commit
	./wdk/commit 	-> commit

*/
char* get_real_name(char *full_name)
{
    int i = 0;
    for (i = strlen(full_name); i > 0 ; i--)
        if (full_name[i] == '/')
            return (full_name + i+1);
    return full_name;
}


/*
	To avoid long output like "iw dev sta0 scan"
	using continuous fgets
*/
int print_shell_result(char *cmd)
{
    void *myhdr = NULL;
    char *myptr = NULL;
    int myfree = 0;
    int ret = EXIT_FAILURE;

    while (!exec_cmd4(&myhdr, &myptr, myfree, cmd)) {
        if (myptr) {
            STDOUT("%s\n", myptr);
        }
        ret = EXIT_SUCCESS;
    }

    if (ret == EXIT_FAILURE) {
        LOG("Failed to run command\n");
        return -1;
    }
    return 0;
}


#if 0
static void printf_arguments(int argc, char **argv)
{
    int i = 0;
    char cmd_buffer[1000] = {0};
    strcat(cmd_buffer, "--------> wdk calling ");

    for (i = 0; i < argc; i++) {
        strcat(cmd_buffer, argv[i]);
        strcat(cmd_buffer, " ");

    }

    LOG("%s", cmd_buffer);
}
#endif

int main(int argc, char *argv[])
{
    int i = 0;
    int found = 0;
    struct timeval timecount;
    g_log_on = 0;
	int ret = 0;


#if 0
    if (check_debug())
#endif
        debug_on();

    //printf_arguments(argc, argv);

    for (i = 0; i < sizeof(programs)/sizeof(programs[0]); i++) {
        if(strcmp(programs[i].name, get_real_name(argv[0])) == 0) {
            gettimeofbegin(&timecount);
            /* print help information */
            if ((argc ==2) && (str_equal(argv[1], "help") || (str_equal(argv[1], "--help")))){
                STDOUT("wdk multi-call binary\n \n%s\n",  programs[i].description);		
	   /* execute the function */
            }  else if ((ret=programs[i].function(argc - 1, argv + 1)) == -1) {
            	LOG("wdk program %s returns error", programs[i].name);
		STDOUT("wdk multi-call binary\n \n%s\n",  programs[i].description);
            }
            LOG("\n[%s] spend=%ldms", programs[i].name, gettimevaldiff(&timecount));
            found = 1;
        }
    }

    if (0 == found) {
        printf("No such command:%s\n", argv[0]);
        print_help();

    }

    return ret;
}







