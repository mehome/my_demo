/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#include "your_support_functions.h"
#include <spotify_embedded.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define printf(...) \
            do { fprintf(stderr, __VA_ARGS__); } while (0)


static uint8_t memory_block[SP_RECOMMENDED_MEMORY_BLOCK_SIZE];

static SpError error_occurred = kSpErrorOk;

static uint8_t has_logged_in = 0;
static uint8_t has_logged_out = 0;

static struct SpSampleFormat previous_sample_format = {0};

static struct SpMetadata previous_metadata = {0};
static uint8_t previous_playing = 0;
static uint8_t previous_shuffle = 0;
static uint8_t previous_repeat = 0;
static uint32_t previous_position_sec = 0;
static uint8_t previous_active = 0;

void ErrorCallback(SpError err, void *context) {
  printf("Spotify Embedded error %d occurred\n", err);
  error_occurred = err;
}

void ConnectionNotify(enum SpConnectionNotification event, void *context) {
  if (event == kSpConnectionNotifyLoggedIn) {
    printf("Logged in successfully\n");
    has_logged_in = 1;
  } else if (event == kSpConnectionNotifyLoggedOut) {
    printf("Logged out\n");
    has_logged_out = 1;
  }
}

size_t PlaybackAudioData(const int16_t *samples, size_t sample_count,
                         const struct SpSampleFormat *sample_format,
                         uint32_t *samples_buffered, void *context) {
  size_t samples_writeable_without_blocking, samples_written;

  /*
   * Check whether the sample format is the same as last time we received the
   * callback and reinitialize the driver with the new format if necessary.
   * Error handling is left out for simplicity.
   */
  if (memcmp(sample_format, &previous_sample_format, sizeof(struct SpSampleFormat)) != 0) {
    SoundInitDriver(sample_format);  // application-defined function
    memcpy(&previous_sample_format, sample_format, sizeof(struct SpSampleFormat));
  }

  /*
   * The callback must not block. Therefore, check how many samples the
   * audio driver will accept without blocking.
   */
  samples_writeable_without_blocking = SoundGetWriteableWithoutBlocking();
                                              // application-defined function
  if (samples_writeable_without_blocking < sample_count)
    sample_count = samples_writeable_without_blocking;

  /* Write the samples to the audio driver. */
  samples_written = SoundWriteSamples(samples, sample_count);
                                              // application-defined function

  /*
   * Determine the number of samples that are in the audio pipeline that
   * haven't been played yet. This should include any samples that are buffered
   * by the application, but it should also include the audio latency of the driver.
   * This is to ensure that the playback position within the track is measured
   * accurately.
   */
  *samples_buffered = SoundGetUnplayedSamples();

  /*
   * Return the number of samples that have actually been written.
   * Any remaining samples in the provided buffer will be provided again
   * in the next invocation of the callback.
   */
  return samples_written;
}

void PlaybackApplyVolume(uint16_t volume, void *context) {
  /*
   * Call an application-defined function that applies the volume
   * to the output of the audio driver.
   */
  SoundSetOutputVolume(volume);
}

void PlaybackNotify(enum SpPlaybackNotification event, void *context) {
  SpError err;
  struct SpMetadata metadata;

  printf("PlaybackNotify: ");

  if (event == kSpPlaybackNotifyTrackChanged) {
    err = SpGetMetadata(&metadata, kSpMetadataTrackCurrent);
    if (err == kSpErrorFailed) {
      printf("Nothing playing\n");
      return;
    }
    printf("Playing track: \"%s\" - %s (%d:%02d)\n", metadata.track, metadata.artist,
           (metadata.duration_ms / 1000) / 60, (metadata.duration_ms / 1000) % 60);
  } else if (event == kSpPlaybackNotifyNext) {
    printf("Skipped to next track\n");
  } else if (event == kSpPlaybackNotifyPrev) {
    printf("Skipped to previous track\n");
  } else if (event == kSpPlaybackNotifyPlay) {
    printf("Playing\n");
  } else if (event == kSpPlaybackNotifyPause) {
    printf("Paused\n");
  } else if (event == kSpPlaybackNotifyShuffleOn) {
    printf("Shuffle ON\n");
  } else if (event == kSpPlaybackNotifyShuffleOff) {
    printf("Shuffle OFF\n");
  } else if (event == kSpPlaybackNotifyRepeatOn) {
    printf("Repeat ON\n");
  } else if (event == kSpPlaybackNotifyRepeatOff) {
    printf("Repeat OFF\n");
  } else if (event == kSpPlaybackNotifyBecameActive) {
    printf("Became active speaker\n");
  } else if (event == kSpPlaybackNotifyBecameInactive) {
    printf("Not active speaker anymore\n");
  } else if (event == kSpPlaybackNotifyLostPermission) {
    printf("Spotify is playing on another device\n");
  } else if (event == kSpPlaybackEventAudioFlush) {
    /* The application should discard any audio samples it has buffered. */
    SoundFlushBuffers();
  } else {
    printf("Event %d\n", event);
  }
}

void PlaybackSeek(uint32_t position_ms, void *context) {
  printf("PlaybackSeek: Playback seeked to position %d:%02d\n",
         (position_ms / 1000) / 60, (position_ms / 1000) % 60);
}

void DebugMessage(const char *debug_message, void *context) {
    printf("Debug: %s\n", debug_message);
}


int main(int argc, char *argv[]) {
  SpError err;
  struct SpConfig config;
  struct SpConnectionCallbacks connection_cb;
  struct SpPlaybackCallbacks playback_cb;
  struct SpDebugCallbacks debug_cb;
  struct SpMetadata metadata;
  uint8_t playing, shuffle, repeat, active;
  uint32_t position_sec;
  const char *username, *password;

  memset(&config, 0, sizeof(config));

  config.api_version = SP_API_VERSION;
  config.memory_block = memory_block;
  config.memory_block_size = sizeof(memory_block);
  config.app_key = GetAppKey();
  config.app_key_size = GetAppKeySize();
  /*
   * The MAC address typically works well as a unique ID.
   * GetMACAddress() is provided by the application.
   */
  config.unique_id = GetMACAddress();
  config.display_name = "Quick Start";
  config.brand_name = "FooCorp";
  config.model_name = "FooBarX2000";
  config.error_callback = ErrorCallback;
  config.error_callback_context = NULL;
  /*
   * Flag to enable the embedded zero config discovery service.
   * Set to 1 to enable.
   */
  config.zeroconf_serve = 1;
  /*
   * The port the embedded discovery web server should run on.
   * Leave a 0 to use a sytem assigned port.
   */
  config.zeroconf_port = 5000;
  /*
   * The host name should be unique on the local network.
   * Used in the mDNS based discovery phase.
   */
  config.host_name = GetHostname();

  err = SpInit(&config);
  if (err != kSpErrorOk) {
    printf("Error %d\n", err);
    return 0;
  }

  memset(&connection_cb, 0, sizeof(connection_cb));
  connection_cb.on_notify = ConnectionNotify;
  SpRegisterConnectionCallbacks(&connection_cb, NULL);

  memset(&playback_cb, 0, sizeof(playback_cb));
  playback_cb.on_audio_data = PlaybackAudioData;
  playback_cb.on_apply_volume = PlaybackApplyVolume;
  playback_cb.on_notify = PlaybackNotify;
  playback_cb.on_seek = PlaybackSeek;
  SpRegisterPlaybackCallbacks(&playback_cb, NULL);

  memset(&debug_cb, 0, sizeof(debug_cb));
  debug_cb.on_message = DebugMessage;
  SpRegisterDebugCallbacks(&debug_cb, NULL);

//  username = GetSpotifyUsername();
//  password = GetSpotifyPassword();
//  err = SpConnectionLoginPassword(username, password);
//  if (err != kSpErrorOk) {
    /*
     * The function would only return an error if we hadn't invoked SpInit()
     * or if we had passed an empty username or password.
     * Actual login errors will be reported to ErrorCallback().
     */
//    printf("Error %d\n", err);
//    SpFree();
//    return 0;
//  }

  while (error_occurred == kSpErrorOk) {
    SpPumpEvents();
    /*
    if (has_logged_in) {
      printf("Login was successful.\n");
      has_logged_in = 0;
    }
    if (has_logged_out) {
      printf("Logged out. Exiting.\n");
      break;
    }
    */
    /*
     * Get the volume level of the audio driver using an application-defined
     * function. If it doesn't match the volume level that was last reported
     * to the library, report the new volume.
     */
     /*
    if (SoundGetOutputVolume() != SpPlaybackGetVolume()) {
      SpPlaybackUpdateVolume(SoundGetOutputVolume());
    }
    */
    /*
     * Retrieve the metadata of the current track and compare against the
     * previous metadata. If the metadata changed, update the display.
     */
     /*
    err = SpGetMetadata(&metadata, kSpMetadataTrackCurrent);
    if (err == kSpErrorFailed)
      memset(&metadata, 0, sizeof(metadata));
    if (memcmp(&metadata, &previous_metadata, sizeof(metadata))) {
      if (err == kSpErrorFailed)
        printf("Nothing playing\n");
      else
        printf("Playing track: \"%s\" - %s (%d:%02d)\n", metadata.track, metadata.artist,
               (metadata.duration_ms / 1000) / 60, (metadata.duration_ms / 1000) % 60);
      memcpy(&previous_metadata, &metadata, sizeof(previous_metadata));
    }

    playing = SpPlaybackIsPlaying();
    if (playing != previous_playing) {
      printf(playing ? "Playing\n" : "Paused\n");
      previous_playing = playing;
    }

    shuffle = SpPlaybackIsShuffled();
    if (shuffle != previous_shuffle) {
      printf(shuffle ? "Shuffle ON\n" : "Shuffle OFF\n");
      previous_shuffle = shuffle;
    }

    repeat = SpPlaybackIsRepeated();
    if (repeat != previous_repeat) {
      printf(repeat ? "Repeat ON\n" : "Repeat OFF\n");
      previous_repeat = repeat;
    }

    active = SpPlaybackIsActiveDevice();
    if (active != previous_active) {
      printf(active ? "Active speaker\n" : "Not active speaker\n");
      previous_active = active;
    }

    position_sec = SpPlaybackGetPosition() / 1000;
    if (position_sec != previous_position_sec) {
      printf("Playback position %d:%02d\n", position_sec / 60, position_sec % 60);
      previous_position_sec = position_sec;
    }*/
  }

  SpFree();

  return 0;
}
