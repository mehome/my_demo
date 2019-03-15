#include <stdio.h>
#include <stdlib.h>
#include "cc_box.h"
#include <libcchip/platform.h>
// #include <libcchip/function/mp3_play/mp3_play.h>

extern int WavRecorder(char *filename, unsigned int sec);
int wavRecorder_handle(char *argv[]){
	inf("enter %s,Recorder %d sec wav to %s begin!",__func__,atoi(argv[2]),argv[1]);
	int err=WavRecorder(argv[1],atoi(argv[2]));
	if(err<0){
		err("WavRecorder failure ,err=%d",err);
		return -1;
	}
	return 0;
}

int wavplayer_handle(char *argv[]){
	if(!argv[1] || !strcmp(argv[1],"--help")){
		raw("---wavpalyer help!---compile in:"__DATE__" "__TIME__"---\n");
		raw("               [file path] : play file!\n");
		raw("--load         [file path] : load file to memry for play!\n");
		raw("--no-load      [file path] : play file in flash!\n");
		raw("set [All] [0]  [63]        : set ALL 0 volume to 63%%!\n");
		return 0;
	}		
	int ret=strcmp(argv[1],"set");
	if(!ret){
		if(NULL==argv[4]){
			err("args num is too less!");
			return -2;
		}
		playbackCtl para={0};	
		para.name = argv[2];
		para.index = atoi(argv[3]);
		para.volume = atoi(argv[4]);
		do{
			ret=amixer_set_playback_volume(&para);
			if(ret){
				err("set volume failure,ret=%d",ret);
				return -3;
			}
			// sleep(1);
		}while(0);
		return 0;
	}
	ret=strcmp(argv[1],"--no-load");
	if(!ret){
		ret=wav_play(argv[2]);	
		if(ret)return -4;
		return 0;
	}
	ret=strcmp(argv[1],"--load");
	if(!ret){
		ret=wav_play_preload(argv[2]);	
		if(ret)return -5;
		return 0;
	}	
	ret=tone_wav_play(argv[1]);	
	if(ret)return -6;
	return 0;	
}
// #define ASR_FORMAT		SND_PCM_FORMAT_S16_LE
// #define ASR_SPEED		16000
// #define ASR_CHANNELS	1
// int mp3player_handle(char *argv[]){
	// if(!(argv[1]&&argv[1][0])){
		// raw("	--queue: user default args to play mp3 list,example:\n");
		// raw("		mp3player --queue /tmp/1.mp3 /tmp/2.mp3 \n");
		// raw("	--spec: user special args to play signel mp3 file, example:\n");
		// raw("		mp3player --spec default 44100 2 /tmp/res/wake_up.mp3\n");
		// return 0;
	// }
	// if(!strcmp("--spec",argv[1])){
		// assert_arg(4,-2);
		// char *dev=argv[2];
		// int asrBit=atoi(argv[3]);
		// int chl=atoi(argv[4]);
		// char *path=argv[5];		
		// inf("%s %s %d %d %d",dev,path,ASR_FORMAT,asrBit,chl);
		// int ret=mp3_play(dev,path,ASR_FORMAT,asrBit,chl);
		// if(ret<0){
			// return -3;
		// }
		// return 0;
	// }
	// if(!strcmp("--queue",argv[1])){
		// assert_arg(2,-4);
		// int i=0;
		// for(i=2;(argv[i] && argv[i][0]);i++){
			// int ret=mp3_play("wavplay",argv[i],ASR_FORMAT,ASR_SPEED,ASR_CHANNELS);
			// if(ret<0){
				// return -5;
			// }		
		// }
		// return 0;		
	// }
	// return 0;	
// }
static int nameCmdline_handle(char *argv[]){
	int i=0;
	assert_arg(2,-1);
	char *cmdline=get_name_cmdline(argv[1],atoi(argv[2]));
	if(cmdline){
		return -2;
	}
	clog(Hred,cmdline);
	return 0;
}
static int pidCmdline_handle(char *argv[]){
	int i=0;
	assert_arg(2,-1);
	char *cmdline=get_pid_cmdline(atoi(argv[1]),atoi(argv[2]));
	if(cmdline){
		return -2;
	}
	clog(Hred,cmdline);
	return 0;
}
static handle_item_t box_handle_array[]={
	ADD_HANDLE_ITEM(wavplayer,	NULL)
	ADD_HANDLE_ITEM(wavRecorder,NULL)
	// ADD_HANDLE_ITEM(mp3player,	NULL)
	ADD_HANDLE_ITEM(nameCmdline,NULL)
	ADD_HANDLE_ITEM(pidCmdline,	NULL)
};

int main(int argc, char *argv[]){
	int err=0;
	int i=0;
	log_init("l=11111");
	for(i=0;i<getCount(box_handle_array);i++){
		if(strcmp(get_last_name(argv[0]),box_handle_array[i].name)){
			continue;
		}
		if(box_handle_array[i].handle){
			err=box_handle_array[i].handle(argv);
			if(err<0){
				err("%s handle excute failure,err=%d",box_handle_array[i].name,err);
				return -1;
			}
		}
		// inf("%s handle excute done!",box_handle_array[i].name);
		return 0;
	}
}