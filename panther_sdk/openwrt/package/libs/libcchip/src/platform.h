#ifdef __cplusplus
extern "C" {
#endif
#ifndef PLATFORM_H_
#define PLATFORM_H_ 1
#include "function/common/misc.h"
#include "function/log/slog.h"
#include "function/alsa_extern/amixer_ctrl.h"
#include "function/dueros_socket_client/dueros_socket_client.h"
#include "function/thread_ops/posix_thread_ops.h"
#include "interface/interaction_log_op/interaction_log_op.h"
#include "function/alsa_extern/amixer_ctrl.h"
#include "function/common/px_timer.h"
#include "function/pwm_control/pwmconfig.h"
int uartd_fifo_write(char *contex);
int uartd_fifo_write_fmt(const char *fmt,...);
int uartd_fifo_write_data(char *contex, unsigned int len);

int timer_cdb_init(int signo);
int timer_cdb_save(void);

int tone_wav_play(const char *path);

inline void thread_wait(void);
void thread_resume(void);
void thread_resume_all(void);
void thread_pause(void);

#endif
#ifdef __cplusplus
}
#endif