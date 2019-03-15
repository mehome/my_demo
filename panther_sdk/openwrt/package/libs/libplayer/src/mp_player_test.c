/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*
*   \file
*   \brief
*   \author Montage
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <player.h>
#include <pthread.h>
#include <unistd.h>

#define TEST_CASE_NUM 16
#define AVS_WRITE_SIZE 4096
#define TEST_FIFO "/tmp/mp_player_fifo"

static player_t *player;
static char *test_uri;
static int interval = 5;
static FILE *test_fp = NULL;

void set_mplayer_status (player_t *player, mplayer_status_t status)
{
  mplayer_t *mplayer = mp_player_get_priv(player);

  if (!mplayer)
  {
      mplayer->status = MPLAYER_IS_DEAD;
      return;
  }

  //pthread_mutex_lock (&mplayer->mutex_status);
  mplayer->status = status;
  //pthread_mutex_unlock (&mplayer->mutex_status);
}
static mplayer_status_t get_mplayer_status (player_t *player)
{
  mplayer_t *mplayer = mp_player_get_priv(player);
  mplayer_status_t status;

  if (!mplayer)
    return MPLAYER_IS_DEAD;

  //pthread_mutex_lock (&mplayer->mutex_status);
  status = mplayer->status;
  //pthread_mutex_unlock (&mplayer->mutex_status);
  return status;
}
void init_fifo(void)
{
    mkfifo(TEST_FIFO, 0666);
}
void uninit_fifo()
{
    if(0 != unlink(TEST_FIFO)) {
        printf("destroy fifo fail\n");
    }
}
int open_fifo(void)
{
    int fd;
    test_fp = fopen(TEST_FIFO, "w+");
    if(!test_fp) {
        printf("open fifo fail\n");
        return -1;
    }
    fd = fileno(test_fp);
    fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL) | O_NONBLOCK));

    return 0;
}
void close_fifo(void)
{
    fclose(test_fp);
}
void output_stream_writer(char *start, int size)
{
    int result = 0, wsize = size;

again:
    result = fwrite(start, sizeof(char), sizeof(char) * wsize, test_fp);
#if 0
    if(result != wsize) {
        printf("write fail wsize %d result %d\n", wsize, result);
        start += result;
        wsize -= result;
        sleep(1);
        goto again;
    }
    else
#endif
    if(result == wsize)
    {
        printf("write %d\n", wsize);
    }
    else
    {
       printf("result %d wsize %d\n", result, wsize);
    }
}
void output_stream_worker(char *buf, int size, int mode)
{
    int remain = size;
    char *str = buf;

    if(mode == 0) {
        while(1) {
            if(remain < AVS_WRITE_SIZE) {
                output_stream_writer(str, remain);
                break;
            }
            else
            {
                output_stream_writer(str, AVS_WRITE_SIZE);
                str += AVS_WRITE_SIZE;
                remain -= AVS_WRITE_SIZE;
            }
        }
    }
    else if(mode == 1) {
        while(1) {
            if(remain < AVS_WRITE_SIZE) {
                output_stream_writer(str, remain);
                break;
            }
            else
            {
                output_stream_writer(str, AVS_WRITE_SIZE);
                str += AVS_WRITE_SIZE;
                remain -= AVS_WRITE_SIZE;
            }
            usleep(100000);
        }
    }
}
void irrigate_stream(int mode)
{
    FILE *fp;
    int size;
    char *buf;
    size_t result;

    fp = fopen(test_uri, "r");
    if(!fp) {
        printf("file : (%s) not found\n", test_uri);
        return;
    }
    fseek(fp , 0 , SEEK_END);
    size = ftell(fp);
    rewind(fp);
    printf("(%s) file size: %d\n", test_uri, size);
    buf = (char *) malloc(sizeof(char) *size);
    if (!buf) {
        printf("malloc fail\n");
        return;
    }
    result = fread(buf, 1, size, fp);
    if(result != size) {
        printf("read file error\n");
        return;
    }
    output_stream_worker(buf, size, mode);

    free(buf);

    fclose(fp);
}
void *recv_msg(void *arg)
{
    for(;;)
    {
        mp_player_handle_massage(player);

        usleep(100);
    }
}
void set_url_2play_2finish_test()
{
    printf("[ RUN\t\t] SET URL -> PLAY -> FINISH\n");
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    printf("[ \t\tOK] SET -> PLAY -> FINISH\n");
}
void set_url_2play_2pause_test()
{
    printf("[ RUN\t\t] SET URL -> PLAY -> PAUSE\n");
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    mp_player_pause(player, NULL);
    sleep(1);
    printf("[ \t\tOK] SET -> PLAY -> PAUSE\n");
}
void set_url_2play_2stop_test()
{
    printf("[ RUN\t\t] SET URL -> PLAY -> STOP\n");
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    mp_player_stop(player, NULL);
    sleep(1);
    printf("[ \t\tOK] SET -> PLAY -> STOP\n");
}
void set_url_2play_2pause_2play_2finish_test()
{
    printf("[ RUN\t\t] SET URL -> PLAY -> PAUSE -> PLAY -> FINISH\n");
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    mp_player_pause(player, NULL);
    sleep(1);
    mp_player_play(player, NULL);
    sleep(interval);
    printf("[ \t\tOK] SET -> PLAY -> PAUSE -> PLAY -> FINISH\n");
}
void set_url_2play_2stop_2playagain_2finish_test()
{
    printf("[ RUN\t\t] SET URL -> PLAY -> STOP -> PLAY -> FINISH\n");
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    mp_player_stop(player, NULL);
    sleep(1);
    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    printf("[ \t\tOK] SET -> PLAY -> STOP -> PLAY -> FINISH\n");
}
void set_url_2play_stress_test()
{
    pthread_attr_t attr;
    pthread_t thread_recv;

    printf("[ RUN\t\t] SET URL -> PLAY -> STOP -> PLAY -> FINISH\n");

    pthread_attr_init (&attr);

    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create (&thread_recv, &attr, recv_msg, NULL);

    mp_player_set_resource(player, test_uri, NULL);
    mp_player_play(player, NULL);
    sleep(interval);
    mp_player_stop(player, NULL);
    sleep(interval);

    printf("[ \t\tOK] SET -> PLAY -> STOP -> PLAY -> FINISH\n");
}
void set_stream_2play_2finish_test()
{
    pthread_attr_t attr;
    pthread_t thread_recv;
    int i;

    printf("[ RUN\t\t] SET STREAM -> PLAY -> FINISH\n");

    init_fifo();

    pthread_attr_init (&attr);

    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create (&thread_recv, &attr, recv_msg, NULL);

    player_mrl_remove (player);

    mp_player_set_resource(player, TEST_FIFO, NULL);

    mp_player_play(player, NULL);
    if(0 != open_fifo()) {
        uninit_fifo();
        return;
    }
    irrigate_stream(0);

    for(i=0;i<20;i++)
    {
        mp_player_request_stream_read_count(player);
        sleep(1);
    }

    close_fifo();

  //  sleep(100);

    uninit_fifo();

    printf("[ \t\tOK] SET -> PLAY -> FINISH\n");
}
void set_stream_2play_2stop_2play_2finish_test()
{
    printf("[ RUN\t\t] SET STREAM -> PLAY -> STOP -> SET STREAM -> PLAY -> FINISH\n");

    /* first time : set - play - stop */
    init_fifo();

    mp_player_set_resource(player, TEST_FIFO, NULL);

    mp_player_play(player, NULL);

    if(0 != open_fifo()) {
        uninit_fifo();
        return;
    }
    irrigate_stream(0);

    close_fifo();

    sleep(interval);

    mp_player_stop(player, NULL);

    close_fifo();

    mp_player_set_resource(player, TEST_FIFO, NULL);

    if(0 != open_fifo()) {
        uninit_fifo();
        return;
    }
    mp_player_play(player, NULL);

    irrigate_stream(0);

    sleep(interval);

    close_fifo();
    uninit_fifo();
    printf("[ \t\tOK] SET -> PLAY -> STOP -> SET -> PLAY -> FINISH\n");
}
void usage_show(void)
{
    printf("\n usage:\n");
    printf(" l\tlist\n");
    printf(" a\trun all test case\n");
    printf(" c\tchoose test case\n");
    printf(" t\tset interval\n");
    printf(" h\thelp\n");
}
void testcase_show(void)
{
    printf("\n[ CASE ]\n");
    printf("[  1  ] SET URL\n");
    printf("[  2  ] SET URL -> PLAY\n");
    printf("[  3  ] SET URL -> PLAY -> PAUSE\n");
    printf("[  4  ] SET URL -> PLAY -> STOP\n");
    printf("[  5  ] SET URL -> PLAY -> FINISH\n");
    printf("[  6  ] SET URL -> PLAY -> PAUSE -> PLAY\n");
    printf("[  7  ] SET URL -> PLAY -> PAUSE -> PLAY -> FINISH\n");
    printf("[  8  ] SET URL -> PLAY -> STOP  -> PLAY\n");
    printf("[  9  ] SET URL -> PLAY -> STOP  -> PLAY -> FINISH\n");
}
void testcase_run(int num)
{
    printf("\nrun test case %d\n", num);

    /* start to install player*/
    player = mp_player_init();
    if(player == NULL) {
        printf("testcase_run fail\n");
        return;
    }

    if(num == 1) {
        set_url_2play_2finish_test();
    }
    else if(num == 2) {
        set_url_2play_2pause_test();
    }
    else if(num == 3) {
        set_url_2play_2stop_test();
    }
    else if(num == 4) {
        set_url_2play_2pause_2play_2finish_test();
    }
    else if(num == 5) {
        set_url_2play_2stop_2playagain_2finish_test();
    }
    else if(num == 6) {
        set_url_2play_stress_test();
    }
    else if(num == 10) {
        set_stream_2play_2finish_test();
    }
    else if(num == 11) {
        set_stream_2play_2stop_2play_2finish_test();
    }
    /* start to uninstall player*/
    mp_player_uninit(player);
}
void alltestcase_run(void)
{
    int n;
    for(n=1;n<TEST_CASE_NUM;n++) {
        testcase_run(n);
    }
}
int main(int argc, char **argv)
{
    char c;
    int n;

    if(argc != 2) {
        printf("./mp_player_test input_file\n");
        return 0;
    }

    test_uri = argv[1];
    while(1) {
        printf("Press\t'l' 'a' 'c' 'h' 't' : ");
        fflush (stdout);
        c = getchar();

        if(c == EOF) {
            break;
        }
        switch(c) {
            case 'l':
                testcase_show();
                break;
            case 'a':
                alltestcase_run();
                break;
            case 't':
                printf("\ninput command interval (secs) and press enter: ");
                scanf("%d", &n);
                interval = n;
                break;
            case 'c':
                printf("\ninput test case number and press enter: ");
                scanf("%d", &n);
                if(n > 0 && n <= TEST_CASE_NUM) {
                    testcase_run(n);
                }
                else
                {
                    usage_show();
                }
                break;
            case 'h':
                usage_show();
                break;
            default:
                usage_show();
                break;
        }
    }
    return 0;
}
