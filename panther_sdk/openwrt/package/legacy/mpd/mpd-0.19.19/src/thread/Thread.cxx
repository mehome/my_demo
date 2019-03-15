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
#include "Thread.hxx"
#include "util/Error.hxx"

#ifdef ANDROID
#include "java/Global.hxx"
#endif

bool
Thread::Start(void (*_f)(void *ctx), void *_ctx, int prio, int stack,Error &error)
{
	assert(!IsDefined());
	
	pthread_attr_t attr;

	f = _f;
	ctx = _ctx;

#ifdef WIN32
	handle = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &id);
	if (handle == nullptr) {
		error.SetLastError("Failed to create thread");
		return false;
	}
#else
#ifndef NDEBUG
	creating = true;
#endif

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack );
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	if( prio > 0 ) {
		struct sched_param param;
		
		param.sched_priority = prio;
		pthread_attr_setschedpolicy (&attr, SCHED_RR);
 		pthread_attr_setschedparam(&attr, &param);
 		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	}

	int e = pthread_create(&handle, nullptr, ThreadProc, this);
	pthread_attr_destroy(&attr);
	
	if (e != 0) {
#ifndef NDEBUG
		creating = false;
#endif
		error.SetErrno(e, "Failed to create thread");
		return false;
	}

	defined = true;
#ifndef NDEBUG
	creating = false;
#endif
#endif

	return true;
}

void
Thread::Join()
{
	assert(IsDefined());
	assert(!IsInside());

#ifdef WIN32
	::WaitForSingleObject(handle, INFINITE);
	::CloseHandle(handle);
	handle = nullptr;
#else
	pthread_join(handle, nullptr);
	defined = false;
#endif
}

#ifdef WIN32

DWORD WINAPI
Thread::ThreadProc(LPVOID ctx)
{
	Thread &thread = *(Thread *)ctx;

	thread.f(thread.ctx);
	return 0;
}

#else

void *
Thread::ThreadProc(void *ctx)
{
	Thread &thread = *(Thread *)ctx;

#ifndef NDEBUG
	/* this works around a race condition that causes an assertion
	   failure due to IsInside() spuriously returning false right
	   after the thread has been created, and the calling thread
	   hasn't initialised "defined" yet */
	thread.defined = true;
#endif

	thread.f(thread.ctx);

#ifdef ANDROID
	Java::DetachCurrentThread();
#endif

	return nullptr;
}

#endif
