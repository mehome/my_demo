#include "speech.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include "mpdcli.h"



int text_to_sound(char *buf)
{
    int ret  = 0;
    int state = MpdGetPlayState();
    warning("state:%d",state);
    if(state == MPDS_PLAY)
        MpdPause();

    char file[1024] = {0};
    snprintf(file, 1024, "/root/iot/%s.wav", buf);
    //wav_preload(file);
    exec_cmd2("aplay -D common %s",file);

	if(state == MPDS_PLAY) {
		warning("resume play...");
		MpdPlay(-1);
	}
	return 0;
}


int ColumnTextToSound(char *buf)
{
	int ret  = 0;

	char file[1024] = {0};
	snprintf(file, 1024, "/root/iot/%s.wav", buf);
	//wav_preload(file);
	if(IsMpg123PlayerPlaying())		//此时有TTS播放，需打断
	{
		set_intflag(1);
	}
	exec_cmd2("aplay -D common %s",file);

	return ret ;
}










