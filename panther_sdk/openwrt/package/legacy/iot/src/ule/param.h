#ifndef __PARAM_H__
#define __PARAM_H__

typedef struct _RobotData {
	int code;
	char *buf;
}RobotData;

RobotData *newRobotData();
void freeRobotData(RobotData *data);
void setRobotDataCode(RobotData *data, int code);
void setRobotDataBuf(RobotData *data, char *buf);
int   getSign(char *question, char *file, char *body, int len) ;


#endif






