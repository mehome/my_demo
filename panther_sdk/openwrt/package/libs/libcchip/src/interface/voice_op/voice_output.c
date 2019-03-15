#include <stdio.h>
#include <alsa/asoundlib.h>
#include <function/alsa_extern/amixer_ctrl.h>

//3.2.11. 获取音响静音状态
int get_player_cmd_mute() {
	return cdb_get_int("$mute_status", 0);
}

//3.2.11. 静音设置
int set_player_cmd_mute(int mute)
{
	int ret = 0, vol = 0;
	playbackCtl para={
		"Media",
	};
	switch(mute)
	{
		case 0:
			cdb_set_int("$mute_status", 0);
			vol = cdb_get_int("$ra_vol", 0);
			para.volume = vol;
			ret = amixer_set_playback_volume(&para);
			if(ret < 0){
				err("amixer_set_playback_volume failure !");
				return -1;				
			}
			break;
		case 1:
			inf("vol: 0\n");
			cdb_set_int("$mute_status", 1);
			para.volume = 0;
			ret = amixer_set_playback_volume(&para);
			if(ret<0){
				err("amixer_set_playback_volume failure !");
				return -2;
			}
			break;
		default:
			err("mute flag error");
			break;
	}
	return 0;
}

//3.2.14. 设置均衡器模式
int set_equalizer_mode(int mode)
{	
	int ret = 0;
	switch(mode)
	{
		case 0:
			//0  关闭均衡
			break;
		case 1:
			//1  Classic 模式
			break;
		case 2:
			//2  Popular 模式
			break;
		case 3:
			//3  Jazzy 模式
			break;
		case 4:
			//4  Vocal 模式
			break;
		default:
			err("no such mode!");
			break;
	}
	return 0;
}

//3.2.15. 查询均衡器
int get_equalizer_mode(void)
{
	return 0;
}


//3.2.16. 声道设置
// flag 0：双声道 1:左声道， 2:右声道
int set_voice_channel(int flag)
{
	int ret = 0, vol = 0;
	playbackCtl channel={
		"Media",
	};

	channel.channel = flag;
	if(channel.channel == 0){
		vol = cdb_get_int("$ra_vol", 0);
		channel.volume = vol;
	}

	ret=amixer_set_playback_channel(&channel);
	if(ret<0){
		err("%s failure!",__func__);
		return -1;
	} else {
		cdb_set_int("$audio_channel", flag);
	}
	return 0;
}

// 获取声道设置
int get_voice_channel() {
	return cdb_get_int("$audio_channel", 0);
}


