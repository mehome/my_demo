#ifndef __ALSA_PLAY_H__
#define __ALSA_PLAY_H__

extern void alsa_init(void);
extern void alsa_close(void);
extern void playback_go(char *pData, unsigned long len);



#endif

