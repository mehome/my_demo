void lcd_show(char *mode,char *str)
{
    int   err = 0;
    char *args[4] = {0};
    char  args0[64] = "hotplug_call";
    char  args1[64] = {0};
    char  args2[64] = {0};
    args[0] = args0;
    args[1] = args1;
    args[2] = args2;
    if(str == NULL || mode == NULL)
    {
        dbgprintf("point  is null\n");
        return;
    }
    dbgprintf("lcd_show----%s- %s\n",mode,str);
    strcpy(args1,mode);
    strcpy(args2,str);
    err = socket_main(3,args);
    if(err != 0)
        dbgprintf("send message err302---%d\n",err);

}
/*
 * #0:disable;
 * #1:internet radio
 * #2:airplay/dlna
 * #3:localplay
 */
void dut_op_switch_mode(int mode )
{

    //int err = 0;
    switch( mode )
    {
    case 1:     /* 2 */
    {
        dbgprintf("Entering DLNA/AirPlayer---###\n");
        eval("/usr/bin/mpc", "stop", NULL);
        eval("/usr/bin/mpc", "clear", NULL);
        cdb_set("$ra_func", "2");
        eval("/bin/sh", "/lib/wdk/commit", NULL );
        lcd_show("s","2");

    }
    break;

    case 2:      /* 3 */
    {
        dbgprintf("Entering LocalPlayer---####\n." );
        cdb_set("$ra_func", "3");
        eval("/bin/sh", "/lib/wdk/commit", NULL );
        lcd_show("s","3");

    }
    break;

    case 3:     /* 1 */
    {
        dbgprintf("Entering smart  voice---!!\n");
        eval("/usr/bin/mpc", "stop", NULL);
        eval("/usr/bin/mpc", "clear", NULL);
        cdb_set("$ra_func", "5");
        lcd_show("s","5");

    }

    break;
    case 5:
    {
        dbgprintf("Entering Internet Radio---!!!\n." );
        cdb_set("$ra_func", "1");
        eval("/bin/sh", "/lib/wdk/commit", NULL );
        lcd_show("s","1");
    }
    break;
    default:
        break;
    }
//	strcpy(args1,"s");
//	err = socket_main(2,args);
//	if(err != 0)
//	dbgprintf("send message err351---%d\n.",err);
    return;
}

static void type_button_action_released(void) {
    char *button = getenv("BUTTON");
    char *seen = getenv("SEEN");
    char *hstapd = getenv("HOSTAPD");
    char *wpasup = getenv("WPASUP");
    int   is = 0, ik=0;

    ik = atoi( &button[4]);

    if( seen ) is = atoi(seen);

#ifdef HOTPLUG_DBG
    dbgprintf("released gpio button %d\n", ik );
#endif
    if( !ik && is >= 3 )
    {
        eval("/sbin/mtd", "erase", "mtd2", NULL);
        reboot(RB_AUTOBOOT);
    }
    else if( !ik && is <= 1 )
    {

        //op_switch_mode ...
    }
    else if( ik == 1 && !strcmp(hstapd, "1") )
    {
        eval("/usr/sbin/hostapd_cli", "wps_pbc", NULL);
    }
    else if( ik == 1 && !strcmp(wpasup, "1") )
    {
        eval("/usr/sbin/wpa_cli", "wps_pbc", NULL);
    }
    else if( ik == 1 && is >= 3 )
    {
        eval("bin/sh", "/lib/wdk/omnicfg_ctrl", "trigger", NULL);
    }
    else if( ik == 2 )
    {
        int mode = 0;    /*mode的取值不能放在文件锁之前*/
        mode = cdb_get_int("$ra_func");
        dut_op_switch_mode(mode);
    }
    return;
}
