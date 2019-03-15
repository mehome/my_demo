//
//  pal_sample.c
//  pal_sdk
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include "config.h"
#include "pal.h"
#include "cJSON.h"

//#include "opus_codec.h"
#define OPUS_SAMPLE_FILE  "/tmp/sample.opus"
#define PCM_SAMPLE_FILE  "/tmp/sample.pcm"

int AliPal_Voice_Handle(int argc, const char * argv[]) 
{
	int i, ret, read;
    int sum = 0, count = 0;
	int raw_len = 640;
	
	struct timeval opus_begin;
    gettimeofday(&opus_begin, NULL);

    if(pal_asr_start() != 0)
		printf("[VOICE_DEBUG]pal_asr_start() fail!\n");
	
	FILE *fp = NULL;

	fp = fopen(OPUS_SAMPLE_FILE, "rb");
    if (!fp) 
	{
        printf("[VOICE_DEBUG]fopen %s failed!\n", OPUS_SAMPLE_FILE);
		
        return -1;
    }
		
    unsigned char *raw = malloc(raw_len * sizeof(unsigned char));
  
    while ((read = fread(raw, sizeof(unsigned char), raw_len, fp)) != 0) 
	{
        sum += read;
	
        ret = pal_asr_send_buffer((char *)raw, read * sizeof(unsigned char));
		
		if (ret == PAL_VAD_STATUS_STOP) 
		{
            printf("[VOICE_DEBUG] Detect PAL_VAD_STATUS_STOP\n");
			
            break;
        }
    }
	
    free(raw);
	
    fclose(fp);
	
	struct pal_rec_result *result = pal_asr_stop();
	if (result) 
	{
		AliPal_Parse_tts(result);
	
		printf("====result->raw= %s\n", result->raw);
		printf("====result->status = %d\n", result->status);
		printf("====result->should_restore_player_status = %d\n", result->should_restore_player_status);
		printf("====result->asr_result = %s\n", result->asr_result);
		printf("====result->task_status = %d\n", result->task_status);
		printf("====result->tts = %s\n", result->tts);
	
		pal_rec_result_destroy(result);	
    }
	
	struct timeval opus_end;
    gettimeofday(&opus_end, NULL);
    int elapse = ((opus_end.tv_sec - opus_begin.tv_sec) * 1000000 + (opus_end.tv_usec - opus_begin.tv_usec));
    printf("[VOICE_DEBUG]##########opus sum:%d, time:%dus\n",sum, elapse);	

	return 0;
	
}

#if 0
static void douglas_asr_test(int format)
{
	int i, ret;
	struct timeval opus_begin;
    gettimeofday(&opus_begin, NULL);

   
    if(pal_asr_start() == 0)
		printf("[VOICE_DEBUG]pal_asr_start() success!\n");
	else
		printf("[VOICE_DEBUG]pal_asr_start() fail!\n");
	
	FILE *fp = NULL;

	if (format == PAL_AUDIO_FORMAT_OPUS) 
	{
        fp = fopen(OPUS_SAMPLE_FILE, "rb");
		if(fp) printf("[VOICE_DEBUG]fopen %s success!\n", OPUS_SAMPLE_FILE);
    } 
	else if (format == PAL_AUDIO_FORMAT_PCM)
	{
        fp = fopen(PCM_SAMPLE_FILE, "rb");
		if(fp) printf("[VOICE_DEBUG]fopen %s success!\n", PCM_SAMPLE_FILE);
	
    } 
	else 
	{
        printf("unsupport format\n");
    }
	
    if (!fp) 
	{
        printf("No file opened\n");
		
        return;
    }
	
    int file_size;
	
    fseek(fp, 0L, SEEK_END);
	
    file_size = ftell(fp);
	
    fseek(fp, 0L, SEEK_SET);

	int raw_len = 640;
	
	printf("====================================================== begin\n");
	
    unsigned char *raw = malloc(raw_len * sizeof(unsigned char));
	
    int read;
    int sum = 0;
	int count = 0;

    while ((read = fread(raw, sizeof(unsigned char), raw_len, fp)) != 0) 
	{
        sum += read;
		
        //usleep(10 * 1000);
		printf("[VOICE_DEBUG]Befor pal_asr_send_buffer()\n");
        ret = pal_asr_send_buffer((char *)raw, read * sizeof(unsigned char));
		printf("[VOICE_DEBUG]After pal_asr_send_buffer()\n");
		
		if (ret == PAL_VAD_STATUS_STOP) 
		{
            printf("[VOICE_DEBUG] Detect PAL_VAD_STATUS_STOP\n");
			
            break;
        }
		
    }
	
	printf("===================================================== end sum: %d\n", sum);
	
    free(raw);
	
    fclose(fp);
	
	printf("[VOICE_DEBUG]Befor pal_asr_stop()\n");
	struct pal_rec_result *result = pal_asr_stop();
	AliPal_Parse_tts(result);
	printf("[VOICE_DEBUG]After pal_asr_stop()\n");
	
	printf("====result->raw= %s\n", result->raw);
	printf("====result->status = %d\n", result->status);
	printf("====result->should_restore_player_status = %d\n", result->should_restore_player_status);
	printf("====result->asr_result = %s\n", result->asr_result);
	printf("====result->task_status = %d\n", result->task_status);
	printf("====result->tts = %s\n", result->tts);
	
	if (result) 
	{
		pal_rec_result_destroy(result);
		//free(result);
    }
	
	struct timeval opus_end;
    gettimeofday(&opus_end, NULL);
    int elapse = ((opus_end.tv_sec - opus_begin.tv_sec) * 1000000 + (opus_end.tv_usec - opus_begin.tv_usec));
    printf("[VOICE_DEBUG]##########opus time %dus\n",elapse);	

}
#endif

static void pal_callback_fn(const char *cmd, int cmd_type, char *buffer, int buffer_capacity, void *user)
{
	
	printf("\033[31;43m[VOICE_DEBUG] %s cmd:%s \033[0m\n", __func__, cmd);  

	AliPal_Parse_Voice(cmd);
	
}

int AliPal_Voice_Config(void)
{
	///int format = PAL_AUDIO_FORMAT_PCM;
	int format = PAL_AUDIO_FORMAT_OPUS;
    struct pal_config config;
	
    memset(&config, 0, sizeof(struct pal_config));

	config.ca_file_path = "/alinkfile/ca.pem"; //ca.pem路径，如/tmp／ca.pem

	config.format = format;

	config.sample_rate = 16000;

	config.channels = 1;

	config.bits_per_sample = 16;

	config.callback = pal_callback_fn;
	
	if(pal_init(&config) != 0)
		printf("[VOICE_DEBUG]pal_init() fail!\n");

	//if(pal_asr_start() != 0)
	//		printf("[VOICE_DEBUG]pal_asr_start() fail!\n");
	
}

#if 0
int AliPal_Voice_Handle(int argc, const char * argv[]) 
{
    printf("---[VOICE_DEBUG]Entry %s()----%d---\n",__FUNCTION__, __LINE__);

	int ret;
	/*
	///int format = PAL_AUDIO_FORMAT_PCM;
	int format = PAL_AUDIO_FORMAT_OPUS;
    struct pal_config config;
	
    memset(&config, 0, sizeof(struct pal_config));

	config.ca_file_path = "/alinkfile/ca.pem"; //ca.pem路径，如/tmp／ca.pem

	config.format = format;

	config.sample_rate = 16000;

	config.channels = 1;

	config.bits_per_sample = 16;

	config.callback = pal_callback_fn;
	
	if(pal_init(&config) == 0)
		printf("[VOICE_DEBUG]pal_init() success!\n");
	else
		printf("[VOICE_DEBUG]pal_init() fail!\n");
    
    //sleep(5);
	*/
    //douglas_asr_test(format);
	douglas_asr_test(PAL_AUDIO_FORMAT_OPUS);
    
    //sleep(5);
	
	
#if 0
	printf("[VOICE_DEBUG]Befor pal_get_tts()\n");

	struct pal_rec_result* result =  pal_get_tts("我要听刘德华的歌");
	printf("====result->raw= %s\n", result->raw);
	printf("====result->status = %d\n", result->status);
	printf("====result->should_restore_player_status = %d\n", result->should_restore_player_status);
	printf("====result->asr_result = %s\n", result->asr_result);
	printf("====result->task_status = %d\n", result->task_status);
	printf("====result->tts = %s\n", result->tts);
	
	printf("[VOICE_DEBUG]After pal_get_tts()\n");
    if (result)
	{
        printf("===== %s\n", result->raw);
    }
#endif
    /*while (1) 
	{
        //sleep(1);
        pause();
    }*/
	
	//opus_codec_destroy();
	
    return 0;
	
}
#endif


int Init_Opus_Decode(void)
{
	int iRet = 0;
	iRet = pal_asr_start();
	if(iRet < 0)
	{
		error("[VOICE_DEBUG]pal_asr_start() fail!\n");
		return -1;
	}
	
	return 0;	
}

int Start_Opus_Decode(char *i_pData, int DataLen)
{
	int iRet = 0;
	
	iRet = pal_asr_send_buffer(i_pData, DataLen * sizeof(unsigned char));
	
	return iRet;
}

void End_Opus_Decdoe(void)
{	
	struct pal_rec_result *result;

	result = pal_asr_stop();
	if (result) 
	{
		AliPal_Parse_tts(result);	
		printf("====result->raw= %s\n", result->raw);
		printf("====result->status = %d\n", result->status);
		printf("====result->should_restore_player_status = %d\n", result->should_restore_player_status);
		printf("====result->asr_result = %s\n", result->asr_result);
		printf("====result->task_status = %d\n", result->task_status);
		printf("====result->tts = %s\n", result->tts);
		
    }

	pal_rec_result_destroy(result);

}




