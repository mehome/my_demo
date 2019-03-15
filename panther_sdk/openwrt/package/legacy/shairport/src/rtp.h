#ifndef _RTP_H
#define _RTP_H

#include <sys/socket.h>
#include "common.h"

int rtp_setup(rtsp_conn_info * conn, int controlport, int timingport,
	      int *lsport, int *lcport, int *ltport);
void rtp_shutdown(void);
void rtp_request_resend(int sock, seq_t first, seq_t last);

#endif // _RTP_H
