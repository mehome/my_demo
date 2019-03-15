/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "PlayerCommands.hxx"
#include "CommandError.hxx"
#include "queue/Playlist.hxx"
#include "PlaylistPrint.hxx"
#include "client/Client.hxx"
#include "mixer/Volume.hxx"
#include "Partition.hxx"
#include "Instance.hxx"
#include "protocol/Result.hxx"
#include "protocol/ArgParser.hxx"
#include "AudioFormat.hxx"
#include "ReplayGainConfig.hxx"

#ifdef ENABLE_DATABASE
#include "db/update/Service.hxx"
#endif

#ifdef MONTAGE_CTRL_PLAYER
//#include <mon.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
extern "C"
{
	#include <cdb.h>
}



#define COMMAND_STATUS_STATE            "state"
#define COMMAND_STATUS_REPEAT           "repeat"
#define COMMAND_STATUS_SINGLE           "single"
#define COMMAND_STATUS_CONSUME          "consume"
#define COMMAND_STATUS_RANDOM           "random"
#define COMMAND_STATUS_PLAYLIST         "playlist"
#define COMMAND_STATUS_PLAYLIST_LENGTH  "playlistlength"
#define COMMAND_STATUS_SONG             "song"
#define COMMAND_STATUS_SONGID           "songid"
#define COMMAND_STATUS_NEXTSONG         "nextsong"
#define COMMAND_STATUS_NEXTSONGID       "nextsongid"
#define COMMAND_STATUS_TIME             "time"
#define COMMAND_STATUS_BITRATE          "bitrate"
#define COMMAND_STATUS_ERROR            "error"
#define COMMAND_STATUS_CROSSFADE	"xfade"
#define COMMAND_STATUS_MIXRAMPDB	"mixrampdb"
#define COMMAND_STATUS_MIXRAMPDELAY	"mixrampdelay"
#define COMMAND_STATUS_AUDIO		"audio"
#define COMMAND_STATUS_UPDATING_DB	"updating_db"

#ifdef MONTAGE_ENABLE_DLNA_PAUSE
#define COMMAND_STATUS_DLNA_PAUSE   "dlna_pause"
#endif

#ifdef MONTAGE_ENABLE_LINKIP
#define COMMAND_STATUS_LINKIP   "linkip"
#endif

#ifdef TURING_MPD_CHANGE
#include "PlayerControl.hxx"
#endif

#ifdef MONTAGE_CTRL_PLAYER

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

static int monfd = -1;
int creat_socket_server(char *dest)
{
		int monfd = -1;
		struct sockaddr_un c_addr;
		
		if ((monfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
			perror("fail to create socket");
			goto fail;
		}	
		memset(&c_addr, 0, sizeof(struct sockaddr_un));
		c_addr.sun_family = AF_UNIX;
		strcpy(c_addr.sun_path, MONC_SOCK);
		unlink(MONC_SOCK);

		if (bind(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
			perror("fail to bind");
			goto fail;
		}

		memset(&c_addr, 0, sizeof(struct sockaddr_un));
		c_addr.sun_family = AF_UNIX;
		strcpy(c_addr.sun_path, MONS_SOCK);

		if (connect(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
			perror("fail to connect");
			goto fail;
		}
		return monfd;
		
fail:
		if (monfd >= 0)
			close(monfd);
		return -1;
}

unsigned int inform_ctrl_msg(char cmd, char *data)
{	
	int monfd = -1, len;	
	struct sockaddr_un c_addr;
	struct mon_param mon;	
	char buf[MAX_BUF_LEN];	
	memset(&mon, 0, sizeof(struct mon_param));
	//cprintf("%s:%d,  cmd :%d", __func__,__LINE__, cmd);
	switch (cmd)	{		
		case MONDLNA:		
		case MONAIRPLAY:		
		case MONSPOTIFY:		
			mon.type = cmd;	
		len = sizeof(struct mon_param);			
		break;		
		default:		
			perror("Not support now");			
			return 1;
	}	
	if ((monfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {	
		perror("fail to create socket");		
		goto fail;	
	}
	memset(&c_addr, 0, sizeof(struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;	
	strcpy(c_addr.sun_path, MONC_SOCK);
	unlink(MONC_SOCK);	
	if (bind(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {		
		perror("fail to bind");		
		goto fail;	
	}	
	memset(&c_addr, 0, sizeof(struct sockaddr_un));	
	c_addr.sun_family = AF_UNIX;	
	strcpy(c_addr.sun_path, MONS_SOCK);	
	if (connect(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {	
		perror("fail to connect");		
		goto fail;	
	}	
	if (send(monfd, &mon, len, 0) == -1) {	
		perror("fail to send");		
		goto fail;	
	}
	if (mon.resp) {		
		struct timeval tv;		
		fd_set fds;		
		tv.tv_sec = 0;		
		tv.tv_usec = 100 * 1000;		
		FD_ZERO(&fds);		
		FD_SET(monfd, &fds);		
		select(monfd + 1, &fds, 0, 0, &tv);		
		if (FD_ISSET(monfd, &fds)) {			
			ssize_t recvsize;			
			struct sockaddr_un from;			
			socklen_t fromlen = sizeof(from);			
			char buf[256];			
			if ((recvsize = recv(monfd, buf, sizeof(buf), 0)) < 0) {
				goto fail;	
			}			
			printf("%s\n", buf);		
			}
	}

	fail:	
		if (monfd >= 0)		
		close(monfd);	
		return 0;
}
void mon_init()
{

	monfd = creat_socket_server(MONC_SOCK);
	//cprintf("%s:%s:%d monfd:%d\n",__FILE__, __func__, __LINE__,monfd);

}

void mon_exit()
{
	if(monfd >= 0) {
		close(monfd);
	}
	monfd = -1;
}
#endif

CommandResult
handle_play(Client &client, unsigned argc, char *argv[])
{
	int song = -1;

#ifdef MONTAGE_CTRL_PLAYER
inform_ctrl_msg(MONDLNA,NULL);
#endif
	
	if (argc == 2 && !check_int(client, &song, argv[1]))
		return CommandResult::ERROR;
	PlaylistResult result = client.partition.PlayPosition(song);
	return print_playlist_result(client, result);
}

CommandResult
handle_playid(Client &client, unsigned argc, char *argv[])
{
	int id = -1;

	if (argc == 2 && !check_int(client, &id, argv[1]))
		return CommandResult::ERROR;

	PlaylistResult result = client.partition.PlayId(id);
	return print_playlist_result(client, result);
}

CommandResult
handle_stop(Client &client,
	    gcc_unused unsigned argc, gcc_unused char *argv[])
{
	cdb_set_int("$current_play_state", 0);
	client.partition.Stop();
	return CommandResult::OK;
}

CommandResult
handle_currentsong(Client &client,
		   gcc_unused unsigned argc, gcc_unused char *argv[])
{
	playlist_print_current(client, client.playlist);
	return CommandResult::OK;
}

CommandResult
handle_pause(Client &client,
	     unsigned argc, char *argv[])
{
	if (argc == 2) {
		bool pause_flag;
		if (!check_bool(client, &pause_flag, argv[1]))
			return CommandResult::ERROR;

		client.player_control.SetPause(pause_flag);
	} else
		client.player_control.Pause();

	return CommandResult::OK;
}
#ifdef TURING_MPD_CHANGE
static PlayerState last_state = PlayerState::STOP;
static int last_volume = -1;
static int last_song = -1;
#if 0
#define cprintf(fmt, args...) do { \
FILE *fp = fopen("/dev/console", "w"); \
if (fp) { \
	fprintf(fp, fmt , ## args); \
	fclose(fp); \
	} \
} while (0)
#else
#define cprintf(fmt, args...)  
#endif
#endif

CommandResult
handle_status(Client &client,
	      gcc_unused unsigned argc, gcc_unused char *argv[])
{
#ifdef TURING_MPD_CHANGE
	int volume;
#endif
	const char *state = nullptr;
	int song;
#ifdef TURING_MPD_CHANGE
	struct PlayerControl &pc = client.player_control;
#endif

	const auto player_status = client.player_control.GetStatus();

	switch (player_status.state) {
	case PlayerState::STOP:
		state = "stop";
		break;
	case PlayerState::PAUSE:
		state = "pause";
		break;
	case PlayerState::PLAY:
		state = "play";
		break;
#ifdef MONTAGE_MPD_CMD_TRIGGER
	case PlayerState::BUFFERING:
		state = "buffering";
		break;
#endif
	}
#ifdef TURING_MPD_CHANGE
		if(player_status.state != last_state) {
			pc.listener.OnTuringChange();
			if(player_status.state == PlayerState::PAUSE || player_status.state == PlayerState::PLAY) {
				pc.Lock();
				pc.SetTuringState(PLAY);
				pc.Unlock();
			} else {
				pc.Lock();
				pc.SetTuringState(STOP);
				pc.Unlock();
				last_song = -1;
			}
		}
		last_state = player_status.state;
#endif

	const playlist &playlist = client.playlist;
	client_printf(client,
		      "volume: %i\n"
		      COMMAND_STATUS_REPEAT ": %i\n"
		      COMMAND_STATUS_RANDOM ": %i\n"
		      COMMAND_STATUS_SINGLE ": %i\n"
		      COMMAND_STATUS_CONSUME ": %i\n"
		      COMMAND_STATUS_PLAYLIST ": %li\n"
		      COMMAND_STATUS_PLAYLIST_LENGTH ": %i\n"
		      COMMAND_STATUS_MIXRAMPDB ": %f\n"
		      COMMAND_STATUS_STATE ": %s\n",
		      volume_level_get(client.partition.outputs),
		      playlist.GetRepeat(),
		      playlist.GetRandom(),
		      playlist.GetSingle(),
		      playlist.GetConsume(),
		      (unsigned long)playlist.GetVersion(),
		      playlist.GetLength(),
		      client.player_control.GetMixRampDb(),
		      state);
#ifdef TURING_MPD_CHANGE
			volume = volume_level_get(client.partition.outputs);
			if(volume != last_volume) {

				pc.Lock();
				pc.SetTuringState(VOLUME);
				pc.Unlock();
				pc.listener.OnTuringChange();
			}
			last_volume = volume;
#endif

#ifdef MONTAGE_ENABLE_DLNA_PAUSE
	client_printf(client,
			COMMAND_STATUS_DLNA_PAUSE ": %i\n",
			playlist.GetDlna());
#endif

#ifdef MONTAGE_ENABLE_LINKIP
	client_printf(client,
			COMMAND_STATUS_LINKIP ": %ul\n",
			playlist.GetLinkip());
#endif

	if (client.player_control.GetCrossFade() > 0)
		client_printf(client,
			      COMMAND_STATUS_CROSSFADE ": %i\n",
			      int(client.player_control.GetCrossFade() + 0.5));

	if (client.player_control.GetMixRampDelay() > 0)
		client_printf(client,
			      COMMAND_STATUS_MIXRAMPDELAY ": %f\n",
			      client.player_control.GetMixRampDelay());
	unsigned songid = 0;
	song = playlist.GetCurrentPosition();
	if (song >= 0) {
		
		client_printf(client,
			      COMMAND_STATUS_SONG ": %i\n"
			      COMMAND_STATUS_SONGID ": %u\n",
			      song, playlist.PositionToId(song));
		songid = playlist.PositionToId(song);
#ifdef TURING_MPD_CHANGE
		if(song != last_song) {
			cprintf("last_song :%d, song :%d songid:%d\n",  last_song, song, songid);
			
			pc.listener.OnTuringChange();
			if(song != -1 && last_song != -1) {
				#if 0
				if(last_song +1 == song) {
					pc.Lock();
					pc.SetTuringState(NEXT);
					pc.Unlock();
				}
				if(last_song - 1 == song) {
					pc.Lock();
					pc.SetTuringState(PREV);
					pc.Unlock();
				}
				#else
				pc.Lock();
				pc.SetTuringState(CHANGE);
				pc.Unlock();
				#endif
			}
		}
		
#endif
	}
	last_song = song;

	if (player_status.state != PlayerState::STOP) {
		client_printf(client,
			      COMMAND_STATUS_TIME ": %i:%i\n"
			      "elapsed: %1.3f\n"
			      COMMAND_STATUS_BITRATE ": %u\n",
			      player_status.elapsed_time.RoundS(),
			      player_status.total_time.IsNegative()
			      ? 0u
			      : unsigned(player_status.total_time.RoundS()),
			      player_status.elapsed_time.ToDoubleS(),
			      player_status.bit_rate);

		if (player_status.audio_format.IsDefined()) {
			struct audio_format_string af_string;

			client_printf(client,
				      COMMAND_STATUS_AUDIO ": %s\n",
				      audio_format_to_string(player_status.audio_format,
							     &af_string));
		}
	}

#ifdef ENABLE_DATABASE
	const UpdateService *update_service = client.partition.instance.update;
	unsigned updateJobId = update_service != nullptr
		? update_service->GetId()
		: 0;
	if (updateJobId != 0) {
		client_printf(client,
			      COMMAND_STATUS_UPDATING_DB ": %i\n",
			      updateJobId);
	}
#endif

	Error error = client.player_control.LockGetError();
	if (error.IsDefined())
		client_printf(client,
			      COMMAND_STATUS_ERROR ": %s\n",
			      error.GetMessage());

	song = playlist.GetNextPosition();
	if (song >= 0) {
		client_printf(client,
			      COMMAND_STATUS_NEXTSONG ": %i\n"
			      COMMAND_STATUS_NEXTSONGID ": %u\n",
			      song, playlist.PositionToId(song));
	}

	return CommandResult::OK;
}

CommandResult
handle_next(Client &client,
	    gcc_unused unsigned argc, gcc_unused char *argv[])
{
	playlist &playlist = client.playlist;

	/* single mode is not considered when this is user who
	 * wants to change song. */
	const bool single = playlist.queue.single;
	playlist.queue.single = false;

	client.partition.PlayNext();

	playlist.queue.single = single;
	return CommandResult::OK;
}

CommandResult
handle_previous(Client &client,
		gcc_unused unsigned argc, gcc_unused char *argv[])
{
	client.partition.PlayPrevious();
	return CommandResult::OK;
}

CommandResult
handle_repeat(Client &client, gcc_unused unsigned argc, char *argv[])
{
	bool status;
	if (!check_bool(client, &status, argv[1]))
		return CommandResult::ERROR;

	client.partition.SetRepeat(status);
	return CommandResult::OK;
}

CommandResult
handle_single(Client &client, gcc_unused unsigned argc, char *argv[])
{
	bool status;
	if (!check_bool(client, &status, argv[1]))
		return CommandResult::ERROR;

	client.partition.SetSingle(status);
	return CommandResult::OK;
}

#ifdef MONTAGE_ENABLE_DLNA_PAUSE
CommandResult
handle_dlna_pause(Client &client, gcc_unused unsigned argc, char *argv[])
{
    bool status;

    if (!check_bool(client, &status, argv[1]))
		return CommandResult::ERROR;

	client.partition.SetDlna(status);
	return CommandResult::OK;
}
#endif

#ifdef MONTAGE_ENABLE_LINKIP
CommandResult
handle_linkip(Client &client, gcc_unused unsigned argc, char *argv[])
{
    uint32_t linkip;

    if (!check_uint32(client, &linkip, argv[1])) {
		return CommandResult::ERROR;
    }
	client.partition.SetLinkip(linkip);
	
	return CommandResult::OK;
}
#endif

CommandResult
handle_consume(Client &client, gcc_unused unsigned argc, char *argv[])
{
	bool status;
	if (!check_bool(client, &status, argv[1]))
		return CommandResult::ERROR;

	client.partition.SetConsume(status);
	return CommandResult::OK;
}

CommandResult
handle_random(Client &client, gcc_unused unsigned argc, char *argv[])
{
	bool status;
	if (!check_bool(client, &status, argv[1]))
		return CommandResult::ERROR;

	client.partition.SetRandom(status);
	client.partition.outputs.SetReplayGainMode(replay_gain_get_real_mode(client.partition.GetRandom()));
	return CommandResult::OK;
}

CommandResult
handle_clearerror(gcc_unused Client &client,
		  gcc_unused unsigned argc, gcc_unused char *argv[])
{
	client.player_control.ClearError();
	return CommandResult::OK;
}

CommandResult
handle_seek(Client &client, gcc_unused unsigned argc, char *argv[])
{
	unsigned song;
	SongTime seek_time;

	if (!check_unsigned(client, &song, argv[1]))
		return CommandResult::ERROR;
	if (!ParseCommandArg(client, seek_time, argv[2]))
		return CommandResult::ERROR;

	PlaylistResult result =
		client.partition.SeekSongPosition(song, seek_time);
	return print_playlist_result(client, result);
}

CommandResult
handle_seekid(Client &client, gcc_unused unsigned argc, char *argv[])
{
	unsigned id;
	SongTime seek_time;

	if (!check_unsigned(client, &id, argv[1]))
		return CommandResult::ERROR;
	if (!ParseCommandArg(client, seek_time, argv[2]))
		return CommandResult::ERROR;

	PlaylistResult result =
		client.partition.SeekSongId(id, seek_time);
	return print_playlist_result(client, result);
}

CommandResult
handle_seekcur(Client &client, gcc_unused unsigned argc, char *argv[])
{
	const char *p = argv[1];
	bool relative = *p == '+' || *p == '-';
	SignedSongTime seek_time;
	if (!ParseCommandArg(client, seek_time, p))
		return CommandResult::ERROR;

	PlaylistResult result =
		client.partition.SeekCurrent(seek_time, relative);
	return print_playlist_result(client, result);
}

CommandResult
handle_crossfade(Client &client, gcc_unused unsigned argc, char *argv[])
{
	unsigned xfade_time;

	if (!check_unsigned(client, &xfade_time, argv[1]))
		return CommandResult::ERROR;
	client.player_control.SetCrossFade(xfade_time);

	return CommandResult::OK;
}

CommandResult
handle_mixrampdb(Client &client, gcc_unused unsigned argc, char *argv[])
{
	float db;

	if (!check_float(client, &db, argv[1]))
		return CommandResult::ERROR;
	client.player_control.SetMixRampDb(db);

	return CommandResult::OK;
}

CommandResult
handle_mixrampdelay(Client &client, gcc_unused unsigned argc, char *argv[])
{
	float delay_secs;

	if (!check_float(client, &delay_secs, argv[1]))
		return CommandResult::ERROR;
	client.player_control.SetMixRampDelay(delay_secs);

	return CommandResult::OK;
}

CommandResult
handle_replay_gain_mode(Client &client,
			gcc_unused unsigned argc, char *argv[])
{
	if (!replay_gain_set_mode_string(argv[1])) {
		command_error(client, ACK_ERROR_ARG,
			      "Unrecognized replay gain mode");
		return CommandResult::ERROR;
	}

	client.partition.outputs.SetReplayGainMode(replay_gain_get_real_mode(client.playlist.queue.random));
	return CommandResult::OK;
}

CommandResult
handle_replay_gain_status(Client &client,
			  gcc_unused unsigned argc, gcc_unused char *argv[])
{
	client_printf(client, "replay_gain_mode: %s\n",
		      replay_gain_get_mode_string());
	return CommandResult::OK;
}
