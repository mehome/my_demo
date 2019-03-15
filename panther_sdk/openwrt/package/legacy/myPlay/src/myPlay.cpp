#include "myPlay.h"
#include "SimpleAudioPlayerImpl.h"
#include "AudioPlayerInterface.h"


using namespace duerOSDcsApp::mediaPlayer; 
int myPlayProxyInit(SimpleAudioPlayerImpl *playerIstPtr){
	do{
		pause();
	}while(1);
	return 0;
}

int main(int argc, char *argv[]){

	myPlayLogIint("l=111111");
	traceSig(SIGSEGV);
	int ret=0;
	if(!argv[1] || !strcmp(argv[1],"--help")){
		myPlayRaw("             [file path] : play file!\n");
		myPlayRaw("--device default [file path] : user alsa device:All to play!\n");
		return 0;
	}
	ret=strcmp(argv[1],"--device");
	if(!ret){
		assert_arg(3,-1);
		std::string device=argv[2];
		SimpleAudioPlayerImpl playerIst(device);
		std::string path=argv[3];
		myPlayInf("%s palying %s ...",device.c_str(),path.c_str());
		playerIst.audioPlay(path.c_str(),RES_FORMAT::AUDIO_MP3, 0, 15000);
		ret=myPlayProxyInit(&playerIst);
		if(ret<0){
			myPlayInf("palying %s ...",path.c_str());
			return -1;
		}
		return 0;
	}


	assert_arg(1,-1);
	SimpleAudioPlayerImpl playerIst("common");
	std::string path=argv[1];
	myPlayInf("palying %s ...",path.c_str());
	playerIst.audioPlay(path.c_str(),RES_FORMAT::AUDIO_MP3, 0, 15000);
	ret=myPlayProxyInit(&playerIst);
	if(ret<0){
		myPlayErr("myPlayProxyInit failure!");
		return -1;
	}
	return 0;
}

