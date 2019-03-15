#include "pwmconfig.h"

/* PWM export */
int pwm_export(unsigned int pwm)
{
	int fd;
	char pwmchan[1] = "";
	fd = open(SYSFS_PWM_DIR "/pwmchip0/export", O_WRONLY);
	if (fd < 0) {
		err ("\nFailed export PWM\n");
		return -1;
	}else{
		inf ("export %d success\n",pwm);

	}

	sprintf(pwmchan,"%d",pwm);
	
	write(fd, pwmchan, sizeof(pwmchan));
	close(fd);
	return 0;

}

/* PWM unexport */
int pwm_unexport(unsigned int pwm)
{
	int fd;
	char pwmchan[1] = "";
	int ret;

	fd = open(SYSFS_PWM_DIR "/pwmchip0/unexport", O_WRONLY);
	if (fd < 0) {
		err ("\nFailed unexport PWM\n");
		return -1;
	}else{
		
	}
	
	ret = write(fd, pwmchan, sizeof(pwmchan));
	if (ret < 0){
		err ("write pwm%d faile\n",pwm);

	}else{
		inf ("write pwm%d success\n",pwm);
	}		
	close(fd);
	return 0;
}


/* PWM configuration */
int fla = 1;
int pwm_config_par(unsigned int pwm, unsigned int period, unsigned int duty_cycle,int breathe)
{
	int fd,len_period,len_duty_cycle;
	char period_size[MAX_BUF] = {0};
	char duty_cycle_size[MAX_BUF] = {0};
	char openchan[128] = {0};
	int ret;
	
	len_period = snprintf(period_size, sizeof(period_size), "%d", period);
	len_duty_cycle = snprintf(duty_cycle_size, sizeof(duty_cycle_size), "%d", duty_cycle);

	/* set pwm period */
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/period",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM%d period\n",pwm);
		return -1;
	}else{
		inf ("\nSuccess open pwm%d period\n",pwm);
	}
	
	ret = write(fd, period_size, len_period);
	if (ret < 0){
		err ("write period pwm%d faile\n",pwm);

	}else{
		inf ("write period pwm%d success\n",pwm);
	}	
	/* set pwm duty cycle */
	memset(openchan,0,sizeof(openchan));
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/duty_cycle",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM duty cycle\n");
		return -1;
	}else{
		inf ("\nSuccess open pwm%d duty \n",pwm);
	}

	if (breathe){
	
		int breathe_duty_cycle = 0;
		while(breathe){
			len_duty_cycle = snprintf(duty_cycle_size, sizeof(duty_cycle_size), "%d", breathe_duty_cycle);
			printf ("1duty_cycle = %d\n",breathe_duty_cycle);
			ret = write(fd, duty_cycle_size, len_duty_cycle);
			if (ret < 0){
				err ("write duty_cycle %d faile\n",pwm);

			}else{
				inf ("write duty_cycle %d success\n",pwm);
			}
			if (breathe_duty_cycle < period && fla == 1){
				breathe_duty_cycle = breathe_duty_cycle + 4000*1;

			}else {
				fla = 0;
			}
			
			if (breathe_duty_cycle >= 4000 && fla == 0){
				breathe_duty_cycle = breathe_duty_cycle - 4000*1;
				if (breathe_duty_cycle < 0){
					fla = 1;
				}
			//	len_duty_cycle = snprintf(duty_cycle_size, sizeof(duty_cycle_size), "%d", breathe_duty_cycle);
			//	printf ("2duty_cycle = %d\n",breathe_duty_cycle);
			}else{
				fla = 1;
			}
			usleep(20*1000);
		}
	}else{
			ret = write(fd, duty_cycle_size, len_duty_cycle);
			if (ret < 0){
				err ("write duty_cycle %d faile\n",pwm);
			
			}else{
				inf ("write duty_cycle %d success\n",pwm);
			}			
	}
	close(fd);
	return 0;

}


/* PWM configuration */
int PWM_Period_config(unsigned int pwm, unsigned int period)
{
	int fd,len_period;
	char period_size[MAX_BUF] = {0};
	char duty_cycle_size[MAX_BUF] = {0};
	char openchan[128] = {0};
	int ret;
	
	len_period = snprintf(period_size, sizeof(period_size), "%d", period);


	/* set pwm period */
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/period",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM%d period\n",pwm);
		return -1;
	}else{
		inf ("\nSuccess open pwm%d period\n",pwm);
	}
	
	ret = write(fd, period_size, len_period);
	if (ret < 0){
		err ("write period pwm%d faile\n",pwm);

	}else{
		inf ("write period pwm%d success\n",pwm);
	}			
	close(fd);
	return 0;

}


/* PWM duty_cycle */
int PWM_Duty_Cycle_config(unsigned int pwm, unsigned int duty_cycle)
{
	int fd,len_duty_cycle;
	char duty_cycle_size[MAX_BUF] = {0};
	char openchan[128] = {0};
	int ret;
	

	len_duty_cycle = snprintf(duty_cycle_size, sizeof(duty_cycle_size), "%d", duty_cycle);
	
	/* set pwm duty cycle */
	memset(openchan,0,sizeof(openchan));
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/duty_cycle",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM duty cycle\n");
		return -1;
	}else{
		inf ("\nSuccess open pwm%d duty \n",pwm);
	}
	
	ret = write(fd, duty_cycle_size, len_duty_cycle);
	if (ret < 0){
		err ("write duty_cycle %d faile\n",pwm);

	}else{
		inf ("write duty_cycle %d success\n",pwm);
	}		
	close(fd);
	return 0;

}


/* PWM configuration */
int pwm_config(unsigned int pwm, unsigned int period, unsigned int duty_cycle)
{
	int fd,len_period,len_duty_cycle;
	char period_size[MAX_BUF] = {0};
	char duty_cycle_size[MAX_BUF] = {0};
	char openchan[128] = {0};
	int ret;
	
	len_period = snprintf(period_size, sizeof(period_size), "%d", period);
	len_duty_cycle = snprintf(duty_cycle_size, sizeof(duty_cycle_size), "%d", duty_cycle);

	/* set pwm period */
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/period",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM%d period\n",pwm);
		return -1;
	}else{
		inf ("\nSuccess open pwm%d period\n",pwm);
	}
	
	ret = write(fd, period_size, len_period);
	if (ret < 0){
		err ("write period pwm%d faile\n",pwm);

	}else{
		inf ("write period pwm%d success\n",pwm);
	}	
	/* set pwm duty cycle */
	memset(openchan,0,sizeof(openchan));
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/duty_cycle",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed set PWM duty cycle\n");
		return -1;
	}else{
		inf ("\nSuccess open pwm%d duty \n",pwm);
	}
	
	ret = write(fd, duty_cycle_size, len_duty_cycle);
	if (ret < 0){
		err ("write duty_cycle %d faile\n",pwm);

	}else{
		inf ("write duty_cycle %d success\n",pwm);
	}		
	close(fd);
	return 0;

}

/* PWM enable */
int pwm_enable(unsigned int pwm)
{
	int fd;
	char openchan[128] = {0};
	char enable_pwm[2] = {0};

	
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/enable",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed enable PWM%d \n",pwm);
		return -1;
	}else{
		inf ("\nSuccess enable PWM%d \n",pwm);	
	}

	sprintf(enable_pwm,"%s","1");
	int ret = write(fd, enable_pwm, sizeof(enable_pwm));
	if (ret < 0){
		err ("write enable %d faile\n",pwm);

	}else{
		inf ("write enable %d success\n",pwm);
	}
	close(fd);
	return 0;


}

/* PWM disable */
int pwm_disable(unsigned int pwm)
{
	int fd;
	char openchan[128] = {0};
	char disable_pwm[2] = {0};
	int ret;
	
	sprintf(openchan,SYSFS_PWM_DIR"/pwmchip0/pwm%d/enable",pwm);
	fd = open(openchan, O_WRONLY);
	if (fd < 0) {
		err ("\nFailed open PWM%d\n",pwm);
		return -1;
	}else{
		inf ("\nSuccess open PWM%d\n",pwm);
	}

	sprintf(disable_pwm,"%s","0");
	ret = write(fd, disable_pwm, sizeof(disable_pwm));
		if (ret < 0){
		err ("write disable %d faile\n",pwm);

	}else{
		inf ("write disable %d success\n",pwm);
	}
	close(fd);
	return 0;

}

void Rgb_Led_Off(int PwmChannel)
{
    if(pwm_disable(PwmChannel) < 0){
    	err("PWM disable error!\n");
    	return -1;
    }
}

void Rgb_Led_Off_All(void)
{
	int i = 0;
	for (i = 0;i < 4; i++){
		Rgb_Led_Off(i);
	}
}


void Rgb_Led_Setcolor(uint8_t r,uint8_t g,uint8_t b)
{
	int R,G,B;
	uint16_t t_r,t_g,t_b;
	int base_num = 3906;

	if (r > 0xff || g > 0xff || b > 0xff){
		err("Set color err");
	}
	t_r = r + 1;
	t_g = g + 1;	
	t_b = b + 1;

	R = t_r * DUTYCYCLE;
	G = t_g * DUTYCYCLE;
	B = t_b * DUTYCYCLE;

	PWM_Duty_Cycle_config(PWM0_R,R);
	PWM_Duty_Cycle_config(PWM1_G,G);
	PWM_Duty_Cycle_config(PWM3_B,B);

}


void Rgb_Set_Singer_Color(char *color)
{
	int c;
	if (!strcmp(color,"RED")){
		c = 1;		
	}else if (!strcmp(color,"ORA")){
		c = 2;	
	}else if (!strcmp(color,"YEL")){
		c = 3;	
	}else if (!strcmp(color,"GRE")){
		c = 4;	
	}else if (!strcmp(color,"BLU")){
		c = 5;	
	}else if (!strcmp(color,"CYA")){
		c = 6;
	}else if (!strcmp(color,"PUR")){
		c = 7;

	}	

	switch(c){
		case 'RED':
			break;
		case 'ORA':
			break;
		case 'YEL':
			break;
		case 'GRE':
			break;
		case 'BLU':
			break;
		case 'CYA':
			break;
		case 'PUR':
			break;
		defalut:
			break;
																
	}

}

void Rgb_All_Init(void)
{
	int MyPeriod = 1000000; //100000000 period 设置为1s//10000000-> 10ms
	float rate = 0.003912;
    int MyDuty = PERIOD * rate;
	int PwmChannel = 0;
	for (PwmChannel = 0;PwmChannel  < 4;PwmChannel++){
		if(pwm_export(PwmChannel) < 0){
			err("PWM export error!\n");
			return -1;
		}
		if(pwm_config(PwmChannel,PERIOD,DUTYCYCLE) < 0){
			err("PWM config error!\n");
			return -1;
		}
		/* enable corresponding PWM Channel */
		if(pwm_enable(PwmChannel) < 0){
			err("PWM enable error!\n");
			return -1;
		}		
	}

//	printf("PWM successfully enabled with period - %dms, duty cycle - %2.1f%%\n", MyPeriod/1000000, rate*100);
	
}



