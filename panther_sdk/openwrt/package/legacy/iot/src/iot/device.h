#ifndef __DEVICE_H__
#define __DEVICE_H__

int GetDeviceID(char *);
unsigned char * GetUserID(char *apiKey, char *aesKey);
int GetDeviceSTotal(void);
int GetDeviceSFree(void);
int GetDeviceBattery(void);
int GetDeviceVolume(void);



#endif