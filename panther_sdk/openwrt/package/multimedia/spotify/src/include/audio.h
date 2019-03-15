/**
 * Copyright (c) 2013-2014 Spotify AB
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#ifndef _AUDIO_H
#define _AUDIO_H

#include "spotify_embedded.h"

typedef SpError (*SpAudioInitMixer)(void);
#if defined(__PANTHER__)
typedef SpError (*SpAudioOpen)(void);
typedef SpError (*SpAudioClose)(void);
#endif
typedef SpError (*SpAudioFlush)(void);
typedef size_t  (*SpOnFetchAudioData)(
    const int16_t *samples, size_t sample_count,
    const struct SpSampleFormat *sample_format, uint32_t *samples_buffered,
    void *context);
typedef void (*SpOnPause)(int pause);
typedef void (*SpSetVolume)(int vol);
typedef void (*SpOnTrackChanged)(int duration_ms);

typedef void (*SpOutputDebugMessage)(const char *debug_message,
					      void *context);

struct SpAudioCallbacks {
	SpAudioInitMixer audio_init;
#if defined(__PANTHER__)
	SpAudioOpen audio_open;
	SpAudioClose audio_close;
#endif
	SpAudioFlush audio_flush;
	SpOnFetchAudioData audio_data;
	SpOnPause audio_pause;
	SpSetVolume audio_volume;
	SpOnTrackChanged audio_changed;
};

void SpAudioGetCallbacks(struct SpAudioCallbacks *callbacks);
void SpSetDebugMessageCallback(
    SpOutputDebugMessage audio_debug_message);

// audio thread is used when ring buffer enable
void start_audio_thread(void);

// Get cdb ra_vol value
// return -1 if we request to get volume again in 5 second.
long get_audio_volume_ra(void);

// Re-scale from Spotify volume level to RA volume level
int VOL_SP_TO_RA_SCALE(int spotify_vol);

// Re-scale from RA volume level to Spotify volume level
int VOL_RA_TO_SP_SCALE(int ra_vol);

#endif // _AUDIO_H
