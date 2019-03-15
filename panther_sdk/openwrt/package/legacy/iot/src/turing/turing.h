#ifndef __TURING_H__
#define __TURING_H__

//#ifdef ENABLE_COMMSAT
typedef enum {
	TURING_FUNC_CHAT = 20000,
	TURING_SLEEP_CONTROL,		  //休眠控制
	TURING_VOLUME_CONTROL,		  //音量控制
	TURING_WEATHER_INQUIRY,       //天气查询  
	TURING_TIME_INQUIRY,			 
	TURING_DATE_INQUIRY,		  //日期时间查询
	TURING_COUNT_CALCLATE,		  //计算器
	TURING_MUSIC_PLAY,            //播音乐
	TURING_STORY_TELL,			  //讲故事
	TURING_POEMS_RECITE,		  //讲故事
	TURING_POETRY_INTER,		  //古诗词
	TURING_ANIMAL_SOUND,          //动物叫声
	TURING_KNOWLEDGE,			  //百科知识
	TURING_PHONE_CALL,            //打电话
	TURING_SOUND_GUESS,			  //猜叫声
	TURING_CHINESE_ENGLISH,       //中英互译
	TURING_DANCE,		 		  //跳舞
	TURING_ENGLISH_DIALOGUES = 20018,     //英文对话
	TURING_MUSICAL_INSTRUMENTS,   //乐器声音
	TURING_NATURE_SOUND,          //大自然的声音	
	TURING_SCREEN_BRIGHTNESSS,    //屏幕亮度
	TURING_ELECTRICITY_QUERY,     //电量查询
	TURING_MOTION_CONTROL,        //运动控制
	TURING_TAKE_PHOTO,     		  //拍照
	TURING_ALARM_CLOCK,           //闹钟
	TURING_OPEN_APP,     		  //打开应用
	TURING_KNOWLEDGE_BASE, 		   //知识库
	TURING_ACTYIVE_INTER = 29998, //主动交互
	TURING_MAEKED_WORDS,		  //提示语
}TuringWifiFunc;

typedef enum {
	TURING_ERROR_IN_PROGRESS = 40000, //正在进行流式识别
	TURING_ERROR_VALUE_ERROR = 40001, //字段错误
	TURING_ERROR_ILLEAGAL_VALUE, //字段错误
	TURING_ERROR_VALUE_NULL_OR_MISSING, //字段为空或错误
	TURING_ERROR_ASR_FAILURE, //语音解析失败
	TURING_ERROR_TTS_FAILURE, //文本转语音失败
	TURING_ERROR_NLP_FAILURE, //语义解析失败
	TURING_ERROR_TOKEN_INVALID_VALUE,	//无效token
	TURING_ERROR_IS_EXPIRED, //过期
	TURING_ERROR_ACTIVE_INTERACTION_CORPUS_NOT_EXIST = 40010, //主动交互语料不存在
	TURING_ERROR_GREETING_CORPUS_NOT_EXIST, //打招呼语料不存在
	TURING_ERROR_REQUEST_IS_FORBIDDEN,	//拒绝请求
	TURING_ERROR_OUT_OF_DEVICE_COUNT_LIMIT,	//请求超出限制
	TURING_ERROR_UNKNOW_ERROR=49999,	//未知错误
}TuringErrorCode;


enum {
	HTTP_REQUEST_NONE= 0,
	HTTP_REQUEST_STARTED,
	HTTP_REQUEST_ONGING,
	HTTP_REQUEST_DONE,
	HTTP_REQUEST_CLOSE,
	HTTP_REQUEST_CANCLE,
};


extern void ClearHttpRequestCurlHandle();

extern void NewHttpRequest();
extern void SetHttpRequestWait(int wait);
extern int IsHttpRequestDone();
extern int IsHttpRequestFinshed();
extern void SetHttpRequestState(int state);
extern int IsHttpRequestCancled();
static int send_data_to_server_by_socket(int index, char *pData, int iLength);


enum {
	HTTP_RECV_NONE= 0,
	HTTP_RECV_STARTED,
	HTTP_RECV_CANCLE,
};

void SetHttpRecvState(int state);
void SetHttpRecvCancel();
int IsHttpRecvCancled();
int IsHttpRecvFinshed();





#endif


