
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include "alink_export.h"
#include "json_parser.h"
#include "montage_api.h"
#include <openssl/aes.h>


#if 1
//#define RAW_DATA_DEVICE

#define Method_PostData    "postDeviceData"
#define Method_PostRawData "postDeviceRawData"
#define Method_GetAlinkTime "getAlinkTime"

#define PostDataFormat      "{\"ErrorCode\":{\"value\":\"%d\"},\"Hue\":{\"value\":\"%d\"},\"Luminance\":{\"value\":\"%d\"},\"Switch\":{\"value\":\"%d\"},\"WorkMode\":{\"value\":\"%d\"}}"
#define post_data_buffer_size    (512)

network_status g_mtg_network_status = 0;
static int need_report = 1; /* force to report when device login */

#ifndef RAW_DATA_DEVICE
	static char post_data_buffer[post_data_buffer_size];
#else
	static char raw_data_buffer[post_data_buffer_size];

	/* rawdata byte order
	 *
	 * rawdata[0] = 0xaa;
	 * rawdata[1] = 0x07;
	 * rawdata[2] = device.power;
	 * rawdata[3] = device.work_mode;
	 * rawdata[4] = device.temp_value;
	 * rawdata[5] = device.light_value;
	 * rawdata[6] = device.time_delay;
	 * rawdata[7] = 0x55;
	 */
	#define RAW_DATA_SIZE           (8)
	uint8_t device_state_raw_data[RAW_DATA_SIZE] = \
				{0xaa, 0x07, 1, 8, 10, 50, 10, 0x55};
#endif

enum {
    ATTR_ERRORCODE_INDEX,
    ATTR_HUE_INDEX,
    ATTR_LUMINANCE_INDEX,
    ATTR_SWITCH_INDEX,
    ATTR_WORKMODE_INDEX,
    ATTR_MAX_NUMS
};
#define DEVICE_ATTRS_NUM   (ATTR_MAX_NUMS)

int device_state[] = {0, 10, 50, 1, 2};/* default value */
char *device_attr[] = {
    "ErrorCode",
    "Hue",
    "Luminance",
    "Switch",
    "WorkMode",
    NULL
};

int play_notice_voice(network_status status)
{
	return 0;
}

void main_loop(void)
{
    uint32_t time_start, time_end;
    struct timeval tv = { 0 };
	static uint32_t work_time = 1; //default work time 1s

    gettimeofday(&tv, NULL);
    time_start = tv.tv_sec;
    int ret;
    do {
        if (need_report) {
		#ifdef RAW_DATA_DEVICE
            /*
             * Note: post data to cloud,
             * use sample alink_post_raw_data()
             * or alink_post()
             */
            /* sample for raw data device */
            alink_post_raw_data(device_state_raw_data, RAW_DATA_SIZE);

		#else
            /* sample for json data device */
            snprintf(post_data_buffer, post_data_buffer_size, PostDataFormat,
                    device_state[ATTR_ERRORCODE_INDEX],
                    device_state[ATTR_HUE_INDEX],
                    device_state[ATTR_LUMINANCE_INDEX],
                    device_state[ATTR_SWITCH_INDEX],
                    device_state[ATTR_WORKMODE_INDEX]);
	
            ret = alink_report(Method_PostData, post_data_buffer);
		
		#endif

            need_report = 0;
        }
	
        usleep(100);
        gettimeofday(&tv, NULL);
        time_end = tv.tv_sec;
    } while ((time_start + work_time) > time_end);

}
#endif


/**************************************************************************
***************************Ali PAL Voice Module**************************
***************************************************************************/
#define PCM_FPATH 		"/tmp/sample.pcm"
#define OPUS_FPATH		"/tmp/sample.opus"
#define ALIPAL_VERSION "1.0"

void AliPal_Platform_Init(void)
{
	printf("*********************************************************************\n");
	printf("***CompDate:%s-%s Version:%s***\n", __DATE__, __TIME__, ALIPAL_VERSION);
	printf("**********************************************************************\n");
}

void WaitAllPthread(void)
{
	DestoryDecodePthread();
		
	DestoryEncodePthread();

	DestoryMonitorPthread();

	DestoryCapturePthread();
}

int main(int argc, char *argv[])
{
	int ret;

	AliPal_Platform_Init();
	
	AliPal_Voice_Config();

	CreateCapturePthread();

	CreateMonitorPthread();	

	CreateEncodePthread();

	CreateDecodePthread();
	
	WaitAllPthread();

	return 0;

}





