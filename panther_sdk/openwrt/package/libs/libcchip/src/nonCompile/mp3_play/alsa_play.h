#ifndef __ALSA_PLAY_H__
#define __ALSA_PLAY_H__


extern void *play_audio_pthread(void);
extern void playback_go(char *pData, unsigned long len);
//extern void alsa_init(void);
int alsa_init(char *odev,int format, int rate, int channels);
extern void alsa_close(void);
extern void snd_Play_Buf(char *buf, unsigned int count);
extern void PlayWav(char *pPath);

#endif

