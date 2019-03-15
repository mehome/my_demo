#ifndef _PLAYER_H
#define _PLAYER_H

#include "audio.h"

typedef struct {
    int encrypted;
	uint8_t aesiv[16], aeskey[16];
	int32_t fmtp[12];
    int32_t rtpmap[4];
} stream_cfg;

typedef uint16_t seq_t;

// wrapped number between two seq_t.
static inline uint16_t seq_diff(seq_t a, seq_t b)
{
	int16_t diff = b - a;
	return diff;
}

int player_play(stream_cfg * cfg);
void player_stop(void);

void player_volume(double f);
void player_flush(seq_t seqno, uint32_t rtptime);
void player_resync(void);

void player_put_packet(void *conn, seq_t seqno, uint32_t timestamp,
		       uint8_t * data, int len);
void player_set_timestamp(uint32_t timestamp);
#endif //_PLAYER_H
