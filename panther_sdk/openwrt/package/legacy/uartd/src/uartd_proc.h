
#ifndef __UARTD_PROC_H__
#define __UARTD_PROC_H__
#include "uartfifo.h"
#define NO_TEST_MODE -1

extern void StartConnecting_led();
extern void StartConnectedOk_led();
extern void StartConnectedFAL_led();

static int Process_COL_CMD( BUF_INF *cmd);
static int Process_CHE_CMD( BUF_INF *cmd);
static int Process_MAC_CMD( BUF_INF *cmd);
static int Process_PLM_CMD( BUF_INF *cmd);
static int Process_SSID_CMD( BUF_INF *cmd);
static int Process_AT_SW_TFPLAY_CMD(void);
static int Process_AT_SW_USBPLAY_CMD(void);
static int Process_TLK_CMD( BUF_INF *cmd);
static int Process_DUE_CMD(BUF_INF *cmd);
static int Process_PLP_CMD( BUF_INF *cmd);
static int Process_PLY_CMD( BUF_INF *cmd);
static int Process_VOL_CMD( BUF_INF *cmd);
static int Process_WIFI_CMD( BUF_INF *cmd);
static int Process_AT_FACTORY_CMD(BUF_INF *cmd);
static int Process_AT_NAME_CMD(BUF_INF *pCmd);
static int Process_AT_PLAYMODE(BUF_INF *pCMD);
static int Process_AT_MAC1_CMD(BUF_INF *pCMD);
static int Process_AT_MAC2_CMD(BUF_INF *pCMD);
static int Process_AT_SSID_CMD(BUF_INF *pCMD);
static int Process_AT_KEYS_CMD(BUF_INF *pCMD);
static int Process_AT_TIME_CMD(BUF_INF *cmd);
static int Process_AT_MCU_VER_CMD(BUF_INF *pCMD);
static int Process_AT_LANGUAGE_CMD(BUF_INF *pCMD);
static int Process_AT_TESTMODE_CMD(BUF_INF *cmd);
static int Process_AT_SIDP_CMD(BUF_INF *pCmd);
static int Process_AT_KEY_CMD(BUF_INF *pCMD);
static int Process_AT_BATTERY(BUF_INF *cmd);
static int Process_AT_CHARGE(BUF_INF *cmd);
static int Process_AT_IN(BUF_INF *cmd);
static int Process_AT_BTS_SLP(BUF_INF *cmd);
static int Process_AT_BCN(BUF_INF *cmd);
static int Process_AT_BSC(BUF_INF *cmd);
static int Process_MIC_MUT(BUF_INF *cmd);
static int Process_MIC_UNM(BUF_INF *cmd);
static int Process_Stop_Time(BUF_INF *cmd);
static int Process_Get_Time(BUF_INF *cmd);
static int Process_AT_CHARGE(BUF_INF *cmd);
static int Process_POWER_OFF_CMD(BUF_INF *pCMD);
static int Process_LED_CMD(BUF_INF *cmd);

#endif



