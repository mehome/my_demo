#ifndef __CAPTURE_AUDIO_PTHREAD_H__
#define __CAPTURE_AUDIO_PTHREAD_H__

#include <stdio.h>
typedef struct _record
{
    int first_put;
    FILE *record_fp;
    int record_size;
}record_t;

extern void StartTlkPlay_led(void);
extern void StartPowerOn_led(void);
extern void StartTlkOn_led(void);

extern void CreateCapturePthread(void);
extern void DestoryCapturePthread(void);



extern unsigned int CaptureFifoSeek( unsigned char *buffer, unsigned int len);
extern unsigned int CaptureFifoLength();
extern unsigned int CaptureFifoPut(unsigned char *buffer, unsigned int len);
extern unsigned int CaptureFifoDelData(unsigned int len);

extern void CaptureFifoClear();
extern void CaptureFifoDeinit();
extern void CaptureFifoInit();

extern void StartCaptureAudio();
extern void SetCaptureWait(int wait);
extern void Start_record_data(void);
extern int get_record_status(void);

#endif /* __CAPTURE_AUDIO_PTHREAD_H__ */


