#include <libcchip/platform.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
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
