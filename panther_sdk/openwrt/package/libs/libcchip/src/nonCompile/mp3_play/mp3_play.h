#ifndef __MP3_PLAY_H__
#define __MP3_PLAY_H__
#include <alsa/asoundlib.h>

int mp3_play(char *odev,char *path,int fmt,int samplerate,int chn);

#endif


