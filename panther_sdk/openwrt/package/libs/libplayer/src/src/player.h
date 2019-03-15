/*
 * GeeXboX libplayer: a multimedia A/V abstraction layer API.
 * Copyright (C) 2006-2008 Benjamin Zores <ben@geexbox.org>
 * Copyright (C) 2007-2008 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * This file is part of libplayer.
 *
 * libplayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libplayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libplayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PLAYER_H
#define PLAYER_H

/**
 * \file player.h
 *
 * GeeXboX libplayer public API header.
 */

/**
 * \mainpage
 *
 * libplayer is a multimedia A/V abstraction layer API. Its goal is to
 * interact with Enna Media Center.
 *
 * libplayer provides a generic A/V API that relies on various multimedia
 * player for Linux systems. It currently supports
 * <a href="http://www.mplayerhq.hu">MPlayer</a> (through slave-mode), <a
 * href="http://www.xinehq.de">xine</a>, <a href="http://www.videolan.org">
 * VLC</a> and <a href="http://www.gstreamer.org">GStreamer</a>.
 *
 * Its main goal is to provide an unique API that player frontends can use
 * to control any kind of multimedia player underneath. For example, it
 * provides a library to easily control MPlayer famous slave-mode.
 *
 * \section mtlevel MT-Level
 * Most functions in this API are indicated as being MT-Safe in multithreaded
 * applications. That is right <b>only</b> if the functions are used
 * concurrently with the same (#player_t) controller. Else, unexpected
 * behaviours can appear.
 */

#ifdef __cplusplus
extern "C" {
#if 0 /* avoid EMACS indent */
}
#endif /* 0 */
#endif /* __cplusplus */

#ifndef pl_unused
#if defined(__GNUC__)
#  define pl_unused __attribute__((unused))
#else
#  define pl_unused
#endif
#endif

#define PL_STRINGIFY(s) #s
#define PL_TOSTRING(s) PL_STRINGIFY(s)

#define PL_VERSION_INT(a, b, c) (a << 16 | b << 8 | c)
#define PL_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define PL_VERSION(a, b, c) PL_VERSION_DOT(a, b, c)

#define LIBPLAYER_VERSION_MAJOR  2
#define LIBPLAYER_VERSION_MINOR  0
#define LIBPLAYER_VERSION_MICRO  1

#define LIBPLAYER_VERSION_INT PL_VERSION_INT(LIBPLAYER_VERSION_MAJOR, \
                                             LIBPLAYER_VERSION_MINOR, \
                                             LIBPLAYER_VERSION_MICRO)
#define LIBPLAYER_VERSION     PL_VERSION(LIBPLAYER_VERSION_MAJOR, \
                                         LIBPLAYER_VERSION_MINOR, \
                                         LIBPLAYER_VERSION_MICRO)
#define LIBPLAYER_VERSION_STR PL_TOSTRING(LIBPLAYER_VERSION)
#define LIBPLAYER_BUILD       LIBPLAYER_VERSION_INT

#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

/**
 * \brief Return LIBPLAYER_VERSION_INT constant.
 */
unsigned int libplayer_version (void);


/***************************************************************************/
/*                                                                         */
/* Player (Un)Initialization                                               */
/*  Mandatory for all operations below                                     */
/*                                                                         */
/***************************************************************************/

/**
 * \brief Player controller.
 *
 * This controls a multimedia player.
 */
typedef struct player_s player_t;

/** \brief Player types. */
typedef enum player_type {
  PLAYER_TYPE_XINE,
  PLAYER_TYPE_MPLAYER,
  PLAYER_TYPE_VLC,
  PLAYER_TYPE_GSTREAMER,
  PLAYER_TYPE_DUMMY
} player_type_t;

/** \brief Player video outputs. */
typedef enum player_vo {
  PLAYER_VO_AUTO = 0,
  PLAYER_VO_NULL,
  PLAYER_VO_X11,
  PLAYER_VO_X11_SDL,
  PLAYER_VO_XV,
  PLAYER_VO_GL,
  PLAYER_VO_FB,
  PLAYER_VO_DIRECTFB,
  PLAYER_VO_VDPAU,
  PLAYER_VO_VAAPI,
} player_vo_t;

/** \brief Player audio outputs. */
typedef enum player_ao {
  PLAYER_AO_AUTO = 0,
  PLAYER_AO_NULL,
  PLAYER_AO_ALSA,
  PLAYER_AO_OSS,
  PLAYER_AO_PULSE,
} player_ao_t;

/** \brief Player events. */
typedef enum player_event {
  PLAYER_EVENT_UNKNOWN,
  PLAYER_EVENT_PLAYBACK_START,
  PLAYER_EVENT_PLAYBACK_STOP,
  PLAYER_EVENT_PLAYBACK_FINISHED,
  PLAYER_EVENT_PLAYLIST_FINISHED,
  PLAYER_EVENT_PLAYBACK_PAUSE,
  PLAYER_EVENT_PLAYBACK_UNPAUSE,
} player_event_t;

/** \brief Player verbosity. */
typedef enum {
  PLAYER_MSG_NONE,          /* no error messages */
  PLAYER_MSG_VERBOSE,       /* super-verbose mode: mostly for debugging */
  PLAYER_MSG_INFO,          /* working operations */
  PLAYER_MSG_WARNING,       /* harmless failures */
  PLAYER_MSG_ERROR,         /* may result in hazardous behavior */
  PLAYER_MSG_CRITICAL,      /* prevents lib from working */
} player_verbosity_level_t;

typedef enum {
  PLAYER_QUALITY_NORMAL,    /* normal picture quality */
  PLAYER_QUALITY_LOW,       /* slightly degraded picture for fastest playback */
  PLAYER_QUALITY_LOWEST,    /* degraded picture, suitable for low-end CPU */
} player_quality_level_t;

/** \brief Parameters for player_init() .*/
typedef struct player_init_param_s {
  /** Audio output driver. */
  player_ao_t ao;
  /** Video output driver. */
  player_vo_t vo;
  /** Window ID to attach the video (X Window). */
  uint32_t winid;

  /** Public event callback. */
  int (*event_cb) (player_event_t e, void *data);
  /** User data for event callback. */
  void *data;

  /**
   * Display to use with X11 video outputs.
   *
   * The string has to follow the same rules that the DISPLAY environment
   * variable. If \p display is NULL, then the environment variable is
   * considered.
   */
  const char *display;

  /** Picture decoding quality. */
  player_quality_level_t quality;

} player_init_param_t;

/**
 * \name Player (Un)Initialization.
 * @{
 */

/**
 * \brief Initialization of a new player controller.
 *
 * Multiple player controllers can be initialized with any wrappers.
 * The same Window ID can be used to attach their video.
 *
 * For a description of each parameters supported by this function:
 * \see ::player_init_param_t
 *
 * When a parameter in \p param is 0 (or NULL), its default value is used.
 * If \p param is NULL, then all default values are forced for all parameters.
 *
 * Wrappers supported (even partially):
 *  GStreamer, MPlayer, VLC, xine
 *
 * \param[in] type        Type of wrapper to load.
 * \param[in] verbosity   Level of verbosity to set.
 * \param[in] param       Parameters, NULL for default values.
 * \return Player controller, NULL otherwise.
 */
player_t *player_init (player_type_t type, player_verbosity_level_t verbosity,
                       player_init_param_t *param);

/**
 * \brief Uninitialization of a player controller.
 *
 * All MRL objects in the internal playlist will be freed.
 *
 * Wrappers supported (even partially):
 *  GStreamer, MPlayer, VLC, xine
 *
 * \warning Must be used only as the last player function for a controller.
 * \param[in] player      Player controller.
 */
void player_uninit (player_t *player);

/**
 * \brief Set verbosity level.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] level       Level of verbosity to set.
 */
void player_set_verbosity (player_t *player, player_verbosity_level_t level);

void player_request_stream_read_count (player_t *player);

unsigned long long player_get_stream_read_count(player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Media Resource Locater (MRL) Helpers                                    */
/*  MRLs can have multiple types and are used to define a stream           */
/*                                                                         */
/***************************************************************************/

/**
 * \brief MRL object.
 *
 * This handles an audio, video or image resource.
 */
typedef struct mrl_s mrl_t;

/** \brief MRL types. */
typedef enum mrl_type {
  MRL_TYPE_UNKNOWN,
  MRL_TYPE_AUDIO,
  MRL_TYPE_VIDEO,
  MRL_TYPE_IMAGE,
} mrl_type_t;

/*
 * Support by wrappers
 *
 *                           GStreamer     MPlayer        VLC         xine
 *                         ------------ ------------ ------------ ------------
 */
/** \brief MRL resources. */
typedef enum mrl_resource {
  MRL_RESOURCE_UNKNOWN,

  /* Local Streams */
  MRL_RESOURCE_FIFO,        /*  NO           NO           NO           NO   */
  MRL_RESOURCE_FILE,        /*  YES          YES          YES          YES  */
  MRL_RESOURCE_STDIN,       /*  NO           NO           NO           NO   */

  /* Audio CD */
  MRL_RESOURCE_CDDA,        /*  NO           YES          NO           NO   */
  MRL_RESOURCE_CDDB,        /*  NO           YES          NO           NO   */

  /* Video discs */
  MRL_RESOURCE_DVD,         /*  NO           YES          YES          YES  */
  MRL_RESOURCE_DVDNAV,      /*  NO           YES          YES          YES  */
  MRL_RESOURCE_VCD,         /*  NO           YES          NO           NO   */

  /* Radio/Television */
  MRL_RESOURCE_DVB,         /*  NO           YES          NO           NO   */
  MRL_RESOURCE_PVR,         /*  NO           NO           NO           NO   */
  MRL_RESOURCE_RADIO,       /*  NO           YES          NO           NO   */
  MRL_RESOURCE_TV,          /*  NO           YES          NO           NO   */
  MRL_RESOURCE_VDR,         /*  NO           NO           NO           YES  */

  /* Network Streams */
  MRL_RESOURCE_FTP,         /*  NO           YES          YES          NO   */
  MRL_RESOURCE_HTTP,        /*  NO           YES          YES          YES  */
  MRL_RESOURCE_MMS,         /*  NO           YES          YES          YES  */
  MRL_RESOURCE_NETVDR,      /*  NO           NO           NO           YES  */
  MRL_RESOURCE_RTP,         /*  NO           YES          YES          YES  */
  MRL_RESOURCE_RTSP,        /*  NO           YES          YES          NO   */
  MRL_RESOURCE_SMB,         /*  NO           YES          YES          NO   */
  MRL_RESOURCE_TCP,         /*  NO           NO           NO           YES  */
  MRL_RESOURCE_UDP,         /*  NO           YES          YES          YES  */
  MRL_RESOURCE_UNSV,        /*  NO           YES          YES          NO   */
} mrl_resource_t;

/** \brief Arguments for local streams. */
typedef struct mrl_resource_local_args_s {
  char *location;           /*  YES          YES          YES          YES  */
  int playlist;             /*  NO           NO           NO           NO   */
} mrl_resource_local_args_t;

/** \brief Arguments for audio CD. */
typedef struct mrl_resource_cd_args_s {
  char *device;             /*  NO           YES          NO           NO   */
  uint8_t speed;            /*  NO           YES          NO           NO   */
  uint8_t track_start;      /*  NO           YES          NO           NO   */
  uint8_t track_end;        /*  NO           YES          NO           NO   */
} mrl_resource_cd_args_t;

/** \brief Arguments for video discs. */
typedef struct mrl_resource_videodisc_args_s {
  char *device;             /*  NO           YES          YES          YES  */
  uint8_t speed;            /*  NO           NO           NO           NO   */
  uint8_t angle;            /*  NO           YES          YES          NO   */
  uint8_t title_start;      /*  NO           YES          YES          YES  */
  uint8_t title_end;        /*  NO           YES          NO           NO   */
  uint8_t chapter_start;    /*  NO           NO           YES          NO   */
  uint8_t chapter_end;      /*  NO           NO           NO           NO   */
  uint8_t track_start;      /*  NO           YES          NO           NO   */
  uint8_t track_end;        /*  NO           NO           NO           NO   */
  char *audio_lang;         /*  NO           NO           NO           NO   */
  char *sub_lang;           /*  NO           NO           NO           NO   */
  uint8_t sub_cc;           /*  NO           NO           NO           NO   */
} mrl_resource_videodisc_args_t;

/** \brief Arguments for radio/tv streams. */
typedef struct mrl_resource_tv_args_s {
  char *device;             /*  NO           NO           NO           YES  */
  char *driver;             /*  NO           NO           NO           YES  */
  char *channel;            /*  NO           YES          NO           NO   */
  uint8_t input;            /*  NO           YES          NO           NO   */
  int width;                /*  NO           NO           NO           NO   */
  int height;               /*  NO           NO           NO           NO   */
  int fps;                  /*  NO           NO           NO           NO   */
  char *output_format;      /*  NO           NO           NO           NO   */
  char *norm;               /*  NO           YES          NO           NO   */
} mrl_resource_tv_args_t;

/** \brief Arguments for network streams. */
typedef struct mrl_resource_network_args_s {
  char *url;                /*  NO           YES          NO           YES  */
  char *username;           /*  NO           YES          NO           NO   */
  char *password;           /*  NO           YES          NO           NO   */
  char *user_agent;         /*  NO           NO           NO           NO   */
} mrl_resource_network_args_t;

/** \brief Snapshot image file type. */
typedef enum mrl_snapshot {
  MRL_SNAPSHOT_JPG,         /*  NO           YES          NO           NO   */
  MRL_SNAPSHOT_PNG,         /*  NO           YES          NO           NO   */
  MRL_SNAPSHOT_PPM,         /*  NO           YES          NO           NO   */
  MRL_SNAPSHOT_TGA,         /*  NO           NO           NO           NO   */
} mrl_snapshot_t;

/** \brief MRL metadata. */
typedef enum mrl_metadata_type {
  MRL_METADATA_TITLE,
  MRL_METADATA_ARTIST,
  MRL_METADATA_GENRE,
  MRL_METADATA_ALBUM,
  MRL_METADATA_YEAR,
  MRL_METADATA_TRACK,
  MRL_METADATA_COMMENT,
} mrl_metadata_type_t;

/** \brief MRL CDDA/CDDB metadata. */
typedef enum mrl_metadata_cd_type {
  MRL_METADATA_CD_DISCID,
  MRL_METADATA_CD_TRACKS,
} mrl_metadata_cd_type_t;

/** \brief MRL DVD/DVDNAV metadata. */
typedef enum mrl_metadata_dvd_type {
  MRL_METADATA_DVD_TITLE_CHAPTERS,
  MRL_METADATA_DVD_TITLE_ANGLES,
  MRL_METADATA_DVD_TITLE_LENGTH,
} mrl_metadata_dvd_type_t;

/** \brief MRL properties. */
typedef enum mrl_properties_type {
  MRL_PROPERTY_SEEKABLE,
  MRL_PROPERTY_LENGTH,
  MRL_PROPERTY_AUDIO_BITRATE,
  MRL_PROPERTY_AUDIO_BITS,
  MRL_PROPERTY_AUDIO_CHANNELS,
  MRL_PROPERTY_AUDIO_SAMPLERATE,
  MRL_PROPERTY_VIDEO_BITRATE,
  MRL_PROPERTY_VIDEO_WIDTH,
  MRL_PROPERTY_VIDEO_HEIGHT,
  MRL_PROPERTY_VIDEO_ASPECT,
  MRL_PROPERTY_VIDEO_CHANNELS,
  MRL_PROPERTY_VIDEO_STREAMS,
  MRL_PROPERTY_VIDEO_FRAMEDURATION,
} mrl_properties_type_t;

#define PLAYER_VIDEO_ASPECT_RATIO_MULT         10000.0    /* *10000         */
#define PLAYER_VIDEO_FRAMEDURATION_RATIO_DIV   90000.0    /* 1/90000 sec    */

/**
 * \name Media Resource Locater (MRL) Helpers.
 * @{
 */

/**
 * \brief Create a new MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * The argument \p args and the strings provided with \p args must be
 * allocated dynamically. The pointers are freed by libplayer when a mrl
 * is no longer available.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] res         Resource type.
 * \param[in] args        Arguments specific to the resource type.
 * \return MRL object, NULL otherwise.
 */
mrl_t *mrl_new (player_t *player, mrl_resource_t res, void *args);

/**
 * \brief Add a subtitle file to a MRL object.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] subtitle    Location of the subtitle file to be added.
 */
void mrl_add_subtitle (player_t *player, mrl_t *mrl, char *subtitle);

/**
 * \brief Free a MRL object.
 *
 * Never use this function when the MRL (or a linked MRL) is set in the
 * playlist of a player controller.
 *
 * \warning Must be used only as the last mrl function for one MRL object.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object.
 */
void mrl_free (player_t *player, mrl_t *mrl);

/**
 * \brief Get type of the stream.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Type of MRL object.
 */
mrl_type_t mrl_get_type (player_t *player, mrl_t *mrl);

/**
 * \brief Get resource of the stream.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Resource of MRL object.
 */
mrl_resource_t mrl_get_resource (player_t *player, mrl_t *mrl);

/**
 * \brief Get metadata of the stream.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning The returned pointer must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] m           Type of metadata to get.
 * \return Metadata string, NULL otherwise.
 */
char *mrl_get_metadata (player_t *player, mrl_t *mrl, mrl_metadata_type_t m);

/**
 * \brief Get metadata of a track with CDDA/CDDB MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning The returned pointer must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] trackid     Track ID on the CD.
 * \param[out] length     Length of the track (millisecond).
 * \return Title of the track (CDDB only), NULL otherwise.
 */
char *mrl_get_metadata_cd_track (player_t *player,
                                 mrl_t *mrl, int trackid, uint32_t *length);

/**
 * \brief Get metadata of a CDDA/CDDB MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] m           Type of metadata to get.
 * \return Metadata value.
 */
uint32_t mrl_get_metadata_cd (player_t *player,
                              mrl_t *mrl, mrl_metadata_cd_type_t m);

/**
 * \brief Get metadata of a title with DVD/DVDNAV MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] titleid     Title ID on the DVD.
 * \param[in] m           Type of metadata to get.
 * \return Metadata value.
 */
uint32_t mrl_get_metadata_dvd_title (player_t *player, mrl_t *mrl,
                                     int titleid, mrl_metadata_dvd_type_t m);

/**
 * \brief Get metadata of a DVD/DVDNAV MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning The returned pointer must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[out] titles     How many titles on the DVD.
 * \return Volume ID, NULL otherwise.
 */
char *mrl_get_metadata_dvd (player_t *player, mrl_t *mrl, uint8_t *titles);

/**
 * \brief Get subtitle metadata of the MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * The \p pos argument is the position of the subtitle in the internal list
 * of libplayer. The first subtitle begins with 1.
 * \p id returned by this function can be used with player_subtitle_select().
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning The pointers (\p name and \p lang) must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] pos         Position of the subtitle.
 * \param[out] id         ID of the subtitle, NULL to ignore.
 * \param[out] name       Name of the subtitle, NULL to ignore.
 * \param[out] lang       Language of the subtitle, NULL to ignore.
 * \return 1 for success, 0 if the subtitle is not available.
 */
int mrl_get_metadata_subtitle (player_t *player, mrl_t *mrl, int pos,
                               uint32_t *id, char **name, char **lang);

/**
 * \brief Get the number of available subtitles.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Number of subtitles.
 */
uint32_t mrl_get_metadata_subtitle_nb (player_t *player, mrl_t *mrl);

/**
 * \brief Get audio metadata of the MRL object.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * The \p pos argument is the position of the audio stream in the internal list
 * of libplayer. The first audio stream begins with 1.
 * \p id returned by this function can be used with player_audio_select().
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning The pointers (\p name and \p lang) must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] pos         Position of the audio stream.
 * \param[out] id         ID of the audio stream, NULL to ignore.
 * \param[out] name       Name of the audio stream, NULL to ignore.
 * \param[out] lang       Language of the audio stream, NULL to ignore.
 * \return 1 for success, 0 if the audio stream is not available.
 */
int mrl_get_metadata_audio (player_t *player, mrl_t *mrl, int pos,
                            uint32_t *id, char **name, char **lang);

/**
 * \brief Get the number of available audio streams.
 *
 * This function can be slow when the stream is not (fastly) reachable.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Number of audio streams.
 */
uint32_t mrl_get_metadata_audio_nb (player_t *player, mrl_t *mrl);

/**
 * \brief Get property of the stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] p           Type of property.
 * \return Property value.
 */
uint32_t mrl_get_property (player_t *player,
                           mrl_t *mrl, mrl_properties_type_t p);

/**
 * \brief Get audio codec name of the stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning The returned pointer must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Audio codec name, NULL otherwise.
 */
char *mrl_get_audio_codec (player_t *player, mrl_t *mrl);

/**
 * \brief Get video codec name of the stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning The returned pointer must be freed when no longer used.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Video codec name, NULL otherwise.
 */
char *mrl_get_video_codec (player_t *player, mrl_t *mrl);

/**
 * \brief Get size of the resource.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \return Size of the stream (bytes).
 */
off_t mrl_get_size (player_t *player, mrl_t *mrl);

/**
 * \brief Take a video snapshot.
 *
 * One frame at the \p pos (in second) is saved to \p dst.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object, NULL for current.
 * \param[in] pos         Time position (second).
 * \param[in] t           Image file type.
 * \param[in] dst         Destination file, NULL for default filename
 *                        in the current directory.
 */
void mrl_video_snapshot (player_t *player, mrl_t *mrl,
                         int pos, mrl_snapshot_t t, const char *dst);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Player to MRL connection                                                */
/*                                                                         */
/***************************************************************************/

/** \brief Player MRL add mode. */
typedef enum player_mrl_add {
  PLAYER_MRL_ADD_NOW,
  PLAYER_MRL_ADD_QUEUE,
  PLAYER_MRL_INSERT_QUEUE
} player_mrl_add_t;

/**
 * \name Player to MRL connection.
 * @{
 */

/**
 * \brief Get current MRL set in the internal playlist.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return MRL object.
 */
mrl_t *player_mrl_get_current (player_t *player);

/**
 * \brief Set MRL object in the internal playlist.
 *
 * If a MRL was already set in the playlist, then the current is freed and
 * replaced by the new MRL object.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object to set.
 */
void player_mrl_set (player_t *player, mrl_t *mrl);

/**
 * \brief Append MRL object in the internal playlist.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] mrl         MRL object to append.
 * \param[in] when        Just append, or append and go to the end to play.
 */
void player_mrl_append (player_t *player, mrl_t *mrl, player_mrl_add_t when);

/**
 * \brief Remove current MRL object in the internal playlist.
 *
 * Current MRL object is freed on the way.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_mrl_remove (player_t *player);

/**
 * \brief Remove all MRL objects in the internal playlist.
 *
 * All MRL objects are freed on the way.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_mrl_remove_all (player_t *player);

/**
 * \brief Go the the previous MRL object in the internal playlist.
 *
 * Playback is started if a previous MRL object exists.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_mrl_previous (player_t *player);

/**
 * \brief Go the the next MRL object in the internal playlist.
 *
 * Playback is started if a next MRL object exists.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_mrl_next (player_t *player);

/**
 * \brief Go to the next MRL object accordingly to the loop and shuffle.
 *
 * The behaviour is the same that player_mrl_next() if the 'loop' or the 'shuffle'
 * is not enabled and the playback mode is not AUTO.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_mrl_continue (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Player tuning & properties                                              */
/*                                                                         */
/***************************************************************************/

/** \brief Player playback mode. */
typedef enum player_pb {
  PLAYER_PB_SINGLE = 0,
  PLAYER_PB_AUTO,
} player_pb_t;

/** \brief Player loop mode. */
typedef enum player_loop {
  PLAYER_LOOP_DISABLE = 0,
  PLAYER_LOOP_ELEMENT,
  PLAYER_LOOP_PLAYLIST,
} player_loop_t;

/** \brief Player frame dropping mode. */
typedef enum player_framedrop {
  PLAYER_FRAMEDROP_DISABLE,
  PLAYER_FRAMEDROP_SOFT,
  PLAYER_FRAMEDROP_HARD,
} player_framedrop_t;

/** \brief Player X11 window flags. */
typedef enum player_x_window_flags {
  PLAYER_X_WINDOW_AUTO = 0,
  PLAYER_X_WINDOW_X    = (1 << 0),
  PLAYER_X_WINDOW_Y    = (1 << 1),
  PLAYER_X_WINDOW_W    = (1 << 2),
  PLAYER_X_WINDOW_H    = (1 << 3),
} player_x_window_flags_t;

/**
 * \name Player tuning & properties.
 * @{
 */

/**
 * \brief Get current time position in the current stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return Time position (millisecond).
 */
int player_get_time_pos (player_t *player);

#ifdef QQ_COMPATIBLE
/**
 * \new added.
 */
int player_get_time_len (player_t *player);
#endif
/**
 * \brief Get percent position in the current stream.
 *
 * Wrapper supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return Percent position.
 */
int player_get_percent_pos (player_t *player);

/**
 * \brief Set playback mode.
 *
 * If the playback mode is set to PLAYER_PB_AUTO, then loop and shuffle can
 * be used with the internal playlist. By default, AUTO will just going
 * to the next available MRL object in the playlist and start a new playback.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] pb          Mode to use.
 */
void player_set_playback (player_t *player, player_pb_t pb);

/**
 * \brief Set loop mode and value.
 *
 * Only enabled if playback mode is auto, see player_set_playback().
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] loop        Mode to use (one element or the whole playlist).
 * \param[in] value       How many loops, negative for infinite.
 */
void player_set_loop (player_t *player, player_loop_t loop, int value);

/**
 * \brief Shuffle playback in the internal playlist.
 *
 * Only enabled if playback mode is auto, see player_set_playback().
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Different of 0 to enable.
 */
void player_set_shuffle (player_t *player, int value);

/**
 * \brief Set frame dropping with video playback.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] fd          Frame dropping type to set.
 */
void player_set_framedrop (player_t *player, player_framedrop_t fd);

/**
 * \brief Set the mouse position to the player.
 *
 * The main goal is to select buttons in DVD menu. The coordinates are
 * relative to the top-left corner of the root window. The root window is
 * \p winid passed with player_init().
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] x           X coordinate (pixel).
 * \param[in] y           Y coordinate (pixel).
 */
void player_set_mouse_position (player_t *player, int x, int y);

/**
 * \brief Set properties of X11 window handled by libplayer.
 *
 * Origin to the top-left corner.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning Only usable with video outputs X11 compliant.
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] x           X coordinate (pixel).
 * \param[in] y           Y coordinate (pixel).
 * \param[in] w           Width (pixel).
 * \param[in] h           Height (pixel).
 * \param[in] flags       Flags to select properties to change.
 */
void player_x_window_set_properties (player_t *player,
                                     int x, int y, int w, int h, int flags);

/**
 * \brief Show a text on the On-screen Display.
 *
 * Coordinates are not usable with MPlayer wrapper. The text is always shown
 * from the top-left corner.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] text        Text to show on the OSD.
 * \param[in] x           X coordinate (pixel).
 * \param[in] y           Y coordinate (pixel).
 * \param[in] duration    Duration (millisecond).
 */
void player_osd_show_text (player_t *player,
                           const char *text, int x, int y, int duration);

/**
 * \brief Enable/disable On-screen Display.
 *
 * With the MPlayer wrapper, this function must be called after every
 * player_playback_start() if OSD must be disabled.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Different of 0 to enable.
 */
void player_osd_state (player_t *player, int value);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Playback related controls                                               */
/*                                                                         */
/***************************************************************************/

/** \brief Player playback state. */
typedef enum player_pb_state {
  PLAYER_PB_STATE_IDLE,
  PLAYER_PB_STATE_PAUSE,
  PLAYER_PB_STATE_PLAY,
} player_pb_state_t;

/** \brief Player playback seek mode. */
typedef enum player_pb_seek {
  PLAYER_PB_SEEK_RELATIVE,
  PLAYER_PB_SEEK_ABSOLUTE,
  PLAYER_PB_SEEK_PERCENT,
} player_pb_seek_t;

/**
 * \name Playback related controls.
 * @{
 */

/**
 * \brief Get current playback state.
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return Playback state.
 */
player_pb_state_t player_playback_get_state (player_t *player);

/**
 * \brief Start a new playback.
 *
 * The playback is always started from the beginning.
 *
 * Wrappers supported (even partially):
 *  GStreamer, MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_playback_start (player_t *player);

/**
 * \brief Stop playback.
 *
 * Wrappers supported (even partially):
 *  GStreamer, MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_playback_stop (player_t *player);

/**
 * \brief Pause and unpause playback.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_playback_pause (player_t *player);

/**
 * \brief Seek in the stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Value for seeking (millisecond or percent).
 * \param[in] seek        Seeking mode.
 */
void player_playback_seek (player_t *player, int value, player_pb_seek_t seek);

/**
 * \brief Seek chapter in the stream.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Value for seeking.
 * \param[in] absolute    Mode, 0 for relative.
 */
void player_playback_seek_chapter (player_t *player, int value, int absolute);

/**
 * \brief Change playback speed.
 *
 * This function can't be used to play in backward.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Factor of playback speed to set.
 */
void player_playback_speed (player_t *player, float value);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Audio related controls                                                  */
/*                                                                         */
/***************************************************************************/

/** \brief Player mute state. */
typedef enum player_mute {
  PLAYER_MUTE_UNKNOWN,
  PLAYER_MUTE_ON,
  PLAYER_MUTE_OFF
} player_mute_t;

/**
 * \name Audio related controls.
 * @{
 */

/**
 * \brief Get current volume.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return Volume (percent).
 */
int player_audio_volume_get (player_t *player);

/**
 * \brief Set volume.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Volume to set (percent).
 */
void player_audio_volume_set (player_t *player, int value);

/**
 * \brief Get mute state.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \return Mute state.
 */
player_mute_t player_audio_mute_get (player_t *player);

/**
 * \brief Set mute state.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Mute state to set.
 */
void player_audio_mute_set (player_t *player, player_mute_t value);

/**
 * \brief Set audio delay.
 *
 * Only useful with video files to set delay between audio and video streams.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Delay to set (millisecond).
 * \param[in] absolute    Mode, 0 for relative.
 */
void player_audio_set_delay (player_t *player, int value, int absolute);

/**
 * \brief Select audio ID.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] audio_id    ID of the audio stream to select.
 */
void player_audio_select (player_t *player, int audio_id);

/**
 * \brief Select the previous audio ID.
 *
 * It stays on the same audio ID if no previous stream exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_audio_prev (player_t *player);

/**
 * \brief Select the next audio ID.
 *
 * It stays on the same audio ID if no next stream exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_audio_next (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Video related controls                                                  */
/*                                                                         */
/***************************************************************************/

/** \brief Player video aspect. */
typedef enum player_video_aspect {
  PLAYER_VIDEO_ASPECT_BRIGHTNESS,
  PLAYER_VIDEO_ASPECT_CONTRAST,
  PLAYER_VIDEO_ASPECT_GAMMA,
  PLAYER_VIDEO_ASPECT_HUE,
  PLAYER_VIDEO_ASPECT_SATURATION,
} player_video_aspect_t;

/**
 * \name Video related controls.
 * @{
 */

/**
 * \brief Set video aspect.
 *
 * Wrappers supported (even partially):
 *  none
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] aspect      Aspect to change.
 * \param[in] value       Value for aspect to set.
 * \param[in] absolute    Mode, 0 for relative.
 */
void player_video_set_aspect (player_t *player, player_video_aspect_t aspect,
                              int8_t value, int absolute);

/**
 * \brief Set video panscan.
 *
 * Wrappers supported (even partially):
 *  none
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Value for panscan to set.
 * \param[in] absolute    Mode, 0 for relative.
 */
void player_video_set_panscan (player_t *player, int8_t value, int absolute);

/**
 * \brief Set video aspect ratio.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Ratio to set.
 */
void player_video_set_aspect_ratio (player_t *player, float value);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Subtitles related controls                                              */
/*                                                                         */
/***************************************************************************/

/** \brief Player subtitle alignment. */
typedef enum player_sub_alignment {
  PLAYER_SUB_ALIGNMENT_TOP,
  PLAYER_SUB_ALIGNMENT_CENTER,
  PLAYER_SUB_ALIGNMENT_BOTTOM,
} player_sub_alignment_t;

/**
 * \name Subtitles related controls.
 * @{
 */

/**
 * \brief Set subtitle delay.
 *
 * Only useful with video files to set delay between audio stream and
 * the subtitles.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Delay to set (millisecond).
 */
void player_subtitle_set_delay (player_t *player, int value);

/**
 * \brief Set subtitle alignment.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] a           Alignment to set.
 */
void player_subtitle_set_alignment (player_t *player,
                                    player_sub_alignment_t a);

/**
 * \brief Set subtitle position.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Position to set.
 */
void player_subtitle_set_position (player_t *player, int value);

/**
 * \brief Set subtitle visibility.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Different of 0 to view the subtitles.
 */
void player_subtitle_set_visibility (player_t *player, int value);

/**
 * \brief Set subtitle scale.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Scale to set.
 * \param[in] absolute    Mode, 0 for relative.
 */
void player_subtitle_scale (player_t *player, int value, int absolute);

/**
 * \brief Select subtitle ID.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] sub_id      ID of the subtitle to select.
 */
void player_subtitle_select (player_t *player, int sub_id);

/**
 * \brief Select the previous subtitle ID.
 *
 * It stays on the same subtitle ID if no previous subtitle exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_subtitle_prev (player_t *player);

/**
 * \brief Select the next subtitle ID.
 *
 * It stays on the same subtitle ID if no next subtitle exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_subtitle_next (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* DVD specific controls                                                   */
/*                                                                         */
/***************************************************************************/

/** \brief Player DVDnav commands. */
typedef enum player_dvdnav {
  PLAYER_DVDNAV_UP,
  PLAYER_DVDNAV_DOWN,
  PLAYER_DVDNAV_RIGHT,
  PLAYER_DVDNAV_LEFT,
  PLAYER_DVDNAV_MENU,
  PLAYER_DVDNAV_SELECT,
  PLAYER_DVDNAV_PREVMENU,
  PLAYER_DVDNAV_MOUSECLICK,
} player_dvdnav_t;

/**
 * \name DVD specific controls.
 * @{
 */

/**
 * \brief DVD Navigation commands.
 *
 * Wrappers supported (even partially):
 *  MPlayer, xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Command to send.
 */
void player_dvd_nav (player_t *player, player_dvdnav_t value);

/**
 * \brief Select DVD angle.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] angle       Angle to select.
 */
void player_dvd_angle_select (player_t *player, int angle);

/**
 * \brief Select the previous DVD angle.
 *
 * It stays on the same if no previous angle exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_dvd_angle_prev (player_t *player);

/**
 * \brief Select the next DVD angle.
 *
 * It stays on the same if no next angle exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_dvd_angle_next (player_t *player);

/**
 * \brief Select DVD title.
 *
 * Wrappers supported (even partially):
 *  MPlayer, VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] title       Title to select.
 */
void player_dvd_title_select (player_t *player, int title);

/**
 * \brief Select the previous DVD title.
 *
 * It stays on the same if no previous title exists.
 *
 * Wrappers supported (even partially):
 *  VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_dvd_title_prev (player_t *player);

/**
 * \brief Select the next DVD title.
 *
 * It stays on the same if no next title exists.
 *
 * Wrappers supported (even partially):
 *  VLC
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_dvd_title_next (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* TV/DVB specific controls                                                */
/*                                                                         */
/***************************************************************************/

/**
 * \name TV/DVB specific controls.
 * @{
 */

/**
 * \brief Select TV channel.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] channel     Channel to select.
 */
void player_tv_channel_select (player_t *player, const char *channel);

/**
 * \brief Select the previous TV channel.
 *
 * It stays on the same if no previous channel exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_tv_channel_prev (player_t *player);

/**
 * \brief Select the next TV channel.
 *
 * It stays on the same if no next channel exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_tv_channel_next (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Radio specific controls                                                 */
/*                                                                         */
/***************************************************************************/

/**
 * \name Radio specific controls.
 * @{
 */

/**
 * \brief Select radio channel.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] channel     Channel to select.
 */
void player_radio_channel_select (player_t *player, const char *channel);

/**
 * \brief Select the previous radio channel.
 *
 * It stays on the same if no previous channel exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_radio_channel_prev (player_t *player);

/**
 * \brief Select the next radio channel.
 *
 * It stays on the same if no next channel exists.
 *
 * Wrappers supported (even partially):
 *  MPlayer
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 */
void player_radio_channel_next (player_t *player);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* VDR specific controls                                                   */
/*                                                                         */
/***************************************************************************/

/** \brief Player VDR commands. */
typedef enum player_vdr {
  PLAYER_VDR_UP = 0,
  PLAYER_VDR_DOWN,
  PLAYER_VDR_LEFT,
  PLAYER_VDR_RIGHT,
  PLAYER_VDR_OK,
  PLAYER_VDR_BACK,
  PLAYER_VDR_CHANNELPLUS,
  PLAYER_VDR_CHANNELMINUS,
  PLAYER_VDR_RED,
  PLAYER_VDR_GREEN,
  PLAYER_VDR_YELLOW,
  PLAYER_VDR_BLUE,
  PLAYER_VDR_PLAY,
  PLAYER_VDR_PAUSE,
  PLAYER_VDR_STOP,
  PLAYER_VDR_RECORD,
  PLAYER_VDR_FASTFWD,
  PLAYER_VDR_FASTREW,
  PLAYER_VDR_POWER,
  PLAYER_VDR_SCHEDULE,
  PLAYER_VDR_CHANNELS,
  PLAYER_VDR_TIMERS,
  PLAYER_VDR_RECORDINGS,
  PLAYER_VDR_MENU,
  PLAYER_VDR_SETUP,
  PLAYER_VDR_COMMANDS,
  PLAYER_VDR_0,
  PLAYER_VDR_1,
  PLAYER_VDR_2,
  PLAYER_VDR_3,
  PLAYER_VDR_4,
  PLAYER_VDR_5,
  PLAYER_VDR_6,
  PLAYER_VDR_7,
  PLAYER_VDR_8,
  PLAYER_VDR_9,
  PLAYER_VDR_USER_1,
  PLAYER_VDR_USER_2,
  PLAYER_VDR_USER_3,
  PLAYER_VDR_USER_4,
  PLAYER_VDR_USER_5,
  PLAYER_VDR_USER_6,
  PLAYER_VDR_USER_7,
  PLAYER_VDR_USER_8,
  PLAYER_VDR_USER_9,
  PLAYER_VDR_VOLPLUS,
  PLAYER_VDR_VOLMINUS,
  PLAYER_VDR_MUTE,
  PLAYER_VDR_AUDIO,
  PLAYER_VDR_INFO,
  PLAYER_VDR_CHANNELPREVIOUS,
  PLAYER_VDR_NEXT,
  PLAYER_VDR_PREVIOUS,
  PLAYER_VDR_SUBTITLES,
} player_vdr_t;

/**
 * \name VDR specific controls.
 * @{
 */

/**
 * \brief VDR commands.
 *
 * Wrappers supported (even partially):
 *  xine
 *
 * \warning MT-Safe in multithreaded applications (see \ref mtlevel).
 * \param[in] player      Player controller.
 * \param[in] value       Command to send.
 */
void player_vdr (player_t *player, player_vdr_t value);

/**
 * @}
 */

/***************************************************************************/
/*                                                                         */
/* Global libplayer functions                                              */
/*                                                                         */
/***************************************************************************/

/**
 * \name Global libplayer functions.
 * @{
 */

/**
 * \brief Test if a wrapper is enabled.
 *
 * \warning MT-Safe in multithreaded applications.
 * \param[in] type        Player type.
 * \return 1 if enabled, 0 otherwise.
 */
int libplayer_wrapper_enabled (player_type_t type);

/**
 * \brief Test if a resource is supported by a wrapper.
 *
 * \warning MT-Safe in multithreaded applications.
 * \param[in] type        Player type.
 * \param[in] res         Resource type.
 * \return 1 if supported, 0 otherwise.
 */
int libplayer_wrapper_supported_res (player_type_t type, mrl_resource_t res);

/*****************************************************************************/
/*                              Slave Commands                               */
/*****************************************************************************/

typedef enum slave_cmd {
  SLAVE_UNKNOWN = 0,
  SLAVE_DVDNAV,             /* dvdnav              int                       */
  SLAVE_GET_PROPERTY,       /* get_property        string                    */
  SLAVE_LOADFILE,           /* loadfile            string [int]              */
  SLAVE_OSD_SHOW_TEXT,      /* osd_show_text       string [int]   [int]      */
  SLAVE_PAUSE,              /* pause                                         */
  SLAVE_QUIT,               /* quit               [int]                      */
  SLAVE_RADIO_SET_CHANNEL,  /* radio_set_channel   string                    */
  SLAVE_RADIO_STEP_CHANNEL, /* radio_step_channel  int                       */
  SLAVE_SEEK,               /* seek                float  [int]              */
  SLAVE_SEEK_CHAPTER,       /* seek_chapter        int    [int]              */
  SLAVE_SET_MOUSE_POS,      /* set_mouse_pos       int     int               */
  SLAVE_SET_PROPERTY,       /* set_property        string  string            */
  SLAVE_STOP,               /* stop                                          */
  SLAVE_SUB_LOAD,           /* sub_load            string                    */
  SLAVE_SUB_POS,            /* sub_pos             int                       */
  SLAVE_SUB_SCALE,          /* sub_scale           int    [int]              */
  SLAVE_SWITCH_RATIO,       /* switch_ratio        float                     */
  SLAVE_SWITCH_TITLE,       /* switch_title       [int]                      */
  SLAVE_TV_SET_CHANNEL,     /* tv_set_channel      string                    */
  SLAVE_TV_SET_NORM,        /* tv_set_norm         string                    */
  SLAVE_TV_STEP_CHANNEL,    /* tv_step_channel     int                       */
  SLAVE_VOLUME,             /* volume              int    [int]              */
  SLAVE_REQUEST_STREAM_READ_COUNT
} slave_cmd_t;

/**
 * mplayer structure by a wrapper.
 */
typedef enum mplayer_dvdnav {
  MPLAYER_DVDNAV_UP     = 1,
  MPLAYER_DVDNAV_DOWN   = 2,
  MPLAYER_DVDNAV_LEFT   = 3,
  MPLAYER_DVDNAV_RIGHT  = 4,
  MPLAYER_DVDNAV_MENU   = 5,
  MPLAYER_DVDNAV_SELECT = 6,
  MPLAYER_DVDNAV_PREV   = 7,
  MPLAYER_DVDNAV_MOUSE  = 8,
} mplayer_dvdnav_t;

typedef enum mplayer_sub_alignment {
  MPLAYER_SUB_ALIGNMENT_TOP    = 0,
  MPLAYER_SUB_ALIGNMENT_CENTER = 1,
  MPLAYER_SUB_ALIGNMENT_BOTTOM = 2,
} mplayer_sub_alignment_t;

typedef enum mplayer_framedropping {
  MPLAYER_FRAMEDROPPING_DISABLE = 0,
  MPLAYER_FRAMEDROPPING_SOFT    = 1,
  MPLAYER_FRAMEDROPPING_HARD    = 2,
} mplayer_framedropping_t;

typedef enum mplayer_seek {
  MPLAYER_SEEK_RELATIVE = 0,
  MPLAYER_SEEK_PERCENT  = 1,
  MPLAYER_SEEK_ABSOLUTE = 2,
} mplayer_seek_t;

/* Status of MPlayer child */
typedef enum mplayer_status {
  MPLAYER_IS_IDLE,
  MPLAYER_IS_LOADING,
  MPLAYER_IS_PLAYING,
  MPLAYER_IS_PAUSE,
  MPLAYER_IS_STOP,
  MPLAYER_IS_DEAD
} mplayer_status_t;

typedef enum mplayer_eof {
  MPLAYER_EOF_NO,
  MPLAYER_EOF_END,
  MPLAYER_EOF_STOP,
} mplayer_eof_t;

typedef enum checklist {
  CHECKLIST_COMMANDS,
  CHECKLIST_PROPERTIES,
} checklist_t;

/* property and value for a search in the fifo_out */
typedef struct mp_search_s {
  char *property;
  char *value;
} mp_search_t;

typedef struct mp_identify_clip_s {
  int cnt;
  int property;
} mp_identify_clip_t;

/* union for set_property */
typedef union slave_value {
  int i_val;
  float f_val;
  const char *s_val;
} slave_value_t;

typedef enum item_state {
  ITEM_OFF  = 0,
  ITEM_ON   = (1 << 0),
  ITEM_HACK = (1 << 1),
} item_state_t;

#define ALL_ITEM_STATES (ITEM_HACK | ITEM_ON)

typedef enum opt_conf {
  OPT_OFF = 0,
  OPT_MIN,
  OPT_MAX,
  OPT_RANGE,
} opt_conf_t;

typedef struct item_opt_s {
  opt_conf_t conf;
  int min;
  int max;
  struct item_opt_s *next;  /* unused ATM */
} item_opt_t;

typedef struct item_list_s {
  const char *str;
  const int state_lib;    /* states of the item in libplayer */
  item_state_t state_mp;  /* state of the item in MPlayer    */
  item_opt_t *opt;        /* options of the item in MPlayer  */
} item_list_t;

/* player specific structure */
typedef struct mplayer_s {
  item_list_t *slave_cmds;    /* private list of commands   */
  item_list_t *slave_props;   /* private list of properties */
  pid_t        pid;           /* forked process PID         */

  /* manage the initialization of MPlayer */
  pthread_mutex_t mutex_start;
  pthread_cond_t  cond_start;
  int             start_ok;

  /* communications between the father and the son         */
  int   pipe_in[2];   /* pipe to send commands to MPlayer  */
  int   pipe_out[2];  /* pipe to receive results           */
  FILE *fifo_in;      /* fifo on the pipe_in  (write only) */
  FILE *fifo_out;     /* fifo on the pipe_out (read only)  */
  pthread_t th_fifo;

  sem_t sem;  /* common to 'loadfile' and 'get_property' */

  /* for the MPlayer properties, see slave_result() */
  pthread_mutex_t mutex_search;
  mp_search_t     search;

  pthread_mutex_t mutex_verbosity;
  int             verbosity;

  /* manage the status of MPlayer */
  pthread_cond_t   cond_status;
  pthread_mutex_t  mutex_status;
  mplayer_status_t status;

  /* indicate receive position response and store time position */
  bool recv_pos_resp;
  unsigned long time_pos;

  /* store stream read count of MPlayer */
  unsigned long stream_read_count;

  /* ask thread to terminate */
  int             terminate;
  pthread_mutex_t  mutex_terminate;
} mplayer_t;

/* handle massage status */
typedef enum handle_msg_state {
  HMSG_FINISH  = 0,
  HMSG_BUSY  = 1,
} handle_msg_state_t;

void player_reset_position_response(player_t *player);
bool player_get_position_response (player_t *player, unsigned long *position);
mplayer_status_t player_get_status(player_t *player);

/**
 * Only for mplayer api.
 */
player_t *mp_player_init(void);
void mp_player_uninit(player_t *player);
void mp_player_request_stream_read_count(player_t *player);
unsigned long long mp_player_get_stream_read_count(player_t *player);
void mp_player_reset_position_response(player_t *player);
bool mp_player_get_position_response(player_t *player, unsigned long *position);
mplayer_status_t mp_player_get_status(player_t *player);
void mp_player_set_resource(player_t *player, const char *resource, void *cb);
void mp_player_play(player_t *player, void *cb);
void mp_player_stop(player_t *player, void *cb);
void mp_player_pause(player_t *player, void *cb);
void mp_player_seek(player_t *player, int position_ms);
void mp_player_request_position(player_t *player);
void mp_player_get_position(player_t *player, int *time_pos, int *percent_pos);
int mp_player_get_volume(player_t *player);
void mp_player_set_volume(player_t *player, int volume);
bool mp_player_get_mute(player_t *player);
void mp_player_set_mute(player_t *player);
mplayer_t *mp_player_get_priv(player_t *player);
int mp_player_handle_massage(player_t *player);
const char *mp_element_state_get_name(mplayer_status_t state);
/**
 * @}
 */

#ifdef __cplusplus
#if 0 /* avoid EMACS indent */
{
#endif /* 0 */
}
#endif /* __cplusplus */

#endif /* PLAYER_H */