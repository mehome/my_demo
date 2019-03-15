#ifndef __HUABEI_H__
#define __HUABEI_H__



#define HUABEI_UNBIND					"HuabeiUnbind"
#define HUABEI_STATUS_REPORT	  		"HuabeiStatusReport"
#define HUABEI_TEST				    	"HuabeiTest"
#define HUABEI_SELECT_SUBJECT			"HuabeiSelectSubject"
#define HUABEI_LOCAL					"HuabeiLocal"
#define HUABEI_DATA_RE0					"HuabeiDataRes0"
#define HUABEI_DATA_RE1					"HuabeiDataRes1"
#define HUABEI_DATA_RE2					"HuabeiDataRes2"

#define HUABEI_NEXT						"HuabeiNext"
#define HUABEI_PREV						"HuabeiPrev"
#define HUABEI_CONFIRM					"HuabeiConfirm"
#define HUABEI_OK						"HuabeiOk"
#define HUABEI_PLAY						"HuabeiPlay"



#define CAPTURE_END 		     	    "CaptureEnd"
#define SPEECH_START    				"StartSpeech"
#define CHAT_START  					"StartChat"
#define CHAT_PLAY 						"PlayChat"

enum{
	HUABEI_PLAY_PREV = 0,
	HUABEI_PLAY_NEXT,
};


enum{
	HUABEI_MESSAGE_SEND_INTERCOM = 0,
	HUABEI_MESSAGE_SEND_UNBIND,
	HUABEI_MESSAGE_SEND_SELECT_SUBJECT,
};
enum{
	HUABEI_FILE_UPLOAD_SPEECH_RECOGNITION = 1, 
	HUABEI_FILE_UPLOAD_WECHAT_INTERCOM, 
};
//

enum{
	HUABEI_STATUS_REPORT_DEVICE_COMPLETION     = 0x7F, 
	HUABEI_STATUS_REPORT_DEVICE_IDLE	       = 0x80,
	HUABEI_STATUS_REPORT_DEVICE_PAUSE	       = 0x81,
	HUABEI_STATUS_REPORT_DEVICE_RECV_OK	       = 0x82,	
	HUABEI_STATUS_REPORT_DEVICE_LOW_BATTERY	   = 0x83,
	HUABEI_STATUS_REPORT_DEVICE_CHAREGING	   = 0x84,
	HUABEI_STATUS_REPORT_DEVICE_CHARGE_FULL	   = 0x85,
	HUABEI_STATUS_REPORT_DEVICE_SHUT_DOWN	   = 0x85,
};




#endif

