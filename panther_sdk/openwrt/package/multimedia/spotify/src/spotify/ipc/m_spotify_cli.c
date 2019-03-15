#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ipc/ipc_message.h"
#include "ipc/msg_handler.h"

int parse_args(int argc, char **argv, struct spotify_ipc_param *ctrl)
{
	if (argc > 1) {
		if (strcmp(argv[1], "d") == 0) {
			if (--argc == 2) {
				char display_name[128] = {0};
				ctrl->senderType = NOTIFY_SET_DISPLAY_NAME;
				ctrl->isNeedResp = 0;
				//_system("cdb set spotify_display_name %d", argv[2]);
				cdb_set("$spotify_display_name", argv[2]);
				system("cdb commit");
			}
		} else if (strcmp(argv[1], "s") == 0) {
				ctrl->senderType = NOTIFY_SPOTIFY_STOP;
				ctrl->isNeedResp = 0;
		} else if (strcmp(argv[1], "t") == 0) {
				ctrl->senderType = NOTIFY_SPOTIFY_TOGGLE;
				ctrl->isNeedResp = 0;
		} else if (strcmp(argv[1], "v") == 0) {
				if(!argv[2])goto error;
				ctrl->senderType = NOTIFY_SPOTIFY_VOLUME;
				ctrl->isNeedResp = 0;				
				memcpy(ctrl->data, argv[2], strlen(argv[2]));
				ctrl->len = strlen(argv[2]);
			} else {
			perror("Not support now");
			goto error;
		}
	} else {
		goto error;
	}
	return 0;
error:
	printf("commands:\n");
	printf("  d <display name> : set display name\n");
	printf("  s                : stop spotify\n");
	printf("  t                : toggle play/pause spotify\n");
	printf("  v [0-100]        : set spotify volume\n");
	return -1;
}

int main(int argc, char *argv[])
{
    int cfd = -1;
    struct sockaddr_un c_addr;
    struct spotify_ipc_param ctrl;

    memset(&ctrl, 0, sizeof(struct spotify_ipc_param));

    if (parse_args(argc, argv, &ctrl) == -1) {
        printf("fail to parse args\n");
        goto fail;
    }

    if ((cfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        printf("fail to create socket\n");
        goto fail;
    }
//    printf("sizeof spotify_ipc_param %d\n", sizeof(struct spotify_ipc_param));

    memset(&c_addr, 0, sizeof(struct sockaddr_un));
    c_addr.sun_family = AF_UNIX;
    strcpy(c_addr.sun_path, SPOTIFY_SOCK_S);

    if (connect(cfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
        printf("fail to connect\n");
        goto fail;
    }

//    printf("sizeof spotify_ipc_param %d\n", sizeof(struct spotify_ipc_param));
    if (send(cfd, &ctrl, sizeof(struct spotify_ipc_param), 0) == -1) {
        printf("fail to send\n");
        goto fail;
    }
    printf("\n\nsend notify spotify\n");

fail:
    if (cfd >= 0)
        close(cfd);
}
