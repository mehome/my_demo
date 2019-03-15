#ifndef __WIFIAUDIO_SYSTEMCOMMAND_H__
#define __WIFIAUDIO_SYSTEMCOMMAND_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <netinet/in.h>
#include <arpa/inet.h>//IP地址转换所需要的头文件

#include <net/ethernet.h>//ether_addr（mac帧结构）
#include <netinet/ether.h>//mac地址转换的函数定义

#include "WIFIAudio_RealTimeInterface.h"
#include "WIFIAudio_GetCommand.h"
#include "WIFIAudio_FifoCommunication.h"
#include "WIFIAudio_UciConfig.h"
#include "WIFIAudio_Semaphore.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_LightControlFormat.h"
#include "WIFIAudio_LightControlUart.h"
#include "WIFIAudio_LightControl.h"
#include "WIFIAudio_ChannelCompare.h"



/******************************************
 ** 函数名: WIFIAudio_SystemCommand_WlanGetApListTopStr
 ** 功  能: 获取当前环境可以连接的wifi信息，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_WlanGetApListTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_wlanConnectAp
 ** 功  能: 将设备连接到一个wifi
 ** 参  数: 一个指向WAGC_ConnectAp_t的指针，内含所需连接wifi的信息
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_wlanConnectAp(WAGC_pConnectAp_t pconnectap);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr
 ** 功  能: 查询当前连接路由器，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetCurrentWirelessConnectTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetDeviceInfo
 ** 功  能: 获取当前设备信息，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetDeviceInfo(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getOneTouchConfigure
 ** 功  能: 获取一键配置网络状态，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigure(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setOneTouchConfigure
 ** 功  能: 设置一键配置网络状态
 ** 参  数: 设置的状态值
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setOneTouchConfigure(int value);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName
 ** 功  能: 获取一键配置网络重命名状态，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getOneTouchConfigureSetDeviceName(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName
 ** 功  能: 设置一键配置网络重命名状态
 ** 参  数: 设置的状态值
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setOneTouchConfigureSetDeviceName(int value);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getLoginAmazonStatus
 ** 功  能: 查询亚马逊登录的状态
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonStatus(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getLoginAmazonNeedInfo
 ** 功  能: 获取亚马逊登录所需要的信息
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getLoginAmazonNeedInfo(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_LogoutAmazon
 ** 功  能: 登出亚马逊
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_LogoutAmazon(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetPlayerStatusTopStr
 ** 功  能: 查询当前设备的播放状态，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetPlayerStatusTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_playlist
 ** 功  能: 配合推送列表，播放当前播放列表的第num首歌，num从1开始
 ** 参  数: 播放列表当中的偏移量，从1开始
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_playlist(int num);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_pause
 ** 功  能: 暂停当前音乐播放
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_pause(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_play
 ** 功  能: 暂停后，重新播放
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_play(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_prev
 ** 功  能: 播放上一首
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_prev(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_next
 ** 功  能: 播放下一首
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_next(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_seek
 ** 功  能: 快进快退
 ** 参  数: num为0到音乐播放的完整时间，单位是秒
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_seek(int num);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_stop
 ** 功  能: 停止当前播放
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_stop(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_vol
 ** 功  能: 设置音量
 ** 参  数: num是音量值，从0到100
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_vol(int num);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getPlayerCmd_volTopStr
 ** 功  能: 获取音量，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_volTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_mute
 ** 功  能: 设置是否静音，1为静音，0为非静音
 ** 参  数: flag为1时设置为静音，0为非静音
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_mute(int flag);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr
 ** 功  能: 获取静音设置，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_muteTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_loopmode
 ** 功  能: 设置循环播放模式
 ** 参  数: num为设定循环模式
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_loopmode(int num);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_equalizer
 ** 功  能: 设置均衡器模式
 ** 参  数: mode为均衡器模式
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_equalizer(int mode);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getEqualizerTopStr
 ** 功  能: 查询均衡器，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getEqualizerTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_slave_channel
 ** 功  能: 主音箱声道设置
 ** 参  数: value为声道，0为正常播放，1为只放左声道，2为只放右声道
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_slave_channel(int value);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getPlayerCmd_slave_channel
 ** 功  能: 获取音箱声道
 ** 参  数: 无
 ** 返回值: 返回音箱声道值，0为正常播放，1为只放左声道，2为只放右声道，其他值为获取失败
 ******************************************/ 
extern int WIFIAudio_SystemCommand_getPlayerCmd_slave_channel(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setSSID
 ** 功  能: 修改设备ssid名称，后台实现
 ** 参  数: 指向新ssid名称的字符串指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setSSID(char *pssid);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setNetwork
 ** 功  能: 修改设备wifi密码，后台实现
 ** 参  数: 指向WAGC_Network_t结构体的指针，内含密码和是否设置密码标志信息
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setNetwork(WAGC_pNetwork_t pnetwork);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_restoreToDefault
 ** 功  能: 恢复出厂设置，删除所有用户设置，后台实现
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_restoreToDefault(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_reboot
 ** 功  能: 设备重启，后台实现
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_reboot(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setDeviceName
 ** 功  能: 设置设备名
 ** 参  数: pname为指向新设备名的字符串指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setDeviceName(char *pname);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setShutdown
 ** 功  能: 指定时间关闭设备，可能要在后台实现
 ** 参  数: second为指定的秒数，之后关机，0为立即关机，-1为取消关机
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setShutdown(int second);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getShutdown
 ** 功  能: 获取关闭设备的时间，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getShutdown(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getManuFacturerInfo
 ** 功  能: 获取制造商信息，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getManuFacturerInfo(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPowerWifiDown
 ** 功  能: 关闭设备wifi，可能要在后台实现
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPowerWifiDown(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetTheLatestVersionNumberOfFirmwareTopStr
 ** 功  能: 获取最新固件版本号，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetTheLatestVersionNumberOfFirmwareTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getMvRemoteUpdate
 ** 功  能: 设备固件升级，如果有固件就下载并烧写，后台实现
 ** 参  数: 指向用于升级设备的固件url字符串指针
 ** 返回值: 固件存在返回0，固件不存在返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_getMvRemoteUpdate(char *purl);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setTimeSync
 ** 功  能: 设备时间同步
 ** 参  数: 指向WAGC_DateTime_t结构体的指针，内含同步时间信息
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setTimeSync(WAGC_pDateTime_t pdatetime);

/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getDeviceTimeTopStr
 ** 功  能: 获取设备时间
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getDeviceTimeTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getAlarmClockTopStr
 ** 功  能: 获取定时器信息，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: num为第几个定时器，从1开始
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getAlarmClockTopStr(int num);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_alarmStop
 ** 功  能: 停止当前闹钟播放
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_alarmStop(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_setPlayerCmd_switchmode
 ** 功  能: 输入源切换
 ** 参  数: mode为输入源模式
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_setPlayerCmd_switchmode(int mode);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getPlayerCmd_switchmodeTopStr
 ** 功  能: 获取输入源，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getPlayerCmd_switchmodeTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SelectLanguage
 ** 功  能: 语音提示语种选择
 ** 参  数: language为语种
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SelectLanguage(int language);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_getLanguageTopStr
 ** 功  能: 获取语音提示语种，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_getLanguageTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetCurrentPlaylistTopJsonString
 ** 功  能: 获取当前播放列表，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetCurrentPlaylistTopJsonString(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_DeletePlaylistMusic
 ** 功  能: 删除当前音乐播放列表当中的音乐，index为列表当中的序号，从1开始
 ** 参  数: index为歌曲在列表当中的序号，从1开始
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_DeletePlaylistMusic(int index);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_setrt
 ** 功  能: 设置录音时间，单位为秒
 ** 参  数: value为秒数
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_setrt(int value);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_playrec
 ** 功  能: 播放录音文件
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_playrec(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_playurlrec
 ** 功  能: 临时播放url录音文件
 ** 参  数: 录音mp3文件的访问地址
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_playurlrec(char *precurl);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_StorageDeviceOnlineState
 ** 功  能: 获取外接存储设备在线状态，生成一个WARTI_Str结构体，返回指向它的指针
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_StorageDeviceOnlineStateTopStr(void);


/******************************************
/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString
 ** 功  能: 获取T卡播放列表，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetTfCardPlaylistInfoTopJsonString(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_PlayTfCard
 ** 功  能: 播放T卡歌曲，index为列表当中的序号，从1开始
 ** 参  数: index为歌曲在列表当中的序号，从1开始
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_PlayTfCard(int index);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString
 ** 功  能: 获取U盘播放列表，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetUsbDiskPlaylistInfoTopJsonString(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_PlayUsbDisk
 ** 功  能: 播放U盘歌曲，index为列表当中的序号，从1开始
 ** 参  数: index为歌曲在列表当中的序号，从1开始
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_PlayUsbDisk(int index);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr
 ** 功  能: 获取同步播放设备信息扩展
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetSynchronousInfoExTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_RemoveSynchronousEx
 ** 功  能: 解除从音箱同步扩展
 ** 参  数: mac地址的字符串指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_RemoveSynchronousEx(char *pmac);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SetSynchronousEx_vol
 ** 功  能: 调整子音箱音量扩展
 ** 参  数: 指向SetSynchronousEx结构体的指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SetSynchronousEx_vol(WAGC_pSetSynchronousEx_t psetsynchronousex);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_startrecord
 ** 功  能: 开始录音
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_startrecord(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_stoprecord
 ** 功  能: 停止录音
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_stoprecord(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_fixedrecord
 ** 功  能: 定长录音
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_fixedrecord(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr
 ** 功  能: 获取录音文件URL
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRecordFileURLTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_vad
 ** 功  能: 语音活动检测
 ** 参  数: 指向WAGC_xzxAction_t结构体的指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_vad(WAGC_pxzxAction_t pxzxaction);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_Bluetooth_Search
 ** 功  能: 搜索周边蓝牙设备
 ** 参  数: 指向WAGC_xzxAction_t结构体的指针
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_Bluetooth_Search(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr
 ** 功  能: 获取周边蓝牙设备信息
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_Bluetooth_GetDeviceListTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_Bluetooth_Connect
 ** 功  能: 连接蓝牙设备
 ** 参  数: 蓝牙设备地址
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_Bluetooth_Connect(char *paddress);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_Bluetooth_Disconnect
 ** 功  能: 断开蓝牙设备
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_Bluetooth_Disconnect(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr
 ** 功  能: 获取当前语音平台
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetSpeechRecognitionTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SetSpeechRecognition
 ** 功  能: 切换语音平台
 ** 参  数: 切换语音平台的类型
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SetSpeechRecognition(int platform);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SetSingleLight
 ** 功  能: 设置组灯
 ** 参  数: 设置组灯的各种数据
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SetSingleLight(WAGC_pSetSingleLight_t pSetSingleLight);

/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetAlexaLanguageTopStr
 ** 功  能: 查询Alexa语种
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetAlexaLanguageTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SetAlexaLanguage
 ** 功  能: 切换Alexa语种
 ** 参  数: 切换Alexa的语种
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SetAlexaLanguage(char* pLanguage);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_UpdataConexant
 ** 功  能: 开始升级Conexant
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_UpdataConexant(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_SetTestMode
 ** 功  能: 设置测试模式
 ** 参  数: 测试模式的使能位
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_SetTestMode(int mode);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr
 ** 功  能: 左右声道录音对比
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_ChannelCompareTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_ChangeNetworkConnect
 ** 功  能: 在收到UDP广播之后，网络标志为1，调用该接口将标志置为0
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_ChangeNetworkConnect(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_normalrecord
 ** 功  能: 开始普通录音
 ** 参  数: 录音秒数
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_normalrecord(int second);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr
 ** 功  能: 获取普通录音文件URL
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetNormalURLTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_micrecord
 ** 功  能: 开始mic录音
 ** 参  数: 录音秒数
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_micrecord(int second);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr
 ** 功  能: 获取mic录音文件URL
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetMicURLTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_refrecord
 ** 功  能: 开始参考信号录音
 ** 参  数: 录音秒数
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_refrecord(int second);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr
 ** 功  能: 获取参考信号录音文件URL
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_xzxAction_GetRefURLTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_xzxAction_stopModeRecord
 ** 功  能: 停止模式录音
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_xzxAction_stopModeRecord(void);

/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetDownloadProgressTopStr
 ** 功  能: 获取固件下载进度
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetDownloadProgressTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr
 ** 功  能: 获取蓝牙固件下载进度
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_GetBluetoothDownloadProgressTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_DisconnectWiFi
 ** 功  能: 断开当前网络连接
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_DisconnectWiFi(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_PowerOnAutoPlay
 ** 功  能: 设置开机自动播放音乐使能
 ** 参  数: 是否使能
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_PowerOnAutoPlay(int enable);

/******************************************
 ** 函数名: WIFIAudio_SystemCommand_RemoveAlarmClock
 ** 功  能: 删除闹钟
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_RemoveAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_EnableAlarmClock
 ** 功  能: 闹钟使能
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_EnableAlarmClock(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_RemovePlayPlan
 ** 功  能: 修改闹钟播放的内容
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_RemovePlayPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetAlarmClockAndPlayPlanTopJsonString
 ** 功  能: 查询闹钟和定时播放信息，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetAlarmClockAndPlayPlanTopJsonString(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString
 ** 功  能: 查询定时播放列表，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetPlanMusicListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_EnablePlayMusicAsPlan
 ** 功  能: 定时播放使能
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_EnablePlayMusicAsPlan(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetPlanMixedListTopJsonString
 ** 功  能: 查询定时播放混合列表，返回一个添加了ret:OK的json字符串
 ** 参  数: 无
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetPlanMixedListTopJsonString(WAGC_pAlarmAndPlan_t pAlarmAndPlan);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth
 ** 功  能: WiFi和蓝牙固件升级
 ** 参  数: 指向用于固件url字符串指针
 ** 返回值: 固件存在返回0，固件不存在返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_UpgradeWiFiAndBluetooth(char *purl);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr
 ** 功  能: 获取升级状态
 ** 参  数: 无
 ** 返回值: 成功返回指向WARTI_Str结构体指针，失败返回NULL
 ******************************************/ 
extern WARTI_pStr WIFIAudio_SystemCommand_UpgradeWiFiAndBluetoothStateTopStr(void);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_RemoveShortcutKeyList
 ** 功  能: 删除快捷键
 ** 参  数: 快捷键编号
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_RemoveShortcutKeyList(int key);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString
 ** 功  能: 查询快捷键，返回一个添加了ret:OK的json字符串
 ** 参  数: 快捷键编号
 ** 返回值: 成功返回指向Json字符串的指针，失败返回NULL
 ******************************************/ 
extern char* WIFIAudio_SystemCommand_GetShortcutKeyListTopJsonString(int key);


/******************************************
 ** 函数名: WIFIAudio_SystemCommand_EnableAutoTimeSync
 ** 功  能: 设置联网自动同步时间使能
 ** 参  数: 是否使能
 ** 返回值: 成功返回0，失败返回其他值
 ******************************************/ 
extern int WIFIAudio_SystemCommand_EnableAutoTimeSync(int enable);


#ifdef __cplusplus
}
#endif

#endif
