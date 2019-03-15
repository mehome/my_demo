/*
 * =====================================================================================
 *
 *       Filename:  preinit.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/28/2016 11:00:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "include/mtdm_types.h"

#include <shutils.h>
#include <strutils.h>
#include <netutils.h>

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/* this is a raw syscall - man 2 pivot_root */
extern int pivot_root(const char *new_root, const char *put_old);

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/
int calc_tmpfs_size_k(void)
{
    // /proc/meminfo 
    //     MemTotal:          28004 kB
    FILE *fp;
    char buf[MSBUF_LEN];
    char *ptr;
    int limit = 5 * 1024 * 1024;
    int size = limit / 1024;

    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        while ((ptr = new_fgets(buf, sizeof(buf), fp)) != NULL) {
            if(!strncmp(ptr, "MemTotal:", sizeof("MemTotal:")-1)) {
                ptr += (sizeof("MemTotal:")-1);
                size = atoi(ptr);
                break;
            }
        }
        fclose(fp);
    }
    
    if (((size / 2) < limit) && (size > limit)) {
        size = size - limit;
    }
    else {
        size = size / 2;
    }

    return size;
}

void build_essential(void)
{
    char buf[MSBUF_LEN];

    mkdir("/dev", 0777);
    mkdir("/tmp", 0777);
    mkdir("/dev/shm", 0777);
    mkdir("/dev/pts", 0777);

    mount("proc", "/proc", "proc", MS_MGC_VAL, NULL);
    mount("sysfs", "/sys", "sysfs", MS_MGC_VAL, NULL);
    snprintf(buf, sizeof(buf), "mode=1777,size=%dk", ((calc_tmpfs_size_k()/1024)*1024));
    mount("tmpfs", "/tmp", "tmpfs", MS_NODEV | MS_NOSUID, buf);
    mount("devpts", "/dev/pts", "devpts", MS_MGC_VAL, NULL);
    mount("debugfs", "/sys/kernel/debug", "debugfs", MS_MGC_VAL, NULL);
}

typedef enum
{
    mtd_not_found   = -1,
    mtd_no          =  0,
    mtd_unformatted =  1,
    mtd_formatted   =  2,
}MTDState;

char *get_mtd_part(int part)
{
    static char mtd_block[USBUF_LEN];

    if (part >= 0) {
        if (f_isdir(mtd_block)) {
            sprintf(mtd_block, "/dev/mtdblock/");
        }
        else {
            sprintf(mtd_block, "/dev/mtdblock");
        }
        sprintf(mtd_block, "%s%d", mtd_block, part);
        if (f_exists(mtd_block)) {
            return mtd_block;
        }
    }

    return NULL;
}

int find_mtd_part(char *name)
{
    FILE *fp;
    char buf[MSBUF_LEN];
    char *ptr;
    int part = mtd_not_found;

    fp = fopen("/proc/mtd", "r");
    if (fp) {
        while ((ptr = new_fgets(buf, sizeof(buf), fp)) != NULL) {
            if(strstr(ptr, name)) {
                sscanf(ptr, "mtd%d:", &part);
                break;
            }
        }
        fclose(fp);
    }

    return part;
}

static int
mount_move(char *oldroot, char *newroot, char *dir)
{
    struct stat s;
    char olddir[64];
    char newdir[64];
    int ret;

    snprintf(olddir, sizeof(olddir), "%s%s", oldroot, dir);
    snprintf(newdir, sizeof(newdir), "%s%s", newroot, dir);

    if (stat(olddir, &s) || !S_ISDIR(s.st_mode)) {
        cprintf("%s:%d error for olddir:[%s]\n", __func__, __LINE__, olddir);
        return -1;
    }

    if (stat(newdir, &s) || !S_ISDIR(s.st_mode)) {
        cprintf("%s:%d error for newdir:[%s]\n", __func__, __LINE__, newdir);
        return -1;
    }

    ret = mount(olddir, newdir, NULL, MS_NOATIME | MS_MOVE, NULL);

    return ret;
}

int pivot(char *rw_root, char *ro_root)
{
    char pivotdir[64];
    int ret;

    /* mount -o move /proc "new"/proc */
    if (mount_move("", rw_root, "/proc")) {
        cprintf("mount -o move /proc %s/proc failed %s\n", rw_root, strerror(errno));
        return -1;
    }

    snprintf(pivotdir, sizeof(pivotdir), "%s%s", rw_root, ro_root);

    ret = pivot_root(rw_root, pivotdir);

    if (ret < 0) {
        cprintf("pivot_root failed %s %s: %s\n", rw_root, pivotdir, strerror(errno));
        return -1;
    }

    /* mount -o move "old"/dev /dev */
    mount_move(ro_root, "", "/dev");
    mount_move(ro_root, "", "/tmp");
    mount_move(ro_root, "", "/sys");
    mount_move(ro_root, "", "/overlay");

    return 0;
}

void fopivot(char *rw_root, char *ro_root)
{
    int has_mini_fo = 0;
    int mounted = 0;
    FILE *fp;
    char buf[MSBUF_LEN];
    char data[MSBUF_LEN];

    fp = fopen("/proc/filesystems", "r");
    if (fp) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if(strstr(buf, "mini_fo")) {
                has_mini_fo = 1;
            }
        }
        fclose(fp);
    }

    if (has_mini_fo) {
        snprintf(buf, sizeof(buf), "mini_fo:%s", rw_root);
        snprintf(data, sizeof(data), "base=/,sto=%s", rw_root);
        mounted = !mount(buf, "/mnt", "mini_fo", MS_RELATIME, data);
    }
    else {
        eval("mount", "--bind", "/", "/mnt");
        eval("mount", "--bind", "-o", "union", rw_root, "/mnt");
        mounted = 1;
    }

    if (mounted) {
        pivot("/mnt", ro_root);
    }
    else {
        cprintf("%s:%d failed, Internal Error!!\n", __func__, __LINE__);

        pivot(rw_root, ro_root);
    }
}

void build_mount_root(void)
{
    // /proc/mtd
    //     mtd4: 00e4ffd0 00010000 "rootfs"
    //     mtd5: 00a50000 00010000 "rootfs_data"
    int state = mtd_no;
    int part = find_mtd_part("rootfs_data");
    char *mtd_jffs2 = get_mtd_part(part);

    if (mtd_jffs2) {
// forcely use jffs2, assume it is formatted!
#if 1
        state = mtd_formatted;
#else
        unsigned int magic;
        FILE *fp;

        state = mtd_unformatted;
        
        fp = fopen(mtd_jffs2, "rb");
        if (fp) {
            fread(&magic, sizeof(unsigned int), 1, fp);
            if (magic != 0xdeadc0de) {
                state = mtd_formatted;
            }
            fclose(fp);
        }
#endif
    }

    switch (state) {
        case mtd_no:
            cprintf("mount_no_mtd\n");
            mkdir("/tmp/etc", 0777);
            mount("mini_fo:/tmp/etc", "/etc", "mini_fo", MS_RELATIME, "base=/etc,sto=/tmp/etc");
            break;
        case mtd_formatted:
            cprintf("mount_jffs2\n");
            mkdir("/tmp/overlay", 0777);
            if (!mount(mtd_jffs2, "/tmp/overlay", "jffs2", MS_RELATIME, NULL)) {
                cprintf("switching to jffs2\n");
                mount_move("/tmp", "", "/overlay");
                fopivot("/overlay", "/rom");
                break;
            }
        case mtd_unformatted:
            cprintf("mount_no_jffs2, using ramdisk\n");
            mkdir("/tmp/root", 0777);
            mount("root", "/tmp/root", "tmpfs", MS_RELATIME, NULL);
            fopivot("/tmp/root", "/rom");
            break;
    }
}

int main(int argc, char *argv[])
{
    char *new_argv[] = { "/sbin/init", NULL };
    struct timeval sAlltv;

    gettimeofbegin(&sAlltv);
    cprintf("\n");
    cprintf("preinit begin: %ld\n", time(NULL));

    build_essential();
    build_mount_root();

    cprintf("preinit end: %ld used time: %ld\n", time(NULL), gettimevaldiff(&sAlltv));

    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
    execvp(new_argv[0], new_argv);

    return 0;
}


