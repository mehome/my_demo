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

#include "DeviceIo/DeviceIo.h"
#include <cmath>
#include <LoggerUtils/DcsSdkLogger.h>
#include "alsa/asoundlib.h"
extern "C" {
	#include <libcchip/platform.h>
//	#include <libcchip/function/back_trace/back_trace.h>	
}

namespace duerOSDcsApp {
namespace framework {

#define USER_VOL_MIN                    0   // keep some voice
#define USER_VOL_MAX                    100

#define AUDIO_MIN_VOLUME                0
#define AUDIO_MAX_VOLUME                1000

typedef struct {
    int     volume;
    bool    is_mute;
} user_volume_t;

static snd_mixer_t*      mixer_fd   = nullptr;
static snd_mixer_elem_t* mixer_elem = nullptr;
static user_volume_t    user_volume = {0, false};
static pthread_mutex_t  user_volume_mutex;

static void mixer_exit() {
    if (mixer_fd != nullptr) {
        snd_mixer_close(mixer_fd);
        mixer_fd = nullptr;
    }

    mixer_elem = nullptr;
}

static int mixer_init(const char* card, const char* elem) {
    const char* _card = card ? card : "default";
#ifdef MTK8516
    const char* _elem = elem ? elem : "TAS5760";
#else
    const char* _elem = elem ? elem : "COMMON";
#endif

    // open mixer
    if (snd_mixer_open(&mixer_fd, 0) < 0) {
        APP_ERROR("%s: snd_mixer_open error!\n", __func__);
        goto failed;
    }

    // Attach an HCTL to an opened mixer
    if (snd_mixer_attach(mixer_fd, _card) < 0) {
        APP_ERROR("%s: snd_mixer_attach error!\n", __func__);
        goto failed;
    }

    // register mixer
    if (snd_mixer_selem_register(mixer_fd, nullptr, nullptr) < 0) {
        APP_ERROR("%s: snd_mixer_selem_register error!\n", __func__);
        goto failed;
    }

    // load mixer
    if (snd_mixer_load(mixer_fd) < 0) {
        APP_ERROR("%s: snd_mixer_load error!\n", __func__);
        goto failed;
    }

    // each for
    for (mixer_elem = snd_mixer_first_elem(mixer_fd); mixer_elem;
            mixer_elem = snd_mixer_elem_next(mixer_elem)) {
        if (snd_mixer_elem_get_type(mixer_elem) == SND_MIXER_ELEM_SIMPLE
                && snd_mixer_selem_is_active(mixer_elem)) {
            if (strcmp(snd_mixer_selem_get_name(mixer_elem), _elem) == 0) {
                return 0;
            }
        }
    }

    APP_ERROR("%s: Cannot find master mixer elem!\n", __func__);
failed:
    mixer_exit();
    return -1;
}

static int mixer_set_volume(unsigned int user_vol) {
    long mix_vol;

    mix_vol = (user_vol > AUDIO_MAX_VOLUME) ? AUDIO_MAX_VOLUME : user_vol;
    mix_vol = (mix_vol < AUDIO_MIN_VOLUME) ? AUDIO_MIN_VOLUME : user_vol;

    if (mixer_elem == nullptr) {
        APP_INFO("%s: mixer_elem is NULL! mixer_init() will be called.\n", __func__);
        mixer_init(nullptr, nullptr);
    } else if (mixer_elem != nullptr) {
        snd_mixer_selem_set_playback_volume_range(mixer_elem, AUDIO_MIN_VOLUME, AUDIO_MAX_VOLUME);
        snd_mixer_selem_set_playback_volume_all(mixer_elem, mix_vol);
    }

    return 0;
}

static unsigned int mixer_get_volume() {
    long int alsa_left = AUDIO_MIN_VOLUME, alsa_right = AUDIO_MIN_VOLUME;
    int mix_vol = 0;

    if (mixer_elem == nullptr) {
        APP_INFO("%s: mixer_elem is NULL! mixer_init() will be called.\n", __func__);
        mixer_init(nullptr, nullptr);
    } else if (mixer_elem != nullptr) {
        snd_mixer_selem_set_playback_volume_range(mixer_elem, AUDIO_MIN_VOLUME, AUDIO_MAX_VOLUME);
        snd_mixer_selem_get_playback_volume(mixer_elem, SND_MIXER_SCHN_FRONT_LEFT,  &alsa_left);
        snd_mixer_selem_get_playback_volume(mixer_elem, SND_MIXER_SCHN_FRONT_RIGHT, &alsa_right);
		
        mix_vol = (alsa_left + alsa_right) >> 1;
    }

    return mix_vol;
}

static void user_set_volume(double user_vol) {
    double k, audio_vol;

    user_vol = (user_vol > USER_VOL_MAX) ? USER_VOL_MAX : user_vol;
    user_vol = (user_vol < USER_VOL_MIN) ? USER_VOL_MIN : user_vol;

    /* set volume will unmute */
    if (user_volume.is_mute) {
        user_vol = user_volume.volume;
        user_volume.is_mute = false;
    } else {
        user_volume.volume = user_vol;
    }
    k = (double)AUDIO_MAX_VOLUME / USER_VOL_MAX;

    audio_vol = k * user_vol;

    mixer_set_volume(audio_vol);
}

static int user_get_volume() {
    double k, offset, audio_vol;
    int user_vol = 0;

    audio_vol = mixer_get_volume();

    k = (double)(USER_VOL_MAX - USER_VOL_MIN) / (AUDIO_MAX_VOLUME - AUDIO_MIN_VOLUME);
    offset = USER_VOL_MAX - k * AUDIO_MAX_VOLUME;

    user_vol = ceil(k * audio_vol + offset);

    user_vol = (user_vol > USER_VOL_MAX) ? USER_VOL_MAX : user_vol;
    user_vol = (user_vol < USER_VOL_MIN) ? USER_VOL_MIN : user_vol;

    APP_DEBUG("[%s] audio_vol:%f  user_vol:%d\n", __FUNCTION__, audio_vol, user_vol);

    return user_vol;
}

DeviceIo* DeviceIo::m_instance = nullptr;
DeviceInNotify* DeviceIo::m_notify = nullptr;
pthread_once_t DeviceIo::m_initOnce = PTHREAD_ONCE_INIT;
pthread_once_t DeviceIo::m_destroyOnce = PTHREAD_ONCE_INIT;

DeviceIo::DeviceIo() {
    int ret = 0;

    m_notify = nullptr;

    ret = mixer_init(nullptr, nullptr);

    if (ret) {
        APP_ERROR("[%s] error: mixer_init fail, err is:%d\n",  __FUNCTION__, ret);

        return;
    }

    ret = pthread_mutex_init(&user_volume_mutex, nullptr);

    if (ret) {
        APP_ERROR("[%s] error: pthread_mutex_init fail, err is:%d\n",  __FUNCTION__, ret);

        return;
    }

    user_volume.volume = user_get_volume();

    m_destroyOnce = PTHREAD_ONCE_INIT;
}

DeviceIo::~DeviceIo() {
    pthread_mutex_destroy(&user_volume_mutex);
    mixer_exit();
    m_notify = nullptr;
    m_initOnce = PTHREAD_ONCE_INIT;
}

DeviceIo* DeviceIo::getInstance() {
    pthread_once(&m_initOnce, DeviceIo::init);

    return m_instance;
}

void DeviceIo::releaseInstance() {
    pthread_once(&m_destroyOnce, DeviceIo::destroy);
}

void DeviceIo::init() {
    if (m_instance == nullptr) {
        m_instance = new DeviceIo;
		timer_cdb_init(__SIGRTMIN+6);
    }
}

void DeviceIo::destroy() {
    delete m_instance;
    m_instance = nullptr;
}

void DeviceIo::setNotify(DeviceInNotify* notify) {
    if (notify) {
        m_notify = notify;
    }
}

DeviceInNotify* DeviceIo::getNotify() {
    return m_notify;
}

#define DU_LED 				"led"			//dueros：灯效显示命令头
int DeviceIo::controlLed(LedState cmd, void *data, int len) 
{
	char cmd_buf[16] = {0};
	int cmd_len = 0;
	switch(cmd){
		case LedState::LED_NET_RECOVERY:
		case LedState::LED_NET_WAIT_CONNECT:
		case LedState::LED_NET_DO_CONNECT:
		case LedState::LED_NET_CONNECT_FAILED:
		case LedState::LED_NET_CONNECT_SUCCESS:
		case LedState::LED_NET_WAIT_LOGIN:
		case LedState::LED_NET_DO_LOGIN:
		case LedState::LED_NET_LOGIN_FAILED:
		case LedState::LED_NET_LOGIN_SUCCESS:
		case LedState::LED_WAKE_UP:		 // no param
		case LedState::LED_SPEECH_PARSE:
		case LedState::LED_PLAY_TTS:
		case LedState::LED_PLAY_RESOURCE:
		case LedState::LED_BT_WAIT_PAIR:
		case LedState::LED_BT_DO_PAIR:
		case LedState::LED_BT_PAIR_FAILED:
		case LedState::LED_BT_PAIR_SUCCESS:
		case LedState::LED_BT_PLAY:
		case LedState::LED_BT_CLOSE:
		case LedState::LED_MUTE:
		case LedState::LED_ALARM:
		case LedState::LED_SLEEP_MODE:
		case LedState::LED_OTA_DOING:
		case LedState::LED_OTA_SUCCESS:
		case LedState::LED_CLOSE_A_LAYER:
		case LedState::LED_ALL_OFF:	 // no param
			sprintf(cmd_buf, "%s%d", DU_LED, cmd);
			cmd_len = strlen(DU_LED) + sizeof(int);
			break;
		case LedState::LED_WAKE_UP_DOA:  // param -180 ~ 180
			break;
		case LedState::LED_VOLUME:
			break;
		case LedState::LED_DISABLE_MIC:
			break;
		default:
//			inf(cmd_buf);
			break;
	}
	if(cmd_len > 0){
		uartd_fifo_write_data(cmd_buf, cmd_len);
	}
    return 0;
}

int DeviceIo::controlBt(BtControl cmd, void *data, int len) {
	trc(__func__);
	char cmd_buf[16] = {0};
	sprintf(cmd_buf, "bluetooth%d",cmd);
	APP_INFO(cmd_buf);

	switch(cmd){
		case BtControl::BT_OPEN:{
			break;
		}
		case BtControl::BT_CLOSE:{
			break;
		}
    	case BtControl::BT_IS_OPENED:{
			break;
    	}
    	case BtControl::BT_IS_CONNECTED:{
			break;
    	}
    	case BtControl::BT_UNPAIR:{
			break;
    	}
    	case BtControl::BT_PAUSE_PLAY:{
			break;
    	}
    	case BtControl::BT_RESUME_PLAY:{
			break;
		}
		case BtControl::BT_AVRCP_FWD:{
			break;
		}
		case BtControl::BT_AVRCP_BWD:{
			break;
		}
	    case BtControl::BT_AVRCP_STOP:{
			break;
	    }
     	case BtControl::BT_HFP_RECORD:{
			break;
     	}
    	case BtControl::BLE_OPEN_SERVER:{
			break;
    	}
    	case BtControl::BLE_CLOSE_SERVER:{
			break;
    	}
    	case BtControl::BLE_SERVER_SEND:{
			inf("send BLE_SERVER_SEND, len: %d", len+9);
			char buf[1024] = {0};
			memcpy(buf, "blenet_du", strlen("blenet_du"));
			memcpy(&buf[9], data, len);
			uartd_fifo_write_data(buf, len + 9);
			usleep(100*1000);
			break;
    	}
    	case BtControl::BLE_IS_OPENED:{
			break;
    	}
    	case BtControl::GET_BT_MAC:{
			break;
    	}
    	case BtControl::GET_WIFI_MAC:{
			cdb_get_str("$wanif_mac",(char *)data,len,"");
			break;
   		}
    	case BtControl::GET_WIFI_IP:{
			cdb_get_str("$wanif_ip",(char *)data,len,"");			
			break;
    	}
    	case BtControl::GET_WIFI_SSID:{
			cdb_get_str("$ap_ssid1",(char *)data,len,"");
			break;
    	}
    	case BtControl::GET_WIFI_BSSID:{
			cdb_get_str("$wl_bss_ssid2",(char *)data,len,"");			
			break;
    	}
	}
    return 0;
}

int DeviceIo::transmitInfrared(std::string& infraredCode) {
    return 0;
}

int DeviceIo::openMicrophone() {
    return 0;
}

int DeviceIo::closeMicrophone() {
    return 0;
}

bool DeviceIo::isMicrophoneOpened() {
    return true;
}

void DeviceIo::setVolume(int vol, int track_id) {
    pthread_mutex_lock(&user_volume_mutex);
    user_set_volume(vol);
    pthread_mutex_unlock(&user_volume_mutex);
}

int DeviceIo::getVolume(int track_id) {
    int user_vol;

    pthread_mutex_lock(&user_volume_mutex);

    if (user_volume.is_mute) {
        user_vol = 0;
    } else {
        user_vol = user_volume.volume;
    }

    pthread_mutex_unlock(&user_volume_mutex);

    return user_vol;
}

int DeviceIo::setMute(bool mute) {
    int ret = -1;

    pthread_mutex_lock(&user_volume_mutex);

    if (mute && !user_volume.is_mute) {
        user_volume.is_mute = true;
        mixer_set_volume(0);
        ret = 0;
    } else if (!mute && user_volume.is_mute) {
        /* set volume will unmute */
        user_set_volume(0);
        ret = 0;
    }

    pthread_mutex_unlock(&user_volume_mutex);

    return ret;
}

bool DeviceIo::isMute() {
    bool ret;

    pthread_mutex_lock(&user_volume_mutex);
    ret = user_volume.is_mute;
    pthread_mutex_unlock(&user_volume_mutex);

    return ret;
}

int DeviceIo::getAngle() {
    return 0;
}

bool DeviceIo::getSn(char *sn)
{
    return false;
}

bool DeviceIo::setSn(char * sn) {
    return false;
}

std::string DeviceIo::getVersion() {
    return "";
}

bool DeviceIo::inOtaMode() {
    return false;
}

void DeviceIo::rmOtaFile() {

}

} // namespace framework
} // namespace duerOSDcsApp
