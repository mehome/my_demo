/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#ifndef YOUR_SUPPORT_FUNCTIONS_H
#define YOUR_SUPPORT_FUNCTIONS_H

#include <spotify_embedded.h>
#include "log.h"
#include <pthread.h>

/**
 * store sample format of current track
 */
extern struct SpSampleFormat current_sample_format;

/**
 * set the state if paused
 * 1 : pause 
 * 0 : play
 */
extern int paused;

/**
 *  set the state if logined.
 *  1 : login, get alsa control
 *  0 : logout, release alsa control
 */
extern int isLogined;

extern int isAvaliablePlayer;
extern int isWlanConnected;

/**
 *  set 1 if alsa owner want to be changed by others(shairport or mpd)
 */
extern int isPauseForChangeALSAOwner;

/**
 *  mutex for spotify pumpEvent
 */
extern pthread_mutex_t esdk_mutex;

/**
 * Return a Spotify Application Key from the file "spotify_appkey.key".
 * For use in SpConfig::app_key.
 */
const uint8_t *GetAppKey();

/**
 * Return the size of the Spotify Application Key.
 * For use in SpConfig::app_key_size.
 */
const size_t GetAppKeySize();


/**
 * Start a thread to play audio ring buffer
 */
void StartPlayAudioThread();

/**
 * Set flag to leave playaudio thread
 */
void StopPlayAutioThread();

/**
 * Initialize the sound mixer for audio volume.
 * Used within the main function.
 */
SpError SoundInitMixer();

/**
 * Check if alsa handle = NULL, need to recreate
 */
int IsSoundNeedToInit();

/**
 * Initialize the sound driver for the given sample format.
 * Used within the callback SpCallbackPlaybackAudioData().
 */
void SoundInitDriver(const struct SpSampleFormat *sample_format);

/**
 * close sound drive
 */
void SoundCloseDriver();

/**
 * drop buffer in alsa handle
 */
void SoundCancel();

/**
 * prepare alsa handle
 */
void SoundResume();

/**
 * Return the number of samples that can be written to the audio driver
 * without blocking.
 * Used within the callback SpCallbackPlaybackAudioData().
 */
size_t SoundGetWriteableWithoutBlocking();

/**
 * Write samples to the audio driver. Returns the number of samples that
 * could be written.
 * Used within the callback SpCallbackPlaybackAudioData().
 */
size_t SoundWriteSamples(const int16_t *samples, size_t sample_count);

/**
 * Returns the number of samples that have been written but that have not
 * been played yet (i.e., the buffered samples + the audio latency of the
 * driver).
 * Used within the callback SpCallbackPlaybackAudioData().
 */
size_t SoundGetUnplayedSamples();

/**
 * Set the playback volume of the audio driver.
 * Used within the callback SpCallbackPlaybackApplyVolume().
 */
void SoundSetOutputVolume(uint16_t volume);

/**
 * Retrieve the playback volume of the audio driver.
 */
uint16_t SoundGetOutputVolume();

/**
 * Initialize Ring buffers
 */
int SoundInitBuffers();

int SoundFreeBuffers();

/**
 * Discard any samples buffered by the application.
 * Used by the event kSpPlaybackEventAudioFlush.
 */
void SoundFlushBuffers();

/**
 * Toggle playing state as pause or play
 */
void SoundTogglePause(int pause);

/**
 * save volume to DUT
 */
void commit_ra_vol();

#endif  // YOUR_SUPPORT_FUNCTIONS_H
