#ifndef __COMMSAT_H__
#define __COMMSAT_H__

#define WAKE_UP 					"WakeUp"			//
#define CAPTURE_START 				"CaptureStart"		//
#define CAPTURE_END					"CaptureEnd"		//
#define IDENTIFY_OK					"Identify_OK"		// 
#define IDENTIFY_ERR				"Identify_ERR"		//
#define VOICE_PLAY_END				"VoicePlaybackEnd" 	//
#define VOICE_INTERACTION			"VoiceInteraction"	//

#define SUSPEND						"Suspend"	//
#define RESUME						"Resume"	//
#define STANDY						"Standby"	//

#define KEY_WORD			"AXX+KEY+"	//
#define JSON_DATA			"AXX+JSN+"	//
#define EMOTION_INOF		"AXX+EMO+"	//

int OnWriteMsgToCommsat(char* cmd,char *data);
#endif 