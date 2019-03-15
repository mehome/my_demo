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

#ifndef MPD_GLOBAL_EVENTS_HXX
#define MPD_GLOBAL_EVENTS_HXX

#ifdef WIN32
#include <windows.h>
/* DELETE is a WIN32 macro that poisons our namespace; this is a
   kludge to allow us to use it anyway */
#ifdef DELETE
#undef DELETE
#endif
#endif

class EventLoop;

namespace GlobalEvents {
	enum Event {
		/** an idle event was emitted */
		IDLE,

		/** must call playlist_sync() */
		PLAYLIST,

		/** the current song's tag has changed */
		TAG,

#ifdef WIN32
		/** shutdown requested */
		SHUTDOWN,
#endif
#ifdef TURING_MPD_CHANGE
		TURING,
#endif
		MAX
	};

	typedef void (*Handler)();

	void Initialize(EventLoop &loop);

	void Deinitialize();

	void Register(Event event, Handler handler);

	void Emit(Event event);
}

#endif /* MAIN_NOTIFY_H */
