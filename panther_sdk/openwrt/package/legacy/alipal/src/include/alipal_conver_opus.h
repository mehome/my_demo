#ifndef _ALIPAL_CONVER_OPUS_H_
#define _ALIPAL_CONVER_OPUS_H_

#define FRAME_SAMPLES (320)

#define MAX_DATA_BYTES (320 * sizeof(short))

#define PCM_SAMPLE_FILE "/tmp/sample.pcm"

#define PCM_TO_OPUS "/tmp/sample.opus"


int AliPal_Conver_Opus(char *pcmf_path, char *opus_fpath);

int Init_Opus_Encode(void);

int Start_Opus_Encode(char *i_pData, int DataLen, char *o_pData);

int End_Opus_Encdoe(void);







#endif
