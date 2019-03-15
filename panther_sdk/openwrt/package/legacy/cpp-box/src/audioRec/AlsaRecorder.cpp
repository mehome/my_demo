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

#include "AlsaRecorder.h"
#include <audioResample/audioResample.h>

#define ASSERT_RET(condition, ret)                                          \
do                                                                          \
{                                                                           \
    if (!(condition))                                                       \
    {                                                                       \
        err(" [recorder error] %d: I am here %s:%d ", ret, __FUNCTION__, __LINE__);\
        return ret;                                                         \
    }                                                                       \
}while(0)

namespace duerOSDcsApp {
namespace application {
AudioRecorder::AudioRecorder():_m_rec(NULL){
    ;
}
AudioRecorder::~AudioRecorder(){
    ;
}

int AudioRecorder::setHwparams(struct recorder* rec) {
    snd_pcm_hw_params_t* params = NULL;
    int err = 0;
    unsigned int rate = 0;
    snd_pcm_format_t format;
    snd_pcm_uframes_t size;
    snd_pcm_t* handle = (snd_pcm_t*)rec->wavein_hdl;
	inf("chl:%u,sample_rate:%u",ALSA_REC_CHANNELS,ALSA_REC_SAMPLE_RATE);
    rec->buffer_time = ALSA_REC_BUF_TIME;
    rec->period_time = ALSA_REC_PERIOD_TIME;

    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(handle, params);

    if (err < 0) {
        err("Broken configuration for this PCM");
        return err;
    }

    err = snd_pcm_hw_params_set_access(handle, params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        err("Access type not available");
        return err;
    }

    err = snd_pcm_hw_params_set_format(handle, params,
                                       SND_PCM_FORMAT_S16_LE);

    if (err < 0) {
        err("Sample format non available");
        return err;
    }

    err = snd_pcm_hw_params_set_channels(handle, params, ALSA_REC_CHANNELS);

    if (err < 0) {
        err("Channels count non available");
        return err;
    }

    rate = ALSA_REC_SAMPLE_RATE;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);

    if (err < 0) {
        err("Set rate failed");
        return err;
    }

    if (rate != ALSA_REC_SAMPLE_RATE) {
        err("Rate mismatch");
        return -EINVAL;
    }

    err = snd_pcm_hw_params_set_period_time_near(handle, params,
            &rec->period_time, 0);

    if (err < 0) {
        err("set period time fail");
        return err;
    }

    err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
            &rec->buffer_time, 0);

    if (err < 0) {
        err("set buffer time failed");
        return err;
    }

    err = snd_pcm_hw_params_get_period_size(params, &size, 0);

    if (err < 0) {
        err("get period size fail");
        return err;
    }

    rec->period_frames = size;
    err = snd_pcm_hw_params_get_buffer_size(params, &size);

    if (size == rec->period_frames) {
        err("Can't use period equal to buffer size (%lu == %lu)",
            size, rec->period_frames);
        return -EINVAL;
    }

    rec->buffer_frames = size;
    rec->bits_per_frame = ALSA_REC_FRAME_BITS;

    /* set to driver */
    err = snd_pcm_hw_params(handle, params);

    if (err < 0) {
        err("Unable to install hw params:");
        return err;
    }

    return 0;
}

int AudioRecorder::setSwparams(struct recorder* rec) {
    int err = 0;
    snd_pcm_sw_params_t* swparams = NULL;
    snd_pcm_t* handle = (snd_pcm_t*)(rec->wavein_hdl);

    snd_pcm_sw_params_alloca(&swparams);
    err = snd_pcm_sw_params_current(handle, swparams);

    if (err < 0) {
        err("get current sw para fail");
        return err;
    }

    err = snd_pcm_sw_params_set_avail_min(handle, swparams,
                                          rec->period_frames);

    if (err < 0) {
        err("set avail min failed");
        return err;
    }

    if ((err = snd_pcm_sw_params(handle, swparams)) < 0) {
        err("unable to install sw params:");
        return err;
    }

    return 0;
}

int AudioRecorder::prepareRecBuffer(struct recorder* rec) {
    size_t sz = (rec->period_frames * rec->bits_per_frame / 8);

    rec->audiobuf = (char*)malloc(sz);
    if (!rec->audiobuf) {
        return -ENOMEM;
    }

    return 0;
}

void AudioRecorder::freeRecBuffer(struct recorder* rec) {
    if (rec->audiobuf) {
        free(rec->audiobuf);
        rec->audiobuf = NULL;
    }
}

int AudioRecorder::openRecorder(struct recorder* rec) {
    int err = 0;

    do {
        err = snd_pcm_open((snd_pcm_t**)&rec->wavein_hdl, rec->alsa_rec_device,
                           SND_PCM_STREAM_CAPTURE, 0);

        if (err < 0) {
            err("retrying: Can't open \"%s\" PCM device. %s", 
                            rec->alsa_rec_device, snd_strerror(err));
            break;
        } else {
            inf("success open \"%s\" PCM device.", rec->alsa_rec_device);
        }

        err = setHwparams(rec);
        if (err) {
            break;
        }

        err = setSwparams(rec);
        if (err) {
            break;
        }

        return 0;
    } while (0);

    if (rec->wavein_hdl) {
        snd_pcm_close((snd_pcm_t*) rec->wavein_hdl);
    }

    rec->wavein_hdl = NULL;

    return err;
}

#define FFMPEG_RESAMPLE_ENABLE	1
#define USER_MY_RESAMPLE 		!FFMPEG_RESAMPLE_ENABLE
#define UAER_FFMPEG_RESAMPLE	FFMPEG_RESAMPLE_ENABLE

#if	UAER_FFMPEG_RESAMPLE
#define OUT_SAMPLE_RATE 16000
#define OUT_CHANNELS	1
#define IN_SAMPLE_RATE	44100
#define IN_CHANNELS		2
#endif

void* record_thread_proc(void* para) {
    struct recorder* rec = (struct recorder*) para;
    snd_pcm_t* handle = (snd_pcm_t*)rec->wavein_hdl;
    int frames = 0, r = 0;
    int bytes = 0;
#if USER_MY_RESAMPLE
	inf("USER_MY_RESAMPLE--------------");
	resample *resample441to16 = new Resample();
	frames = rec->period_frames;
	bytes = frames * rec->bits_per_frame / 8;
	char *frame16k=(char *)malloc(bytes*160/441/2);
	if(frame16k == NULL){
		err("MALLOC ERROR");
		return NULL;
	}
#endif
#if UAER_FFMPEG_RESAMPLE
	audioResample * pResample = new(audioResample);
	int err=pResample->init(
		AV_SAMPLE_FMT_S16,OUT_SAMPLE_RATE	,OUT_CHANNELS	,
		AV_SAMPLE_FMT_S16,IN_SAMPLE_RATE	,IN_CHANNELS	,
		OUT_SAMPLE_RATE
	);
	if(err<0){
		return NULL;
	}
#endif
    while (rec->running) {
        frames = rec->period_frames;

        r = snd_pcm_readi(handle, rec->audiobuf, frames);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < frames) ) {
			snd_pcm_wait(handle, 100);
		} else if (r == -EINTR || r == -EPIPE || r == -ESTRPIPE) {
            snd_pcm_recover(handle, r, 1);
            continue;
        } else if (r < 0) {
            err("ERROR: %s\n", snd_strerror(r));
            break;
        }
        bytes = r * rec->bits_per_frame / 8;
#if USER_MY_RESAMPLE
		unsigned long int len = 0;
		memset(frame16k, 0, bytes*160/441/2);
		resample441to16->resample_buf_441_to_16(rec->audiobuf, bytes, frame16k, &len);
		inf("bytes=%d,outRealSize=%d",bytes,len)
#endif
#if UAER_FFMPEG_RESAMPLE
		int len = 0;
		char *frame16k=NULL;
		int err=pResample->convert((uint8_t **)&frame16k,&len,(const uint8_t **)&(rec->audiobuf),bytes);
		if(err<0){
			continue;
		}
#endif		
        rec->on_data_ind(frame16k, len, rec->userdata);
    }
#if USER_MY_RESAMPLE
	if(frame16k){
		free(frame16k);
	}
#endif
#if UAER_FFMPEG_RESAMPLE		
	pResample->deInit();
	delete(pResample);
#endif
    return rec;
}

int AudioRecorder::recorderOpen(const char* alsa_rec_device,
                            void(*callback)(char* data, unsigned long len, void *userdata),
                            void *userdata) {
    int ret = 0;
    ASSERT_RET(NULL == _m_rec, -RECORD_ERR_OPEN);
    _m_rec = (struct recorder*)calloc(sizeof(struct recorder), 1);
    ASSERT_RET(_m_rec, -RECORD_ERR_MEMFAIL);
    _m_rec->on_data_ind = callback;
    _m_rec->alsa_rec_device = alsa_rec_device;
    _m_rec->userdata = userdata;
    do {
        ret = openRecorder(_m_rec);
        if (ret) {
            break;
        }

        ret = prepareRecBuffer(_m_rec);
        if (ret) {
            break;
        }

        _m_rec->running = 1;

        ret = pthread_create(&_m_rec->rec_thread, NULL, record_thread_proc, (void*)_m_rec);
        if (ret) {
            break;
        }

        return 0;
    } while (0);

    if (_m_rec->rec_thread) {
        pthread_cancel(_m_rec->rec_thread);
        pthread_join(_m_rec->rec_thread, NULL);
    }

    freeRecBuffer(_m_rec);

    if (_m_rec) {
        free(_m_rec);
        _m_rec = NULL;
    }

    return ret;
}

int AudioRecorder::recorderClose(void) {
    int ret = 0;

    ASSERT_RET(_m_rec, -RECORD_ERR_HANDLE);

    _m_rec->running = 0;
    pthread_join(_m_rec->rec_thread, NULL);

    snd_pcm_drop((snd_pcm_t*)_m_rec->wavein_hdl);
    snd_pcm_close((snd_pcm_t*)_m_rec->wavein_hdl);

    freeRecBuffer(_m_rec);

    if (_m_rec) {
        free(_m_rec);
        _m_rec = NULL;
    }

    return ret;
}
}
}
