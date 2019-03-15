/*
 * Slave-clocked ALAC stream player. This file is part of Shairport.
 * Copyright (c) James Laird 2011, 2013
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <openssl/aes.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef MONTAGE_CTRL_PLAYER
#include <mon.h>
#endif

#include "common.h"
#include "player.h"
#include "rtp.h"

#ifdef FANCY_RESAMPLING
#include <samplerate.h>
#endif

#include "alac.h"
#include <mpd/client.h>
#include <libcchip/platform.h>

// parameters from the source
static unsigned char *aesiv;
static AES_KEY aes;
static int sampling_rate, frame_size;

#define FRAME_BYTES(frame_size) (4*frame_size)
// maximal resampling shift - conservative
#define OUTFRAME_BYTES(frame_size) (4*(frame_size+3))

static pthread_t player_thread;
static int please_stop;
static int encrypted; // Normally the audio is encrypted, but it may be not

static alac_file *decoder_info;
static int alsa_off = 0;
#ifdef FANCY_RESAMPLING
static int fancy_resampling = 1;
static SRC_STATE *src;
#endif

// interthread variables
static double volume = 1.0;
static int fix_volume = 0x10000;
static pthread_mutex_t vol_mutex = PTHREAD_MUTEX_INITIALIZER;

// default buffer size
// needs to be a power of 2 because of the way BUFIDX(seqno) works
#define BUFFER_FRAMES  512
#define MAX_PACKET      2048

typedef struct audio_buffer_entry {	// decoded audio packets
	int ready;
	signed short *data;
	uint32_t timestamp;
} abuf_t;
static abuf_t audio_buffer[BUFFER_FRAMES];
#define BUFIDX(seqno) ((seq_t)(seqno) % BUFFER_FRAMES)

// mutex-protected variables
static seq_t ab_read, ab_write;
static int ab_buffering = 1, ab_synced = 0, ab_pause = 0;
static pthread_mutex_t ab_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t ab_timestamp = 0;
static char ab_alac = 0;

void air_syslog(const char *format, ...)
{
	char buf[512];
	va_list args;
		
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	
	openlog("AirPlay", 0, 0);
	syslog(LOG_USER | LOG_NOTICE, "%s", buf);
	closelog();

	return;
}

static void ab_resync(int init)
{
	unsigned int i;

	//warn("init:[%s]",init == 1?"Y":"N");
	if (!init) {
		ab_pause = 1;
	}

	ab_synced = 0;
	ab_buffering = 1;

	for (i = 0; i < BUFFER_FRAMES; i++) {
		audio_buffer[i].ready = 0;
	}
}

// the sequence numbers will wrap pretty often.
// this returns true if the second arg is after the first
static inline int seq_order(seq_t a, seq_t b)
{
	signed short d = b - a;
	return d > 0;
}

static void alac_decode(short *dest, uint8_t * buf, int len)
{
	unsigned char packet[MAX_PACKET];
	assert(len <= MAX_PACKET);
    int outsize;

    if (encrypted) {
        unsigned char iv[16];
        int aeslen = len & ~0xf;
        memcpy(iv, aesiv, sizeof(iv));
        AES_cbc_encrypt(buf, packet, aeslen, &aes, iv, AES_DECRYPT);
        memcpy(packet+aeslen, buf+aeslen, len-aeslen);

        //int outsize;

        alac_decode_frame(decoder_info, packet, dest, &outsize);
    } else {
        alac_decode_frame(decoder_info, buf, dest, &outsize);
    }
	assert(outsize == FRAME_BYTES(frame_size));
}

static int init_decoder(int32_t fmtp[12])
{
	alac_file *alac;

	frame_size = fmtp[1];	// stereo samples
	sampling_rate = fmtp[11];

	int sample_size = fmtp[3];
	if (sample_size != 16)
		die("only 16-bit samples supported!");

	alac = alac_create(sample_size, 2);
	if (!alac)
		return 1;
	decoder_info = alac;

	alac->setinfo_max_samples_per_frame = frame_size;
	alac->setinfo_7a = fmtp[2];
	alac->setinfo_sample_size = sample_size;
	alac->setinfo_rice_historymult = fmtp[4];
	alac->setinfo_rice_initialhistory = fmtp[5];
	alac->setinfo_rice_kmodifier = fmtp[6];
	alac->setinfo_7f = fmtp[7];
	alac->setinfo_80 = fmtp[8];
	alac->setinfo_82 = fmtp[9];
	alac->setinfo_86 = fmtp[10];
	alac->setinfo_8a_rate = fmtp[11];
	alac_allocate_buffers(alac);
	return 0;
}

static void free_decoder(void)
{
	alac_free(decoder_info);
}

#ifdef FANCY_RESAMPLING
static int init_src(void)
{
	int err;
	if (fancy_resampling)
		src = src_new(SRC_SINC_MEDIUM_QUALITY, 2, &err);
	else
		src = NULL;

	return err;
}

static void free_src(void)
{
	src_delete(src);
	src = NULL;
}
#endif

static void init_buffer(void)
{
	int i;
	for (i = 0; i < BUFFER_FRAMES; i++)
		audio_buffer[i].data = malloc(OUTFRAME_BYTES(frame_size));
	ab_resync(1);
}

static void free_buffer(void)
{
	int i;
	for (i = 0; i < BUFFER_FRAMES; i++)
		free(audio_buffer[i].data);
}

void player_put_packet(void *pconn, seq_t seqno, uint32_t timestamp,
		       uint8_t * data, int len)
{
	abuf_t *abuf = 0, *abuf_t = 0;
	rtsp_conn_info *conn = pconn;
	int16_t buf_fill, diff;
	int audio_socket = conn->audio_socket;

	if (alsa_off)
		return;

	pthread_mutex_lock(&ab_mutex);
	if (!ab_synced) {
		ab_write = seqno - 1;
		ab_read = seqno;
		ab_synced = 1;
	}

	if (!ab_buffering) {
		/*To avoid we lost lot of pkt and cause overrun */
		if ((seq_diff(ab_read, seqno)) >= BUFFER_FRAMES) {
			/*Too Late pkt */
			if (!(seq_order(ab_read, seqno))) {
				goto done;
			} else {
				/*How to handle skip large index ?? */
				warn("WiFi environment might be terrible");
				ab_resync(0);
			}
		}
	}

	abuf = audio_buffer + BUFIDX(seqno);
	if (abuf->ready) {
		air_syslog("!!! This abuf have data exist. r:%d w:%d s:%d",
		      ab_read, ab_write, seqno);
		goto done;
	}

	diff = seq_diff(ab_write, seqno);
	if (diff == 1) {	// expected packet
		ab_write = seqno;
	} else if (diff > 1) {	// newer than expected
		air_syslog("Lost seq:%d, w:%d r:%d\n",seqno, ab_write, ab_read);
		abuf_t = audio_buffer + BUFIDX(ab_read);
		rtp_request_resend(audio_socket, ab_write + 1, seqno - 1);
		ab_write = seqno;
	} else if (seq_order(ab_read, seqno)) {	// late but not yet played
		air_syslog("Late but can use: seq:%d, w:%d r:%d\n",seqno, ab_write, ab_read);
	} else {		// too late.
		air_syslog("Too late cannot use: seq:%d, w:%d r:%d\n",seqno, ab_write, ab_read);
		abuf = 0;
	}
	buf_fill = seq_diff(ab_read, ab_write);
	pthread_mutex_unlock(&ab_mutex);

	if (abuf) {
        if (encrypted)
            alac_decode(abuf->data, data, len);
        else
            memcpy(abuf->data, data, len);
        abuf->ready = 1;
		abuf->timestamp = timestamp;
	}

	pthread_mutex_lock(&ab_mutex);

	if ((ab_buffering && buf_fill >= config.buffer_start_fill)) {
		ab_buffering = 0;
		ab_pause = 0;
		if (config.output->resume)
			config.output->resume();
	}
done:
	pthread_mutex_unlock(&ab_mutex);
}

static short lcg_rand(void)
{
	static unsigned long lcg_prev = 12345;
	lcg_prev = lcg_prev * 69069 + 3;
	return lcg_prev & 0xffff;
}

static inline short dithered_vol(short sample)
{
	static short rand_a, rand_b;
	long out;

	out = (long)sample *fix_volume;
	if (fix_volume < 0x10000) {
		rand_b = rand_a;
		rand_a = lcg_rand();
		out += rand_a;
		out -= rand_b;
	}
	return out >> 16;
}

static double bf_playback_rate = 1.0;

static short *buffer_get_frame(void)
{
	int16_t buf_fill;
	seq_t read;

	pthread_mutex_lock(&ab_mutex);

	if (ab_buffering)
		goto nodata;

	buf_fill = seq_diff(ab_read, ab_write);
	abuf_t *curframe = audio_buffer + BUFIDX(ab_read);

	/*Avoid insert buffer quickly into Alsa */
	if (buf_fill < 1)
		goto nodata;

	if (buf_fill >= BUFFER_FRAMES) {	// overrunning! uh-oh. restart at a sane distance
		ab_read = ab_write - config.buffer_start_fill;
	}

	read = ab_read;
	ab_read++;
	buf_fill--;

	if (!curframe->ready) {
		air_syslog("missing frame %d. r:%d w:%d", read, ab_read, ab_write);
		goto nodata;
	}
	curframe->ready = 0;

	if (buf_fill < 0) {
		ab_resync(1);
		goto nodata;
	}
	pthread_mutex_unlock(&ab_mutex);

	return curframe->data;

nodata:
	pthread_mutex_unlock(&ab_mutex);
	return 0;
}

static int stuff_buffer(double playback_rate, short *inptr, short *outptr)
{
	int i;
	int stuffsamp = frame_size;
	int stuff = 0;
	double p_stuff;

	p_stuff = 1.0 - pow(1.0 - fabs(playback_rate - 1.0), frame_size);

	if (rand() < p_stuff * RAND_MAX) {
		stuff = playback_rate > 1.0 ? -1 : 1;
		stuffsamp = rand() % (frame_size - 1);
	}

	pthread_mutex_lock(&vol_mutex);
	for (i = 0; i < stuffsamp; i++) {	// the whole frame, if no stuffing
		*outptr++ = dithered_vol(*inptr++);
		*outptr++ = dithered_vol(*inptr++);
	};
	if (stuff) {
		if (stuff == 1) {
			// interpolate one sample
			*outptr++ =
			    dithered_vol(((long)inptr[-2] +
					  (long)inptr[0]) >> 1);
			*outptr++ =
			    dithered_vol(((long)inptr[-1] +
					  (long)inptr[1]) >> 1);
		} else if (stuff == -1) {
			inptr++;
			inptr++;
		}
		for (i = stuffsamp; i < frame_size + stuff; i++) {
			*outptr++ = dithered_vol(*inptr++);
			*outptr++ = dithered_vol(*inptr++);
		}
	}
	pthread_mutex_unlock(&vol_mutex);

	return frame_size + stuff;
}

static void *player_thread_func(void *arg)
{
	int play_samples;
	int buffering = 0;
	int use_silence;

	signed short *inbuf, *outbuf, *silence;
	outbuf = malloc(OUTFRAME_BYTES(frame_size));
	silence = malloc(OUTFRAME_BYTES(frame_size));
	memset(silence, 0, OUTFRAME_BYTES(frame_size));

#ifdef FANCY_RESAMPLING
	float *frame, *outframe;
	SRC_DATA srcdat;
	if (fancy_resampling) {
		frame = malloc(frame_size * 2 * sizeof(float));
		outframe = malloc(2 * frame_size * 2 * sizeof(float));

		srcdat.data_in = frame;
		srcdat.data_out = outframe;
		srcdat.input_frames = FRAME_BYTES(frame_size);
		srcdat.output_frames = 2 * FRAME_BYTES(frame_size);
		srcdat.src_ratio = 1.0;
		srcdat.end_of_input = 0;
	}
#endif

	while (!please_stop) {

		inbuf = buffer_get_frame();

		if (ab_pause)
			config.output->cancel();

		if (!inbuf)
			inbuf = silence;

#ifdef FANCY_RESAMPLING
		if (fancy_resampling) {
			int i;
			pthread_mutex_lock(&vol_mutex);
			for (i = 0; i < 2 * FRAME_BYTES(frame_size); i++) {
				frame[i] = (float)inbuf[i] / 32768.0;
				frame[i] *= volume;
			}
			pthread_mutex_unlock(&vol_mutex);
			srcdat.src_ratio = bf_playback_rate;
			src_process(src, &srcdat);
			assert(srcdat.input_frames_used ==
			       FRAME_BYTES(frame_size));
			src_float_to_short_array(outframe, outbuf,
						 FRAME_BYTES(frame_size) * 2);
			play_samples = srcdat.output_frames_gen;
		} else
#endif
		play_samples = stuff_buffer(bf_playback_rate, inbuf, outbuf);	// 352
		config.output->play(outbuf, play_samples);

	}

	free(outbuf);
	free(silence);

	return;
}

// to compliant with 5-scale each volumeup/down
static double mod5_scale(double f)
{
	int q;
	// from 0 MAX to -30 MIN db
	f = (f + 30) / 30.0;
	q = (f * 100 + 2.5) / 5;
	unsigned volume = q*5;
//	clog(Hred,"vol=%u",volume);
	return (double)(q * 5) / 100.0;
}

// takes the volume as specified by the airplay protocol
void player_volume(double f)
{
	//double linear_volume = pow(10.0, 0.05*f);
	double linear_volume = mod5_scale(f);
	if (linear_volume < 0)
		linear_volume = 0;
	unsigned volume=linear_volume*100;
	set_system_volume(volume);
//	if (config.output->volume) {
//		clog(Hred,"vol=lf",linear_volume);
//		config.output->volume(linear_volume);
//	} else {
//		pthread_mutex_lock(&vol_mutex);
//		volume = linear_volume;
//		fix_volume = 65536.0 * volume;
//		pthread_mutex_unlock(&vol_mutex);
//	}
}

void player_flush(seq_t seqno, uint32_t rtptime)
{
	pthread_mutex_lock(&ab_mutex);
	ab_resync(0);
	pthread_mutex_unlock(&ab_mutex);
}

struct mpd_connection *new_mpd_conn(void)
{
	struct mpd_connection *conn;

	conn = mpd_connection_new("127.0.0.1", 0, 0);
	if (conn == NULL) {
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		mpd_connection_free(conn);
		return NULL;
	}

	return conn;
}

static int ctrl_mpd_stop()
{
	struct mpd_connection *conn;
	conn = new_mpd_conn();

	if (conn) {
		int ret;
		ret = mpd_run_dlna_pause(conn, true);
		//warn("DLNA pause:%s",ret?"OK":"FAIL");
		/*make sure DLNA switch to PAUSE_PLAYBACK */
		//usleep(1500000);
		ret = mpd_run_clear(conn);
		//warn("DLNA clear:%s",ret?"OK":"FAIL");

		mpd_connection_free(conn);
	}
	return 0;
}

int player_play(stream_cfg * stream)
{
    encrypted = stream->encrypted;
    if (config.buffer_start_fill > BUFFER_FRAMES)
        die("specified buffer starting fill %d > buffer size %d",
            config.buffer_start_fill, BUFFER_FRAMES);
    if (encrypted) {
        AES_set_decrypt_key(stream->aeskey, 128, &aes);
        aesiv = stream->aesiv;
    }

    if (stream->rtpmap[1] == 1)
	{
		ab_alac = 1;
        init_decoder(stream->fmtp);
	}
    else
    {
		ab_alac = 0;
        sampling_rate = 44100;
        frame_size = 352;
    }
	// must be after decoder init
	init_buffer();
#ifdef FANCY_RESAMPLING
	init_src();
#endif

	please_stop = 0;

#ifdef MONTAGE_CTRL_PLAYER
	inform_ctrl_msg(MONAIRPLAY, NULL);
#endif

	alsa_off = config.output->start(sampling_rate);
	pthread_create(&player_thread, NULL, player_thread_func, NULL);

	return 0;
}

void player_stop(void)
{
	please_stop = 1;
	pthread_kill(player_thread, SIGUSR1);
	pthread_join(player_thread, NULL);
	config.output->stop();
	if (config.cmd_stop) {
		if (system(config.cmd_stop))
			warn("exec of external stop command failed\n");
	}
	free_buffer();
	free_decoder();
#ifdef FANCY_RESAMPLING
	free_src();
#endif
}

void player_set_timestamp(uint32_t timestamp)
{
	pthread_mutex_lock(&ab_mutex);
	ab_timestamp = timestamp;
	pthread_mutex_unlock(&ab_mutex);

}
