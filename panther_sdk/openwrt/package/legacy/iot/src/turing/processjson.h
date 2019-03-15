#ifndef __PROCESS_JSON_H__
#define __PROCESS_JSON_H__

#include "common.h"

#ifdef TURN_ON_SLEEP_MODE

enum {
	INTER_STATE_NONE= 0,
	INTER_STATE_ERROR,
	INTER_STATE_SUCCESS,
	INTER_STATE_CANCLE,
	INTER_STATE_DONE,
};

void SetErrorCount(int count);
void IncreaseErrorCount();
int ShouldEnterSleepMode();
int IsInterSuccess();
void SetInterState(int state);
int IsInterFinshed();
#endif

extern void AddErrorCount(void);
extern void seterrorcount(int count);
extern int GetErrorCount(void);
extern void EnterInteractionMode(void);
extern void EndInteractionMode(void);
extern void InterruptInteractionMode(void);

extern void set_interact_status(int flag);
extern int  get_interact_status(void);
extern void text_2_sound(char *text);
extern int continue_to_play_tf();
void mpg123_player(char *url);  //add by frog 2018-07-09 18:15:03
extern char *CreateRequestJson(char *key, char *userid, char *token, int iLength, int index, int realTime, int tone, int type);
#endif

