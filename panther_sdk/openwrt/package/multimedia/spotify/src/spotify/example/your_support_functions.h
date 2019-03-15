/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#ifndef YOUR_SUPPORT_FUNCTIONS_H
#define YOUR_SUPPORT_FUNCTIONS_H

#include <spotify_embedded.h>

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
 * Prompt the user for the username for the Spotify account.
 * For use with SpConnectionLoginPassword().
 */
const char *GetSpotifyUsername();

/**
 * Prompt the user for the password for the Spotify account.
 * For use with SpConnectionLoginPassword().
 */
const char *GetSpotifyPassword();

/**
 * Return the MAC address or some other unique device ID.
 * For use in SpConfig::unique_id.
 */
const char *GetMACAddress();

/**
 * Return the host name or some other name unique on the local network.
 * For use in SpConfig::host_name.
 */
const char *GetHostname();

/**
 * Initialize the sound driver for the given sample format.
 * Used within the callback SpCallbackPlaybackAudioData().
 */
void SoundInitDriver(const struct SpSampleFormat *sample_format);

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
 * Discard any samples buffered by the application.
 * Used by the event kSpPlaybackEventAudioFlush.
 */
void SoundFlushBuffers();

#endif  // YOUR_SUPPORT_FUNCTIONS_H
