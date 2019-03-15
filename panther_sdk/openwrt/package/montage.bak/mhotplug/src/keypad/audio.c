int get_play_status()
{
    FILE *fp;
    char  buf[128], *line;
    int   ret = 0;

    fp = popen("mpc status", "r");
    if (fp == NULL) {
        perror("popen");
        return 0;
    }

    while ( (line = fgets(buf, sizeof(buf), fp)) != NULL ) {

        if ( strstr(line, "[playing]") != NULL )
        {
            ret = 1;
            break;
        }
    }
    line = NULL;
    pclose(fp);

    return ret;
}

int get_pause_status()
{
    FILE *fp;
    char  buf[128], *line;
    int   ret = 0;

    fp = popen("mpc status", "r");
    if (fp == NULL) {
        perror("popen");
        return 0;
    }

    while ( (line = fgets(buf, sizeof(buf), fp)) != NULL ) {

        if ( strstr(line, "[paused]") != NULL )
        {
            ret = 1;
            break;
        }
    }
    line = NULL;
    pclose(fp);

    return ret;
}

int get_playlist_count()
{
    char buf[16];
    int  n = 0;

    FILE *fp = popen("mpc playlist | wc -l", "r");
    fgets(buf, sizeof(buf), fp);
    n = atoi(buf);
    pclose(fp);

    return n;
}

void mpc_create_playlist()
{
    eval("mpc", "update", "1", NULL);
    sleep(1);
    return;
}
static int get_op_work_mode(void)
{
    int work_mode;
    work_mode = cdb_get_int("$op_work_mode");
    return work_mode;
}
unsigned char get_baidu_status(void)
{
    FILE * fp;
    char   status[512];
    memset(status,0,sizeof(status));
    fp = popen("pidof baiduasr","r");
    fgets(status,sizeof(status),fp);
    pclose(fp);
    if(strlen(status))
    {
        dbgprintf("baidu pid ---%s\n",status);
        return 1;
    }
    return 0;

}
unsigned char get_wifi_status(void)
{
    unsigned char wifi_status;
    int           work_mode;
    char          status[100];
    FILE *        fp;

    work_mode = get_op_work_mode();
    if(work_mode == AP_MODE)
        wifi_status = WIFI_NOT_CONNECT;
    else if (work_mode == WISP_MODE || work_mode == STA_MODE)
    {
        fp = popen("ifconfig br1 | grep -e \"inet addr:\"","r");
        fgets(status,sizeof(status),fp);
        pclose(fp);

        printf("status:%s\n",status);

        if(strstr(status,"inet addr:") && strstr(status,"Bcast:") && strstr(status,"Mask:"))
            wifi_status = WIFI_CONNECT;
        else
            wifi_status = WIFI_NOT_CONNECT;
    }
    else
    {
        printf("work mode error\n");
        wifi_status = WIFI_NOT_CONNECT;
    }

    return wifi_status;
}

static void type_keypad(void)
{
    char *args[4] = {0};
    char  args0[64] = "hotplug_call";
    char  args1[64] = {0};
    char  args2[64] = {0};
    int   err = 0,try_time =0, ik = 0, ic = 0;
    char *key = getenv("KEY");
    char *chan = getenv("CHAN");
    args[0] = args0;
    args[1] = args1;
    args[2] = args2;
    if( key ) ik = atoi(key);
    if( chan ) ic = atoi(chan);

#ifdef HOTPLUG_DBG
    dbgprintf("released madc button=%d, CHAN=%d\n", ik, ic );
#endif

    if( ik == 1 && ic == 1 )
    {         //VOL +
        char vol[5];
        eval("/usr/bin/mpc", "volume", "+5", NULL);
        cdb_get("$ra_vol", vol);
        strcpy(args1,"v");
        strcpy(args2,vol);
        err = socket_main(3,args);
        if(err != 0)
            dbgprintf("send message err---%d\n",err);
    }
    else if( ik == 2 && ic == 1 )
    {         //VOL -
        char vol[5];
        eval("/usr/bin/mpc", "volume", "-5", NULL);
        cdb_get("$ra_vol", vol);
        strcpy(args1,"v");
        strcpy(args2,vol);
        err = socket_main(3,args);
        if(err != 0)
            dbgprintf("send message err---%d\n",err);
    }
    else if( ik == 3 && ic == 1 )
    {
        eval("/usr/bin/mpc", "seek", "+10%", NULL);
    }
    else if( ik == 4 && ic == 1 )
    {
        //eval("/usr/bin/mpc", "seek", "-10%", NULL);
        if(!get_wifi_status())
        {
            char *args[4] = {0};
            char  args0[64] = "hotplug_call";
            char  args1[64] = {0};
            char  args2[64] = {0};
            args[0] = args0;
            args[1] = args1;
            args[2] = args2;
            strcpy(args1,"n");
            err = socket_main(2,args);
            if(err != 0)
                dbgprintf("send message err302---%d\n",err);
            return;
        }
        else
        {
            if(get_baidu_status())
                system("killall baiduasr");
            else
            {
                eval("/usr/bin/mpc", "stop", NULL);
                eval("/usr/bin/mpc", "clear", NULL);
                eval("/bin/baiduasr",NULL );
                usleep(50);
                char *args[4] = {0};
                char  args0[64] = "hotplug_call";
                char  args1[64] = {0};
                char  args2[64] = {0};
                args[0] = args0;
                args[1] = args1;
                args[2] = args2;
                strcpy(args1,"s");
                strcpy(args2,"5");
                dbgprintf("Entering smart  voice---!!!%s\n",args2);
                cdb_set("$ra_func", "5");
                err = socket_main(3,args);
                if(err != 0)
                    dbgprintf("send message err302---%d\n",err);
            }
        }
    }
    else if( ik == 1 && ic == 0 )
    {          //FORWAORD
        int mode = 0;
        mode = cdb_get_int("$ra_func");
        if( !get_play_status()  &&  !get_pause_status())
            return;
        if(mode == 3)
        {
            eval("/usr/bin/mpc", "next", NULL);
        }
        else if (mode == 1)
        {
            eval("/lib/wdk/rakey", "forward", NULL);
        }
    }
    else if( ik == 2 && ic == 0 )
    {          //BACKWORD
        int mode = 0;
        mode = cdb_get_int("$ra_func");
        dbgprintf(" play mode = %d\n",mode);
        if( !get_play_status()  &&  !get_pause_status())
            return;
        if(mode == 3)
        {
            eval("/usr/bin/mpc", "repeat", "on",  NULL);
            eval("/usr/bin/mpc", "prev", NULL);
        }
        else if (mode == 1)
        {
            eval("/lib/wdk/rakey", "backward", NULL);
        }

    }
    else if( ik == 3 && ic == 0 )
    {          //PLAY/PAUSE
        int mode = 0;
        mode = cdb_get_int("$ra_func");
        dbgprintf(" play mode = %d\n",mode);
        if( mode !=3 && mode != 1)
        {
            dbgprintf("not audio play mode = %d\n",mode);
            return;
        }
        dbgprintf("wifistatus = %d\n",get_wifi_status());
        if(mode == 1)
        {
            if(!get_wifi_status())
            {
                char *args[4] = {0};
                char  args0[64] = "hotplug_call";
                char  args1[64] = {0};
                char  args2[64] = {0};
                args[0] = args0;
                args[1] = args1;
                args[2] = args2;
                strcpy(args1,"n");
                err = socket_main(2,args);
                if(err != 0)
                    dbgprintf("send message err302---%d\n",err);
                return;
            }
            if(  !get_play_status()  &&  !get_pause_status() )
            {
                eval("/lib/wdk/rakey", "1", NULL);
            }
            else
            {
                dbgprintf("keypad 3---1111\n");
                eval("/usr/bin/mpc", "toggle", NULL);
            }
        }
        else if(mode == 3)
        {
            if( (    !get_play_status() || !get_playlist_count() ) &&  !get_pause_status())
            {
#if 1
                try_time = 3;
                while(try_time--)
                {
                    if( !get_playlist_count())
                        mpc_create_playlist();
                    dbgprintf("keypad 3---2\n");
                    eval("/usr/bin/mpc", "play", NULL);
                    //eval("/usr/bin/mpc", "repeat", "on",  NULL);
                    usleep(50*1000);
                    if(get_play_status())
                    {
                        return;
                    }
                }
#endif

            }
            else
            {
                dbgprintf("keypad 3---3\n");
                eval("/usr/bin/mpc", "toggle", NULL);
            }
        }
    }
    else if( ik == 4 && ic == 0 )
    {
        char *args[4] = {0};
        char  args0[64] = "hotplug_call";
        char  args1[64] = {0};
        char  args2[64] = {0};
        int   mode = 0;
        mode = cdb_get_int("$ra_func");
        args[0] = args0;
        args[1] = args1;
        args[2] = args2;
        strcpy(args1,"m");
        sprintf(args2,"%d",mode);
        //eval("/usr/bin/st7565p_lcd_cli", "m", "2" , NULL);
        err = socket_main(3,args);
        if(err != 0)
            dbgprintf("send message err302---%d\n.",err);
        sleep(1);
        if(get_play_status())
        {
            eval("/usr/bin/mpc", "play", NULL);
        }
        else
        {
            strcpy(args1,"s");
            dbgprintf("send message ---line =%d\n.",__LINE__);
            err = socket_main(2,args);
            if(err != 0)
                dbgprintf("send message err302---%d\n.",err);
            dbgprintf("send message ---line =%d\n.",__LINE__);
        }
    }
}
