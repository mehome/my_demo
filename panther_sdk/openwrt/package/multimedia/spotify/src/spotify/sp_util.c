/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#include <alsa/asoundlib.h>
#include "sp_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "ring_buffer.h"

#ifndef PATH_TO_APP_KEY_FILE
#define PATH_TO_APP_KEY_FILE "spotify_appkey.key"
#endif

struct SpSampleFormat current_sample_format = {0};
int paused = 1;

static uint8_t app_key_buf[400];
static size_t app_key_size = 0;

static char spotify_username[SP_MAX_USERNAME_LENGTH + 1];
static char spotify_password[100];

//static int current_audio_channels;
//static int current_audio_sample_rate;

#define MAX_SPOTIFY_VOLUME 65535
#define FLOAT_MAX_SPOTIFY_VOL 65535.0
#define MIXER_DEVICE "default"
#define MIXER_CONTROL "PCM"

static snd_pcm_t *handle = NULL;

static pthread_mutex_t mutex_alsa_handle;
static int audio_available = 0;

struct alsa_mixer {
    const char *device;
    const char *control;

    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    long volume_min;
    long volume_max;
    long volume_set;
};

static struct alsa_mixer volume_mixer;
static pthread_t audio_thread;
static int audio_thread_finish = 1;
static int quit_audio_thread = 0;

#define cprintf(fmt, args...) do { \
    FILE *fp = fopen("/dev/console", "w"); \
    if (fp) { \
        fprintf(fp, fmt , ## args); \
        fclose(fp); \
        } \
    } while (0)

#define SPOTIFY_DEBUG

void log_alsa(const char *format, ...) {
    char buf[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

#ifdef SPOTIFY_DEBUG
    cprintf("%s\n", buf);
#else
    fprintf(stderr, "%s\n", buf);
#endif
}

static void LoadAppKeyFromFile() {
    /*
    * TODO: In production code, the Application Key should be compiled into
    *       the executable or stored in an encrypted location. Read it from there.
    */
    FILE *f;
    f = fopen(PATH_TO_APP_KEY_FILE, "rb");
    if (!f) {
        fprintf(stderr, "Could not read Spotify app key from file '%s'.\n",
        	PATH_TO_APP_KEY_FILE);
        abort();
    }
    app_key_size = fread(app_key_buf, 1, sizeof(app_key_buf), f);
    fclose(f);
}

const uint8_t *GetAppKey() {
    if (!app_key_size)
        LoadAppKeyFromFile();
    return app_key_buf;
}

const size_t GetAppKeySize() {
    if (!app_key_size)
        LoadAppKeyFromFile();
    return app_key_size;
}

static void play_audio(const int16_t *samples, size_t sample_count) {
    int c;
    size_t written;
    size_t frame_count;

    frame_count = sample_count / current_sample_format.channels;

    written = 0;
    while (written < frame_count) {
        pthread_mutex_lock(&mutex_alsa_handle);
        if (handle == NULL || audio_available == 0 || quit_audio_thread == 1) {
            pthread_mutex_unlock(&mutex_alsa_handle);
            return;
        }
        c = snd_pcm_writei(handle, samples, frame_count);
#if 0
            snd_pcm_sframes_t delay;
            snd_pcm_delay(handle, &delay);
            log_alsa("debug message: pcm wirte %d check pcm delay %d", c, delay);
#endif
        if (c < 0) {
            if (c == -EBADFD)
                snd_pcm_prepare(handle);
            else {
                c = snd_pcm_recover(handle, c, 0);
            if (c < 0) {
                log_alsa("Failed to recover %s", snd_strerror(-errno));
                snd_pcm_close(handle);
                handle = NULL;
                pthread_mutex_unlock(&mutex_alsa_handle);
                return;
            }

            }
        } else {
            written += c;
        }

        if (written < frame_count)
            snd_pcm_wait(handle, 100);

        pthread_mutex_unlock(&mutex_alsa_handle);
    }
}

#define BUFFER_CHUNK				2048
void *thread_to_play_audio(void *unused) {
    int16_t samples[BUFFER_CHUNK];
    size_t take_size = BUFFER_CHUNK;
    size_t enque_size = 0;

    quit_audio_thread = 0;
    audio_thread_finish = 0;
    while (quit_audio_thread == 0) {
#if 0
        if (paused) {
            usleep(50000);
            continue;
        }
#endif

        //pthread_mutex_lock(&esdk_mutex);

        enque_size = ring_buffer_size();

        if (enque_size >= BUFFER_CHUNK)
            take_size = BUFFER_CHUNK;
        else if (enque_size > 0)
            take_size = enque_size;
        else {
            //pthread_mutex_unlock(&esdk_mutex);
            usleep(1000);	// change from 500ms to 1ms, or music playing underflow
            continue;
        }

        if (ring_buffer_dequeue(samples, take_size) < 0) {
            //pthread_mutex_unlock(&esdk_mutex);
            log_alsa("ring_buffer_dequeue take_size %d", take_size);
            usleep(1000);	// change from 500ms to 1ms, or music playing underflow
            continue;
        }
        //pthread_mutex_unlock(&esdk_mutex);
        // send audio buffer to driver
        play_audio(samples, take_size);
    }
    log_alsa("audio play thread exit!!");
    audio_thread_finish = 1;
    return NULL;
}

void StartPlayAudioThread() {
    while (audio_thread_finish != 1) {
        log_alsa("stop previous job ... thread finish flag %d, quit thread flag %d\n",  audio_thread_finish, quit_audio_thread);
        quit_audio_thread = 1;
        usleep(500000);
    }
    log_alsa("%s, %d\n", __func__, __LINE__);

    pthread_mutex_init(&mutex_alsa_handle, 0);
    pthread_create(&audio_thread, NULL, thread_to_play_audio, NULL);
}

void StopPlayAutioThread() {
    quit_audio_thread = 1;
}

SpError SoundInitMixer() {
    // reference from mpd alsa_mixer_plugin.c
    volume_mixer.control = MIXER_CONTROL;
    volume_mixer.device = MIXER_DEVICE;
    volume_mixer.volume_set = -1;
    int err;

    err = snd_mixer_open(&volume_mixer.handle, 0);
    if (err < 0) {
        log_alsa("snd_mixer_open() failed: %s", snd_strerror(err));
        return kSpErrorFailed;
	}

    if ((err = snd_mixer_attach(volume_mixer.handle, volume_mixer.device)) < 0) {
        snd_mixer_close(volume_mixer.handle);
        log_alsa("failed to attach to %s: %s",
			    volume_mixer.device, snd_strerror(err));
        return kSpErrorFailed;
    }

    if ((err = snd_mixer_selem_register(volume_mixer.handle, NULL, NULL)) < 0) {
        snd_mixer_close(volume_mixer.handle);
        log_alsa("snd_mixer_selem_register() failed: %s",
                    snd_strerror(err));
        return kSpErrorFailed;
    }

    if ((err = snd_mixer_load(volume_mixer.handle)) < 0) {
        snd_mixer_close(volume_mixer.handle);
        log_alsa("snd_mixer_load() failed: %s\n",
                    snd_strerror(err));
        return kSpErrorFailed;
    }

    snd_mixer_elem_t *elem = snd_mixer_first_elem(volume_mixer.handle);
    while (elem) {
    if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE) {
        if (strcmp(volume_mixer.control,
                snd_mixer_selem_get_name(elem)) == 0) {
                break;
            }
        }
        elem = snd_mixer_elem_next(elem);
    }

    if (elem) {
        volume_mixer.elem = elem;
/*        snd_mixer_selem_get_playback_volume_range(volume_mixer.elem,
                                &volume_mixer.volume_min,
                                &volume_mixer.volume_max);
                                */
        char val[8] = {0};
        if (cdb_get("$hw_vol_max", val) >= 0)
            volume_mixer.volume_max = atoi(val);
        else
            volume_mixer.volume_max = 0xff;

        if (cdb_get("$hw_vol_min", val) >= 0)
            volume_mixer.volume_min = atoi(val);
        else
            volume_mixer.volume_min = 0;
        //volume_mixer.volume_max = 0xdb; //For wm8750
        //volume_mixer.volume_min = 0x99; //For wm8750
        //snd_mixer_selem_set_playback_volume_all(volume_mixer.elem,
        //    (volume_mixer.volume_max + volume_mixer.volume_min)/2);
        return kSpErrorOk;
    }
    snd_mixer_close(volume_mixer.handle);
    return kSpErrorFailed;
}

int IsSoundNeedToInit() {
    int ret = 0;
    pthread_mutex_lock(&mutex_alsa_handle);
    if (handle == NULL)
        ret = 1;
    pthread_mutex_unlock(&mutex_alsa_handle);

    return ret;
}

void SoundInitDriver(const struct SpSampleFormat *sample_format) {
    int err;
    int dir = 0;

    log_alsa("SoundInitDriver to %d Hz %s\n",
    sample_format->sample_rate, sample_format->channels == 1 ? "Mono" : "Stereo");
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    SoundCloseDriver();

    pthread_mutex_lock(&mutex_alsa_handle);
    log_alsa("snd_pcm_open\n");
    err = snd_pcm_open(&handle, MIXER_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        log_alsa("audio open error: %s", snd_strerror(err));
        handle = NULL;
        return;
    }

    log_alsa("snd_pcm_set_params\n");
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);
    err = snd_pcm_hw_params_any(handle, params);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_channels(handle, params, sample_format->channels);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_rate_near(handle, params, (unsigned int*)&(sample_format->sample_rate), 0);
    assert(err >= 0);

    unsigned buffer_time = 500000;
    log_alsa("buffer_time %d\n", buffer_time);

    unsigned period_time = buffer_time / 4;
    log_alsa("buffer_time %d\n", period_time);
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, 0);
    assert(err >= 0);
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, 0);
    assert(err >= 0);

    err = snd_pcm_hw_params(handle, params);
    assert(err >= 0);

    snd_pcm_uframes_t chunk_size = 0;
    snd_pcm_uframes_t buffer_size = 0;
    snd_pcm_uframes_t start_threshold = 0;
    snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    assert(chunk_size != buffer_size);
#if 0
    snd_pcm_sw_params_current(handle, swparams);
    log_alsa("set avail min %d\n", chunk_size);
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, chunk_size);
    assert(err >= 0);
    err = snd_pcm_sw_params(handle, swparams);
    assert(err >= 0);
#endif

#if 0
    if (snd_pcm_hw_params_can_pause(params))
        log_alsa("hw supports pause");
    else
        log_alsa("hw not supports pause");
#endif

    audio_available = 1;
    snd_pcm_prepare(handle);
    pthread_mutex_unlock(&mutex_alsa_handle);
    log_alsa("Init done\n");
//    current_audio_channels = sample_format->channels;
//    current_audio_sample_rate = sample_format->sample_rate;
}

void SoundCancel() {
    //ring_buffer_flush();
    pthread_mutex_lock(&mutex_alsa_handle);
    audio_available = 0;
    if(handle) {
        int err = 0;
        log_alsa("[alsa] drop handle");
        err = snd_pcm_drop(handle);
        if (err < 0)
            log_alsa("snd_pcm_drop err %d", err);
        log_alsa("[alsa] prepare handle");
        snd_pcm_prepare(handle);
        if (err < 0)
            log_alsa("snd_pcm_prepare err %d", err);
    }
    pthread_mutex_unlock(&mutex_alsa_handle);
}

void SoundPause() {
    pthread_mutex_lock(&mutex_alsa_handle);
    if(handle) {
        int err = 0;
        audio_available = 0;
        log_alsa("[alsa] reset handle");
        err = snd_pcm_reset(handle);
        if (err < 0)
            log_alsa("snd_pcm_reset err %d", err);
#if 0
        log_alsa("[alsa] pause handle");
        err = snd_pcm_pause(handle, 1);
        if (err < 0)
            log_alsa("snd_pcm_pause err %d", err);
#endif
    }
    pthread_mutex_unlock(&mutex_alsa_handle);
}

void SoundResume() {
    audio_available = 1;
}

void SoundCloseDriver() {
    pthread_mutex_lock(&mutex_alsa_handle);
    if(handle) {
        int err;
        audio_available = 0;
        log_alsa("[alsa] drop handle");
        snd_pcm_drop(handle);
        log_alsa("[alsa] close handle");
        err = snd_pcm_close(handle);
        if (err < 0)
            log_alsa("snd_pcm_close err");
        handle = NULL;
    }
    pthread_mutex_unlock(&mutex_alsa_handle);

    //ring_buffer_flush();
}

size_t SoundGetWriteableWithoutBlocking() {
    /*
    * TODO: This example can write an infinite number of samples without blocking.
    *       Implement this for your audio library.
    */
    return ring_buffer_remaining();
}

size_t SoundWriteSamples(const int16_t *samples, size_t sample_count) {
    snd_pcm_sframes_t delay;
    //uint32_t buffered;
    size_t accepted = 0;

    if (sample_count == 0 || paused)
        goto end;

    // queue audio samples in local ring buffer
    if (ring_buffer_queue(samples, sample_count) == 0)
        accepted = sample_count;

    end:
#if 0
    // Note: this calculation is incorrect if the audio format changed between mono
    // and stereo.  A real integration should track sample count accurately.
    if(handle && snd_pcm_delay(handle, &delay) == 0)
        buffered = delay * current_sample_format.channels;
    else
        buffered = 0;
#endif
    return accepted;
}

size_t SoundGetUnplayedSamples() {
    return ring_buffer_size();
}

int vol_update_flag = 0;
static struct timeval vol_update_time;

void new_vol_update_time() {
    vol_update_flag = 1;
    gettimeofday(&vol_update_time, NULL);
}

void commit_ra_vol() {
    if (vol_update_flag) {
        struct timeval now;

        gettimeofday(&now, NULL);

        if ((now.tv_sec - vol_update_time.tv_sec) > 5) {
            system("cdb commit");
            vol_update_flag = 0;
            log_alsa("spotify cdb commit\n");
        }
    }
}

void SoundSetOutputVolume(uint16_t volume) {
    float vol = volume;
    long new_vol = 0;

    if (volume_mixer.elem != NULL) {
        new_vol = (long)(((vol / FLOAT_MAX_SPOTIFY_VOL) * (volume_mixer.volume_max - volume_mixer.volume_min) +
			volume_mixer.volume_min) + 0.5);

        new_vol = (new_vol > volume_mixer.volume_max) ? volume_mixer.volume_max : new_vol;
        new_vol = (new_vol < volume_mixer.volume_min) ? volume_mixer.volume_min : new_vol;

        if(new_vol > volume_mixer.volume_min)
            snd_mixer_selem_set_playback_volume_all(volume_mixer.elem, new_vol);
        else
            snd_mixer_selem_set_playback_volume_all(volume_mixer.elem, 0);
    }

    char ra_vol[32] = {0};
    int ra_vol_int = (int)(vol / FLOAT_MAX_SPOTIFY_VOL * 100);

    cdb_get("$ra_vol", &ra_vol);

    //log_alsa("set output volue cdb %s\n", ra_vol);
    if (atoi(ra_vol) != ra_vol_int) {
        snprintf(ra_vol, 4, "%d", ra_vol_int);
        cdb_set("$ra_vol", ra_vol);
        new_vol_update_time();
    }
}

#if 0
uint16_t SoundGetOutputVolume() {
    uint16_t current_vol = 0;
    int ret = 0;
    if (volume_mixer.elem != NULL) {
        ret = snd_mixer_selem_get_playback_volume(volume_mixer.elem,
                    SND_MIXER_SCHN_FRONT_LEFT , &volume_mixer.volume_set);
        //log_alsa("vol get %d", volume_mixer.volume_set);
    }

    if(volume_mixer.volume_set > 0) {
        // reference from mpd alsa_mixer_plugin.c
        current_vol = (int)(MAX_SPOTIFY_VOLUME* (((float)(volume_mixer.volume_set - volume_mixer.volume_min)) /
				   (volume_mixer.volume_max - volume_mixer.volume_min)) + 0.5);
//        log_alsa("vol get %d", current_vol);
    }

    return current_vol;
}
#else
// alsa get current volume failed in run time, should get volume from cdb?

uint16_t SoundGetOutputVolume() {
    uint16_t current_vol = 0;
    char vol[8] = {0};

    cdb_get("$ra_vol", &vol);
    //current_vol = (int)(MAX_SPOTIFY_VOLUME * ((float)atoi(vol) / 100) + 0.5);
    current_vol = (int)(MAX_SPOTIFY_VOLUME * ((float)atoi(vol) / 100));

    return current_vol;
}
#endif

int SoundInitBuffers() {
    return ring_buffer_init();
}

int SoundFreeBuffers() {
    ring_buffer_destory();
}

void SoundFlushBuffers() {
    ring_buffer_flush();
    pthread_mutex_lock(&mutex_alsa_handle);
    if(handle) {
        int err = 0;
#if 0
        log_alsa("[alsa] reset handle");
        err = snd_pcm_reset(handle);
        if (err < 0)
            log_alsa("snd_pcm_reset err %d", err);
#else
        log_alsa("[alsa] drop handle");
        err = snd_pcm_drop(handle);
        if (err < 0)
            log_alsa("snd_pcm_drop err %d", err);
#endif
    }
    pthread_mutex_unlock(&mutex_alsa_handle);
}

void SoundTogglePause(int pause) {
    if(paused == pause)
        return;

    if(pause == 1) {
        SoundCancel();
        //SoundPause();
    } else {
        SoundResume();
    }

    paused = pause;
}

