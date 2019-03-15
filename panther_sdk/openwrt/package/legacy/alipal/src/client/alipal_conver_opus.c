#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "opus.h"
#include "opus_defines.h"

#include "alipal_conver_opus.h"

static OpusEncoder* mp_opuEncoder = NULL;
static int m_frameSamples = FRAME_SAMPLES;
static int m_maxDataBytes = MAX_DATA_BYTES;
static int m_vbr = 1;
static int m_bitRate = 27800;
static int m_complexity = 8;
static int m_signal = OPUS_SIGNAL_VOICE;
static int m_sample_rate = 16000;
static int m_channels = 1;

static int initialized = 0;

int opus_codec_init(int sample_rate, int channels, int VBR, int bit_rate,
                    int complexity, int signal) 
{
    ///printf("sample_rate %d, channels %d, vbr %d, bit_rate %d, complexity %d, signal %d\n", sample_rate, channels, VBR, bit_rate, complexity, signal);
    if (initialized) 
	{
        printf("Opus codec initialized\n");
        return 0;
    }
	
    int error = 0;
    m_sample_rate = sample_rate;
    m_channels = channels;
    
    //mp_opuEncoder = opus_encoder_create(m_sample_rate, m_channels,
    //                                    OPUS_APPLICATION_VOIP, &error);
    mp_opuEncoder = opus_encoder_create(m_sample_rate, m_channels,
                                        OPUS_APPLICATION_RESTRICTED_LOWDELAY, &error);
    if (!mp_opuEncoder) {
        //throw exception FIXME
        printf("opus_codec_init create opuEncoder fail!\n");
        error = -1;
    } else {
        printf("opus_codec_init create opuEncoder success!\n");
        //should we configure these seperately ? FIXME
        
        m_vbr = VBR;
        m_bitRate = bit_rate;
        m_complexity = complexity;
        m_signal = signal;
        int ret = OPUS_OK;
        ret += opus_encoder_ctl(mp_opuEncoder, OPUS_SET_VBR(m_vbr));
        ret += opus_encoder_ctl(mp_opuEncoder, OPUS_SET_BITRATE(m_bitRate));
        ret += opus_encoder_ctl(mp_opuEncoder,
                                OPUS_SET_COMPLEXITY(m_complexity));
        ret += opus_encoder_ctl(mp_opuEncoder, OPUS_SET_SIGNAL(m_signal));
        if (OPUS_OK == ret) {
            printf("set param success!\n");
            initialized = 1;
        } else {
            error = -1;
            initialized = 0;
            opus_encoder_destroy(mp_opuEncoder);
            mp_opuEncoder = NULL;
            printf("set param failed. destroy mp_opuEncoder\n");
        }
    }
    return error;
}

int opus_codec_destroy() {
    //printf("opus_codec_destroy++\n");
    if (!initialized) {
        printf("Opus codec is not initialized\n");
        return 0;
    }
    if (mp_opuEncoder) {
        opus_encoder_destroy(mp_opuEncoder);
    }
    mp_opuEncoder = NULL;
    initialized = 0;
    //printf("opus destroy--\n");
    return 0;
}

int opus_codec_encode(const char *sample_data, int sample_data_len, unsigned char *buffer,
                      int buffer_capacity, int *buffer_len) {
    //printf("------ Encode buffer %d bytes ------\n", sample_data_len);
    //struct timeval begin;
    //gettimeofday(&begin, NULL);
    if (!initialized) {
        return -1;
    }
    if (!sample_data || sample_data_len <= 0 || !buffer || buffer_capacity <= 0) {
        return -1;
    }
    short *samples = (short *)sample_data;
    if (sample_data_len != (FRAME_SAMPLES * sizeof(short))) {
        printf("sample len should be %d, cannot be %d\n", m_frameSamples * sizeof(short), sample_data_len);
        return -1;
    }
    if (buffer_capacity < sample_data_len) {
        return -1;
    }
    if (!initialized) {
        printf("opus Codec not initialized properly");
        return -1;
    }
    *buffer_len = 0;
    //if encoder is NULL FIXME
    if (NULL != mp_opuEncoder) {
        //length is frame_size*channels*sizeof(opus_int16)
        int ret = opus_encode(mp_opuEncoder, samples, m_frameSamples, buffer,
                              m_maxDataBytes);
        //printf("------ encode ret %d ------\n", ret);
        if (ret < 0) {
            *buffer_len = 0;
            printf("encode err, code = %d\n", ret);
            return -1;
        }
        *buffer_len = ret;
    } else {
        printf("mp_opuEncoder is NULL. should not happen.\n");
    }
    //printf("------ encode success ------\n");
    //struct timeval end;
    //gettimeofday(&end, NULL);
    //int elapse = ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec));
    //printf("------ time consumed %d microsecond ------\n", elapse);
    return 0;
}

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i&0xFF;
    ch[1] = (i>>8)&0xFF;
    ch[2] = (i>>16)&0xFF;
    ch[3] = i>>24;
}

int AliPal_Conver_Opus(char *pcm_fpath, char *opus_fpath)
{
	int ret = 0, file_size = 0;
	FILE *fp = NULL;
	FILE *fd = NULL;

	if(opus_codec_init(m_sample_rate, m_channels, m_vbr, m_bitRate, m_complexity, m_signal) < 0)
	{
		printf("[VOICE_DEBUG]opus_codec_init fialed!\n");
	}
   
    fp = fopen(pcm_fpath, "rb");
    if (!fp) 
	{
       	printf("[VOICE_DEBUG]open %s file failed!\n", PCM_SAMPLE_FILE);
        return -1;
    }

	fd = fopen(opus_fpath, "wb");
	if(!fd)
	{
		printf("[VOICE_DEBUG]open %s file failed!\n", PCM_TO_OPUS);
		return -1;
	}
  
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    int raw_len = 320;
    short *raw = malloc(raw_len * sizeof(short));
    int read;
    int sum = 0;
    int buffer_capacity = 1024;
    char buffer[1024] = {0};
    int buffer_len = 0;
    unsigned char int_field[4];
	int i;
    opus_int16 indata[FRAME_SAMPLES];
	
	struct timeval begin;
    gettimeofday(&begin, NULL);

	while ((read = fread(raw, sizeof(short), raw_len, fp)) != 0) {
        sum += read;
      
		for(i=0;i<FRAME_SAMPLES;i++)
			indata[i]=raw[i]<<8 | raw[i]>>8;
		int enc_ret = opus_codec_encode(indata, raw_len * sizeof(short), buffer, buffer_capacity, &buffer_len);
       
		int_to_char(buffer_len, int_field);
		
		//write opus data header
		if(fwrite(int_field, sizeof(unsigned char), sizeof(unsigned char), fd) < 1)
		{
			//printf("[VOICE_DEBUG] fwrite %s data header failed!\n", PCM_TO_OPUS);
			return -1;
		}
		
		//write opus data
		if(fwrite(buffer, sizeof(unsigned char), buffer_len, fd) < 1)
		{
			//printf("[VOICE_DEBUG]fwrite %s data failed!\n", PCM_TO_OPUS);
			return -1;
		}
		
		///usleep(10 * 1000);
		
    }

	fseek(fd, 0L, SEEK_END);
	
    int opus_fsize = ftell(fd);

	fseek(fd, 0L, SEEK_SET);

	struct timeval end;
    gettimeofday(&end, NULL);
    int elapse = ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec));
    printf("[VOICE_DEBUG] %dB pcm encode %dB opus time %dus\n",file_size, opus_fsize, elapse);
	
    free(raw);
    
	fclose(fp);
	
	fclose(fd);
    
    ret = opus_codec_destroy();
	
    ///printf("------ destroy ret: %d ------\n", ret);

	return 0;
	
}




int Init_Opus_Encode(void)
{
	int iRet = 0;

	iRet = opus_codec_init(m_sample_rate, m_channels, m_vbr, m_bitRate, m_complexity, m_signal);
	if(iRet < 0)
		printf("[VOICE_DEBUG]opus_codec_init() fialed!\n");
	
	return iRet;
}

int Start_Opus_Encode(char *i_pData, int DataLen, char *o_pData)
{
	int i, iRet, buffer_len = 0;

	short *raw = malloc(FRAME_SAMPLES * sizeof(short));

	opus_int16 indata[FRAME_SAMPLES];

	memcpy(raw, i_pData, DataLen);

	for(i=0;i<FRAME_SAMPLES;i++)
		indata[i]=raw[i]<<8 | raw[i]>>8;
	
	iRet = opus_codec_encode(indata, FRAME_SAMPLES * sizeof(short), o_pData, 1024, &buffer_len);
	if(iRet < 0)
	{
		printf("[VOICE_DEBUG]opus_codec_encode() failed!\n");
		return -1;
	}
	
	free(raw);

	return buffer_len;
	
}

int End_Opus_Encdoe(void)
{
	int iRet;
	
	iRet = opus_codec_destroy();
	if(iRet < 0)
	{
		printf("[VOICE_DEBUG]opus_codec_encode() fialed!\n");
		return -1;
	}
	
	return 0;
}



