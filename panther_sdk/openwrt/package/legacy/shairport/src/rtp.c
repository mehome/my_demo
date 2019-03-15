/*
 * Apple RTP protocol handler. This file is part of Shairport.
 * Copyright (c) James Laird 2013
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "common.h"
#include "player.h"

// only one RTP session can be active at a time.
static int rtp_running = 0;
static int please_shutdown;

static char client_ip_string[INET6_ADDRSTRLEN];	// the ip string pointing to the client
static SOCKADDR rtp_client_control_socket;

static int control_socket;
static int timing_socket;

static SOCKADDR rtp_client;
static pthread_t rtp_audio_thread;
static pthread_t rtp_control_thread;

static void *rtp_audio_receiver(void *arg)
{
	rtsp_conn_info *conn = (rtsp_conn_info *) arg;
	int audio_socket = conn->audio_socket;
	// we inherit the signal mask (SIGUSR1)
	uint8_t packet[2048], *pktp;
	ssize_t nread;
	fd_set fdread;
	int maxfd = -1;
	int ret = 0;
	struct timeval tv_in;

	while (1) {
		tv_in.tv_sec = 0;
		tv_in.tv_usec = 500000;

		FD_ZERO(&fdread);
		FD_SET(audio_socket, &fdread);
		maxfd = MAX(maxfd, audio_socket);

		ret = select(maxfd+1, &fdread, NULL, NULL, &tv_in);

		if (please_shutdown)
			break;

		if(ret == 0)
			continue;
		else if (ret == -1) {
			if(errno == EINTR)
				continue;
			break;
		}

		if(FD_ISSET(audio_socket, &fdread))
			nread = recv(audio_socket, packet, sizeof(packet), 0);
		else
			continue;
		if (nread < 0)
			break;

		ssize_t plen = nread;
		uint8_t type = packet[1] & ~0x80;
		if (type == 0x60 || type == 0x56 || type == 0x54) {	// audio data / resend / sync
			pktp = packet;

			if (type == 0x54) {
				unsigned int next_minus_latency;
				unsigned int next;
				unsigned char diff;
				char time[256];

				next_minus_latency =
				    ntohl(*(unsigned int *)&(pktp[4]));
				next = ntohl(*(unsigned int *)&(pktp[16]));
				air_syslog("!!!! Sync next pkt timestamp:%x-%x = latency:%d\n",next,next_minus_latency ,next-next_minus_latency);

				conn->current++;
				if (conn->current > conn->total)
					conn->current = conn->total;
				continue;
			}

			if (type == 0x56) {
				pktp += 4;
				plen -= 4;
			}
			seq_t seqno = ntohs(*(unsigned short *)(pktp + 2));

			uint32_t timestamp =
			    ntohl(*(unsigned long *)(pktp + 4));

			pktp += 12;
			plen -= 12;

			// check if packet contains enough content to be reasonable
			if (plen >= 16) {
				player_put_packet(conn, seqno, timestamp, pktp,
						  plen);
				continue;
			}
			if (type == 0x56 && seqno == 0) {
				debug(RTP,
				      "resend-related request packet received, ignoring.\n");
				continue;
			}
		}
		//debug(RTP, "Unknown RTP packet of type 0x%02X length %d\n", type, nread);
	}

	close(audio_socket);

	return NULL;
}

#if 0
static void *rtp_control_receiver(void *arg)
{
	uint8_t packet[2048], *pktp;
	ssize_t nread;

	while (1) {
		if (please_shutdown)
			break;
		nread = recv(control_socket, packet, sizeof(packet), 0);
		if (nread <= 0)
			break;

		ssize_t plen = nread;
		uint8_t type = packet[1] & ~0x80;

		if (type == 0x54) {
			debug(RTP, "Sync Packet Received:\n");
		} else if (type == 0x56) {
			pktp = packet + 4;
			plen -= 4;

			seq_t seqno = ntohs(*(unsigned short *)(pktp + 2));

			pktp += 12;
			plen -= 12;

			if (plen >= 16) {
				debug(RTP, "Got Resend pkt seqno:%d\n", seqno);
				player_put_packet(seqno, pktp, plen);
				continue;
			}
			if (type == 0x56 && seqno == 0) {
				debug(RTP,
				      "resend-related request packet received, ignoring.\n");
				continue;
			}
		} else
			debug(RTP,
			      "Control Port -- Unknown RTP packet of type 0x%02X length %d.",
			      packet[1], nread);
	}
	debug(CTRL, "Control RTP thread interrupted. terminating.");
	close(control_socket);

	return NULL;
}
#endif

static int bind_port(SOCKADDR * remote, int *sock)
{
	struct addrinfo hints, *info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = remote->SAFAMILY;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	int ret = getaddrinfo(NULL, "0", &hints, &info);

	if (ret < 0)
		die("failed to get usable addrinfo?! %s", gai_strerror(ret));

	*sock = socket(remote->SAFAMILY, SOCK_DGRAM, IPPROTO_UDP);
	ret = bind(*sock, info->ai_addr, info->ai_addrlen);

	freeaddrinfo(info);

	if (ret < 0)
		die("could not bind a UDP port!");

	int sport;
	SOCKADDR local;
	socklen_t local_len = sizeof(local);
	getsockname(*sock, (struct sockaddr *)&local, &local_len);
#ifdef AF_INET6
	if (local.SAFAMILY == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&local;
		sport = htons(sa6->sin6_port);
	} else
#endif
	{
		struct sockaddr_in *sa = (struct sockaddr_in *)&local;
		sport = htons(sa->sin_port);
	}

	return sport;
}

void rtp_setup(rtsp_conn_info * conn, int cport, int tport, int *lsport,
	       int *lcport, int *ltport)
{
	SOCKADDR *remote = (SOCKADDR *) & (conn->remote);
	char portstr[20];

	if (rtp_running)
		die("rtp_setup called with active stream!");

	debug(RTP, "rtp_setup: cport=%d tport=%d\n", cport, tport);

	// we do our own timing and ignore the timing port.
	// an audio perfectionist may wish to learn the protocol.

	memcpy(&rtp_client, remote, sizeof(rtp_client));
#ifdef AF_INET6
	if (rtp_client.SAFAMILY == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&rtp_client;
		sa6->sin6_port = htons(cport);
	} else
#endif
	{
		struct sockaddr_in *sa = (struct sockaddr_in *)&rtp_client;
		sa->sin_port = htons(cport);
	}

	*lsport = bind_port(remote, &(conn->audio_socket));
	*lcport = bind_port(remote, &control_socket);
	*ltport = bind_port(remote, &timing_socket);

	debug(RTP, "Listen for audio, control, timing on ports: %d, %d, %d\n",
	      *lsport, *lcport, *ltport);

	please_shutdown = 0;
	pthread_create(&rtp_audio_thread, NULL, &rtp_audio_receiver, conn);
	//pthread_create(&rtp_control_thread, NULL, &rtp_control_receiver, NULL);

	rtp_running = 1;
	return;
}

void rtp_shutdown(void)
{
	if (!rtp_running)
		die("rtp_shutdown called without active stream!");

	please_shutdown = 1;
	pthread_kill(rtp_audio_thread, SIGUSR1);
	void *retval;
	pthread_join(rtp_audio_thread, &retval);
	rtp_running = 0;
}

void rtp_request_resend(int audio_socket, seq_t first, seq_t last)
{
	if (!rtp_running)
		return;

	char req[8];		// *not* a standard RTCP NACK
	req[0] = 0x80;
	req[1] = 0x55 | 0x80;	// Apple 'resend'
	*(unsigned short *)(req + 2) = htons(1);	// our seqnum
	*(unsigned short *)(req + 4) = htons(first);	// missed seqnum
	*(unsigned short *)(req + 6) = htons(last - first + 1);	// count

	sendto(audio_socket, req, sizeof(req), 0,
	       (struct sockaddr *)&rtp_client, sizeof(rtp_client));
/*	
    if(sendto(control_socket, req, sizeof(req), 0, (struct sockaddr*)&rtp_client_control_socket, sizeof(struct sockaddr_in)) == -1)
		debug(RTP,"Error sento-ing to the audio spcket\n");
*/
}
