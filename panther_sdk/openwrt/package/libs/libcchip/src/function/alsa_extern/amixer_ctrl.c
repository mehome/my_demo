#include "amixer_ctrl.h"

int amixer_set_playback_volume(playbackCtl *pCtl)
{	
	int err=0;
	snd_mixer_t *mixer_handle=NULL;
	int ret=snd_mixer_open(&mixer_handle, 0);
	if(ret){
		err("snd_mixer_open failure,ret=%d",ret);
		err=-1;	goto exit;
	}
	ret=snd_mixer_attach(mixer_handle,"default");
	if(ret){
		err("snd_mixer_attach failure,ret=%d",ret);
		err=-2;	goto exit;
	}
	ret=snd_mixer_selem_register(mixer_handle,NULL,NULL);
	if(ret){
		err("snd_mixer_selem_register failure,ret=%d",ret);
		err=-3;	goto exit;
	}
	ret=snd_mixer_load(mixer_handle);
	if(ret){
		err("snd_mixer_load failure,ret=%d",ret);
		err=-4;	goto exit;
	}		
	snd_mixer_selem_id_t *selem_id=NULL;
	snd_mixer_selem_id_alloca(&selem_id);
	snd_mixer_selem_id_set_index(selem_id,pCtl->index);
	snd_mixer_selem_id_set_name(selem_id,pCtl->name);
	snd_mixer_elem_t *volume_elem= snd_mixer_find_selem(mixer_handle,selem_id);
	if(NULL==volume_elem) {
		err("snd_mixer_find_selem failure!");
		err=-5;
		goto exit;
	}
	snd_mixer_selem_set_playback_volume_range(volume_elem,CCHIP_VOL_MIN,CCHIP_VOL_MAX);	
	switch(pCtl->channel){
		case 0:
			ret=snd_mixer_selem_set_playback_volume_all(volume_elem,pCtl->volume);
			break;
		case 1:
			ret=snd_mixer_selem_set_playback_volume(volume_elem,SND_MIXER_SCHN_FRONT_LEFT, pCtl->volume); 
			break;
		case 2:
			ret=snd_mixer_selem_set_playback_volume(volume_elem,SND_MIXER_SCHN_FRONT_RIGHT, pCtl->volume); 
			break;
		default:
			err("channel %d is not exist", pCtl->channel);
			break;			
	}
	if(ret) err=-6;
exit:
	snd_mixer_close(mixer_handle);
	return err;
}
int amixer_set_playback_channel(playbackCtl *pCtl)
{	
	int err=0;
	snd_mixer_t *mixer_handle=NULL;
	int ret=snd_mixer_open(&mixer_handle, 0);
	if(ret){
		err("snd_mixer_open failure,ret=%d",ret);
		err=-1;	goto exit;
	}
	ret=snd_mixer_attach(mixer_handle,"default");
	if(ret){
		err("snd_mixer_attach failure,ret=%d",ret);
		err=-2;	goto exit;
	}
	ret=snd_mixer_selem_register(mixer_handle,NULL,NULL);
	if(ret){
		err("snd_mixer_selem_register failure,ret=%d",ret);
		err=-3;	goto exit;
	}
	ret=snd_mixer_load(mixer_handle);
	if(ret){
		err("snd_mixer_load failure,ret=%d",ret);
		err=-4;	goto exit;
	}		
	snd_mixer_selem_id_t *selem_id=NULL;
	snd_mixer_selem_id_alloca(&selem_id);
	snd_mixer_selem_id_set_index(selem_id,pCtl->index);
	snd_mixer_selem_id_set_name(selem_id,pCtl->name);
	snd_mixer_elem_t *volume_elem= snd_mixer_find_selem(mixer_handle,selem_id);
	if(NULL==volume_elem) {
		err("snd_mixer_find_selem failure!");
		err=-5;
		goto exit;
	}
	snd_mixer_selem_set_playback_volume_range(volume_elem,CCHIP_VOL_MIN,CCHIP_VOL_MAX);
	switch(pCtl->channel){
		case 0:
			ret=snd_mixer_selem_set_playback_volume_all(volume_elem,pCtl->volume);
			break;
		case 1:
			ret=snd_mixer_selem_set_playback_volume(volume_elem,SND_MIXER_SCHN_FRONT_RIGHT, 0); 
			break;
		case 2:
			ret=snd_mixer_selem_set_playback_volume(volume_elem,SND_MIXER_SCHN_FRONT_LEFT, 0); 
			break;
		default: 
			err("no such channel\n");
			break;
	}
	if(ret) err=-6;
exit:
	snd_mixer_close(mixer_handle);
	return err;
}

inline int set_mixer_volume(char *name,int volume){
	playbackCtl para={0};
	para.name = name;
	para.volume = volume;
	if(IS_OVER_VOL_RANGE(volume)){
		return -1;
	}
	int ret=amixer_set_playback_volume(&para);
	if(ret<0){
		err("amixer_set_playback_volume failure!");
		return -1;
	}
	return 0;
}
inline int get_mixer_volume(char *name){
	int err=0;
	snd_mixer_t *mixer_handle=NULL;
	int ret=snd_mixer_open(&mixer_handle, 0);
	if(ret){
		err("snd_mixer_open failure,ret=%d",ret);
		err=-1;	goto exit;
	}
	ret=snd_mixer_attach(mixer_handle,"default");
	if(ret){
		err("snd_mixer_attach failure,ret=%d",ret);
		err=-2;	goto exit;
	}
	ret=snd_mixer_selem_register(mixer_handle,NULL,NULL);
	if(ret){
		err("snd_mixer_selem_register failure,ret=%d",ret);
		err=-3;	goto exit;
	}
	ret=snd_mixer_load(mixer_handle);
	if(ret){
		err("snd_mixer_load failure,ret=%d",ret);
		err=-4;	goto exit;
	}		
	snd_mixer_selem_id_t *selem_id=NULL;
	snd_mixer_selem_id_alloca(&selem_id);
	snd_mixer_selem_id_set_index(selem_id,0);
	snd_mixer_selem_id_set_name(selem_id,name);
	snd_mixer_elem_t *volume_elem= snd_mixer_find_selem(mixer_handle,selem_id);
	if(NULL==volume_elem) {
		err("snd_mixer_find_selem failure!");
		err=-5;
		goto exit;
	}
	long int alsa_right=CCHIP_VOL_MIN;
	long int alsa_left=CCHIP_VOL_MIN;
	snd_mixer_selem_set_playback_volume_range(volume_elem,CCHIP_VOL_MIN,CCHIP_VOL_MAX);
	snd_mixer_selem_get_playback_volume(volume_elem, SND_MIXER_SCHN_FRONT_LEFT,  &alsa_left);
	snd_mixer_selem_get_playback_volume(volume_elem, SND_MIXER_SCHN_FRONT_RIGHT, &alsa_right);
	// inf("alsa_left=%d",alsa_left);
	// inf("alsa_right=%d",alsa_right);
	// inf("all=%d",(alsa_left + alsa_right) >> 1);
exit:
	snd_mixer_close(mixer_handle);
	if(err<0)return err;
	return (alsa_left + alsa_right) >> 1;
}
int set_system_volume(unsigned volume)
{
	int ret=set_mixer_volume("COMMON",volume);
	if(ret<0){
		return -1;
	}
	cdb_set_int("$ra_vol", volume);	
	ret=uartd_fifo_write_fmt("setvol%03d",(int)volume);
	if(ret<0){
		err("uartd_fifo_write_fmt failure!")
		return -3;
	}
	return 0;
}
