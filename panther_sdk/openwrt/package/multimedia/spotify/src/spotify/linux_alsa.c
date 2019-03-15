/**
 * Copyright (c) 2017 Spotify AB
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#include <alsa/asoundlib.h>
#include "spotify_embedded.h"
#include "audio.h"
#include "dummy.h"
#include <libcchip/platform.h>

static snd_pcm_t *pcm_handle;
//static snd_mixer_selem_id_t *selem_handle;
//static snd_mixer_elem_t *volume_pcm;
//static snd_mixer_elem_t *volume_master;
static struct SpSampleFormat fmt = { 2, 44100 };

static unsigned long last_vol_time;
//static long alsa_mix_minv, alsa_mix_maxv, alsa_mix_range;

static const char *alsa_device_names[] = {
	"media",
	NULL
};

static SpOutputDebugMessage debug_callback;

void log_alsa(const char *format, ...) {
    char buf[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    debug_callback(buf, NULL);
}

#ifdef CERT_DEBUG
int need_log_data_income = 1;
#endif

#ifdef RING_BUFFER_ENABLE

#include "ring_buffer.h"

static pthread_mutex_t mutex_alsa_handle;
static pthread_t audio_thread;
static int audio_thread_finish = 1;
static int quit_audio_thread = 1;

#define BUFFER_CHUNK				2048
static void play_audio(const int16_t *samples, size_t sample_count) {
    int c;
    size_t written;
    size_t frame_count;

    frame_count = sample_count / fmt.channels;

    written = 0;
    while (written < frame_count) {
        pthread_mutex_lock(&mutex_alsa_handle);
        if (pcm_handle == NULL || quit_audio_thread == 1) {
            pthread_mutex_unlock(&mutex_alsa_handle);
            return;
        }
        c = snd_pcm_writei(pcm_handle, samples, frame_count);
#ifdef CERT_DEBUG
	if (need_log_data_income) {
		if (sample_count)
			log_alsa("snd_pcm_writei sample_count %d r %d", sample_count, c);
		if (sample_count && c > 0)
			need_log_data_income = 0;
	}
#endif
        if (c < 0) {
            if (c == -EBADFD)
                snd_pcm_prepare(pcm_handle);
            else if(snd_pcm_recover(pcm_handle, c, 0) < 0) {
                log_alsa("Failed to recover %s", snd_strerror(-errno));
                //snd_pcm_close(pcm_handle);
                //pcm_handle = NULL;
                pthread_mutex_unlock(&mutex_alsa_handle);
                return;
            }
        } else {
            written += c;
        }

        if (written < frame_count)
            snd_pcm_wait(pcm_handle, 10);	// from 100 ms to 10ms

        pthread_mutex_unlock(&mutex_alsa_handle);
    }
}

void *thread_to_play_audio(void *unused) {
    int16_t samples[BUFFER_CHUNK];
    size_t take_size = BUFFER_CHUNK;
    size_t enque_size = 0;

    quit_audio_thread = 0;
    audio_thread_finish = 0;
    while (quit_audio_thread == 0) {
        if (ring_buffer_size() >= BUFFER_CHUNK)
            take_size = BUFFER_CHUNK;
        else
            take_size = ring_buffer_size();

        if (take_size > 0 && ring_buffer_dequeue(samples, take_size) == 0) {
            play_audio(samples, take_size);
        } else {
            usleep(1000);
            continue;
        }
    }
    //log_alsa("audio play thread exit!!");
    audio_thread_finish = 1;
    return NULL;
}

void start_audio_thread(void) {
	if (quit_audio_thread) {
		//log_alsa("%s, %d\n", __func__, __LINE__);
		pthread_mutex_init(&mutex_alsa_handle, 0);
		pthread_create(&audio_thread, NULL, thread_to_play_audio, NULL);
	}
}

void stop_audio_thread(void) {
    quit_audio_thread = 1;
}

#endif

static int audio_set_param(void)
{
#ifdef RING_BUFFER_ENABLE
    pthread_mutex_lock(&mutex_alsa_handle);
#endif

    int err;
    snd_pcm_hw_params_t *params;

    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(pcm_handle, params);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_channels(pcm_handle, params, fmt.channels);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, (unsigned int*)&(fmt.sample_rate), 0);
    assert(err >= 0);

#if 0
    unsigned buffer_time = 500000;
    unsigned period_time = buffer_time / 4;
    err = snd_pcm_hw_params_set_period_time_near(pcm_handle, params, &period_time, 0);
    assert(err >= 0);
    err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, params, &buffer_time, 0);
    assert(err >= 0);
#else
    unsigned period_time;

    err = snd_pcm_hw_params_get_period_time(params, &period_time, 0);
    assert(err >= 0);
    log_alsa("period_time %d\n", period_time);

    unsigned buffer_time = period_time * 4;
    err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, params, &buffer_time, 0);
    assert(err >= 0);

    err = snd_pcm_hw_params_get_buffer_time(params, &buffer_time, 0);
    assert(err >= 0);
    log_alsa("buffer_time %d\n", buffer_time);

    snd_pcm_uframes_t buffer_size;
    err = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    assert(err >= 0);
    log_alsa("buffer_size %d\n", buffer_size);
#endif

    err = snd_pcm_hw_params(pcm_handle, params);
    assert(err >= 0);

#ifdef RING_BUFFER_ENABLE
    pthread_mutex_unlock(&mutex_alsa_handle);
#endif

    return 0;
}

static size_t audio_data(const int16_t *samples, size_t sample_count,
                         const struct SpSampleFormat *sample_format,
                         uint32_t *samples_buffered, void *context)
{
	int r;

	if (pcm_handle == NULL)
		return sample_count;

	if (sample_format->channels == 0 || sample_format->sample_rate == 0) {
		//log_alsa("PlaybackAudioData %d, samplerate %d, count %d\n", sample_format->channels, sample_format->sample_rate, sample_count);
		return 0;
	} else if (memcmp(&fmt, sample_format, sizeof(fmt)) != 0) {
		//snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, fmt.channels, fmt.sample_rate, 1, 500000);

#ifdef RING_BUFFER_ENABLE
		if(ring_buffer_size() == 0) {
			fmt = *sample_format;
			audio_set_param();
		} else {
			return 0;
		}
#else
		fmt = *sample_format;
		snd_pcm_drain(pcm_handle);
		audio_set_param();
#endif

	}

#ifdef RING_BUFFER_ENABLE
	int samples_written = 0;

	if (ring_buffer_remaining() < sample_count)
		sample_count = ring_buffer_remaining();

	if (ring_buffer_queue(samples, sample_count) == 0)
		samples_written = sample_count;

    /*
    * Determine the number of samples that are in the audio pipeline that
    * haven't been played yet. This should include any samples that are buffered
    * by the application, but it should also include the audio latency of the driver.
    * This is to ensure that the playback position within the track is measured
    * accurately.
    */
	*samples_buffered = ring_buffer_size();

    /*
    * Return the number of samples that have actually been written.
    * Any remaining samples in the provided buffer will be provided again
    * in the next invocation of the callback.
    */
    //log_alsa("samples_buffered = %d, samples_written = %d\n", *samples_buffered, samples_written);
	return samples_written;
#else
	r = snd_pcm_writei(pcm_handle, samples, sample_count / sample_format->channels);

#ifdef CERT_DEBUG
	if (need_log_data_income) {
		if (sample_count)
			log_alsa("snd_pcm_writei sample_count %d r %d", sample_count, r);
		if (sample_count && r > 0)
			need_log_data_income = 0;
	}
#endif

	if (r < 0) {
		if (r == -EBADFD) {
			log_alsa("snd_pcm_prepare\n");
			//snd_pcm_prepare(pcm_handle);
			if (snd_pcm_prepare(pcm_handle) < 0)
				log_alsa("failed to snd_pcm_prepare\n");
		}
		else
			snd_pcm_recover(pcm_handle, r, 1);
		return 0;
	}

	//log_alsa("snd_pcm_writei sample_count %d r %d", sample_count, r);

	return r * sample_format->channels;
#endif
}

static void audio_pause(int pause)
{
#ifdef RING_BUFFER_ENABLE
	if (pause)
		stop_audio_thread();
	else
		start_audio_thread();

	pthread_mutex_lock(&mutex_alsa_handle);
#endif

	if (pcm_handle != NULL)
		snd_pcm_pause(pcm_handle, pause);

#ifdef RING_BUFFER_ENABLE
	pthread_mutex_unlock(&mutex_alsa_handle);
#endif
}

//long VOL_RA_TO_HW_SCALE(int ra_vol)
//{
//	float fvol = ra_vol;
//
//	long new_vol = (alsa_mix_range) * (fvol / 100.0) + alsa_mix_minv + 0.5;
//
//	if (new_vol <= alsa_mix_minv)
//		new_vol = 0;
//
//	//log_alsa("--- %d trans ra to hw volme %d\n", ra_vol, new_vol);
//
//	return new_vol;
//}

int VOL_SP_TO_RA_SCALE(int spotify_vol)
{
	float fvol = spotify_vol;

	int new_vol = 100.0 * (fvol / 65535.0) + 0.5;

	//log_alsa("--- %d trans sp to ra volme %d\n", spotify_vol, new_vol);

	return new_vol;
}

int VOL_RA_TO_SP_SCALE(int ra_vol)
{
	int new_vol = 65535.0 * ((float)(ra_vol) / 100.0) + 0.5;

	//log_alsa("~~~ %ld trans ra to sp volme %d\n", ra_vol, new_vol);

	return new_vol;
}

long get_audio_volume_ra(void)
{
	long outvol;
#if 0
	int vol = 0;
	if (volume_pcm != NULL) {
		snd_mixer_selem_get_playback_volume(volume_pcm, 0, &outvol);
		log_alsa("pcm 0 out volme %ld\n", outvol);
		snd_mixer_selem_get_playback_volume(volume_pcm, 1, &outvol);
		log_alsa("pcm 1 out volme %ld\n", outvol);
	}
	if (volume_master != NULL) {
		snd_mixer_selem_get_playback_volume(volume_master, 0, &outvol);
		log_alsa("master 0 out volme %ld\n", outvol);
		snd_mixer_selem_get_playback_volume(volume_master, 1, &outvol);
		log_alsa("master 1 out volme %ld\n", outvol);
	}
#endif

	if((gettimevalmsec(NULL) - last_vol_time) < 5000)
		return -1;

	char vol[8] = {0};

	cdb_get("$ra_vol", &vol);
	outvol = atoi(vol);

	return outvol;
}

static void audio_volume(int vol)
{
//	clog(Hred,"vol=%u",vol);
	unsigned volume=VOL_SP_TO_RA_SCALE(vol);
//	clog(Hred,"volume=%u",volume);
	set_system_volume(volume);
#if 0
	long new_vol = 0;
	//long min = 153, max = 219, new_vol = 0;
	int ra_vol = VOL_SP_TO_RA_SCALE(vol);

#if 1
	if (volume_pcm != NULL) {
		//snd_mixer_selem_get_playback_volume_range(volume_pcm, &min, &max);
		//new_vol = vol * (max - min) / 65535 + min;
		snd_mixer_selem_set_playback_volume_all(volume_pcm, VOL_RA_TO_HW_SCALE(ra_vol));
	}
	if (volume_master != NULL) {
		//snd_mixer_selem_get_playback_volume_range(volume_master, &min, &max);
		//new_vol = vol * (max - min) / 65535 + min;
		snd_mixer_selem_set_playback_volume_all(volume_master, VOL_RA_TO_HW_SCALE(ra_vol));
	}

	char strvol[4];

	snprintf(strvol, 4, "%d", ra_vol);
	cdb_set("$ra_vol", strvol);
#else

    // if we commit cdb at the end of mspotfy, dut may encounter problem when doing cdb reset.
    // so let mpc do this difficult job, even we don't want to depend on mpc.

	char strvol[32];

	snprintf(strvol, 32, "mpc volume %d", ra_vol);
	system(strvol);
#endif
#endif
	last_vol_time = gettimevalmsec(NULL);
}

static SpError audio_flush(void)
{
#ifdef RING_BUFFER_ENABLE
	ring_buffer_flush();
	pthread_mutex_lock(&mutex_alsa_handle);
#endif

	if (pcm_handle != NULL) {
		snd_pcm_drop(pcm_handle);
		snd_pcm_prepare(pcm_handle);
		//snd_pcm_reset(pcm_handle);
	}

#ifdef RING_BUFFER_ENABLE
	pthread_mutex_unlock(&mutex_alsa_handle);
#endif

#ifdef CERT_DEBUG
	need_log_data_income = 1;
#endif

	return kSpErrorOk;
}

#if defined(__PANTHER__)
static SpError audio_open(void)
{
	const char **pcm_name;
	for (pcm_name = alsa_device_names; *pcm_name != NULL; pcm_name++) {
#ifdef RING_BUFFER_ENABLE
		if (snd_pcm_open(&pcm_handle, *pcm_name, SND_PCM_STREAM_PLAYBACK, 0) == 0) // block
#else
		if (snd_pcm_open(&pcm_handle, *pcm_name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) == 0)
#endif
			break;
	}
	if (*pcm_name == NULL) {
		log_alsa("Couldn't open ALSA pcm handle.");
		return kSpErrorFailed;
	}

	audio_set_param();
}

static SpError audio_close(void)
{
	if (pcm_handle != NULL) {
		snd_pcm_close(pcm_handle);
		pcm_handle = NULL;
	}
}
#endif
static SpError audio_init(void)
{
	const char **pcm_name;

//	snd_mixer_t *mixer_handle;
//
//	if (snd_mixer_open(&mixer_handle, 0) != 0) {
//		log_alsa("Couldn't open ALSA mixer.");
//		return kSpErrorFailed;
//	}

	for (pcm_name = alsa_device_names; *pcm_name != NULL; pcm_name++) {
#ifdef RING_BUFFER_ENABLE
		if (snd_pcm_open(&pcm_handle, *pcm_name, SND_PCM_STREAM_PLAYBACK, 0) == 0) // block
#else
		if (snd_pcm_open(&pcm_handle, *pcm_name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) == 0)
#endif
			break;
	}
	if (*pcm_name == NULL) {
		log_alsa("Couldn't open ALSA pcm handle.");
		return kSpErrorFailed;
	}

#if 0
	if (snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, fmt.channels, fmt.sample_rate, 1, 500000) != 0) {
		log_alsa("Couldn't set pcm params.");
		snd_pcm_close(pcm_handle);
		pcm_handle = NULL;
		return kSpErrorFailed;
	}
#else

	audio_set_param();
#endif

//	if (snd_mixer_attach(mixer_handle, *pcm_name) != 0) {
//		log_alsa("Couldn't attach to ALSA mixer.");
//		return kSpErrorFailed;
//	}
//	if (snd_mixer_selem_register(mixer_handle, NULL, NULL) != 0) {
//		log_alsa("Couldn't register ALSA mixer.");
//		return kSpErrorFailed;
//	}
//	if (snd_mixer_load(mixer_handle) != 0) {
//		log_alsa("Couldn't load ALSA mixer.");
//		return kSpErrorFailed;
//	}
//
//	snd_mixer_selem_id_alloca(&selem_handle);
//	snd_mixer_selem_id_set_index(selem_handle, 0);
//	snd_mixer_selem_id_set_name(selem_handle, "Master");
//	volume_master = snd_mixer_find_selem(mixer_handle, selem_handle);
//
//	snd_mixer_selem_id_alloca(&selem_handle);
//	snd_mixer_selem_id_set_index(selem_handle, 0);
//	snd_mixer_selem_id_set_name(selem_handle, "Media");
//	volume_pcm = snd_mixer_find_selem(mixer_handle, selem_handle);
//
//	if (volume_pcm == NULL && volume_master == NULL) {
//		log_alsa("Couldn't find Spotify or Master volume controls.");
//		// can still run, although changing volume won't be supported
//	}
//
//#if 0
//	char val[32];
//
//	if (cdb_get("$hw_vol_max", &val) >= 0)
//		alsa_mix_maxv = atoi(val);
//	else
//		alsa_mix_maxv = 0xff;
//
//	if (cdb_get("$hw_vol_min", &val) >= 0)
//		alsa_mix_minv = atoi(val);
//	else
//		alsa_mix_minv = 0;
//#else
//	if (NULL != volume_pcm) {
//		snd_mixer_selem_get_playback_volume_range(volume_pcm, &alsa_mix_minv, &alsa_mix_maxv);
//	} else {
//		alsa_mix_maxv = 0xff;
//		alsa_mix_minv = 0;
//	}
//#endif
//
//	alsa_mix_range = alsa_mix_maxv - alsa_mix_minv;

	last_vol_time = gettimevalmsec(NULL);

	return kSpErrorOk;
}

static void audio_changed(int duration_ms) {}
void SpAudioGetCallbacks(struct SpAudioCallbacks *callbacks)
{
	if (!callbacks)
		return;
	callbacks->audio_init   = audio_init;
#if defined(__PANTHER__)
	callbacks->audio_open   = audio_open;
	callbacks->audio_close  = audio_close;
#endif
	callbacks->audio_data   = audio_data;
	callbacks->audio_pause  = audio_pause;
	callbacks->audio_volume = audio_volume;
	callbacks->audio_flush  = audio_flush;
	callbacks->audio_changed = audio_changed;
}

void SpSetDebugMessageCallback(SpOutputDebugMessage audio_debug_message)
{
	debug_callback = audio_debug_message;
}
