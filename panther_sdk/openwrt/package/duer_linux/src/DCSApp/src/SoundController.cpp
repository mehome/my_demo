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

#include "DCSApp/SoundController.h"
#include "DCSApp/ActivityMonitorSingleton.h"
#include "DCSApp/DeviceIoWrapper.h"
extern "C" {
	#include <libcchip/platform.h>
}


namespace duerOSDcsApp {
namespace application {

using namespace duerOSDcsSDK::sdkInterfaces;

SoundController *SoundController::mSoundController = NULL;
pthread_once_t SoundController::m_initOnce = PTHREAD_ONCE_INIT;
pthread_once_t SoundController::m_destroyOnce = PTHREAD_ONCE_INIT;

SoundController::SoundController() {
    pthread_mutex_init(&m_mutex, NULL);
    m_destroyOnce = PTHREAD_ONCE_INIT;
    m_pcmPlayer = new mediaPlayer::RapidPcmPlayer();
}

SoundController::~SoundController() {
    pthread_mutex_destroy(&m_mutex);
    m_initOnce = PTHREAD_ONCE_INIT;
}

SoundController *SoundController::getInstance() {
    pthread_once(&m_initOnce, &SoundController::init);
    return mSoundController;
}

void SoundController::addObserver(std::shared_ptr<LocalSourcePlayerInterface> mediaInterface) {
    if (mediaInterface) {
        m_observers.insert(mediaInterface);
    }
}

void SoundController::init() {
    if (mSoundController == NULL) {
        mSoundController = new SoundController();
    }
}

void SoundController::destory() {
    if (mSoundController) {
        delete mSoundController;
        mSoundController = NULL;
    }
}

void SoundController::release() {
    pthread_once(&m_destroyOnce, SoundController::destory);
}

void SoundController::audioPlay(const std::string &source,
                                bool needFocus,
                                void (*start_callback)(void *arg),
                                void *start_cb_arg,
                                void (*finish_callback)()) {
    for (auto observer : m_observers) {
        if (observer) {
            observer->playLocalSource(source, needFocus, start_callback, start_cb_arg, finish_callback);
        }
    }
}
static const char *wakeUpSoundList[]={
	"./appresources/en.mp3",
	"./appresources/here.mp3",
	"./appresources/hereNi.mp3",
	"./appresources/iAmHere.mp3"
};

void SoundController::wakeUp() {
#if 1
	 static unsigned int idx=0;
	 #define LIST_COUNT (sizeof(wakeUpSoundList)/sizeof(char *))
	 idx++;idx%=LIST_COUNT;
	 inf("LIST_COUNT=%u,idx=%u",LIST_COUNT,idx);
	 audioPlay(wakeUpSoundList[idx], false,NULL, NULL, NULL);
#else
    audioPlay("./appresources/du.mp3", false, NULL, NULL, NULL);
#endif
}
void SoundController::commonStartCB(void *arg){
	uartd_fifo_write_fmt("duerSceneLedSet101");
//	uartd_fifo_write_fmt("duerSceneLedSet%03u",(unsigned int)duerScene_t::TIP_SOUND_START);
}

void SoundController::commonFinishCB(void){
	uartd_fifo_write_fmt("duerSceneLedSet102");
//	uartd_fifo_write_fmt("duerSceneLedSet%03u",(unsigned int)duerScene_t::TIP_SOUND_END);
}

void SoundController::linkStartFirst() {
	err(__func__);
    audioPlay("./appresources/link_start_first.mp3", true,
		 DeviceIoWrapper::getInstance()->led_first_poweron, NULL, commonFinishCB);
}
#if 0
void SoundController::linkStart() {
    audioPlay("./appresources/link_start.mp3", true, 
		DeviceIoWrapper::getInstance()->led_connect_start, NULL, NULL);
}
#endif

void SoundController::linkConnecting() {
    audioPlay("./appresources/link_connecting.mp3", true, 
		DeviceIoWrapper::getInstance()->led_connecting, NULL, NULL);
}

void SoundController::linkSuccess(void(*callback)()) {
    audioPlay("./appresources/link_success.mp3", true,
		 DeviceIoWrapper::getInstance()->led_connect_succeed, NULL, commonFinishCB);
}

void SoundController::linkFailedPing(void(*callback)()) {
    audioPlay("./appresources/link_failed_ping.mp3", true,
 		 DeviceIoWrapper::getInstance()->led_network_fault, NULL, commonFinishCB);
}

void SoundController::linkFailedIp(void(*callback)()) {
    audioPlay("./appresources/link_failed_ip.mp3", true, 
		 DeviceIoWrapper::getInstance()->led_connection_routing_failed, NULL, commonFinishCB);
}

void SoundController::linkExit(void(*callback)()) {
    audioPlay("./appresources/link_exit.mp3", true, NULL, NULL, callback);
}

void SoundController::reLink() {
    audioPlay("./appresources/re_link.mp3", true, 
	 DeviceIoWrapper::getInstance()->led_connecting, NULL, NULL);
}

void SoundController::reLinkSuccess(void(*callback)()) {
    audioPlay("./appresources/re_link_success.mp3", true,
		DeviceIoWrapper::getInstance()->led_connect_succeed, NULL, commonFinishCB);
}

void SoundController::reLinkFailed() {
    audioPlay("./appresources/re_link_failed.mp3", true, 
		DeviceIoWrapper::getInstance()->led_network_fault, NULL, commonFinishCB);
}

void SoundController::btUnpaired() {
    audioPlay("./appresources/btb_unpaired.mp3", true, NULL, NULL, NULL);
}

void SoundController::btPairSuccess(void(*callback)()) {
    audioPlay("./appresources/bt_pair_success.mp3", true, NULL, NULL, callback);
}

void SoundController::btPairFailedPaired() {
    audioPlay("./appresources/bt_pair_failed_paired.mp3", true, NULL, NULL, NULL);
}

void SoundController::btPairFailedOther() {
    audioPlay("./appresources/bt_pair_failed_other.mp3", true, NULL, NULL, NULL);
}

void SoundController::btDisconnect(void(*callback)()) {
    audioPlay("./appresources/bt_disconnect.mp3", true, NULL, NULL, callback);
}

void SoundController::networkConnectFailed() {
    audioPlay("./appresources/network_connect_failed.mp3", true,  
		DeviceIoWrapper::getInstance()->led_network_exception, NULL, commonFinishCB);
}

void SoundController::networkSlow() {
    audioPlay("./appresources/network_slow.mp3", false, 
		 DeviceIoWrapper::getInstance()->led_slow_network, NULL, NULL);
}

void SoundController::openBluetooth(void(*callback)(void *arg), void *arg) {
    audioPlay("./appresources/open_bluetooth.mp3", true, callback, arg, NULL);
}

void SoundController::closeBluetooth(void(*callback)(void *arg), void *arg) {
    audioPlay("./appresources/close_bluetooth.mp3", true, callback, arg, NULL);
}

void SoundController::volume() {
    m_pcmPlayer->stop();
    m_pcmPlayer->play();
    ActivityMonitorSingleton::getInstance()->updateActiveTimestamp();
}

void SoundController::playTts(const std::string& content, bool needFocus, void (*callback)()) {
    for (auto observer : m_observers) {
        if (observer) {
            observer->playTts(content, needFocus, callback);
        }
    }
}

void SoundController::serverConnecting() {
    audioPlay("./appresources/server_connecting.mp3", false, NULL, NULL, NULL);
}

void SoundController::serverConnectFailed() {
    audioPlay("./appresources/server_connect_failed.mp3", false, NULL, NULL, NULL);
}

void SoundController::bleNetworkConfig() {
    audioPlay("./appresources/ble_network_config.mp3", false, NULL, NULL, NULL);
}

void SoundController::accountUnbound(void(*callback)()) {
    audioPlay("./appresources/unbound.mp3", true, 
		DeviceIoWrapper::getInstance()->led_login_failure,NULL,commonFinishCB);
}

void SoundController::hotConnected() {
    audioPlay("./appresources/hot_connected.mp3",false, NULL, NULL, NULL);
}

void SoundController::waitLogin() {
    audioPlay("./appresources/wait_login.mp3", false, NULL, NULL, NULL);
}

void SoundController::muteOn(){
    audioPlay("./appresources/mute_on.mp3", false, NULL, NULL, NULL);
}

void SoundController::muteOff(){
    audioPlay("./appresources/mute_off.mp3", false, NULL, NULL, NULL);
}

void SoundController::network_fail_invalid_password(){
    audioPlay("./appresources/invalid_password.mp3", false, NULL, NULL, NULL);
}

void SoundController::network_fail_connect_router_failed(){
    audioPlay("./appresources/connect_router_failed.mp3", false, NULL, NULL, NULL);
}

}  // namespace application
}  // namespace duerOSDcsApp
