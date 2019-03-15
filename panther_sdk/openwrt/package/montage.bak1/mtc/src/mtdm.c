
/**
 * @file mtdm.c
 *
 * @author Frank Wang
 * @date   2015-10-06
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#if defined(__PANTHER__)
#include <alsa/asoundlib.h>
#endif

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_inf.h"
#include "include/mtdm_fsm.h"
#include "include/mtdm_proc.h"

#include <libutils/ipc.h>
#include <libcchip/function/common/px_timer.h>

const char *applet_name;

#if defined(__PANTHER__)
MtdmData *mtdm;
#else
static int semId[MAX_SEM_LIST] = { -1 };
MtdmData mtdmdata = {
    .boot.misc = 0,
    .rule.OMODE = OP_UNKNOWN,
};
MtdmData *mtdm = &mtdmdata;
#endif

/**
 *  try to lock
 *
 *  @return none
 *
 */
void mtdm_lock(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        down(mtdm->semId[lockname]);
    }
    else {
        cprintf("lockname(%d) is invalid\n",lockname);
    }
}

/**
 *  try to unlock
 *
 *  @return none
 *
 */
void mtdm_unlock(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        up(mtdm->semId[lockname]);
    }
    else {
        cprintf("lockname(%d) is invalid\n",lockname);
    }
}

int mtdm_get_semid(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        return mtdm->semId[lockname];
    }
    else {
        cprintf("lockname is invalid\n");
    }

    return UNKNOWN;
}

static void mtdm_save_pid(char *fpid)
{
    FILE *pidfile;

    mkdir("/var/run", 0755);
    if ((pidfile = fopen(fpid, "w")) != NULL) {
        fprintf(pidfile, "%d\n", getpid());
        (void) fclose(pidfile);
    }
}

static void mtdm_shutdown()
{
    mtdm->terminate = 1;
    close(mtdm->inf_sock);
    mtdm->inf_sock = -1;
    logger (LOG_INFO, "mtdm_shutdown[%d]", getpid());
#if !defined(__PANTHER__)
    int pid = read_pid_file(MTDM_PID);

    if (pid == UNKNOWN || pid != getpid()) {
        logger (LOG_INFO, "mtdm pid is [%d], but pid is [%d]", getpid(), pid);
    }
    else {
        mtdm_fsm_schedule(fsm_stop);
    }
#endif
}
static int mtdm_get_sw_info(char *build, int build_len)
{
    exec_cmd3(build, build_len-1, "cat /etc/sw_build");
    return 0;
}

static void mtdm_debug_init(void)
{
    mtdm->boot.flags |= PROC_DBG;
    start_stop_daemon("/sbin/syslogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL,
        "-S -s 500");
    start_stop_daemon("/sbin/klogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL,
        "-c 1");
    // default output to stdout, and report log in /var/log/messages
    setloglevel(LOG_DEBUG);
    openlogger(applet_name, 0);
}

/*
 * mtdm_init
 * Only init cdb firstly
 */
static void mtdm_init(void)
{
#if !defined(__PANTHER__)
    char cpuclk[][4] = {"60","64","80","120","160","202","240","295","320","384","480","512","640","0"};
    char sysclk[][4] = {"60","64","80","120","150","160","0"};
    char hver[128] = {0};
    int hw_ver[4] = {0};
    int cpuclki, sysclki;
#endif
    char tmp[128] = {0};
    char vid[32] = {0};
    char pid[32] = {0};
    char sw_ver[128] = {0};
    char sw_build[128] = {0};
    unsigned long _id;

    mkdir("/var/run", 0755);
    mkdir("/var/lib", 0755);
    mkdir("/var/log", 0755);

    if(0>cdb_attach_db())
    {
        if(0>cdb_init())
            cprintf("CDB init failed...\n");
    }

    system("/sbin/bootvars rdb");

    boot_cdb_get("mac0", tmp);
    sprintf(mtdm->boot.MAC0, "%s", tmp);

    boot_cdb_get("mac1", tmp);
    sprintf(mtdm->boot.MAC1, "%s", tmp);

    boot_cdb_get("mac2", tmp);
    sprintf(mtdm->boot.MAC2, "%s", tmp);

#if !defined(__PANTHER__)
    boot_cdb_get("hver", hver);
#endif
    boot_cdb_get("id", tmp);

    _id = strtoul(tmp, NULL, 0);
    sprintf(vid, "%04lx",(_id >> 16));
    sprintf(pid, "%04lx",(_id & 0x0FFFF));

#if !defined(__PANTHER__)
    sscanf(hver, "%3u.%3u.%3u.%3u", &hw_ver[3], &hw_ver[2], &hw_ver[1], &hw_ver[0]);
    cpuclki = (hw_ver[2]&0xF0)>>4;
    sysclki = (hw_ver[2]&0x0F);
    if (hw_ver[3] & 0x80) {
        mtdm->boot.misc |= PROC_DBG;
        start_stop_daemon("/sbin/syslogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
            "-S -s 500");
        start_stop_daemon("/sbin/klogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
            "-c 1");
        // default output to stdout, and report log in /var/log/messages
        setloglevel(LOG_DEBUG);
        openlogger(applet_name, 0);
    }
#endif

    mtdm_get_sw_info(sw_build, sizeof(sw_build));
#if !defined(__PANTHER__)
    cdb_set("$hw_ver", hver);
#endif
    cdb_set("$sw_ver", sw_ver);
    cdb_set("$sw_build", sw_build);
	cdb_set("$tmp_wifi_ver", sw_build);
    cdb_set("$sw_vid", vid);
    cdb_set("$sw_pid", pid);
#if defined(__PANTHER__)
    cdb_set("$chip_cpu", "550");
    cdb_set("$chip_sys", "180");
#else
    cdb_set("$chip_cpu", cpuclk[cpuclki]);
    cdb_set("$chip_sys", sysclk[sysclki]);
#endif

    if(!iface_exist("sta0")) {
        exec_cmd2("iw dev wlan0 interface add sta0 type managed");
        set_mac("sta0", mtdm->boot.MAC2);
#if !defined(__PANTHER__)
        ifconfig("sta0", IFUP, NULL, NULL);
        ifconfig("sta0", IFDOWN, "0.0.0.0", NULL);
#endif
    }

    checkup_main(0, 0);
    cdb_service_commit_change(NULL);
    cdb_save();
}

static void mtdm_deinit(void)
{
    if (mtdm->boot.flags & PROC_DBG) {
        killall("syslogd", SIGTERM);
        killall("klogd", SIGTERM);
        closelogger();
    }
}

#if !defined(__PANTHER__)
static void mtdm_start_boot(int sData)
{
    logger (LOG_INFO, "system boot");
	mtdm_monitor_init();
    mtdm_proc_handle(ProcessBoot);
}
#endif

static void mtdm_start_shutdown(int sData, void *data)
{
    logger (LOG_INFO, "system shutdown");
    mtdm_proc_handle(ProcessShutdown);
}

#define GUARD_SIZE      1024
#define MTDM_SHMSZ      (sizeof(MtdmData) + GUARD_SIZE)
static int mtdm_shmid;
static unsigned char *mtdm_shm;

int mtdm_ipc_unlock(void)
{
    struct sembuf sops[1];

    mtdm->lock_cnt--;
    if(mtdm->lock_cnt > 0)
        return 0;

    mtdm->lock_holder = 0;

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = 1;         /* Increment value by one */
    sops[0].sem_flg = 0;

retry:
    if (semop(mtdm->ipc_lock_semid, sops, 1) < 0) {
       if(errno==EINTR)
           goto retry;
       logger (LOG_INFO, "mtdm_ipc_unlock[%d]:%d", getpid(), errno);
       exit(EXIT_FAILURE);
    }

    return 0;
}

int mtdm_ipc_lock(void)
{
    struct sembuf sops[1];

    if(getpid() == mtdm->lock_holder)
    {
        mtdm->lock_cnt++;
        return 0;
    }

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = -1;
    sops[0].sem_flg = 0;

retry:
    if (semop(mtdm->ipc_lock_semid, sops, 1) < 0) {
       if(errno==EINTR)
           goto retry;
       logger (LOG_INFO, "mtdm_ipc_lock[%d]:%d", getpid(), errno);
       exit(EXIT_FAILURE);
    }

    mtdm->lock_holder = getpid();
    mtdm->lock_cnt = 1;

    return 0;
}

static int ipc_init(void)
{
    key_t key = ftok("/usr/bin/mtdaemon", 200);
    int mtdm_semid;

    if ((mtdm_semid = semget(key, 1, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((mtdm_shmid = shmget(key, MTDM_SHMSZ, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((mtdm_shm = shmat(mtdm_shmid, NULL, 0)) == (unsigned char *) -1)
        goto error;

    memset(mtdm_shm, 0, MTDM_SHMSZ);

    mtdm = (MtdmData *) mtdm_shm;
    mtdm->bootState = bootState_start;

    if(mtdm_semid>=0)
        mtdm->ipc_lock_semid = mtdm_semid;

    mtdm->lock_holder = 0;
    mtdm->lock_cnt = 0;
    semctl(mtdm->ipc_lock_semid, 0, SETVAL, 1);

#if defined(CONFIG_PACKAGE_mpd_mini)
    mtdm->exec_mpd_mode_chk = 0;
#endif

    mtdm->timerThreadAlreadyLaunch = FALSE;
    list_init(&mtdm->timerListRunning);
    list_init(&mtdm->timerListExpired);
    list_init(&mtdm->timerListExec);
    mtdm->inf_sock = -1;
    mtdm->rule.OMODE = OP_UNKNOWN;

#if defined(CDB_IPC_LOCK)
    if(0 > cdb_ipc_connect())
        return -1;

    cdb_ipc_init();
#endif

#if defined(OMNICFG_IPC_LOCK)
    if(0 > omnicfg_ipc_connect())
        return -1;

    omnicfg_ipc_init();
#endif

    return 0;

error:
    return -1;
}

#if defined(CONFIG_PACKAGE_mpd_mini)
void sig_usr1_handler(int signo)
{
    mtdm->exec_mpd_mode_chk = 1;
    return;
}
#endif

void sigchld_handler(int sig)
{
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  return;
}

void kill_proc(char *process_name)
{
    char command_buf[256];

    snprintf(command_buf, 256, "killall %s", process_name);

    system(command_buf);
}

void mtdm_killall(void)
{
    int retry = 2;

    signal(SIGTERM, SIG_IGN);

    kill_proc("mtdaemon");
    sleep(3);

    while(retry-->0)
    {
        kill_proc("syslogd");
        kill_proc("klogd");
        kill_proc("hostapd");
        kill_proc("wpa_supplicant");
        kill_proc("mpdaemon");
        kill_proc("uhttpd");
        kill_proc("crond");
        kill_proc("telnetd");
        kill_proc("omnicfg_bc");
        kill_proc("shairport");
        kill_proc("upmpdcli");
        kill_proc("dnsmasq");
        kill_proc("ntpclient");
        kill_proc("mpg123");
        kill_proc("wpa_cli");
        kill_proc("udhcpc");
        kill_proc("avahi-daemon");
        kill_proc("mspotify");
        kill_proc("omnicfg");
        sleep(2);
    }

    system("rm -rf /tmp/*");
    system("rm -f /var/log/*");
    system("echo 1 > /proc/sys/vm/drop_caches");

    system("cdb fclear");
}

#if defined(__PANTHER__)
void snd_dev_init(char *dev_name)
{
    snd_pcm_t *handle;

    if(snd_pcm_open(&handle, dev_name, SND_PCM_STREAM_PLAYBACK, 0) == 0)
    snd_pcm_close(handle);
}

void preinit_snd_devs(void)
{
//    snd_dev_init("spotify");
//    snd_dev_init("media");
//    snd_dev_init("airplay");
	  snd_dev_init("default");
	  snd_dev_init("common");
	  snd_dev_init("alerts");
	  snd_dev_init("tipsound");
}
#endif

extern void infHandler(char *argv0);
int main(int argc, char **argv)
{
    int boot = 1;
    int i = 0;
    int debug = 0;
    int foreground = 0;
//	showCompileTmie("mtdaemon",cslog);
    if (argc > 1) {
        if (!strcmp(argv[1], "boot")) {
            memset(argv[1], 0, strlen(argv[1]));
            boot = 1;
        }
        else if (!strcmp(argv[1], "shutdown")) {
            if(0>ipc_init())
                return -1;
            mtdm_start_shutdown(0,0);
            return 0;
        }
        else if (!strcmp(argv[1], "-k")) {
            mtdm_killall();
            return 0;
        }
        else if (!strcmp(argv[1], "-df"))
        {
            debug = 1;
            foreground = 1;
        }
        else if (!strcmp(argv[1], "-d"))
        {
            debug = 1;
        }
        else if (!strcmp(argv[1], "-f"))
        {
            foreground = 1;
        }
    }

    if(0>ipc_init())
        return -1;

    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, mtdm_shutdown);
    signal(SIGINT, mtdm_shutdown);
#if defined(CONFIG_PACKAGE_mpd_mini)
    signal(SIGUSR1, sig_usr1_handler);
#endif

    if(foreground || (fork() == 0)) {

        if(!debug) {
            //check PID
            if (read_pid_file(MTDM_PID)!=UNKNOWN && read_pid_file(MTDM_PID)!=getpid()) {
                cprintf("MTDM is already running?\n");
                return RET_ERR;
            }
        }

        applet_name = strrchr(argv[0], '/');
        if (applet_name)
            applet_name++;
        else
            applet_name = argv[0];

        //write PID
        mtdm_save_pid(MTDM_PID);

        //create sem
        for (i=semWin; i<MAX_SEM_LIST; i++) {
            sem_create(ftok("/usr/bin/mtdaemon", 100+i), &mtdm->semId[i]);
            semctl(mtdm->semId[i], 0, SETVAL, 1);
        }

// atexit means that signal rises again when exit
//        atexit(mtdm_shutdown);

        if(debug)
            mtdm_debug_init();
#ifdef CONFIG_PACKAGE_duer_linux
		exec_cmd("tar xvf /dev/mtd7 -C / && mtd erase /dev/mtd7");// recover duercfg files	
#endif		
        mtdm_init();
        mtdm_proc_init();

        TimerInit(argv[0]);
        if (boot)
        {
            pid_t child;

            preinit_snd_devs();

            logger (LOG_INFO, "system boot");

            child = fork();
            if(child==0) {
                //sleep(2);   // unmark for boundary condition test
                mtdm_proc_handle(ProcessBoot);
                exit(0);
            }
            else if(child < 0) {
                mtdm_proc_handle(ProcessBoot);
            }

            // unmark to wait until boot process done
            //while(bootState_done != mtdm->bootState)
            //     usleep(10000);
            mtdm_monitor_init(argv[0]);
        }

        infHandler(argv[0]);

        TimerDeInit();

        mtdm_deinit();

        //remove sem
        for (i=semWin; i<MAX_SEM_LIST; i++) {
            sem_delete(mtdm->semId[i]);
        }

        //remove pid file
        remove(MTDM_PID);
    }

    return 0;
}

void pthread_exit(void *__retval)
{
    /* link with pthread is not allowed */
    abort();
}

