/*
 * RTSP protocol handler. This file is part of Shairport.
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

#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mon.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#ifdef JSONC
#include <json.h>
#else
#include <json/json.h>
#endif

#include "common.h"
#include "player.h"
#include "rtp.h"
#include "mdns.h"

#ifdef AF_INET6
#define INETx_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#define INETx_ADDRSTRLEN INET_ADDRSTRLEN
#endif

int ctrlfd = -1;
int shair_pid;
// only one thread is allowed to use the player at once.
// it monitors the request variable (at least when interrupted)
static pthread_mutex_t playing_mutex = PTHREAD_MUTEX_INITIALIZER;
static int please_shutdown = 0;
static pthread_t playing_thread;
static pthread_t dacp_update_thread;

#define MAX_MUSIC_NAME_LEN 128
static char song_name[MAX_MUSIC_NAME_LEN];

struct dacp_info dacp_ctx;

// determine if we are the currently playing thread
static inline int rtsp_playing(void)
{
	if (pthread_mutex_trylock(&playing_mutex)) {
		return pthread_equal(playing_thread, pthread_self());
	} else {
		pthread_mutex_unlock(&playing_mutex);
		return 0;
	}
}

static void rtsp_take_player(void)
{
	if (rtsp_playing())
		return;

	if (pthread_mutex_trylock(&playing_mutex)) {
		debug(RTSP, "shutting down playing thread\n");
		// XXX minor race condition between please_shutdown and signal delivery
		please_shutdown = 1;
		pthread_kill(playing_thread, SIGUSR1);
		pthread_mutex_lock(&playing_mutex);
	}
	playing_thread = pthread_self();
}

void rtsp_shutdown_stream(void)
{
	rtsp_take_player();
	pthread_mutex_unlock(&playing_mutex);
}

// write conns to tmp file for mcc
static void update_state_to_file(int nconns)
{
	FILE *fp = fopen("/tmp/airinfo", "w");

	if (fp) {
		//fprintf(fp, "nconns=%d\n", nconns);
		fprintf(fp, "%d\n", nconns);
		fclose(fp);
	}
}

// keep track of the threads we have spawned so we can join() them
static rtsp_conn_info **conns = NULL;
static int nconns = 0;
static void track_thread(rtsp_conn_info * conn)
{
	conns = realloc(conns, sizeof(rtsp_conn_info *) * (nconns + 1));
	conns[nconns] = conn;
	nconns++;
	update_state_to_file(nconns);
}

static void cleanup_threads(void)
{
	void *retval;
	int i;
	for (i = 0; i < nconns;) {
		if (conns[i]->rtsp_running == 0) {
			if (conns[i]->thread)
				pthread_join(conns[i]->thread, &retval);
			free(conns[i]);
			nconns--;
			update_state_to_file(nconns);
			if (nconns)
				conns[i] = conns[nconns];
		} else {
			i++;
		}
	}
}

// park a null at the line ending, and return the next line pointer
// accept \r, \n, or \r\n
static char *nextline(char *in, int inbuf)
{
	char *out = NULL;
	while (inbuf) {
		if (*in == '\r') {
			*in++ = 0;
			out = in;
		}
		if (*in == '\n') {
			*in++ = 0;
			out = in;
		}

		if (out)
			break;

		in++;
		inbuf--;
	}
	return out;
}

typedef struct {
	int nheaders;
	char *name[16];
	char *value[16];

	int contentlength;
	char *content;

	// for requests
	char method[16];

	// for responses
	int respcode;
} rtsp_message;

static rtsp_message *msg_init(void)
{
	rtsp_message *msg = malloc(sizeof(rtsp_message));
	memset(msg, 0, sizeof(rtsp_message));
	return msg;
}

static int msg_add_header(rtsp_message * msg, char *name, char *value)
{
	if (msg->nheaders >= sizeof(msg->name) / sizeof(char *)) {
		warn("too many headers?!");
		return 1;
	}

	msg->name[msg->nheaders] = strdup(name);
	msg->value[msg->nheaders] = strdup(value);
	msg->nheaders++;

	return 0;
}

static char *msg_get_header(rtsp_message * msg, char *name)
{
	int i;
	for (i = 0; i < msg->nheaders; i++)
		if (!strcasecmp(msg->name[i], name))
			return msg->value[i];
	return NULL;
}

static void msg_free(rtsp_message * msg)
{
	int i;
	for (i = 0; i < msg->nheaders; i++) {
		free(msg->name[i]);
		free(msg->value[i]);
	}
	if (msg->content)
		free(msg->content);
	free(msg);
}

static int msg_handle_line(rtsp_conn_info * conn, rtsp_message ** pmsg,
			   char *line)
{
	rtsp_message *msg = *pmsg;

	if (!msg) {
		msg = msg_init();
		*pmsg = msg;
		char *sp, *p;

		air_syslog("RTSP Request: %s", line);
		p = strtok_r(line, " ", &sp);
		if (!p)
			goto fail;
		strncpy(msg->method, p, sizeof(msg->method) - 1);

		p = strtok_r(NULL, " ", &sp);
		if (!p)
			goto fail;

		p = strtok_r(NULL, " ", &sp);
		if (!p)
			goto fail;
		if (strcmp(p, "RTSP/1.0"))
			goto fail;

		return -1;
	}

	if (strlen(line)) {
		char *p;
		p = strstr(line, ": ");
		if (!p) {
			warn("bad header: >>%s<<", line);
			goto fail;
		}
		*p = 0;
		p += 2;
		msg_add_header(msg, line, p);
		debug(RTSP, "    %s: %s\n", line, p);
		return -1;
	} else {
		char *cl = msg_get_header(msg, "Content-Length");
		if (cl)
			return atoi(cl);
		else
			return 0;
	}

fail:
	*pmsg = NULL;
	msg_free(msg);
	return 0;
}

static rtsp_message *rtsp_read_request(rtsp_conn_info * conn)
{
	int fd = conn->fd;
	ssize_t buflen = 512;
	char *buf = malloc(buflen + 1);

	rtsp_message *msg = NULL;

	ssize_t nread;
	ssize_t inbuf = 0;
	int msg_size = -1;

	while (msg_size < 0) {
		if (please_shutdown) {
			goto shutdown;
		}
		nread = read(fd, buf + inbuf, buflen - inbuf);
		if (!nread) {
			debug(RTSP, "RTSP connection closed\n");
			goto shutdown;
		}
		if (nread < 0) {
			if (errno == EINTR)
				continue;
			perror("read failure");
			goto shutdown;
		}
		inbuf += nread;

		char *next;
		while (msg_size < 0 && (next = nextline(buf, inbuf))) {
			msg_size = msg_handle_line(conn, &msg, buf);

			if (!msg) {
				warn("no RTSP header received");
				goto shutdown;
			}

			inbuf -= next - buf;
			if (inbuf)
				memmove(buf, next, inbuf);
		}
	}

	if (msg_size > buflen) {
		buf = realloc(buf, msg_size);
		if (!buf) {
			warn("too much content");
			goto shutdown;
		}
		buflen = msg_size;
	}

	while (inbuf < msg_size) {
		nread = read(fd, buf + inbuf, msg_size - inbuf);
		if (!nread)
			goto shutdown;
		if ((nread < 0) && (errno == EINTR))
			continue;
		if (nread < 0) {
			perror("read failure");
			goto shutdown;
		}
		inbuf += nread;
	}

	msg->contentlength = inbuf;
	msg->content = buf;
	return msg;

shutdown:
	free(buf);
	if (msg) {
		msg_free(msg);
	}
	return NULL;
}

static void msg_write_response(int fd, rtsp_message * resp)
{
	char pkt[1024];
	int pktfree = sizeof(pkt);
	char *p = pkt;
	int i, n;

	n = snprintf(p, pktfree,
		     "RTSP/1.0 %d %s\r\n", resp->respcode,
		     resp->respcode == 200 ? "OK" : "Error");
	debug(RTSP, "RTSP Response: %s", pkt);
	pktfree -= n;
	p += n;

	for (i = 0; i < resp->nheaders; i++) {
		debug(RTSP, "    %s: %s\n", resp->name[i], resp->value[i]);
		n = snprintf(p, pktfree, "%s: %s\r\n", resp->name[i],
			     resp->value[i]);
		pktfree -= n;
		p += n;
		if (pktfree <= 0)
			die("Attempted to write overlong RTSP packet");
	}

	if (pktfree < 3)
		die("Attempted to write overlong RTSP packet");

	strcpy(p, "\r\n");
	write(fd, pkt, p - pkt + 2);
}

static void handle_options(rtsp_conn_info * conn,
			   rtsp_message * req, rtsp_message * resp)
{
	resp->respcode = 200;
	msg_add_header(resp, "Public",
		       "ANNOUNCE, SETUP, RECORD, "
		       "PAUSE, FLUSH, TEARDOWN, "
		       "OPTIONS, GET_PARAMETER, SET_PARAMETER");
}

static void handle_teardown(rtsp_conn_info * conn,
			    rtsp_message * req, rtsp_message * resp)
{
	if (!rtsp_playing())
		return;
	resp->respcode = 200;
	msg_add_header(resp, "Connection", "close");
	memset(song_name, 0, MAX_MUSIC_NAME_LEN);
	please_shutdown = 1;
}

static void handle_flush(rtsp_conn_info * conn,
			 rtsp_message * req, rtsp_message * resp)
{
	char *hdr;
	uint32_t rtptime = 0;
	static int first = 0;
	seq_t last_seq;
	if (!rtsp_playing())
		return;

	hdr = msg_get_header(req, "RTP-Info");

	if (hdr) {
		char *p, *t, *end;
		t = strstr(hdr, "rtptime=");
		p = strstr(hdr, "seq=");
		if (t) {
			t = strchr(t, '=') + 1;
			if (t)
				rtptime = strtoul(t, NULL, 0);
		}
		if (!p)
			return;
		p = strchr(p, '=') + 1;
		end = strchr(p, ';');
		end = '\0';
		last_seq = atoi(p);
		//warn("last seq:%d timestamp:%x\n",last_seq,rtptime);
	}
	player_flush(last_seq, rtptime);
	resp->respcode = 200;
}

static void dacp_ctrl(rtsp_conn_info * conn, rtsp_message * req)
{
	int ret;
	pthread_attr_t tattr;
	pthread_t tid;

	conn->active_remote = msg_get_header(req, "Active-Remote");
	conn->dacp_id = msg_get_header(req, "DACP-ID");

	if ((conn->active_remote == NULL) || (conn->dacp_id == NULL))
		return;

	if (dacp_ctx.dacp_id)
		free(dacp_ctx.dacp_id);
	if (dacp_ctx.active_remote)
		free(dacp_ctx.active_remote);

	dacp_ctx.dacp_id = strdup(conn->dacp_id);
	dacp_ctx.active_remote = strdup(conn->active_remote);
	dacp_ctx.start = 1;

	/* initialized with DETACHED attr recycle resouce auto */
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

	ret =
	    pthread_create(&dacp_update_thread, &tattr, dacp_thread_func, &dacp_ctx);
/*
	ret =
	    pthread_create(&dacp_update_thread, &tattr, dacp_update_thread_func, &dacp_ctx);
*/

	pthread_attr_destroy(&tattr);
	if (ret)
		die("Failed to create DACP update thread!");
	conn->dacp_thread = dacp_update_thread;
}

static void handle_setup(rtsp_conn_info * conn,
			 rtsp_message * req, rtsp_message * resp)
{
	int cport, tport;
	int lsport = 0, lcport = 0, ltport = 0;
	char *hdr;

	hdr = msg_get_header(req, "Transport");
	if (!hdr)
		return;

	char *p;
	p = strstr(hdr, "control_port=");
	if (!p)
		return;
	p = strchr(p, '=') + 1;
	cport = atoi(p);

	p = strstr(hdr, "timing_port=");
	if (!p)
		return;
	p = strchr(p, '=') + 1;
	tport = atoi(p);

	debug(RTSP, "control port:%d timing port:%d\n", cport, tport);
	rtsp_take_player();
	rtp_setup(conn, cport, tport, &lsport, &lcport, &ltport);
	if (!lsport)
		return;

	player_play(&conn->stream);
	dacp_ctrl(conn, req);
#if 0
	char *resphdr = malloc(strlen(hdr) + 20);
	strcpy(resphdr, hdr);
	sprintf(resphdr + strlen(resphdr), ";server_port=%d", sport);
#endif

	char resphdr[200];
	snprintf(resphdr, sizeof(resphdr),
		 "RTP/AVP/UDP;unicast;mode=record;server_port=%d;control_port=%d;timing_port=%d",
		 lsport, lsport, lsport);

	msg_add_header(resp, "Transport", resphdr);

	msg_add_header(resp, "Session", "1");

	debug(RTP, "server port:%d control port:%d\n", lsport, lcport);
	resp->respcode = 200;
}

static void handle_ignore(rtsp_conn_info * conn,
			  rtsp_message * req, rtsp_message * resp)
{
	resp->respcode = 200;
}

static void handle_record(rtsp_conn_info * conn,
			  rtsp_message * req, rtsp_message * resp)
{
	resp->respcode = 200;
	msg_add_header(resp, "Audio-Latency", "11025");

}

static void handle_set_parameter_metadata(rtsp_conn_info * conn,
					  rtsp_message * req,
					  rtsp_message * resp)
{
	char *cp = req->content;
	int cl = req->contentlength;

	unsigned int off = 8;

	while (off < cl) {
		char tag[5];
		strncpy(tag, cp + off, 4);
		tag[4] = '\0';
		off += 4;

		uint32_t vl = ntohl(*(uint32_t *) (cp + off));
		off += sizeof(uint32_t);

		char *val = malloc(vl + 1);
		strncpy(val, cp + off, vl);
		val[vl] = '\0';
		off += vl;

		if (!strncmp(tag, "minm ", 4)) {
			snprintf(song_name, MAX_MUSIC_NAME_LEN, "%s", val);
			debug(RTSP, "META Title: %s\n", val);
			free(val);
			return;
		} else
			warn("Other META Current Not Support\n");
#if 0
		if (!strncmp(tag, "asal ", 4)) {
			debug(RTSP, "META Album: %s\n", val);

		} else if (!strncmp(tag, "asar ", 4)) {
			debug(RTSP, "META Artist: %s\n", val);

		} else if (!strncmp(tag, "ascm ", 4)) {
			debug(RTSP, "META Comment: %s\n", val);

		} else if (!strncmp(tag, "asgn ", 4)) {
			debug(RTSP, "META Genre: %s\n", val);

		}
#endif
		free(val);
	}
}

static void handle_set_parameter_coverart(rtsp_conn_info * conn,
					  rtsp_message * req,
					  rtsp_message * resp)
{
	return;
}

static void handle_set_parameter_parameter(rtsp_conn_info * conn,
					   rtsp_message * req,
					   rtsp_message * resp)
{
	char *cp = req->content;
	int cp_left = req->contentlength;
	char *next;

	while (cp_left && cp) {
		next = nextline(cp, cp_left);
		cp_left -= next - cp;

		if (!strncmp(cp, "volume: ", 8)) {
			float volume = atof(cp + 8);
			debug(RTSP, "volume: %f\n", volume);
			player_volume(volume);
		} else if (!strncmp(cp, "progress: ", 10)) {
			char *progress = cp + 10;
			unsigned int start, curr, end;
			sscanf(progress, "%u/%u/%u", &start, &curr, &end);
			conn->total = (end - start) / 44100;
			conn->current =
			    (curr > start) ? (curr - start) / 44100 : 0;
		} else
			warn("Unrecognised parameter\n");
	}
}

static void handle_set_parameter(rtsp_conn_info * conn,
				 rtsp_message * req, rtsp_message * resp)
{

	char *ct = msg_get_header(req, "Content-Type");

	if (ct) {
		debug(RTSP, "SET_PARAMETER Content-Type: %s\n", ct);

		if (!strncmp(ct, "application/x-dmap-tagged", 25)) {
			debug(RTSP,
			      "received metadata tags in SET_PARAMETER request\n");

			handle_set_parameter_metadata(conn, req, resp);
		} else if (!strncmp(ct, "image/jpeg", 10) ||
			   !strncmp(ct, "image/png", 10) ||
			   !strncmp(ct, "image/none", 10)) {
			debug(RTSP,
			      "received image in SET_PARAMETER request\n");

			handle_set_parameter_coverart(conn, req, resp);
		} else if (!strncmp(ct, "text/parameters", 15)) {
			debug(RTSP,
			      "received parameters in SET_PARAMETER request\n");

			handle_set_parameter_parameter(conn, req, resp);
		} else
			warn("received unknown Content-Type %s in SET_PARAMETER request\n", ct);
	}

	resp->respcode = 200;
}

static void handle_announce(rtsp_conn_info * conn,
			    rtsp_message * req, rtsp_message * resp)
{

	char *paesiv = NULL;
	char *prsaaeskey = NULL;
	char *pfmtp = NULL;
    char *rtpmap = NULL;
	char *cp = req->content;
	int cp_left = req->contentlength;
	char *next;
	while (cp_left && cp) {
		next = nextline(cp, cp_left);
		cp_left -= next - cp;

		if (!strncmp(cp, "a=fmtp:", 7))
			pfmtp = cp + 7;

		if (!strncmp(cp, "a=aesiv:", 8))
			paesiv = cp + 8;

		if (!strncmp(cp, "a=rsaaeskey:", 12))
			prsaaeskey = cp + 12;

		if (!strncmp(cp, "a=rtpmap:", 9))
			rtpmap = cp+9;

		cp = next;
	}

    if ((paesiv == NULL) && (prsaaeskey == NULL)) {
        debug(RTSP,"Unencrypted session requested\n");
        conn->stream.encrypted = 0;
        } else {
        debug(RTSP,"Encrypted session requested\n");
        conn->stream.encrypted = 1;
    }

    if (conn->stream.encrypted) {
		int len, keylen;
		uint8_t *aesiv = base64_dec(paesiv, &len);
		if (len != 16) {
			warn("client announced aeskey of %d bytes, wanted 16", len);
			free(aesiv);
			return;
		}
		memcpy(conn->stream.aesiv, aesiv, 16);
		free(aesiv);

		uint8_t *rsaaeskey = base64_dec(prsaaeskey, &len);
		uint8_t *aeskey = rsa_apply(rsaaeskey, len, &keylen, RSA_MODE_KEY);
		free(rsaaeskey);
		if (keylen != 16) {
			warn("client announced rsaaeskey of %d bytes, wanted 16",
				 keylen);
			free(aeskey);
			return;
		}
		memcpy(conn->stream.aeskey, aeskey, 16);
		free(aeskey);
	}

	int i;
    char *token;
    for (i=0; i<sizeof(conn->stream.fmtp)/sizeof(conn->stream.fmtp[0]); i++)
    {
        if (!pfmtp)
            break;
        conn->stream.fmtp[i] = atoi(strsep(&pfmtp, " \t"));
    }

    for (i=0; i<sizeof(conn->stream.rtpmap)/sizeof(conn->stream.rtpmap[0]); i++)
    {
        if (!rtpmap)
            break;

        if (i==1) {
            if (strcmp(token = strsep(&rtpmap, " \t/"), "L16") == 0)
                conn->stream.rtpmap[i] = 0; // PCM
            else
                conn->stream.rtpmap[i] = 1; // ALAC
        }
        else
            conn->stream.rtpmap[i] = atoi(strsep(&rtpmap, " \t\\"));
    }

    resp->respcode = 200;
}

static struct method_handler {
	char *method;
	void (*handler) (rtsp_conn_info * conn, rtsp_message * req,
			 rtsp_message * resp);
} method_handlers[] = {
	{
	"OPTIONS", handle_options}, {
	"ANNOUNCE", handle_announce}, {
	"FLUSH", handle_flush}, {
	"TEARDOWN", handle_teardown}, {
	"SETUP", handle_setup}, {
	"GET_PARAMETER", handle_ignore}, {
	"SET_PARAMETER", handle_set_parameter}, {
	"RECORD", handle_record}, {
	NULL, NULL}
};

static void apple_challenge(int fd, rtsp_message * req, rtsp_message * resp)
{
	char *hdr = msg_get_header(req, "Apple-Challenge");
	if (!hdr)
		return;

	SOCKADDR fdsa;
	socklen_t sa_len = sizeof(fdsa);
	getsockname(fd, (struct sockaddr *)&fdsa, &sa_len);

	int chall_len;
	uint8_t *chall = base64_dec(hdr, &chall_len);
	uint8_t buf[48], *bp = buf;
	int i;
	memset(buf, 0, sizeof(buf));

	if (chall_len > 16) {
		warn("oversized Apple-Challenge!");
		free(chall);
		return;
	}
	memcpy(bp, chall, chall_len);
	free(chall);
	bp += chall_len;

#ifdef AF_INET6
	if (fdsa.SAFAMILY == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)(&fdsa);
		memcpy(bp, sa6->sin6_addr.s6_addr, 16);
		bp += 16;
	} else
#endif
	{
		struct sockaddr_in *sa = (struct sockaddr_in *)(&fdsa);
		memcpy(bp, &sa->sin_addr.s_addr, 4);
		bp += 4;
	}

	for (i = 0; i < 6; i++)
		*bp++ = config.hw_addr[i];

	int buflen, resplen;
	buflen = bp - buf;
	if (buflen < 0x20)
		buflen = 0x20;

	uint8_t *challresp = rsa_apply(buf, buflen, &resplen, RSA_MODE_AUTH);
	char *encoded = base64_enc(challresp, resplen);

	// strip the padding.
	char *padding = strchr(encoded, '=');
	if (padding)
		*padding = 0;

	msg_add_header(resp, "Apple-Response", encoded);
	free(challresp);
	free(encoded);
}

static char *make_nonce(void)
{
	uint8_t random[8];
	int fd = open("/dev/random", O_RDONLY);
	if (fd < 0)
		die("could not open /dev/random!");
	read(fd, random, sizeof(random));
	close(fd);
	return base64_enc(random, 8);
}

static int rtsp_auth(char **nonce, rtsp_message * req, rtsp_message * resp)
{

	if (!config.password)
		return 0;
	if (!*nonce) {
		*nonce = make_nonce();
		goto authenticate;
	}

	char *hdr = msg_get_header(req, "Authorization");
	if (!hdr || strncmp(hdr, "Digest ", 7))
		goto authenticate;

	char *realm = strstr(hdr, "realm=\"");
	char *username = strstr(hdr, "username=\"");
	char *response = strstr(hdr, "response=\"");
	char *uri = strstr(hdr, "uri=\"");

	if (!realm || !username || !response || !uri)
		goto authenticate;

	char *quote;
	realm = strchr(realm, '"') + 1;
	if (!(quote = strchr(realm, '"')))
		goto authenticate;
	*quote = 0;
	username = strchr(username, '"') + 1;
	if (!(quote = strchr(username, '"')))
		goto authenticate;
	*quote = 0;
	response = strchr(response, '"') + 1;
	if (!(quote = strchr(response, '"')))
		goto authenticate;
	*quote = 0;
	uri = strchr(uri, '"') + 1;
	if (!(quote = strchr(uri, '"')))
		goto authenticate;
	*quote = 0;

	uint8_t digest_urp[16], digest_mu[16], digest_total[16];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, username, strlen(username));
	MD5_Update(&ctx, ":", 1);
	MD5_Update(&ctx, realm, strlen(realm));
	MD5_Update(&ctx, ":", 1);
	MD5_Update(&ctx, config.password, strlen(config.password));
	MD5_Final(digest_urp, &ctx);

	MD5_Init(&ctx);
	MD5_Update(&ctx, req->method, strlen(req->method));
	MD5_Update(&ctx, ":", 1);
	MD5_Update(&ctx, uri, strlen(uri));
	MD5_Final(digest_mu, &ctx);

	int i;
	char buf[33];
	for (i = 0; i < 16; i++)
		sprintf(buf + 2 * i, "%02X", digest_urp[i]);
	MD5_Init(&ctx);
	MD5_Update(&ctx, buf, 32);
	MD5_Update(&ctx, ":", 1);
	MD5_Update(&ctx, *nonce, strlen(*nonce));
	MD5_Update(&ctx, ":", 1);
	for (i = 0; i < 16; i++)
		sprintf(buf + 2 * i, "%02X", digest_mu[i]);
	MD5_Update(&ctx, buf, 32);
	MD5_Final(digest_total, &ctx);

	for (i = 0; i < 16; i++)
		sprintf(buf + 2 * i, "%02X", digest_total[i]);

	if (!strcmp(response, buf))
		return 0;
	warn("auth failed");

authenticate:
	resp->respcode = 401;
	int hdrlen = strlen(*nonce) + 40;
	char *authhdr = malloc(hdrlen);
	snprintf(authhdr, hdrlen, "Digest realm=\"taco\", nonce=\"%s\"",
		 *nonce);
	msg_add_header(resp, "WWW-Authenticate", authhdr);
	free(authhdr);
	return 1;
}

static void *rtsp_conversation_thread_func(void *pconn)
{
	// SIGUSR1 is used to interrupt this thread if blocked for read
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	rtsp_conn_info *conn = pconn;

	rtsp_message *req, *resp;
	char *hdr, *auth_nonce = NULL;
	while ((req = rtsp_read_request(conn))) {
		resp = msg_init();
		resp->respcode = 400;

		apple_challenge(conn->fd, req, resp);
		hdr = msg_get_header(req, "CSeq");
		if (hdr)
			msg_add_header(resp, "CSeq", hdr);
		msg_add_header(resp, "Audio-Jack-Status",
			       "connected; type=analog");

		if (rtsp_auth(&auth_nonce, req, resp))
			goto respond;

		struct method_handler *mh;
		for (mh = method_handlers; mh->method; mh++) {
			if (!strcmp(mh->method, req->method)) {
				mh->handler(conn, req, resp);
				break;
			}
		}

respond:
		msg_write_response(conn->fd, resp);
		msg_free(req);
		msg_free(resp);
	}

	air_syslog("Closing RTSP connection\n");
	if (conn->fd > 0)
		close(conn->fd);
	if (rtsp_playing()) {
		rtp_shutdown();
		player_stop();
		please_shutdown = 0;
		pthread_mutex_unlock(&playing_mutex);
	}
	if (auth_nonce)
		free(auth_nonce);
	conn->rtsp_running = 0;
	air_syslog("Closing RTSP connection finished\n");
	return NULL;
}

// this function is not thread safe.
static const char *format_address(struct sockaddr *fsa)
{
	static char string[INETx_ADDRSTRLEN];
	void *addr;
#ifdef AF_INET6
	if (fsa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)(fsa);
		addr = &(sa6->sin6_addr);
	} else
#endif
	{
		struct sockaddr_in *sa = (struct sockaddr_in *)(fsa);
		addr = &(sa->sin_addr);
	}
	return inet_ntop(fsa->sa_family, addr, string, sizeof(string));
}

int create_ctrl_socket(void)
{
	int fd, len;
	struct sockaddr_un addr;
	struct ctrl_param *ctrl_arg;
	char msg[MAX_BUF_LEN];

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("Fail to create socket");
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, AIRS_SOCK);
	unlink(addr.sun_path);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("Fail to bind");
		close(fd);
		return -1;
	}
	return fd;
}

static int parse_handle_msg(const char *input, char *buf)
{
	struct ctrl_param *ctrl = (struct ctrl_param *)input;
	char str[32];
	char *cmd;
	int i;
	rtsp_conn_info *conn;

	switch (ctrl->type) {
	case VOLUME:
		memcpy(str, ctrl->data, ctrl->len);
		str[ctrl->len] = '\0';
		player_volume(atoi(str));
		break;
	case DACPCTRL: {
		struct dacp_info *dacpinfo = (struct dacp_info *)&(dacp_ctx);
		memcpy(str, ctrl->data, ctrl->len);
		str[ctrl->len] = '\0';
		if (dacpinfo->active_remote) {
			cmd = (char *)malloc(128 + strlen(dacpinfo->active_remote));
			sprintf(cmd,
				"curl 'http://%s:%d/ctrl-int/1/%s' -H 'Active-Remote: %s' -H 'Host: starlight.local.'",
				inet_ntoa(*((struct in_addr *)&dacpinfo->addr)), dacpinfo->port, str,
				dacpinfo->active_remote);
			air_syslog("----------  cmd: [%s]\n", cmd);
			fprintf(stderr, "cmd: %s\n", cmd);
			system(cmd);
			free(cmd);
			sprintf(buf, "%s", nconns ? str : "");
		}
		break;
	}
	case STATECTRL:
		if(nconns) {
			for (i = 0; i < nconns;) {
				conn = conns[i];
				if(pthread_equal(playing_thread, conn->thread)) {
					sprintf(buf, "%s", "YES");
					if (ctrl->len) {
						if ((strncmp(ctrl->data, "stop", ctrl->len) == 0)) {
							please_shutdown = 1;
							pthread_kill(playing_thread,SIGUSR1);
						}
						break;
					}
				}
				i++;
			}
			if(i==nconns)
				sprintf(buf, "%s", "NO");
		}
		else
			sprintf(buf, "%s", "NO");
		break;
	case AIRMUSIC:
		for (i = 0; i < nconns; i++) {
			conn = conns[i];
			if(pthread_equal(playing_thread, conn->thread)) {
				sprintf(buf,
					"name=%s\ntime=%.2u:%.2u,%.2u:%.2u,",
					song_name, (conn->total) / 60,
					(conn->total) % 60,
					(conn->current) / 60,
					(conn->current) % 60);
				break;
			}
		}
		break;
	default:
		perror("Not support now");
		break;
	}
	return 0;
}

static int handle_ctrl(int fd)
{
	struct ctrl_param *ctrl_arg;
	struct sockaddr_un addr, from;
	char msg[MAX_BUF_LEN];
	char buf[128];
	socklen_t fromlen = sizeof(from);
	ssize_t recvsize;

	recvsize =
	    recvfrom(fd, msg, MAX_BUF_LEN, 0, (struct sockaddr *)&from,
		     &fromlen);

	if (recvsize <= 0) {
		perror("Recv size less 0");
	}

	ctrl_arg = (struct ctrl_param *)msg;

	memset(buf, 0, 128);
	parse_handle_msg(msg, (char *)&buf);

	if (ctrl_arg->resp && strlen(buf) != 0)
		if (sendto(fd, &buf, strlen(buf)+1, 0, (struct sockaddr *)&from, fromlen)
		    == -1) {
			perror("ctrl socket fail to send");
		}
}

static void receive_event(struct ubus_context *ctx, struct ubus_event_handler *ev,
                          const char *type, struct blob_attr *msg)
{
	char *str;
	struct json_object *obj_msg, *obj_id;
	int i;
	rtsp_conn_info *conn;

	str = blobmsg_format_json(msg, true);
	obj_msg = json_tokener_parse(str);
	if (obj_msg && (obj_id = json_object_object_get(obj_msg, "sender"))) {
		if (strcmp("airplay", json_object_get_string(obj_id))) {
			if(nconns) {
				for (i = 0; i < nconns;) {
					conn = conns[i];
					if(pthread_equal(playing_thread, conn->thread)) {
						please_shutdown = 1;
						pthread_kill(playing_thread,SIGUSR1);
						break;
					}
					i++;
				}
			}
		}
	} else {
		debug(RTSP, "Failed to parse msg or no key named \"sender\" in event %s\n", type);
	}
	free(str);
}

static int ubus_event_register(struct ubus_context *ctx, char event[])
{
	static struct ubus_event_handler listener;
	int ret = 0;

	memset(&listener, 0, sizeof(listener));
	listener.cb = receive_event;

	ret = ubus_register_event_handler(ctx, &listener, event);

	if (ret) {
		debug(RTSP, "Error while registering for event '%s': %s\n", event, ubus_strerror(ret));
		return -1;
	}

	return 0;
}

void rtsp_listen_loop(void)
{
	struct addrinfo hints, *info, *p;
	char portstr[6];
	int *sockfd = NULL;
	int nsock = 0;
	int i, ret;
	struct ubus_context *ctx;
	char uevent_name[] = "stop_music";

	shair_pid = getpid();

	if ((ctrlfd = create_ctrl_socket()) < 0)
		die("fell out of the ctrlfd");

	ctx = ubus_connect(NULL);
	if (!ctx) {
		die("Shairport failed to connect to ubus");
	}
	ret = ubus_event_register(ctx, uevent_name);
	if (ret) {
		die("Shairport failed to register event");
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	snprintf(portstr, 6, "%d", config.port);

	debug(RTSP, "listen socket port request is \"%s\".\n", portstr);
	ret = getaddrinfo(NULL, portstr, &hints, &info);
	if (ret) {
		die("getaddrinfo failed: %s", gai_strerror(ret));
	}

	for (p = info; p; p = p->ai_next) {
		int fd = socket(p->ai_family, p->ai_socktype, IPPROTO_TCP);
		int yes = 1;

		ret =
		    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

#ifdef IPV6_V6ONLY
		// some systems don't support v4 access on v6 sockets, but some do.
		// since we need to account for two sockets we might as well
		// always.
		if (p->ai_family == AF_INET6)
			ret |=
			    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes,
				       sizeof(yes));
#endif

		if (!ret)
			ret = bind(fd, p->ai_addr, p->ai_addrlen);

		// one of the address families will fail on some systems that
		// report its availability. do not complain.
		if (ret) {
			debug(RTSP, "Failed to bind to address %s\n",
			      format_address(p->ai_addr));
			continue;
		}
		debug(RTSP, "Bound to address %s\n",
		      format_address(p->ai_addr));

		listen(fd, 5);
		nsock++;
		sockfd = realloc(sockfd, nsock * sizeof(int));
		sockfd[nsock - 1] = fd;
	}

	freeaddrinfo(info);

	if (!nsock)
		die("could not bind any listen sockets!--");

	int maxfd = -1;
	fd_set fds;

	mdns_register();

	debug(RTSP, "Listening for connections.\n");

	shairport_startup_complete();

	int acceptfd;
	struct timeval tv;

	air_syslog("AirPlay Start\n");
	while (1) {
		tv.tv_sec = 0;
		tv.tv_usec = 200 * 1000;

		FD_ZERO(&fds);

		for (i = 0; i < nsock; i++) {
			FD_SET(sockfd[i], &fds);
			if (sockfd[i] > maxfd)
				maxfd = sockfd[i];
		}

		if (ctrlfd >= 0) {
			FD_SET(ctrlfd, &fds);
			maxfd = MAX(maxfd, ctrlfd);
		}

		FD_SET(ctx->sock.fd, &fds);
		maxfd = MAX(maxfd, ctx->sock.fd);

		ret = select(maxfd + 1, &fds, 0, 0, &tv);
		if (ret == 0)
			continue;
		else if (ret == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		cleanup_threads();

		acceptfd = -1;
		for (i = 0; i < nsock; i++) {
			if (FD_ISSET(sockfd[i], &fds)) {
				acceptfd = sockfd[i];
				break;
			}
		}

		if (FD_ISSET(ctrlfd, &fds)) {
			handle_ctrl(ctrlfd);
		}

		if (FD_ISSET(ctx->sock.fd, &fds)) {
			ubus_handle_event(ctx);
		}

		/*Avoid getting teardown, and get rtsp connect */
		if (please_shutdown) {
			air_syslog("new RTSP connection but during shutdown skip wait next\n");
			continue;
		}

		if (acceptfd > 0) {
			rtsp_conn_info *conn = malloc(sizeof(rtsp_conn_info));
			memset(conn, 0, sizeof(rtsp_conn_info));
			socklen_t slen = sizeof(conn->remote);

			conn->fd =
			    accept(acceptfd, (struct sockaddr *)&conn->remote,
				   &slen);

			if (conn->fd < 0) {
				perror("failed to accept connection");
				free(conn);
			} else {
				pthread_t rtsp_conversation_thread;
				ret =
				    pthread_create(&rtsp_conversation_thread,
						   NULL,
						   rtsp_conversation_thread_func,
						   conn);

				if (ret)
					die("Failed to create RTSP receiver thread!");

				conn->thread = rtsp_conversation_thread;
				conn->rtsp_running = 1;
				track_thread(conn);
			}
		}
	}
	perror("select");
	die("fell out of the RTSP select loop");
}
