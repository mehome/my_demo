/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#include "your_support_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_TO_APP_KEY_FILE
#define PATH_TO_APP_KEY_FILE "spotify_appkey.key"
#endif

static uint8_t app_key_buf[400];
static size_t app_key_size = 0;

static char spotify_username[SP_MAX_USERNAME_LENGTH + 1];
static char spotify_password[100];

static int current_audio_channels;
static int current_audio_sample_rate;
static size_t total_samples_written;
static int previous_sample_seconds;

static uint16_t sound_volume = 32768;   /* Start out at half of maximum volume */

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

const char *GetSpotifyUsername() {
  fprintf(stderr, "Type Spotify username: ");
  fgets(spotify_username, sizeof(spotify_username), stdin);
  spotify_username[strlen(spotify_username) - 1] = 0;   /* strip trailing newline */
  return spotify_username;
}

const char *GetSpotifyPassword() {
  fprintf(stderr, "Type Spotify password (will be printed to the screen): ");
  fgets(spotify_password, sizeof(spotify_password), stdin);
  spotify_password[strlen(spotify_password) - 1] = 0;   /* strip trailing newline */
  return spotify_password;
}

const char *GetMACAddress() {
  return "TODO: Get MAC address or other unique ID";
}

const char *GetHostname() {
  return "todo-hostname";
}

void SoundInitDriver(const struct SpSampleFormat *sample_format) {
  fprintf(stderr, "TODO: Initialize sound driver to %d Hz %s\n",
          sample_format->sample_rate, sample_format->channels == 1 ? "Mono" : "Stereo");
  current_audio_channels = sample_format->channels;
  current_audio_sample_rate = sample_format->sample_rate;
  total_samples_written = 0;
  previous_sample_seconds = 0;
}

size_t SoundGetWriteableWithoutBlocking() {
  /*
   * TODO: This example can write an infinite number of samples without blocking.
   *       Implement this for your audio library.
   */
  return ~0;
}

size_t SoundWriteSamples(const int16_t *samples, size_t sample_count) {
  int sample_seconds;
  total_samples_written += sample_count;
  sample_seconds = (int)total_samples_written / (current_audio_channels * current_audio_sample_rate);
  if (sample_seconds != previous_sample_seconds) {
    fprintf(stderr, "TODO: %d seconds of audio samples written\n", sample_seconds);
    previous_sample_seconds = sample_seconds;
  }

  return sample_count;  /* This example reports all samples as written. */
}

size_t SoundGetUnplayedSamples() {
  /*
   * TODO: This example does not buffer samples.
   *       Implement this for your audio library.
   */
  return 0;
}

void SoundSetOutputVolume(uint16_t volume) {
  fprintf(stderr, "TODO: Apply volume %d to audio driver\n", volume);
  sound_volume = volume;
}

uint16_t SoundGetOutputVolume() {
  /*
   * TODO: Retrieve output volume from audio driver, in case it is possible
   * that someone else has changed the volume without updating sound_volume.
   */
  return sound_volume;
}

void SoundFlushBuffers() {
  fprintf(stderr, "TODO: Flush audio buffers\n");
}
