#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>
#include <termios.h>


#include "aconfig.h"
#include "gettext.h"
#include "formats.h"
#include "version.h"

#include "capture.h"

#include "resample_441_to_16.h"

#include "turing.h"
#include "fifobuffer.h"
#include "uuid.h"
#include "myutils/debug.h"
#include "common.h"
#include "init.h"
#ifdef ENABLE_OPUS
#include "opus/opus_audio_encode.h"
#endif

//#define WRITE_FILE


#define	BUFFER_SIZE	4096*2
#define FRAME_LEN	640 
#define HINTS_SIZE  100
#define UPLOAD_FILE_NAME     "/tmp/pcm.raw"



#define POST_MAX_NUMBER 16384
#define CAPURE_TIMEOUT 15

#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif

#ifndef le16toh
#include <asm/byteorder.h>
#define le16toh(x) __le16_to_cpu(x)
#define be16toh(x) __be16_to_cpu(x)
#define le32toh(x) __le32_to_cpu(x)
#define be32toh(x) __be32_to_cpu(x)
#endif


#define DEFAULT_FORMAT		SND_PCM_FORMAT_U8
#define FORMAT_DEFAULT		-1
#define FORMAT_RAW			0
#define FORMAT_WAVE			1

/* global data */
/* global data */
static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);


static snd_pcm_t *handle;
static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams, rhwparams, thwparams;

static int timelimit = 15;
static int quiet_mode = 0;
static int file_type = FORMAT_DEFAULT;
static int open_mode = 0;
static snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

static int mmap_flag = 1;
static int interleaved = 1;
static int in_aborting = 0;
static u_char *audiobuf = NULL;

snd_pcm_uframes_t chunk_size = 0;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static int avail_min = -1;
static int start_delay = 0;
static int stop_delay = 0;

static int fatal_errors = 0;
static long long max_file_size = 0;
static int max_file_time = 0;
static size_t bits_per_sample = 0, bits_per_frame = 0;
static long long chunk_bytes;


static int fd = -1;
static off64_t pbrec_count = LLONG_MAX, fdcount,fdcount1;


static long capture(char *orig_name);

static void begin_wave(int fd, size_t count);
static void end_wave(int fd);
static void set_params(void);
static long capture_iflytek(char *orig_name);


extern GLOBALPRARM_STRU g;

static const struct fmt_capture {
	void (*start) (int fd, size_t count);
	void (*end) (int fd);
	char *what;
	long long max_filesize;
} fmt_rec_table[] = {
	{	NULL,		NULL,		N_("raw data"),		LLONG_MAX },
	/* FIXME: can WAV handle exactly 2GB or less than it? */
	{	begin_wave,	end_wave,	N_("WAVE"),		2147483648LL },
};


/*
 *	Subroutine to clean up before exit.
 */
static void prg_exit(int code) 
{

	if (handle)
		snd_pcm_close(handle);
	handle = NULL;
	if(audiobuf)
		free(audiobuf);
	audiobuf = NULL;
	exit(code);
}



static int captureState = CAPTURE_NONE;
static int captureEnd = 0;
static int interrupt  = 0;           //现在录音和上传进程是否处于被打断状态（按键打断、微信端打断）

static pthread_mutex_t captureMtx = PTHREAD_MUTEX_INITIALIZER;


void SetCaptureEnd(int end)
{

	pthread_mutex_lock(&captureMtx);
	captureEnd = end;
	pthread_mutex_unlock(&captureMtx);

}
int IsCaptureEnd() 
{	
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureEnd == 1);
	captureEnd = 0;
	pthread_mutex_unlock(&captureMtx);
	return ret;
}
int IsCaptureCancled()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_CANCLE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}

void SetCaptureState(int state)
{
	pthread_mutex_lock(&captureMtx);
	captureState = state;
	pthread_mutex_unlock(&captureMtx);
}
int IsCaptureFinshed()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_NONE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}

int IsCaptureDone()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_DONE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}
int IsCaptureRunning()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_ONGING);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}

int IsInterrupt()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (interrupt  == 1);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}

void SetInterruptState(int state)
{
	pthread_mutex_lock(&captureMtx);
	interrupt = state;
	pthread_mutex_unlock(&captureMtx);
}


int capture_voice(void)
{
		int errno;
		char *pcm_name = NULL;
		int  err;
		int times = 0;
		long  ret =	-1;

		SetVADFlag(RECORD_VAD_START);
		//pcm_name = "default";
		//pcm_name = "plughw:0,0";
		pcm_name = "default";
		debug("pcm_name:%s", pcm_name);

		stream = SND_PCM_STREAM_CAPTURE;
		file_type = FORMAT_WAVE;
		start_delay = 1;
		chunk_size = -1;
		rhwparams.format = SND_PCM_FORMAT_S16_LE;
	

#ifndef ENABLE_CODEC_ES8316
		rhwparams.rate = 44100; 	
		rhwparams.channels = 2;
#else
		rhwparams.rate = 16000;		
		rhwparams.channels = 1;
#endif

		thwparams.rate = 16000;
		thwparams.channels = 1;
		thwparams.format = SND_PCM_FORMAT_S16_LE;
		
		err = snd_pcm_open(&handle, pcm_name,stream, open_mode);
		if (err < 0) {
			error(_("audio open error: %s"), snd_strerror(err));
			return 1;
		}
		chunk_size = 1024;
	
		hwparams = rhwparams;
	
		audiobuf = (u_char *)malloc(1024);
		if (audiobuf == NULL) {
			error(_("not enough memory"));
			return 1;
		}

		if (mmap_flag) {
			readi_func = snd_pcm_mmap_readi;		
			writei_func = snd_pcm_mmap_writei;
	
		} else {
			readi_func = snd_pcm_readi;		
			writei_func = snd_pcm_writei;
		}
		set_params();
		CaptureFifoClear();
		ret = capture("/tmp/snd.wav");
		error("capture len %d bytes", ret);
exit:
		if(handle) {
			snd_pcm_drain(handle); 
			snd_pcm_close(handle);
		}	
		handle  = NULL;
		if(audiobuf)
			free(audiobuf);
		audiobuf =NULL;

		error("CAPTURE_DONE...");
		return ret;
}

static void show_available_sample_formats(snd_pcm_hw_params_t* params)
{
	snd_pcm_format_t format;

	fprintf(stderr, "Available formats:\n");
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
			fprintf(stderr, "- %s\n", snd_pcm_format_name(format));
	}
}
static unsigned int g_chunk_length;

static void set_params(void)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		error("Broken configuration for this PCM: no configurations available");
		prg_exit(EXIT_FAILURE);
	}

	if (mmap_flag) {
		snd_pcm_access_mask_t *mask = alloca(snd_pcm_access_mask_sizeof());
		snd_pcm_access_mask_none(mask);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
		err = snd_pcm_hw_params_set_access_mask(handle, params, mask);
	} else if (interleaved)
		err = snd_pcm_hw_params_set_access(handle, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	else
		err = snd_pcm_hw_params_set_access(handle, params,
						   SND_PCM_ACCESS_RW_NONINTERLEAVED);
	if (err < 0) {
		error("Access type not available");
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (err < 0) {
		error("Sample format non available");
		show_available_sample_formats(params);
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	if (err < 0) {
		error("Channels count non available");
		prg_exit(EXIT_FAILURE);
	}


	rate = hwparams.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < hwparams.rate || (float)rate * 0.95 > hwparams.rate) {
		if (!quiet_mode) {
			char plugex[64];
			const char *pcmname = snd_pcm_name(handle);
			warning("Warning: rate is not accurate (requested = %iHz, got = %iHz)", rate, hwparams.rate);
			if (! pcmname || strchr(snd_pcm_name(handle), ':'))
				*plugex = 0;
			else
				snprintf(plugex, sizeof(plugex), "(-Dplug:%s)",
					 snd_pcm_name(handle));
				error("please, try the plug plugin %s\n",plugex);
		}
	}
	rate = hwparams.rate;
	if (buffer_time == 0 && buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
							    &buffer_time, 0);
		assert(err >= 0);
		if (buffer_time > 500000)
			buffer_time = 500000;
	}
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
	}
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle, params,
							     &period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle, params,
							     &period_frames, 0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
							     &buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
							     &buffer_frames);
	}

	assert(err >= 0);

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		error("Unable to install hw params:");
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);

	if (chunk_size == buffer_size) {
		error("Can't use period equal to buffer size (%lu == %lu)", chunk_size, buffer_size);
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_sw_params_current(handle, swparams);
	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) rate * avail_min / 1000000;
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);

	/* round up to closest transfer boundary */
	n = buffer_size;
	if (start_delay <= 0) {
		start_threshold = n + (double) rate * start_delay / 1000000;
	} else
		start_threshold = (double) rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
	assert(err >= 0);
	if (stop_delay <= 0) 
		stop_threshold = buffer_size + (double) rate * stop_delay / 1000000;
	else
		stop_threshold = (double) rate * stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		error("unable to install sw params:");
		prg_exit(EXIT_FAILURE);
	}
	info("chunk_size:%ld",chunk_size);
	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
    info("bits_per_sample = %d,hwparams.channels=%d",bits_per_sample,hwparams.channels);

	bits_per_frame = bits_per_sample * hwparams.channels;
    info("bits_per_frame = %d,bits_per_sample=%d",bits_per_frame,bits_per_sample);
    
	chunk_bytes = chunk_size * bits_per_frame / 8;
    info("chunk_bytes=%lld,bits_per_frame:%d",chunk_bytes,bits_per_frame);
    info("bits_per_frame=%d,chunk_bytes:%lld",bits_per_frame,chunk_bytes);

    
	g_chunk_length = 5 * hwparams.channels * 16 / 8 * hwparams.rate;

    info("bits_per_frame=%d,chunk_bytes:%lld",bits_per_frame,chunk_bytes);
	audiobuf = realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		error("not enough memory");
		prg_exit(EXIT_FAILURE);
	}

}

/* I/O error handler */
static void xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		error("status error: %s", snd_strerror(res));
		prg_exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		if (fatal_errors) {
			error("fatal %s: %s",
					stream == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",
					snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		if ((res = snd_pcm_prepare(handle))<0) {
			error("xrun: prepare error: %s", snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {

		if (stream == SND_PCM_STREAM_CAPTURE) {
			warning("capture stream format change? attempting recover...");
			if ((res = snd_pcm_prepare(handle))<0) {
				error("xrun(DRAINING): prepare error: %s", snd_strerror(res));
				prg_exit(EXIT_FAILURE);
			}
			return;
		}
	}

	error("read/write error, state = %s", snd_pcm_state_name(snd_pcm_status_get_state(status)));
	prg_exit(EXIT_FAILURE);
}

/* I/O suspend handler */
static void suspend(void)
{
	int res;

	while ((res = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		if ((res = snd_pcm_prepare(handle)) < 0) {
			error("suspend: prepare error: %s", snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
	}
}


/*
 *  read function
 */

static ssize_t pcm_read(u_char *data, size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;

	if (count != chunk_size) {
		count = chunk_size;
	}

	while (count > 0 && !in_aborting) {
		r = readi_func(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
				snd_pcm_wait(handle, 100);
		} else if (r == -EPIPE) {
			xrun();
		} else if (r == -ESTRPIPE) {
			suspend();
		} else if (r < 0) {
			error("read error: %s", snd_strerror(r));
			prg_exit(EXIT_FAILURE);
		}
		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return result;
}

/*
 * Safe read (for pipes)
 */
 
static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0 ) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}


/* that was a big one, perhaps somebody split it :-) */

/* setting the globals for playing raw data */
static void init_raw_data(void)
{
	hwparams = rhwparams;
}

/* calculate the data count to read from/to dsp */
static off64_t calc_count()
{
	off64_t count;

	if (timelimit == 0)
    {
		count = pbrec_count;
	} 
    else 
	{
		count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
		count *= (off64_t)timelimit;
	}

	return count < pbrec_count ? count : pbrec_count;
}



/* write a WAVE-header */
static void begin_wave(int fd, size_t cnt)
{
	WaveHeader h;
	WaveFmtBody f;
	WaveChunkHeader cf, cd;
	int bits;
	u_int tmp;
	u_short tmp2;

	/* WAVE cannot handle greater than 32bit (signed?) int */
	if (cnt == (size_t)-2)
		cnt = 0x7fffff00;

	bits = 8;
	switch ((unsigned long) hwparams.format) {
	case SND_PCM_FORMAT_U8:
		bits = 8;
		break;
	case SND_PCM_FORMAT_S16_LE:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S32_LE:
        case SND_PCM_FORMAT_FLOAT_LE:
		bits = 32;
		break;
	case SND_PCM_FORMAT_S24_LE:
	case SND_PCM_FORMAT_S24_3LE:
		bits = 24;
		break;
	default:
		error("Wave doesn't support %s format...", snd_pcm_format_name(hwparams.format));
		prg_exit(EXIT_FAILURE);
	}
	h.magic = WAV_RIFF;
	tmp = cnt + sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + sizeof(WaveChunkHeader) - 8;
	h.length = LE_INT(tmp);
	h.type = WAV_WAVE;

	cf.type = WAV_FMT;
	cf.length = LE_INT(16);

        if (hwparams.format == SND_PCM_FORMAT_FLOAT_LE)
                f.format = LE_SHORT(WAV_FMT_IEEE_FLOAT);
        else
                f.format = LE_SHORT(WAV_FMT_PCM);
	f.channels = LE_SHORT(hwparams.channels);
	f.sample_fq = LE_INT(hwparams.rate);
#if 0
	tmp2 = (samplesize == 8) ? 1 : 2;
	f.byte_p_spl = LE_SHORT(tmp2);
	tmp = dsp_speed * hwparams.channels * (u_int) tmp2;
#else
	tmp2 = hwparams.channels * snd_pcm_format_physical_width(hwparams.format) / 8;
	f.byte_p_spl = LE_SHORT(tmp2);
	tmp = (u_int) tmp2 * hwparams.rate;
#endif
	f.byte_p_sec = LE_INT(tmp);
	f.bit_p_spl = LE_SHORT(bits);

	cd.type = WAV_DATA;
	cd.length = LE_INT(cnt);

	if (write(fd, &h, sizeof(WaveHeader)) != sizeof(WaveHeader) ||
	    write(fd, &cf, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader) ||
	    write(fd, &f, sizeof(WaveFmtBody)) != sizeof(WaveFmtBody) ||
	    write(fd, &cd, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader)) {
		error("write error");
		prg_exit(EXIT_FAILURE);
	}
}
#ifdef WRITE_FILE 
/* write a WAVE-header */
static void begin_wave1(int fd, size_t cnt)
{
	WaveHeader h;
	WaveFmtBody f;
	WaveChunkHeader cf, cd;
	int bits;
	u_int tmp;
	u_short tmp2;

	/* WAVE cannot handle greater than 32bit (signed?) int */
	if (cnt == (size_t)-2)
		cnt = 0x7fffff00;

	bits = 8;
	switch ((unsigned long) thwparams.format) {
	case SND_PCM_FORMAT_U8:
		bits = 8;
		break;
	case SND_PCM_FORMAT_S16_LE:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S32_LE:
        case SND_PCM_FORMAT_FLOAT_LE:
		bits = 32;
		break;
	case SND_PCM_FORMAT_S24_LE:
	case SND_PCM_FORMAT_S24_3LE:
		bits = 24;
		break;
	default:
		error("Wave doesn't support %s format...", snd_pcm_format_name(thwparams.format));
		prg_exit(EXIT_FAILURE);
	}
	h.magic = WAV_RIFF;
	tmp = cnt + sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + sizeof(WaveChunkHeader) - 8;
	h.length = LE_INT(tmp);
	h.type = WAV_WAVE;

	cf.type = WAV_FMT;
	cf.length = LE_INT(16);

        if (thwparams.format == SND_PCM_FORMAT_FLOAT_LE)
                f.format = LE_SHORT(WAV_FMT_IEEE_FLOAT);
        else
                f.format = LE_SHORT(WAV_FMT_PCM);
	f.channels = LE_SHORT(thwparams.channels);
	f.sample_fq = LE_INT(thwparams.rate);
#if 0
	tmp2 = (samplesize == 8) ? 1 : 2;
	f.byte_p_spl = LE_SHORT(tmp2);
	tmp = dsp_speed * thwparams.channels * (u_int) tmp2;
#else
	tmp2 = thwparams.channels * snd_pcm_format_physical_width(thwparams.format) / 8;
	f.byte_p_spl = LE_SHORT(tmp2);
	tmp = (u_int) tmp2 * thwparams.rate;
#endif
	f.byte_p_sec = LE_INT(tmp);
	f.bit_p_spl = LE_SHORT(bits);

	cd.type = WAV_DATA;
	cd.length = LE_INT(cnt);

	if (write(fd, &h, sizeof(WaveHeader)) != sizeof(WaveHeader) ||
	    write(fd, &cf, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader) ||
	    write(fd, &f, sizeof(WaveFmtBody)) != sizeof(WaveFmtBody) ||
	    write(fd, &cd, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader)) {
		error("write error");
		prg_exit(EXIT_FAILURE);
	}
}

static void end_wave1(int fd)
{				/* only close output */
	WaveChunkHeader cd;
	off64_t length_seek;
	off64_t filelen;
	u_int rifflen;
	
	length_seek = sizeof(WaveHeader) +
		      sizeof(WaveChunkHeader) +
		      sizeof(WaveFmtBody);
	cd.type = WAV_DATA;
	cd.length = fdcount1 > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(fdcount1);
	filelen = fdcount1 + 2*sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + 4;
	rifflen = filelen > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(filelen);
	if (lseek64(fd, 4, SEEK_SET) == 4)
		write(fd, &rifflen, 4);
	if (lseek64(fd, length_seek, SEEK_SET) == length_seek)
		write(fd, &cd, sizeof(WaveChunkHeader));
	if (fd != 1)
		close(fd);
}

#endif

static void end_wave(int fd)
{				/* only close output */
	WaveChunkHeader cd;
	off64_t length_seek;
	off64_t filelen;
	u_int rifflen;
	
	length_seek = sizeof(WaveHeader) +
		      sizeof(WaveChunkHeader) +
		      sizeof(WaveFmtBody);
	cd.type = WAV_DATA;
	cd.length = fdcount > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(fdcount);
	filelen = fdcount + 2*sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + 4;
	rifflen = filelen > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(filelen);
	if (lseek64(fd, 4, SEEK_SET) == 4)
		write(fd, &rifflen, 4);
	if (lseek64(fd, length_seek, SEEK_SET) == length_seek)
		write(fd, &cd, sizeof(WaveChunkHeader));
	if (fd != 1)
		close(fd);
}


static int safe_open(const char *name)
{
	int fd;
	
	if(access(name,0) == 0){
		info("remove %s",name);
		remove(name);
	}
	
	fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		if (errno != ENOENT )
			return -1;
	}
	return fd;
}

#define BIG_ENDIAN 0
static int vad(short samples[], int len)
{
  int i;
#if BIG_ENDIAN
  unsigned char *p;
  unsigned char temp;
#endif
  long long sum = 0, sum2 = 0;
  for(i = 0; i < len; i++) {
#if BIG_ENDIAN
	p = &samples[i];
	temp = p[0];
	p[0] = p[1];
	p[1] = temp;
#endif
	sum += (samples[i] * samples[i]);
	//sum2 += (1000000);
	sum2 += g.m_iVadThreshold;
  }
  if (sum > sum2)
	  return 1;
  else
	  return 0;
}
static int vad_voice(short samples[], int len, int *notVoice)
{
  int i;
#if BIG_ENDIAN
  unsigned char *p;
  unsigned char temp;
#endif
  long long sum = 0, sum2 = 0, sum3 = 0;
  for(i = 0; i < len; i++) {
#if BIG_ENDIAN
	p = &samples[i];
	temp = p[0];
	p[0] = p[1];
	p[1] = temp;
#endif
	sum += (samples[i] * samples[i]);
	//sum2 += (1000000);
	sum2 += g.m_iVadThreshold;
	sum3 += g.m_iVadThreshold * 3;
  }
  if(sum > sum3) 
  	*notVoice = 1;
  else
  	*notVoice = 0;

  info("sum = %lld,sum2 = %lld",sum,sum2);
  if (sum > sum2) 
	  return 1;
  else
	  return 0;
}


extern void PrintSysTime(char *pSrc);

static char byVADCont = 32;

int SetVadCount(char vadCont)
{
	byVADCont = vadCont;
}
int GetVadCount()
{
	return byVADCont;
}

char GetVADFlag(void)
{
	char byRet = 1;

	//pthread_mutex_lock(&g.vadFlag.VAD_mtx);
	
	byRet = g.vadFlag;
	
	//pthread_mutex_unlock(&g.vadFlag.VAD_mtx);

	return byRet;
}

void SetVADFlag(char byFlag)
{
	//pthread_mutex_lock(&g.vadFlag.VAD_mtx);
	
	g.vadFlag = byFlag;
	
	//pthread_mutex_unlock(&g.vadFlag.VAD_mtx);
}



static long capture(char *orig_name)
{
	int fd;
	unsigned int total_len = 0; 
	long pcm_count = 0;
	int iCount = 0;
	char byVadEndFlag = 0;
	off64_t count, rest;        /* number of bytes to capture */

	/* get number of bytes to capture */
	count = calc_count();   
	if (count == 0)
		count = LLONG_MAX;

	/* compute the number of bytes per file */
	max_file_size = max_file_time * snd_pcm_format_size(hwparams.format,hwparams.rate * hwparams.channels);
    //printf("hwparams.rate = %d,hwparams.channels = %d,max_file_time = %d\n",hwparams.rate ,hwparams.channels,max_file_time);
    
    //printf("count = %lld,max_file_size = %lld,file_type = %d\n",count,max_file_size,file_type);
	/* WAVE-file should be even (I'm not sure), but wasting one byte
	   isn't a problem (this can only be in 8 bit mono) */
	if (count < LLONG_MAX)
		count += count % 2;
	else
		count -= count % 2;
		
	rest = count;
	if (rest > fmt_rec_table[file_type].max_filesize)
		rest = fmt_rec_table[file_type].max_filesize;
	if (max_file_size && (rest > max_file_size)) 
		rest = max_file_size;

    printf("count = %lld,rest = %lld,max_file_size = %lld\n",count,rest,max_file_size);

#ifdef WRITE_FILE
	void *wav;
#ifdef CAPTURE_PCM_8K_16BIT
	wav = wav_write_open("/tmp/pcm.wav", 8000, 16, 1);
#endif
#ifdef CAPTURE_PCM_16K_16BIT
	wav = wav_write_open("/tmp/pcm.wav", 16000, 16, 1);
#endif

	if (!wav) 
    {
		error("Unable to open wav file");
		prg_exit(EXIT_FAILURE);
	}

#endif
	int   isVoice = 0;
	char byFlag = 0;
	char byVadFlag = 0;
	char byStart = 0;
	char byVAD_StartFlag = 1;

	char *pAudioBuffer = NULL;
	unsigned long wAudioLen = 0;
	unsigned long notVoiceLen = 0;
	unsigned long perSecCount = rhwparams.rate*rhwparams.channels*2;
	unsigned long notVoicethreshold = 5 * perSecCount ;
	char notVoiceCount = 0;
	int uploadPlay = 0, uploadPlayCount = 0;
	unsigned long uploadPlayLen = 0;
    unsigned long intervalLen = 0;

	warning("Start Capture ");
	debug("notVoicethreshold:%lu", notVoicethreshold);
	getuuidString(g.identify);
	debug("g.identify:%s",g.identify);
	g.m_CaptureFlag = 1;

    int type = GetTuringType();  //2:不使用VAD
    //chunk_bytes = 2560 * 2 = 5120
    size_t c = (rest <= (off64_t)chunk_bytes) ? (size_t)rest : chunk_bytes;
    size_t f = c * 8 / bits_per_frame;
    
    warning("rest = %lld,chunk_bytes = %lld,bits_per_frame = %d,c=%d,f=%d",rest,chunk_bytes,bits_per_frame,c,f);

    SetVADFlag(RECORD_VAD_START);
	while (rest > 0) //480000
	{
		int ret;
		if(IsCaptureCancled()) {
			warning("Cancle Capture... ");
            SetCaptureState(CAPTURE_NONE);  //设置录音结束标志
            SetHttpRequestCancle();         //停止上传录音数据
			CaptureFifoClear();
			break;
		}
		
		g.m_CaptureFlag = 2;   //正在录音
		// VAD判断
		if (RECORD_VAD_END == GetVADFlag())   //VAD结束
		{
			error("RECORD_VAD_END");
			break;
		}
		
		if(rest <= 0 ) {
			error("time out...");
			break;
		}

		if (pcm_read(audiobuf, f) != f)    //f = 2560
        {
			error("pcm_read");
			break;
		}
        // c = 5120;
		count -= c;
		rest -= c;
		fdcount += c ;
		fdcount1 += c ;

        if(IsCaptureCancled()) 
        {
            warning("Cancle Capture... ");
            SetCaptureState(CAPTURE_NONE);  //设置录音结束标志
            SetHttpRequestCancle();         //停止上传录音数据
            CaptureFifoClear();
            break;
        }

        //clear_garbage_data(c);

		char *tmp = NULL;
		unsigned long len = 0;
		unsigned int fifoPutLen = 0;

		pAudioBuffer = audiobuf;
		wAudioLen = c;
#ifndef ENABLE_CODEC_ES8316	
		tmp = malloc(c*160/441/2);

#ifdef CAPTURE_PCM_16K_16BIT
		resample_buf_441_to_16(audiobuf, c, tmp, &len);
#endif
#ifdef CAPTURE_PCM_8K_16BIT
		resample_buf_441_to_8k(audiobuf, c, tmp, &len);
#endif
		pAudioBuffer = tmp;
		wAudioLen = len;
#endif

#ifdef WRITE_FILE
		wav_write_data(wav, pAudioBuffer, wAudioLen);
#endif
		CaptureFifoPut(pAudioBuffer, wAudioLen);
        if(byFlag == 0)
        {
            byFlag = 1;
            NewHttpRequest();
            info("Do it just one time");
        }
        
#ifdef ENABLE_YIYA
        //本意：开始录音后，3秒钟未监听到声音则停止本次语音识别
        //结果：只录3秒钟而不管是否有人声
		total_len += c;        
		if(type !=2 && total_len > perSecCount * 3)   // > 16000*2 *3
        {
            info("total len = %d ,too big",total_len);
			SetVADFlag(RECORD_VAD_END);
		}
#endif

#ifndef ENABLE_HUABEI		
		if ((type != 2) && (0 == byVadEndFlag) )
		{
			if (0 == byStart)
			{
				// vad返回值为1时为is voice
				info("0 == byStart");
				//if (1 == vad(pAudioBuffer, wAudioLen/2))
				if (1 == vad_voice(pAudioBuffer, wAudioLen/2, &uploadPlay))
				{
					info("voice data -----");
					byStart = 1;
					isVoice = 1;
				} 
				else 
				{
				    info("silence data +++++");
					isVoice =  0;
				}
			}
			else 
			{
				if (0 == vad_voice(pAudioBuffer, wAudioLen/2, &uploadPlay))
				{
			    debug("silence data +++++");
				isVoice =  0;
                    intervalLen += c;
                    if(intervalLen > perSecCount/2)   //说话后，两个字间隔大于1/3秒，认为结束
                    {
                        info("Don't you want to say any more ?");
                        SetCaptureState(CAPTURE_NONE);  //让上传PCM数据的线程早点获取结束录音的消息以便尽快将剩余数据上传完毕
                        SetVADFlag(RECORD_VAD_END);
                        goto END;
                    }
				}
				else
				{
				debug("voice data ------ ");
					isVoice = 1;
                    intervalLen = 0;
				}
				
			}
#ifdef ENABLE_YIYA	
			/*
			 * uploadPlay 和	 isVoice的阀值不一样
			 * uploadPlayLen判断时候播放“发送成功”
			 */
			if(uploadPlay == 0)
		    { 
				uploadPlayLen  += (unsigned long)c;
				//info("uploadPlayLen:%ld=====", uploadPlayLen);
				uploadPlayCount++;
			}
			else {
				uploadPlayCount = 0;
				uploadPlayLen 	= 0;
			}
#endif
			if(isVoice == 0) 
            {
				notVoiceLen += (unsigned long)c;
                //按下按键，静音超过5秒，就停止识别
                if(notVoiceLen >  notVoicethreshold)  // > 64000
                {
#ifdef ENABLE_YIYA
			/* 连续notVoicethreshold/perSecCount秒vad为0,则结束本次结束 */
				if(uploadPlayLen > notVoicethreshold)
					SetNotPlay(1);
#endif
                    notVoiceLen = 0;
                    info("Are you  dumb ?");
                    SetVADFlag(RECORD_VAD_END);
                    SetCaptureState(CAPTURE_NONE);
                    goto END;
                }
			} 
            else 
            {
				notVoiceLen = 0;
			}
			
		}
#endif
#ifndef ENABLE_CODEC_ES8316	
		if (tmp)
			free(tmp);
#endif
END:
		pcm_count += (long)wAudioLen;
        
#if defined(USE_UPLOAD_AMR) || defined(ENABLE_OPUS)
			if(pcm_count > CAPTURE_SIZE)
				StartNewEncode();
#else

#endif


	}
	//SetCaptureState(CAPTURE_DONE);
	
#ifdef WRITE_FILE
	wav_write_close(wav);
#endif
	g.m_CaptureFlag = 3;
	warning("End Capture");
	warning("pcm_count :%ld", pcm_count);
	return pcm_count;
}
#if 1

static int chatState = CHAT_NONE;
static pthread_mutex_t chatMtx = PTHREAD_MUTEX_INITIALIZER;


int IsChatCancled()
{
	int ret;
	pthread_mutex_lock(&chatMtx);
	ret = (chatState == CHAT_CANCLE);
	pthread_mutex_unlock(&chatMtx);
	return ret;
}

void SetChatState(int state)
{
	pthread_mutex_lock(&chatMtx);
	chatState = state;
	pthread_mutex_unlock(&chatMtx);
}
int IsChatFinshed()
{
	int ret;
	pthread_mutex_lock(&chatMtx);
	ret = (chatState == CHAT_NONE);
	pthread_mutex_unlock(&chatMtx);
	return ret;
}
int IsChatDone()
{
	int ret;
	pthread_mutex_lock(&chatMtx);
	ret = (chatState == CHAT_DONE);
	pthread_mutex_unlock(&chatMtx);
	return ret;
}
int IsChatRunning()
{
	int ret;
	pthread_mutex_lock(&chatMtx);
	ret = (chatState == CHAT_ONGING);
	pthread_mutex_unlock(&chatMtx);
	return ret;
}

#define      WRITE_CHAT_FILE

static long capture_chat(char *orig_name)
{
	struct VoiceData *p1, *p2;
	void *wav;
	
	int fd1, fd2;

	unsigned int	total_len					=	0; 
	long			pcm_count					=	0;

	static int iCount = 0;

	char byVadEndFlag = 0;

	off64_t count, rest;		/* number of bytes to capture */

	/* get number of bytes to capture */
	count = calc_count();
	if (count == 0)
		count = LLONG_MAX;

	/* compute the number of bytes per file */
	max_file_size = max_file_time *
		snd_pcm_format_size(hwparams.format,
				    hwparams.rate * hwparams.channels);
	rest =  count;
	/* WAVE-file should be even (I'm not sure), but wasting one byte
	   isn't a problem (this can only be in 8 bit mono) */
	if (count < LLONG_MAX)
		count += count % 2;
	else
		count -= count % 2;
	
	if (rest > fmt_rec_table[file_type].max_filesize)
		rest = fmt_rec_table[file_type].max_filesize;
	if (max_file_size && (rest > max_file_size)) 
		rest = max_file_size;	


#ifdef WRITE_CHAT_FILE
	void *wav1 = NULL;
	wav1 = wav_write_open("/tmp/44k.wav", 44100, 16, 2);
	if (!wav1) {
		error("Unable to open /tmp/44k.wav");
		prg_exit(EXIT_FAILURE);
	}
#endif

	unlink(orig_name);
	wav = wav_write_open(orig_name, 8000, 16, 1);
	if (!wav) {
		error("Unable to open %s",orig_name);
		prg_exit(EXIT_FAILURE);
	}
	char byFlag = 0;
	char byVadFlag = 0;
	char byStart = 0;
	char byVAD_StartFlag = 0;


	char *pAudioBuffer = NULL;
	unsigned long wAudioLen = 0;
	
	SetVADFlag(RECORD_VAD_START);
	info("Start Capture");

	while (rest >0 ) 
	{

		if(IsChatCancled()) {
			warning("Cancle Capture...");
			break;
		}
		SetChatState(CHAT_ONGING);
		

		if (RECORD_VAD_END == GetVADFlag())
		{
			if (0 == byFlag)
			{
				byFlag = 1;
			}
			iCount = 0;
			break;
		}
		
		if(1 == GetWavPlay()) {
			usleep(200);
			continue;
		}
		int ret;
		if(rest <= 0 ) {
			
			break;
		}
		size_t c = (rest <= (off64_t)chunk_bytes) ?
			(size_t)rest : chunk_bytes;

		size_t f = c * 8 / bits_per_frame;

		if (pcm_read(audiobuf, f) != f)
			break;


		count -= c;
		rest -= c;
		fdcount += c ;
		fdcount1 += c ;
		char *tmp = NULL;
		unsigned long len = 0;
		pAudioBuffer = audiobuf;
		wAudioLen = c;
		
#ifndef ENABLE_CODEC_ES8316

		tmp= malloc(c*160/441/2);
		
		resample_buf_441_to_8k(audiobuf, c, tmp, &len);
#ifdef WRITE_CHAT_FILE
		wav_write_data(wav1, audiobuf, c);	
#endif
		pAudioBuffer = tmp;
		wAudioLen = len;
#endif
#ifdef ENABLE_YIYA
		//pcm_volume_filter(pAudioBuffer, wAudioLen * 8 / bits_per_frame, 1, 16, SND_PCM_FORMAT_S16_LE);
#endif

		wav_write_data(wav, pAudioBuffer, wAudioLen);	
#ifndef ENABLE_YIYA
		if ((0 == byVadEndFlag) && (1 == byVAD_StartFlag))
		{
			if (0 == byStart)
			{
				if (1 == vad(pAudioBuffer, wAudioLen/2))
				{
				
					byStart = 1;
				}
			}
			else if (0 == vad(pAudioBuffer, wAudioLen/2))
			{
				iCount++;
				
				if (byVADCont <= iCount)
				{
					if (0 == byVadFlag)
					{
						byVadFlag = 1;
						iCount = 0;

						byVadEndFlag = 1; // 此次本地VAD结束，任务完成
						byVAD_StartFlag = 0;
				
						SetVADFlag(RECORD_VAD_END);
					}
				}
			}
			else
			{
				iCount = 0;
			}
		}
#endif
#ifndef ENABLE_CODEC_ES8316			
		if (tmp) {
			free(tmp);
			tmp = NULL;
		}
#endif
		pcm_count += (long)wAudioLen;
		if (pcm_count >= 8192)
		{
			byVAD_StartFlag = 1;
		}
	}
	//exec_cmd("uartdfifo.sh tlkoff");
	wav_write_close(wav);
	
#ifdef WRITE_CHAT_FILE
	wav_write_close(wav1);
#endif

	warning("Capture End");
	warning("chatState:%d",chatState);
	
	return pcm_count;
}




int capture_turing_chat(char *name)
{
		int errno;
		char *pcm_name = NULL;
		int  err;
		int times = 0;
		long  ret =	-1;

		SetVADFlag(RECORD_VAD_START);

		pcm_name = "default";
	
		info("pcm_name:%s ", pcm_name);

		stream = SND_PCM_STREAM_CAPTURE;

		file_type = FORMAT_WAVE;
	
		start_delay = 1;
	
		chunk_size = -1;
	
		rhwparams.format = SND_PCM_FORMAT_S16_LE;
	
#ifndef ENABLE_CODEC_ES8316
		rhwparams.rate = 44100;		
		rhwparams.channels = 2;
#else
		rhwparams.channels = 1;
		rhwparams.rate = 8000;		
#endif
	
		thwparams.rate = 16000;
		thwparams.channels = 1;
		thwparams.format = SND_PCM_FORMAT_S16_LE;
		err = snd_pcm_open(&handle, pcm_name,stream, open_mode);
		if (err < 0) {
			error(_("audio open error: %s"), snd_strerror(err));
			return 1;
		}
		chunk_size = 1024;
	
		hwparams = rhwparams;
	
		audiobuf = (u_char *)malloc(1024);
		if (audiobuf == NULL) {
			error(_("not enough memory"));
			return 1;
		}

		if (mmap_flag) {
		
			readi_func = snd_pcm_mmap_readi;		
			writei_func = snd_pcm_mmap_writei;
	
		} else {
			readi_func = snd_pcm_readi;		
			writei_func = snd_pcm_writei;
	
		}

		set_params();

		ret = capture_chat(name);

exit:
		if(handle){
			snd_pcm_drain(handle); 
			snd_pcm_close(handle);
		}	
		handle  = NULL;
		if(audiobuf)
			free(audiobuf);
		audiobuf =NULL;	
		return ret;
}
#endif





