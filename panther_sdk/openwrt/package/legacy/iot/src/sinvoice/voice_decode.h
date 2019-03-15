#ifndef __VOICE_DECODE_H__
#define __VOICE_DECODE_H__

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    void (*voice_decoder_start)(void);
    void (*voice_decoder_finish)( char *, char *);
} voice_decoder_cb;

int voice_decoder_init(char *company,voice_decoder_cb *vdc);
int voice_decode_put_data(char *buf,int size);
int voice_decode_deinit();

#ifdef __cplusplus
}
#endif

#endif
