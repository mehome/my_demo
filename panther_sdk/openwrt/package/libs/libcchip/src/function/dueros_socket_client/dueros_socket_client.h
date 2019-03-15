#ifndef _DUEROS_SOCKET_CLIENT_H_
#define _DUEROS_SOCKET_CLIENT_H_
#include <stdarg.h>

#define UNIX_DOMAIN "/tmp/UNIX.baidu"

enum DeviceInput {
	DEVICEINPUT_MIN=-1,
    KEY_ONE_SHORT,
    KEY_ONE_LONG,
    KEY_ONE_DOUBLE,
    KEY_ONE_LONG_10S,
    KEY_BLUETOOTH_SHORT,
    KEY_BLUETOOTH_LONG = 5,
    KEY_MIC_MUTE,
    SLIDER_PRESSED,
    SLIDER_RELEASE,
    VOLUME_CHANGED,

    // for mtk devkit
    KEY_VOLUME_UP = 10,
    KEY_VOLUME_DOWN,
    KEY_VOLUME_MUTE,
    KEY_WAKE_UP,
    KEY_ENTER_AP,
    KEY_EXIT_AP = 15,

    //蓝牙等待配对
    BT_WAIT_PAIR,
    //蓝牙配对中
    BT_DO_PAIR,
    //蓝牙已被绑定,请解绑后再绑定其他设备
    BT_PAIR_FAILED_PAIRED,
    //配对失败,请重试
    BT_PAIR_FAILED_OTHER,
    //蓝牙配对成功
    BT_PAIR_SUCCESS = 20,
    //手机关闭蓝牙蓝牙断开
    BT_DISCONNECT,
    //蓝牙开始播放
    BT_START_PLAY,
    //蓝牙结束播放
    BT_STOP_PLAY,
    //ble
    BLE_CLIENT_CONNECT,
    BLE_CLIENT_DISCONNECT = 25,
    BLE_SERVER_RECV,
    //BT CALL
    BT_HFP_AUDIO_CONNECT,
    BT_HFP_AUDIO_DISCONNECT,
    BT_POWER_OFF,

    KEY_CLOSE_TIMER_ALARM = 30,
    KEY_IS_SLEEP_STATE,
    KEY_PLAY_PAUSE,

    KEY_SHUT_DOWN,
    KEY_SLEEP_MODE,
	KEY_RT_VOLUME,	
	KEY_PLAYBACK_NEXT = 36,
	KEY_PLAYBACK_PREV = 37,
	KEY_PRESS_WAKEUP,
	DEVICEINPUT_MAX
};


int send_button_direction_to_dueros(int sum, int src, ...);
int send_player_url_to_dueros(int sum, const char *src, ...);
int send_data_to_dueros(const void *data, unsigned int len);
int send_to_dueros(const void *data, int len);
#endif
