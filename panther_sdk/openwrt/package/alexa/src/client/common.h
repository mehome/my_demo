#ifndef __COMMON_H__
#define __COMMON_H__

#define ALERT_EVENT_QUEUE
#define AUDIO_REQUEST_IDLE		0
#define AUDIO_REQUEST_START		1	// 开始请求
#define AUDIO_REQUEST_UPDATE	2	// 上传数据
#define AUDIO_REQUEST_REC		3	// 接收数据

#define AUDIO_STATUS_PLAY 		0
#define AUDIO_STATUS_STOP		1

#define MUTE_STATUS					1
#define UMUTE_STATUS				0
extern int GetPlayStatus();
extern void SetPlayStatus(int iStatus);
extern int OnWriteMsgToUartd(const char *pData);
extern char *GetSynchronizeStateEventJosnData(char *pBoundary);
extern char *GetSpeechStartedEventJosnData(char *pBoundary);
extern char *GetSpeechFinishedEventJosnData(char *pBoundary);
extern char *GetSpeechRecognizeEventjsonData(void);
extern char *GetPlaybackStartedEventJosnData(char *pBoundary);
extern char *GetPlaybackPausedEventJosnData(char *pBoundary);
extern char *GetPlaybackStopEventJosnData(char *pBoundary);
extern char *GetPlaybackNearlyFinishedEventJosnData(char *pBoundary);
extern char *GetPlaybackQueueClearEventJosnData(char *pBoundary);
#ifdef ALERT_EVENT_QUEUE
extern char *GetAlertEventJosnData(char *pBoundary,char *data);
#else
extern char *GetAlertEventJosnData(char *pBoundary, int iStatus);
#endif
//extern char *GetPlaybackFinishedEventJosnData(char *pBoundary);
extern char *GetPlaybackFinishedEventJosnData(char *pBoundary, char *pData);
extern char *GetPlaybackFailedEventJosnData(char *pBoundary);
extern char *GetVolumeChangedEventJosnData(char *pBoundary, int iStatus);
extern char *GetPlaybackControllerEventJosnData(char *pBoundary, int iStatus);
extern char *GetSettingJosnData(char *pBoundary);

extern void PrintSysTime(char *pSrc);
extern int myexec_cmd(const char *cmd);

extern char InitAmazonConnectStatus(void);
extern void SetAmazonConnectStatus(char byStatus);
extern char GetAmazonConnectStatus(void);
extern char GetAmazonPlayBookFlag(void);
extern void SetAmazonPlayBookFlag(char byStatus);
extern void SetAmazonLogInStatus(char byStatus);
extern char GetAmazonLogInStatus(void);
extern void SetAudioRequestStatus(char byStatus);
extern char GetAudioRequestStatus(void);

extern char GetTTSAudioStatus(void);
extern void SetTTSAudioStatus(char byStatus);
extern void SetSpeechStatus(char byStatus);
extern char InitAmazonConnectStatus(void);
extern void SetCallbackStatus(char byStatus);
extern char GetCallbackStatus(void);
extern char GetRecordFlag(void);
extern void SetRecordFlag(char byFlag);
extern char GetMuteFlag(void);
extern void SetMuteFlag(char byFlag);
extern char GetSendInterValFlag(void);
extern void SetSendInterValFlag(char byFlag);
extern void OnUpdateURL(char *pBaseURL);
extern void SetDownchannelStatus(char byStatus);
extern char GetDownchannelStatus(void);

extern char GetWiFiConnectStatus(void);

#endif /* __COMMON_H__ */

