#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "debug.h"

#define MTD_FACTORY     "/dev/mtd3"
#define ETH0_OFFSET     0x28
#define RA0_OFFSET      0x04

#define MACADDR_LEN     6
#define WIFIRF_LEN  512

#define MEMGETINFO  _IOR('M', 1, struct mtd_info_user)
#define MEMERASE    _IOW('M', 2, struct erase_info_user)
#define MEMUNLOCK   _IOW('M', 6, struct erase_info_user)

struct erase_info_user
{
    unsigned int start;
    unsigned int length;
};

struct mtd_info_user
{
    unsigned char type;
    unsigned int flags;
    unsigned int size;
    unsigned int erasesize;
    unsigned int oobblock;
    unsigned int oobsize;
    unsigned int ecctype;
    unsigned int eccsize;
};

int mtd_write(int phy, char **value)
{
    int sz = 0;
    int i;
    struct mtd_info_user mtdInfo;
    struct erase_info_user mtdEraseInfo;
    int fd = open(MTD_FACTORY, O_RDWR | O_SYNC);
    unsigned char *buf, *ptr;
    if(fd < 0)
    {
        DEBUG_ERR( "Could not open mtd device: %s\n", MTD_FACTORY);
        return -1;
    }
    if(ioctl(fd, MEMGETINFO, &mtdInfo))
    {
        DEBUG_ERR( "Could not get MTD device info from %s\n", MTD_FACTORY);
        close(fd);
        return -1;
    }
    mtdEraseInfo.length = sz = mtdInfo.erasesize;
    buf = (unsigned char *)malloc(sz);
	if(NULL == buf){
		DEBUG_ERR("Allocate memory for sz failed.");
		close(fd);
		return -1;        
	}
	if(read(fd, buf, sz) != sz){
        DEBUG_ERR( "read() %s failed\n", MTD_FACTORY);
        goto write_fail;
    }
    mtdEraseInfo.start = 0x0;
    for (mtdEraseInfo.start; mtdEraseInfo.start < mtdInfo.size; mtdEraseInfo.start += mtdInfo.erasesize)
    {
        ioctl(fd, MEMUNLOCK, &mtdEraseInfo);
        if(ioctl(fd, MEMERASE, &mtdEraseInfo))
        {
            DEBUG_ERR("Failed to erase block on %s at 0x%x", MTD_FACTORY, mtdEraseInfo.start);
            goto write_fail;
        }
    }
    if (phy == 1)
        ptr = buf + ETH0_OFFSET;
    if (phy == 2)
        ptr = buf + RA0_OFFSET;
    for (i = 0; i < MACADDR_LEN; i++, ptr++)
        *ptr = strtoul(value[i], NULL, 16);
    lseek(fd, 0, SEEK_SET);
    if (write(fd, buf, sz) != sz)
    {
        DEBUG_ERR("write() %s failed", MTD_FACTORY);
        goto write_fail;
    }

    close(fd);
    free(buf);
    return 0;
write_fail:
    close(fd);
    free(buf);
    return -1;
}
