/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 *
 * Alibaba Group retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have
 * obtained a separate written license from Alibaba Group., you are not
 * authorized to utilize all or a part of this computer program for any
 * purpose (including reproduction, distribution, modification, and
 * compilation into object code), and you must immediately destroy or
 * return to Alibaba Group all copies of this computer program.  If you
 * are licensed by Alibaba Group, your rights to utilize this computer
 * program are limited by the terms of that license.  To obtain a license,
 * please contact Alibaba Group.
 *
 * This computer program contains trade secrets owned by Alibaba Group.
 * and, unless unauthorized by Alibaba Group in writing, you agree to
 * maintain the confidentiality of this computer program and related
 * information and to not disclose this computer program and related
 * information to any other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND
 * Alibaba Group EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h> 
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <string.h>

#include "platform.h"
#include "montage_api.h"

static FILE *fp;

#define otafilename "/tmp/alinkota.bin"

#define CDB_PROCFS_FILENAME     "/proc/cdb"	
#define MAX_CDB_NAME_LENGTH          32
#define MAX_CDB_CMD_STRING_LENGTH    1280
#define CDB_NAME_SIZE MAX_CDB_NAME_LENGTH
#define CDB_CMD_SIZE MAX_CDB_CMD_STRING_LENGTH
#define MAX_COMMAND_LEN		1024
#define MAX_ARGVS    20

extern int sys_verify_img(char *ifile);
extern int mtd_write(const char *path, const char *mtd, int quiet);

#if 1

int writeline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "w");
    if (fp == NULL) {
        printf("open file %s failed", path);
        return -1;
    }
    if((fputs(buffer, fp)) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        printf("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}


int str2args (const char *str, char *argv[], char *delim, int max)
{
    char *p;
    int n;
    p = (char *) str;
    for (n=0; n < max; n++)
    {
        if (0==(argv[n]=strtok_r(p, delim, &p)))
            break;
    }
    return n;
}

int str2argv(char *buf, char **v_list, char c)
{
    int n;
    char del[4];

    del[0] = c;
    del[1] = 0;
    n=str2args(buf, v_list, del, MAX_ARGVS);
    v_list[n] = 0;
    return n;
}

int exec_cmd2(const char *cmd, ...)
{
	char buf[MAX_COMMAND_LEN];
	va_list args;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	return exec_cmd(buf);
}

static void free_memory(int all)
{
    char alive_list[200] = { 0 };
    char *WDKUPGRADE_KEEP_ALIVE = NULL;

    if (all) {
        strcat(alive_list, "init\\|ash\\|watchdog\\|ota\\|telnetd\\|ps\\|$$");
    }
    else {
        strcat(alive_list, "init\\|uhttpd\\|omnicfg_bc\\|hostapd\\|ash\\|watchdog\\|alink\\|telnetd\\|http\\|ps\\|$$");
    }

    if ((WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE")) != NULL) {
        char *argv[10];
        int num = str2argv(WDKUPGRADE_KEEP_ALIVE, argv, ' ');
        while (num-- > 0) {
            strcat(alive_list, "\\|");
            strcat(alive_list, argv[num]);
        }
    }

    printf("Free memory now, all: [%d]", all);
    printf("Kill programs to free memory without: [%s]", alive_list);
    exec_cmd2("ps | grep -v '%s' | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9 > /dev/null 2>&1", alive_list);

    writeline("/proc/sys/vm/drop_caches", "3");
    sleep(1);
    writeline("/proc/sys/vm/drop_caches", "2");
    sync();
}
#endif

void platform_flash_program_start(void)
{
    printf("--%d--%s--!----\n", __LINE__, __FUNCTION__);
    play_notice_voice(m_upgrade_start);
    sleep(3);
    free_memory(0);
    fp = fopen(otafilename, "a+");
    assert(fp);
    return;
}

int platform_flash_program_write_block(_IN_ char *buffer, _IN_ uint32_t length)
{
    unsigned int written_len = 0;
    written_len = fwrite(buffer, 1, length, fp);

    if (written_len != length)
        return -1;
    return 0;
}

int platform_flash_program_stop(void)
{
    if (fp != NULL) {
        fclose(fp);
    }
    if( sys_verify_img(otafilename)) {
        fprintf(stderr, "Failed.\n");
        play_notice_voice(m_upgrade_fail);
        system("reboot");
        return 0;
    }else{       
        fprintf(stderr, "OK.\n");
        mtd_write(otafilename, "firmware", 0 );   //write to firmware partition
        sync();
        sleep(3);
    }
    
    play_notice_voice(m_upgrade_success);
    sleep(3);
    system("reboot");
    /* check file md5, and burning it to flash ... finally reboot system */

    return 0;
}
