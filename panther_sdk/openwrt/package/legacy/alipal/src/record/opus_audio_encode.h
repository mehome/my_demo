#ifndef _OPUS_AUDIO_ENCODE_H_
#define _OPUS_AUDIO_ENCODE_H_


sem_t encodeSem;

void StartNewEncode();

void SetEncodeState(int state);

unsigned int EncodeFifoLength();

unsigned int EncodeFifoSeek( unsigned char *buffer, unsigned int len);

#endif