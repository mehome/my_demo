#ifndef __COMMON_H__
#define __COMMON_H__

//#define TURN_ON_SLEEP_MODE


#define USE_MPDCLI

#define      USE_MPD_PLAYLIST 

#define  USE_AMR_8K_16BIT

//#define USE_PCM_8K_16BIT 1

//#define USE_PCM_16K_16BIT 1

//#define USE_AMR_16K_16BIT

#ifdef USE_PCM_8K_16BIT
#define ASR_PCM_8K_16BIT
#define CAPTURE_PCM_8K_16BIT

#endif
#ifdef USE_PCM_16K_16BIT
#define ASR_PCM_16K_16BIT
#define CAPTURE_PCM_16K_16BIT
#endif

#ifdef USE_AMR_8K_16BIT
#define USE_UPLOAD_AMR
#define ASR_AMR_8K_16BIT
//#define CAPTURE_PCM_8K_16BIT
#endif

#ifdef USE_AMR_16K_16BIT
#define USE_UPLOAD_AMR
#define ASR_AMR_16K_16BIT
//#define CAPTURE_PCM_16K_16BIT
#endif


#ifdef USE_PCM_8K_16BIT
#undef UPLOAD_SIZE
#define UPLOAD_SIZE 8 *4 *1024
#endif

#ifdef USE_PCM_16K_16BIT
#undef UPLOAD_SIZE
#define UPLOAD_SIZE (2*8 * 1024)
#endif


#ifdef USE_AMR_16K_16BIT
#undef UPLOAD_SIZE
#define UPLOAD_SIZE      640 * 2
#define CAPTURE_SIZE 1024 * 8 * 2

#endif


#ifdef USE_AMR_8K_16BIT
//#define UPLOAD_SIZE 1024 *4/4*2*2*2*2 
#undef UPLOAD_SIZE
#define UPLOAD_SIZE      640
#define CAPTURE_SIZE 1024 * 8

#endif

#ifdef ENABLE_OPUS
//#define UPLOAD_SIZE       640 *4
#define UPLOAD_SIZE       640 *15 
#define CAPTURE_SIZE 1024 * 8 * 4 
#endif


#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/null", 0, NULL); \
})

#define PTHREAD_SET_UNATTACH_ATTR(attr) \
		pthread_attr_init(&attr); \
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)


unsigned long get_file_size(const char *path);
long getCurrentTime()   ; 

#define TURING_PLAYLIST_JSON "/usr/script/playlists/turingPlayList.json"
#define TURING_PLAYLIST_M3U "/usr/script/playlists/turingPlayList.m3u"
#define CURRENT_PLAYLIST_JSON  "/usr/script/playlists/currentPlaylist.json"
#define CURRENT_PLAYLIST_M3U "/usr/script/playlists/currentPlaylist.m3u"
#define MC_CURRENT_PLAYLIST_M3U "/usr/script/playlists/mcCurrentPlaylist.m3u"

#define CURRENT_PLAYLIST "currentPlaylist"
#define MC_CURRENT_PLAYLIST "mcCurrentPlaylist"
#define TURING_PLAYLIST "turingPlayList"

extern unsigned long GetDeviceTotalSize(char *path); 
extern unsigned long GetDeviceFreeSize(char *path);

extern unsigned long getFileSize(const char *path);
extern int GetFileLength(char *file);

extern int GetMac(char *id);
extern int wav_play2(const char *file);

typedef int (*QuitCB)(void);

#endif

