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

#include "AlsaController.h"

extern "C" {
	#include <libcchip/platform.h>
}

namespace duerOSDcsApp {
namespace mediaPlayer {

#ifdef MTK8516
#define ALSA_MAX_BUFFER_TIME 500000
#else
#define ALSA_MAX_BUFFER_TIME 500000
#endif

AlsaController::AlsaController(const std::string &audio_dev) : m_pcmHandle(NULL),
                                                               m_alsaCanPause(false),
                                                               m_abortFlag(false),
                                                               m_chunkBytes(0) {
    assert(!audio_dev.empty());
    m_pcmDevice = audio_dev;
}

AlsaController::~AlsaController() {
}

bool AlsaController::openDevice() {
    int ret = snd_pcm_open(&m_pcmHandle, m_pcmDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        return false;
    }
    return true;
}

bool AlsaController::setParams(unsigned int rate, unsigned int channels) {
    snd_pcm_hw_params_t *params = NULL;
    uint32_t buffer_time = 0;
    uint32_t period_time = 0;
    int ret = 0;
    snd_pcm_hw_params_alloca(&params);

    ret = snd_pcm_hw_params_any(m_pcmHandle, params);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_set_access(m_pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_set_format(m_pcmHandle, params, SND_PCM_FORMAT_S16_LE);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_set_channels(m_pcmHandle, params, channels);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_set_rate_near(m_pcmHandle, params, &rate, 0);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    if (ret < 0) {
        return false;
    }
    buffer_time = buffer_time > ALSA_MAX_BUFFER_TIME ? ALSA_MAX_BUFFER_TIME : buffer_time;
    period_time = buffer_time / 4;
    ret = snd_pcm_hw_params_set_buffer_time_near(m_pcmHandle, params, &buffer_time, 0);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_set_period_time_near(m_pcmHandle, params, &period_time, 0);
    if (ret < 0) {
        return false;
    }

    ret = snd_pcm_hw_params(m_pcmHandle, params);
    if (ret < 0) {
        return false;
    }
    m_alsaCanPause = snd_pcm_hw_params_can_pause(params);

    snd_pcm_uframes_t chunk_size = 0;
    snd_pcm_uframes_t buffer_size = 0;
    ret = snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
    if (ret < 0) {
        return false;
    }
    ret = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (ret < 0) {
        return false;
    }
	int bits_per_sample = snd_pcm_format_physical_width(SND_PCM_FORMAT_S16_LE);
	m_chunkBytes = chunk_size * bits_per_sample / 8;
	m_bits_per_frame = bits_per_sample * channels;
	inf("chunBytes = %d, chunk_size = %d, bits_per_frame=%d\n", m_chunkBytes, chunk_size, m_bits_per_frame );
	return true;
}

bool AlsaController::init(unsigned int rate, unsigned int channels) {
    if (!openDevice()) {
        return false;
    }
    if (!setParams(rate, channels)) {
        return false;
    }
    return true;
}

bool AlsaController::isAccessable() const {
    return m_pcmHandle != NULL;
}

bool AlsaController::alsaPrepare() {
    if (m_pcmHandle != NULL) {
        snd_pcm_prepare(m_pcmHandle);
        m_abortFlag = false;
        return true;
    }
    return false;
}

bool AlsaController::alsaPause() {
    int err = 0;
    if (m_alsaCanPause) {
        if ((err = snd_pcm_pause(m_pcmHandle, 1)) < 0) {
            return false;
        }
    } else {
        if ((err = snd_pcm_drop(m_pcmHandle)) < 0) {
            return false;
        }
    }

    return true;
}

bool AlsaController::alsaResume() {
    int err = 0;
    if (snd_pcm_state(m_pcmHandle) == SND_PCM_STATE_SUSPENDED) {
        while ((err = snd_pcm_resume(m_pcmHandle)) == -EAGAIN) {
            sleep(1);
        }
    }

    if (m_alsaCanPause) {
        if ((err = snd_pcm_pause(m_pcmHandle, 0)) < 0) {
            return false;
        }
    } else {
        if ((err = snd_pcm_prepare(m_pcmHandle)) < 0) {
            return false;
        }
    }

    return true;
}

bool AlsaController::aslaAbort() {
    if (m_pcmHandle != NULL) {
        m_abortFlag = true;
        snd_pcm_abort(m_pcmHandle);
        return true;
    }
    return false;
}

void AlsaController::writeStream(unsigned int channels,
                                  const void *buffer,
                                  unsigned long buff_size) {
    if (m_abortFlag || m_pcmHandle == NULL) {
        return;
    }
	char *p = (char *)buffer;

	int r = 0;
	size_t c = buff_size * 8 / m_bits_per_frame;
	while ( c > 0 ) {
		r = snd_pcm_writei(m_pcmHandle, p,  c);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < c)) {
				snd_pcm_wait(m_pcmHandle, 100);
		} else if (r == -EPIPE) {
				snd_pcm_prepare(m_pcmHandle);
				r = snd_pcm_recover(m_pcmHandle, r, 1);
				err("EPIPE, Buffer Underrun.");
				if ( (r > 0) && (r <= c) ) {
					r = snd_pcm_writei(m_pcmHandle, p, r );
				}
		} else if (r == -ESTRPIPE) {
				while ((r = snd_pcm_resume(m_pcmHandle)) == -EAGAIN)
					usleep(100);	/* wait until the suspend flag is released */
				if (r < 0) {
					if ((r = snd_pcm_prepare(m_pcmHandle)) < 0) {
						err("alsa resume: pcm_prepare %s", snd_strerror(r));
						break;
					}
					continue;
				}
		} else if (r < 0) {
				err("failed to snd_pcm_writei: %s", snd_strerror(r));
				break;
		}
		if (r > 0) {
				c -= r;
				p += (r * m_bits_per_frame) / 8;
		}
		
		if (m_abortFlag) {
			break;
		}
    }
}

bool AlsaController::alsaClear() {
    if (NULL != m_pcmHandle) {
        snd_pcm_drop(m_pcmHandle);
        return true;
    }
    return false;
}

int AlsaController::alsaChunkByte() {
	return m_chunkBytes;
}

bool AlsaController::alsaClose() {
    if (NULL == m_pcmHandle) {
        return false;
    }
    snd_pcm_drop(m_pcmHandle);
    snd_pcm_close(m_pcmHandle);
    m_pcmHandle = NULL;
    return true;
}

}  // mediaPlayer
}  // duerOSDcsApp
