/**
 * Copyright (c) 2013-2014 Spotify AB
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#ifndef __APPKEY_HEADER__
#define __APPKEY_HEADER__

#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "spotify_embedded.h"

/// The application key is specific to each project, and allows Spotify
/// to produce statistics on how our service is used.
extern uint8_t g_appkey[];
/// The size of the application key.
extern size_t g_appkey_size;

SpError SpReadAppkey(const char *keyfile);

#endif
