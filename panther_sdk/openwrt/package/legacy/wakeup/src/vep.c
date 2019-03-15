#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT)
#define _GNU_SOURCE
#define __USE_GNU
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include <signal.h>

#include "vepug-api.h"
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "interface.h"
#include "debug.h"

#pragma GCC push_options
#pragma GCC optimize ("-O4")

#define __DCACHE_ALIGNED__		__attribute__ ((aligned (32)))

#define READER_NEEDS_SIZES

#include "asp.h"
#include "spf-postapi.h"
#include "cmdlutils.h"

void *p;
void *smr[NUM_MEM_REGIONS];
mem_reg_t reg[NUM_MEM_REGIONS];
static PROFILE_TYPE(t) profile;

#define DBG_OUTPUT_RAW_DATA

#define USE_USB_AUDIO

//#define USE_REALTIME_SCHEDULER

#define SET_NICE

#define CACHE_OPTIMIZATION

//#define COLOR_DYING

//#define DEBUG_DUMP2

#if defined(CONFIG_INPUT_MIC_HW2_REF_SW2) || defined(CONFIG_INPUT_MIC_HW4_REF_SW2)
#define CONFIG_LOOPBACK_AUDIO
#endif

#if defined(CONFIG_INPUT_MIC_HW2_REF_HW2)
#define CONFIG_MIC_ARRAY
#define MIC2_REF2
#endif

#if defined(CONFIG_INPUT_MIC_HW4_REF_HW2) || defined(CONFIG_INPUT_MIC_HW4_REF_SW2) \
    || defined(CONFIG_INPUT_MIC_HW3_REF_HW2) || defined(CONFIG_INPUT_MIC_HW3_REF_SW2)
#define CONFIG_MIC_ARRAY
#define CONFIG_MIC_ARRAY_4_CHAN
#endif

#if defined(CONFIG_INPUT_MIC_HW2_REF_SW2)
#define CONFIG_HARDWARE_AUDIO_44_1_KHZ
#endif

#if defined(CONFIG_LOOPBACK_AUDIO) || defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

static SwrContext *swr_loopback_audio = NULL;
static SwrContext *swr_hardware_audio = NULL;
#endif

#define error printf

static pthread_t asp_pthread;
static pthread_t convert_pthread;
static pthread_t input_pthread;
static snd_pcm_t *handle_loopback_audio = NULL;
static snd_pcm_t *handle_hardware_audio = NULL;

#if defined(DBG_OUTPUT_RAW_DATA)
#if defined(CONFIG_LOOPBACK_AUDIO)
//static FILE *loopback_fp = NULL;
#endif
static FILE *hardware_fp = NULL;
static FILE *ref0_fp = NULL;
static FILE *ref1_fp = NULL;
static FILE *mic0_fp = NULL;
static FILE *mic1_fp = NULL;
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
static FILE *mic2_fp = NULL;
static FILE *mic3_fp = NULL;
#endif
static FILE *asp_fp = NULL;
#endif


#if defined(CONFIG_LOOPBACK_AUDIO)
#define SOURCE_LOOPBACK_SAMPLE_RATE		44100
#define SOURCE_LOOPBACK_CHANNELS		2
#endif

#if defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
#define SOURCE_HARDWARE_SAMPLE_RATE		44100
#else
#define SOURCE_HARDWARE_SAMPLE_RATE		16000
#endif

#if defined(CONFIG_MIC_ARRAY)
#define SOURCE_HARDWARE_CHANNELS			6
#else
#define SOURCE_HARDWARE_CHANNELS			2
#endif

#define BYTES_PER_SAMPLE		2

#define ASP_SAMPLE_RATE			16000

static int sample_rate = ASP_SAMPLE_RATE;
static int channels = 6;

int hardware_audio_init()
{
	int ret;

#if defined(CONFIG_MIC_ARRAY)
	char *device = "plug:micArray";
#elif defined(USE_USB_AUDIO)
	char *device = "plughw:0,0";
#else
	char *device = "default";
#endif

#if defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
	sample_rate = 44100;
#else
	sample_rate = 16000;
#endif

#if defined(CONFIG_MIC_ARRAY)
	channels = 6;
#else
	channels = 2;
#endif

	snd_pcm_hw_params_t *hw_params = NULL;

	if(snd_pcm_open(&handle_hardware_audio, device, SND_PCM_STREAM_CAPTURE, 0) < 0)
	{ 
		error("Error snd_pcm_open [ %s]", device);  
		goto err;
	} 

	if((ret = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		error("unable to malloc hw_params: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params_any(handle_hardware_audio, hw_params)) < 0)
	{
		error("unable to setup hw_params: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params_set_access(handle_hardware_audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		error("unable to set access mode: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params_set_format(handle_hardware_audio, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		error("unable to set format: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params_set_channels(handle_hardware_audio, hw_params, channels)) < 0)
	{
		error("unable to set channels %d: %s\n", channels, snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params_set_rate(handle_hardware_audio, hw_params, sample_rate, 0)) < 0)
	{
		error("unable to set samplerate: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params(handle_hardware_audio, hw_params)) < 0)
	{
		error("unable to set hw parameters: %s\n",snd_strerror(ret));
		goto err;
	}

	snd_pcm_hw_params_free(hw_params);
	return 0;

err:
	if(handle_hardware_audio != NULL)
	{
		snd_pcm_close(handle_hardware_audio);
	}
	exit(-1);
	return -1;
}

#if defined(CONFIG_LOOPBACK_AUDIO)
int loopback_audio_init()
{
	int ret;
	char *device = "plughw:Loopback,1,0";
	snd_pcm_hw_params_t *hw_params = NULL;

	if(snd_pcm_open(&handle_loopback_audio, device, SND_PCM_STREAM_CAPTURE, 0) < 0)
	{ 
		error("Error snd_pcm_open [ %s]", device);  
		goto err;
	} 
	if((ret = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		error("unable to malloc hw_params: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_any(handle_loopback_audio, hw_params)) < 0)
	{
		error("unable to setup hw_params: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_access(handle_loopback_audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		error("unable to set access mode: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_format(handle_loopback_audio, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		error("unable to set format: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_channels(handle_loopback_audio, hw_params, SOURCE_LOOPBACK_CHANNELS)) < 0)
	{
		error("unable to set channels: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_rate(handle_loopback_audio, hw_params, SOURCE_LOOPBACK_SAMPLE_RATE, 0)) < 0)
	{
		error("unable to set samplerate: %s\n",snd_strerror(ret));
		goto err;
	}

	if((ret = snd_pcm_hw_params(handle_loopback_audio, hw_params)) < 0)
	{
		error("unable to set hw parameters: %s\n",snd_strerror(ret));
		goto err;
	}
	snd_pcm_hw_params_free(hw_params);
	return 0;

err:
	if(handle_loopback_audio != NULL)
	{
		snd_pcm_close(handle_loopback_audio);
	}
	exit(-1);
	return -1;
} 
#endif

int sw_resampler_init()
{
#if defined(CONFIG_LOOPBACK_AUDIO)
	swr_loopback_audio = (void *) swr_alloc_set_opts(NULL,  // we're allocating a new context
					AV_CH_LAYOUT_STEREO, //AV_CH_LAYOUT_MONO,  // out_ch_layout
					AV_SAMPLE_FMT_S16,    // out_sample_fmt
					ASP_SAMPLE_RATE,      // out_sample_rate
					AV_CH_LAYOUT_STEREO, // in_ch_layout
					AV_SAMPLE_FMT_S16,   // in_sample_fmt
					44100,       // in_sample_rate
					0,                    // log_offset
					NULL);                // log_ctx
	if (swr_loopback_audio == NULL)
	{
		printf("swr_loopback_audio_alloc_set_opts failed.\n");
		return -1;
	}
	swr_init(swr_loopback_audio);
#endif

#if defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
	swr_hardware_audio = (void *) swr_alloc_set_opts(NULL,  // we're allocating a new context
					AV_CH_LAYOUT_STEREO, //AV_CH_LAYOUT_MONO,  // out_ch_layout
					AV_SAMPLE_FMT_S16,    // out_sample_fmt
					ASP_SAMPLE_RATE,      // out_sample_rate
					AV_CH_LAYOUT_STEREO, // in_ch_layout
					AV_SAMPLE_FMT_S16,   // in_sample_fmt
					44100,       // in_sample_rate
					0,                    // log_offset
					NULL);                // log_ctx
	if (swr_hardware_audio == NULL)
	{
		printf("swr_loopback_audio_alloc_set_opts failed.\n");
		return -1;
	}
	swr_init(swr_hardware_audio);
#endif

#if defined(DBG_OUTPUT_RAW_DATA)
	if (wakeupStartArgsPtr->wakeup_mode == 1)
	{
#if defined(CONFIG_LOOPBACK_AUDIO)
		//loopback_fp = fopen("/tmp/misc/loopback.raw", "wb");
#endif
		//hardware_fp = fopen("/tmp/misc/hardware.raw", "wb");
		ref0_fp = fopen("/tmp/misc/ref0.raw", "wb");
		ref1_fp = fopen("/tmp/misc/ref1.raw", "wb");
		mic0_fp = fopen("/tmp/misc/mic0.raw", "wb");
		mic1_fp = fopen("/tmp/misc/mic1.raw", "wb");
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
		mic2_fp = fopen("/tmp/misc/mic2.raw", "wb");
		mic3_fp = fopen("/tmp/misc/mic3.raw", "wb");
#endif
		asp_fp = fopen("/tmp/misc/asp.raw", "wb");
	}
#endif

	return 0;
}

struct rb
{
	volatile unsigned int ridx;
	volatile unsigned int widx;
	unsigned int depth;
	unsigned int chunk_size;
	unsigned char *buf;
	unsigned long long* timestamp;
};

volatile unsigned long long timestamp;
volatile unsigned long long drop_target_ts;

/* each read gets 20ms audio data */
#if defined(CONFIG_LOOPBACK_AUDIO)
#define SOURCE_LOOPBACK_SAMPLES_PER_READ	(SOURCE_LOOPBACK_SAMPLE_RATE / 5)
#define SOURCE_LOOPBACK_BYTES_PER_READ		(SOURCE_LOOPBACK_SAMPLES_PER_READ * SOURCE_LOOPBACK_CHANNELS * BYTES_PER_SAMPLE)
#endif
#define SOURCE_HARDWARE_SAMPLES_PER_READ	(SOURCE_HARDWARE_SAMPLE_RATE / 5)
#define SOURCE_HARDWARE_BYTES_PER_READ		(SOURCE_HARDWARE_SAMPLES_PER_READ * SOURCE_HARDWARE_CHANNELS * BYTES_PER_SAMPLE)
#define SOURCE_BUF_CHUNKS			10

struct rb source_hardware;
#if defined(CONFIG_LOOPBACK_AUDIO)
struct rb source_loopback;
#endif

#define CONVERT_SAMPLE_RATE			16000
#define CONVERT_SAMPLES_PER_WRITE	(CONVERT_SAMPLE_RATE / 5)

#define REF_CHANNELS				2
#define REF_BYTES_PER_WRITE			(CONVERT_SAMPLES_PER_WRITE * REF_CHANNELS * BYTES_PER_SAMPLE)

#if defined(CONFIG_INPUT_MIC_HW2_REF_SW2) || defined(CONFIG_INPUT_MIC_HW2_REF_HW2)
#define MIC_CHANNELS				2
#elif defined(CONFIG_MIC_ARRAY_4_CHAN)
#define MIC_CHANNELS				4
#endif
#define MIC_BYTES_PER_WRITE			(CONVERT_SAMPLES_PER_WRITE * MIC_CHANNELS * BYTES_PER_SAMPLE)

#if defined(CONFIG_KWD_SENSORY) || defined(CONFIG_KWD_KITTAI) || defined(CONFIG_KWD_SKV)
#define CONVERT_BUF_CHUNKS			6
#else
#define CONVERT_BUF_CHUNKS			10
#endif

struct rb input_mic __DCACHE_ALIGNED__;
struct rb input_ref __DCACHE_ALIGNED__;

#define ASP_CHANNELS				1
#define ASP_SAMPLES_PER_WRITE		(ASP_SAMPLE_RATE / 5)
#define ASP_BYTES_PER_WRITE			(ASP_SAMPLES_PER_WRITE * ASP_CHANNELS * BYTES_PER_SAMPLE)
#if defined(CONFIG_KWD_SENSORY) || defined(CONFIG_KWD_KITTAI) || defined(CONFIG_KWD_SKV)
#define ASP_BUF_CHUNKS				6
#else
#define ASP_BUF_CHUNKS				10
#endif
#define ASP_SAMPLES_PER_PROC		128

struct rb asp;

static unsigned char src_hardware_buf[SOURCE_BUF_CHUNKS * SOURCE_HARDWARE_BYTES_PER_READ] __DCACHE_ALIGNED__;
static unsigned long long src_hardware_timestamp[SOURCE_BUF_CHUNKS];
#if defined(CONFIG_LOOPBACK_AUDIO)
static unsigned char src_loopback_buf[SOURCE_BUF_CHUNKS * SOURCE_LOOPBACK_BYTES_PER_READ] __DCACHE_ALIGNED__;
static unsigned long long src_loopback_timestamp[SOURCE_BUF_CHUNKS];
#endif
static unsigned char mic_buf[CONVERT_BUF_CHUNKS * MIC_BYTES_PER_WRITE] __DCACHE_ALIGNED__;
static unsigned long long mic_timestamp[CONVERT_BUF_CHUNKS];
static unsigned char ref_buf[CONVERT_BUF_CHUNKS * REF_BYTES_PER_WRITE] __DCACHE_ALIGNED__;
static unsigned long long ref_timestamp[CONVERT_BUF_CHUNKS];
static unsigned char asp_buf[ASP_BUF_CHUNKS * ASP_BYTES_PER_WRITE] __DCACHE_ALIGNED__;
static unsigned long long asp_timestamp[ASP_BUF_CHUNKS];
void rb_init(void)
{
	source_hardware.ridx = source_hardware.widx = 0;
	source_hardware.buf = src_hardware_buf;
	source_hardware.timestamp = src_hardware_timestamp;
	source_hardware.chunk_size = SOURCE_HARDWARE_BYTES_PER_READ;
	source_hardware.depth = SOURCE_BUF_CHUNKS;

#if defined(CONFIG_LOOPBACK_AUDIO)
	source_loopback.ridx = source_loopback.widx = 0;
	source_loopback.buf = src_loopback_buf;
	source_hardware.timestamp = src_loopback_timestamp;
	source_loopback.chunk_size = SOURCE_LOOPBACK_BYTES_PER_READ;
	source_loopback.depth = SOURCE_BUF_CHUNKS;
#endif

	input_mic.ridx = input_mic.widx = 0;
	input_mic.buf = mic_buf;
	input_mic.timestamp = mic_timestamp;
	input_mic.chunk_size = MIC_BYTES_PER_WRITE;
	input_mic.depth = CONVERT_BUF_CHUNKS;

	input_ref.ridx = input_ref.widx = 0;
	input_ref.buf = ref_buf;
	input_ref.timestamp = ref_timestamp;
	input_ref.chunk_size = REF_BYTES_PER_WRITE;
	input_ref.depth = CONVERT_BUF_CHUNKS;

	asp.ridx = asp.widx = 0;
	asp.buf = asp_buf;
	asp.timestamp = asp_timestamp;
	asp.chunk_size = ASP_BYTES_PER_WRITE;
	asp.depth = ASP_BUF_CHUNKS;
}
// static pthread_mutex_t rbMtx = PTHREAD_MUTEX_INITIALIZER;
void rb_clear(struct rb *prb){
	// pthread_mutex_lock(&rbMtx);
	prb->widx = prb->ridx;
	// pthread_mutex_unlock(&rbMtx);
}

int rb_empty(struct rb *prb)
{
	int val=0;
	// pthread_mutex_lock(&rbMtx);
	val=(prb->widx == prb->ridx);
	// pthread_mutex_unlock(&rbMtx);
	return val;
}

int rb_full(struct rb *prb)
{
	int val=0;
	// pthread_mutex_lock(&rbMtx);
	val=((((prb->widx) + 1) % prb->depth) == (prb->ridx));
	// pthread_mutex_unlock(&rbMtx);
	return val;
}

int rb_full2(struct rb *prb)
{
	int val=0;
	// pthread_mutex_lock(&rbMtx);
	val=(rb_full(prb) || ((((prb->widx) + 2) % prb->depth) == (prb->ridx)));
	// pthread_mutex_unlock(&rbMtx);
	return val;
}

int rb_use(struct rb *prb)
{
	int val=0;
	// pthread_mutex_lock(&rbMtx);
	val = (prb->widx - prb->ridx);
	if(val < 0)
		val += prb->depth;
	// pthread_mutex_unlock(&rbMtx);
	return val;
}

void rb_write_timestamp(struct rb *prb, unsigned long long ts)
{
	prb->timestamp[prb->widx] = ts;
}

unsigned long long rb_read_timestamp(struct rb *prb)
{
	return prb->timestamp[prb->ridx];
}

unsigned char *rb_waddr(struct rb *prb)
{
	unsigned char *addr=NULL;
	// pthread_mutex_lock(&rbMtx);
	addr=&prb->buf[prb->widx * prb->chunk_size];
	// pthread_mutex_unlock(&rbMtx);
	return addr;
}

unsigned char *rb_raddr(struct rb *prb)
{
	unsigned char *addr=NULL;
	// pthread_mutex_lock(&rbMtx);
	addr=&prb->buf[prb->ridx * prb->chunk_size];
	// pthread_mutex_unlock(&rbMtx);
	return addr;
}

void rb_widx_inc(struct rb *prb)
{
	// pthread_mutex_lock(&rbMtx);
	prb->widx = ((prb->widx + 1) % prb->depth);
	// pthread_mutex_unlock(&rbMtx);
}

void rb_ridx_inc(struct rb *prb)
{
	// pthread_mutex_lock(&rbMtx);
	prb->ridx = ((prb->ridx + 1) % prb->depth);
	// pthread_mutex_unlock(&rbMtx);
}

void rb_widx_inc2(struct rb *prb)
{
	// pthread_mutex_lock(&rbMtx);
	prb->widx = ((prb->widx + 2) % prb->depth);
	// pthread_mutex_unlock(&rbMtx);
}

void rb_ridx_inc2(struct rb *prb)
{
	// pthread_mutex_lock(&rbMtx);
	prb->ridx = ((prb->ridx + 2) % prb->depth);
	// pthread_mutex_unlock(&rbMtx);
}

void rb_ridx_inc5(struct rb *prb)
{
	// pthread_mutex_lock(&rbMtx);
	prb->ridx = ((prb->ridx + 5) % prb->depth);
	// pthread_mutex_unlock(&rbMtx);
}

#if 1
#define __NR_Linux                      4000
#define __NR_print                      (__NR_Linux + 360)
#define __NR_panic                      (__NR_Linux + 361)
#define __NR_cpu_control        (__NR_Linux + 362)

#define LOGLEVEL_EMERG          0       /* system is unusable */
#define LOGLEVEL_ALERT          1       /* action must be taken immediately */
#define LOGLEVEL_CRIT           2       /* critical conditions */
#define LOGLEVEL_ERR            3       /* error conditions */
#define LOGLEVEL_WARNING        4       /* warning conditions */
#define LOGLEVEL_NOTICE         5       /* normal but significant condition */
#define LOGLEVEL_INFO           6       /* informational */
#define LOGLEVEL_DEBUG          7       /* debug-level messages */

static inline void mt_printk(int level, char *buf, int len)
{
        syscall(__NR_print, level, buf, len);
}
#endif

#if defined(SET_NICE)
static int _set_nice(int nice)
{
    pid_t tid;
    int ret, max_retry = 20;

    tid = syscall(SYS_gettid);
    if (nice > 19 || nice < -20)
        return -EINVAL;

    while (max_retry > 0)
    {
        ret = setpriority(PRIO_PROCESS, tid, nice);
        if (ret == 0)
            return 0;
        if (ret == EINTR || ret == EAGAIN)
        {
            max_retry--;
        }
        else
        {
            break;
        }
    }
    return -1;
}
#endif

// large enough to accommodate 44.1k 2-channel (3528 bytes), or 16k 6-channels(3840 bytes)
unsigned char tmpbuf[SOURCE_HARDWARE_BYTES_PER_READ] __DCACHE_ALIGNED__;
void *input_thread(void *ptr)
{
#if defined(CONFIG_LOOPBACK_AUDIO)
	int len_loopback;
#endif
	int len_hardware;

#if defined(SET_NICE)
	if (0 != _set_nice(-20))
		printf("input_thread set nice failed\n");
#endif

#if defined(DEBUG_DUMP2)
	hardware_fp = fopen("/tmp/misc/hardware.raw", "wb");
#endif

	showProcessThreadId("");
	posixThreadOps_t *wakeup_waitctl = wakeupStartArgsPtr->wakeup_waitctl;
	if(!wakeup_waitctl){
		return NULL;
	}
#if defined(CONFIG_LOOPBACK_AUDIO)
	snd_pcm_readi(handle_hardware_audio, tmpbuf, SOURCE_HARDWARE_SAMPLES_PER_READ);
	snd_pcm_readi(handle_hardware_audio, tmpbuf, SOURCE_HARDWARE_SAMPLES_PER_READ);
#endif

	while(1)
	{	
#if defined(CONFIG_LOOPBACK_AUDIO)
		if(rb_full(&source_loopback) || rb_full(&source_hardware))
#else
		if(rb_full(&source_hardware))
#endif
		{
#if defined(CONFIG_LOOPBACK_AUDIO)
			len_loopback = snd_pcm_readi(handle_loopback_audio, tmpbuf, SOURCE_LOOPBACK_SAMPLES_PER_READ);
#endif
			len_hardware = snd_pcm_readi(handle_hardware_audio, tmpbuf, SOURCE_HARDWARE_SAMPLES_PER_READ);
			WAKEUP_WAR("DROP");
			mt_printk(LOGLEVEL_CRIT, "DROP\n", 5);
			timestamp++;
		}
		else
		{
			posix_thread_ops_listen_wait(wakeup_waitctl);//仅在开关麦克风时控制
			timestamp++;

#if defined(CONFIG_LOOPBACK_AUDIO)
			len_loopback = snd_pcm_readi(handle_loopback_audio, (void *) rb_waddr(&source_loopback), SOURCE_LOOPBACK_SAMPLES_PER_READ);
#endif
			len_hardware = snd_pcm_readi(handle_hardware_audio, (void *) rb_waddr(&source_hardware), SOURCE_HARDWARE_SAMPLES_PER_READ);
#if defined(CONFIG_LOOPBACK_AUDIO)
			rb_write_timestamp(&source_loopback, timestamp);
			rb_widx_inc(&source_loopback);
#endif
#if defined(DEBUG_DUMP2)
			if(timestamp > 900)
			{
				if(hardware_fp)
				{
					fclose(hardware_fp);
					hardware_fp = NULL;
				}
			}
			else
			{
				if(hardware_fp)
					fwrite(rb_waddr(&source_hardware), 1, SOURCE_HARDWARE_BYTES_PER_READ, hardware_fp);
			}
#endif
			rb_write_timestamp(&source_hardware, timestamp);
			rb_widx_inc(&source_hardware);
#if defined(DBG_OUTPUT_RAW_DATA)
			/*if (asp_mode == 1)
			{
#if defined(CONFIG_LOOPBACK_AUDIO)
				fwrite(rb_waddr(&source_loopback), 1, SOURCE_LOOPBACK_BYTES_PER_READ, loopback_fp);
#endif
				fwrite(rb_waddr(&source_hardware), 1, SOURCE_HARDWARE_BYTES_PER_READ, hardware_fp);
			}*/
#endif
			//if(rb_use(&source_loopback)>=20)
			//	printf("(Use %d %d)\n", rb_use(&source_loopback), rb_use(&source_hardware));
		}

#if defined(CONFIG_LOOPBACK_AUDIO)
		if((len_loopback!=SOURCE_LOOPBACK_SAMPLES_PER_READ)||(len_hardware!=SOURCE_HARDWARE_SAMPLES_PER_READ))
#else
		if(len_hardware!=SOURCE_HARDWARE_SAMPLES_PER_READ)
#endif
		{
#if defined(CONFIG_LOOPBACK_AUDIO)
			WAKEUP_TRACE("Fatal error: %d %d, restart\n", len_loopback, len_hardware);
#else
			WAKEUP_TRACE("Fatal error: %d, restart\n", len_hardware);
#endif
			inf("Fatal %d\n", len_hardware);
			mt_printk(LOGLEVEL_CRIT, "FATAL\n", 6);
			//abort();
#if defined(CONFIG_LOOPBACK_AUDIO)
			if(handle_loopback_audio)
			{
				snd_pcm_close(handle_loopback_audio);
				handle_loopback_audio = NULL;
			}
#endif
			if(len_hardware == -EAGAIN)
			{
				snd_pcm_wait(handle_hardware_audio, 10);
				continue;
			}
			if(len_hardware == -EPIPE)
			{
				snd_pcm_recover(handle_hardware_audio, len_hardware, 0);
				continue;
			}
			if(handle_hardware_audio)
			{
				snd_pcm_close(handle_hardware_audio);
				handle_hardware_audio = NULL;
			}
#if defined(CONFIG_LOOPBACK_AUDIO)
			loopback_audio_init();
#endif
			hardware_audio_init();
		}
	}
}

#define ASP_CHANNEL_SAMPLES_PER_WRITE	((ASP_SAMPLE_RATE * 8)/1000)		// 8ms
#define ASP_CHANNEL_BYTES_PER_WRITE		(ASP_CHANNEL_SAMPLES_PER_WRITE * BYTES_PER_SAMPLE)
#if defined(CONFIG_LOOPBACK_AUDIO) || defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
static uint8_t swr_temp_buf[(SOURCE_LOOPBACK_SAMPLES_PER_READ * 2) + 64] __DCACHE_ALIGNED__;
static uint8_t swr_buf[REF_BYTES_PER_WRITE*2] __DCACHE_ALIGNED__;
#endif
unsigned long long last_convert_timestamp;
void *convert_thread(void *ptr)
{
#if defined(CONFIG_LOOPBACK_AUDIO)
	uint8_t *input_ref_in[2] = { (void *) 0, (void *) 0 };
	uint8_t *input_ref_out[2] = { (void *) 0, (void *) 0 };
	int ret_swr_loopback;
	unsigned short *in_loopback;
	static int first_swr = 1;
	int swr_input_samples;
#endif
#if defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
	uint8_t *input_mic_in[2] = { (void *) 0, (void *) 0 };
	uint8_t *input_mic_out[2] = { (void *) 0, (void *) 0 };
	int ret_swr_hardware;
#endif
	unsigned short *in_micarray;
	unsigned short *ref0, *ref1;
	unsigned short *mic0, *mic1;
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
	unsigned short *mic2, *mic3;
#endif
	int i, n;
	unsigned long long ts;

#if defined(SET_NICE)
	if (0 != _set_nice(-20))
		printf("convert_thread set nice failed\n");
#endif

#if defined(DEBUG_DUMP2)
	mic0_fp = fopen("/tmp/misc/mic0.raw", "wb");
#endif

	showProcessThreadId("");
	while(1)
	{
//    	 posix_thread_ops_listen_wait(wakeup_waitctl);//用于实现开关麦克风
		if(rb_full(&input_ref) || rb_full(&input_mic))
		{
			usleep(10000);
			//printf("Convert FULL\n");
		}
		else
		{
			if((rb_use(&source_hardware))<1)
			{
				usleep(10000);
				//printf("Convert\n");
			}
			else
			{
				ts = rb_read_timestamp(&source_hardware);
				if(ts<last_convert_timestamp)
					mt_printk(LOGLEVEL_CRIT, "CONVERT BUG\n", 12);
				last_convert_timestamp = ts;

#if defined(CONFIG_LOOPBACK_AUDIO)
				input_ref_out[0] = swr_buf;

				if(first_swr)
				{
					input_ref_in[0] = swr_temp_buf;

					swr_convert(swr_loopback_audio, input_ref_out, CONVERT_SAMPLES_PER_WRITE * 2
									   ,(const uint8_t **) input_ref_in, (SOURCE_LOOPBACK_SAMPLES_PER_READ * 2) + 45);
					first_swr = 0;
				}

				input_ref_in[0] = rb_raddr(&source_loopback);

				ret_swr_loopback = swr_convert(swr_loopback_audio, input_ref_out, CONVERT_SAMPLES_PER_WRITE * 2
									   ,(const uint8_t **) input_ref_in, SOURCE_LOOPBACK_SAMPLES_PER_READ * 2);
				//printf("swr_convert: %d %d\n", ret_swr_loopback, CONVERT_SAMPLES_PER_WRITE);
				if(ret_swr_loopback!=CONVERT_SAMPLES_PER_WRITE*2)
				{
					printf("SWR REF loopback ret %d %d\n", ret_swr_loopback, CONVERT_SAMPLES_PER_WRITE * 2);
				}

				in_loopback = (unsigned short *) &swr_buf[0];

				ref0 = (unsigned short *)(rb_waddr(&input_ref) + (ASP_CHANNEL_BYTES_PER_WRITE * 0));
				ref1 = (unsigned short *)(rb_waddr(&input_ref) + (ASP_CHANNEL_BYTES_PER_WRITE * 1));

				n = 0;
				for(i=0;i<(CONVERT_SAMPLES_PER_WRITE);i++)
				{
#if defined(CACHE_OPTIMIZATION)
					if(n%16==0)
					{
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (ref0));
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (ref1));
					}
#endif

					ref0[n] = in_loopback[0];
					ref1[n] = in_loopback[1];
					n++;
					in_loopback += SOURCE_LOOPBACK_CHANNELS;

					if((ASP_CHANNEL_SAMPLES_PER_WRITE-1)==(i%ASP_CHANNEL_SAMPLES_PER_WRITE))
					{
#if defined(DBG_OUTPUT_RAW_DATA)
						if (wakeupStartArgsPtr->wakeup_mode == 1)
						{
							fwrite(ref0, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, ref0_fp);
							fwrite(ref1, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, ref1_fp);
						}
#endif
						ref0 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
						ref1 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
						n = 0;
					}
				}

				rb_ridx_inc(&source_loopback);
#endif

#if defined(CONFIG_HARDWARE_AUDIO_44_1_KHZ)
				input_mic_in[0] = rb_raddr(&source_hardware);
				input_mic_out[0] = swr_buf;

				ret_swr_hardware = swr_convert(swr_hardware_audio, input_mic_out, CONVERT_SAMPLES_PER_WRITE * 2
									   ,(const uint8_t **) input_mic_in, SOURCE_HARDWARE_BYTES_PER_READ * 2);
				//printf("swr_convert: %d %d\n", ret_swr_hardware, CONVERT_SAMPLES_PER_WRITE * 2);
				if(ret_swr_hardware<(CONVERT_SAMPLES_PER_WRITE*2))
				{
					printf("SWR MIC hardware ret %d %d\n", ret_swr_hardware, CONVERT_SAMPLES_PER_WRITE);
					memmove(swr_buf + ((CONVERT_SAMPLES_PER_WRITE*2 - ret_swr_loopback) * REF_CHANNELS * BYTES_PER_SAMPLE)
							, swr_buf
							, ((CONVERT_SAMPLES_PER_WRITE*2 - ret_swr_loopback) * REF_CHANNELS * BYTES_PER_SAMPLE));
				}

				in_micarray = (unsigned short *)&swr_buf[0];
#else
				in_micarray = (unsigned short *)rb_raddr(&source_hardware);
#endif

#if !defined(CONFIG_LOOPBACK_AUDIO)
				ref0 = (unsigned short *)(rb_waddr(&input_ref) + (ASP_CHANNEL_BYTES_PER_WRITE * 0));
				ref1 = (unsigned short *)(rb_waddr(&input_ref) + (ASP_CHANNEL_BYTES_PER_WRITE * 1));
#endif

				mic0 = (unsigned short *)(rb_waddr(&input_mic) + (ASP_CHANNEL_BYTES_PER_WRITE * 0));
				mic1 = (unsigned short *)(rb_waddr(&input_mic) + (ASP_CHANNEL_BYTES_PER_WRITE * 1));
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
				mic2 = (unsigned short *)(rb_waddr(&input_mic) + (ASP_CHANNEL_BYTES_PER_WRITE * 2));
				mic3 = (unsigned short *)(rb_waddr(&input_mic) + (ASP_CHANNEL_BYTES_PER_WRITE * 3));
#endif

				n = 0;
				for(i=0;i<(SOURCE_HARDWARE_SAMPLES_PER_READ);i++)
				{
#if defined(CACHE_OPTIMIZATION)
					if(n%16==0)
					{
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (mic0));
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (mic1));

#if defined(CONFIG_MIC_ARRAY_4_CHAN)
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (mic2));
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (mic3));
#endif

#if !defined(CONFIG_LOOPBACK_AUDIO)
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (ref0));
						__asm__(
								"   .set    arch=mips32r2   \n"
								"   pref    30, 32(%0)       \n"
								:   : "r" (ref1));
#endif
					}
#endif

					mic0[n] = in_micarray[1];
					mic1[n] = in_micarray[0];
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
					mic2[n] = in_micarray[3];
					mic3[n] = in_micarray[2];
#endif

#if defined(COLOR_DYING)
					in_micarray[1] = 0x7fff;
					in_micarray[0] = 0x7fff;
					in_micarray[3] = 0x7fff;
					in_micarray[2] = 0x7fff;
#endif

#if !defined(CONFIG_LOOPBACK_AUDIO)
#if !defined(MIC2_REF2)					
					ref0[n] = in_micarray[4];
					ref1[n] = in_micarray[5];
#else
					ref0[n] = in_micarray[3];
					ref1[n] = in_micarray[2];
#endif
#endif
					n++;
					in_micarray += SOURCE_HARDWARE_CHANNELS;

					if((ASP_CHANNEL_SAMPLES_PER_WRITE-1)==(i%ASP_CHANNEL_SAMPLES_PER_WRITE))
					{
#if defined(DEBUG_DUMP2)
						if(last_convert_timestamp > 900)
						{
							if(mic0_fp)
							{
								fclose(mic0_fp);
								mic0_fp = NULL;
							}
						}
						else
						{
							if(mic0_fp)
								fwrite(mic0, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, mic0_fp);
						}
#endif
#if defined(DBG_OUTPUT_RAW_DATA)						
						if (wakeupStartArgsPtr->wakeup_mode == 1)
						{
#if !defined(CONFIG_LOOPBACK_AUDIO)
							fwrite(ref0, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, ref0_fp);
							fwrite(ref1, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, ref1_fp);
#endif

							fwrite(mic0, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, mic0_fp);
							fwrite(mic1, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, mic1_fp);
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
							fwrite(mic2, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, mic2_fp);
							fwrite(mic3, BYTES_PER_SAMPLE, ASP_CHANNEL_SAMPLES_PER_WRITE, mic3_fp);
#endif
						}
#endif
#if !defined(CONFIG_LOOPBACK_AUDIO)
						ref0 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
						ref1 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
#endif
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
						mic0 += ASP_CHANNEL_SAMPLES_PER_WRITE * 4;
						mic1 += ASP_CHANNEL_SAMPLES_PER_WRITE * 4;
						mic2 += ASP_CHANNEL_SAMPLES_PER_WRITE * 4;
						mic3 += ASP_CHANNEL_SAMPLES_PER_WRITE * 4;
#else
						mic0 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
						mic1 += ASP_CHANNEL_SAMPLES_PER_WRITE * 2;
#endif
						n = 0;
					}				
				}

				rb_ridx_inc(&source_hardware);
				rb_write_timestamp(&input_ref, ts);
				rb_widx_inc(&input_ref);
				rb_write_timestamp(&input_mic, ts);
				rb_widx_inc(&input_mic);
			}
		}
	}
}

unsigned long long last_asp_timestamp;
void *asp_thread(void *ptr)
{
	//int ret;
	int i = 0;
	unsigned short *ref_in;
	unsigned short *mic_in;
	unsigned short *asp_out;
	unsigned long long ts;

#if defined(SET_NICE)
	if (0 != _set_nice(-20))
		printf("asp_thread set nice failed\n");
#endif

#if defined(DEBUG_DUMP2)
	int j;

	asp_fp = fopen("/tmp/misc/asp.raw", "wb");
#endif

	//signed short *v1, *v2;
	showProcessThreadId("");
	while(1)
	{
    	// posix_thread_ops_listen_wait(wakeup_waitctl);
		if(rb_full(&asp))
		{
			usleep(10000);
//			printf("asp FULL\n");
		}
		else
		{
			if((rb_use(&input_ref)<1)||(rb_use(&input_mic)<1))
			{
				usleep(10000);
//				printf("asp WAIT\n");
			}
			else
			{
				ref_in = (unsigned short *)rb_raddr(&input_ref);
				mic_in = (unsigned short *)rb_raddr(&input_mic);
				asp_out = (unsigned short *)rb_waddr(&asp);

				ts = rb_read_timestamp(&input_mic);
				if(ts<last_asp_timestamp)
					mt_printk(LOGLEVEL_CRIT, "ASP BUG\n", 8);
				last_asp_timestamp = ts;

//				printf("asp DO\n");
				for(i=0;i<25;i++)
				{
					vepug_process_beammux(reg,(pcm_t *)mic_in,(pcm_t *)ref_in,(pcm_t *)asp_out);

#if defined(COLOR_DYING)
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
					for(j=0;j<(ASP_SAMPLES_PER_PROC * 4);j++)
#else
					for(j=0;j<(ASP_SAMPLES_PER_PROC * 2);j++)
#endif
					{
						mic_in[j] = 0x8000;
					}
#endif

					asp_out += ASP_SAMPLES_PER_PROC;
					ref_in += (ASP_SAMPLES_PER_PROC * 2);
#if defined(CONFIG_MIC_ARRAY_4_CHAN)
					mic_in += (ASP_SAMPLES_PER_PROC * 4);
#else
					mic_in += (ASP_SAMPLES_PER_PROC * 2);
#endif
				}

#if defined(DEBUG_DUMP2)
				if(last_asp_timestamp > 900)
				{
					if(asp_fp)
						fclose(asp_fp);
					asp_fp = NULL;
				}
				else
				{
					if(asp_fp)
						fwrite((unsigned short *)rb_waddr(&asp), BYTES_PER_SAMPLE, ASP_SAMPLES_PER_PROC * 25, asp_fp);
				}
#endif

#if defined(DBG_OUTPUT_RAW_DATA)
				if (wakeupStartArgsPtr->wakeup_mode == 1)
				{
					fwrite((unsigned short *)rb_waddr(&asp), BYTES_PER_SAMPLE, ASP_SAMPLES_PER_PROC * 25, asp_fp);
				}
#endif
				rb_ridx_inc(&input_ref);
				rb_ridx_inc(&input_mic);
				rb_write_timestamp(&asp, ts);
				inf("asp w timestamp %lld\n", ts);
				rb_widx_inc(&asp);
			}
		}
	}
}

char* ASP_acquire_data(int *samples, unsigned long long *ts)
{
#if defined(CONFIG_KWD_SENSORY) || defined(CONFIG_KWD_KITTAI) || defined(CONFIG_KWD_SKV)
	while(1)
	{
		if(rb_use(&asp)<1)
		{
			usleep(20000);
		//	printf("THF WAIT\n");
		}
		else
		{
			if(samples){
				*samples = (CONVERT_SAMPLES_PER_WRITE);
			}
			if(ts)
			{
				*ts = rb_read_timestamp(&asp);
			//	inf("asp timestamp %lld %d\n", *ts, rb_use(&asp));
				if(*ts <= drop_target_ts)
				{
			//		inf("drop %lld <= %lld\n", *ts, drop_target_ts);
					ASP_release_data();
					continue;
				}
			}
		//	printf("THF DO\n");
			return rb_raddr(&asp);
		}
	}
#else
	while(1)
	{
		if(rb_use(&asp)<5)
		{
			usleep(30000);
			//printf("KWD WAIT\n");
		}
		else
		{
			if(samples){
				*samples = (CONVERT_SAMPLES_PER_WRITE * 5);
			}
			//printf("KWD DO\n");
			return rb_raddr(&asp);
		}
	}
#endif
}

// #define BACK_UP_PATH "/tmp/rSnd.raw"
void ASP_clear_data(void){	
//	inf("asp data clear %lld", timestamp);
//	show_rt_time("ASP_clear_data",cslog);
	drop_target_ts = timestamp + 1;
	wakeupStartArgsPtr->isAspDataClearedFlag=1;
	return;
#if 0
	if (NULL != handle_hardware_audio) {
        snd_pcm_drain(handle_hardware_audio);
    }
	int i=0;
	for(i=0;i<ASP_BUF_CHUNKS;i++){
		if(rb_empty(&source_hardware) &&
	#if defined(CONFIG_LOOPBACK_AUDIO)
			rb_empty(&source_loopback) &&
	#endif		
			rb_empty(&input_mic) &&
			rb_empty(&input_ref) &&
			rb_empty(&asp)
		){
			inf("asp date is cleared!");
			memset(source_hardware.buf,0,source_hardware.chunk_size*source_hardware.depth);
			// memset(input_ref.buf,0,input_ref.chunk_size*input_ref.depth);
			// memset(input_mic.buf,0,input_mic.chunk_size*input_mic.depth);
			// memset(asp.buf,0,asp.chunk_size*asp.depth);			
			wakeupStartArgsPtr->isAspDataClearedFlag=1;
			return;	
		}
		int size=0;
		char *pData=ASP_acquire_data(&size);
#ifdef BACK_UP_PATH				
		fd_write_file(BACK_UP_PATH,pData,size,"wb+");
#endif	
		ASP_release_data();
		war("asp cleared chunks:%d",i+1);
	}
#endif
}

void ASP_release_data(void)
{
#if defined(CONFIG_KWD_SENSORY) || defined(CONFIG_KWD_KITTAI) || defined(CONFIG_KWD_SKV)
	rb_ridx_inc(&asp);
#else
	rb_ridx_inc5(&asp);
#endif
}

extern int vep_opt(void);
int vep_init(void);

#if defined(USE_REALTIME_SCHEDULER)
#include <sched.h>
struct _sched_param
{
	int _sched_priority;
};
static int _sched_setparam(pid_t pid, const struct _sched_param *param)
{
    return syscall(SYS_sched_setparam, pid, param);
}
static int _sched_setscheduler(pid_t pid, int sched, const struct _sched_param *param)
{
    return syscall(SYS_sched_setscheduler, pid, sched, param);
}
#endif

#if defined(CONFIG_ASP_VEP_OPT)
int vep_opt_init(void)
{
#define TARGET_TABLE_BASE 	0x20000000UL
#define TARGET_TABLE_LENGTH	0x1000

    void *base;
    unsigned long *func_tbl;
    int fd;

    fd = open("/tmp/.libvep", O_CREAT | O_RDWR | O_TRUNC);
    if(0 > fd)
    {
        printf("open libvep failed\n");
        return -1;
    }

    base = mmap((void *) TARGET_TABLE_BASE,
                TARGET_TABLE_LENGTH,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_ANONYMOUS | MAP_PRIVATE,
                fd,
                (off_t) 0);

    if (0 > ((int)base))
    {
        printf("MMAP failed\n");
        return -1;
    }

	return 0;
}
#endif

int ASP_init()
{
	int iRet;
	pthread_attr_t input_thread_attr;
	pthread_attr_t convert_thread_attr;
	pthread_attr_t asp_thread_attr;

#if defined(USE_REALTIME_SCHEDULER)
	struct _sched_param param;
	pid_t mypid = getpid();

	param._sched_priority = 1;
	_sched_setscheduler(mypid, SCHED_FIFO, &param);
#endif

#if 0
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
#endif

	rb_init();

	sw_resampler_init();

#if defined(CONFIG_ASP_VEP_OPT)
	if((0>vep_opt_init()) || (0>vep_opt()))
	{
        WAKEUP_ERR("VEP optimization failed\n");
        exit(-1);
	}
#endif

	vep_init();

#if defined(CONFIG_LOOPBACK_AUDIO)
	loopback_audio_init();
#endif

	hardware_audio_init();

#if 1
	pthread_attr_init(&input_thread_attr);
	pthread_attr_setstacksize(&input_thread_attr, 65536);

	iRet = pthread_create(&input_pthread, &input_thread_attr, input_thread, NULL);
	if(iRet != 0)
	{
		WAKEUP_ERR("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
#endif

#if 1
	pthread_attr_init(&convert_thread_attr);
	pthread_attr_setstacksize(&convert_thread_attr, 65536);

	iRet = pthread_create(&convert_pthread, &convert_thread_attr, convert_thread, NULL);
	if(iRet != 0)
	{
		WAKEUP_ERR("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
#endif

#if 1
	pthread_attr_init(&asp_thread_attr);
	pthread_attr_setstacksize(&asp_thread_attr, 65536);

	iRet = pthread_create(&asp_pthread, &asp_thread_attr, asp_thread, NULL);
	if(iRet != 0)
	{
		WAKEUP_ERR("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}

	//pthread_setaffinity_np(asp_pthread, sizeof(cpu_set_t), &cpuset);
#endif

	return 0;
}




extern void assign_profile(void *b);

#define VEP_PROFILE_PREFIX "/etc/"
//#define VEP_PROFILE "/etc/profile_4mic-9outs-circular-r33-AEC-AGC.bix"
int vep_init(void)
{
    unsigned int smem = 16000; //1792;
    void *mem;

    int i;
    err_t err;


#if defined(CONFIG_INPUT_MIC_HW2_REF_SW2) || defined(CONFIG_INPUT_MIC_HW2_REF_HW2)
    p = read_profile(VEP_PROFILE_PREFIX "profile_2mic-3outs-dist66-SAEC-AGC-2018-06-14.bix");
#elif defined(CONFIG_INPUT_MIC_HW3_REF_SW2) || defined(CONFIG_INPUT_MIC_HW3_REF_HW2)
    p = read_profile(VEP_PROFILE_PREFIX "profile_3mic-7outs-circular-radius35-SAEC-AGC24db-2018-07-09.bix");
#else
    p = read_profile(VEP_PROFILE_PREFIX "profile_4mic-9outs-circular-r33-AEC-AGC.bix");
#endif

//	p = read_profile(VEP_PROFILE);
//	printf(VEP_PROFILE);
//	printf("\n");

    if (p==NULL)
    {
        printf("VEP read_profile failed\n");
        exit(-2);
    }

    smem = vepug_get_hook_size();
    mem = malloc(smem);

    vepug_get_mem_size(p, reg, mem);

    free(mem);

    for (i = 0; i < NUM_MEM_REGIONS; i++)
    {
        reg[i].mem = smr[i] = (void *)malloc(reg[i].size);
        //fprintf(stderr, "I need %d bytes of memory in memory region %d to work.\n", reg[i].size, i + 1);
    }

    err = vepug_init(p, reg);

    if (err.err)
    {
        //fprintf(stderr, "Config error %s %d memb:%d pid:%d\n", __text_error[err.err], err.err, err.memb, err.pid);
        exit(-3);
    }

    return 0;
}

#endif
