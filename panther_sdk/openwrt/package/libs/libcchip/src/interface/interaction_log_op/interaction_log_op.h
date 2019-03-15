#ifdef __cplusplus
extern "C" {
#endif
#ifndef INTERACTION_LOG_OP_H_
#define INTERACTION_LOG_OP_H_ 1
#include "../../libcchip.h"

int path_get_size(size_t *p_size,char *path);

#define interactionPlog(path,fmt, args...) ({ \
    FILE *fp = fopen(path,"a"); \
    if (fp) { \
        fprintf(fp,fmt,##args); \
		fprintf(fp,"\n"); \
        fclose(fp); \
    } \
})

#define INTERACTION_LOG_MAX_SIZE  1024*1024*2
#define INTERACTION_LOG_SAVE_PATH "/tmp/misc/interaction.log"
#define interaction_log_capture(x...) ({\
	if(0==(access(INTERACTION_LOG_SAVE_PATH,F_OK))){\
		size_t logSize=0;\
		path_get_size(&logSize,INTERACTION_LOG_SAVE_PATH);\
		if(logSize <= INTERACTION_LOG_MAX_SIZE){\
			interactionPlog(INTERACTION_LOG_SAVE_PATH,x);\
		}else{\
			unlink(INTERACTION_LOG_SAVE_PATH);\
			interactionPlog(INTERACTION_LOG_SAVE_PATH,"your interaction log size over %u bytes, it has been reseted!",INTERACTION_LOG_MAX_SIZE);\
			interactionPlog(INTERACTION_LOG_SAVE_PATH,x);\
		}\
	}\
})

char* interaction_log_capture_enable();
int interaction_log_capture_disable();

#endif

#ifdef __cplusplus
}
#endif
