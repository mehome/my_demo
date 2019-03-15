#include <stdio.h>  
#include <stdlib.h>  
#include <alsa/asoundlib.h>
  
//int main(int argc, char *argv[])  
void alesa_set(void)
{  
    int i;  
    int ret;  
    int buf[128];  
    unsigned int val;  
    int dir=0;  
    char *buffer;  
    int size;  
    snd_pcm_uframes_t frames;  
    snd_pcm_uframes_t periodsize;  
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params; 
    
      
    //1. ��PCM�����һ������Ϊ0��ζ�ű�׼����  
    ret = snd_pcm_open(&handle, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0);  
    if (ret < 0) {  
        perror("snd_pcm_open");  
        exit(1);  
    }  
      
    //2. ����snd_pcm_hw_params_t�ṹ��  
    ret = snd_pcm_hw_params_malloc(&hw_params);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_malloc");  
        exit(1);  
    }  
    //3. ��ʼ��hw_params  
    ret = snd_pcm_hw_params_any(handle, hw_params);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_any");  
        exit(1);  
    }  
    //4. ��ʼ������Ȩ��  
    ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_set_access");  
        exit(1);  
    }  
    //5. ��ʼ��������ʽSND_PCM_FORMAT_U8,8λ  
    ret = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_set_format");  
        exit(1);  
    }  
    //6. ���ò����ʣ����Ӳ����֧���������õĲ����ʣ���ʹ����ӽ���  
    //val = 44100,��Щ¼������Ƶ�ʹ̶�Ϊ8KHz  
      
  
    val = 16000;  
    ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &val, &dir);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_set_rate_near");  
        exit(1);  
    }  
    //7. ����ͨ������  
    ret = snd_pcm_hw_params_set_channels(handle, hw_params, 1);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params_set_channels");  
        exit(1);  
    }  
      
    /* Set period size to 32 frames. */  
    frames = 128;  
    periodsize = frames * 2;  
    ret = snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &periodsize);  
    if (ret < 0)   
    {  
         printf("Unable to set buffer size %li : %s\n", frames * 2, snd_strerror(ret));  
    }  
    periodsize /= 2;  
  
    ret = snd_pcm_hw_params_set_period_size_near(handle, hw_params, &periodsize, 0);  
    if (ret < 0)   
    {  
        printf("Unable to set period size %li : %s\n", periodsize,  snd_strerror(ret));  
    }  
                                    
    //8. ����hw_params  
    ret = snd_pcm_hw_params(handle, hw_params);  
    if (ret < 0) {  
        perror("snd_pcm_hw_params");  
        exit(1);  
    }  
      
     /* Use a buffer large enough to hold one period */  
    snd_pcm_hw_params_get_period_size(hw_params, &frames, &dir);  
                                  
    size = frames * 2; /* 2 bytes/sample, 2 channels */  
    buffer = (char *) malloc(size);  
    fprintf(stderr,  "frames = %d, size = %d\n",  frames, size);  
      
           
    //10. �ر�PCM�豸���  
    snd_pcm_close(handle);  
      
    return 0;  
}  
