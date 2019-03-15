#ifndef _ALIPAL_CONVER_OPUS_H_
#define _ALIPAL_CONVER_OPUS_H_

int Init_Opus_Decode(void);

int Start_Opus_Decode(char *i_pData, int DataLen);

void End_Opus_Decdoe(void);

#endif