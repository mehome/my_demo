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

#include <fstream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <vector>
#include <string>
#include "LoggerUtils/DcsSdkLogger.h"
#include "DeviceIo/DeviceIo.h"
#include "DCSApp/DuerLinkWrapper.h"
#include "DCSApp/DeviceIoWrapper.h"
#include "DCSApp/SoundController.h"
#include "DCSApp/Configuration.h"

namespace duerOSDcsApp {
namespace application {

using namespace duerLink;
using std::string;
using std::vector;

DuerLinkWrapper *DuerLinkWrapper::s_duerLinkWrapper = NULL;
pthread_once_t  DuerLinkWrapper::s_initOnce = PTHREAD_ONCE_INIT;
pthread_once_t  DuerLinkWrapper::s_destroyOnce = PTHREAD_ONCE_INIT;

DuerLinkWrapper::DuerLinkWrapper() : m_networkStatus{DUERLINK_NETWORK_SUCCEEDED},
                                     m_isLoopNetworkConfig{false},
                                     m_isNetworkOnline{false},
                                     m_isFirstNetworkReady{true},
                                     m_isFromConfigNetwork{false} {
    s_destroyOnce = PTHREAD_ONCE_INIT;
}

DuerLinkWrapper::~DuerLinkWrapper() {
    s_initOnce = PTHREAD_ONCE_INIT;
}

DuerLinkWrapper* DuerLinkWrapper::getInstance() {
    pthread_once(&s_initOnce, &DuerLinkWrapper::init);
    return s_duerLinkWrapper;
}

void DuerLinkWrapper::init() {
    if (s_duerLinkWrapper == nullptr) {
        s_duerLinkWrapper = new DuerLinkWrapper();
    }
}

void DuerLinkWrapper::destroy() {
    delete s_duerLinkWrapper;
    s_duerLinkWrapper = nullptr;
}

void DuerLinkWrapper::release() {
    pthread_once(&s_destroyOnce, DuerLinkWrapper::destroy);
}

void DuerLinkWrapper::setCallback(IDuerLinkWrapperCallback *callback) {
    m_callback = callback;
}

void DuerLinkWrapper::initDuerLink() {

    DuerLinkCchipInstance::get_instance()->init_config_network_parameter(duerLink::platform_type::EHodor,
                                                                       duerLink::auto_config_network_type::EAll,
                                                                       DeviceIoWrapper::getInstance()->getDeviceId(),
                                                                       "");

    DuerLinkCchipInstance::get_instance()->set_networkConfig_observer(this);

    DuerLinkCchipInstance::get_instance()->set_monitor_observer(this);

    DuerLinkCchipInstance::get_instance()->init_softAp_env_cb();

    DuerLinkCchipInstance::get_instance()->init_ble_env_cb();

    DuerLinkCchipInstance::get_instance()->init_wifi_connect_cb();
}

void DuerLinkWrapper::startNetworkRecovery() {
    DuerLinkCchipInstance::get_instance()->start_network_recovery();
    DuerLinkCchipInstance::get_instance()->start_network_monitor();
}

bool DuerLinkWrapper::startNetworkConfig() {
	inf("========================startNetworkConfig==========================");
    return DuerLinkCchipInstance::get_instance()->start_network_config(3*60);
}

void DuerLinkWrapper::stopNetworkConfig() {
	inf("========================stopNetworkConfig==========================");
    DuerLinkCchipInstance::get_instance()->stop_network_config();
}

void DuerLinkWrapper::bleClientConnected() {
    DuerLinkCchipInstance::get_instance()->ble_client_connected();
}

void DuerLinkWrapper::bleClientDisconnected() {
    DuerLinkCchipInstance::get_instance()->ble_client_disconnected();
}

bool DuerLinkWrapper::bleRecvData(void *data, int len) {
    return DuerLinkCchipInstance::get_instance()->ble_recv_data(data, len);
}

DuerLinkNetworkStatus DuerLinkWrapper::getNetworkStatus() const {
    return m_networkStatus;
}

void DuerLinkWrapper::setNetworkStatus(DuerLinkNetworkStatus networkStatus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    DuerLinkWrapper::m_networkStatus = networkStatus;
}

void DuerLinkWrapper::verifyNetworkStatus(bool wakeupTrigger) {
    APP_INFO("verifyNetworkStatus wakeupTrigger:%d", wakeupTrigger);
    DuerLinkCchipInstance::get_instance()->ping_network(true);
}

void DuerLinkWrapper::notify_network_config_status(duerLink::notify_network_status_type notify_type) {

    APP_INFO("notify_network_config_status: %d", DuerLinkCchipInstance::get_instance()->get_operation_type());

    switch (notify_type) {
        case duerLink::ENetworkConfigStarted: {
            APP_INFO("notify_network_config_status: ENetworkConfigStarted=====");			
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(true);
            setNetworkStatus(DUERLINK_NETWORK_CONFIG_STARTED);
            DeviceIoWrapper::getInstance()->exitMute();
//            DeviceIoWrapper::getInstance()->ledNetWaitConnect();
            if (!m_isLoopNetworkConfig) {
                if (DuerLinkCchipInstance::get_instance()->get_operation_type() == operation_type::EAutoConfig) {
                    SoundController::getInstance()->linkStartFirst();
                } else if (DuerLinkCchipInstance::get_instance()->get_operation_type() == operation_type::EManualConfig) {
                    SoundController::getInstance()->linkStartFirst();
                }
                DeviceIoWrapper::getInstance()->cancelMusic();
                m_isLoopNetworkConfig = true;
            }
            break;
        }
        case duerLink::ENetworkConfigIng: {
            APP_INFO("notify_network_config_status: ENetworkConfigIng=====");
            setNetworkStatus(DUERLINK_NETWORK_CONFIGING);
            if (DuerLinkCchipInstance::get_instance()->get_operation_type() != operation_type::EAutoEnd) {
//                DeviceIoWrapper::getInstance()->ledNetDoConnect();
                SoundController::getInstance()->linkConnecting();
            }
            break;
        }
        case duerLink::ENetworkLinkFailed: {
            APP_INFO("notify_network_config_status: ENetworkLinkFailed=====");
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            //Network config failed, reset wpa_supplicant.conf
            DuerLinkCchipInstance::get_instance()->set_wpa_conf(false);

            setNetworkStatus(DUERLINK_NETWORK_CONFIG_FAILED);
            if (DuerLinkCchipInstance::get_instance()->get_operation_type() != operation_type::EAutoEnd) {
//                DeviceIoWrapper::getInstance()->ledNetConnectFailed();
                SoundController::getInstance()->linkFailedPing(DuerLinkWrapper::networkLinkFailed);
            }
            break;
        }
        case duerLink::ENetworkConfigRouteFailed: {
            APP_INFO("notify_network_config_status: ENetworkConfigRouteFailed=====");
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            //Network config failed, reset wpa_supplicant.conf
            DuerLinkCchipInstance::get_instance()->set_wpa_conf(false);

            setNetworkStatus(DUERLINK_NETWORK_CONFIG_FAILED);
            if (DuerLinkCchipInstance::get_instance()->get_operation_type() != operation_type::EAutoEnd) {
//                DeviceIoWrapper::getInstance()->ledNetConnectFailed();
                SoundController::getInstance()->linkFailedIp(SoundController::commonFinishCB);
            }
            break;
        }
        case duerLink::ENetworkConfigExited: {
            APP_INFO("notify_network_config_status: ENetworkConfigExited=====");
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            setNetworkStatus(DUERLINK_NETWORK_FAILED);
            m_isLoopNetworkConfig = false;
//            DeviceIoWrapper::getInstance()->ledNetOff();
            SoundController::getInstance()->linkExit(NULL);
            APP_INFO("notify_network_config_status: ENetworkConfigExited=====End");
            break;
        }
        case duerLink::ENetworkLinkSucceed: {

            APP_INFO("notify_network_config_status: ENetworkLinkSucceed===== [%d]",
                     DuerLinkCchipInstance::get_instance()->get_operation_type());
            setFromConfigNetwork(false);
            if (DeviceIoWrapper::getInstance()->isTouchStartNetworkConfig()) {
                /// from networkConfig
                setFromConfigNetwork(true);
            }
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            //Network config succed, update wpa_supplicant.conf
            DuerLinkCchipInstance::get_instance()->stop_network_config_timeout_alarm();
            DuerLinkCchipInstance::get_instance()->set_wpa_conf(true);

            setNetworkStatus(DUERLINK_NETWORK_CONFIG_SUCCEEDED);
            if (!isFromConfigNetwork()) {
//                DeviceIoWrapper::getInstance()->ledNetOff();
            }

            m_isLoopNetworkConfig = false;
            if (m_callback) {
                m_callback->networkReady();
            }
            break;
        }
        case duerLink::ENetworkRecoveryStart: {
            APP_INFO("notify_network_config_status: ENetworkRecoveryStart=====");
            DeviceIoWrapper::getInstance()->setRecoveryingNetwork(true);
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(true);
            setNetworkStatus(DUERLINK_NETWORK_RECOVERY_START);
//            DeviceIoWrapper::getInstance()->ledNetRecoveryConnect();
            SoundController::getInstance()->reLink();
            break;
        }
        case duerLink::ENetworkRecoverySucceed: {
            APP_INFO("notify_network_config_status: ENetworkRecoverySucceed=====");
            DeviceIoWrapper::getInstance()->setRecoveryingNetwork(false);
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            setNetworkStatus(DUERLINK_NETWORK_RECOVERY_SUCCEEDED);

            setFromConfigNetwork(false);
            if (m_callback) {
                m_callback->networkReady();
            }
            break;
        }
        case duerLink::ENetworkRecoveryFailed: {
            APP_INFO("notify_network_config_status: ENetworkRecoveryFailed=====");
            DeviceIoWrapper::getInstance()->setRecoveryingNetwork(false);
            DeviceIoWrapper::getInstance()->setTouchStartNetworkConfig(false);
            setNetworkStatus(DUERLINK_NETWORK_RECOVERY_FAILED);
//            DeviceIoWrapper::getInstance()->ledNetConnectFailed();
            SoundController::getInstance()->reLinkFailed();
            break;
        }
        case duerLink::ENetworkDeviceConnected: {
//            DeviceIoWrapper::getInstance()->ledNetDoConnect();
            SoundController::getInstance()->hotConnected();
            break;
        }
        default:
            break;
    }
}

void DuerLinkWrapper::network_status_changed(InternetConnectivity current_status, bool wakeupTrigger) {
    if (current_status == InternetConnectivity::AVAILABLE) {
        if (!isNetworkOnline()) {
            setNetworkOnline(true);
            if (m_callback) {
                m_callback->duerlinkNetworkOnlineStatus(isNetworkOnline());
            }
        }
        setNetworkStatus(DUERLINK_NETWORK_SUCCEEDED);
    } else {
        if (isNetworkOnline()) {
            setNetworkOnline(false);
            if (m_callback) {
                m_callback->duerlinkNetworkOnlineStatus(isNetworkOnline());
            }
        }
        if (!DeviceIoWrapper::getInstance()->isTouchStartNetworkConfig()) {
            setNetworkStatus(DUERLINK_NETWORK_FAILED);
            if (wakeupTrigger) {
                wakeupDuerLinkNetworkStatus();
            }
        }
    }
}

void DuerLinkWrapper::wakeupDuerLinkNetworkStatus() {
    switch (m_networkStatus) {
        case DUERLINK_NETWORK_CONFIG_STARTED: {
            APP_INFO("wakeup_duer_link_network_status: DUERLINK_NETWORK_CONFIG_STARTED=====");
//            DeviceIoWrapper::getInstance()->ledNetWaitConnect();
            SoundController::getInstance()->networkConnectFailed();
            break;
        }
        case DUERLINK_NETWORK_CONFIGING: {
            APP_INFO("wakeup_duer_link_network_status: DUERLINK_NETWORK_CONFIGING=====");
//            DeviceIoWrapper::getInstance()->ledNetDoConnect();
            SoundController::getInstance()->linkConnecting();
            break;
        }
        case DUERLINK_NETWORK_RECOVERY_START: {
            APP_INFO("wakeup_duer_link_network_status: DUERLINK_NETWORK_RECOVERY_START=====");
//            DeviceIoWrapper::getInstance()->ledNetRecoveryConnect();
            SoundController::getInstance()->reLink();
            break;
        }
        case DUERLINK_NETWORK_FAILED: {
            APP_INFO("wakeup_duer_link_network_status: DUERLINK_NETWORK_FAILED=====");
//            DeviceIoWrapper::getInstance()->ledNetConnectFailed();
            SoundController::getInstance()->networkConnectFailed();
            break;
        }
        case DUERLINK_NETWORK_RECOVERY_FAILED: {
            APP_INFO("wakeup_duer_link_network_status: DUERLINK_NETWORK_RECOVERY_FAILED=====");
//            DeviceIoWrapper::getInstance()->ledNetConnectFailed();
            SoundController::getInstance()->reLinkFailed();
            break;
        }
        default:
            break;
    }
}

void DuerLinkWrapper::networkLinkSuccess() {
//    DeviceIoWrapper::getInstance()->ledNetOff();
    if (framework::DeviceIo::getInstance()->inOtaMode()) {
        DeviceIoWrapper::getInstance()->setMute(false);
        framework::DeviceIo::getInstance()->rmOtaFile();
    }
}

void DuerLinkWrapper::networkLinkFailed() {
    if (framework::DeviceIo::getInstance()->inOtaMode()) {
        DeviceIoWrapper::getInstance()->setMute(false);
        framework::DeviceIo::getInstance()->rmOtaFile();
    }
}

void DuerLinkWrapper::startDiscoverAndBound(const string& client_id) {
    DuerLinkCchipInstance::get_instance()->init_discovery_parameter(DeviceIoWrapper::getInstance()->getDeviceId()
                                                                         , client_id, "br1");
    DuerLinkCchipInstance::get_instance()->set_dlp_data_observer(this);
    DuerLinkCchipInstance::get_instance()->start_discover_and_bound();
}

string DuerLinkWrapper::NotifyReceivedData(const string& jsonPackageData, int iSessionID) {
    if (m_callback) {
        m_callback->duerlinkNotifyReceivedData(jsonPackageData);
    }
    return "";
}

bool DuerLinkWrapper::sendDlpMsgToAllClients(const string &sendBuffer) {
    return DuerLinkCchipInstance::get_instance()->send_dlp_msg_to_all_clients(sendBuffer);
}

bool DuerLinkWrapper::sendDlpMessageBySessionId(
    const string& message, unsigned short sessionId) {
    return DuerLinkCchipInstance::get_instance()->send_dlp_msg_by_specific_session_id(message,
                                                                                    sessionId);
}

bool DuerLinkWrapper::isNetworkOnline() const {
    return m_isNetworkOnline;
}

void DuerLinkWrapper::setNetworkOnline(bool isNetworkOnline) {
    DuerLinkWrapper::m_isNetworkOnline = isNetworkOnline;
}

bool DuerLinkWrapper::startLoudspeakerCtrlDevicesService(duerLink::device_type deviceTypeValue,
                                                         const string &clientId,
                                                         const string& message) {
    return DuerLinkCchipInstance::get_instance()->start_loudspeaker_ctrl_devices_service(deviceTypeValue,
                                                                                       clientId,
                                                                                       message);
}

bool DuerLinkWrapper::sendMsgToDevicesBySpecType(const string &msgBuf,
                                                 duerLink::device_type deviceTypeValue) {
        return DuerLinkCchipInstance::get_instance()->send_msg_to_devices_by_spec_type(msgBuf,
                                                                                     deviceTypeValue);
}

bool DuerLinkWrapper::disconnectDevicesConnectionsBySpeType(duerLink::device_type deviceTypeValue,
                                                            const string& message) {
    return DuerLinkCchipInstance::get_instance()->disconnect_devices_connections_by_spe_type(deviceTypeValue,
                                                                                           message);
}
void DuerLinkWrapper::networkLinkOrRecoverySuccess() {
	
//    DeviceIoWrapper::getInstance()->ledNetConnectSuccess();
    if (isFromConfigNetwork()) {
        SoundController::getInstance()->linkSuccess(SoundController::commonFinishCB);
    } else {
        SoundController::getInstance()->reLinkSuccess(DuerLinkWrapper::networkLinkSuccess);
    }
}

bool DuerLinkWrapper::getCurrentConnectedDeviceInfo(duerLink::device_type deviceType,
                                                    string& clientId,
                                                    string& deviceId) {
    return DuerLinkCchipInstance::get_instance()->get_curret_connect_device_info(clientId,
                                                                               deviceId,
                                                                               deviceType);
}

bool DuerLinkWrapper::isFirstNetworkReady() const {
    return m_isFirstNetworkReady;
}

void DuerLinkWrapper::setFirstNetworkReady(bool isFirstNetworkReady) {
    DuerLinkWrapper::m_isFirstNetworkReady = isFirstNetworkReady;
}

bool DuerLinkWrapper::isFromConfigNetwork() const {
    return m_isFromConfigNetwork;
}

void DuerLinkWrapper::setFromConfigNetwork(bool isFromConfigNetwork) {
    DuerLinkWrapper::m_isFromConfigNetwork = isFromConfigNetwork;
}

}  // namespace application
}  // namespace duerOSDcsApp
