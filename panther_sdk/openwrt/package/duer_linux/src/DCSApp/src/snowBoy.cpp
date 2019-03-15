#include <Others/snowboy-detect.h>
#include <stdio.h>

#define RESOURE_FILE_NAME   "/etc/universal_detect.res"
#define RESOURE_MODEL_NAME  "/etc/hotword.umdl"
#define SENSITIVITY         "0.8"

int snowboy_handle(char *argv[]){
	int hotword =0;
	int count = 0;
	short g_buffer[1600];
	bool is_end = false;
	SnowboyInit(RESOURE_FILE_NAME, RESOURE_MODEL_NAME, true);
	SnowboySetSensitivity (SENSITIVITY);
	FILE* fd = fopen(argv[1], "rb");
	while (!feof(fd)) {
		count = fread(g_buffer, 2, 1600, fd);
		hotword = SnowboyRunDetection(g_buffer, count, is_end);
		if (hotword > 0) {
			printf("YES, hotword %d detected.\n", hotword);
			SnowboyReset();
		}
	}
	return 0;
}

