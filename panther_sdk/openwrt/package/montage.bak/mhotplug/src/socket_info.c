#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socket_info.h"

#define PATH "/var/connect.socket"

#define C_PATH "/var/"
#define MAX 64

void usage(){
    printf("Usage: lcd [options]\n");
    printf("Provide basic lcd service, control it  by st7565p_cli\n");
    printf("\n");
    printf("Options:\n");
    printf("    i                           intialize lcd\n");
    printf("    k                           kill lcd host\n");
    printf("    c                           clear lcd screen\n");
    printf("    s				show standby mode text\n");
    printf("    p <station>                 playing station name\n");
    printf("    t                           pause playing\n");
    printf("    f <no> <station>            favorite station show and add\n");
    printf("    m <no> <dev>                mode show and select\n");
    printf("    v <value>                   volume animation\n");
    printf("\n");
}

int parse_args(int argc, char **argv, char *buf)
{
    const char delims[8] = ",";

    memset(buf, 0, MAX);

    if (argc > 1) {
        strcat(buf, argv[1]);
        strcat(buf, delims);

        // initial
        if (strcmp(argv[1], "i") == 0)
            ;
        // kill socket
        else if (strcmp(argv[1], "k") == 0)
        {
            if (argc > 2)
            {
                strcat(buf, argv[2]);
                strcat(buf, delims);
            }
        }
        // clear
        else if (strcmp(argv[1], "c") == 0)
            ;
        else if (strcmp(argv[1], "n") == 0)
            ;
        // standby mode
        else if (strcmp(argv[1], "s") == 0)
        {
            if (argc > 2)
            {
                if (atoi(argv[2]) > 5|| atoi(argv[2]) < 1)
                    goto error;
                else
                {
                    strcat(buf, argv[2]);
                    strcat(buf, delims);
                    if (argc > 3)
                    {
                        strcat(buf, argv[3]);
                        strcat(buf, delims);
                    }
                }
            }
        }
        // paly station
        else if (strcmp(argv[1], "p") == 0) {
            if (argc == 3) {
                strcat(buf, argv[2]);
                strcat(buf, delims);
            }
        }
        // pause playing
        else if (strcmp(argv[1], "t") == 0)
            ;
        // favorite
        else if (strcmp(argv[1], "f") == 0) {
            if (argc > 2) {
                if (atoi(argv[2]) > 6 || atoi(argv[2]) < 1)
                    goto error;
                else {
                    strcat(buf, argv[2]);
                    strcat(buf, delims);

                    if (argc > 3) {
                        strcat(buf, argv[3]);
                        strcat(buf, delims);
                    }
                }
            }
        }
        // player mode
        else if (strcmp(argv[1], "m") == 0) {
            if (argc > 2) {
                if (atoi(argv[2]) > 5 || atoi(argv[2]) < 1)
                    goto error;
                else {
                    strcat(buf, argv[2]);
                    strcat(buf, delims);

                    if (argc > 3) {
                        strcat(buf, argv[3]);
                        strcat(buf, delims);
                    }
                }
            }
        }
        // volume
        else if (strcmp(argv[1], "v") == 0) {
            if (argc > 2) {
                if (atoi(argv[2]) > 100 || atoi(argv[2]) < 0)
                    goto error;
                else {
                    strcat(buf, argv[2]);
                    strcat(buf, delims);
                }
            }
        }
        else
            goto error;
    }
    else {
        goto error;
    }

    return 0;
error:
    usage();
    return -1;

}

int socket_main(int argc, char *argv[])
{
    int                cfd, len;
    struct sockaddr_un un_addr;
    char               buf[MAX];

    if (parse_args(argc, argv, buf) == -1)
    {
        return 1;
    }

    if ((cfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("fail to create socket");
        return 2;
    }

    memset(&un_addr, 0, sizeof(struct sockaddr_un));
    un_addr.sun_family = AF_UNIX;

    sprintf(un_addr.sun_path, "%s%d", C_PATH, getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un_addr.sun_path);

    unlink(un_addr.sun_path);

    if (bind(cfd, (struct sockaddr *)&un_addr, len) == -1) {
        perror("fail to bind");
        return 3;
    }

    if (chmod(un_addr.sun_path, S_IRWXU) < 0) {
        perror("fail to change model");
        return 4;
    }

    memset(&un_addr, 0, sizeof(struct sockaddr_un));
    un_addr.sun_family = AF_UNIX;
    strcpy(un_addr.sun_path, PATH);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(un_addr.sun_path);

    if (connect(cfd, (struct sockaddr *)&un_addr, len) < 0) {
        perror("fail to connect");
        return 5;
    }

    if (write(cfd, buf, strlen(buf) + 1) == -1) {
        //if (write(cfd, args, sizeof(struct user_args)) == -1){
        perror("fail to write");
        return 6;
    }
#if 1
    if (read(cfd, buf, MAX) == -1) {
        //if (read(cfd, args, sizeof(struct user_args)) == -1){
        perror("peer read");
        return 7;
    }

    printf("%s\n", buf);
#endif
    close(cfd);

    return 0;
}
