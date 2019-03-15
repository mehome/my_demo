/*
 * =====================================================================================
 *
 *       Filename:  mpd.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/26/2016 03:59:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

#include <mpd/connection.h>
#include <mpd/mixer.h>

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

//#define MPD_PRINT_USED_TIME

#define DAEMON_BIN "/usr/bin/mpdaemon"
#define DAEMON_OCONF "/etc/mpd.conf"
#define DAEMON_CONF "/tmp/mpd.conf"
#define DAEMON_PID_FILE "/var/mpd.pid"

#define RALIST_FILE "/etc/ralist"

#define PANDORA_BIN "/lib/wdk/pandora"


#if 0
/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

static void mpd_add_ralists(void)
{
    char cmd[BUF_SIZE];

    sprintf(cmd, "ls -1 /playlists/ | "
                 "sort -n | sed -e '/audioPlayList/d' -e '/^mmcblk[0-9].$/d' -e '/^sd[a-z].[0-9].$/d' "
                 "> /tmp/ralists");
    exec_cmd(cmd);
    sprintf(cmd, "awk '{print \"\\\"/playlists/\"$0\"\\\"\"}' /tmp/ralists | xargs awk '{print $0}' > /tmp/raurls");
    exec_cmd(cmd);
    sprintf(cmd, "awk 'BEGIN{i=0;j=0} NR==FNR{a[i]=$0;i=i+1} "
                 "NR>FNR{FS=\"-\";b[j]=$1;sub(/[0-9]*-/,\"\",$0);sub(/.m3u/,\"\",$0);print $0\"&\"a[j]\"&\"b[j];j=j+1}' "
                 "/tmp/raurls /tmp/ralists > /etc/ralist"); 
    exec_cmd(cmd);
    unlink("/tmp/ralists");
    unlink("/tmp/raurls");
}

#endif

int call_libmpdclient_set_volume(int volume)
{
    struct mpd_connection *conn;
    int ret = 0;
    conn = mpd_connection_new(NULL, 0, 30000);
    if (conn == NULL) {
        return -1;
    }
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        mpd_connection_free(conn);
        conn = NULL;
        return -1;
    }
    ret = mpd_run_set_volume(conn, volume);
    mpd_connection_free(conn);

    if (ret == false)
        cprintf("%s fail...\n", __func__);

    return ret;
}

static int start(void)
{
    char *Codec[] = {
        "ES9023",
        "USB Audio",
        NULL
    };
//    int  hasCodec = 0;
    char **pCodec = Codec;
    int  ra_func = cdb_get_int("$ra_func", 0);
    char tmp[BUF_SIZE];
//    char cmd[BUF_SIZE];
    char *ptr;
    FILE *fp;

#ifdef MPD_PRINT_USED_TIME
    struct timeval tv;

    cprintf("%s:%d start mpd time: %ld\n", __func__, __LINE__, gettimevalmsec(0));
    gettimeofbegin(&tv);
#endif

    if (ra_func != raFunc_ALL_DISABLE) {

        config_loopback();
        
        if ((fp = fopen(DAEMON_OCONF, "r"))) {
            while ( new_fgets(tmp, sizeof(tmp), fp) != NULL ) {
                if (!strncmp(tmp, "playlist_directory", 18)) {
                    ptr = tmp;
                    while (*ptr) {
                        if (*ptr++ == '\"') {
                            strcpy(tmp, ptr);
                            if (tmp[strlen(tmp)-1] == '\"')
                                tmp[strlen(tmp)-1] = '\0';
                            break;
                        }
                    }
                    if( !f_isdir(tmp) ) {
                        mkdir(tmp, 0777);
                    }
                    break;
                }
            }
            fclose(fp);
        }
        cp(DAEMON_OCONF, DAEMON_CONF);

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d config loopback, mkdir, cp used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif

        cdb_get("$mpd_codec_info", tmp);
        while (*pCodec) {
            if (strstr(tmp, *pCodec++)) {
//                hasCodec = 1;
                break;
            }
        }
#if 0        
        if (hasCodec) {
            int card = 0;
            int device = 0;
            sscanf(tmp, "card %d: %*[^,], device %d:", &card, &device);
           // sprintf(cmd, "sed -i '/alsa/,/oss/{s/#*\tdevice.*/\tdevice\t\t\"plughw:%d,%d\"/g}' " DAEMON_CONF, card, device);
            //exec_cmd(cmd);
           // sprintf(cmd, "sed -i '/alsa/,/oss/{s/#*\tmixer_type.*/\tmixer_type\t\t\"software\"/g}' " DAEMON_CONF);
           // exec_cmd(cmd);
        }
        else if (strstr(tmp, "tas5711")) {
            sprintf(cmd, "sed -i '/alsa/,/oss/{s/.*mixer_control.*/\tmixer_control\t\"Master\"/g}' " DAEMON_CONF);
            exec_cmd(cmd);
        }
#endif

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d check codec used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif

#if 1
        start_stop_daemon(DAEMON_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL,
            "%s", DAEMON_CONF);
#else
        start_stop_daemon(DAEMON_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL,
            "--stdout --stderr --no-daemon %s", DAEMON_CONF);
#endif

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d run mpdaemon used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif
//
//				int volume;
//				volume = cdb_get_int("$ra_vol", 0);
//				volume += 201;
//#if 1
//	   if( volume > 100 ) 
//         volume = (int)((float)(volume / 255.0) * 100.0);
//        call_libmpdclient_set_volume( volume );
//#else
//		int vol,volume;
//		vol = atoi(ra_vol);
//		if(vol>=155) {
//				volume = vol - 155;
//		} else {
//				volume = vol;
//		}
//        sprintf(cmd, "mpc volume %d", volume);
//        cprintf("%s:%d run mpdaemon cmd: %s\n", __func__, __LINE__,cmd);
//        exec_cmd(cmd);
//        sprintf(cmd,  "uci set xzxwifiaudio.config.volume=%d", volume);
//    	exec_cmd(cmd);
//    	sprintf(cmd,  "uci commit");
//        exec_cmd(cmd);
//#endif

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d mpc volume used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif

        cdb_set("$mpd_nocheck", "0");

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d set mpd_nocheck used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif

        //mpd_add_ralists();

#ifdef MPD_PRINT_USED_TIME
        cprintf("%s:%d add ralist used time: %ld\n", __func__, __LINE__, gettimevaldiff(&tv));
        gettimeofbegin(&tv);
#endif
    }

#ifdef MPD_PRINT_USED_TIME
    cprintf("%s:%d finish mpd time: %ld\n", __func__, __LINE__, gettimevalmsec(0));
#endif

    return 0;
}

static int stop(void)
{
    char cmd[BUF_SIZE];

    cdb_set("$mpd_nocheck", "1");

    if( f_exists(PANDORA_BIN) ) {
        sprintf(cmd, PANDORA_BIN " stop 2> /dev/null");
        exec_cmd(cmd);
        sprintf(cmd, PANDORA_BIN " shutdown 2> /dev/null");
        exec_cmd(cmd);
    }
    if( f_exists(DAEMON_PID_FILE) ) {
        start_stop_daemon(DAEMON_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, DAEMON_PID_FILE, NULL);
        unlink(DAEMON_PID_FILE);
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int mpd_main(montage_proc_t *proc, char *cmd)
{
	//if(get_pid_by_name("mpdaemon") < 0){
		if(strcmp(cmd, "boot") == 0 || strcmp(cmd, "start") == 0){
		    if(get_pid_by_name("mpdaemon") > 0){
			    return -1;
			  }
		}
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

