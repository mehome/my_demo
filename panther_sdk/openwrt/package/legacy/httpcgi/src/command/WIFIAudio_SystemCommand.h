#ifndef __WIFIAUDIO_SYSTEMCOMMAND_H__
#define __WIFIAUDIO_SYSTEMCOMMAND_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>

#include "WIFIAudio_RealTimeInterface.h"
#include "WIFIAudio_GetCommand.h"
#include "WIFIAudio_RetJson.h"

#define WIFIAUDIO_BT_UP_PROGRESS	"/tmp/bt_up_progress"
#define WIFIAUDIO_WIFI_UP_PROGRESS	"/tmp/wifi_up_progress"
#define WIFIAUDIO_CONEXANTSFS_PATHNAME	"/tmp/oem_image.sys"
#define WIFIAUDIO_CONEXANTBIN_PATHNAME	"/tmp/iflash.bin"

extern WARTI_pStr WIFIAudio_SystemCommand_WlanGetApListTopStr(void);
extern int WIFIAudio_SystemCommand_wlanConnectAp(WAGC_pConnectAp_t pconnectap);
extern WARTI_pStr WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetDeviceInfo(void);
extern WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigure(void);
extern int WIFIAudio_SystemCommand_setOneTouchConfigure(int value);
extern WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName(void);
extern int WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName(int value);
extern WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonStatus(void);
extern WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonNeedInfo(void);
extern int WIFIAudio_SystemCommand_LogoutAmazon(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetWakeupTimesTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetPlayerStatusTopStr(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_playlist(int num);
extern int WIFIAudio_SystemCommand_setPlayerCmd_pause(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_play(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_prev(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_next(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_seek(int num);
extern int WIFIAudio_SystemCommand_setPlayerCmd_stop(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_vol(int num);
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_volTopStr(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_mute(int flag);
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_loopmode(int num);
extern int WIFIAudio_SystemCommand_setPlayerCmd_equalizer(int mode);
extern WARTI_pStr WIFIAudio_SystemCommand_getEqualizerTopStr(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_slave_channel(int value);
extern int WIFIAudio_SystemCommand_getPlayerCmd_slave_channel(void);
extern int WIFIAudio_SystemCommand_setSSID(char *pssid);
extern int WIFIAudio_SystemCommand_setNetwork(WAGC_pNetwork_t pnetwork);
extern int WIFIAudio_SystemCommand_restoreToDefault(void);
extern int WIFIAudio_SystemCommand_reboot(void);
extern int WIFIAudio_SystemCommand_setDeviceName(char *pname);
extern int WIFIAudio_SystemCommand_setShutdown(int second);
extern WARTI_pStr WIFIAudio_SystemCommand_getShutdown(void);
extern WARTI_pStr WIFIAudio_SystemCommand_getManuFacturerInfo(void);
extern int WIFIAudio_SystemCommand_setPowerWifiDown(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetTheLatestVersionNumberOfFirmwareTopStr(void);
extern int WIFIAudio_SystemCommand_getMvRemoteUpdate(char *purl);
extern int WIFIAudio_SystemCommand_setTimeSync(WAGC_pDateTime_t pdatetime);
extern WARTI_pStr WIFIAudio_SystemCommand_getDeviceTimeTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_getAlarmClockTopStr(int num);
extern int WIFIAudio_SystemCommand_alarmStop(void);
extern int WIFIAudio_SystemCommand_setPlayerCmd_switchmode(int mode);
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_switchmodeTopStr(void);
extern int WIFIAudio_SystemCommand_SelectLanguage(int language);
extern WARTI_pStr WIFIAudio_SystemCommand_getLanguageTopStr(void);
extern char* WIFIAudio_SystemCommand_GetCurrentPlaylistTopJsonString(void);
extern int WIFIAudio_SystemCommand_DeletePlaylistMusic(int index);
extern int WIFIAudio_SystemCommand_xzxAction_setrt(int value);
extern int WIFIAudio_SystemCommand_xzxAction_playrec(void);
extern int WIFIAudio_SystemCommand_xzxAction_playurlrec(char *precurl);
extern WARTI_pStr WIFIAudio_SystemCommand_GetMicarrayInfoTopStr(void);
extern int WIFIAudio_SystemCommand_MicarrayRecord(int value);
extern WARTI_pStr WIFIAudio_SystemCommand_MicarrayNormalTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_MicarrayMicTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_MicarrayRefTopStr(void);
extern int WIFIAudio_SystemCommand_MicarrayStop(void);
extern WARTI_pStr WIFIAudio_SystemCommand_StorageDeviceOnlineStateTopStr(void);
extern char* WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString(void);
extern int WIFIAudio_SystemCommand_PlayTfCard(int index);
extern char* WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString(void);
extern int WIFIAudio_SystemCommand_PlayUsbDisk(int index);
extern WARTI_pStr WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr(void);
extern int WIFIAudio_SystemCommand_RemoveSynchronousEx(char *pmac);
extern int WIFIAudio_SystemCommand_SetSynchronousEx_vol(WAGC_pSetSynchronousEx_t psetsynchronousex);
extern int WIFIAudio_SystemCommand_xzxAction_startrecord(void);
extern int WIFIAudio_SystemCommand_xzxAction_stoprecord(void);
extern int WIFIAudio_SystemCommand_xzxAction_fixedrecord(void);
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr(void);
extern int WIFIAudio_SystemCommand_xzxAction_vad(WAGC_pxzxAction_t pxzxaction);
extern int WIFIAudio_SystemCommand_Bluetooth_Search(void);
extern WARTI_pStr WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr(void);
extern int WIFIAudio_SystemCommand_Bluetooth_Connect(char *paddress);
extern int WIFIAudio_SystemCommand_Bluetooth_Disconnect(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr(void);
extern int WIFIAudio_SystemCommand_SetSpeechRecognition(int platform);
extern int WIFIAudio_SystemCommand_SetSingleLight(WAGC_pSetSingleLight_t pSetSingleLight);
extern WARTI_pStr WIFIAudio_SystemCommand_GetAlexaLanguageTopStr(void);
extern int WIFIAudio_SystemCommand_SetAlexaLanguage(char* pLanguage);
extern int WIFIAudio_SystemCommand_UpdataConexant(void);
extern int WIFIAudio_SystemCommand_SetTestMode(int mode);
extern WARTI_pStr WIFIAudio_SystemCommand_GetTestModeTopStr(void);
extern int WIFIAudio_SystemCommand_PlayTestMusic(char *purl);
extern int WIFIAudio_SystemCommand_StopTestMusic(void);
extern WARTI_pStr WIFIAudio_SystemCommand_LogCaptureEnable(void);
extern int WIFIAudio_SystemCommand_LogCaptureDisable(void);
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr(void);
extern int WIFIAudio_SystemCommand_ChangeNetworkConnect(void);
extern int WIFIAudio_SystemCommand_xzxAction_normalrecord(int second);
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr(void);
extern int WIFIAudio_SystemCommand_xzxAction_micrecord(int second);
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr(void);
extern int WIFIAudio_SystemCommand_xzxAction_refrecord(int second);
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr(void);
extern int WIFIAudio_SystemCommand_xzxAction_stopModeRecord(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetDownloadProgressTopStr(void);
extern WARTI_pStr WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr(void);
extern int WIFIAudio_SystemCommand_DisconnectWiFi(void);
extern int WIFIAudio_SystemCommand_PowerOnAutoPlay(int enable);
extern int WIFIAudio_SystemCommand_RemoveAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern int WIFIAudio_SystemCommand_EnableAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern int WIFIAudio_SystemCommand_RemovePlayPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern char* WIFIAudio_SystemCommand_GetAlarmClockAndPlayPlanTopJsonString(void);
extern char* WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern int WIFIAudio_SystemCommand_EnablePlayMusicAsPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern char* WIFIAudio_SystemCommand_GetPlanMixedListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan);
extern int WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth(char *purl);
extern WARTI_pStr WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr(void);
extern int WIFIAudio_SystemCommand_RemoveShortcutKeyList(int key);
extern char* WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString(int key);
extern int WIFIAudio_SystemCommand_EnableAutoTimeSync(int enable);
extern WARTI_pStr WIFIAudio_SystemCommand_LogCaptureEnableTopStr(void);
extern int WIFIAudio_SystemCommand_LogCaptureDisable(void);

#ifdef __cplusplus
}
#endif

#endif
