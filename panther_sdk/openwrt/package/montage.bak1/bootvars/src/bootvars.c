/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Technology, Inc. All rights reserved.                                |
|                                                                              |
+=============================================================================*/
/*!
*   \file bootvars.c
*   \brief Read bootvars from flash
*   \author Montage
*/

/*=============================================================================+
| Included Files                                                               |
+=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <unistd.h>

#include "otp.h"

#define ___swab16(x) \
     ((unsigned short)( \
         (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) | \
         (((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))

#define ___swab32(x) \
     ((unsigned int)( \
         (((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
         (((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) | \
         (((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) | \
         (((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
#define be32_to_cpu(x) ___swab32(x)
#define be16_to_cpu(x) ___swab16(x)
#else
#define be32_to_cpu(x) (x)
#define be16_to_cpu(x) (x)
#endif

struct cdb_id
{
#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
    unsigned short array:1;
    unsigned short idx:7;
    unsigned short type:3;
    unsigned short mod:5;
#else
    unsigned short mod:5;
    unsigned short type:3;
    unsigned short idx:7;
    unsigned short array:1;
#endif
};

#define CDB_ID_(type, index, array)     (((31 & 0x1f) << 11) | ((type & 0x07) << 8) | ((index & 0x7f) << 1) | (array & 0x01))

enum
{
    CDB_INT = 1,
    CDB_STR = 2,
    CDB_IP = 3,
    CDB_MAC = 4,
    CDB_VER = 3,

    CDB_BOOT_HVER = CDB_ID_(CDB_VER, 0, 0),
    CDB_BOOT_ID = CDB_ID_(CDB_INT, 1, 0),
    CDB_BOOT_FILE = CDB_ID_(CDB_STR, 2, 0),
    CDB_BOOT_MAC = CDB_ID_(CDB_MAC, 3, 1),
    CDB_BOOT_MODE = CDB_ID_(CDB_INT, 4, 0),
    CDB_BOOT_VER = CDB_ID_(CDB_VER, 5, 0),
    CDB_BOOT_BUF = CDB_ID_(CDB_INT, 6, 0),
    CDB_BOOT_SZ = CDB_ID_(CDB_INT, 7, 0),
    CDB_BOOT_SRC = CDB_ID_(CDB_INT, 8, 0),
    CDB_BOOT_IP = CDB_ID_(CDB_IP, 9, 0),
    CDB_BOOT_MSK = CDB_ID_(CDB_IP, 10, 0),
    CDB_BOOT_GW = CDB_ID_(CDB_IP, 11, 0),
    CDB_BOOT_SVR = CDB_ID_(CDB_IP, 12, 0),
    CDB_BOOT_SRC2 = CDB_ID_(CDB_INT, 13, 0),
    CDB_BOOT_LOG_SRC = CDB_ID_(CDB_INT, 14, 0),
    CDB_BOOT_LOG_SZ = CDB_ID_(CDB_INT, 15, 0),
    CDB_BOOT_CDB_LOC = CDB_ID_(CDB_INT, 16, 0),
    CDB_BOOT_CDB_SZ = CDB_ID_(CDB_INT, 17, 0),
    CDB_BOOT_CVER = CDB_ID_(CDB_STR, 18, 0),
    CDB_BOOT_RFC = CDB_ID_(CDB_STR, 19, 0),
    CDB_BOOT_PLL = CDB_ID_(CDB_INT, 20, 0),
    CDB_BOOT_TXVGA = CDB_ID_(CDB_STR, 21, 0),
    CDB_BOOT_RXVGA = CDB_ID_(CDB_STR, 22, 0),
    CDB_BOOT_SERIAL = CDB_ID_(CDB_INT, 23, 0),
    CDB_BOOT_PIN = CDB_ID_(CDB_STR, 24, 0),
    CDB_BOOT_FREQ_OFS = CDB_ID_(CDB_INT, 25, 0),
    CDB_BOOT_MADC_VAL = CDB_ID_(CDB_STR, 26, 1),
    CDB_BOOT_LNA = CDB_ID_(CDB_STR, 27, 0),
    CDB_BOOT_POWERCFG = CDB_ID_(CDB_INT, 42, 0),

    CDB_ID_SZ = sizeof (struct cdb_id),
    CDB_AID_SZ = CDB_ID_SZ + sizeof (int),      /* array idx , ie 4 */
    CDB_LEN_SZ = sizeof (short),
    CDB_CRC_SZ = 4,
};

#define bootvar_log(format, args...)        //printf(format, ##args)

//-------------------------------------------------------------
struct cdbobj
{
    union
    {
        unsigned short v;
        struct cdb_id f;
    } id;
    unsigned short len;
    int idx;                    // skip if not arrary
};

typedef struct bootvar
{
    int mode;
    unsigned int ver;
    unsigned char mac0[8];
    unsigned char mac1[8];
    unsigned char mac2[8];
    int vlan;

    unsigned int ip;
    unsigned int msk;
    unsigned int gw;
    unsigned int server;
    unsigned int load_sz;
    unsigned int load_addr;
    unsigned int load_src;
    unsigned int load_src2;
    unsigned int log_src;
    unsigned int log_sz;
    unsigned int id;
    unsigned int hver;
    unsigned int pll;
    unsigned int serial;
    char file[32];
    char cver[16];
    char rfc[128];
    char txvga[128];
    char rxvga[128];
    char pin[16];
    unsigned int freq_ofs;
    char madc_val0[64];
    char madc_val1[64];
    char lna[32];
    unsigned int powercfg;
} bootvar;

bootvar bootvars;

struct parmd
{
    void *val;
    union
    {
        unsigned short v;
        struct cdb_id f;
    } id;
    char *name;
    char *desc;
    int dirty_bit;
};

#define CDB_DPTR(p) ((void*)(&p->idx))
#define CDB_IDV(p)   (be16_to_cpu(p->id.v))
#define CDB_DLEN(p) ((be16_to_cpu(p->len)+3)&0xfffc)
#define CDB_AIDX(p) (be32_to_cpu(p->idx))
#define CDB_AIDX_SZ sizeof(int)
#define CDBV(id)    (*((unsigned short*)&id))

#define CDBE(x, i, n, d, b)     { .val = x, .id.v = i, .name = n, .desc = d, .dirty_bit = b }

struct parmd parmds[] = {
    CDBE(&bootvars.hver, CDB_BOOT_HVER, "hver", "h/w ver info", 1),
    CDBE(&bootvars.cver, CDB_BOOT_CVER, "cver", "customer ver info", 1),
    CDBE(&bootvars.id, CDB_BOOT_ID, "id", "MSB: verder id, LSB: product id", 1),
    CDBE(&bootvars.file, CDB_BOOT_FILE, "file", "loading file name", 1),
    CDBE(&bootvars.mac0, CDB_BOOT_MAC, "mac0", "mac address 0", 1),
    CDBE(&bootvars.mac1, CDB_BOOT_MAC, "mac1", "mac address 1", 1),
    CDBE(&bootvars.mac2, CDB_BOOT_MAC, "mac2", "mac address 2", 1),
    CDBE(&bootvars.mode, CDB_BOOT_MODE, "mode", "boot mode", 1),
    CDBE(&bootvars.ver, CDB_BOOT_VER, "ver", "ver info", 1),
    CDBE(&bootvars.load_addr, CDB_BOOT_BUF, "buf", "loading buffer", 1),
    CDBE(&bootvars.load_sz, CDB_BOOT_SZ, "size", "loading size", 1),
    CDBE(&bootvars.load_src, CDB_BOOT_SRC, "addr", "loading from", 1),
    CDBE(&bootvars.ip, CDB_BOOT_IP, "ip", "ip address", 1),
    CDBE(&bootvars.msk, CDB_BOOT_MSK, "msk", "net mask", 1),
    CDBE(&bootvars.gw, CDB_BOOT_GW, "gw", "gateway ip", 1),
    CDBE(&bootvars.server, CDB_BOOT_SVR, "server", "tftp server ip", 1),
    CDBE(&bootvars.load_src2, CDB_BOOT_SRC2, "backup_addr", "loading from backup addr", 1),
    CDBE(&bootvars.log_src, CDB_BOOT_LOG_SRC, "log_addr", "loading log from", 1),
    CDBE(&bootvars.log_sz, CDB_BOOT_LOG_SZ, "log_size", "log size", 1),
    CDBE(&bootvars.rfc, CDB_BOOT_RFC, "rfc", "rfc", 1),
    CDBE(&bootvars.pll, CDB_BOOT_PLL, "pll", "custom PLL setting", 1),
    CDBE(&bootvars.txvga, CDB_BOOT_TXVGA, "txvga", "txvga calib value", 1),
    CDBE(&bootvars.rxvga, CDB_BOOT_RXVGA, "rxvga", "rxvga calib value", 1),
    CDBE(&bootvars.serial, CDB_BOOT_SERIAL, "serial", "serial number", 1),
    CDBE(&bootvars.pin, CDB_BOOT_PIN, "pin", "WPS PIN number", 1),
    CDBE(&bootvars.freq_ofs, CDB_BOOT_FREQ_OFS, "freq_ofs", "frequnecy offset calib value", 1),
    CDBE(&bootvars.madc_val0, CDB_BOOT_MADC_VAL, "madc_val0", "madc value 0", 1),
    CDBE(&bootvars.madc_val1, CDB_BOOT_MADC_VAL, "madc_val1", "madc value 1", 1),
    CDBE(&bootvars.lna, CDB_BOOT_LNA, "lna", "lna gain", 1),
    CDBE(&bootvars.powercfg, CDB_BOOT_POWERCFG, "powercfg", "power configurations", 1),
};

//-------------------------------------------------------------
#define CDB_ID_NUM          (sizeof(parmds)/sizeof(struct parmd))
#define CDB_ID_END          0xffff

#define isdigit(c)          ((c) >= '0' && (c) <= '9')

//-------------------------------------------------------------
static int id_to_idx(unsigned short id);
unsigned short name_to_idx(char *name);

/*!
 * function: id_to_idx
 *
 *  \brief
 *  \param  id
 *  \return 0: ok
 */
static int id_to_idx(unsigned short id)
{
    int i;
    for (i = 0; i < CDB_ID_NUM; i++)
    {
        if (id == parmds[i].id.v)
            return i;
    }
    bootvar_log("Invalid id: %08x\n", id);
    return -1;
}

/*!
 * function: name_to_id
 *
 *  \brief  store to flash
 *  \param  name
 *  \return 0: ok
 */
unsigned short name_to_id(char *name)
{
    struct parmd *pd;
    int i;

    for (i = 0, pd = &parmds[0]; i < CDB_ID_NUM; i++, pd++)
    {
        if (!strcmp(pd->name, name))
        {
            return pd->id.v;
        }
    }
    return 0;
}

/*!
 * function: name_to_idx
 *
 *  \brief  convert array index
 *  \param  name
 *  \return 0: ok
 */
unsigned short name_to_idx(char *name)
{
    unsigned short array_num = 0;
    int i, num, len = strlen(name);

    if (len == 0)
        return 0;

    for (num = 0, i = 1; i <= 3; i++, num++)
        if (!isdigit(name[len - i]))
            break;

    if (num)
        array_num = (unsigned short) atoi(name + len - num);
    return array_num;

}

extern unsigned short crc16_ccitt(const void *buf, int len);

int cdb_check(const unsigned char *cdb_base, int size)
{
    unsigned short tcrc =  be16_to_cpu(*((unsigned short *) (&cdb_base[size - 4])));

    return !(tcrc == crc16_ccitt(cdb_base, (size - 4)));
}

static int output_to_rdb = 0;
extern int cdb_set(const char *name, void *value);
void create_bootvars_rdb(void)
{
    char key[64];
    char value[512];
    struct parmd *pd;
    int i;
    unsigned long tmp;
    unsigned char *ip;

    for(i = 0, pd = &parmds[0]; i < CDB_ID_NUM; i++, pd++)
    {
        if(0 == pd->dirty_bit)
        {
            snprintf(key, 64, "$boot_%s", pd->name);

            value[0] = '\0';
            switch (pd->id.f.type)
            {
                case CDB_INT:
                    sprintf(value, "%u", *(unsigned int *) pd->val);
                    break;
                case CDB_IP:
                    tmp = htonl(*(unsigned long *) (pd->val));
                    ip = (unsigned char *)(&tmp);
                    sprintf(value, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                    break;
                case CDB_MAC:
                    sprintf(value, "%s", ether_ntoa(pd->val));
                    break;
                case CDB_STR:
                    sprintf(value, "%s", (char *)pd->val);
                    break;
            }
            cdb_set(key, value);
        }
    }
}

#define BOOTVARS_FILE   "/tmp/.bootvars"
#define BOOTVARS_FILE_T "/tmp/.bootvars.tmp"
#define BOOTVARS_MTD    "/dev/mtd1"
#define FLASH_PAGE_NUM  (64)

#define ISPWR2(x)   (!(x & (x - 1)))

void create_bootvars_file(void)
{
    FILE *fp = NULL;
    struct parmd *pd;
    int i;
    unsigned long tmp;
    unsigned char *ip;

    fp = fopen(BOOTVARS_FILE_T, "w");
    if(NULL != fp)
    {
        for(i = 0, pd = &parmds[0]; i < CDB_ID_NUM; i++, pd++)
        {
            if(0 == pd->dirty_bit)
            {
                fprintf(fp, "%s=", pd->name);
                switch (pd->id.f.type)
                {
                    case CDB_INT:
                        fprintf(fp, "%08x", *(unsigned int *) pd->val);
                        break;
                    case CDB_IP:
                        tmp = htonl(*(unsigned long *) (pd->val));
                        ip = (unsigned char *)(&tmp);
                        fprintf(fp, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                        break;
                    case CDB_MAC:
                        fprintf(fp, "%s", ether_ntoa(pd->val));
                        break;
                    case CDB_STR:
                        fprintf(fp, "%s", (char *)pd->val);
                        break;
                }
                fprintf(fp, "\n");
            }
        }
    }

    if(fp)
    {
        fclose(fp);
    }

    rename(BOOTVARS_FILE_T, BOOTVARS_FILE);
    bootvar_log("Create %s for bootvars done\n", BOOTVARS_FILE);

    return;
}

void parse_bootvars(void *buf, unsigned int flash_page_size, unsigned int flash_block_size)
{
    struct parmd *pd;
    struct cdbobj *p;
    unsigned short array_num = 0;
    int idx, dl, num = 0, pidx = 0;
    void *dp;

    if(!ISPWR2(flash_block_size))
    {
        bootvar_log("!!!Error: flash_page_size is not 2^x\n");
        return;
    }
#define FLASH_PAGE_ADDR_MASK    (flash_page_size - 1)

    bootvar_log("parse_bootvars buf = %p, page_size = %d, block_size = %d\n", buf, flash_page_size, flash_block_size);
    p = (struct cdbobj *)buf;
    while ((unsigned int)p < (unsigned int)(buf + flash_block_size))
    {
        if ((CDB_IDV(p) == CDB_ID_END)
            || ((CDB_DLEN(p) + CDB_ID_SZ + CDB_LEN_SZ)
                 > (flash_page_size - (((unsigned int)p - (unsigned int)buf) & FLASH_PAGE_ADDR_MASK)))
            || cdb_check((const unsigned char *) p, (CDB_DLEN(p) + CDB_ID_SZ + CDB_LEN_SZ)))
        {
            if (p == (buf + flash_page_size * pidx))
            {
                bootvar_log("parse_bootvars end at empty page\n");
                break;
            }
            else
            {
                pidx++;
                p = (struct cdbobj *) ((unsigned int) buf + flash_page_size * pidx);
                continue;
            }
        }

        // on flash format is always big-endian
        p->id.v = be16_to_cpu(p->id.v);

        if ((idx = id_to_idx(p->id.v)) < 0)
        {
            goto Next;
        }
        array_num = 0;

        if (p->id.f.array)
        {
            array_num = CDB_AIDX(p);
        }
        pd = &parmds[idx + array_num];

        if (pd->val == 0)
        {
            goto Next;
        }
        dp = CDB_DPTR(p);
        dl = CDB_DLEN(p);
        if (p->id.f.array)
        {
            dp += 4;            /* 4 bytes index */
            dl -= 4;
        }
        dl -= CDB_CRC_SZ;       // remove crc
        memcpy(pd->val, dp, dl);

        if((p->id.f.type==CDB_IP) || (p->id.f.type==CDB_INT))
            *(unsigned long *)pd->val = be32_to_cpu(*(unsigned long *)pd->val);

        if (CDB_STR == p->id.f.type)
        {
            *(((char *) pd->val) + dl) = 0;
        }
        pd->dirty_bit = 0;
        num++;
Next:
        p = (struct cdbobj *) (CDB_DPTR(p) + CDB_DLEN(p));
    }

    if (!num)
    {
        bootvar_log("bootvar is empty!\n");
    }
    else
    {
        if(output_to_rdb)
            create_bootvars_rdb();
        else
            create_bootvars_file();
    }

    return;
}

void mtd_bootvar_read(int is_test)
{
    int mtd_fp = -1;
    mtd_info_t mtd_info;
    size_t read_len = 0;
    unsigned char *buf = NULL;
    int i;

    mtd_fp = open(BOOTVARS_MTD, O_RDONLY);
    if(0 > mtd_fp)
    {
        bootvar_log("Could not open %s\n", BOOTVARS_MTD);
        goto Exit;
    }
    if(ioctl(mtd_fp, MEMGETINFO, &mtd_info))
    {
        bootvar_log("Could not get MTD device info from %s\n", BOOTVARS_MTD);
        goto Exit;
    }
    if(0 >= mtd_info.size)
    {
        bootvar_log("%s MTD size is not positive\n", BOOTVARS_MTD);
        goto Exit;
    }

    buf = (unsigned char *)malloc(mtd_info.size);
    if(NULL == buf)
    {
        bootvar_log("!!!OOM, could not alloc buf with size 0x%x\n", mtd_info.size);
        goto Exit;
    }
    memset(buf, 0, mtd_info.size);

    read_len = read(mtd_fp, buf, mtd_info.size);
    if(read_len != mtd_info.size)
    {
        perror("read bootvars form flash");
        goto Exit;
    }

    if(is_test)
    {
        for(i=0; i < read_len; i++)
        {
            if(0 == (i & 0xfUL))
            {
                bootvar_log("\n0x%08x:", i);
            }
            bootvar_log(" %02x", (unsigned char)(buf[i] & 0xffUL));
        }
        bootvar_log("\n");
    }
    else
    {
        parse_bootvars(buf, (mtd_info.size)/FLASH_PAGE_NUM, mtd_info.size);
    }

Exit:
    if(buf)
    {
        free(buf);
    }

    if(mtd_fp)
    {
        close(mtd_fp);
    }

    return;
}

void otp_mac_increase(unsigned char *mac)
{
    int i = 5, v, b_add_next = 0;
    do
    {
        v = mac[i] & 0xff;
        v++;
        if ((v & 0xff) == 0)
        {
            b_add_next = 1;
        }
        else
        {
            b_add_next = 0;
        }
        mac[i] = v & 0xff;
        i--;
    } while ((i > 2) && (b_add_next == 1));
}

void otpdata_free(struct otpdata* data)
{
    if(data->mac_addr)
        free(data->mac_addr);

    if(data->fofs)
        free(data->fofs);

    if(data->txvga_gain)
        free(data->txvga_gain);

    if(data->txp_diff)
        free(data->txp_diff);
}

void mtd_otpvar_read()
{
    struct otpdata otp_data = {
        .txvga_gain = NULL,
        .fofs = NULL,
        .txp_diff = NULL,
        .mac_addr = NULL
    };
    int ret;
    char value[32];
//  unsigned char test_mac_addr[6] = {
//          0x64, 0x64, 0x64, 0x02, 0xff, 0xff
//  };

    ret = get_otp_data(&otp_data);
    if(ret != OTP_ERR_OK)
    {
        return;
    }

    if(otp_data.mac_addr)
    {
        memset(value, 0, sizeof(value));
        sprintf(value, "%s", ether_ntoa((void*)otp_data.mac_addr));
//      sprintf(value, "%s", ether_ntoa((void*)test_mac_addr));
        cdb_set("$boot_mac0", value);
        cdb_set("$otp_mac", value);

        otp_mac_increase(otp_data.mac_addr);
//      otp_mac_increase(test_mac_addr);
        memset(value, 0, sizeof(value));
        sprintf(value, "%s", ether_ntoa((void*)otp_data.mac_addr));
//      sprintf(value, "%s", ether_ntoa((void*)test_mac_addr));
        cdb_set("$boot_mac2", value);
    }

    if(otp_data.fofs)
    {
        memset(value, 0, sizeof(value));
        sprintf(value, "%02x", otp_data.fofs[0]);
        cdb_set("$otp_fofs", value);
    }

    if(otp_data.txvga_gain)
    {
        memset(value, 0, sizeof(value));
        sprintf(value, "%02x%02x%02x%02x%02x%02x%02x",
                otp_data.txvga_gain[0], otp_data.txvga_gain[1],
                otp_data.txvga_gain[2], otp_data.txvga_gain[3],
                otp_data.txvga_gain[4], otp_data.txvga_gain[5],
                otp_data.txvga_gain[6]);
        cdb_set("$otp_txvga_gain", value);
    }

    if(otp_data.txp_diff)
    {
        memset(value, 0, sizeof(value));
        sprintf(value, "%02x%02x", otp_data.txp_diff[0], otp_data.txp_diff[1]);
        cdb_set("$otp_txp_diff", value);
    }

    otpdata_free(&otp_data);
}

#define DUMP_FLASH_TEST  (1)
int main(int argc, char *argv[])
{
    FILE *fp = NULL;

    if((argc > 1) && (!strcmp(argv[1],"rdb")))
    {
        output_to_rdb = 1;
        mtd_bootvar_read(0);
        mtd_otpvar_read();
        return 0;
    }

    fp = fopen(BOOTVARS_FILE, "r");
    if(NULL == fp)
    {
        if((argc > 1) && (!strcmp(argv[1],"test")))
        {
            bootvar_log("dump flash for test!\n");
            mtd_bootvar_read(DUMP_FLASH_TEST);
        }
        else
        {
            bootvar_log("File no exist, create it!\n");
            mtd_bootvar_read(!DUMP_FLASH_TEST);
        }
    }

    if(fp)
    {
        fclose(fp);
    }

    return 0;
}
