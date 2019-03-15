#ifndef __HEADER_MPC_HEADER__
#define  __HEADER_MPC_HEADER__

#include <alsa/asoundlib.h>


int pcm_stream_init(snd_pcm_t **pcm);
int pcm_stream_input(snd_pcm_t *pcm, char *pcm_buf, int len);
int pcm_stream_close(snd_pcm_t *pcm);


#endif  /* __HEADER_MPC_HEADER__ */