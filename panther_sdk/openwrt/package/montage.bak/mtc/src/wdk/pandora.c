
#include "wdk.h"

#define PANDORA_BIN                 "/usr/bin/pianobar"
#define PANDORA_CONFIG_FILE         "/root/.config/pianobar/config"
#define PANDORA_STATE_FILE          "/root/.config/pianobar/state"
#define PANDORA_LOGIN_FILE          "/tmp/pianlogin"
#define PANDORA_PLAY_SONG_FILE      "/tmp/piansong"
#define PANDORA_STATION_LIST_FILE   "/tmp/pianstat"
#define PANDORA_STATION_INDEX_FILE  "/tmp/pianstat_idx"

static void pandora_config()
{
    char buf[128] = {0};
    FILE *fp = NULL;

    if((fp = fopen(PANDORA_CONFIG_FILE, "w")) != NULL) {
        fprintf(fp, "execute_command = /usr/bin/mpc\n");
        fprintf(fp, "event_command2 = /usr/bin/pian_ev\n");

        memset(buf, 0, sizeof(buf));
        cdb_get("$pandora_user", buf);
        fprintf(fp, "user = %s\n", buf);

        memset(buf, 0, sizeof(buf));
        cdb_get("$pandora_password", buf);
        fprintf(fp, "password = %s\n", buf);

        memset(buf, 0, sizeof(buf));
        cdb_get("$pandora_proxy", buf);
        LOG("proxy %s\n", buf);
        if (buf != NULL)
            fprintf(fp, "proxy = http://%s\n", buf);

        memset(buf, 0, sizeof(buf));
        cdb_get("$pandora_license", buf);
        fprintf(fp, "tls_fingerprint = %s\n", buf);

        fclose(fp);
    }
}

static void pandora_start()
{
    remove(PANDORA_STATE_FILE);
    //exec_cmd(PANDORA_BIN);
    system(PANDORA_BIN);
}

static void pandora_shutdown()
{
    remove(PANDORA_LOGIN_FILE);
    remove(PANDORA_PLAY_SONG_FILE);
    remove(PANDORA_STATION_LIST_FILE);
    remove(PANDORA_STATION_INDEX_FILE);
    exec_cmd("pian_cli q");
    sleep(1);
    killall("pianobar", SIGTERM);
}

static void pandora_play(char *arg)
{
    remove(PANDORA_STATION_LIST_FILE);
    exec_cmd("pian_cli s");
    sleep(1);
    exec_cmd2("pian_cli %s", arg);

    writeline(PANDORA_STATION_INDEX_FILE, arg);
}

static void pandora_next()
{
    exec_cmd("pian_cli n");
}

static void pandora_stop()
{
    exec_cmd("pian_cli S");
}

static void pandora_volume(char *arg)
{
    int vol = cdb_get_int("$ra_vol", 0);

    if (!strncmp(arg, "up", 2)) {
        vol += 5;
        if (vol > 100) vol = 100;
    } else if (!strncmp(arg, "down", 4)) {
        vol -= 5;
        if (vol < 0) vol = 0;
    }
    exec_cmd2("mpc volume %d", vol);
    exec_cmd("cdb commit");
}

static void pandora_get_login_status()
{
    int wanif_state = cdb_get_int("$wanif_state", 0);

    //writeline("/tmp/mpd_save", "1");

    if (wanif_state != 2)
        remove(PANDORA_LOGIN_FILE);

    if (f_exists(PANDORA_LOGIN_FILE)) {
        char buffer[8] = {0};
        readline(PANDORA_LOGIN_FILE, buffer, sizeof(buffer));
        STDOUT(buffer);
    } else
        STDOUT("0");
}

static void pandora_get_play_song()
{
    //writeline("/tmp/mpd_save", "1");

    if (f_exists(PANDORA_PLAY_SONG_FILE)) {
        char buffer[512] = {0};
        readline(PANDORA_PLAY_SONG_FILE, buffer, sizeof(buffer));
        STDOUT(buffer);
    } else
        STDOUT("None");
}

static int get_station_list_size()
{
    FILE *fp = fopen(PANDORA_STATION_LIST_FILE, "r");
    char buf[256] = {0};
    int number_of_lines = 0;

    if (fp) {
        while (fgets(buf, sizeof(buf), fp) != NULL )
           number_of_lines++;

        fclose(fp);
    }

    return number_of_lines;
}

static void pandora_get_station_list()
{
    int list_size = get_station_list_size();
    FILE *fp = fopen(PANDORA_STATION_LIST_FILE, "r");
    FILE *fp_tmp = fopen("/tmp/pdlist", "a");
    char buf[256] = {0};
    int list_cnt = 1;

    if (fp && fp_tmp) {
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            LOG("buf %s size %d cnt %d", buf, list_size, list_cnt);
            if (buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if (list_cnt == list_size)
                fprintf(fp_tmp, "%s", buf);
            else
                fprintf(fp_tmp, "%s;", buf);
            list_cnt++;
        }
    }

    if (fp)
        fclose(fp);
    if (fp_tmp)
        fclose(fp_tmp);

    system("cat /tmp/pdlist");

    remove("/tmp/pdlist");
}

static void pandora_get_station_index()
{
    if (f_exists(PANDORA_STATION_INDEX_FILE)) {
        char buffer[8] = {0};
        readline(PANDORA_STATION_INDEX_FILE, buffer, sizeof(buffer));
        STDOUT(buffer);
    } else
        STDOUT("0");
}

int wdk_pandora(int argc, char **argv)
{    if (argc == 0)
        return -1;

    LOG("wdk_pandora %s\n", argv[0]);

    if (strcmp(argv[0], "conf") == 0) {
        pandora_config();
    } else if (strcmp(argv[0], "start") == 0) {
        pandora_start();
    } else if (strcmp(argv[0], "shutdown") == 0) {
        pandora_shutdown();
    } else if (strcmp(argv[0], "play") == 0) {
        if (argc == 2)
            pandora_play(argv[1]);
    } else if (strcmp(argv[0], "next") == 0) {
        pandora_next();
    } else if (strcmp(argv[0], "stop") == 0) {
        pandora_stop();
    } else if (strcmp(argv[0], "volume") == 0) {
         if (argc == 2)
            pandora_volume(argv[1]);
    } else if (strcmp(argv[0], "get_login_status") == 0) {
        pandora_get_login_status();
    } else if (strcmp(argv[0], "get_play_song") == 0) {
        pandora_get_play_song();
    } else if (strcmp(argv[0], "get_stations") == 0) {
        pandora_get_station_list();
    } else if (strcmp(argv[0], "get_station_idx") == 0) {
        pandora_get_station_index();
    } else {
        return -1;
    }

    return 0;
}


