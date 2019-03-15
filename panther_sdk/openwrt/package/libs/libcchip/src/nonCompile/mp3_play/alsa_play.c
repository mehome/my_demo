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

#include <function/fifobuffer/fifobuffer.h>

#define _LARGEFILE64_SOURCE 1

#ifdef SND_CHMAP_API_VERSION
#define CONFIG_SUPPORT_CHMAP	1
#endif

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

#define DEFAULT_FORMAT		SND_PCM_FORMAT_S16_BE
#define DEFAULT_SPEED 		24000

#define FORMAT_DEFAULT		-1
#define FORMAT_RAW		0
#define FORMAT_VOC		1
#define FORMAT_WAVE		2
#define FORMAT_AU		3

/* global data */
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);

enum {
	VUMETER_NONE,
	VUMETER_MONO,
	VUMETER_STEREO
};

static snd_pcm_t *handle;

static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams, rhwparams;

static int in_aborting = 0;
static u_char *audiobuf = NULL;
static snd_pcm_uframes_t chunk_size = 0;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

static int avail_min = -1;
static int start_delay = 0;
static int stop_delay = 0;
static int monotonic = 0;
static int can_pause = 0;
static int fatal_errors = 0;
static int buffer_pos = 0;
static size_t bits_per_sample, bits_per_frame;
size_t alsa_play_chunk_bytes;
static int test_coef = 8;
static int test_nowait = 0;
static long long max_file_size = 0;
static int max_file_time = 0;
static int use_strftime = 0;
volatile static int recycle_capture_file = 0;
static long term_c_lflag = -1;

static int fd = -1;
static off64_t pbrec_count = LLONG_MAX, fdcount;

#ifdef CONFIG_SUPPORT_CHMAP
static snd_pcm_chmap_t *channel_map = NULL; /* chmap to override */
static unsigned int *hw_map = NULL; /* chmap to follow */
#endif

/*
 *	Subroutine to clean up before exit.
 */
static void prg_exit(int code) 
{
	if (handle)
		snd_pcm_close(handle);

	exit(code);
}

#ifdef CONFIG_SUPPORT_CHMAP
static int setup_chmap(void)
{
	snd_pcm_chmap_t *chmap = channel_map;
	char mapped[hwparams.channels];
	snd_pcm_chmap_t *hw_chmap;
	unsigned int ch, i;
	int err;

	if (!chmap)
		return 0;

	if (chmap->channels != hwparams.channels) {
		//printf(("Channel numbers don't match between hw_params and channel map"));
		return -1;
	}
	err = snd_pcm_set_chmap(handle, chmap);
	if (!err)
		return 0;

	hw_chmap = snd_pcm_get_chmap(handle);
	if (!hw_chmap) {
		//printf("Warning: unable to get channel map\n");
		return 0;
	}

	if (hw_chmap->channels == chmap->channels &&
	    !memcmp(hw_chmap, chmap, 4 * (chmap->channels + 1))) {
		/* maps are identical, so no need to convert */
		free(hw_chmap);
		return 0;
	}

	hw_map = calloc(hwparams.channels, sizeof(int));
	if (!hw_map) {
		//printf(("not enough memory"));
		return -1;
	}

	memset(mapped, 0, sizeof(mapped));
	for (ch = 0; ch < hw_chmap->channels; ch++) {
		if (chmap->pos[ch] == hw_chmap->pos[ch]) {
			mapped[ch] = 1;
			hw_map[ch] = ch;
			continue;
		}
		for (i = 0; i < hw_chmap->channels; i++) {
			if (!mapped[i] && chmap->pos[ch] == hw_chmap->pos[i]) {
				mapped[i] = 1;
				hw_map[ch] = i;
				break;
			}
		}
		if (i >= hw_chmap->channels) {
			char buf[256];
			//printf(("Channel %d doesn't match with hw_parmas"), ch);
			snd_pcm_chmap_print(hw_chmap, sizeof(buf), buf);
			//fprintf(stderr, "hardware chmap = %s\n", buf);
			return -1;
		}
	}
	free(hw_chmap);
	return 0;
}
#else
#define setup_chmap()	0
#endif

#ifndef timersub
#define	timersub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \
} while (0)
#endif

#ifndef timermsub
#define	timermsub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
	if ((result)->tv_nsec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_nsec += 1000000000L; \
	} \
} while (0)
#endif

/* I/O error handler */
static void xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		//printf(("status error: %s"), snd_strerror(res));
		prg_exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		if (fatal_errors) {
			/*printf(("fatal %s: %s"),
					stream == SND_PCM_STREAM_PLAYBACK ? ("underrun") : ("overrun"),
					snd_strerror(res));*/
			prg_exit(EXIT_FAILURE);
		}
		if (monotonic) {
#ifdef HAVE_CLOCK_GETTIME
			struct timespec now, diff, tstamp;
			clock_gettime(CLOCK_MONOTONIC, &now);
			snd_pcm_status_get_trigger_htstamp(status, &tstamp);
			timermsub(&now, &tstamp, &diff);
			/*fprintf(stderr, _("%s!!! (at least %.3f ms long)\n"),
				stream == SND_PCM_STREAM_PLAYBACK ? _("underrun") : _("overrun"),
				diff.tv_sec * 1000 + diff.tv_nsec / 10000000.0);*/
#else
			//printf("underrun...");
#endif
		} else {
			struct timeval now, diff, tstamp;
			gettimeofday(&now, 0);
			snd_pcm_status_get_trigger_tstamp(status, &tstamp);
			timersub(&now, &tstamp, &diff);
			/*fprintf(stderr, ("%s!!! (at least %.3f ms long)\n"),
				stream == SND_PCM_STREAM_PLAYBACK ? ("underrun") : ("overrun"),
				diff.tv_sec * 1000 + diff.tv_usec / 1000.0);*/
		}

		if ((res = snd_pcm_prepare(handle))<0) {
			//printf(("xrun: prepare error: %s"), snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} 
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
		//printf("SND_PCM_STATE_DRAINING");
	}
	//printf(("read/write error, state = %s"), snd_pcm_state_name(snd_pcm_status_get_state(status)));
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
			//printf(("suspend: prepare error: %s"), snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
	}
}

static void do_test_position(void)
{
	static long counter = 0;
	static time_t tmr = -1;
	time_t now;
	static float availsum, delaysum, samples;
	static snd_pcm_sframes_t maxavail, maxdelay;
	static snd_pcm_sframes_t minavail, mindelay;
	static snd_pcm_sframes_t badavail = 0, baddelay = 0;
	snd_pcm_sframes_t outofrange;
	snd_pcm_sframes_t avail, delay;
	int err;

	err = snd_pcm_avail_delay(handle, &avail, &delay);
	if (err < 0)
		return;
	outofrange = (test_coef * (snd_pcm_sframes_t)buffer_frames) / 2;
	if (avail > outofrange || avail < -outofrange ||
	    delay > outofrange || delay < -outofrange) {
	  badavail = avail; baddelay = delay;
	  availsum = delaysum = samples = 0;
	  maxavail = maxdelay = 0;
	  minavail = mindelay = buffer_frames * 16;
	  /*fprintf(stderr, ("Suspicious buffer position (%li total): "
	  	"avail = %li, delay = %li, buffer = %li\n"),
	  	++counter, (long)avail, (long)delay, (long)buffer_frames);*/
	}
}

/*
 */
#ifdef CONFIG_SUPPORT_CHMAP
static u_char *remap_data(u_char *data, size_t count)
{
	static u_char *tmp, *src, *dst;
	static size_t tmp_size;
	size_t sample_bytes = bits_per_sample / 8;
	size_t step = bits_per_frame / 8;
	size_t alsa_play_chunk_bytes;
	unsigned int ch, i;

	if (!hw_map)
		return data;

	alsa_play_chunk_bytes = count * bits_per_frame / 8;
	if (tmp_size < alsa_play_chunk_bytes) {
		free(tmp);
		tmp = malloc(alsa_play_chunk_bytes);
		if (!tmp) {
			//printf(("not enough memory"));
			exit(1);
		}
		tmp_size = count;
	}

	src = data;
	dst = tmp;
	for (i = 0; i < count; i++) {
		for (ch = 0; ch < hwparams.channels; ch++) {
			memcpy(dst, src + sample_bytes * hw_map[ch],
			       sample_bytes);
			dst += sample_bytes;
		}
		src += step;
	}
	return tmp;
}

static u_char **remap_datav(u_char **data, size_t count)
{
	static u_char **tmp;
	unsigned int ch;

	if (!hw_map)
		return data;

	if (!tmp) {
		tmp = malloc(sizeof(*tmp) * hwparams.channels);
		if (!tmp) {
			//printf(("not enough memory"));
			exit(1);
		}
		for (ch = 0; ch < hwparams.channels; ch++)
			tmp[ch] = data[hw_map[ch]];
	}
	return tmp;
}
#else
#define remap_data(data, count)		(data)
#define remap_datav(data, count)	(data)
#endif

/*
 *  write function
 */

static ssize_t pcm_write(u_char *data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;

	// 如果不足一个chunk_size，则插入静音
	if (count < chunk_size) {
		snd_pcm_format_set_silence(hwparams.format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
		count = chunk_size;
	}
	data = remap_data(data, count);
	while (count > 0) {
		r = writei_func(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			if (!test_nowait)
				snd_pcm_wait(handle, 100);
		} else if (r == -EPIPE) {
			xrun();
		} else if (r == -ESTRPIPE) {
			suspend();
		} else if (r < 0) {
			//printf(("write error: %s"), snd_strerror(r));
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
		//printf(("Broken configuration for this PCM: no configurations available"));
		prg_exit(EXIT_FAILURE);
	}

	// 交错访问
	err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		//printf(("Access type not available"));
		prg_exit(EXIT_FAILURE);
	}

	// SND_PCM_FORMAT_S16_BE
	err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (err < 0) {
		//printf(("Sample format non available"));
		prg_exit(EXIT_FAILURE);
	}

	// 1 channels
	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	if (err < 0) {
		//printf(("Channels count non available"));
		prg_exit(EXIT_FAILURE);
	}

	// 24000
	rate = hwparams.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);

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
	monotonic = snd_pcm_hw_params_is_monotonic(params);
	can_pause = snd_pcm_hw_params_can_pause(params);
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		//printf(("Unable to install hw params:"));
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		/*printf(("Can't use period equal to buffer size (%lu == %lu)"),
		      chunk_size, buffer_size);*/
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
		//printf(("unable to install sw params:"));
		//snd_pcm_sw_params_dump(swparams, log);
		prg_exit(EXIT_FAILURE);
	}

	if (setup_chmap())
		prg_exit(EXIT_FAILURE);

	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	alsa_play_chunk_bytes = chunk_size * bits_per_frame / 8;
	//printf("set params, alsa_play_chunk_bytes:%d, chunk_size:%d\n", alsa_play_chunk_bytes, chunk_size);
	audiobuf = realloc(audiobuf, alsa_play_chunk_bytes);
	if (audiobuf == NULL) {
		//printf(("not enough memory"));
		prg_exit(EXIT_FAILURE);
	}

	//printf("real chunk_size = %i, frags = %i, total = %i\n", chunk_size, setup.buf.block.frags, setup.buf.block.frags * chunk_size);

	buffer_frames = buffer_size;	/* for position test */
}
  
static ssize_t SNDWAV_WritePcm(unsigned char *data_buf, size_t wcount)  
{  
    ssize_t r;  
    ssize_t result = 0;  
    unsigned char *data = data_buf;  
  
    if (wcount < chunk_size) {
		//printf("pcm set silence..\n");
        snd_pcm_format_set_silence(rhwparams.format,   
            data + wcount * bits_per_frame / 8,   
            (chunk_size - wcount) * rhwparams.channels);  
        wcount = chunk_size;  
    }  
    while (wcount > 0) {  
        r = snd_pcm_writei(handle, data, wcount);  
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {  
            snd_pcm_wait(handle, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(handle);  
            //printf( " Buffer Underrun ");  
        } else if (r == -ESTRPIPE) {              
            //printf( " Need suspend");          
        } else if (r < 0) {  
            //printf( "Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
        if (r > 0) {  
            result += r;  
            wcount -= r;  
            data += r * bits_per_frame / 8;  
        }  
    }  
    return result;  
}  

void snd_Play_Buf(char *buf, unsigned int count)  
{  
	int ret;
    unsigned int  load;  
    unsigned int  written = 0;  
    unsigned int  c;  
  
    load = 0;

	//int iReadNumber = count / sndpcm->alsa_play_chunk_bytes;	
	//int iReadLastNumber = count % sndpcm->alsa_play_chunk_bytes;
	//printf("count=%u\n",count);
	//printf("alsa_play_chunk_bytes=%lu\n", alsa_play_chunk_bytes);
    while (written < count) {  
		
	  	c = count - written;	 
		if(c > alsa_play_chunk_bytes)
			load = alsa_play_chunk_bytes;
		else
			load = c; 
		//printf("load=%lu\n",load);
        /* Transfer to size frame */  
        load = load * 8 / bits_per_frame; 
		//SNDWAV_WritePcm_Buf()
        ret = SNDWAV_WritePcm(buf, load);  
        if (ret != load)  
            break;  
        
        ret = ret * bits_per_frame / 8;  
        written += ret;  
		//printf("written=%lu\n",written);
        load = 0;  
    }  
}   


/* playing raw data */
// 这个函数是否可以直接写alsa_play_chunk_bytes大小到pcm
void playback_go(char *pData, unsigned long len)
{
	int l, r;
	off64_t written = 0;
	off64_t c;

	int iCount = 0;

	int iReadNumber = len / alsa_play_chunk_bytes;	
	int iReadLastNumber = len % alsa_play_chunk_bytes;

	//printf("alsa_play_chunk_bytes:%d\n", alsa_play_chunk_bytes);

	// loaded为0， alsa_play_chunk_bytes根据chunk_size动态获取， count为最大值LLONG_MAX
	// audiobuf的大小为alsa_play_chunk_bytes
	l= 0;
	while (written < pbrec_count && !in_aborting) {
		// do while主要是用来读取数据，
		do {
			c = pbrec_count - written;
			if (c > alsa_play_chunk_bytes)
				c = alsa_play_chunk_bytes;
			c -= l;

			if (c == 0)
				break;

			if (iCount == iReadNumber)			
			{
				//printf("aaaaaaa  l:%d\r\n", l);				
				memcpy(audiobuf + l, pData, iReadLastNumber);
				r = iReadLastNumber;
				pData += iReadLastNumber;
			}
			else
			{
				//printf("bbbbbbbbbb l:%d\r\n", l);
				memcpy(audiobuf + l, pData, alsa_play_chunk_bytes);
				r = alsa_play_chunk_bytes;
				pData += alsa_play_chunk_bytes;
			}			

			iCount++;
			fdcount += r;
			if (r == 0)
				break;
			l += r;
		} while ((size_t)l < alsa_play_chunk_bytes);

		l = l * 8 / bits_per_frame;
		r = pcm_write(audiobuf, l);
		if (r != l)
			break;
		r = r * bits_per_frame / 8;
		written += r;
		l = 0;
	}
}

static void alsa_params_init(void)
{
	size_t dta;

	pbrec_count = LLONG_MAX;
	fdcount = 0;

	//playback_go(fd, dta, pbrec_count, FORMAT_RAW, name);

	set_params();
}

//void alsa_init(void)
int alsa_init(char *odev,int format, int rate, int channels)
{
	/*rhwparams.format = DEFAULT_FORMAT;
	rhwparams.rate = DEFAULT_SPEED;
	rhwparams.channels = 1;*/

	rhwparams.format = format;
	rhwparams.rate = rate;
	rhwparams.channels = channels;

	int err;

	err = snd_pcm_open(&handle, odev, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf(("audio open error: %s"), snd_strerror(err));
		return -1;
	}

	chunk_size = 1024;
	hwparams = rhwparams;

	audiobuf = (u_char *)malloc(1024);
	if (audiobuf == NULL) {
		printf(("not enough memory"));
		return -2;
	}

	writei_func = snd_pcm_writei;

	alsa_params_init();
	return 0;
}

void alsa_close(void)
{
	if (handle != NULL)
	{		
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		handle = NULL;	
	}	
	if (audiobuf != NULL)	
	{
		free(audiobuf);
		audiobuf = NULL;	
	}
}

