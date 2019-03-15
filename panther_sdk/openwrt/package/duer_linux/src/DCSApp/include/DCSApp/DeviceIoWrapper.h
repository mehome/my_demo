/*
 * Copyright (c) 2017 Baidu, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef DUEROS_DCS_APP_SAMPLEAPP_DEVICEIOWRAPPER_H_
#define DUEROS_DCS_APP_SAMPLEAPP_DEVICEIOWRAPPER_H_

#include <DeviceIo/DeviceIo.h>
#include "ApplicationManager.h"
#include "DeviceTools/Timer.h"

namespace duerOSDcsApp {
namespace application {

enum class duerScene_t{
	MIN=-1,
	FIRST_POWERON,
	CONNECT_START,
	CONNECTING,
	CONNECT_SUCCEED,
	WRONG_PASSWORD,
	UNSUPPORTED_ROUTE,
	CONNECTION_ROUTING_FAILED,
	DEVICE_BOOT,
	DEVICE_REBOOT,
	FACTORY_SET,
	TURN_DOWN_THE_VOLUME,
	PARDON_AGAIN,
	NETWORK_FAULT,
	BINDING_SUCCESS,
	NETWORK_EXCEPTION,
	SLOW_NETWORK,
	LOGIN_FAILURE,
	WAIT_MOMENT,
	IDEL,
	WAKEUP,
	LISTENING,
	THINKING,
	TALKING,
	PLAY_LISTEN,
	PLAY_SILENCE,
	ENTER_CONFIG_NET_MODE,	
	TIP_SOUND_START=101,
	TIP_SOUND_END=102,
	MAX,
};

class DeviceIoWrapperObserverInterface {
public:
    virtual void touchStartAsr() {};

    virtual void btStartPlay() {};

    virtual void btStopPlay() {};

    virtual void btDisconnect() {};

    virtual void btPowerOff() {};
};

enum TouchEvent {
    TOUCH_PRESSED,
    TOUCH_RELEASED
};

class DeviceIoWrapper : public framework::DeviceInNotify {
public:
    static DeviceIoWrapper *getInstance();

    void release();

    void callback(framework::DeviceInput event, void *data, int len);

    bool isBluetoothOpen();

    bool isBluetoothConnected();

    void openBluetooth();

    void closeBluetooth();

    void unpairBluetooth();

    void transmitAudioToBluetooth(const char *audioData,
                                  unsigned int len);

    int fetchWakeupDirection();

	static void led_first_poweron(void* args);
	static void led_connect_start(void* args);
	static void led_connecting(void* args);
	static void led_connect_succeed(void* args);
	static void led_wrong_password(void* args);
	static void led_unsupported_route(void* args);
	static void led_connection_routing_failed(void* args);
	static void led_device_boot(void* args);
	static void led_device_reboot(void* args);
	static void led_factory_set(void* args);
	static void led_turn_down_the_volume(void* args);
	static void led_pardon_again(void* args);
	static void led_network_fault(void* args);
	static void led_binding_success(void* args);
	static void led_network_exception(void* args);
	static void led_slow_network(void* args);
	static void led_login_failure(void* args);
	static void led_wait_moment(void* args);
	static void led_idel(void* args);
	static void led_wakeup(void* args);
	static void led_listening(void* args);
	static void led_thinking(void* args);
	static void led_talking(void* args);
	static void led_play_listen(void* args);
	static void led_play_silence(void* args);
	static void led_tip_sound_start(void* args);
	static void led_tip_sound_end(void* args);
	static void led_enter_config_net_mode(void* args);

#if 1
    void ledWakeUp(int angle);

    void ledTts();

    void ledAsr();

    void ledMute();

    void ledBtOff();

    void ledBtClose();

    void ledSpeechOff();

    void ledMuteOff();

    void ledNetOff();

    //等待联网
    void ledNetWaitConnect();
    //联网中
    void ledNetDoConnect();
    //联网成功
    void ledNetConnectSuccess();
    //断网灯效
    void ledNetConnectFailed();

    void ledNetRecoveryConnect();

    void ledNetWaitLogin();

    void ledNetLoginFailed();

    //蓝牙等待配对
    void ledBtWaitPair();
#endif
    int getCurrentVolume() const;

    void setCurrentVolume(int currentVolume);

    int getAlertVolume() const;

    void setAlertVolume(int volume);

    bool isMute() const;

    int setMute(bool mute);

//    void ledVolume();

    //恢复蓝牙播放
    void btResumePlay();

    //暂停蓝牙播放
    void btPausePlay();

    //蓝牙播放上一个
    void btPlayPrevious();

    //蓝牙播放下一个
    void btPlayNext();

    std::string getDeviceId();

    std::string getVersion();

    /// set ApplicationManager
    void setApplicationManager(std::shared_ptr<ApplicationManager> applicationManager);

    void cancelMusic();

    /// exit mute model
    void exitMute();

    /// request mute event
    void muteChanged();

    /// request volume event
    void volumeChanged();

    /// button control microphone
    void handleMicrophone();

    bool isBtPlaying();

    static void deviceioWrapperBtSoundPlayFinished();

    static void deviceioWrapperBtDisconnectSoundPlayFinished();

    int getDirection() const;

    void setDirection(int direction);

    /**
     * Adds an observer to be notified of DeviceIo changes.
     *
     * @param observer The new observer to notify of DeviceIo changes.
     */
    void addObserver(std::shared_ptr<DeviceIoWrapperObserverInterface> observer);

    /**
     * Removes an observer from the internal collection of observers synchronously. If the observer is not present,
     * nothing will happen.
     *
     * @param observer The observer to remove.
     */
    void removeObserver(std::shared_ptr<DeviceIoWrapperObserverInterface> observer);

//    void ledAlarm();

//    void ledAlarmOff();

//    void ledOtaDoing();

    void setTouchStartNetworkConfig(bool isTouchStartNetworkConfig);

    bool isTouchStartNetworkConfig() const;

    void setAlertRing(bool isAlertRing);

    bool isAlertRing();

    void wakeupVolumeDown();

    void wakeupVolumeUp();

    std::string getBtMac();

    std::string getWifiMac();

    std::string getWifiIp();

    std::string getWifiSsid();

    std::string getWifiBssid();

    bool isRecognizing() const;

    void setRecognizing(bool isRecognizing);

    void initCommonVolume(int volume);

    void initAlertVolume(int volume);

    void setRecoveryingNetwork(bool isRecoveryingNetwork);

    bool isBleOpened();

    void closeLocalActiveAlert();

    bool isSleepMode() const;

    void enterSleepMode(bool isLedLightOn);

    void exitSleepMode();

    void handleSleepMode();

    void ledSleepMode();

    void ledUnSleepMode();

    bool isPhoneConnected() const;

    void timeToTenMinutes();

    int transmitInfrared(std::string& infraredCode);

    bool isPlayAndPause() const;

private:
    DeviceIoWrapper();

    ~DeviceIoWrapper();

    static void init();

    static void destroy();

    void handleVolumeMute();

    void handleVolumeDown(int dec);

    void handleVolumeUp(int inc);

    void handleWakeUp();

    //button control bluetooth
    void handleBluetooth();

    void handleTouchEvent(TouchEvent event);

    void handleSliderSlide(int incre);
    void handleSetRTvolume(int volume);

    static void staticFunc(void *arg);

    void setSleepMode(bool isSleepMode);

    void setPlayAndPause(bool isPlayAndPause);

    void handlePlayAndPause();

    static DeviceIoWrapper *s_deviceIoWrapper;

    static pthread_once_t s_initOnce;

    static pthread_once_t s_destroyOnce;

    int m_vloumeWhenPressed;

    bool m_btPhoneConnectFlag;

    bool m_isTouchStartNetworkConfig;

    //标识蓝牙是否正在播放
    bool m_isBtPlaying;

    std::shared_ptr<ApplicationManager> m_applicationManager;

    /// 角度记录
    int m_direction;

    char *m_btTransBuffer;

    int m_btBufferPos;

    /// The @c DeviceIoWrapperObserverInterface to notify DeviceIo change.
    std::unordered_set<std::shared_ptr<DeviceIoWrapperObserverInterface>> m_observers;

    bool m_isAlertRing;

    /// is_recognizing
    bool m_isRecognizing;

    ///@ is recovery network
    bool m_isRecoveryingNetwork;

    deviceCommonLib::deviceTools::Timer m_scheduledTimer;

    bool m_sleepMode;

    bool m_isPlayAndPause;
};

}  // namespace application
}  // namespace duerOSDcsApp

#endif  // DUEROS_DCS_APP_SAMPLEAPP_DEVICEIOWRAPPER_H_
