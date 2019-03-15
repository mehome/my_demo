
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mon.h>
#include "common.h"
#include "player.h"

int parse_args(int argc, char **argv, struct ctrl_param *ctrl)
{
	if (argc > 1) {
		if (strcmp(argv[1], "v") == 0) {
			if (--argc == 2) {
				ctrl->type = VOLUME;
				memcpy(ctrl->data, argv[2], strlen(argv[2]));
				ctrl->len = strlen(argv[2]);
			}
		} else if (strcmp(argv[1], "d") == 0) {
			if (--argc == 2) {
				ctrl->type = DACPCTRL;
				memcpy(ctrl->data, argv[2], strlen(argv[2]));
				ctrl->len = strlen(argv[2]);
				if(strstr(ctrl->data, "volume"))
					ctrl->resp = 1;
			}
		} else if (strcmp(argv[1], "s") == 0) {
			ctrl->type = STATECTRL;
			if (--argc == 2) {
				memcpy(ctrl->data, argv[2], strlen(argv[2]));
				ctrl->len = strlen(argv[2]);
			}
			ctrl->resp = 1;
		} else if (strcmp(argv[1], "a") == 0) {
			ctrl->type = AIRMUSIC;
			ctrl->resp = 1;
		} else {
			perror("Not support now");
			goto error;
		}
	} else {
		goto error;
	}
	return 0;
error:
	return -1;

}

int main(int argc, char *argv[])
{
	int cfd = -1, len;
	struct sockaddr_un c_addr;
	struct ctrl_param ctrl;
	char buf[MAX_BUF_LEN];

	memset(&ctrl, 0, sizeof(struct ctrl_param));

	if (parse_args(argc, argv, &ctrl) == -1)
		goto fail;

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

	if (send(cfd, &ctrl, sizeof(struct ctrl_param) - 1 + ctrl.len, 0) == -1) {
		perror("fail to send");
		goto fail;
	}

	if (ctrl.resp) {
		struct timeval tv;
		fd_set fds;

		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		FD_ZERO(&fds);
		FD_SET(cfd, &fds);

		select(cfd + 1, &fds, 0, 0, &tv);

		if (FD_ISSET(cfd, &fds)) {
			ssize_t recvsize;
			struct sockaddr_un from;
			socklen_t fromlen = sizeof(from);

			char buf[256];
			if ((recvsize = recv(cfd, buf, sizeof(buf), 0)) < 0) {
				goto fail;
			}
			printf("%s\n", buf);
		}
	}
fail:
	if (cfd >= 0)
		close(cfd);

	return 0;
}
