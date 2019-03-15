#ifndef _MON_H
#define _MON_H

#define MAX(a,b) (((a)>(b))?(a):(b))

#define MONS_SOCK		"/var/mons.socket"
#define MONC_SOCK		"/var/monc.socket"

/*AIRPLAY Unix Socket*/
#define AIRS_SOCK		"/var/airs.socket"
#define AIRC_SOCK		"/var/airc.socket"

/*SPOTIFY Unix Socket*/
#define SPOTS_SOCK	"/var/spotify_s.socket"
#define SPOTC_SOCK	"/var/spotify_c.socket"
#define NOTIFY_SPOTIFY_STOP 1

#define MAX_BUF_LEN 64 

//#define MONITOR_INPUTDEV
#ifdef MONITOR_INPUTDEV
#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

// MADC mapping
#define FORWARD		BTN_1
#define BACKWARD	BTN_2 
#define TOGGLE		BTN_3
#define VOLUP		BTN_5
#define VOLDWN		BTN_6

// GPIO mapping
#define RB_RST		BTN_0
#define WPS			BTN_1 
#define SWITCH		BTN_2

#define BTN_RELEASE		0
#define BTN_PRESS		1
#define BTN_CONTINUE	2

#define RST_TO_DFT	5
#define OMNI_TRI	3

#endif

enum {
    VOLUME = 1,
    DACPCTRL,
    STATECTRL,
    AIRMUSIC,
};

enum {
    MONDLNA = 1,
    MONAIRPLAY,
	MONSPOTIFY,
	MONNONE,
};
enum {
	PLAY_STATE_STOP= 0,
	PLAY_STATE_PLAYING,
	PLAY_STATE_PAUSE,
	PLAY_STATE_UNKNOWN,
};
struct mon_param {
	unsigned char type;
    unsigned char resp;
	char *data;
} __attribute__ ((packed));

struct ctrl_param {
    unsigned char type;
    unsigned char resp;
    unsigned int len;
    char data[128];
} __attribute__ ((packed));

struct spotify_ipc_param {
    unsigned char senderType;
    unsigned char isNeedResp;
		unsigned int len;
    char data[128];
} __attribute__ ((packed));

unsigned int ctrl_play_dlna(unsigned int mode);
unsigned int ctrl_play_airplay(unsigned int mode);
unsigned int ctrl_play_spotify(unsigned int mode);
#ifdef __cplusplus  
extern "C" {  
#endif
unsigned int inform_ctrl_msg(char cmd, char *data);
#ifdef __cplusplus  
}  
#endif 
#endif // _MON_H
