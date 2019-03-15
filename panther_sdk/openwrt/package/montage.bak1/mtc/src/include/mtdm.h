
/**
 * @file mtdm.h
 *
 * @author
 * @date    2015-10-06
 */

#ifndef _MTDM_H_
#define _MTDM_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <cdb.h>
#include <shutils.h>
#include <strutils.h>
#include <netutils.h>
#include <linklist.h>
#include <ipc.h>
#include <include/timer.h>

#include "include/mtdm_types.h"
#include "serv/misc.h"

#define MTDM_PID "/var/run/mtdm.pid"
#define UNKNOWN (-1)

#define OMNICFG_R_CONNECTING       1
#define OMNICFG_R_SUCCESS          2

typedef enum
{
    opMode_db       = 0,
    opMode_ap       = 1,
    opMode_wr       = 2,
    opMode_wi       = 3,
    opMode_rp       = 4,
    opMode_br       = 5,
    opMode_smart    = 6,
    opMode_mb       = 7,
    opMode_p2p      = 8,
    opMode_client   = 9,
    opMode_smartcfg = 99,
}OPMode;

typedef enum
{
    wanMode_unknown = 0,
    wanMode_static  = 1,
    wanMode_dhcp    = 2,
    wanMode_pppoe   = 3,
    wanMode_pptp    = 4,
    wanMode_l2tp    = 5,
    wanMode_bigpond = 6,
    wanMode_pppoe2  = 7,
    wanMode_pptp2   = 8,
    wanMode_3g      = 9,
}WANMode;

typedef enum
{
    wlRole_NONE     = 0,
    wlRole_STA      = 1,
    wlRole_AP       = 2,
}WLRole;

typedef enum
{
    Wanif_State_DISCONNECTED  = 0,
    Wanif_State_CONNECTING    = 1,
    Wanif_State_CONNECTED     = 2,
    Wanif_State_DISCONNECTING = 3,
}WANIFState;

typedef enum
{
    raFunc_ALL_DISABLE    = 0,
    raFunc_INTERNET_RADIO = 1,
    raFunc_DLNA_AIRPLAY   = 2,
    raFunc_LOCAL_STORAGE  = 3,
    raFunc_ALL_ENABLE     = 4,
}RAFunc;

typedef enum
{
    semWin = 0,
    semFSMData,
    semTimer,
    semCfg,

    MAX_SEM_LIST
}SemList;

typedef enum
{
    timerTest = 0,
    timerMtdmStartBoot,
    timerMtdmBrProbe,
    timerMtdmAPList,
    timerMtdmDelayCommit,
    timerMtdmSnifferTimeout,

    timerMax
}fixTimerList_t;

typedef struct {
    char MAC0[18];
    char MAC1[18];
    char MAC2[18];
    unsigned long flags;
}MtdmBoot;

typedef struct {
    int op_work_mode;
    int wan_mode;
}MtdmCdb;

typedef struct {
    char LAN[10];
    char WAN[10];
    char WANB[10];
    char STA[10];
    char AP[10];
    char BRSTA[10];
    char BRAP[10];
    char IFPLUGD[10];
    char LMAC[18];
    char WMAC[18];
    char STAMAC[18];
    char WIFLINK;
    int HOSTAPD;
    int WPASUP;
    int OMODE; // op_work_mode
    int WMODE; // wan_mode
}MtdmRule;

typedef struct {
    int reconf_wpa;
}MtdmMisc;

typedef enum
{
    bootState_done  = 0,
    bootState_start = 1,
    bootState_wanup = 2,
    bootState_ipup  = 3,
}BootState;

typedef struct {
    MtdmBoot boot;
    MtdmCdb  cdb;
    MtdmRule rule;
    MtdmMisc misc;
    volatile BootState bootState;
    int ipc_lock_semid;
    volatile pid_t lock_holder;
    int lock_cnt;
    int semId[MAX_SEM_LIST];
    mytimer_t timer[timerMax];
    pid_t TimerThread_pid;
    struct list_head timerListRunning;
    struct list_head timerListExpired;
    struct list_head timerListExec;
    int timerThreadAlreadyLaunch;
    int socket_pair[2];
    volatile int terminate;
    int inf_sock;
#if defined(CONFIG_PACKAGE_mpd_mini)
    volatile int exec_mpd_mode_chk;
#endif
}MtdmData;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

void mtdm_lock(SemList lockname);
void mtdm_unlock(SemList lockname);
int mtdm_get_semid(SemList lockname);
int read_pid_file(char *file);

int mtdm_ipc_lock(void);
int mtdm_ipc_unlock(void);

#endif
