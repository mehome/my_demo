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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>

#include "player.h"
#include "player_internals.h"
#include "logs.h"
#include "playlist.h" /* pl_playlist_new pl_playlist_free */
#include "supervisor.h"
#include "event_handler.h"

/* players wrappers */
#include "wrapper_dummy.h"
#ifdef HAVE_XINE
#include "wrapper_xine.h"
#endif /* HAVE_XINE */
#ifdef HAVE_MPLAYER
#include "wrapper_mplayer.h"
#endif /* HAVE_MPLAYER */
#ifdef HAVE_VLC
#include "wrapper_vlc.h"
#endif /* HAVE_VLC */
#ifdef HAVE_GSTREAMER
#include "wrapper_gstreamer.h"
#endif /* HAVE_GSTREAMER */

#define MODULE_NAME "player"

#define FIFO_BUFFER      256

static int
player_event_cb (void *data, int e)
{
  int res = 0;
  player_t *player = data;
  player_pb_t pb_mode = PLAYER_PB_SINGLE;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, "internal event: %i", e);

  if (!player)
  {
    pl_event_handler_sync_release (player->event);
    return -1;
  }

  /* send to the frontend event callback */
  if (player->event_cb)
  {
    pl_supervisor_callback_in (player, pthread_self ());
    res = player->event_cb (e, player->user_data);
    pl_supervisor_callback_out (player);
  }

  if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
  {
    player->state = PLAYER_STATE_IDLE;
    pb_mode = player->pb_mode;
  }

  /* release for supervisor */
  pl_event_handler_sync_release (player->event);

  /* go to the next MRL */
  if (pb_mode == PLAYER_PB_AUTO)
    pl_supervisor_send (player, SV_MODE_NO_WAIT,
                        SV_FUNC_PLAYER_MRL_NEXT_PLAY, NULL, NULL);

  return res;
}

unsigned int
libplayer_version (void)
{
  return LIBPLAYER_VERSION_INT;
}

/***************************************************************************/
/*                                                                         */
/* Player (Un)Initialization                                               */
/*                                                                         */
/***************************************************************************/

player_t *
player_init (player_type_t type,
             player_verbosity_level_t verbosity, player_init_param_t *param)
{
  player_t *player = NULL;
  init_status_t res = PLAYER_INIT_ERROR;
  supervisor_status_t sv_res;
  int ret;

  int *sv_run;
  pthread_t *sv_job;
  pthread_cond_t *sv_cond;
  pthread_mutex_t *sv_mutex;

  player = PCALLOC (player_t, 1);
  if (!player)
    return NULL;

  player->type        = type;
  player->verbosity   = verbosity;
  player->state       = PLAYER_STATE_IDLE;
  player->playlist    = pl_playlist_new (0, 0, PLAYER_LOOP_DISABLE);

  if (param)
  {
    player->ao          = param->ao;
    player->vo          = param->vo;
    player->winid       = param->winid;
    player->x11_display = param->display;
    player->event_cb    = param->event_cb;
    player->user_data   = param->data;
    player->quality     = param->quality;
  }

  pthread_mutex_init (&player->mutex_verb, NULL);

  switch (player->type)
  {
#ifdef HAVE_XINE
  case PLAYER_TYPE_XINE:
    player->funcs = pl_register_functions_xine ();
    player->priv  = pl_register_private_xine ();
    break;
#endif /* HAVE_XINE */
#ifdef HAVE_MPLAYER
  case PLAYER_TYPE_MPLAYER:
    player->funcs = pl_register_functions_mplayer ();
    player->priv  = pl_register_private_mplayer ();
    break;
#endif /* HAVE_MPLAYER */
#ifdef HAVE_VLC
  case PLAYER_TYPE_VLC:
    player->funcs = pl_register_functions_vlc ();
    player->priv  = pl_register_private_vlc ();
    break;
#endif /* HAVE_VLC */
#ifdef HAVE_GSTREAMER
  case PLAYER_TYPE_GSTREAMER:
    player->funcs = pl_register_functions_gstreamer ();
    player->priv  = pl_register_private_gstreamer ();
    break;
#endif /* HAVE_GSTREAMER */
  case PLAYER_TYPE_DUMMY:
    player->funcs = pl_register_functions_dummy ();
    player->priv  = pl_register_private_dummy ();
    break;
  default:
    break;
  }

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player->funcs || !player->priv)
  {
    pl_log (player, PLAYER_MSG_ERROR, MODULE_NAME, "no wrapper registered");
    player_uninit (player);
    return NULL;
  }

  player->supervisor = pl_supervisor_new ();
  if (!player->supervisor)
  {
    player_uninit (player);
    return NULL;
  }

  sv_res = pl_supervisor_init (player, &sv_run, &sv_job, &sv_cond, &sv_mutex);
  if (sv_res != SUPERVISOR_STATUS_OK)
  {
    pl_log (player, PLAYER_MSG_ERROR,
            MODULE_NAME, "failed to init supervisor");
    player_uninit (player);
    return NULL;
  }

  player->event = pl_event_handler_register (player, player_event_cb);
  if (!player->event)
  {
    player_uninit (player);
    return NULL;
  }

  pl_log (player, PLAYER_MSG_INFO, MODULE_NAME, "pl_event_handler_init");
  ret = pl_event_handler_init (player->event,
                               sv_run, sv_job, sv_cond, sv_mutex);
  if (ret)
  {
    pl_log (player, PLAYER_MSG_ERROR,
            MODULE_NAME, "failed to init event handler");
    player_uninit (player);
    return NULL;
  }

  pl_event_handler_enable (player->event);

  player_set_verbosity (player, verbosity);

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_INIT, NULL, &res);
  if (res != PLAYER_INIT_OK)
  {
    player_uninit (player);
    return NULL;
  }

  return player;
}

void
player_uninit (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_UNINIT, NULL, NULL);

  pl_log (player, PLAYER_MSG_INFO, MODULE_NAME, "pl_event_handler_uninit");
  pl_event_handler_disable (player->event);
  pl_event_handler_uninit (player->event);

  pl_supervisor_uninit (player);

  pl_playlist_free (player->playlist);
  pthread_mutex_destroy (&player->mutex_verb);
  PFREE (player->funcs);
  PFREE (player);
}

void
player_set_verbosity (player_t *player, player_verbosity_level_t level)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_VERBOSITY, &level, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Player to MRL connection                                                */
/*                                                                         */
/***************************************************************************/

mrl_t *
player_mrl_get_current (player_t *player)
{
  mrl_t *out = NULL;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return NULL;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_GET_CURRENT, NULL, &out);

  return out;
}

void
player_mrl_set (player_t *player, mrl_t *mrl)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player || !mrl)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_SET, mrl, NULL);
}

void
player_mrl_append (player_t *player, mrl_t *mrl, player_mrl_add_t when)
{
  supervisor_data_mrl_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player || !mrl)
    return;

  in.mrl   = mrl;
  in.value = when;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_APPEND, &in, NULL);
}

void
player_mrl_remove (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_REMOVE, NULL, NULL);
}

void
player_mrl_remove_all (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_REMOVE_ALL, NULL, NULL);
}

void
player_mrl_previous (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_PREVIOUS, NULL, NULL);
}

void
player_mrl_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_NEXT, NULL, NULL);
}

void
player_mrl_continue (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_NEXT_PLAY, NULL, NULL);
}

#ifdef QQ_COMPATIBLE
void
player_mrl_select_song (player_t *player, int idx)
{

  supervisor_data_mrl_t in;
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = idx;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_MRL_SELECT, &in, NULL);
}
#endif
/***************************************************************************/
/*                                                                         */
/* Player tuning & properties                                              */
/*                                                                         */
/***************************************************************************/

int
player_get_time_pos (player_t *player)
{
  int out = -1;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return -1;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_GET_TIME_POS, NULL, &out);

  return out;
}

#ifdef QQ_COMPATIBLE
int
player_get_time_len (player_t *player)
{
  int out = -1;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return -1;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_GET_TIME_LEN, NULL, &out);

  return out;
}
#endif

int
player_get_percent_pos (player_t *player)
{
  int out = -1;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return -1;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_GET_PERCENT_POS, NULL, &out);

  return out;
}

void
player_set_playback (player_t *player, player_pb_t pb)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_PLAYBACK, &pb, NULL);
}

void
player_set_loop (player_t *player, player_loop_t loop, int value)
{
  supervisor_data_mode_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = value;
  in.mode  = loop;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_LOOP, &in, NULL);
}

void
player_set_shuffle (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_SHUFFLE, &value, NULL);
}

void
player_set_framedrop (player_t *player, player_framedrop_t fd)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_FRAMEDROP, &fd, NULL);
}

void
player_set_mouse_position (player_t *player, int x, int y)
{
  supervisor_data_coord_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.x = x;
  in.y = y;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SET_MOUSE_POS, &in, NULL);
}

void
player_x_window_set_properties (player_t *player,
                                int x, int y, int w, int h, int flags)
{
  supervisor_data_window_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.x     = x;
  in.y     = y;
  in.h     = h;
  in.w     = w;
  in.flags = flags;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_X_WINDOW_SET_PROPS, &in, NULL);
}

void
player_osd_show_text (player_t *player,
                      const char *text, int x, int y, int duration)
{
  supervisor_data_osd_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.text     = text;
  in.x        = x;
  in.y        = y;
  in.duration = duration;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_OSD_SHOW_TEXT, &in, NULL);
}

void
player_osd_state (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_OSD_STATE, &value, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Playback related controls                                               */
/*                                                                         */
/***************************************************************************/

player_pb_state_t
player_playback_get_state (player_t *player)
{
  player_pb_state_t out = PLAYER_PB_STATE_IDLE;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return out;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_GET_STATE, NULL, &out);

  return out;
}

void
player_playback_start (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_START, NULL, NULL);
}

void
player_playback_stop (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_STOP, NULL, NULL);
}

void
player_playback_pause (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_PAUSE, NULL, NULL);
}

void
player_playback_seek (player_t *player, int value, player_pb_seek_t seek)
{
  supervisor_data_mode_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = value;
  in.mode  = seek;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_SEEK, &in, NULL);
}

void
player_playback_seek_chapter (player_t *player, int value, int absolute)
{
  supervisor_data_mode_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = value;
  in.mode  = absolute;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_SEEK_CHAPTER, &in, NULL);
}

void
player_playback_speed (player_t *player, float value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_PB_SPEED, &value, NULL);
}

void
player_request_stream_read_count (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_REQUEST_STREAM_READ_COUNT, NULL, NULL);
}

unsigned long long player_get_stream_read_count (player_t *player)
{
  mplayer_t *mplayer;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return 0;

  mplayer = player->priv;

  return mplayer->stream_read_count;
}

void player_reset_position_response(player_t *player)
{
  mplayer_t *mplayer;

  //pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return 0;

  mplayer = player->priv;

  mplayer->recv_pos_resp = false;
}

bool player_get_position_response (player_t *player, unsigned long *position)
{
  mplayer_t *mplayer;

  //pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return 0;

  mplayer = player->priv;

  *position = mplayer->time_pos;

  return mplayer->recv_pos_resp;
}

mplayer_status_t player_get_status (player_t *player)
{
  mplayer_t *mplayer;

  //pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return 0;

  mplayer = player->priv;

  return mplayer->status;
}
/***************************************************************************/
/*                                                                         */
/* Audio related controls                                                  */
/*                                                                         */
/***************************************************************************/

int
player_audio_volume_get (player_t *player)
{
  int out = -1;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return -1;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_VOLUME_GET, NULL, &out);

  return out;
}

void
player_audio_volume_set (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_VOLUME_SET, &value, NULL);
}

player_mute_t
player_audio_mute_get (player_t *player)
{
  player_mute_t out = PLAYER_MUTE_UNKNOWN;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return out;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_MUTE_GET, NULL, &out);

  return out;
}

void
player_audio_mute_set (player_t *player, player_mute_t value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_MUTE_SET, &value, NULL);
}

void
player_audio_set_delay (player_t *player, int value, int absolute)
{
  supervisor_data_mode_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = value;
  in.mode  = absolute;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_SET_DELAY, &in, NULL);
}

void
player_audio_select (player_t *player, int audio_id)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_SELECT, &audio_id, NULL);
}

void
player_audio_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_PREV, NULL, NULL);
}

void
player_audio_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_AO_NEXT, NULL, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Video related controls                                                  */
/*                                                                         */
/***************************************************************************/

void
player_video_set_aspect (player_t *player, player_video_aspect_t aspect,
                         int8_t value, int absolute)
{
  supervisor_data_vo_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.list  = aspect;
  in.value = value;
  in.mode  = absolute;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_VO_SET_ASPECT, &in, NULL);
}

void
player_video_set_panscan (player_t *player, int8_t value, int absolute)
{
  supervisor_data_vo_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.list  = 0;
  in.value = value;
  in.mode  = absolute;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_VO_SET_PANSCAN, &in, NULL);
}

void
player_video_set_aspect_ratio (player_t *player, float value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_VO_SET_AR, &value, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Subtitles related controls                                              */
/*                                                                         */
/***************************************************************************/

void
player_subtitle_set_delay (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SET_DELAY, &value, NULL);
}

void
player_subtitle_set_alignment (player_t *player,
                               player_sub_alignment_t a)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SET_ALIGN, &a, NULL);
}

void
player_subtitle_set_position (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SET_POS, &value, NULL);
}

void
player_subtitle_set_visibility (player_t *player, int value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SET_VIS, &value, NULL);
}

void
player_subtitle_scale (player_t *player, int value, int absolute)
{
  supervisor_data_mode_t in;

  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  in.value = value;
  in.mode  = absolute;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SCALE, &in, NULL);
}

void
player_subtitle_select (player_t *player, int sub_id)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_SELECT, &sub_id, NULL);
}

void
player_subtitle_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_PREV, NULL, NULL);
}

void
player_subtitle_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_SUB_NEXT, NULL, NULL);
}

/***************************************************************************/
/*                                                                         */
/* DVD specific controls                                                   */
/*                                                                         */
/***************************************************************************/

void
player_dvd_nav (player_t *player, player_dvdnav_t value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_NAV, &value, NULL);
}

void
player_dvd_angle_select (player_t *player, int angle)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_ANGLE_SELECT, &angle, NULL);
}

void
player_dvd_angle_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_ANGLE_PREV, NULL, NULL);
}

void
player_dvd_angle_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_ANGLE_NEXT, NULL, NULL);
}

void
player_dvd_title_select (player_t *player, int title)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_ANGLE_SELECT, &title, NULL);
}

void
player_dvd_title_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_TITLE_PREV, NULL, NULL);
}

void
player_dvd_title_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_DVD_TITLE_NEXT, NULL, NULL);
}

/***************************************************************************/
/*                                                                         */
/* TV/DVB specific controls                                                */
/*                                                                         */
/***************************************************************************/

void
player_tv_channel_select (player_t *player, const char *channel)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_TV_CHAN_SELECT, (void *) channel, NULL);
}

void
player_tv_channel_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_TV_CHAN_PREV, NULL, NULL);
}

void
player_tv_channel_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_TV_CHAN_NEXT, NULL, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Radio specific controls                                                 */
/*                                                                         */
/***************************************************************************/

void
player_radio_channel_select (player_t *player, const char *channel)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_RADIO_CHAN_SELECT, (void *) channel, NULL);
}

void
player_radio_channel_prev (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_RADIO_CHAN_PREV, NULL, NULL);
}

void
player_radio_channel_next (player_t *player)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_RADIO_CHAN_NEXT, NULL, NULL);
}

/***************************************************************************/
/*                                                                         */
/* VDR specific controls                                                   */
/*                                                                         */
/***************************************************************************/

void
player_vdr (player_t *player, player_vdr_t value)
{
  pl_log (player, PLAYER_MSG_VERBOSE, MODULE_NAME, __FUNCTION__);

  if (!player)
    return;

  pl_supervisor_send (player, SV_MODE_WAIT_FOR_END,
                      SV_FUNC_PLAYER_VDR, &value, NULL);
}

/***************************************************************************/
/*                                                                         */
/* Global libplayer functions                                              */
/*                                                                         */
/***************************************************************************/

int
libplayer_wrapper_enabled (player_type_t type)
{
  switch (type)
  {
#ifdef HAVE_XINE
  case PLAYER_TYPE_XINE:
#endif /* HAVE_XINE */
#ifdef HAVE_MPLAYER
  case PLAYER_TYPE_MPLAYER:
#endif /* HAVE_MPLAYER */
#ifdef HAVE_VLC
  case PLAYER_TYPE_VLC:
#endif /* HAVE_VLC */
#ifdef HAVE_GSTREAMER
  case PLAYER_TYPE_GSTREAMER:
#endif /* HAVE_GSTREAMER */
  case PLAYER_TYPE_DUMMY:
    return 1;

  default:
    return 0;
  }
}

int
libplayer_wrapper_supported_res (player_type_t type, mrl_resource_t res)
{
  switch (type)
  {
#ifdef HAVE_XINE
  case PLAYER_TYPE_XINE:
    return pl_supported_resources_xine (res);
#endif /* HAVE_XINE */

#ifdef HAVE_MPLAYER
  case PLAYER_TYPE_MPLAYER:
    return pl_supported_resources_mplayer (res);
#endif /* HAVE_MPLAYER */

#ifdef HAVE_VLC
  case PLAYER_TYPE_VLC:
    return pl_supported_resources_vlc (res);
#endif /* HAVE_VLC */

#ifdef HAVE_GSTREAMER
  case PLAYER_TYPE_GSTREAMER:
    return pl_supported_resources_gstreamer (res);
#endif /* HAVE_GSTREAMER */

  case PLAYER_TYPE_DUMMY:
    return pl_supported_resources_dummy (res);

  default:
    return 0;
  }
}

#define MPP_DBG printf
int event_cb(player_event_t e, pl_unused void *data)
{
    MPP_DBG ("Received event (%i)\n", e);

    if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
        MPP_DBG ("PLAYBACK FINISHED\n");

    return 0;
}
player_t *mp_player_init(void)
{
    player_t *player;
    player_init_param_t param;
    player_vo_t vo = PLAYER_VO_NULL;
    player_ao_t ao = PLAYER_AO_ALSA;
    player_type_t type = PLAYER_TYPE_MPLAYER;
    player_quality_level_t quality = PLAYER_QUALITY_NORMAL;
    player_verbosity_level_t verbosity = PLAYER_MSG_INFO;

    memset (&param, 0, sizeof (player_init_param_t));
    param.ao       = ao;
    param.vo       = vo;
    param.event_cb = event_cb;
    param.quality  = quality;

    player = player_init (type, verbosity, &param);
    if(!player) {
        MPP_DBG("mp_player_init fail\n");
        return NULL;
    }
    MPP_DBG("mp_player_init success\n");
    return player;
}
void mp_player_uninit(player_t *player)
{
    MPP_DBG("mp_player_uninit\n");
    player_uninit (player);
}
void mp_player_request_stream_read_count(player_t *player)
{
    //MPP_DBG("mp_player_request_stream_read_count\n");
    player_request_stream_read_count(player);
}
unsigned long long mp_player_get_stream_read_count(player_t *player)
{
    //MPP_DBG("mp_player_get_stream_read_count\n");
    return player_get_stream_read_count(player);
}
void mp_player_reset_position_response(player_t *player)
{
  player_reset_position_response(player);
}
bool mp_player_get_position_response(player_t *player, unsigned long *position)
{
  return player_get_position_response(player, position);
}
mplayer_status_t mp_player_get_status(player_t *player)
{
    return player_get_status(player);
}
void mp_player_set_resource(player_t *player, const char *resource, void *cb)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;

    MPP_DBG("mp_player_set_resource (%s)\n", resource);
    args = calloc (1, sizeof (mrl_resource_local_args_t));
    args->location = (resource && *resource) ? strdup(resource) : NULL;
    mrl = mrl_new (player, MRL_RESOURCE_UNKNOWN, args);
    if (!mrl)
        return;
#ifdef QQ_COMPATIBLE
    player_mrl_append (player, mrl, PLAYER_MRL_ADD_QUEUE);
#else
    player_mrl_append (player, mrl, PLAYER_MRL_INSERT_QUEUE);
#endif
}
void mp_player_play(player_t *player, void *cb)
{
    MPP_DBG("mp_player_play\n");
    if(PLAYER_PB_STATE_PAUSE == player_playback_get_state(player)) {
        player_playback_pause (player);
    }
    else
    {
        player_playback_start (player);
    }
}
void mp_player_stop(player_t *player, void *cb)
{
    MPP_DBG("mp_player_stop\n");
    player_playback_stop (player);
}
void mp_player_pause(player_t *player, void *cb)
{
    MPP_DBG("mp_player_pause\n");
    player_playback_pause (player);
}
void mp_player_seek(player_t *player, int position_ms)
{
    MPP_DBG("mp_player_seek (%d)ms\n", position_ms);
    player_playback_seek (player, position_ms, PLAYER_PB_SEEK_ABSOLUTE);
}
void mp_player_request_position(player_t *player)
{
    MPP_DBG("mp_player_request_position\n");
    player_get_time_pos (player);
}
void mp_player_get_position(player_t *player, int *time_pos, int *percent_pos)
{
    MPP_DBG("mp_player_get_position\n");
    *time_pos = player_get_time_pos (player);
    *percent_pos = player_get_percent_pos (player);
}
int mp_player_get_volume(player_t *player)
{
    MPP_DBG("mp_player_get_volume\n");
    return player_audio_volume_get (player);
}
void mp_player_set_volume(player_t *player, int volume)
{
    MPP_DBG("mp_player_set_volume (%d)\n", volume);
    player_audio_volume_set (player, volume);
}
bool mp_player_get_mute(player_t *player)
{
    return (player_audio_mute_get (player) != PLAYER_MUTE_ON) ? 1 : 0;
}
void mp_player_set_mute(player_t *player)
{
    if(player_audio_mute_get (player) != PLAYER_MUTE_ON) {
        player_audio_mute_set (player, PLAYER_MUTE_ON);
        MPP_DBG ("MUTE\n");
    }
    else
    {
        player_audio_mute_set (player, PLAYER_MUTE_OFF);
        MPP_DBG ("UNMUTE\n");
    }
}
mplayer_t *mp_player_get_priv(player_t *player)
{
  return (mplayer_t *) player->priv;
}

#define ILT(s, f) { s, f, ITEM_OFF, NULL }
#define ARRAY_NB_ELEMENTS(array) (sizeof (array) / sizeof (array[0]))

static const item_list_t g_slave_cmds[] = {
  [SLAVE_DVDNAV]             = ILT ("dvdnav",             ITEM_ON            ),
  [SLAVE_GET_PROPERTY]       = ILT ("get_property",       ITEM_ON            ),
  [SLAVE_LOADFILE]           = ILT ("loadfile",           ITEM_ON            ),
  [SLAVE_OSD_SHOW_TEXT]      = ILT ("osd_show_text",      ITEM_ON            ),
  [SLAVE_PAUSE]              = ILT ("pause",              ITEM_ON            ),
  [SLAVE_QUIT]               = ILT ("quit",               ITEM_ON            ),
  [SLAVE_RADIO_SET_CHANNEL]  = ILT ("radio_set_channel",  ITEM_ON            ),
  [SLAVE_RADIO_STEP_CHANNEL] = ILT ("radio_step_channel", ITEM_ON            ),
  [SLAVE_SEEK]               = ILT ("seek",               ITEM_ON            ),
  [SLAVE_SEEK_CHAPTER]       = ILT ("seek_chapter",       ITEM_ON            ),
  [SLAVE_SET_MOUSE_POS]      = ILT ("set_mouse_pos",      ITEM_ON            ),
  [SLAVE_SET_PROPERTY]       = ILT ("set_property",       ITEM_ON            ),
  [SLAVE_STOP]               = ILT ("stop",               ITEM_ON | ITEM_HACK),
  [SLAVE_SUB_LOAD]           = ILT ("sub_load",           ITEM_ON            ),
  [SLAVE_SUB_POS]            = ILT ("sub_pos",            ITEM_ON            ),
  [SLAVE_SUB_SCALE]          = ILT ("sub_scale",          ITEM_ON            ),
  [SLAVE_SWITCH_RATIO]       = ILT ("switch_ratio",       ITEM_ON            ),
  [SLAVE_SWITCH_TITLE]       = ILT ("switch_title",       ITEM_ON            ),
  [SLAVE_TV_SET_CHANNEL]     = ILT ("tv_set_channel",     ITEM_ON            ),
  [SLAVE_TV_SET_NORM]        = ILT ("tv_set_norm",        ITEM_ON            ),
  [SLAVE_TV_STEP_CHANNEL]    = ILT ("tv_step_channel",    ITEM_ON            ),
  [SLAVE_VOLUME]             = ILT ("volume",             ITEM_ON            ),
  [SLAVE_REQUEST_STREAM_READ_COUNT] = ILT ("stream_read_count", ITEM_ON      ),
  [SLAVE_UNKNOWN]            = ILT (NULL,                 ITEM_OFF           )
};

static const unsigned int g_slave_cmds_nb = ARRAY_NB_ELEMENTS (g_slave_cmds);

static item_state_t
get_state (int lib, item_state_t mp)
{
  int state_lib = lib & ALL_ITEM_STATES;

  if ((state_lib == ITEM_HACK) ||
      (state_lib == ALL_ITEM_STATES && mp == ITEM_OFF))
  {
    return ITEM_HACK;
  }
  else if ((state_lib == ITEM_ON && mp == ITEM_ON) ||
           (state_lib == ALL_ITEM_STATES && mp == ITEM_ON))
  {
    return ITEM_ON;
  }

  return ITEM_OFF;
}

static const char *
get_cmd (player_t *player, slave_cmd_t cmd, item_state_t *state)
{
  mplayer_t *mplayer;
  slave_cmd_t command = SLAVE_UNKNOWN;

  if (!player)
    return NULL;

  mplayer = player->priv;
  if (!mplayer || !mplayer->slave_cmds)
    return NULL;

  if (cmd < g_slave_cmds_nb)
    command = cmd;

  if (state)
    *state = get_state (mplayer->slave_cmds[command].state_lib,
                        mplayer->slave_cmds[command].state_mp);

  return mplayer->slave_cmds[command].str;
}

static mplayer_status_t get_mplayer_status (player_t *player)
{
  mplayer_t *mplayer = player->priv;
  mplayer_status_t status;

  if (!mplayer)
    return MPLAYER_IS_DEAD;

  pthread_mutex_lock (&mplayer->mutex_status);
  status = mplayer->status;
  pthread_mutex_unlock (&mplayer->mutex_status);
  return status;
}

static char *
parse_field (char *line)
{
  char *its;

  /* value start */
  its = strchr (line, '=');
  if (!its)
    return line;
  its++;

  return pl_trim_whitespaces (its);
}

int mp_player_handle_massage (player_t *player)
{
  player_verbosity_level_t level = PLAYER_MSG_VERBOSE;
  unsigned int skip_msg = 0;
  int start_ok = 1, check_init = 1, verbosity = 0;
  mplayer_eof_t wait_uninit = MPLAYER_EOF_NO;
  char buffer[FIFO_BUFFER];
  char *it;
  mplayer_t *mplayer;

  if (!player)
    return HMSG_FINISH;

  mplayer = player->priv;

  if (!mplayer || !mplayer->fifo_out)
    return HMSG_FINISH;

again:
  /* MPlayer's stdout parser */
  if (fgets (buffer, FIFO_BUFFER, mplayer->fifo_out) != NULL)
  {
#if 0
    int i;
    for(i=0; i<FIFO_BUFFER; i++)
    {
      if(i%128==0)
      {
        printf("\n");
      }
      printf("%c", buffer[i]);
    }
    printf("\n");
#endif
    //pthread_mutex_lock (&mplayer->mutex_verbosity);
    verbosity = mplayer->verbosity;
    //pthread_mutex_unlock (&mplayer->mutex_verbosity);

    /*
     * NOTE: In order to detect EOF code, that is necessary to set the
     *       msglevel of 'global' to 6. And in this case, the verbosity of
     *       vd_ffmpeg is increased with _a lot of_ useless messages related
     *       to libpostproc for example.
     *
     * This test will search and skip all strings matching this pattern:
     * \[.*@.*\].*
     */
    if (buffer[0] == '['
        && (it = strchr (buffer, '@')) > buffer
        && strchr (buffer, ']') > it)
    {
      if (verbosity)
        skip_msg++;
      goto again;
    }

    if (verbosity)
    {
      *(buffer + strlen (buffer) - 1) = '\0';

      if (level == PLAYER_MSG_VERBOSE
          && strstr (buffer, "MPlayer interrupted by signal") == buffer)
      {
        level = PLAYER_MSG_CRITICAL;
      }

      if (skip_msg)
      {
        skip_msg = 0;
      }
    }

    /*
     * Here, the result of a property requested by the slave command
     * 'get_property', is searched and saved.
     */
    //pthread_mutex_lock (&mplayer->mutex_search);
    if (mplayer->search.property &&
        (it = strstr (buffer, mplayer->search.property)) == buffer)
    {
      it = parse_field (it);
      mplayer->search.value = strdup (it);
    }

    /*
     * HACK: This part will be used only to detect the end of the slave
     *       command 'get_property'. In this case, libplayer is locked on
     *       mplayer->sem as long as the property will be found or not.
     *
     * NOTE: This hack is no longer necessary since MPlayer r26296.
     */
    else if (strstr (buffer, "Command loadfile") == buffer)
    {
      if (mplayer->search.property)
      {
        PFREE (mplayer->search.property);
        mplayer->search.property = NULL;
        //sem_post (&mplayer->sem);
      }
    }
    //pthread_mutex_unlock (&mplayer->mutex_search);

    /*
     * Search for "End Of File" in order to handle slave command 'stop'
     * or "end of stream".
     */
    if (strstr (buffer, "EOF code:") == buffer)
    {
      /*
       * When code is '4', then slave command 'stop' was used. But if the
       * command is not ENABLE, then this EOF code will be considered as an
       * "end of stream".
       */
      if (strchr (buffer, '4'))
      {
        item_state_t state = ITEM_OFF;
        get_cmd (player, SLAVE_STOP, &state);

        if (state == ITEM_ON)
        {
          if (get_mplayer_status (player) == MPLAYER_IS_IDLE)
            wait_uninit = MPLAYER_EOF_STOP;
          goto again;
        }
      }

      wait_uninit = MPLAYER_EOF_END;

      /*
       * Here we can consider an "end of stream" and sending an event.
       */
      //pthread_mutex_lock (&mplayer->mutex_status);
      if (mplayer->status == MPLAYER_IS_PLAYING)
      {
        mplayer->status = MPLAYER_IS_IDLE;
        //pthread_mutex_unlock (&mplayer->mutex_status);

#if 0 //TODO
        player_event_send (player, PLAYER_EVENT_PLAYBACK_FINISHED);
#endif
      }
      else
      {
        //pthread_mutex_unlock (&mplayer->mutex_status);

        item_state_t state = ITEM_OFF;
        get_cmd (player, SLAVE_STOP, &state);

        /*
         * Oops, 'stop' is arrived just after the parsing of "EOF code" and
         * it was not handled like a stop but like an end of file.
         */
        if (state == ITEM_ON)
        {
          wait_uninit = MPLAYER_EOF_STOP;
        }
      }
    }

    /*
     * HACK: If the slave command 'stop' is not handled by MPlayer, then this
     *       part will find the end instead of "EOF code: 4".
     */
    else if (strstr (buffer, "File not found: ''") == buffer)
    {
      item_state_t state = ITEM_OFF;
      get_cmd (player, SLAVE_STOP, &state);

      if (state != ITEM_HACK)
        goto again;

      if (get_mplayer_status (player) == MPLAYER_IS_IDLE)
        wait_uninit = MPLAYER_EOF_STOP;
    }

    /*
     * Detect when MPlayer playback is really started in order to change
     * the current status.
     */
    else if (strstr (buffer, "Starting playback") == buffer)
    {
      //pthread_mutex_lock (&mplayer->mutex_status);
      if (mplayer->status == MPLAYER_IS_LOADING)
      {
        mplayer->status = MPLAYER_IS_PLAYING;
        //pthread_cond_signal (&mplayer->cond_status);
      }
      else
        mplayer->status = MPLAYER_IS_PLAYING;
      //pthread_mutex_unlock (&mplayer->mutex_status);
    }

    /*
     * This test is useful when MPlayer can't execute a slave command because
     * the buffer is full. It happens for example with 'loadfile', when the
     * location of the file is very long (256 or 4096 with MPlayer >= r29403).
     */
    else if (strstr (buffer,
                     "Command buffer of file descriptor 0 is full") == buffer)
    {
      //pthread_mutex_lock (&mplayer->mutex_status);
      if (mplayer->status == MPLAYER_IS_LOADING)
      {
        //pthread_cond_signal (&mplayer->cond_status);
        mplayer->status = MPLAYER_IS_IDLE;
      }
      //pthread_mutex_unlock (&mplayer->mutex_status);
    }

    /*
     * If this 'uninit' is detected, we can be sure that nothing is playing.
     * The status of MPlayer will be changed and a signal will be sent if
     * MPlayer was loading a stream.
     * But if EOF is detected before 'uninit', this is considered as a
     * 'stop' or an 'end of stream'.
     */
    else if (strstr (buffer, "*** uninit") == buffer)
    {
      switch (wait_uninit)
      {
      case MPLAYER_EOF_STOP:
        wait_uninit = MPLAYER_EOF_NO;
        //sem_post (&mplayer->sem);
        goto again;

      case MPLAYER_EOF_END:
        wait_uninit = MPLAYER_EOF_NO;
        goto again;

      default:
        //pthread_mutex_lock (&mplayer->mutex_status);
        //if (mplayer->status == MPLAYER_IS_LOADING)
          //pthread_cond_signal (&mplayer->cond_status);
        mplayer->status = MPLAYER_IS_IDLE;
        //pthread_mutex_unlock (&mplayer->mutex_status);
      }
    }
    else if (strstr (buffer, "Pause finish") == buffer)
    {
      mplayer->status = MPLAYER_IS_PAUSE;
    }
    else if (strstr (buffer, "Stop finish") == buffer)
    {
      mplayer->status = MPLAYER_IS_STOP;
    }
    else if (strstr (buffer, "Stream read count") == buffer)
    {
      char *str;
      str = &buffer[18]; /* size of "Stream read count " */
      mplayer->stream_read_count = strtoul(str, NULL, 0);
    }
    else if (strstr (buffer, "ANS_time_pos=") == buffer)
    {
      char *str;
      float time_pos = 0.0;
      str = &buffer[13]; /* size of "ANS_time_pos=" */
      time_pos = atof(str);
      time_pos *= 1000;
      mplayer->recv_pos_resp = true;
      mplayer->time_pos = (unsigned long) time_pos;
    }
    /*
     * Check language used by MPlayer. Only english is supported. A signal
     * is sent to the init as fast as possible. If --language is not found,
     * then MPlayer is in english. But if --language is found, then the
     * first language must be 'en' or 'all'.
     * --language-msg is provided by MPlayer >= r29363.
     *
     * FIXME: to find a way to detect when MPlayer is compiled for an other
     *        language with the LINGUAS environment variable.
     */
#if 0 //TODO
    else if (check_init)
    {
      const char *it;
      if (   (it = pl_strrstr (buffer, "--language-msg="))
          || (it = pl_strrstr (buffer, "--language=")))
      {
        it = strchr (it, '=') + 1;
        if (strncmp (it, "en", 2) &&
            strncmp (it, "all", 3))
        {
          start_ok = 0;
          pl_log (player, PLAYER_MSG_ERROR,
                  MODULE_NAME, "only english version of MPlayer is supported");
        }
      }
      else if (strstr (buffer, "-slave") && strstr (buffer, "-idle"))
      {
        pthread_mutex_lock (&mplayer->mutex_start);
        mplayer->start_ok = start_ok;
        pthread_cond_signal (&mplayer->cond_start);
        pthread_mutex_unlock (&mplayer->mutex_start);
        check_init = 0;
      }
    }
#endif
  }
  else
  {
    return HMSG_FINISH;
  }

  return HMSG_BUSY;
}

const char *mp_element_state_get_name (mplayer_status_t state)
{
  switch (state) {
    case MPLAYER_IS_IDLE:
      return "IDLE";
    case MPLAYER_IS_LOADING:
      return "LOADING";
    case MPLAYER_IS_PLAYING:
      return "PLAYING";
    case MPLAYER_IS_DEAD:
      return "DEAD";
    case MPLAYER_IS_PAUSE:
      return "PAUSE";
    default:
      return "NULL";
  }
}
