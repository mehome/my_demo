#ifndef __COMMSAT_KEY_EMOTION_H__
#define __COMMSAT_KEY_EMOTION_H__

#include <json/json.h>

typedef enum {
	TURING_FUNC_CHAT = 20000,      //聊天
	TURING_SLEEP_CONTROL,          //休眠控制
	TURING_VOLUME_CONTROL,         //音量控制
	TURING_WEATHER_INQUIRY,		   //天气查询
	TURING_TIME_INQUIRY,		   //时间查询
	TURING_DATE_INQUIRY,		   //日期查询
	TURING_COUNT_CALCLATE,         //计算器
	TURING_MUSIC_PLAY,             //播音乐
	TURING_STORY_TELL,             //讲故事
	TURING_POEMS_RECITE,	       //背古诗
	TURING_POETRY_INTER,		   //对诗词
	TURING_ANIMAL_SOUND,		   //动物叫声
	TURING_ENCYCLOPEDIC_KNOWLEDGE, //百科知识
	TURING_PHONE_CALL,  		   //打电话
	TURING_SOUND_GUESS,            //猜叫声
	TURING_CHINESE_ENGLISH,		   //中英互译
	TURING_ROBOT_DANCE,			   //跳舞
	TURING_ACTYIVE_INTER = 29998,  //主动交互
	TURING_MAEKED_WORDS,		   //提示语
}TuringWifiFunc;


typedef enum     {
	EMOTION_ID_SURPRISE     	= 10100, //惊讶
	EMOTION_ID_EXCITING 		= 10200, //exciting 兴奋
	EMOTION_ID_HAPPINESS 		= 10300, //happiness 开心
	EMOTION_ID_SELFCONFIDENCE 	= 10400, //self-confidence 自信
	EMOTION_ID_RELAXATION 		= 10500, //relaxation 放松
	EMOTION_ID_FATIGUE 			= 10600, //fatigue 疲劳
	EMOTION_ID_CURIOSITY 		= 10800, //curiosity  好奇 
	EMOTION_ID_YEARNING 		= 10900, //yearning 渴望
	EMOTION_ID_EXPECTATION 		= 11000, //expectation  期待
	EMOTION_ID_VIGILANT 		= 11200, //vigilant 警惕
	EMOTION_ID_LETHARGY			= 20600, //Lethargy 昏睡
	EMOTION_ID_DISSATISFACTION 	= 20500, //Dissatisfaction 不满
	EMOTION_ID_HATE		 		= 20400, //hate 厌恶
	EMOTION_ID_UNHAPPY 			= 20300, //Unhappy 不悦
	EMOTION_ID_ANGER 			= 20200, //anger 愤怒
	EMOTION_ID_GETANGER 		= 20100, //GetAnger 生气
	EMOTION_ID_NERVOUS  		= 20800, //Nervous 紧张
	EMOTION_ID_FEAR		  		= 20900, //fear 恐惧
	EMOTION_ID_DEPRESSED  		= 21000, //depressed 沮丧
	EMOTION_ID_DISAPPOINTMENT  	= 21100, //Disappointment 失望
	EMOTION_ID_ANXIOUS  		= 21200, //anxious 焦虑
	EMOTION_ID_BORING  			= 21300, //Boring 无聊
	EMOTION_ID_DOUBT  			= 21500, //Doubt 疑惑
}EmotionId;


json_object * VolumeControlFunc(json_object *cmd);
json_object * MusicPlayFunc(json_object *cmd);
json_object * StoryTellFunc(json_object *cmd);
json_object * PoemsReciteFunc(json_object *cmd);
json_object * AnimalSoundFunc(json_object *cmd);
json_object * PhoneCallFunc(json_object *cmd);


#endif



