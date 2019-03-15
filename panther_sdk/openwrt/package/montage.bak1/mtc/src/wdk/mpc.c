


#include "wdk.h"


enum mpd_state {
    MPD_STATE_UNKNOWN = 0,
    MPD_STATE_STOP = 1,
    MPD_STATE_PLAY = 2,
    MPD_STATE_PAUSE = 3,
};


static void collect_arguments(int argc, char **argv, char* cmd_buffer)
{
    int i = 0;
    for (i = 0; i < argc; i++) {
        strcat(cmd_buffer, argv[i]);
        if (i != argc-1)
            strcat(cmd_buffer, " ");
    }
}

static void collect_arguments_space(int argc, char **argv, char* cmd_buffer)
{
    int i = 0;
    strcat(cmd_buffer, "add ");
    strcat(cmd_buffer, "\"");


    for (i = 1; i < argc; i++) {
        strcat(cmd_buffer, argv[i]);
        if (i < argc -1)
            strcat(cmd_buffer, " ");
    }

    strcat(cmd_buffer, "\"");

	//LOG("cmd buffer = %s", cmd_buffer);
}


static int mpd_get_status()
{
    char mpd_result[300];
    exec_cmd3(mpd_result, sizeof(mpd_result), "mpc");
    if (strstr(mpd_result, "playing") != NULL)
        return MPD_STATE_PLAY;
    else if  (strstr(mpd_result, "paused") != NULL)
        return MPD_STATE_PAUSE;
    else
        return MPD_STATE_STOP;
}

#include <dirent.h>
#include <stdio.h>

#define PLAYLIST_DIR "/playlists"
#define RADIO_LIST_FILE "/etc/ralist"

static int get_ralist_size()
{
    FILE* fp = fopen(RADIO_LIST_FILE, "r");
    char buf[256] = {0};
    int number_of_lines = 0;

    if (fp) {
        while (fgets(buf, sizeof(buf), fp) != NULL )
           number_of_lines++;

        fclose(fp);
    }

    return number_of_lines;
}
static void rs_load_playlist()
{
    FILE *fp = fopen(RADIO_LIST_FILE, "r");
    FILE *fp_tmp = fopen("/tmp/ralist", "a");
    char buf[256] = {0};
    int list_size = get_ralist_size();
    int list_cnt = 1;

    if (fp && fp_tmp) {
        while (fgets(buf, sizeof(buf), fp) != NULL ) {
            //LOG("buf %s size %d cnt %d", buf, list_size, list_cnt);
            if (buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if (list_cnt == list_size)
                fprintf(fp_tmp, "%s", buf);
            else
                fprintf(fp_tmp, "%s;", buf);
            list_cnt++;
        }

        fclose(fp);
        fclose(fp_tmp);
    }
    system("cat /tmp/ralist");

    remove("/tmp/ralist");
}

static void rs_save(char *argv)
{
    char httpd_result[1024], m3u_fname[256];
    char NAME[256], URI[512], IDX[8];
    char *tok;
    FILE *fp_ra = NULL, *fp_m3u = NULL;

    exec_cmd3(httpd_result, sizeof(httpd_result), "uhttpd -d \"%s\"", argv);
    //LOG("req %s\n uhttpd result %s", argv, httpd_result);

    if ((tok = strtok(httpd_result, "&")) != NULL) {
        LOG("name %s", tok);
        strncpy(NAME, tok, sizeof(NAME));
    } else
        return;

    if ((tok = strtok(NULL, "&")) != NULL) {
        LOG("uri %s", tok);
        strncpy(URI, tok, sizeof(URI));
    } else
        return;

    if ((tok = strtok(NULL, " \n")) != NULL) {
        LOG("idx %s", tok);
        strncpy(IDX, tok, sizeof(IDX));
    } else
        return;
    /*
        save to /playlists/ folder
    */
    sprintf(m3u_fname, "%s/%s-%s.m3u", PLAYLIST_DIR, IDX, NAME);
    if ((fp_m3u = fopen(m3u_fname, "w")) != NULL) {
        fprintf(fp_m3u, "%s", URI);
        fclose(fp_m3u);
    }

    /*
        save to /etc/ralist
    */
    if ((fp_ra = fopen(RADIO_LIST_FILE, "a")) != NULL) {
        fprintf(fp_ra, "%s&%s&%s\n", NAME, URI, IDX);
        fclose(fp_ra);
    }
}

static void rs_delete()
{
    DIR *d;
    struct dirent *dir;
    char buf[512] = {0};

    d = opendir(PLAYLIST_DIR);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.')
                continue;
            if (strcmp(dir->d_name, "audioPlayList")) {
                sprintf(buf, "%s/%s", PLAYLIST_DIR, dir->d_name);
                //LOG("%s\n", buf);
                remove(buf);
            }
        }
        closedir(d);
    }

    remove(RADIO_LIST_FILE);
    return;
}

static void rs_play(char *argv)
{
    int ra_func = cdb_get_int("$ra_func", 0);
    int ra_cur = cdb_get_int("$ra_cur", 0);

    char buf[1024], rs_full_name[270];
    char rs_name[256], *ptr;
    char rs_idx[8] = {0};
    DIR *d;
    struct dirent *dir;
    int i = 0;
    FILE *fp;
    int direct_load = 0;

    if ((ra_func != 1) && (ra_func != 4))
        return;

    exec_cmd3(buf, sizeof(buf), "uhttpd -d \"%s\"", argv);
    strncpy(rs_name, buf, sizeof(rs_name));
    //LOG("req %s\n uhttpd result %s", argv, rs_name);

    d = opendir(PLAYLIST_DIR);

    /*
        get radio staion index..
    */
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.')
                continue;
            //LOG("search %s target %s\n", dir->d_name, rs_name);
            if (strstr(dir->d_name, rs_name)) {
                sprintf(rs_full_name, "%s/%s", PLAYLIST_DIR, dir->d_name);
                ptr = dir->d_name;
                while (*ptr != '-' && i < 8) {
                    rs_idx[i] = *ptr;
                    ptr++;
                    i++;
                }
                rs_idx[i] = '\0';
                break;
            }
        }
        closedir(d);
    } else
        return;

    LOG("found file %s idx %s\n", rs_full_name, rs_idx);

    if (atoi(rs_idx) == ra_cur && mpd_get_status() == MPD_STATE_PLAY)
        return;

    exec_cmd("mpc pause > /dev/null");
    exec_cmd("mpc clear > /dev/null");

    /*
        check if need to use content directly
    */
    if((fp = fopen(rs_full_name, "r")) != NULL) {
        if (fgets(buf, sizeof(buf), fp) != NULL ) {
            if (strstr(buf, ".pls") ||
                strstr(buf, ".m3u"))
                direct_load = 1;
        }
    } else
        return;

    //LOG("is dir load %d\n", direct_load);
    if (direct_load) {
        exec_cmd2("mpc load \"%s\"", buf);
    } else {
        exec_cmd2("mpc load \"%s-%s\"", rs_idx, rs_name);
        //LOG("mpc load \"%s-%s\"", rs_idx, rs_name);
    }

    exec_cmd("mpc play");

    cdb_set("$mpd_curplay", rs_name);
    cdb_set_int("$ra_cur", atoi(rs_idx));
    cdb_set_int("$mpd_curfunc", 1);
    exec_cmd("cdb commit");

    LOG("play %s", argv);
}


/*
	Caller audio.htm:

	/lib/wdk/update  -> mpc update
	/lib/wdk/mpc volume -> mpc volume
	/lib/wdk/mpc load_playlist -> cat /playlists/audioPlayList.m3u ( it exists when /lib/wdk/mpc save audioPlayList)
	/lib/wdk/mpc playlistfile -> mpc -f %file% playlist
	/lib/wdk/mpc currentfile -> mpc -f %file% current
	/lib/wdk/play_status -> mpc play_status -> not support
	/lib/wdk/play_status -> mpc play_status -> not support
	/lib/wdk/mpc listall -> mpc listall
	/lib/wdk/mpc stop -> mpc stop
	/lib/wdk/mpc play -> mpc stop
	/lib/wdk/mpc clear -> mpc clear
	/lib/wdk/mpc rm audioPlayList  -> mpc rm audioPlayList
	/lib/wdk/mpc save audioPlayList  -> mpc save audioPlayList
	/lib/wdk/mpc add sda1/123.wav  -> mpc add sda1/123.wav
	/lib/wdk/mpc play 1 -> mpc play 1


*/
int wdk_mpc(int argc, char **argv)
{
    char cmd_buffer[1000] = {0};

	if (argc == 0) {
		return -1;
	}

    if (str_equal(argv[0] ,"rs")) {
        if (argc >= 2) {
            if (str_equal(argv[1] ,"play")) {
                if (argc >= 3) {
                    collect_arguments(argc, argv, cmd_buffer);
                    rs_play(cmd_buffer + 8);
                }
            }
            if (str_equal(argv[1] ,"stop")) {
                system("mpc pause > dev/null");
                cdb_set("$mpd_curplay", "");
            }
            if (str_equal(argv[1] ,"load_playlist")) {
                rs_load_playlist();
            }
            if (str_equal(argv[1] ,"save")) {
                if (argc >= 3) {
                    collect_arguments(argc, argv, cmd_buffer);
                    rs_save(cmd_buffer + 8);
                }
            }
            if (str_equal(argv[1] ,"del")) {
                rs_delete();
            }
        }
    } else if (str_equal(argv[0] ,"load_playlist")) {
        if (f_exists("/playlists/audioPlayList.m3u")) {
            system("cat /playlists/audioPlayList.m3u");
        }
    } else if (str_equal(argv[0] ,"playlistfile")) {
        system("mpc -f %file% playlist"); /* add file path prefix*/
    } else if (str_equal(argv[0] ,"currentfile")) {
        system("mpc -f %file% current");/* add file path prefix*/
    } else if (str_equal(argv[0] ,"play_status")) {
        STDOUT("%d", mpd_get_status());
    } else {
        strcpy(cmd_buffer, "mpc ");
        if (str_equal(argv[0] ,"add"))
            collect_arguments_space(argc, argv, cmd_buffer);
        else
            collect_arguments(argc, argv, cmd_buffer);

        print_shell_result(cmd_buffer);
    }

    return 0;
}

















