#include "cpp_box.h"
#include "SimpleAudioPlayerImpl.h"
#include "AudioPlayerInterface.h"
int pipeTtt_handle(char *argv[]);

int wakeup_handle(char *argv[]);

int audioRec_handle(char *argv[]);

int tcp_server_handle(char *argv[]);

int myPlay_handle(char *argv[]){	
	using namespace duerOSDcsApp::mediaPlayer; 
	traceSig(SIGSEGV);
	SimpleAudioPlayerImpl playerIst("default");
	assert_arg(1,-1);
	std::string path=argv[1];
	inf("palying %s ...",path.c_str());
	playerIst.audioPlay(path.c_str(),RES_FORMAT::AUDIO_MP3, 0, 15000);	
	do{
		pause();
	}while(1);	
	return 0;
}

static handle_item_t box_handle_array[]={
	ADD_HANDLE_ITEM(pipeTtt,NULL)
	ADD_HANDLE_ITEM(tcp_server,	NULL)
	ADD_HANDLE_ITEM(myPlay,	NULL)
	ADD_HANDLE_ITEM(audioRec,NULL)
	ADD_HANDLE_ITEM(wakeup,NULL)
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
		return 0;
	}
}