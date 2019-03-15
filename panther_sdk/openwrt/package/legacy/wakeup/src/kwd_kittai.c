#if defined(CONFIG_KWD_KITTAI)

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <signal.h>
#include <alsa/asoundlib.h>

#include <console.h>

#include "asp.h"
#include "snowboy-detect.h"
#include "debug.h"

#include "interface.h"

#define RESOURE_FILE_NAME   "/etc/universal_detect.res"
#define RESOURE_MODEL_NAME  "/etc/hotword.umdl"
#define SENSITIVITY         "0.48"

#define SNOWBOY_CHUNK_SIZE  1600
#define BYTES_PER_SAMPLE    2

const int ALSA_REC_CHANNELS = 1;			//1;
const int ALSA_REC_FRAME_BITS = 16;
const int ALSA_REC_SAMPLE_RATE = 16000;		//44100 ;
const int ALSA_REC_BUF_TIME = 500000;// 500ms
//const int ALSA_REC_PERIOD_TIME = 10000; // 10ms
const int ALSA_REC_PERIOD_TIME = 200000; // 200ms
static unsigned short status, done=0;


#include <stdio.h>

#define DEBUG_PRINTF(fmt, args...) do { printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
	
#define DEBUG_ERROR(fmt, args...) do { printf("\033[1;31m""[%s:%d]"#fmt"\r\n""\033[0m", __func__, __LINE__, ##args); } while(0) 

#define cons NULL
//#define CONEXANT_20921

#if defined(CONEXANT_20921)


//#define TEST_CX20921

struct recorder* g_rec;


///recorder object
struct recorder {
    void (*on_data_ind)(char *data, unsigned long len, void *userdata);

    void * wavein_hdl;
    /* thread id may be a struct. by implementation
     * void * will not be ported!! */
    pthread_t rec_thread;
    /*void * rec_thread_hdl;*/

    char *audiobuf;
    int bits_per_frame;
    unsigned int buffer_time;
    unsigned int period_time;
    size_t period_frames;
    size_t buffer_frames;
    const char *alsa_rec_device;
    volatile sig_atomic_t running;
    void *userdata;
	size_t sz;
};

int prepareRecBuffer(struct recorder* rec) {
    rec->sz = (rec->period_frames * rec->bits_per_frame / 8);

    rec->audiobuf = (char*)malloc(rec->sz);
    if (!rec->audiobuf) {
        return -ENOMEM;
    }

    return 0;
}

void clearRecBuffer(struct recorder* rec) {
	memset(rec->audiobuf, 0, rec->sz);	
}


void freeRecBuffer(struct recorder* rec) {
    if (rec->audiobuf) {
        free(rec->audiobuf);
        rec->audiobuf = NULL;
    }
}


int setHwparams(struct recorder* rec) {
    snd_pcm_hw_params_t* params = NULL;
    int err = 0;
    unsigned int rate = 0;
    snd_pcm_format_t format;
    snd_pcm_uframes_t size;
    snd_pcm_t* handle = (snd_pcm_t*)rec->wavein_hdl;

    rec->buffer_time = ALSA_REC_BUF_TIME;
    rec->period_time = ALSA_REC_PERIOD_TIME;

    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(handle, params);

    if (err < 0) {
        DEBUG_ERROR("Broken configuration for this PCM");
        return err;
    }

    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        DEBUG_ERROR("Access type not available");
        return err;
    }

    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        DEBUG_ERROR("Sample format non available");
        return err;
    }

    err = snd_pcm_hw_params_set_channels(handle, params, ALSA_REC_CHANNELS);
    if (err < 0) {
        DEBUG_ERROR("Channels count non available");
        return err;
    }

    rate = ALSA_REC_SAMPLE_RATE;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
    if (err < 0) {
        DEBUG_ERROR("Set rate failed");
        return err;
    }

    if (rate != ALSA_REC_SAMPLE_RATE) {
        DEBUG_ERROR("Rate mismatch");
        return -EINVAL;
    }

    err = snd_pcm_hw_params_set_period_time_near(handle, params, &rec->period_time, 0);
    if (err < 0) {
        DEBUG_ERROR("set period time fail");
        return err;
    }

    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &rec->buffer_time, 0);
    if (err < 0) {
        DEBUG_ERROR("set buffer time failed");
        return err;
    }

    err = snd_pcm_hw_params_get_period_size(params, &size, 0);
    if (err < 0) {
        DEBUG_ERROR("get period size fail");
        return err;
    }

    rec->period_frames = size;
    err = snd_pcm_hw_params_get_buffer_size(params, &size);

    if (size == rec->period_frames) {
        DEBUG_ERROR("Can't use period equal to buffer size (%lu == %lu)",
            size, rec->period_frames);
        return -EINVAL;
    }

    rec->buffer_frames = size;
    rec->bits_per_frame = ALSA_REC_FRAME_BITS;

    /* set to driver */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        DEBUG_ERROR("Unable to install hw params:");
        return err;
    }

    return 0;
}

int setSwparams(struct recorder* rec) {
    int err = 0;
    snd_pcm_sw_params_t* swparams = NULL;
    snd_pcm_t* handle = (snd_pcm_t*)(rec->wavein_hdl);

    snd_pcm_sw_params_alloca(&swparams);
    err = snd_pcm_sw_params_current(handle, swparams);

    if (err < 0) {
        DEBUG_ERROR("get current sw para fail");
        return err;
    }

    err = snd_pcm_sw_params_set_avail_min(handle, swparams,
                                          rec->period_frames);

    if (err < 0) {
        DEBUG_ERROR("set avail min failed");
        return err;
    }

    if ((err = snd_pcm_sw_params(handle, swparams)) < 0) {
        DEBUG_ERROR("unable to install sw params:");
        return err;
    }

    return 0;
}


int openRecorder(struct recorder* rec) {
    int err = 0;

    do {
        err = snd_pcm_open((snd_pcm_t**)&rec->wavein_hdl, rec->alsa_rec_device, SND_PCM_STREAM_CAPTURE, 0);

        if (err < 0) { 
			DEBUG_ERROR("retrying: Can't open \"%s\" PCM device. %s", rec->alsa_rec_device, snd_strerror(err));
            break;
        } else {
            DEBUG_PRINTF("success open \"%s\" PCM device.", rec->alsa_rec_device);
        }

        err = setHwparams(rec);
        if (err) {
            break;
        }

        err = setSwparams(rec);
        if (err) {
            break;
        }

        return 0;
    } while (0);

    if (rec->wavein_hdl) {
        snd_pcm_close((snd_pcm_t*) rec->wavein_hdl);
    }

    rec->wavein_hdl = NULL;

    return err;
}


static ssize_t pcm_read(struct recorder* rec, u_char *data)
{
	ssize_t r;
	size_t result = 0;

	int count = rec->period_frames;
    snd_pcm_t* handle = (snd_pcm_t*)rec->wavein_hdl;
	int bits_per_frame = rec->bits_per_frame;

	while (count > 0) {
		r = snd_pcm_readi(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 100);
		} else if (r == -EINTR || r == -EPIPE || r == -ESTRPIPE) {
			snd_pcm_recover(handle, r, 1);
		} else if (r < 0) {
			DEBUG_ERROR("ERROR: %s\n", snd_strerror(r));
			break;
		}
		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}

	return result;
}

#ifdef TEST_CX20921
#include <sys/time.h>
int isTimeOut(long int startTime){
	long int endTime = 0;
	struct timeval tv; 
	gettimeofday(&tv,NULL);	
	endTime = tv.tv_sec;

	if ((endTime - startTime) > 5) {
		return 1;
	} else {
		return 0;
	}
}

int open_file(const char *name)
{
	int fd;

	if(access(name,0) == 0){
		DEBUG_PRINTF("remove %s",name);
		remove(name);
	}
	
	fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		printf("open error\n");
	}
	return fd;
}
#endif

char* ASP_CX20921_data(int *samples) {

#ifdef TEST_CX20921
	size_t sz = (g_rec->period_frames * g_rec->bits_per_frame / 8);

	int fd = open_file("/tmp/test.raw");


	struct timeval tv; 
	gettimeofday(&tv,NULL); 
	long int startTime = tv.tv_sec;

	while (1) {
		if (isTimeOut(startTime)) {
			DEBUG_PRINTF("is TimeOut..");
			close(fd);
			exit(-1);
		}

		int iLength = pcm_read(g_rec, g_rec->audiobuf);
	
		if (write(fd, g_rec->audiobuf, iLength*2) != iLength*2) 
		{
			printf("16k_capture.wav error\n");
			break;
		}

		memset(g_rec->audiobuf, sz, 0);
	}
#else

	int iLength = pcm_read(g_rec, g_rec->audiobuf);
	//DEBUG_PRINTF("pcm_read iLength:%d", iLength);
	*samples = iLength;

	return g_rec->audiobuf;
#endif
}


int CX20921CaptureInit(char *alsa_rec_device) {
    int ret = 0;
	g_rec = (struct recorder*)calloc(sizeof(struct recorder), 1);
	g_rec->alsa_rec_device = alsa_rec_device;

	do {
        ret = openRecorder(g_rec);
        if (ret) {
            break;
        }

        ret = prepareRecBuffer(g_rec);
        if (ret) {
            break;
        }

        return 0;
    } while (0);

    freeRecBuffer(g_rec);

    if (g_rec) {
        free(g_rec);
        g_rec = NULL;
    }

    return ret;
}

#endif

void * snowboy_start_routine(void *args){
	snowboy_main(args);
}

#if defined(CONFIG_KWD_KITTAI)
#ifdef TEST_CX20921
void ASP_clear_data(void){
	if (NULL != g_rec->wavein_hdl) {
        snd_pcm_drop(g_rec->wavein_hdl);
		snd_pcm_close(g_rec->wavein_hdl);
		g_rec->wavein_hdl = NULL;
		DEBUG_PRINTF("--------hotword restart -------");
		clearRecBuffer(g_rec);
		openRecorder(g_rec);
    }
}
#endif
#endif



int snowboy_main(void *args)
{
    int32 hotword = 0;
    int32 count = SNOWBOY_CHUNK_SIZE;	
    bool is_end = DUER_FALSE;
    int i = 0;
    char str[512];
    unsigned char *asp_audiobuf;
    int samples;
   
	//wakeupStartArgs_t *pStartArgs=(wakeupStartArgs_t *)args;
	//wakeupStartArgs_t *wakeupStartArgsPtr = (wakeupStartArgs_t *)args;
	 // ASP_args_t ASP_args={0};
  //ASP_args.otherArgs=pStartArgs;
    printf("snowboy_main\n");
	posixThreadOps_t *wakeup_waitctl = wakeupStartArgsPtr->wakeup_waitctl;
	if(!wakeup_waitctl){
		WAKEUP_ERR("wakeup_waitctl is NULL");
		return -1;
	}
	  
	setpriority(PRIO_PROCESS, 0, -15);
    /* Initialize audio */
    // CreateMsgPthread();
	//posixThreadOps_t *wakeup_waitctl = pStartArgs->wakeup_waitctl;

#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT) \
   || defined(CONFIG_ASP_SQE) || defined(CONFIG_ASP_YSZ) || defined (CONFIG_ASP_VCP)
    //ASP_init(&ASP_args);
    printf("ASP_init\n");
	ASP_init();
#endif

    SnowboyInit(RESOURE_FILE_NAME, RESOURE_MODEL_NAME, DUER_TRUE);

    SnowboySetSensitivity(SENSITIVITY);
	unsigned long long ts;

#if defined(CONEXANT_20921)
	CX20921CaptureInit("plughw:1,0");
#endif
    while (1)
    {
		posix_thread_ops_listen_wait(wakeup_waitctl);
		//DEBUG_PRINTF("posix_thread_ops_listen_wait");

#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT) \
|| defined(CONFIG_ASP_SQE) || defined(CONFIG_ASP_YSZ) || defined (CONFIG_ASP_VCP)
      //  ASP_acquire_data(&ASP_args);
         int samples=0;
        char *sqe_buf=NULL;
		status = 0;
		//printf("begin ASP_acquire_data\n");
		 sqe_buf = ASP_acquire_data(&samples, &ts);

		//DEBUG_PRINTF("ASP_args samples:%d", ASP_args.samples*2);
#else if defined(CONEXANT_20921)
		asp_audiobuf = ASP_CX20921_data(&samples);
#endif

#if 0
		if(getMutMicStatus()){
			continue;
		}
#endif
		if(wakeupStartArgsPtr->isMutiVadListening)
        {
			if (wakeupStartArgsPtr->pRecordDataInputCallback) {
				#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT) \
				|| defined(CONFIG_ASP_SQE) || defined(CONFIG_ASP_YSZ) || defined (CONFIG_ASP_VCP)
					//pStartArgs->pRecordDataInputCallback(ASP_args.buf,ASP_args.samples*2,(void *)pStartArgs->PortAudioMicrophoneWrapper_ptr);
				    wakeupStartArgsPtr->pRecordDataInputCallback(sqe_buf,samples*2,(void *)wakeupStartArgsPtr->PortAudioMicrophoneWrapper_ptr);
				#else if defined(CONEXANT_20921)
					wakeupStartArgsPtr->pRecordDataInputCallback(asp_audiobuf, samples*2, (void *)wakeupStartArgsPtr->PortAudioMicrophoneWrapper_ptr);
					 //wakeupStartArgsPtr->pRecordDataInputCallback(sqe_buf,samples*2,(void *)wakeupStartArgsPtr->PortAudioMicrophoneWrapper_ptr);
				#endif
			}
            else
            {
				//setListenStatus(0);
				wakeupStartArgsPtr->isMutiVadListening=1;
				usleep(50*1000);
			}
            if(wakeupStartArgsPtr->isMutiVadListening != 1)
            {
                goto SKIP_CHECK_HOT_WORD;
            }

		}        
#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT) || defined(CONFIG_ASP_SQE) || defined(CONFIG_ASP_YSZ) || defined (CONFIG_ASP_VCP)         
           hotword = SnowboyRunDetection((const int16_t *) sqe_buf, samples,is_end);
#else if defined(CONEXANT_20921)
            hotword = SnowboyRunDetection((const int16_t *) asp_audiobuf, samples ,is_end);
#endif
        if (hotword > 0) 
        {
			SnowboyReset();
			i++;
			sprintf(str,"echo %d > /tmp/wakeloop",i); system(str);
			DEBUG_PRINTF("--------hotword is detection -------");
			//system("mpc pause; cdb set playmode 0");
			if(wakeupStartArgsPtr->PortAudioMicrophoneWrapper_ptr)
            {
				uartd_fifo_write("duerSceneLedSet019");
			}
            
			wakeupStartArgsPtr->isWakeUpSucceed=1;
			wakeupStartArgsPtr->isMutiVadListening=1;
	        wakeupStartArgsPtr->isAspDataClearedFlag=0;
			//setListenStatus(1);
            //wakeupStartArgsPtr->vad_end =0;
			//wakeupStartArgsPtr->pStopAllAudio();
			/*if(pStartArgs->userTimerPtr){
				pStartArgs->userTimerPtr->trigger(pStartArgs->userTimerPtr, NULL);
			}*/
		}
SKIP_CHECK_HOT_WORD:
		
#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT) || defined(CONFIG_ASP_SQE) || defined(CONFIG_ASP_YSZ) || defined (CONFIG_ASP_VCP)
        ASP_release_data(); 
#endif

    }
    /* Clean up */
    disp(cons,"Done");

    return 0;
}
#endif

