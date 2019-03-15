#if defined(CONFIG_ASP_SQE)
#define _GNU_SOURCE
#define __USE_GNU
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <alsa/asoundlib.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <semaphore.h>
#include <signal.h>

#include "mnpc_sqe.h"

#if defined(__PANTHER__)
#define error printf

//#define DBG_OUTPUT_RAW_DATA
#define USE_USB_AUDIO
#endif

static pthread_t sqe_pthread;
static pthread_t swr_pthread;
static pthread_t input_pthread;

static SwrContext *swr_loopback_audio = NULL;
static SwrContext *swr_audio = NULL;

static snd_pcm_t *handle_loopback_audio = NULL;
static snd_pcm_t *handle_audio = NULL;

#if defined(DBG_OUTPUT_RAW_DATA)
static FILE *fp1 = NULL;
static FILE *fp2 = NULL;
static FILE *fp3 = NULL;
static FILE *fp4 = NULL;
static FILE *fp5 = NULL;
#endif

int audio_init()
{
	int ret;
#if defined(__PANTHER__) && defined(USE_USB_AUDIO)
	char *device = "plughw:0,0";
#else
	char *device = "default";
#endif
	snd_pcm_hw_params_t *hw_params = NULL;

	if(snd_pcm_open(&handle_audio, device, SND_PCM_STREAM_CAPTURE, 0) < 0)
	{ 
		error("Error snd_pcm_open [ %s]", device);  
		goto err;
	} 
	if((ret = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		error("unable to malloc hw_params: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_any(handle_audio, hw_params)) < 0)
	{
		error("unable to setup hw_params: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_access(handle_audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		error("unable to set access mode: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_format(handle_audio, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		error("unable to set format: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_channels(handle_audio, hw_params, 2)) < 0)
	{
		error("unable to set channels: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_rate(handle_audio, hw_params, 44100, 0)) < 0)
	{
		error("unable to set samplerate: %s\n",snd_strerror(ret));
		goto err;
	}

	/*dir = 0;
	frames = 32;
	if((ret = snd_pcm_hw_params_set_period_size_near(handle_audio,hw_params, &frames, &dir))<0)
	{
		error("unable to set period size: %s\n", snd_strerror(ret));
		goto err;
	}
	frames = 4096; 
	if((ret = snd_pcm_hw_params_set_buffer_size_near(handle_audio,hw_params, &frames)) < 0) {
		error("unable to set buffer size: %s\n", snd_strerror(ret));
		goto err;
	}*/

	if((ret = snd_pcm_hw_params(handle_audio, hw_params)) < 0)
	{
		error("unable to set hw parameters: %s\n",snd_strerror(ret));
		goto err;
	}
	snd_pcm_hw_params_free(hw_params);
	return 0;

err:
	if(handle_audio != NULL)
	{
		snd_pcm_close(handle_audio);
	}
	return -1;
}
 
int loopback_audio_init()
{
	int ret;
	char *device = "plughw:Loopback,1,0";
	//char *device = "plughw:internal,1,0";
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
	if((ret = snd_pcm_hw_params_set_channels(handle_loopback_audio, hw_params, 2)) < 0)
	//if((ret = snd_pcm_hw_params_set_channels(handle_loopback_audio, hw_params, 1)) < 0)
	{
		error("unable to set channels: %s\n",snd_strerror(ret));
		goto err;
	}
	if((ret = snd_pcm_hw_params_set_rate(handle_loopback_audio, hw_params, 44100, 0)) < 0)
	//if((ret = snd_pcm_hw_params_set_rate(handle_loopback_audio, hw_params, 16000, 0)) < 0)
	{
		error("unable to set samplerate: %s\n",snd_strerror(ret));
		goto err;
	}

	/*dir = 0;
	frames = 32;
	if((ret = snd_pcm_hw_params_set_period_size_near(handle_loopback_audio,hw_params, &frames, &dir))<0)
	{
		error("unable to set period size: %s\n", snd_strerror(ret));
		goto err;
	}
	frames = 4096; 
	if((ret = snd_pcm_hw_params_set_buffer_size_near(handle_loopback_audio,hw_params, &frames)) < 0) {
		error("unable to set buffer size: %s\n", snd_strerror(ret));
		goto err;
	}*/

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
	return -1;
} 

int sqe_init()
{
	short ret;

	ret = MNPCSqeInit(2, 2, 2, 2);
	if (ret != 0)
	{
		printf("MNPCSqeInit failed.\n");
		return -1;
	}
	
	swr_loopback_audio = (void *) swr_alloc_set_opts(NULL,  // we're allocating a new context
					AV_CH_LAYOUT_MONO,  // out_ch_layout
					AV_SAMPLE_FMT_S16,    // out_sample_fmt
					16000,      // out_sample_rate
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
	swr_audio = (void *) swr_alloc_set_opts(NULL,  // we're allocating a new context
					AV_CH_LAYOUT_MONO,  // out_ch_layout
					AV_SAMPLE_FMT_S16,    // out_sample_fmt
					16000,      // out_sample_rate
					AV_CH_LAYOUT_STEREO, // in_ch_layout
					AV_SAMPLE_FMT_S16,   // in_sample_fmt
					44100,       // in_sample_rate
					0,                    // log_offset
					NULL);                // log_ctx
	if (swr_audio == NULL)
	{
		printf("swr_loopback_audio_alloc_set_opts failed.\n");
		return -1;
	}
	swr_init(swr_audio);

#if defined(DBG_OUTPUT_RAW_DATA)
	fp1 = fopen("/tmp/1.raw", "wb");
	fp2 = fopen("/tmp/2.raw", "wb");
	fp3 = fopen("/tmp/3.raw", "wb");
	fp4 = fopen("/tmp/4.raw", "wb");
	fp5 = fopen("/tmp/5.raw", "wb");
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
};

#define SOURCE_SAMPLE_RATE		44100
#define SOURCE_CHANNELS				2
#define SOURCE_BYTES_PER_SAMPLE		2
#define SOURCE_SAMPLES_PER_READ		(SOURCE_SAMPLE_RATE / 50)
#define SOURCE_BYTES_PER_READ		(SOURCE_SAMPLES_PER_READ * SOURCE_CHANNELS * SOURCE_BYTES_PER_SAMPLE)
#define SOURCE_BUF_CHUNKS			50

struct rb source_far;
struct rb source_near;

#define SWR_SAMPLE_RATE			16000
#define SWR_CHNNELS					1
#define SWR_BYTES_PER_SAMPLE		2
#define SWR_SAMPLES_PER_WRITE		(SOURCE_SAMPLES_PER_READ * SWR_SAMPLE_RATE / SOURCE_SAMPLE_RATE)
#define SWR_BYTES_PER_WRITE			(SWR_SAMPLES_PER_WRITE * SWR_CHNNELS * SWR_BYTES_PER_SAMPLE)
#define SWR_BUF_CHUNKS				4

struct rb swr_far;
struct rb swr_near;

#define SQE_SAMPLE_RATE			SWR_SAMPLE_RATE
#define SQE_CHNNELS				SWR_CHNNELS
#define SQE_BYTES_PER_SAMPLE	SWR_BYTES_PER_SAMPLE
#define SQE_SAMPLES_PER_WRITE	SWR_SAMPLES_PER_WRITE
#define SQE_BYTES_PER_WRITE		SWR_BYTES_PER_WRITE
#define SQE_BUF_CHUNKS				4

#define SQE_SAMPLES_PER_PROC		320

struct rb sqe;

static unsigned char srcfbuf[SOURCE_BUF_CHUNKS * SOURCE_BYTES_PER_READ];
static unsigned char srcnbuf[SOURCE_BUF_CHUNKS * SOURCE_BYTES_PER_READ];
static unsigned char swrfbuf[SWR_BUF_CHUNKS * SWR_BYTES_PER_WRITE];
static unsigned char swrnbuf[SWR_BUF_CHUNKS * SWR_BYTES_PER_WRITE];
static unsigned char sqebuf[SQE_BUF_CHUNKS * SQE_BYTES_PER_WRITE];
void rb_init(void)
{
	source_far.ridx = source_far.widx = 0;
	source_far.buf = srcfbuf;
	source_far.chunk_size = SOURCE_BYTES_PER_READ;
	source_far.depth = SOURCE_BUF_CHUNKS;

	source_near.ridx = source_near.widx = 0;
	source_near.buf = srcnbuf;
	source_near.chunk_size = SOURCE_BYTES_PER_READ;
	source_near.depth = SOURCE_BUF_CHUNKS;

	swr_far.ridx = swr_far.widx = 0;
	swr_far.buf = swrfbuf;
	swr_far.chunk_size = SWR_BYTES_PER_WRITE;
	swr_far.depth = SWR_BUF_CHUNKS;

	swr_near.ridx = swr_near.widx = 0;
	swr_near.buf = swrnbuf;
	swr_near.chunk_size = SWR_BYTES_PER_WRITE;
	swr_near.depth = SWR_BUF_CHUNKS;

	sqe.ridx = sqe.widx = 0;
	sqe.buf = sqebuf;
	sqe.chunk_size = SWR_BYTES_PER_WRITE;
	sqe.depth = SQE_BUF_CHUNKS;
}

int rb_empty(struct rb *prb)
{
	return (prb->widx == prb->ridx);
}

int rb_full(struct rb *prb)
{
	return ((((prb->widx) + 1) % prb->depth) == (prb->ridx));
}

int rb_use(struct rb *prb)
{
	int val;

	val = (prb->widx - prb->ridx);
	if(val < 0)
		val += prb->depth;

	return val;
}

unsigned char *rb_waddr(struct rb *prb)
{
	return &prb->buf[prb->widx * prb->chunk_size];
}

unsigned char *rb_raddr(struct rb *prb)
{
	return &prb->buf[prb->ridx * prb->chunk_size];
}

void rb_widx_inc(struct rb *prb)
{
	prb->widx = ((prb->widx + 1) % prb->depth);
}

void rb_ridx_inc(struct rb *prb)
{
	prb->ridx = ((prb->ridx + 1) % prb->depth);
}

unsigned char tmpbuf[SOURCE_BYTES_PER_READ];
void *source_thread(void *ptr)
{
	int len1, len2;
	
	while(1)
	{
		if(rb_full(&source_near) || rb_full(&source_far))
		{
			len2 = snd_pcm_readi(handle_loopback_audio, tmpbuf, SOURCE_SAMPLES_PER_READ);
			len1 = snd_pcm_readi(handle_audio, tmpbuf, SOURCE_SAMPLES_PER_READ);
			printf("DROP\n");
		}
		else
		{
			len2 = snd_pcm_readi(handle_loopback_audio, (void *) rb_waddr(&source_near), SOURCE_SAMPLES_PER_READ);
			len1 = snd_pcm_readi(handle_audio, (void *) rb_waddr(&source_far), SOURCE_SAMPLES_PER_READ);
			rb_widx_inc(&source_near);
			rb_widx_inc(&source_far);
#if defined(DBG_OUTPUT_RAW_DATA)
			fwrite(rb_waddr(&source_near), 1, len2*4, fp1);
			fwrite(rb_waddr(&source_far), 1, len1*4, fp2);
#endif
			//if(rb_use(&source_near)>=20)
			//	printf("(Use %d %d)\n", rb_use(&source_near), rb_use(&source_far));
		}

		if((len2!=len1)||(len2!=SOURCE_SAMPLES_PER_READ))
		{
			printf("Fatal error: %d %d, restart\n", len2, len1);
			//abort();
			if(handle_loopback_audio)
			{
				snd_pcm_close(handle_loopback_audio);
				handle_loopback_audio = NULL;
			}
			if(handle_audio)
			{
				snd_pcm_close(handle_audio);
				handle_audio = NULL;
			}
			loopback_audio_init();
			audio_init();
		}
	}
}

uint8_t all_zeros[SOURCE_BYTES_PER_READ];
void *swr_thread(void *ptr)
{
	int ret1, ret2;
	uint8_t *swr_near_in[2] = { (void *) 0, (void *) 0 };
	uint8_t *swr_near_out[2] = { (void *) 0, (void *) 0 };
	uint8_t *swr_far_in[2] = { (void *) 0, (void *) 0 };
	uint8_t *swr_far_out[2] = { (void *) 0, (void *) 0 };

	while(1)
	{
		if(rb_full(&swr_near) || rb_full(&swr_far))
		{
			usleep(10000);
			//printf("SWR FULL\n");
		}
		else
		{
			if(rb_empty(&source_near) || rb_empty(&source_far))
			{
				usleep(10000);
				//printf("SWR\n");
			}
			else
			{

				swr_far_in[0] = rb_raddr(&source_far);
				swr_far_out[0] = rb_waddr(&swr_far);

				ret2 = swr_convert(swr_audio, swr_far_out, SWR_SAMPLES_PER_WRITE
								   ,(const uint8_t **) swr_far_in, SOURCE_SAMPLES_PER_READ);

				if(!memcmp(all_zeros, rb_raddr(&source_near), sizeof(all_zeros)))
				{
					ret1 = ret2;
					memset(rb_waddr(&swr_near), 0, SWR_BYTES_PER_WRITE);
				}
				else
				{
					swr_near_in[0] = rb_raddr(&source_near);
					swr_near_out[0] = rb_waddr(&swr_near);

					ret1 = swr_convert(swr_loopback_audio, swr_near_out, SWR_SAMPLES_PER_WRITE
									   ,(const uint8_t **) swr_near_in, SOURCE_SAMPLES_PER_READ);
				}

				//printf("swr_convert: %d %d\n", ret2, ret1);
				if(ret1!=ret2)
				{
					printf("SWR ret %d %d\n", ret1, ret2);
					//abort();
				}

				//printf("SWR done\n");

				rb_ridx_inc(&source_near);
				rb_ridx_inc(&source_far);

				if(ret1<SWR_SAMPLES_PER_WRITE)
				{
					memmove(rb_waddr(&swr_near) + ((SWR_SAMPLES_PER_WRITE - ret1) * SWR_BYTES_PER_SAMPLE),
							rb_waddr(&swr_near),  ((SWR_SAMPLES_PER_WRITE - ret1) * SWR_BYTES_PER_SAMPLE));
				}

				if(ret2<SWR_SAMPLES_PER_WRITE)
				{
					memmove(rb_waddr(&swr_far) + ((SWR_SAMPLES_PER_WRITE - ret2) * SWR_BYTES_PER_SAMPLE),
							rb_waddr(&swr_far),  ((SWR_SAMPLES_PER_WRITE - ret2) * SWR_BYTES_PER_SAMPLE));
				}

#if defined(DBG_OUTPUT_RAW_DATA)
				fwrite(rb_waddr(&swr_near), 1, SWR_BYTES_PER_WRITE, fp3);
				fwrite(rb_waddr(&swr_far), 1, SWR_BYTES_PER_WRITE, fp4);
#endif
				rb_widx_inc(&swr_near);
				rb_widx_inc(&swr_far);
				//printf("* Use %d %d\n", rb_use(&source_near), rb_use(&source_far));
			}
		}
	}
}

//unsigned char sqe_out[1000];
void *sqe_thread(void *ptr)
{
	//int ret;
	int i = 0;
	unsigned char *near_in;
	unsigned char *far_in;
	unsigned char *sqe_out;
	//signed short *v1, *v2;

	while(1)
	{
		if(rb_full(&sqe))
		{
			usleep(10000);
			//printf("SQE FULL\n");
		}
		else
		{
			if(rb_empty(&swr_near) || rb_empty(&swr_far))
			{
				usleep(10000);
				//printf("SQE WAIT\n");
			}
			else
			{
				near_in = rb_raddr(&swr_near);
				far_in = rb_raddr(&swr_far);
				sqe_out = rb_waddr(&sqe);

				if(!memcmp(all_zeros, near_in, SWR_BYTES_PER_WRITE))
				{
					memcpy(sqe_out, far_in, SWR_BYTES_PER_WRITE);

#if defined(DBG_OUTPUT_RAW_DATA)
					fwrite(sqe_out, 1, SWR_BYTES_PER_WRITE, fp5);
#endif
				}
				else
				{
#if 0
					v2 = (signed short *) &tempbuf[300*1024];
					for(i=0;i<320;i++)
					{
						v2[i] /= 2;
					}
#endif

					MNPCSqeProc((short *) far_in
									  , (short *) near_in
									  , (short *) sqe_out);

#if defined(DBG_OUTPUT_RAW_DATA)
					fwrite(sqe_out, 1, SWR_BYTES_PER_WRITE, fp5);
#endif
				}

				rb_ridx_inc(&swr_near);
				rb_ridx_inc(&swr_far);
				rb_widx_inc(&sqe);
			}
		}
	}
}

char* ASP_acquire_data(int *samples, NULL)
{
	while(1)
	{
		if(rb_empty(&sqe))
		{
			usleep(10000);
			//printf("THF WAIT\n");
		}
		else
		{
			*samples = SQE_SAMPLES_PER_PROC;
			return rb_raddr(&sqe);
		}
	}
}

void ASP_release_data(void)
{
	rb_ridx_inc(&sqe);
}

int ASP_init(void)
{
	int iRet;
	pthread_attr_t source_thread_attr;
	pthread_attr_t swr_thread_attr;
	pthread_attr_t sqe_thread_attr;

	rb_init();

	sqe_init();

	loopback_audio_init();
	audio_init();

#if 1
	pthread_attr_init(&source_thread_attr);
	pthread_attr_setstacksize(&source_thread_attr, 65536);

	iRet = pthread_create(&input_pthread, &source_thread_attr, source_thread, NULL);
	if(iRet != 0)
	{
		printf("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
#endif

#if 1
	pthread_attr_init(&swr_thread_attr);
	pthread_attr_setstacksize(&swr_thread_attr, 65536);

	iRet = pthread_create(&swr_pthread, &swr_thread_attr, swr_thread, NULL);
	if(iRet != 0)
	{
		printf("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
#endif

#if 1
	pthread_attr_init(&sqe_thread_attr);
	pthread_attr_setstacksize(&sqe_thread_attr, 65536);

	iRet = pthread_create(&sqe_pthread, &sqe_thread_attr, sqe_thread, NULL);
	if(iRet != 0)
	{
		printf("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
#endif

	return 0;
}
#endif
