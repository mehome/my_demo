#ifndef AMIXER_CTRL_H_
#define AMIXER_CTRL_H_ 1
#include <alsa/asoundlib.h>
#include <wdk/cdb.h>
#include "../uartd_fifo/uartd_fifo.h"
#include "../common/misc.h"
#define CCHIP_VOL_MIN 0
#define CCHIP_VOL_MAX 100
#define IS_OVER_VOL_RANGE(vol) ({\
	char ret=0;\
	if(vol<CCHIP_VOL_MIN || vol>CCHIP_VOL_MAX){\
		err("volume:%d is over range[%d,%d]",volume,CCHIP_VOL_MIN,CCHIP_VOL_MAX);\
		ret=1;\
	}\
	ret;\
})
#define CCHIP_DEFAULT_COMMON_VOL 50
#define CCHIP_DEFAULT_ALERTS_VOL 50
#define CCHIP_DEFAULT_TIPSOUND_VOL 50
// #define get_vol_set(vol) ((vol*255/100)%256)
// #define get_vol_set(vol) ((255-255*(vol-100)*(vol-100)/10000)%256)
typedef enum playerId_t{
	START_PLAYER_ID=-1,
	MPD,
	SPOTIFY,
	SHAIRPORT,
	END_PLAYER_ID
}playerId_t;

typedef struct playbackCtl{
	char *name;			//声卡名称	:All, Media, Wavplay
	unsigned index;		
	unsigned volume;	//音量: 0~100
	int channel;		//声道: 0:stero, 1:left, 2:right
}playbackCtl;


int amixer_set_playback_channel(playbackCtl *pCtl);
int amixer_set_playback_volume(playbackCtl *pCtl);
int set_system_volume(unsigned volume);
int get_mixer_volume(char *name);
int set_mixer_volume(char *name,int volume);
#endif
