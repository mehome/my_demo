
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

extern 	char g_byWakeUpFlag;

extern int g_iVadThreshold;

//#define  APLAY_BUF
//#define  ARECORD_TEST

//#define WRITE_FILE

#define	BUFFER_SIZE	4096*2
#define FRAME_LEN	640 
#define HINTS_SIZE  100
#define UPLOAD_FILE_NAME     "/www/music/vad16k.raw"


//#define POST_MAX_NUMBER 16384*4
#define POST_MAX_NUMBER 16384*2
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
#define FORMAT_RAW		0
#define FORMAT_WAVE		1

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

static int timelimit = 0;
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
static size_t bits_per_sample, bits_per_frame;
static long long chunk_bytes;

//static snd_output_t *log;

static int fd = -1;
static off64_t pbrec_count = LLONG_MAX, fdcount,fdcount1;


static long capture(char *orig_name, char *pVADValue);

static void begin_wave(int fd, size_t count);
static void end_wave(int fd);
static void set_params(void);

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

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define error(...) do {\
	fprintf(stderr, "%s:%d: ", __FUNCTION__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	putc('\n', stderr); \
} while (0)
#else
#define error(args...) do {\
	fprintf(stderr, "%s:%d: ", __FUNCTION__, __LINE__); \
	fprintf(stderr, ##args); \
	putc('\n', stderr); \
} while (0)
#endif	



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

int capture_voice(char *pVADValue)
{
	int errno;
	char *pcm_name = NULL;
	int  err;
	int times = 0;
	long  ret =	-1;

	if (1 == g_byWakeUpFlag)
	{
		pcm_name = "plughw:1,0";
	}
	else
	{
		pcm_name = "default";
	}
	DEBUG_PRINTF("pcm_name:%s", pcm_name);

	stream = SND_PCM_STREAM_CAPTURE;

	file_type = FORMAT_WAVE;

	start_delay = 1;

	chunk_size = -1;

	rhwparams.format = SND_PCM_FORMAT_S16_LE;

	if (1 == g_byWakeUpFlag)
	{
		rhwparams.rate = 16000;		
		rhwparams.channels = 1;
	}
	else
	{
		rhwparams.rate = 44100;		
		rhwparams.channels = 2;
	}

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
		DEBUG_PRINTF("snd_pcm_mmap_readi\n");
		readi_func = snd_pcm_mmap_readi;		
		writei_func = snd_pcm_mmap_writei;

	} else {
		DEBUG_PRINTF("snd_pcm_readi\n");
		readi_func = snd_pcm_readi;		
		writei_func = snd_pcm_writei;
	}

	set_params();

	ret = capture("/tmp/snd.wav", pVADValue);
	DEBUG_PRINTF("ret=%d",ret);

exit:

	if(handle){
		snd_pcm_drain(handle); 
		snd_pcm_close(handle);
	}	
	handle  = NULL;
	if(audiobuf)
		free(audiobuf);
	audiobuf =NULL;

	DEBUG_PRINTF("capture_voice exit ...");
	
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
		error(_("Broken configuration for this PCM: no configurations available"));
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
		error(_("Access type not available"));
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (err < 0) {
		error(_("Sample format non available"));
		show_available_sample_formats(params);
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	if (err < 0) {
		error(_("Channels count non available"));
		prg_exit(EXIT_FAILURE);
	}


	rate = hwparams.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < hwparams.rate || (float)rate * 0.95 > hwparams.rate) {
		if (!quiet_mode) {
			char plugex[64];
			const char *pcmname = snd_pcm_name(handle);
			fprintf(stderr, _("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n"), rate, hwparams.rate);
			if (! pcmname || strchr(snd_pcm_name(handle), ':'))
				*plugex = 0;
			else
				snprintf(plugex, sizeof(plugex), "(-Dplug:%s)",
					 snd_pcm_name(handle));
			fprintf(stderr, _("         please, try the plug plugin %s\n"),
				plugex);
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
		error(_("Unable to install hw params:"));
		//snd_pcm_hw_params_dump(params, log);
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	//DEBUG_PRINTF("chunk_size=%lu",chunk_size);
	//DEBUG_PRINTF("buffer_size=%lu",buffer_size);
	if (chunk_size == buffer_size) {
		error(_("Can't use period equal to buffer size (%lu == %lu)"),
		      chunk_size, buffer_size);
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
		error(_("unable to install sw params:"));
		//snd_pcm_sw_params_dump(swparams, log);
		prg_exit(EXIT_FAILURE);
	}

	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	chunk_bytes = chunk_size * bits_per_frame / 8;

	//DEBUG_PRINTF("chunk_bytes=%lld\n",chunk_bytes);
	//DEBUG_PRINTF("chunk_bytes=%lld\n",chunk_bytes);
	audiobuf = realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		error(_("not enough memory"));
		prg_exit(EXIT_FAILURE);
	}

	//DEBUG_PRINTF("after set_params");
}

/* I/O error handler */
static void xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		error(_("status error: %s"), snd_strerror(res));
		prg_exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		if (fatal_errors) {
			error(_("fatal %s: %s"),
					stream == SND_PCM_STREAM_PLAYBACK ? _("underrun") : _("overrun"),
					snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		if ((res = snd_pcm_prepare(handle))<0) {
			error(_("xrun: prepare error: %s"), snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {

		if (stream == SND_PCM_STREAM_CAPTURE) {
			fprintf(stderr, _("capture stream format change? attempting recover...\n"));
			if ((res = snd_pcm_prepare(handle))<0) {
				error(_("xrun(DRAINING): prepare error: %s"), snd_strerror(res));
				prg_exit(EXIT_FAILURE);
			}
			return;
		}
	}

	error(_("read/write error, state = %s"), snd_pcm_state_name(snd_pcm_status_get_state(status)));
	prg_exit(EXIT_FAILURE);
}

/* I/O suspend handler */
static void suspend(void)
{
	int res;

	if (!quiet_mode)
		fprintf(stderr, _("Suspended. Trying resume. ")); fflush(stderr);
	while ((res = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		if (!quiet_mode)
			fprintf(stderr, _("Failed. Restarting stream. ")); fflush(stderr);
		if ((res = snd_pcm_prepare(handle)) < 0) {
			error(_("suspend: prepare error: %s"), snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
	}
	if (!quiet_mode)
		fprintf(stderr, _("Done.\n"));
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
			error(_("read error: %s"), snd_strerror(r));
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

	while (count > 0 && !in_aborting) {
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
static off64_t calc_count(void)
{
	off64_t count;

	if (timelimit == 0) {
		//DEBUG_PRINTF("timelimit=0");
		count = pbrec_count;
	} else {
		//DEBUG_PRINTF("timelimit!=0\n");
		count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
		count *= (off64_t)timelimit;
	}
	//DEBUG_PRINTF("count=%lld",count);
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
		error(_("Wave doesn't support %s format..."), snd_pcm_format_name(hwparams.format));
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
		error(_("write error"));
		prg_exit(EXIT_FAILURE);
	}
}
#ifdef ARECORD_TEST
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
		error(_("Wave doesn't support %s format..."), snd_pcm_format_name(thwparams.format));
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
		error(_("write error"));
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
			DEBUG_PRINTF("remove %s",name);
			remove(name);
			usleep(10*1000);
	}
	
	fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0644);
	DEBUG_PRINTF("open %d",fd);
	if (fd == -1) {
		if (errno != ENOENT )
			return -1;
	}
	return fd;
}

/*static int Init_BodyData(void)
{
	char *g_byBodyBuff = NULL;
	short g_BodyLength = 0;

	ft_fifo_clear(g_CaptureFifo);
	getuuidString(&g_boundary.m_postaudioBoundary);

	char *json_str = GetSpeechRecognizeEventjsonData();

	g_byBodyBuff = malloc(strlen(json_str) + 1024);
	if (NULL == g_byBodyBuff)
	{
		DEBUG_PRINTF("Init_BodyData malloc failed..");
		return -1;
	}

	// -- 分界线
	strcpy(g_byBodyBuff, "--");
	strcat(g_byBodyBuff, &g_boundary.m_postaudioBoundary);
	strcat(g_byBodyBuff, "\r\n");

	// create json data
	//DEBUG_PRINTF("create json data..");
	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");


	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "--");
	strcat(g_byBodyBuff, &g_boundary.m_postaudioBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"audio\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/octet-stream\r\n\r\n");

	g_BodyLength = strlen(g_byBodyBuff);

	ft_fifo_put(g_CaptureFifo, g_byBodyBuff, g_BodyLength);

	free(g_byBodyBuff);

	return 0;
}*/

#define BIG_ENDIAN 0
int vad(short samples[], int len)
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
	sum2 += (1000000);
	//sum2 += g_iVadThreshold;
	//printf("%d\t%lld\t%d\n", samples[i], sum, i);
  }
  //printf("sum=%lld, sum2=%lld\n", sum, sum2);
  if (sum > sum2)
	  return 1;
  else
	  return 0;
}
int iVadCount = 0;

static long capture(char *orig_name, char *pVADValue)
{
	struct VoiceData *p1, *p2;

	char byVADCont = 0;

	int fd1;

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

#ifdef WRITE_FILE
	fd1 = safe_open(UPLOAD_FILE_NAME);
	if (fd1 < 0) {
		DEBUG_ERROR("safe_open..filed :%d", fd1);
		prg_exit(EXIT_FAILURE);
	}
#endif

		
	long int startTime = 0;
	long int endTime = 0;
	long int vad_startTime = 0;
	long int vad_endTime = 0;
	char byFlag = 0;
	char byVadFlag = 0;
	char byStart = 0;
	char byVAD_StartFlag = 0;

	if (1 == g_byWakeUpFlag)
	{
		if (chunk_size >= 2000)
		{
			byVADCont = 12;
		}
		else if (chunk_size >= 1000)
		{
			byVADCont = 15;
		}
		else
		{
			byVADCont = 25;
		}
	}
	else
	{
		byVADCont = 28;
	}

	//printf(LIGHT_RED"byVADCont:%d"NONE, byVADCont);

	char *getRecordStatus;
	int status = 0;

	char *pAudioBuffer = NULL;
	unsigned long wAudioLen = 0;

	while (1) 
	{
		int ret;
		if(rest <= 0 )
			break;
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
/*
		if (1 == g_byWakeUpFlag)
		{
			pAudioBuffer = audiobuf;
			wAudioLen = c;
		}
		else
		{
			tmp= malloc(c*160/441/2);

			resample_buf_441_to_16(audiobuf, c, tmp, &len);

			//DEBUG_PRINTF("<capture>----c:%d, len:%d, iCount:%d", c, len, pcm_count);

			pAudioBuffer = tmp;
			wAudioLen = len;
		}
*/		
#ifdef WRITE_FILE
		if (write(fd1, pAudioBuffer, wAudioLen) != wAudioLen) {
			perror("16k_capture.wav");
			prg_exit(EXIT_FAILURE);
		}
#endif

		DEBUG_PRINTF("byVadEndFlag: %d,byVAD_StartFlag:%d", byVadEndFlag, byVAD_StartFlag);

		if ((0 == byVadEndFlag) && (1 == byVAD_StartFlag))
		{
			DEBUG_PRINTF("byStart:%d", byStart);
			if (0 == byStart)
			{
				// vad返回值为1时为is voice
				if (1 == vad(pAudioBuffer, wAudioLen/2))
				{
					DEBUG_PRINTF("Vad ================================== 1");
					byStart = 1;
				}
			}
			else if (0 == vad(pAudioBuffer, wAudioLen/2))
			{
				iCount++;
				DEBUG_PRINTF("Vad iCount:%d==================================", iCount);
				if (byVADCont <= iCount)
				{
					if (0 == byVadFlag)
					{
						byVadFlag = 1;
						iCount = 0;

						byVadEndFlag = 1; // 此次本地VAD结束，任务完成
						byVAD_StartFlag = 0;
						DEBUG_PRINTF(">>>>    VAD Threshold Over :%d    <<<<", g_iVadThreshold);
						
						cdb_set("$amazon_vad_Threshold", pVADValue);
						break;
					}
				}
			}
			else
			{
				DEBUG_PRINTF("00000000000000000000000000000=============", iCount);
				iCount = 0;
			}
		}

		if (tmp)
			free(tmp);

		pcm_count += (long)wAudioLen;
		if (pcm_count >= 22400)
		{
			byVAD_StartFlag = 1;
		}
	}

#ifdef WRITE_FILE
	if (close(fd1) < 0)
		DEBUG_PRINTF("close failed..");
#endif

	DEBUG_PRINTF("end...");

	
	return pcm_count;
}


