#ifndef _PWMCONFIG_H
#define _PWMCONFIG_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/un.h>
#include <pthread.h>
#include <sys/wait.h>
#include "../log/slog.h"

#define PERIOD	1000000		//period = 1000000000->1s T
#define RATE	0.003906				//0.003906
#define DUTYCYCLE	PERIOD*RATE	//duty_clyle
//#define PERIOD	1000000		//period
//#define DUTYCYCLE	3906	//duty_clyle

#define PWM0_R	0	
#define PWM1_G	1
#define PWM3_B	3



#define P_UNIX_DOMAIN "/tmp/UNIX.PWM"

typedef unsigned char  					uint8_t;
typedef unsigned short 					uint16_t;
typedef unsigned int  					uint32_t;

/*** constants ***/
#define SYSFS_PWM_DIR "/sys/class/pwm"
#define MAX_BUF 64

/*** PWM functions ***/
/* PWM export */
int pwm_export(unsigned int pwm);
/* PWM unexport */
int pwm_unexport(unsigned int pwm);
/* PWM configuration */
int pwm_config(unsigned int pwm, unsigned int period, unsigned int duty_cycle);
/* PWM enable */
int pwm_enable(unsigned int pwm);
/* PWM disable */
int pwm_disable(unsigned int pwm);
/*RGB Set color*/
void Rgb_Led_Setcolor(uint8_t r,uint8_t g,uint8_t b);
/*init pwm & rgb*/
void Rgb_All_Init(void);
/*off rgb*/
void Rgb_Led_Off(int PwmChannel);
/*off all tgb*/
void Rgb_Led_Off_All(void);


#endif
