#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <player.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "upnp_connmgr.h"
#include "output_module.h"
#include "output_mplayer.h"
#ifdef QQ_COMPATIBLE
#include "upnp_qplay.h"
#endif
#define NS_TO_MS(ns) (ns / 1000000)
#define MS_TO_NS(ms) (ms * 1000000)

static player_t *player = NULL;
static output_transition_cb_t play_trans_callback_ = NULL;

static void output_mplayer_set_next_uri(const char *uri) {
	printf("----output_mplayer_set_next_uri----\n");
}

//Media Resource Locater (MRL)
static void output_mplayer_set_uri(const char *uri,
		output_update_meta_cb_t meta_cb) {
#ifdef QQ_COMPATIBLE
	if(uri != NULL)
		mp_player_set_resource(player, uri, meta_cb);
	else
		player_mrl_remove(player);
#else
	mrl_t *mrl;
	mrl_resource_local_args_t *args;

	Log_info("mplayer", "Set uri to '%s'", uri);

	args = calloc (1, sizeof (mrl_resource_local_args_t));
	args->location = (uri && *uri) ? strdup(uri) : NULL;
	mrl = mrl_new (player, MRL_RESOURCE_UNKNOWN, args);
	if (!mrl)
		return;
	player_mrl_append (player, mrl, PLAYER_MRL_ADD_QUEUE);
#endif
}

static int output_mplayer_play(output_transition_cb_t callback) {
#ifdef QQ_COMPATIBLE
	mp_player_play(player, (void *)callback);
#else
	play_trans_callback_ = callback;
	player_playback_start (player);
#endif
	return 0;
}

static int output_mplayer_stop(void) {
#ifdef QQ_COMPATIBLE
	mp_player_stop(player, NULL);
#else
	player_playback_stop (player);
#endif
	return 0;
}

static int output_mplayer_pause(void) {
#ifdef QQ_COMPATIBLE
	mp_player_pause(player, NULL);
#else
	player_playback_pause (player);
#endif
	return 0;
}

static int output_mplayer_seek(gint64 position_nanos) {
#ifdef QQ_COMPATIBLE
	mp_player_seek(player, position_nanos/1000000);
#else
	player_playback_seek (player, (int) NS_TO_MS(position_nanos), PLAYER_PB_SEEK_ABSOLUTE);
#endif
	return 0;
}

static int output_mplayer_add_options(GOptionContext *ctx)
{
	printf("----output_gstreamer_add_options----\n");

	return 0;
}
static int output_mplayer_get_position(gint64 *track_duration,
		gint64 *track_pos) {
#ifdef QQ_COMPATIBLE
	int time_pos, percent_pos, time_len;
	long long int pos, len;

	time_pos = player_get_time_pos (player);
	percent_pos = player_get_percent_pos (player);
	time_len = player_get_time_len(player);

	len = (long long int)time_len;
	*track_duration = MS_TO_NS(len);
#else
	int time_pos, percent_pos;
	long long int pos;

	printf("----output_mplayer_get_position----\n");
	time_pos = player_get_time_pos (player);
	percent_pos = player_get_percent_pos (player);

	*track_duration = 0;
#endif
	pos = (long long int) time_pos;
	*track_pos = MS_TO_NS(pos);

	return 0;
}
static int output_mplayer_get_volume(float *v) {

	int volume;
#ifdef QQ_COMPATIBLE
	volume = mp_player_get_volume(player);
	*v = volume;
#else
	volume = player_audio_volume_get (player);
	Log_info("gstreamer", "Query volume fraction: %d", volume);
	*v = volume;
#endif
	return 0;
}
static int output_mplayer_set_volume(float value) {
	int volume;

	printf("----output_mplayer_set_volume----\n");
	Log_info("gstreamer", "Set volume fraction to %f", value);
	volume = (int) (value * 100 + 0.5);
	player_audio_volume_set (player, volume);

	return 0;
}
static int output_mplayer_get_mute(int *m) {
	gboolean val;

	printf("----output_mplayer_get_mute----\n");
	val = (player_audio_mute_get (player) != PLAYER_MUTE_ON) ? 1 : 0;
	*m = val;

	return 0;
}
static int output_mplayer_set_mute(int m) {
	printf("----output_mplayer_set_mute %s----\n", m ? "on" : "off");
	Log_info("gstreamer", "Set mute to %s", m ? "on" : "off");
	if(player_audio_mute_get (player) != PLAYER_MUTE_ON) {
		player_audio_mute_set (player, PLAYER_MUTE_ON);
		printf ("MUTE\n");
	}
	else
	{
		player_audio_mute_set (player, PLAYER_MUTE_OFF);
		printf ("UNMUTE\n");
	}

	return 0;
}

#ifdef QQ_COMPATIBLE
extern void plist_update(void);
#endif
static int event_cb (player_event_t e, pl_unused void *data)
{
#ifdef QQ_COMPATIBLE
#define	TRANSPORT_STOPPED 0
#define	TRANSPORT_PLAYING 1
#define	TRANSPORT_TRANSITIONING 2
#define	TRANSPORT_PAUSED_PLAYBACK 3
#define	TRANSPORT_PAUSED_RECORDING 4
#define	TRANSPORT_RECORDING 5
#define	TRANSPORT_NO_MEDIA_PRESENT 6
	g_plist_lock();
	if (e == PLAYER_EVENT_PLAYBACK_START) //1
	{

		if(g_playlist.seek_song_idx)
		{
			g_playlist.curidx = g_playlist.seek_song_idx;
			g_playlist.seek_song_idx = 0;
		}
		//To determine whether playlist is loop
		else if(g_playlist.curidx + 1 <= g_playlist.totalcnt)
			g_playlist.curidx++ ;
		else
			g_playlist.curidx=1;
		g_playlist.start = TRANSPORT_STOPPED;
	}
	else if (e == PLAYER_EVENT_PLAYBACK_FINISHED) //3
	{
		/*Just update the current song status*/
		printf("Handle Event PLAYBACK FINISHED\n");
	}
	else if (e == PLAYER_EVENT_PLAYLIST_FINISHED) //4
	{
		/* If we set mode w/ PLAYER_LOOP_DISABLE
		* maybe need to update transport state
		* */
		printf("Handle Event PLAYLIST FINISHED\n");
		g_playlist.curidx = -1;
	}
	else
	{
		/* Currently unhandlee event
		* PLAYER_EVENT_PLAYBACK_STOP       //2
		* PLAYER_EVENT_PLAYBACK_PAUSE      //5
		* PLAYER_EVENT_PLAYBACK_UNPAUSE    //6
		* */
		printf("Unhandle Event:%d\n",e);
		g_plist_unlock();
		return 0;
	}
	g_plist_unlock();

	plist_update();
#else
	printf ("Received event (%i)\n", e);

	if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
		printf ("PLAYBACK FINISHED\n");
#endif
	return 0;
}

static int output_mplayer_init(void)
{
	player_init_param_t param;
	player_vo_t vo = PLAYER_VO_NULL;
	player_ao_t ao = PLAYER_AO_ALSA;
	player_type_t type = PLAYER_TYPE_MPLAYER;
	player_quality_level_t quality = PLAYER_QUALITY_NORMAL;
	player_verbosity_level_t verbosity = PLAYER_MSG_INFO;

	printf("----start output_mplayer_init----\n");

	memset (&param, 0, sizeof (player_init_param_t));
	param.ao       = ao;
	param.vo       = vo;
	param.event_cb = event_cb;
	param.quality  = quality;

	player = player_init (type, verbosity, &param);
	printf("----finish output_mplayer_init %p----\n", player);
	if(!player)
		return -1;
	return 0;
}

#ifdef QQ_COMPATIBLE
static void output_mplayer_set_pbmode(int mode, int loop)
{
	if(player)
	{
		player_set_playback(player,mode);
		player_set_loop (player, loop, -1);
	}
}

static void output_mplayer_switch_song(int idx)
{
	if(player)
	{
		player_mrl_select_song(player, idx);
	}
}
#endif

struct output_module mplayer_output = {
	.shortname = "mplayer",
	.description = "MPlayer multimedia player",
	.init        = output_mplayer_init,
	.add_options = output_mplayer_add_options,
	.set_uri     = output_mplayer_set_uri,
	.set_next_uri= output_mplayer_set_next_uri,
	.play        = output_mplayer_play,
	.stop        = output_mplayer_stop,
	.pause       = output_mplayer_pause,
	.seek        = output_mplayer_seek,
	.get_position = output_mplayer_get_position,
	.get_volume  = output_mplayer_get_volume,
	.set_volume  = output_mplayer_set_volume,
	.get_mute  = output_mplayer_get_mute,
	.set_mute  = output_mplayer_set_mute,
#ifdef QQ_COMPATIBLE
	.set_pbmode = output_mplayer_set_pbmode,
	.switch_song = output_mplayer_switch_song,
#endif
};


