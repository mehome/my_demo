
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <mpd/client.h>
#include "mon.h"

static unsigned int send_airplay_msg(struct ctrl_param *ctrl)
{
	int cfd = -1;
	struct sockaddr_un c_addr;
	int ret = 1;

	if ((cfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("fail to create socket");
		goto fail;
	}

	memset(&c_addr, 0, sizeof(struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, AIRC_SOCK);
	unlink(AIRC_SOCK);

	if (bind(cfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
		perror("fail to bind");
		goto fail;
	}

	memset(&c_addr, 0, sizeof(struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, AIRS_SOCK);

	if (connect(cfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
		perror("fail to connect");
		goto fail;
	}

	if (send(cfd, ctrl, sizeof(struct ctrl_param) - 1 + ctrl->len, 0) == -1) {
		perror("fail to send");
		goto fail;
	}

	if (ctrl->resp) {
		struct timeval tv;
		fd_set fds;

		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		FD_ZERO(&fds);
		FD_SET(cfd, &fds);

		select(cfd + 1, &fds, 0, 0, &tv);

		if (FD_ISSET(cfd, &fds)) {
			ssize_t recvsize;
			char buf[256];
			
			if ((recvsize = recv(cfd, buf, sizeof(buf), 0)) < 0) {
				goto fail;
			}
			printf("%s\n", buf);
			ret = 0;
		}
	}
fail:
	if (cfd >= 0)
		close(cfd);

	return ret;
}

static unsigned int stop_dlna(void)
{
	struct mpd_connection *conn;
	
	conn = mpd_connection_new("127.0.0.1", 0, 0);
	
	if (conn == NULL) {
		 return 1;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		mpd_connection_free(conn);
		return 1;
	}

	mpd_run_dlna_pause(conn, true);
	mpd_run_clear(conn);
	//mpd_run_toggle_pause(conn);
	mpd_connection_free(conn);
	
	return 0;
}

static unsigned int stop_airplay(void)
{
	struct ctrl_param ctrl;
	int ret;
	
	memset(&ctrl, 0, sizeof(struct ctrl_param));

	ctrl.type = STATECTRL;
	memcpy(&ctrl.data, "stop", 4);
	ctrl.len = 4;
	ctrl.resp = 1;

	ret = send_airplay_msg(&ctrl);

	return ret;
}

static unsigned int stop_spotify(void)
{
    int cfd = -1; 
    struct sockaddr_un c_addr; 
    struct spotify_ipc_param ctrl;

    memset(&ctrl, 0, sizeof(struct spotify_ipc_param));

    if ((cfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("fail to create socket");
        goto fail;
    }

    memset(&c_addr, 0, sizeof(struct sockaddr_un));
    c_addr.sun_family = AF_UNIX;
    strcpy(c_addr.sun_path, SPOTS_SOCK);

    if (connect(cfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
        perror("fail to connect\n\n");
        goto fail;
    }
    ctrl.senderType = NOTIFY_SPOTIFY_STOP;
    if (send(cfd, &ctrl, sizeof(struct spotify_ipc_param), 0) == -1) {
        perror("fail to send\n\n");
        goto fail;
    }
    perror("\n\nsend notify spotify\n");

fail:
    if (cfd >= 0)
        close(cfd);
}

static unsigned int notify_dlna(void)
{
	struct mpd_connection *conn;
	
	conn = mpd_connection_new("127.0.0.1", 0, 0);
	
	if (conn == NULL) {
		 return 1;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		mpd_connection_free(conn);
		return 1;
	}

	mpd_run_dlna_pause(conn, false);
	mpd_connection_free(conn);
	
	return 0;
}

unsigned int ctrl_play_dlna(unsigned int curmode)
{
	notify_dlna();
	
	if(curmode == MONAIRPLAY)
		return stop_airplay();
	else if(curmode == MONSPOTIFY)
		return stop_spotify();
	else {
		return 0;
	}
}

unsigned int ctrl_play_airplay(unsigned int curmode)
{
	if(curmode == MONDLNA)
		return stop_dlna();
	else if (curmode == MONSPOTIFY)
		return stop_spotify();
}

unsigned int ctrl_play_spotify(unsigned int curmode)
{
	if(curmode == MONDLNA)
		return stop_dlna();
	else if(curmode == MONAIRPLAY)
		return stop_airplay();
}

unsigned int inform_ctrl_msg(char cmd, char *data)
{
	int monfd = -1, len;
	struct sockaddr_un c_addr;
	struct mon_param mon;
	char buf[MAX_BUF_LEN];

	memset(&mon, 0, sizeof(struct mon_param));
	
	switch (cmd)
	{
		case MONDLNA:
		case MONAIRPLAY:
		case MONSPOTIFY:
			mon.type = cmd;
			len = sizeof(struct mon_param);
			break;
		default:
			perror("Not support now");
			return;
	}

	if ((monfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("fail to create socket");
		goto fail;
	}

	memset(&c_addr, 0, sizeof(struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, MONC_SOCK);
	unlink(MONC_SOCK);

	if (bind(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
		perror("fail to bind");
		goto fail;
	}

	memset(&c_addr, 0, sizeof(struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, MONS_SOCK);

	if (connect(monfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
		perror("fail to connect");
		goto fail;
	}

	if (send(monfd, &mon, len, 0) == -1) {
		perror("fail to send");
		goto fail;
	}

	if (mon.resp) {
		struct timeval tv;
		fd_set fds;

		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		FD_ZERO(&fds);
		FD_SET(monfd, &fds);

		select(monfd + 1, &fds, 0, 0, &tv);

		if (FD_ISSET(monfd, &fds)) {
			ssize_t recvsize;
			struct sockaddr_un from;
			socklen_t fromlen = sizeof(from);

			char buf[256];
			if ((recvsize = recv(monfd, buf, sizeof(buf), 0)) < 0) {
				goto fail;
			}
			printf("%s\n", buf);
		}
	}
fail:
	if (monfd >= 0)
		close(monfd);

	return 0;

}
