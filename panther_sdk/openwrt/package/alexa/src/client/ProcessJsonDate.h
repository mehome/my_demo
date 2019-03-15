#ifndef __PROCESS_JSONDATE_H__
#define __PROCESS_JSONDATE_H__

// Content-Type
#define CONTENT_TYPE_JSON			0
#define CONTENT_TYOE_OCTET_STREAM	1

// Event Status
#define STATUS_UUID					0
#define STATUS_CONTENT_TYPE			1
#define STATUS_JSON					2
#define STATUS_OCTET_STREAM			3

// audio player
#define AMAZON_AUDIOPLAYER                   "AudioPlayer"
#define AUDIOPLAYER_PLAY                     "Play"
#define AUDIOPLAYER_STOP                     "Stop"
#define AUDIOPLAYRT_CLEARQUEUE               "ClearQueue"

// speechSynthesizer
#define AMAZON_SPEECHSYNTHESIZER            "SpeechSynthesizer"
#define SPEECHSYNTHESIZER_SPEAK             "Speak"

// Speaker
#define AMAZON_SPEAKER                       "Speaker"
#define SPEAKER_SETVOLUME                    "SetVolume"
#define SPEAKER_ADJUSTVOLUME                "AdjustVolume"
#define SPEAKER_SETMUTE                      "SetMute"

// alerts
#define AMAZON_ALERTS                        "Alerts"
#define ALERTS_SETALERT                      "SetAlert"
#define ALERTS_DELETEALERT					"DeleteAlert"

// system
#define AMAZON_SYSTEM                        "System"
#define SYSTEM_RESETUSERINACTIVITY         "ResetUserInactivity"
#define SETENDPOINT							"SetEndpoint"

// SpeechRecognizer
#define AMAZON_SPEECHRECOGNIZER            "SpeechRecognizer"
#define SPEECHRECOGNIZER_EXPECTSPEECH     "ExpectSpeech"
#define STOP_CAPTURE                        "StopCapture"

// Notifications Interface
#define AMAZON_NOTIFICATIONS			 "Notifications"
#define SETINDICATOR					 "SetIndicator"
#define CLEARINDICATOR					 "ClearIndicator"

#define REPLACE_ALL  		"REPLACE_ALL"
#define ENQUEUE	 			"ENQUEUE"
#define REPLACE_ENQUEUED  	"REPLACE_ENQUEUED"

#define TYPE_REPLACE_ALL		0 	
#define TYPE_ENQUEUE	 		1	
#define TYPE_REPLACE_ENQUEUED	2

#define AMAZON_BOOK_LIST_MAXNUMBER 30
#define AMAZON_BOOK_LIST_MINNUMBER 5
#define AMAZON_BOOK_FUNCTION 0

extern int ProcessAmazonDirective(char *pData, int iSize);

#endif



