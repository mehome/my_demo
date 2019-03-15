#ifndef __PLAYLIST_INFO_JSON_H__
#define __PLATLIST_INFO_JSON_H__
#define WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE (~0)

//#define __DEBUGPRINTF__

#ifdef  __DEBUGPRINTF__
#define PRINTF printf
#else
#define PRINTF //
#endif




typedef struct _WARTI_Music
{
	unsigned char *pArtist;
	unsigned char *pTitle;
	unsigned char *pAlbum;
	unsigned char *pContentURL;
	unsigned char *pCoverURL;
	unsigned char *pToken;
	
}WARTI_Music,*WARTI_pMusic;//�����б�Ԫ�صľ�����Ϣ

typedef struct _WARTI_MusicList
{
	char *pRet;				//�ɹ���־
	int Num;	
	int Index;			//��������
	WARTI_pMusic *pMusicList;
}WARTI_MusicList,*WARTI_pMusicList;//�����б�Ľṹ��

typedef struct _WARTI_MusicListInfo
{
	int KeyNum;			//��ݼ�����
	int Num;			//��������
	WARTI_pMusic *pMusicList;
}WARTI_MusicListInfo,*WARTI_pMusicListInfo;	

struct json_object* WIFIAudio_RealTimeInterface_pMusicTopJsonObject(WARTI_pMusic p);
struct json_object* WIFIAudio_RealTimeInterface_pMusicListInfoTopJsonObject(WARTI_pMusicList inlp,WARTI_pMusicList outlp);
int WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(WARTI_pMusicList *ppmusiclist);
int delete_one_music_to_playlistinfo(char *infilepatch,char *outfilepatch,int num);
struct json_object* delete_one_music_content_to_json_array(WARTI_pMusicList p,int num);
struct json_object* Insert_Songs_pMusicListInfoTopJsonObject(WARTI_pMusicList inlp,WARTI_pMusicList outlp);
int json_freeInformation(WARTI_pMusicList jsoninformation);
#endif