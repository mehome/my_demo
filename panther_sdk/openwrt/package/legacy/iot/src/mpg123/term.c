/*
	term: terminal control

	copyright ?-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/


#include "term.h"


#ifdef HAVE_TERMIOS


#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>



#include "mpg123app.h"
#include "mpgplayer.h"

#include "myutils/debug.h"

static int term_enable = 0;
static int socketPair[2];

static struct termios old_tio;
static int seeking = FALSE;
static int stopped = 0;
static int paused = 0;
static int pause_cycle;

static char inputKey = 0;
static pthread_mutex_t termMtx = PTHREAD_MUTEX_INITIALIZER;


extern out123_handle *ao;

/* Buffered key from a signal or whatnot.
   We ignore the null character... */
static char prekey = 0;
static double pitch;
static long rva; /* (which) rva to do: 0: nothing, 1: radio/mix/track 2: album/audiophile */
static off_t offset = 0;

/* Hm, next step would be some system in this, plus configurability...
   Two keys for everything? It's just stop/pause for now... */
struct keydef { const char key; const char key2; const char* desc; };
struct keydef term_help[] =
{
	 { MPG123_STOP_KEY,  ' ', "interrupt/restart playback (i.e. '(un)pause')" }
	,{ MPG123_NEXT_KEY,    0, "next track" }
	,{ MPG123_PREV_KEY,    0, "previous track" }
	,{ MPG123_NEXT_DIR_KEY, 0, "next directory (next track until directory part changes)" }
	,{ MPG123_PREV_DIR_KEY, 0, "previous directory (previous track until directory part changes)" }
	,{ MPG123_BACK_KEY,    0, "back to beginning of track" }
	,{ MPG123_PAUSE_KEY,   0, "loop around current position (don't combine with output buffer)" }
	,{ MPG123_FORWARD_KEY, 0, "forward" }
	,{ MPG123_REWIND_KEY,  0, "rewind" }
	,{ MPG123_FAST_FORWARD_KEY, 0, "fast forward" }
	,{ MPG123_FAST_REWIND_KEY,  0, "fast rewind" }
	,{ MPG123_FINE_FORWARD_KEY, 0, "fine forward" }
	,{ MPG123_FINE_REWIND_KEY,  0, "fine rewind" }
	,{ MPG123_VOL_UP_KEY,   0, "volume up" }
	,{ MPG123_VOL_DOWN_KEY, 0, "volume down" }
	,{ MPG123_RVA_KEY,      0, "RVA switch" }
	,{ MPG123_VERBOSE_KEY,  0, "verbose switch" }
	,{ MPG123_PLAYLIST_KEY, 0, "list current playlist, indicating current track there" }
	,{ MPG123_TAG_KEY,      0, "display tag info (again)" }
	,{ MPG123_MPEG_KEY,     0, "print MPEG header info (again)" }
	,{ MPG123_HELP_KEY,     0, "this help" }
	,{ MPG123_QUIT_KEY,     0, "quit" }
	,{ MPG123_PITCH_UP_KEY, MPG123_PITCH_BUP_KEY, "pitch up (small step, big step)" }
	,{ MPG123_PITCH_DOWN_KEY, MPG123_PITCH_BDOWN_KEY, "pitch down (small step, big step)" }
	,{ MPG123_PITCH_ZERO_KEY, 0, "reset pitch to zero" }
	,{ MPG123_BOOKMARK_KEY, 0, "print out current position in playlist and track, for the benefit of some external tool to store bookmarks" }
};



#if 0
void term_input(char val)
{	int ret ;
	if(!term_enable)
		return;
reSend:
	warning("send %c ",val);
	if ((ret = send(socketPair[1], &val, sizeof(val), MSG_DONTWAIT) )< 0) {
		if ( errno == EINTR || errno == EAGAIN ) {
			goto reSend;    /* try again */
		}
		error("send %c failed", val);
	}
	warning("send %c ok ret:%d",val, ret);
}
#else
void term_input(char val)
{	
	pthread_mutex_lock(&termMtx);
	inputKey = val;
	pthread_mutex_unlock(&termMtx);
}

#endif


#if 0
/* initialze terminal */
void term_init(void)
{
	warning("term_init");

	term_enable = 0;
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair)!=0) {
        error("can't create signal pipe\n");
        return ;
    }
	term_enable = 1;
}
#else
void term_init(void)
{
	warning("term_init");

	term_enable = 0;
	pthread_mutex_lock(&termMtx);
	inputKey = 0;
	pthread_mutex_unlock(&termMtx);

	term_enable = 1;
}

#endif



static void term_handle_input(mpg123_handle *, out123_handle *, int);



static int print_index(mpg123_handle *mh)
{
	int err;
	size_t c, fill;
	off_t *index;
	off_t  step;
	err = mpg123_index(mh, &index, &step, &fill);
	if(err == MPG123_ERR)
	{
		fprintf(stderr, "Error accessing frame index: %s\n", mpg123_strerror(mh));
		return err;
	}
	for(c=0; c < fill;++c) 
		fprintf(stderr, "[%lu] %lu: %li (+%li)\n",
		(unsigned long) c,
		(unsigned long) (c*step), 
		(long) index[c], 
		(long) (c ? index[c]-index[c-1] : 0));
	return MPG123_OK;
}



/* Go back to the start for the cyclic pausing. */
void pause_recycle(mpg123_handle *fr)
{
	/* Take care not to go backwards in time in steps of 1 frame
		 That is what the +1 is for. */
	pause_cycle=(int)(LOOP_CYCLES/mpg123_tpf(fr));
	offset-=pause_cycle;
}

/* Done with pausing, no offset anymore. Just continuous playback from now. */
void pause_uncycle(void)
{
	offset += pause_cycle;
}

off_t term_control(mpg123_handle *fr, out123_handle *ao)
{
	offset = 0;
	//info("control for frame: %li, enable: %i", (long)mpg123_tellframe(fr), term_enable);
	if(!term_enable) return 0;

	if(paused)
	{
		/* pause_cycle counts the remaining frames _after_ this one, thus <0, not ==0 . */
		if(--pause_cycle < 0)
			pause_recycle(fr);
	}

	do
	{
	
	
		if(IsMpg123Cancled()) {
			break;
		}
		off_t old_offset = offset;
		term_handle_input(fr, ao, stopped|seeking);
		if((offset < 0) && (-offset > framenum)) 
			offset = - framenum;
		if(IsMpg123PlayerStateChange()) {
			warning("IsMpg123PlayerStateChange");
			//IotAudioStatusReport(AudioStatus * status)
			IotDeviceStatusReport();
		}
		SetLastState();
		//sleep(1);
	}while (!intflag && stopped);

	/* Make the seeking experience with buffer less annoying.
	   No sound during seek, but at least it is possible to go backwards. */
	//if(offset)
	if (0)
	{
		if((offset = mpg123_seek_frame(fr, offset, SEEK_CUR)) >= 0)
		debug("seeked to %li", (long)offset);
		else error("seek failed: %s!", mpg123_strerror(fr));
		/* Buffer resync already happened on un-stop? */
		/* if(param.usebuffer) audio_drop(ao);*/
	}
	return 0;
}

/* Stop playback while seeking if buffer is involved. */
static void seekmode(mpg123_handle *mh, out123_handle *ao)
{
	if(!stopped)
	{
		int channels = 0;
		int encoding = 0;
		int pcmframe;
		off_t back_samples = 0;

		stopped = TRUE;
		out123_pause(ao);
		mpg123_getformat(mh, NULL, &channels, &encoding);
		pcmframe = out123_encsize(encoding)*channels;
		if(pcmframe > 0)
			back_samples = out123_buffered(ao)/pcmframe;
		info("seeking back %ld samples from %ld"
		,	(long)back_samples, (long)mpg123_tell(mh));
		mpg123_seek(mh, -back_samples, SEEK_CUR);
		out123_drop(ao);
		info("dropped, now at %ld"
		,	(long)mpg123_tell(mh));
		info("%s", MPG123_STOPPED_STRING);

	}
}

/* Get the next pressed key, if any.
   Returns 1 when there is a key, 0 if not. */
#if 0
static int get_key(int do_delay, char *val)
{
	int ret;
	fd_set r;
	struct timeval t;
	//warning("in get_key");
	/* Shortcut: If some other means sent a key, use it. */
	if(prekey)
	{
		debug("Got prekey: %c\n", prekey);
		*val = prekey;
		prekey = 0;
		return 1;
	}

	t.tv_sec=1;
	t.tv_usec=(do_delay) ? 10*1000 : 0;

	FD_ZERO(&r);
	FD_SET(socketPair[0],&r);

	ret = select(socketPair[0] + 1,&r,NULL,NULL,&t);

	if ( ret < 0 ) {
		if ( errno == EINTR || errno == EAGAIN ) {
		   // continue;
		}      
		//error("Timer Select:");
		return 0;
	}  
	if (ret == 0) {
	//	warning("timeout.tv_sec:%ld\n" ,t.tv_sec);	
		return 0;
	} else {
		 if (FD_ISSET(socketPair[0], &r)) {
                ret = read(socketPair[0], &val, 1);    //don't care what we read
                if(ret <= 0) 
					return 0; /* Well, we couldn't read the key, so there is none. */
				else {
					warning("read %c ",val);
					return 1;
				}
                
            }
	}
}
#else
char    get_key()
{
	char val;
	pthread_mutex_lock(&termMtx);
	val = inputKey;
	pthread_mutex_unlock(&termMtx);
	return val;
}

void term_quit()
{
	pthread_mutex_lock(&termMtx);
	stopped = 0;
	pthread_mutex_unlock(&termMtx);
}

#endif

void input_key_clear()
{
	pthread_mutex_lock(&termMtx);
	inputKey = 0;
	pthread_mutex_unlock(&termMtx);
}
static void term_handle_key(mpg123_handle *fr, out123_handle *ao, char val)
{

	switch(tolower(val))
	{
	case MPG123_BACK_KEY:
		out123_pause(ao);
		out123_drop(ao);
		if(paused) pause_cycle=(int)(LOOP_CYCLES/mpg123_tpf(fr));

		if(mpg123_seek_frame(fr, 0, SEEK_SET) < 0)
		error("Seek to begin failed: %s", mpg123_strerror(fr));

		framenum=0;
	break;
	case MPG123_NEXT_KEY:
		out123_pause(ao);
		out123_drop(ao);
		next_track();
	break;
	case MPG123_NEXT_DIR_KEY:
		out123_pause(ao);
		out123_drop(ao);
		next_dir();
	break;
	case MPG123_QUIT_KEY:
		error("QUIT");
		if(stopped)
		{
			stopped = 0;
			out123_pause(ao); /* no chance for annoying underrun warnings */
			out123_drop(ao);
		}
		set_intflag();
		offset = 0;
	break;
	#if 0
	case MPG123_PAUSE_KEY:
		paused=1-paused;
		out123_pause(ao); /* underrun awareness */
		out123_drop(ao);
		if(paused)
		{
			/* Not really sure if that is what is wanted
				 This jumps in audio output, but has direct reaction to pausing loop. */
			out123_param_float(ao, OUT123_PRELOAD, 0.);
			pause_recycle(fr);
		}
		else
			out123_param_float(ao, OUT123_PRELOAD, 0.);
		if(stopped)
			stopped=0;
		//offset = 0;
		error("%s", (paused) ? MPG123_PAUSED_STRING : MPG123_EMPTY_STRING);
	break;
	#endif
	case MPG123_PAUSE_KEY:
		/* TODO: Verify/ensure that there is no "chirp from the past" when
	   seeking while stopped. */
		stopped=1-stopped;
		if(paused) {
			paused=0;
			offset -= pause_cycle;
		}
		if(stopped) {
			out123_pause(ao);
			warning("SetMpg123PlayerState PLAYER_STATE_PAUSE");
			SetMpg123PlayerState(PLAYER_STATE_PAUSE);
		}
		else
		{
			warning("SetMpg123PlayerState PLAYER_STATE_PLAY");
			SetMpg123PlayerState(PLAYER_STATE_PLAY);
			if(offset) /* If position changed, old is outdated. */
				out123_drop(ao);
			/* No out123_continue(), that's triggered by out123_play(). */
		}
		error("%s", (stopped) ? "pasued" : "playing");
	
		break;
	case MPG123_STOP_KEY:
	case ' ':
		/* TODO: Verify/ensure that there is no "chirp from the past" when
		   seeking while stopped. */
		stopped=1-stopped;
		if(paused) {
			paused=0;
			offset -= pause_cycle;
		}
		if(stopped) {
			out123_pause(ao);
		}
		else
		{
			if(offset) /* If position changed, old is outdated. */
				out123_drop(ao);
			/* No out123_continue(), that's triggered by out123_play(). */
		}
		warning("%s", (stopped) ? MPG123_STOPPED_STRING : MPG123_EMPTY_STRING);
		break;
	case MPG123_FINE_REWIND_KEY:
		seekmode(fr, ao);
		offset--;
	break;
	case MPG123_FINE_FORWARD_KEY:
		seekmode(fr, ao);
		offset++;
	break;
	case MPG123_REWIND_KEY:
		seekmode(fr, ao);
		  offset-=10;
	break;
	case MPG123_FORWARD_KEY:
		seekmode(fr, ao);
		offset+=10;
	break;
	case MPG123_FAST_REWIND_KEY:
		seekmode(fr, ao);
		offset-=50;
	break;
	case MPG123_FAST_FORWARD_KEY:
		seekmode(fr, ao);
		offset+=50;
	break;
	case MPG123_VOL_UP_KEY:
		mpg123_volume_change(fr, 0.02);
	break;
	case MPG123_VOL_DOWN_KEY:
		mpg123_volume_change(fr, -0.02);
	break;
	case MPG123_PITCH_UP_KEY:
	case MPG123_PITCH_BUP_KEY:
	case MPG123_PITCH_DOWN_KEY:
	case MPG123_PITCH_BDOWN_KEY:
	case MPG123_PITCH_ZERO_KEY:
	{
		//double new_pitch = param.pitch;
		//switch(val) /* Not tolower here! */
		//{
		//	case MPG123_PITCH_UP_KEY:    new_pitch += MPG123_PITCH_VAL;  break;
		//	case MPG123_PITCH_BUP_KEY:   new_pitch += MPG123_PITCH_BVAL; break;
		//	case MPG123_PITCH_DOWN_KEY:  new_pitch -= MPG123_PITCH_VAL;  break;
		//	case MPG123_PITCH_BDOWN_KEY: new_pitch -= MPG123_PITCH_BVAL; break;
		//	case MPG123_PITCH_ZERO_KEY:  new_pitch = 0.0; break;
		//}
		//set_pitch(fr, ao, new_pitch);
		//fprintf(stderr, "New pitch: %f\n", param.pitch);
	}
	break;
	case MPG123_VERBOSE_KEY:
		//param.verbose++;
		//if(param.verbose > VERBOSE_MAX)
		//{
			//param.verbose = 0;
			//clear_stat();
		//}
		//mpg123_param(fr, MPG123_VERBOSE, param.verbose, 0);
	break;
	case MPG123_RVA_KEY:
		if(++rva > MPG123_RVA_MAX) rva = 0;
		//if(param.verbose)
		//	fprintf(stderr, "\n");
		mpg123_param(fr, MPG123_RVA, rva, 0);
		mpg123_volume_change(fr, 0.);
	break;
	case MPG123_PREV_KEY:
		out123_pause(ao);
		out123_drop(ao);

		//prev_track();
	break;
	case MPG123_PREV_DIR_KEY:
		out123_pause(ao);
		out123_drop(ao);
		//prev_dir();
	break;
	case MPG123_PLAYLIST_KEY:
		//print_playlist(stderr, 1);

	break;
	case MPG123_TAG_KEY:
		//print_id3_tag(fr, param.long_id3, stderr);
		
	break;
	case MPG123_MPEG_KEY:
	
		//if(param.verbose > 1)
		//	print_header(fr);
		//else
		//	print_header_compact(fr);

	break;
	case MPG123_HELP_KEY:
	{ /* This is more than the one-liner before, but it's less spaghetti. */
		int i;
		warning("\n\n -= terminal control keys =-\n");
		for(i=0; i<(sizeof(term_help)/sizeof(struct keydef)); ++i)
		{
			if(term_help[i].key2) fprintf(stderr, "[%c] or [%c]", term_help[i].key, term_help[i].key2);
			else fprintf(stderr, "[%c]", term_help[i].key);

			fprintf(stderr, "\t%s\n", term_help[i].desc);
		}
		warning("\nAlso, the number row (starting at 1, ending at 0) gives you jump points into the current track at 10%% intervals.\n");
	}
	break;
	case MPG123_FRAME_INDEX_KEY:
	case MPG123_VARIOUS_INFO_KEY:
		
		switch(val) /* because of tolower() ... */
		{
			case MPG123_FRAME_INDEX_KEY:
			print_index(fr);
			{
				long accurate;
				if(mpg123_getstate(fr, MPG123_ACCURATE, &accurate, NULL) == MPG123_OK)
				fprintf(stderr, "Accurate position: %s\n", (accurate == 0 ? "no" : "yes"));
				else
				error("Unable to get state: %s", mpg123_strerror(fr));
			}
			break;
			case MPG123_VARIOUS_INFO_KEY:
			{
				const char* curdec = mpg123_current_decoder(fr);
				if(curdec == NULL) warning("Cannot get decoder info!");
				else warning("Active decoder: %s", curdec);
			}
		}
	break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		off_t len;
		int num;
		num = val == '0' ? 10 : val - '0';
		--num; /* from 0 to 9 */

		/* Do not swith to seekmode() here, as we are jumping once to a
		   specific position. Dropping buffer contents is enough and there
		   is no race filling the buffer or waiting for more incremental
		   seek orders. */
		len = mpg123_length(fr);
		out123_pause(ao);
		out123_drop(ao);
		if(len > 0)
			mpg123_seek(fr, (off_t)( (num/10.)*len ), SEEK_SET);
	}
	break;
	case MPG123_BOOKMARK_KEY:
	//	continue_msg("BOOKMARK");
	break;
	default:
		;
	}
	input_key_clear();
}

static void term_handle_input(mpg123_handle *fr, out123_handle *ao, int do_delay)
{
	//warning("in term_handle_input");
	char val;
	/* Do we really want that while loop? This means possibly handling multiple inputs that come very rapidly in one go. */
	//while(get_key(do_delay, &val))
	if((val = get_key()) != 0)
	{
		term_handle_key(fr, ao, val);
	}
	//input_key_clear();
}

void term_exit(void)
{
	/* Bring cursor back. */
	if(!term_enable)
		return;
	close(socketPair[0]);
    close(socketPair[1]);
}

#endif

