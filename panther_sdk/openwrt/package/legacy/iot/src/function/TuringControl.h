#ifndef __TURING_CONTROL_H__
#define __TURING_CONTROL_H__



#include "common.h"
#include "myutils/debug.h"

enum{
	POWER_STATE_ON = 0,
	POWER_STATE_STANDBY,    
	POWER_STATE_SUSPEND, 
	POWER_STATE_LOCKED, 
};


void SetStopTime();

void StartCaptureNewSession();
void StartHttpRequestNewSession();
void 	ClearStopTime();
void StartChatNewSession();

void StartNewSession();



void TuringSuspend();
void TuringResume();

void TuringPlayEvent();
void TuringStopEvent();
void TuringUpdateDeviceEvent();

void TuringChatPlay();
void TuringSpeechStart(int);
void TuringChatStart();



void SetAiMode(int mode);
int GetAiMode();
void SetIsChat(int mode);
int GetIsChat();



#define PLAY_DONE   	"PlayDone"


#define MPC_VOLUME_EVENT  "MpcVolumeEvent"
#define MPC_PLAY_EVENT    "MpcPlayEvent"
#define MPC_NEXT_EVENT 	  "MpcNextEvent"
#define MPC_PREV_EVENT 	  "MpcPrevEvent"
#define MPC_STOP_EVENT 	  "MpcStopEvent"
#define MPC_TOGGLE_EVENT    "MpcToggleEvent"

#define MPC_SONG_CHANGE 	  "MpcSongChange"

#define MPG_PLAY_EVENT 	  "MpgPlayEvent"
#define MPG_PREV_EVENT 	  "MpgPrevEvent"
#define MPG_NEXT_EVENT 	  "MpgNextEvent"


#define LOCK_EVENT 	  "LockEvent"
#define LOCKED_EVENT 	  "LockedEvent"


#define SHUT_DOWN_EVENT 	  "ShutDownEvent"
#define START_UP_EVENT 	  "StartUpEvent"


#define UPDATE_DEVICE_EVENT 	  "UpdateDevice"


#define SPEECH_START    		   "StartSpeech"
#define WAKEUP_TEST    		        "wakeup"

#define SPEECH_START_NOVAD    "StartSpeechNovad"
#define AI_START              "StartAi"
#define VAD_OFF    				   "VadOff"


#define SPEECH_END      "EndSpeech"

#define CHAT_START  		 "StartChat"
#define CHAT_PLAY 		     "PlayChat"

#define EN_TO_CN  		  	 "EnToCn"
#define CN_TO_EN  		     "CnToEn"
#define PM_STANDBY  		   "TuringStandby"
#define PM_SUSPEND 		      "TuringSuspend"
#define PM_RESUME 		       "TuringResume"

#define PM_FIVE  		     	 "TuringFive"
#define PM_TEN		     		 "TuringTen"


#define REPORT_EVENT 		 "ReportSongName"


#define COLLECT_SONG 		 "CollectSong"
#define SWITCH_COLUMN		 "SwitchColumn"


#define LOW_POWER_EVENT		 "LowPower"
#define NORMAL_POWER_EVENT		 "NormalPower"

#define ALERT_DONE 		     "AlertDone"

#define CAPTURE_END 		     "CaptureEnd"

#define CONFIG_CHANGE 		      "ConfigChange"
#define CONFIG_WRITE		      "ConfigWrite"

#define UPDATE_DEVICE_STATUS 	  "DeviceStatus"

#define CHARGE_EVENT 	  "ChargeEvent"

#define MPC_NEXT 		       "MpcNext"

#define ACTIVE_INRET 		       "ActiveInter"

#define PLAY_MUSIC		       "PlayMusic"

#define WAV_PLAY_START	       "WavPlayStart"
#define WAV_PLAY_END		       "WavPlayEnd"


void TuringMonitorControl();

#endif



