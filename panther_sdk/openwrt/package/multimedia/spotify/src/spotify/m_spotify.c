/**
 * Copyright 2013-2014 Spotify AB. All rights reserved.
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

// Spotify eSDK
#include "spotify_embedded.h"

// Montage Spotify Lib
#include "audio.h"
#include "ipc/msg_handler.h"

#include "log.h"
#include "spotify.h"
#include "appkey.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#ifdef MONTAGE_CTRL_PLAYER
#include <mon.h>
#endif
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#ifdef JSONC
#include <json.h>
#else
#include <json/json.h>
#endif

#define clear_buffer()

static int playing;
static int active;

// extern value
//int isLogined = 0;
// static value
int isAvaliablePlayer = 0;
int isWlanConnected = 0;
static int isNeedToRestartAudioThread = 0;
// static for spotify
static uint8_t memory_block[SP_RECOMMENDED_MEMORY_BLOCK_SIZE];
static SpError error_occurred = kSpErrorOk;
static struct SpMetadata previous_metadata = {0};
static int exit_signal;

static struct SpAudioCallbacks audio_cb;

static void sig_handler(int signo) { exit_signal = 1; }

static void setup_signals(void)
{
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
}

#define cprintf(fmt, args...) do { \
    FILE *fp = fopen("/dev/console", "w"); \
    if (fp) { \
        fprintf(fp, fmt , ## args); \
        fclose(fp); \
        } \
    } while (0)

void LOG(const char *format, ...) {
    char buf[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

#if 1
	//openlog("SPOTIFY", 0, 0);
	//syslog(LOG_USER | LOG_NOTICE, "[%lu] %s", gettimevalmsec(NULL), buf);
	//closelog();
    cprintf("[%lu] %s", gettimevalmsec(NULL), buf);
#else
    fprintf(stderr, "%s\n", buf);
#endif
}

void LogMessage(const char *debug_message, void *context) {
    LOG(debug_message);
}

void BlobCredentials(const char *credentials_blob,
                     const char *username, uint32_t device_alias,
                     void *context) {
    LOG("credentials_blob: %s\n", credentials_blob);
}

void ConnectionMessage(const char *message, void *context) {
    LOG("message: %s\n", message);
}

void DebugMessage(const char *debug_message, void *context) {
    LOG("Debug: %s\n", debug_message);
}

void ErrorCallback(SpError err, void *context) {
    LOG("Spotify Embedded error %d occurred\n", err);
#if 0
    if (err == kSpErrorCorruptTrack)
        ;
    else
        error_occurred = err;
#endif
}

void ConnectionNotify(enum SpConnectionNotification event, void *context) {
    if (event == kSpConnectionNotifyLoggedIn) {
        LOG("Logged in successfully\n");
        isLogined = 1;

        // Spotify app seems set to mute as user logout/login, so update volume anyway.
        long vol = get_audio_volume_ra();
        if (vol != -1 && vol != VOL_SP_TO_RA_SCALE(SpPlaybackGetVolume()))
            SpPlaybackUpdateVolume(VOL_RA_TO_SP_SCALE(vol));

    } else if (event == kSpConnectionNotifyLoggedOut) {
        LOG("Logged out\n");
        isLogined = 0;
    }
}

void PlaybackApplyVolume(uint16_t volume, void *context) {
    /*
    * Call an application-defined function that applies the volume
    * to the output of the audio driver.
    */
    //LOG("-------------- PlaybackApplyVolume : %d\n", volume);
    audio_cb.audio_volume(volume);
}

void PlaybackUnavailableTrack(const char *track_uri, void *context) {
    LOG("PlaybackUnavailableTrack: %s\n", track_uri);
}

static void CallbackPlaybackNotify(enum SpPlaybackNotification event,
				   void *context)
{
	struct SpMetadata metadata;
	switch (event) {
	case kSpPlaybackNotifyPlay:
		clear_buffer();
		LOG("Playback status: playing\n");

        // dut is not active device, do noting
        if (SpPlaybackIsActiveDevice() == 0)
        {
            LOG("IsActiveDevice %d", SpPlaybackIsActiveDevice());
            break;
        }

		#ifdef MONTAGE_CTRL_PLAYER
		inform_ctrl_msg(MONSPOTIFY, NULL);
		#endif

		if (active && !playing) {
			audio_cb.audio_pause(0);
#ifdef RING_BUFFER_ENABLE
			start_audio_thread();
#endif
		}
		playing = 1;
		isAvaliablePlayer = 1;
		break;
	case kSpPlaybackNotifyPause:
		clear_buffer();
		LOG("Playback status: paused\n");
		if (active && playing)
			audio_cb.audio_pause(1);
		playing = 0;
		break;
	case kSpPlaybackNotifyTrackChanged:
		LOG("Track changed\n");
		if (SpGetMetadata(&metadata, kSpMetadataTrackCurrent) ==
		    kSpErrorOk) {
			LOG("Track event: start of %s -- %s\n", metadata.artist,
			    metadata.track);
			if (active && playing)
				audio_cb.audio_changed(metadata.duration_ms);
		} else {
			LOG("Track event: start of unknown track\n");
			if (active && playing)
				audio_cb.audio_changed(0);
		}
		break;
	case kSpPlaybackNotifyMetadataChanged:
		LOG("Metadata changed\n");
		break;
	case kSpPlaybackNotifyNext:
		clear_buffer();
		LOG("Playing skipped to next track\n");
		break;
	case kSpPlaybackNotifyPrev:
		clear_buffer();
		LOG("Playing jumped to previous track\n");
		break;
	case kSpPlaybackNotifyShuffleOn:
		clear_buffer();
		LOG("Shuffle status: 1\n");
		break;
	case kSpPlaybackNotifyShuffleOff:
		clear_buffer();
		LOG("Shuffle status: 0\n");
		break;
	case kSpPlaybackNotifyRepeatOn:
		clear_buffer();
		LOG("Repeat status: 1\n");
		break;
	case kSpPlaybackNotifyRepeatOff:
		clear_buffer();
		LOG("Repeat status: 0\n");
		break;
	case kSpPlaybackNotifyBecameActive:
		active = 1;
#if !defined(__PANTHER__)
		if (playing)
			audio_cb.audio_pause(0);
#else
		audio_cb.audio_open();
#endif
		LOG("Became active\n");
		break;
	case kSpPlaybackNotifyBecameInactive:
		active = 0;
		LOG("Became inactive\n");
#if !defined(__PANTHER__)
		if (playing) {
			audio_cb.audio_pause(1);
			playing = 0;
		}
#else
		audio_cb.audio_close();
#endif
		break;
	case kSpPlaybackNotifyLostPermission:
		LOG("Lost permission\n");
		if (playing)
			audio_cb.audio_pause(1);
		break;
	case kSpPlaybackEventAudioFlush:
		LOG("Audio flush\n");
		audio_cb.audio_flush();
		break;
	case kSpPlaybackNotifyAudioDeliveryDone:
		LOG("Audio delivery done\n");
		break;
	case kSpPlaybackNotifyTrackDelivered:
		LOG("Track delivered\n");
		break;
	default:
		break;
	}
}

void PlaybackSeek(uint32_t position_ms, void *context) {
    LOG("PlaybackSeek: Playback seeked to position %d:%02d\n",
        (position_ms / 1000) / 60, (position_ms / 1000) % 60);
}

static void update_wan_state()
{
    char state[8] = {0};

    int ret = cdb_get("$wanif_state", &state);

    if (state[0] == '2')
        isWlanConnected = 1;
    else
        isWlanConnected = 0;
}

static void update_volume()
{
    long vol = get_audio_volume_ra();
    if (vol != -1 && vol != VOL_SP_TO_RA_SCALE(SpPlaybackGetVolume()))
        SpPlaybackUpdateVolume(VOL_RA_TO_SP_SCALE(vol));
}

static void do_button_event()
{
    if (button_pressed == kButtonDisplayName) {
        char display_name[SP_MAX_DISPLAY_NAME_LENGTH] = {0};
        int ret = cdb_get("$spotify_display_name", &display_name);
        if (ret < 0 || strlen(display_name) == 0) {
            LOG("set display name failed\n");
        } else {
            SpSetDisplayName(display_name);
            LOG("set display name %s\n", display_name);
        }
        button_pressed = kButtonNone;
    } else if (button_pressed == kButtonChangeSource) {
        isAvaliablePlayer = 0;
        SpPlaybackPause();
        button_pressed = kButtonNone;
    } else if (button_pressed == kButtonToggle) {
        if (SpPlaybackIsPlaying())
            SpPlaybackPause();
        else
            SpPlaybackPlay();
        button_pressed = kButtonNone;
    }
}

static void receive_event(struct ubus_context *ctx, struct ubus_event_handler *ev,
                          const char *type, struct blob_attr *msg)
{
    char *str;
    struct json_object *obj_msg, *obj_id;

    str = blobmsg_format_json(msg, true);
    obj_msg = json_tokener_parse(str);
    if (obj_msg && (obj_id = json_object_object_get(obj_msg, "sender"))) {
        if (strcmp("spotify", json_object_get_string(obj_id))) {
            //LOG("receive event %s\n", type);
            SpPlaybackPause();
        }
    } else {
        LOG("Failed to parse msg or no key named \"sender\" in event %s\n", type);
    }
    free(str);
}

static int ubus_event_register(struct ubus_context *ctx, char event[])
{
    static struct ubus_event_handler listener;
    int ret = 0;

    memset(&listener, 0, sizeof(listener));
    listener.cb = receive_event;

    ret = ubus_register_event_handler(ctx, &listener, event);

    if (ret) {
        LOG("Error while registering for event '%s': %s\n", event, ubus_strerror(ret));
        return -1;
    }

    return 0;
}

#ifndef PATH_TO_APP_KEY_FILE
#define PATH_TO_APP_KEY_FILE "spotify_appkey.key"
#endif

#define CLIENT_ID ""

int main(int argc, char *argv[]) {
    SpError err = kSpErrorOk;
    struct SpConfig config;
    struct SpConnectionCallbacks connection_cb;
    struct SpPlaybackCallbacks playback_cb;
    struct SpDebugCallbacks debug_cb;
    struct SpMetadata metadata;
    uint8_t playing, shuffle, repeat;
    uint32_t position_sec;
    struct ubus_context *ctx;
    int ret = 0;
    fd_set fds;
    struct timeval timeout = {0, 0};
    char uevent_name[] = "stop_music";

    setup_signals();

    //pthread_mutex_init(&esdk_mutex, 0);
    memset(&config, 0, sizeof(config));

    config.api_version = SP_API_VERSION;
    config.memory_block = memory_block;
    config.memory_block_size = sizeof(memory_block);
    config.device_type = kSpDeviceTypeSpeaker;

    // app_key is deprecated,
    // in our experience, user can't be logged in by ZeroConfig with app key after eSDK version 2.37
    // The Client ID identifies the application using Spotify,
    // Register your application on Spotify Development Website
    if (strlen(CLIENT_ID))
        config.client_id = CLIENT_ID;
    else {
        const char *kfile = PATH_TO_APP_KEY_FILE;
        if (kfile && kSpErrorOk != (err = SpReadAppkey(kfile))) {
            LOG("Cannot read key file \"%s\"!\n", kfile);
            err = -1;
            goto end;
        }

        config.app_key = g_appkey;
        config.app_key_size = g_appkey_size;
    }

    int i, j = 0;
    char bmac0[32] = {0}, mac0[16] = {0};
    char unique_id[SP_MAX_UNIQUE_ID_LENGTH] = {0};
    char display_name[SP_MAX_DISPLAY_NAME_LENGTH] = {0};
    char user_display_name[SP_MAX_DISPLAY_NAME_LENGTH] = {0};
    char brand_name[SP_MAX_BRAND_NAME_LENGTH] = {0};
    char model_name[SP_MAX_MODEL_NAME_LENGTH] = {0};

    SpAudioGetCallbacks(&audio_cb);

    boot_cdb_get("mac0", &bmac0);
    for (i=0; i<strlen(bmac0) && i<32; i++) {
        if (isxdigit(bmac0[i])) {
            mac0[j] = bmac0[i];
            j++;
        }
    }

    cdb_get("$spotify_display_name", &user_display_name);
    cdb_get("$spotify_brand_name", &brand_name);
    cdb_get("$spotify_model_name", &model_name);

    sprintf(unique_id, "Montage%s", mac0);
    sprintf(display_name, "Montage%s", mac0+6);

    config.unique_id = (argc > 1)? argv[1]: unique_id;
    if (strlen(user_display_name) > 0)
        config.display_name = user_display_name;
    else {
        config.display_name = display_name;
        cdb_set("$spotify_display_name", display_name);
    }
    config.brand_name = (strlen(brand_name) > 0)? brand_name: "Montage_Technology";
    config.model_name = (strlen(model_name) > 0)? model_name: "M88WI6600";

    LOG("unique_id %s\n\n", config.unique_id);
    LOG("display_name %s\n\n", config.display_name);
    LOG("brand_name %s\n\n", config.brand_name);
    LOG("model_name %s\n\n", config.model_name);

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
    config.zeroconf_port_range = 10;
    /*
    * The host name should be unique on the local network.
    * Used in the mDNS based discovery phase.
    */
    config.host_name = config.unique_id;

#if 0
    memset(&debug_cb, 0, sizeof(debug_cb));
    debug_cb.on_message = DebugMessage;
    SpRegisterDebugCallbacks(&debug_cb, NULL);
#endif

    SpSetDebugMessageCallback(LogMessage);

    err = SpInit(&config);
    if (err != kSpErrorOk) {
        LOG("Error %d\n", err);
        goto end;
    }


    memset(&connection_cb, 0, sizeof(connection_cb));
    connection_cb.on_notify = ConnectionNotify;
    connection_cb.on_new_credentials = BlobCredentials;
    connection_cb.on_message = ConnectionMessage;
    SpRegisterConnectionCallbacks(&connection_cb, NULL);

#if 0
    err = SpConnectionLoginPassword("user", "password");
    if (err != kSpErrorOk) {
    /* The function would only return an error if we hadn't invoked SpInit()
    * or if we had passed an empty username or password.
    * Actual login errors will be reported to ErrorCallback().
    */
        LOG("Error %d\n", err);
        SpFree();
        return 0;
    }
#endif

    memset(&playback_cb, 0, sizeof(playback_cb));
    playback_cb.on_audio_data = audio_cb.audio_data;
    playback_cb.on_apply_volume = PlaybackApplyVolume;
    playback_cb.on_notify = CallbackPlaybackNotify;
    playback_cb.on_seek = PlaybackSeek;
    playback_cb.on_unavailable_track = PlaybackUnavailableTrack;
    SpRegisterPlaybackCallbacks(&playback_cb, NULL);

    err = SpConnectionSetConnectivity(kSpConnectivityWireless);
    if (err != kSpErrorOk) {
        LOG("Failed to SetConnectivity %d\n", err);
        goto end;
    }

    ring_buffer_init();
    err = audio_cb.audio_init();
    if (err != kSpErrorOk) {
        LOG("Failed to SoundInitMixer %d\n", err);
        goto end;
    }

    ctx = ubus_connect(NULL);
    if (!ctx) {
        LOG("Failed to connect to ubus\n");
        goto end;
    }
    ret = ubus_event_register(ctx, uevent_name);
    if (ret) {
        goto end;
    }

    ipc_init();
    button_pressed = kButtonNone;

    uint32_t loop_count = 0;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(ctx->sock.fd, &fds);
        ret = select(ctx->sock.fd + 1, &fds, NULL, NULL, &timeout);
        if (ret > 0)
        {
            ubus_handle_event(ctx);
        }
        else if (ret < 0)
        {
            LOG("Spotify select fail %d\n", ret);
            goto end;
        }

        err = SpPumpEvents();

        if (kSpErrorOk != err || exit_signal)
            goto end;

#if 0
        if (isLogined && relogin && have_credentials) {
            SpConnectionLogout();
            while (isLogined)
                SpPumpEvents();
            if (kSpErrorOk != (err = SpConnectionLoginBlob(uname, credentials, 0)))
                goto end;
            relogin = 0;
        }
#endif

        if (loop_count > 100) {
            if (isAvaliablePlayer && active)
                update_volume();
                //update_wan_state();
            loop_count = 0;
        } else
            loop_count++;

        do_button_event();
    }

end:
    SpFree();

    return 0;
}
