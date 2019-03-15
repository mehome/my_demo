#define _GNU_SOURCE
#include "wdk.h"

#define SYSUPGRADE_FLASH_BIN "/sbin/sysupgrade"
#define SYSUPGRADE_RAM_BIN "/tmp/sysupgrade"

#define WDKUPGRADE_FLASH_BIN "/lib/wdk/sysupgrade"
#define WDKUPGRADE_RAM_BIN "/tmp/wdkupgrade"

static int wdkupgrade_result = RET_ERR;

static void free_memory(int all)
{
    char alive_list[200] = { 0 };
    char *WDKUPGRADE_KEEP_ALIVE = NULL;

    if (all) {
        strcat(alive_list, "init\\|ash\\|watchdog\\|wdkupgrade\\|ps\\|$$");
    }
    else {
        strcat(alive_list, "init\\|uhttpd\\|hostapd\\|ash\\|watchdog\\|wdkupgrade\\|http\\|omnicfg_bc\\|ota\\|telnetd\\|WIFIAudio_DownloadBurn\\|posthtmlupdate.cgi\\|httpapi.cgi\\|ps\\|$$");
    }

    if ((WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE")) != NULL) {
        char *argv[10];
        int num = str2argv(WDKUPGRADE_KEEP_ALIVE, argv, ' ');
        while (num-- > 0) {
            strcat(alive_list, "\\|");
            strcat(alive_list, argv[num]);
        }
    }

    LOG("Free memory now, all: [%d]", all);
    LOG("Kill programs to free memory without: [%s]", alive_list);
    exec_cmd2("ps | grep -v '%s' | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9 > /dev/null 2>&1", alive_list);

    writeline("/proc/sys/vm/drop_caches", "3");
    sync();
}

/*
    Get the flash firmware size of current board
*/
static int get_flash_size()
{
    FILE *fp;
    char buf[512];
    int size = 0;

    fp = fopen("/proc/flash", "r");
    if (fp) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if (sscanf(buf, "%d firmware", &size) != -1) {
                break;
            }
        }
        fclose(fp);
    }

    LOG("flash firmware size %d", size);

    return size;
}

/*
    Get the flash image to be uploaded
*/
static int cal_image_size()
{
    int size = f_size("/tmp/firmware");

    LOG("flash firmware size %d", size);

    return size;
}

/*
    calculate the backup playlist file size(sum up)
*/
static int cal_backup_size()
{
    //TODO
#if 0
    local sizes=`cat $ {backup_list} 2> /dev/null | while read file;
do
    [ -d $file ] && {
            du -s $ {file} | awk '{print $1*1024}'
        } || {
        wc -c ${file} | awk '{print $1}'
    }
    done`
    echo "${sizes}" | awk '{sum += $1} END{print sum}'
#endif

    return 0;
}

/*
    Check the firmware versions of ([cdb version] and [firmware.img version]) are same.

    same        => return 1
    not same    => return 0
*/
static int check_firmware_version()
{
    char sw_ver[100] = { 0 };
    char fw_ver[100] = { 0 };
    int ret = RET_OK;

    cdb_get("$sw_ver", sw_ver);

    exec_cmd3(fw_ver, sizeof(fw_ver), "hexdump -s 24 -n 4 -e '\"%%d\"' %s", "/tmp/firmware" );
    if (atoi(fw_ver) == atoi(sw_ver)) {
        LOG("firmware versions are same %s, quit update", fw_ver);
        ret = RET_ERR;
    } else {
        LOG("firmware versions not same %s != %s, meet the need", fw_ver, sw_ver);
    }

    return ret;
}

static int check_flash_size()
{
    int flashSize = get_flash_size();
    int imageSize = cal_image_size();
    int backupSize = cal_backup_size();
    int leftSize = 0;
    int ret = RET_OK;

    if (flashSize == 0) {
        flashSize = 8388608;
    }

    leftSize = flashSize - imageSize - backupSize;

    LOG(">>>>flashSize = %dByte", flashSize);
    LOG(">>>>imageSize = %dByte", imageSize);
    LOG(">>>>backupSize = %dByte", backupSize);
    LOG(">>>>leftSize = %dByte", leftSize);

    if ((leftSize < 0) || (imageSize <= 0)) {
        ret = RET_ERR;
    }

    return ret;
}

static int check_battery_status()
{
    /* TODO implemented */
    LOG("No battery, pass");
    return RET_OK;
}

static int check_powerline_status()
{
    /* TODO implemented */
    LOG("No powerline, pass");
    return RET_OK;
}

static int check_checksum(char *firmware_path)
{
    return RET_OK;
#if 0
    char hwid[100] = { 0 };
    int ret = RET_OK;

    exec_cmd3(hwid, sizeof(hwid), "cat /tmp/.bootvars | grep id= | cut -d '=' -f 2" );
    if (exec_cmd2("chksum -i %s -h %s  -v 1", firmware_path, hwid) != 0) { 
        LOG("Firmware File chksum failed!");
        ret = RET_ERR;
    } else {
        LOG("Firmware File chksum passed!");
    }

    return ret;
#endif
}

/*
    Check if now is suitable for firmware upgrade,
    If ok, return 0
    If fail, return 1
*/
static int check_firmware_upgrade()
{
    LOG("=======>check_firmware_upgrade for conditions");
    if (check_checksum("/tmp/firmware") != RET_OK)
        return RET_ERR;

    if (check_firmware_version() != RET_OK) {
        LOG("Cancel upgrade, Firmware Version the same!");
        return RET_ERR;
    }

    if  (check_flash_size() != RET_OK) {
        LOG("Cancel upgrade, Flash Space not enough!!");
        return RET_ERR;
    }

    if (check_battery_status() != RET_OK) {
        LOG("Cancel upgrade, Low Battery!!");
        return RET_ERR;
    }

    if (check_powerline_status() != RET_OK) {
        LOG("Cancel upgrade, No Power Line!!");
        return RET_ERR;
    }

    LOG("=======>check_firmware_upgrade done");

    return RET_OK;
}

static void reboot_directly()
{
    LOG("Reboot Directly!!");
    exec_cmd("/sbin/reboot2");
    sleep(1);
    LOG("Oops!! Reboot Again!!");
    exec_cmd("/sbin/reboot2");
    exec_cmd("reboot");
}

static void disable_watchdog()
{
    LOG( "kill watchdog!");
    killall("watchdog", SIGKILL);
    LOG( "kill watchdog! done");
}

static void setenv_keep_alive(char *file)
{
    FILE *fp;
    char buf[512];

    fp = fopen(file, "r");
    if (fp) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if (sscanf(buf, "export WDKUPGRADE_KEEP_ALIVE=%s", buf) != -1) {
                LOG("export WDKUPGRADE_KEEP_ALIVE=%s", buf);
                setenv("WDKUPGRADE_KEEP_ALIVE", buf, 1);
                break;
            }
        }
        fclose(fp);
    }
}

static void before_upgrade()
{
#ifdef CONFIG_PACKAGE_mcc
    if (f_exists("/usr/bin/mcc_key")) {
        exec_cmd("mcc_key z 0");
    }
#endif

    if (f_exists("/tmp/wdkupgrade_before"))
        exec_cmd("/tmp/wdkupgrade_before");

    free_memory(1);

    if (f_isdir("/overlay/playlists)"))  {
        writeline("/tmp/backup.lst", "/overlay/playlists)");
    }

    exec_cmd("cdb revert");
    exec_cmd("cdb commit");
}

static void after_upgrade()
{
#ifdef CONFIG_PACKAGE_mcc
    if (f_exists("/usr/bin/mcc_key")) {
        exec_cmd2("mcc_key z %d", (wdkupgrade_result == RET_OK) ? 1 : 2);  
    }
#endif

    if (f_exists("/tmp/wdkupgrade_after")) {
        exec_cmd("/tmp/wdkupgrade_after");
    }

    disable_watchdog();
    LOG("after upgrade: done");
}

/*
    Check the firmware of offset byte 22,

    unsigned short flags;
    #define IH_EXECUTABLE (1<<4)

    0x10 -> it is bootexe
    else -> it is not bootexe
*/
static int check_boot_exe(char *path)
{
    FILE *fp;
    unsigned char k[22] = { 0 };
    int ret = RET_ERR;

    if ((fp = fopen(path, "r")) != NULL) {
        fread(k, 22, 1, fp);
        fclose(fp);

        if (k[21] & 0x10) {
            ret = RET_OK;
        }
    }

    return ret;
}

static int upgrade_core()
{
    char cmd[128];
    /*
     *  If the firmware is bootexe, run the firmware as elf program
     *  else the firmware is just the firmware
     */
    if (check_boot_exe("/tmp/firmware") == RET_OK) {
        //bootexe means it is the exe that can update the boot,just write the boot.img to mtd0
        LOG("Updating boot with bootexe..");
        exec_cmd("dd if=/tmp/firmware of=/tmp/firmware.tmp bs=1 skip=48"); 
        if (chmod("/tmp/firmware.tmp", S_IRWXU) != RET_OK) {
            LOG("chmod /tmp/firmware.tmp fail");
        }
        LOG("Executing /tmp/firmware.tmp");
        snprintf(cmd, sizeof(cmd), "/tmp/firmware.tmp");
    } else {
        LOG("Executing "SYSUPGRADE_RAM_BIN);
        snprintf(cmd, sizeof(cmd), SYSUPGRADE_RAM_BIN" -v -b /tmp/backup.lst /tmp/firmware");
    }

    return exec_cmd(cmd);
}

static void start_upgrade(char *dir, char *firmware_name)
{
    /* not implement for panther yet */
    return;

#if 0
    snprintf(fw_path, sizeof(fw_path), "%s/%s", dir, firmware_name);

    LOG("Open firmware %s to check the header", fw_path);
    if (check_firmware_header(fw_path) == RET_ERR) {
        LOG("firmware header check failed");
        return;
    }

    LOG("firmware header check passed");

    free_memory(0);

    LOG("Copy file from %s to /tmp/firmware", fw_path);
    if (cp(fw_path, "/tmp/firmware") != 0) {
        LOG("Copy file failed");
        goto update_end;
    }

    before_upgrade();
    if (check_firmware_upgrade() != RET_OK) {
        LOG("Firmware Upgrade failed");
    }
    else {
        wdkupgrade_result = upgrade_core();
    }

update_end:
    after_upgrade();
    sleep(3);
    reboot_directly();
#endif
}

static void scan_usb_mmc()
{
    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    char sys_auto_upgrade_name[100] = { 0 };

    cdb_get("$sys_auto_upgrade_name", sys_auto_upgrade_name);

    LOG("Read file content of /var/diskinfo.txt");
    stream = fopen("/var/diskinfo.txt", "r");
    if (stream == NULL) {
        LOG("Result shows no disk inserted");
        return;
    }

    while ((read = getline(&line, &len, stream)) != -1) {
        line[strlen(line) -1] = 0x00; /* delete the \n, or opendir will fail */
        LOG("Found storage device:%s, start to search for %s", line, sys_auto_upgrade_name);

        DIR *d = NULL;
        struct dirent *dir = NULL;
        d = opendir(line);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.')
                    continue;
                if ((dir->d_type == DT_REG) && (strcmp(dir->d_name, sys_auto_upgrade_name) == 0)) {
                    LOG("Found firmware:%s/%s, start to upgrade", line, dir->d_name);
                    start_upgrade(line, sys_auto_upgrade_name);
                    break;
                }
            }
            closedir(d);
        } else {
            LOG("opendir %s  failed, err= %d", line, errno);
        }
    }

    LOG("Search End");
    free(line);
    fclose(stream);
}

/*
    Read diskfile and auto upgrade firmware from disk when disk is inserted to board

    1.The name must be $sys_auto_upgrade_name, firmware.img default
    2.Firmware version must be not same, defined in arch/mips/boot/Makefile
    3.Battery power low will cause fail and exit
*/
static void auto_firmware_upgrade(char *para)
{
    int sys_auto_upgrade = 0;

    sys_auto_upgrade = cdb_get_int("$sys_auto_upgrade", 0);

    if (0 == sys_auto_upgrade) {
        LOG("cdb checked sys_auto_upgrade is 0, auto upgrade firmware exit");
    }
    else {
        LOG("Scanning disk for firmware upgrade");

        /*
        root@router:/# cat /var/diskinfo.txt
        /media/sda1
        /media/sdb1
        */

        scan_usb_mmc();
    }
}

static void web_firmware_upgrade()
{
    update_upload_state(WEB_STATE_ONGOING);
    free_memory(0);

    if (check_firmware_upgrade() != RET_OK) {
        update_upload_state(WEB_STATE_FAILURE);
        LOG("check failed, Do nothing");
    } else {
        update_upload_state(WEB_STATE_FWSTART);
        LOG("Wait 3 seconds to reply web");

        sleep(3);
        before_upgrade();

        LOG("Writing firmware_file to flash...");
        wdkupgrade_result = upgrade_core();

        //TODO Judge firmware update fail via update_result
        update_upload_state(WEB_STATE_SUCCESS);
    }

    after_upgrade();
    sleep(3);
    reboot_directly();
}

/*
    If you want to do something such as light on a led before sysupgrade or after,
    please put scripts with the name:
    /lib/wdk/sysupgrade_before
    /lib/wdk/sysupgrade_after
*/
int wdk_sysupgrade(int argc, char **argv)
{
    char path[1024] = { 0 };
    char daemon_cmd[1024] = { 0 };
    int i = 0;

    get_self_path(path, sizeof(path));

    LOG("sysupgrade is curently at %s", path);
    if (strcmp("/lib/wdk/wdk-bin", path) == 0) {
        cp(SYSUPGRADE_FLASH_BIN, SYSUPGRADE_RAM_BIN);
        cp(WDKUPGRADE_FLASH_BIN, WDKUPGRADE_RAM_BIN);

        if (f_exists("/lib/wdk/sysupgrade_before")) {
            cp("/lib/wdk/sysupgrade_before", "/tmp/wdkupgrade_before");
            chmod("/tmp/wdkupgrade_before", S_IRWXU);
            setenv_keep_alive("/tmp/wdkupgrade_before");
        }

        if (f_exists("/lib/wdk/sysupgrade_after")) {
            cp("/lib/wdk/sysupgrade_after", "/tmp/wdkupgrade_after");
            chmod("/tmp/wdkupgrade_after", S_IRWXU);
        }

        for (i = 0; i < argc; i++) {
            strcat(daemon_cmd, argv[i]);
            strcat(daemon_cmd, " ");
        }
        //-S:start a process unless a matching process is found.
        LOG("Pass daemon cmd = %s", daemon_cmd);
        start_stop_daemon(WDKUPGRADE_RAM_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_10, 0, NULL, 
            "%s", daemon_cmd);
    } else if  (strcmp(WDKUPGRADE_RAM_BIN, path) == 0) {
        if ((argc > 1) && strcmp(argv[0], "auto") == 0) {
            if (argc >= 2) {
                LOG("Enter auto_firmware_upgrade, argument = %s", argv[1]);
                auto_firmware_upgrade(argv[1]);
            } else {
                LOG("Enter auto_firmware_upgrade");
                auto_firmware_upgrade(NULL);
            }
        } else {
            LOG("Enter web_firmware_upgrade");
            web_firmware_upgrade();
        }
    }

    return 0;
}

