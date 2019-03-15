#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <function/fifobuffer/fifobuffer.h>
#include <function/common/fd_op.h>
#include "mad_dec.h"
#include "alsa_play.h"



FT_FIFO *g_PlayAudioFifo=NULL;

int mp3_play(char *odev,char *path,int fmt,int samplerate,int chn){
	int err=0;
	char *pMp3Data=NULL;
	size_t nMp3Size=0;
	int ret=path_get_size(&nMp3Size,path);
	if(ret<0){
		err= -1;
		goto Exit1;
	}
	ret=fd_read_file(&pMp3Data,path);
	if(ret<0){
		err= -2;
		goto Exit1;
	}
	g_PlayAudioFifo = ft_fifo_alloc(nMp3Size);
	if(!g_PlayAudioFifo){
		err= -3;
		goto Exit1;
	}
	ret=alsa_init(odev,fmt,samplerate,chn);
	if(ret<0){
		err= -4;
		goto Exit2;
	}
	ret=decode(pMp3Data,nMp3Size);
	if(ret<0){
		err= -5;
		goto Exit3;
	}
Exit1:
	alsa_close();	
Exit2:
	ft_fifo_free(g_PlayAudioFifo);
Exit3:	
	free(pMp3Data);
	return err;
}


