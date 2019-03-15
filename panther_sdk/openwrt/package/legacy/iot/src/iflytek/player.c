#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alsa/asoundlib.h>
#include <memory.h>
#include <alloca.h>
#include <time.h>
#include <fcntl.h>
#include "myutils/debug.h"

snd_pcm_uframes_t   frames;

snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;  
size_t bits_per_frame ; 
int channels = 1;
size_t chunk_bytes; 



int pcm_stream_init(snd_pcm_t **pcm) 
{
    int rc;
    snd_pcm_hw_params_t *params = NULL;
	size_t bits_per_sample; 
    //snd_pcm_uframes_t   frames;
    unsigned int val = 16000;
    int dir = 0;

    rc = snd_pcm_open(pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc < 0)
    {
        fprintf(stderr,"Failed to open PCM device.\n");
        return -1;
    }

    snd_pcm_hw_params_alloca(&params);
    
    rc = snd_pcm_hw_params_any(*pcm, params);
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params_any\n");
        return -1;
    }
    rc = snd_pcm_hw_params_set_access(*pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params_set_access.\n");
        return -1;
    }
    
    snd_pcm_hw_params_set_format(*pcm, params, format);

		/* 1 for single channel, 2 for stereo */
    rc = snd_pcm_hw_params_set_channels(*pcm, params, channels); 
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params_set_channels\n");
        return -1;
    }

    rc = snd_pcm_hw_params_set_rate_near(*pcm, params, &val, &dir); 
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params_set_rate_near\n");
        return -1;
    }

    rc = snd_pcm_hw_params(*pcm, params);
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params\n");
        return -1;
    }

    rc = snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    //printf("frames = %ld\n", (long)frames );
    if(rc < 0)
    {
        fprintf(stderr,"Failed to snd_pcm_hw_params_get_period_size\n");
       
    }
	bits_per_sample = snd_pcm_format_physical_width(format);  
    bits_per_frame = bits_per_sample * channels;  

	chunk_bytes = frames * bits_per_frame / 8;
	 
    if ((rc = snd_pcm_prepare (*pcm)) < 0) {  
            fprintf (stderr, "cannot prepare audio interface for use (%s)\n",  
                 snd_strerror (rc));  
            return rc; 
    }  
    return 0;
}


int pcm_stream_input(snd_pcm_t *pcm, char *pcm_buf, int len)
{
    int rc = -1;
   		
#if 1
    while( (rc = snd_pcm_writei(pcm, pcm_buf, len))<0)
    {
    	if(IsIflytekCancled()) {
			info("pcm_stream_input  Cancle...");
			break;
    	}
        usleep(20000); 

        if(rc == -EAGAIN )  
        {
            int ms = 20;
            struct  timespec ts;
            ts.tv_sec = ms / 1000;
            ts.tv_nsec = (ms % 1000) * 1000000;
           	nanosleep(&ts, NULL);
            // snd_pcm_wait(pcm, 1000); 
        }  
        else if (rc == -EPIPE)
        {
              fprintf(stderr, "underrun occurred\n");
              snd_pcm_prepare(pcm);
        } 
        else
        {
          fprintf(stderr,"error from writei: %s\n",snd_strerror(rc));
        }  
    }
#else
	size_t wcount;  
	 if (len < frames) {  
        snd_pcm_format_set_silence(format,   
            pcm_buf + len * bits_per_frame / 8,   
            (frames - len) * channels);  
        wcount = frames;  
    } 
    while (wcount > 0) {  
     	rc = snd_pcm_writei(pcm, pcm_buf, wcount);  
        if (rc == -EAGAIN || (rc >= 0 && (size_t)rc < wcount)) {  
            snd_pcm_wait(pcm, 1000);  
        } else if (rc == -EPIPE) {  
            snd_pcm_prepare(pcm);  
            DEBUG_ERR( " Buffer Underrun ");           
        } else if (rc < 0) {  
            DEBUG_ERR( "Error snd_pcm_writei: [%s]", snd_strerror(rc));  
            //break;  
        }  
        if (rc > 0) { 
            wcount -= rc;  
            pcm_buf += rc * bits_per_frame / 8;  
        }  
    } 
	#endif

    return 0;
}


int pcm_stream_close(snd_pcm_t *pcm) 
{
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    return 0;
}



