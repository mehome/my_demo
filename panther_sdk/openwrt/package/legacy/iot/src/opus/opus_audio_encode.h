#ifndef _OPUS_AUDIO_ENCODE_H_
#define _OPUS_AUDIO_ENCODE_H_
#include <semaphore.h>

sem_t encodeSem;

void OpusStartNewEncode();

void OpusSetEncodeState(int state);

unsigned int OpusEncodeFifoLength();

unsigned int OpusEncodeFifoSeek( unsigned char *buffer, unsigned int len);
int CreateOpusEncodePthread(void);
int  DestoryOpusEncodePthread(void);
#endif