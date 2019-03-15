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
#include "Playlist.hxx"
#include "PlaylistError.hxx"
#include "PlayerControl.hxx"
#include "DetachedSong.hxx"
#include "Idle.hxx"
#include "Log.hxx"

#include <errno.h>
#include <assert.h>
#include <cstring>
#include <unistd.h>

#ifdef TURING_MPD_CHANGE 
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#endif

#ifdef MONTAGE_MPD_PATCH
extern "C"
{
#include <cdb.h>
}
#endif

///////////////////////
#ifdef MONTAGE_MPD_PATCH
int my_system(const char * cmd)   
{   
	FILE * fp;   
	int res; char buf[1024];
	if (cmd == NULL)   
	{   
		printf("my_system cmd is NULL!\n");  
		return -1;  
	} 
	if ((fp = popen(cmd, "r") ) == NULL)   
	{   
		perror("popen");  
		printf("popen error: %s/n", strerror(errno)); return -1;   
	}else  
	{  
		while(fgets(buf, sizeof(buf), fp))   
		{   
			printf("%s", buf);   
		} 
		if ( (res = pclose(fp)) == -1)   
		{   
			printf("close popen file pointer fp error!\n"); return res;  
		}   
		else if (res == 0)   
		{  
			return res;  
		}   
		else   
		{   
			printf("popen res is :%d\n", res); return res;   
		}   
	}  
}   
#endif
#ifdef TURING_MPD_CHANGE 
static int OnWriteMsg(char *path,  char* cmd)
{
	int iRet = -1;
	int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, path);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("socket error\n");

		return 1;
	}

	flags = fcntl(iConnect_fd, F_GETFL, 0);
	fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("connecct error\n");
		close(iConnect_fd);
		return 1;
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));

	if (iRet != strlen(cmd))
	{
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

#endif

///////////////////////////
void
playlist::TagModified(DetachedSong &&song)
{
	if (!playing)
		return;

	assert(current >= 0);

	DetachedSong &current_song = queue.GetOrder(current);
	if (song.IsSame(current_song))
		current_song.MoveTagItemsFrom(std::move(song));

	queue.ModifyAtOrder(current);
	queue.IncrementVersion();
	idle_add(IDLE_PLAYLIST);
}

/**
 * Queue a song, addressed by its order number.
 */
static void
playlist_queue_song_order(playlist &playlist, PlayerControl &pc,
			  unsigned order)
{
	assert(playlist.queue.IsValidOrder(order));

	playlist.queued = order;

	const DetachedSong &song = playlist.queue.GetOrder(order);

	FormatDebug(playlist_domain, "queue song %i:\"%s\"",
		    playlist.queued, song.GetURI());

	pc.EnqueueSong(new DetachedSong(song));
}

/**
 * Called if the player thread has started playing the "queued" song.
 */
static void
playlist_song_started(playlist &playlist, PlayerControl &pc)
{
	assert(pc.next_song == nullptr);
	assert(playlist.queued >= -1);

	/* queued song has started: copy queued to current,
	   and notify the clients */

	int current = playlist.current;
	playlist.current = playlist.queued;
	playlist.queued = -1;

	if(playlist.queue.consume)
		playlist.DeleteOrder(pc, current);

	idle_add(IDLE_PLAYER);
}

const DetachedSong *
playlist::GetQueuedSong() const
{
	return playing && queued >= 0
		? &queue.GetOrder(queued)
		: nullptr;
}

void
playlist::UpdateQueuedSong(PlayerControl &pc, const DetachedSong *prev)
{
	if (!playing)
		return;

	if (prev == nullptr && bulk_edit)
		/* postponed until CommitBulk() to avoid always
		   queueing the first song that is being added (in
		   random mode) */
		return;

	assert(!queue.IsEmpty());
	assert((queued < 0) == (prev == nullptr));
	FormatDebug(playlist_domain, "-----------UpdateQueuedSong---------\n");
//#ifdef MONTAGE_MPD_PATCH
#if 0
        DetachedSong &song = queue.GetOrder(current);
        cdb_set_int("$mpd_lastplay", current+1);
        cdb_set("$current_play_uri", (void*)song.GetURI());
        my_system("cdb commit");
        char tempbuff[1024]={0};
        memset(tempbuff,0,sizeof(tempbuff)/sizeof(char));
        sprintf(tempbuff,"creatPlayList wPlayingSong \"%s\"",song.GetURI());
        my_system(tempbuff);
		FormatDebug(playlist_domain, "----------gggggggggggggggggg---------\n");
#endif

	const int next_order = current >= 0
		? queue.GetNextOrder(current)
		: 0;

	if (next_order == 0 && queue.random && !queue.single) {
		/* shuffle the song order again, so we get a different
		   order each time the playlist is played
		   completely */
		const unsigned current_position =
			queue.OrderToPosition(current);

		queue.ShuffleOrder();

		/* make sure that the current still points to
		   the current song, after the song order has been
		   shuffled */
		current = queue.PositionToOrder(current_position);
	}

	const DetachedSong *const next_song = next_order >= 0
		? &queue.GetOrder(next_order)
		: nullptr;

	if (prev != nullptr && next_song != prev) {
		/* clear the currently queued song */
		pc.Cancel();
		queued = -1;
	}

	if (next_order >= 0) {
		if (next_song != prev)
			playlist_queue_song_order(*this, pc, next_order);
		else
			queued = next_order;
	}
}

void
playlist::PlayOrder(PlayerControl &pc, int order)
{
	playing = true;
	queued = -1;

	const DetachedSong &song = queue.GetOrder(order);

	FormatDebug(playlist_domain, "play %i:\"%s\"", order, song.GetURI());

	pc.Play(new DetachedSong(song));
	current = order;

}

static void
playlist_resume_playback(playlist &playlist, PlayerControl &pc);

void
playlist::SyncWithPlayer(PlayerControl &pc)
{
	if (!playing)
		/* this event has reached us out of sync: we aren't
		   playing anymore; ignore the event */
		return;

	pc.Lock();
	const PlayerState pc_state = pc.GetState();
	const DetachedSong *pc_next_song = pc.next_song;
	pc.Unlock();

	if (pc_state == PlayerState::STOP)
		/* the player thread has stopped: check if playback
		   should be restarted with the next song.  That can
		   happen if the playlist isn't filling the queue fast
		   enough */
		playlist_resume_playback(*this, pc);
	else {
		/* check if the player thread has already started
		   playing the queued song */
		if (pc_next_song == nullptr && queued != -1)
			playlist_song_started(*this, pc);

		pc.Lock();
		pc_next_song = pc.next_song;
		pc.Unlock();

		/* make sure the queued song is always set (if
		   possible) */
		if (pc_next_song == nullptr && queued < 0)
			UpdateQueuedSong(pc, nullptr);
	}
}
#ifdef TURING_MPD_CHANGE
void
playlist::SendToTuring(PlayerControl &pc)
{
	
	pc.Lock();
	const unsigned f = pc.GetTuringState();
	//const unsigned f = turing_state.exchange(0);
	pc.Unlock();
	for (unsigned i = 0; i < MAX; ++i) {
		if (f & (1u << i)) {
			switch(i) 
			{
				case VOLUME:
					OnWriteMsg("/tmp/UNIX.turing", "MpcVolumeEvent");
					break;
				case PLAY:
					OnWriteMsg("/tmp/UNIX.turing", "MpcPlayEvent");
					break;
				case STOP:
					OnWriteMsg("/tmp/UNIX.turing", "MpcStopEvent");
					break;
				case PREV:
					OnWriteMsg("/tmp/UNIX.turing", "MpcPrevEvent");
					break;
				case NEXT:
					OnWriteMsg("/tmp/UNIX.turing", "MpcNextEvent");
					break;
				case CHANGE:
					OnWriteMsg("/tmp/UNIX.turing", "MpcSongChange");
					break;
			}
		}
	}
	
	
}
#endif

/**
 * The player has stopped for some reason.  Check the error, and
 * decide whether to re-start playback
 */
static void
playlist_resume_playback(playlist &playlist, PlayerControl &pc)
{
	assert(playlist.playing);
	assert(pc.GetState() == PlayerState::STOP);

	const auto error = pc.GetErrorType();
	if (error == PlayerError::NONE)
		playlist.error_count = 0;
	else
		++playlist.error_count;

	if ((playlist.stop_on_error && error != PlayerError::NONE) ||
	    error == PlayerError::OUTPUT ||
	    playlist.error_count >= playlist.queue.GetLength())
		/* too many errors, or critical error: stop
		   playback */
		playlist.Stop(pc);
	else
		/* continue playback at the next song */
		playlist.PlayNext(pc);
}

void
playlist::SetRepeat(PlayerControl &pc, bool status)
{
	if (status == queue.repeat)
		return;

	queue.repeat = status;

	pc.SetBorderPause(queue.single && !queue.repeat);

	/* if the last song is currently being played, the "next song"
	   might change when repeat mode is toggled */
	UpdateQueuedSong(pc, GetQueuedSong());

	idle_add(IDLE_OPTIONS);

#ifdef MONTAGE_MPD_PATCH
	cdb_set_int("$mpd_repeat", queue.repeat);
#endif
}

static void
playlist_order(playlist &playlist)
{
	if (playlist.current >= 0)
		/* update playlist.current, order==position now */
		playlist.current = playlist.queue.OrderToPosition(playlist.current);

	playlist.queue.RestoreOrder();
}

void
playlist::SetSingle(PlayerControl &pc, bool status)
{
	if (status == queue.single)
		return;

	queue.single = status;

	pc.SetBorderPause(queue.single && !queue.repeat);

	/* if the last song is currently being played, the "next song"
	   might change when single mode is toggled */
	UpdateQueuedSong(pc, GetQueuedSong());

	idle_add(IDLE_OPTIONS);
}

#ifdef MONTAGE_ENABLE_DLNA_PAUSE
void
playlist::SetDlna(PlayerControl &pc, bool status)
{
	if (status == queue.dlna_pause)
		return;

	queue.dlna_pause = status;

	idle_add(IDLE_OPTIONS);
}
#endif

#ifdef MONTAGE_ENABLE_LINKIP
void
playlist::SetLinkip(PlayerControl &pc, uint32_t linkip)
{
	queue.linkip = linkip;

	idle_add(IDLE_OPTIONS);
}
#endif

void
playlist::SetConsume(bool status)
{
	if (status == queue.consume)
		return;

	queue.consume = status;
	idle_add(IDLE_OPTIONS);
}

void
playlist::SetRandom(PlayerControl &pc, bool status)
{
	if (status == queue.random)
		return;

	const DetachedSong *const queued_song = GetQueuedSong();

	queue.random = status;

	if (queue.random) {
		/* shuffle the queue order, but preserve current */

		const int current_position = playing
			? GetCurrentPosition()
			: -1;

		queue.ShuffleOrder();

		if (current_position >= 0) {
			/* make sure the current song is the first in
			   the order list, so the whole rest of the
			   playlist is played after that */
			unsigned current_order =
				queue.PositionToOrder(current_position);
			queue.SwapOrders(0, current_order);
			current = 0;
		} else
			current = -1;
	} else
		playlist_order(*this);

	UpdateQueuedSong(pc, queued_song);

	idle_add(IDLE_OPTIONS);
}

int
playlist::GetCurrentPosition() const
{
	return current >= 0
		? queue.OrderToPosition(current)
		: -1;
}

int
playlist::GetNextPosition() const
{
	if (current < 0)
		return -1;

	if (queue.single && queue.repeat)
		return queue.OrderToPosition(current);
	else if (queue.IsValidOrder(current + 1))
		return queue.OrderToPosition(current + 1);
	else if (queue.repeat)
		return queue.OrderToPosition(0);

	return -1;
}
