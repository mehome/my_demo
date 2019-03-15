
#ifndef __MONATAGE_H__
#define __MONATAGE_H__



void gpio_test_xzx();
void* check_usb_tf();

void factory_mode();

int wifi_autoconfig();
int my_system(char *cmd);
int hangshu(char file[]);
int compare_FileAndUrl(char *filepatch,char *Url);
#define SSBUF_LEN_512    512 
#define SSBUF_LEN_256    256 
#define SSBUF_LEN_128    128 
#define SSBUF_LEN_64    64 
#define SSBUF_LEN_16    16

#endif








