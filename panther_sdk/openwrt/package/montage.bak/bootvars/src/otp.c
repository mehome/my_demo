#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "otp.h"

#define REG_WRITE32(addr, val)  (*(volatile unsigned long *)(addr)) = ((unsigned long)(val))
#define REG_READ32(addr)        (*(volatile unsigned long *)(addr))
#define REG_UPDATE32(x, val, mask) do {                     \
    unsigned int newval;                                    \
    newval = *(volatile unsigned int*) (x);                 \
    newval = (( newval & ~(mask) ) | ( (val) & (mask) ));   \
    *(volatile unsigned int*)(x) = newval;                  \
} while(0)

struct uio_otp {
    int fd;
    unsigned long* reg;
    unsigned long* avdd_reg;
};

static struct uio_otp *otp_op = NULL;

int otp_datalen[OTP_DATA_TYPE_NUM] = {
    OTP_LEN_CUSTOM,     // custom length
    OTP_LEN_FOFS,       //
    OTP_LEN_TXP_DIFF,   //
    4,                  //
    OTP_LEN_MAC,        // MAC
    OTP_LEN_TXVGA,      // OTP_TXVGA
    RESERVED_LEN, RESERVED_LEN  // shuold not match this
};

void dump_otp_to_buf(unsigned char *bufp)
{
    unsigned int i, index_start = OTP_START_INDEX, index_max = OTP_BYTE_SIZE;
    unsigned int reg_data;

    REG_WRITE32(otp_op->avdd_reg, 0);

    reg_data    = OTP_READ_MODE_EN | OTP_CLK_PLUSE_WIDTH;
    REG_WRITE32(otp_op->reg, reg_data);
    REG_UPDATE32(otp_op->avdd_reg, 0, OTP_CLK_PLUSE_WIDTH_BIT11);
    for (i = index_start; index_max > i; i++)
    {
        // Set address
        //printf("%d", i);
        reg_data &= ~(OTP_ADDR_VALID_BIT << OTP_ADDR_SHIFT_BIT);
        reg_data |= ((i & OTP_ADDR_VALID_BIT) << OTP_ADDR_SHIFT_BIT);
        REG_WRITE32(otp_op->reg, reg_data);
        // Enable clock generation
        reg_data |= OTP_CLK_PLUSE_GEN;
        REG_WRITE32(otp_op->reg, reg_data);
        // Wait for data being ready
        while (reg_data & OTP_CLK_PLUSE_GEN)
        {
            reg_data = REG_READ32(otp_op->reg);
        }

        bufp[i] = (unsigned char) (reg_data & OTP_DATA_VALID_BIT);
    }
}

void finalize_otp_uio(void)
{
    if(otp_op == NULL)
    {
        return;
    }

    if (otp_op->fd >= 0)
        close(otp_op->fd);

    free(otp_op);
    otp_op = NULL;
}

int initial_otp_uio(unsigned char *bufp)
{
    unsigned long *mapping_reg = 0;
    otp_op = malloc(sizeof(struct uio_otp));

    otp_op->fd = open(OTP_DEV_PATH, O_RDWR|O_SYNC);
    if (otp_op->fd < 0)
    {
        printf("open device error\n");
        finalize_otp_uio();
        return OTP_ERR_OPEN_DEV;
    }

    mapping_reg = mmap(NULL, 0x10, (PROT_READ | PROT_WRITE), MAP_SHARED, otp_op->fd, 0);
    if (otp_op->reg == MAP_FAILED)
    {
        printf("mmap failed\n");
        finalize_otp_uio();
        return OTP_ERR_MMAP;
    }

    otp_op->reg = mapping_reg + OTP_REG_OFFSET;
    otp_op->avdd_reg = mapping_reg + OTP_AVDD_REG_OFFSET;

    dump_otp_to_buf(bufp);

    return OTP_ERR_OK;
}

/*-----------------------------------------------------------------------------
*  \brief OTP search if otp_id existed
*  \return pos: position of otp_id matched
*  \return < 0: error
-----------------------------------------------------------------------------*/
int otp_search_by_id(unsigned char *otp_bufp, int otp_id)
{
    int p, ret = OTP_ERR_ID_NOT_FOUND, data_len = 0, len_type, find_id;

    if(otp_id < OTP_MIN || otp_id > OTP_MAX) // error parameter
        return OTP_ERR_PARAM;

    for(p = OTP_START_INDEX; p < OTP_BYTE_SIZE && otp_bufp[p] != OTP_EMPTY_VALUE;)
    {
        if(otp_bufp[p] == OTP_OVERWRITTEN_VALUE)
        {
            ++p;
            continue;
        }

        find_id  = (otp_bufp[p] & OTP_ID_MASK);
        //printf("find_id %x, otp_id %x, otp_buf[%d] %x\n", find_id, otp_id, p, otp_bufp[p]);
        if(otp_id == find_id)
        {
            ret = p;
            break;
        }

        len_type = (otp_bufp[p] & OTP_DATALEN_MASK) >> OTP_DATALEN_SHIFT;
        data_len = (len_type > 0) ? otp_datalen[len_type] : otp_bufp[p + 1];
        p += (len_type > 0) ? 1 : 2;

        //printf("data_len %x, otp_buf[%d] %x\n", data_len, p, otp_bufp[p]);
        p += data_len;
    }

    return ret;
}

/*-----------------------------------------------------------------------------
*  \brief OTP read data
*  \param *otp_bufp: buffer filled with otp data
*  \param id: 1 ~ 31
*  \return : pointer for memory to store read data
*  \return NULL : can not read data of otp_id
-----------------------------------------------------------------------------*/
unsigned char * otp_read(unsigned char *otp_bufp, int otp_id)
{
    int datap, data_len;
    unsigned char *src_bufp;
    unsigned char *des_bufp;

    if(otp_op == NULL || otp_bufp == NULL)
        return NULL;

    datap = otp_search_by_id(otp_bufp, otp_id);
    if(datap < 0)
    {
        printf("OTP search error: %d, otp_id %d\n", datap, otp_id);
        return NULL;
    }

    data_len = (otp_bufp[datap] & OTP_DATALEN_MASK) >> OTP_DATALEN_SHIFT;
    if (data_len == 0)
    {
        // [id][len][data]
        datap++;    // move to position of length
        data_len = otp_bufp[datap];
    }
    else
    {
        // [len_type & id][data]
        data_len = otp_datalen[data_len];
    }
    datap++;        // move to position of data
    src_bufp = otp_bufp + datap;

    if (datap + data_len >= OTP_BYTE_SIZE)   // protect for data read
    {
        data_len = OTP_BYTE_SIZE - datap;
    }

    des_bufp = malloc(data_len);

    memcpy(des_bufp, src_bufp, data_len);
    return des_bufp;
}

/*-----------------------------------------------------------------------------
*  \brief OTP filled struct otpdata
*  \param *data: struct otpdata to fill
*  \return OTP_ERR_OK: success
*  \return others: failed
-----------------------------------------------------------------------------*/
int get_otp_data(struct otpdata *data)
{
    int ret = OTP_ERR_OK;
    unsigned char buf[OTP_BYTE_SIZE] = {0};
    ret = initial_otp_uio(buf);
    if(ret != OTP_ERR_OK)
        goto leave;

    data->txvga_gain = otp_read(buf, OTP_TXVGA);
    data->txp_diff = otp_read(buf, OTP_TXP_DIFF);
    data->mac_addr = otp_read(buf, OTP_MAC_ADDR);
    data->fofs = otp_read(buf, OTP_FOFS);

leave:
    finalize_otp_uio();
    return ret;
}

//#define __OTP_DEBUG__
#ifdef  __OTP_DEBUG__

void print_otp_mem(unsigned char *otp_bufp)
{
    int p, c;
    printf("\n===== [debug]OTP memory =====\n");
    for(p = 0; p < 16;p++)
    {
        printf("[%03d]: ", p*16);
        for(c = 0; c < 16; c++)
        {
            printf("%02x ", otp_bufp[p*16 + c]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_data(unsigned char *data, int size)
{
    int p, c;
    printf("\n===== [debug]dump found data =====\n");
    for(c = 0; c < size; c++)
    {
        printf("%02x ", data[c]);
    }
    printf("\n");
}

int main(void)
{
    int ret = OTP_ERR_OK;
    unsigned char buf[OTP_BYTE_SIZE] = {0};
    //unsigned char read_buf[OTP_BYTE_SIZE] = {0};
    struct otpdata data_to_storage;
    ret = initial_otp_uio(buf);
    if(ret != OTP_ERR_OK)
        goto out;

    //print_otp_mem(read_buf);

    ret = otp_read(data_to_storage.txvga_gain, buf, OTP_TXVGA);
    print_data(data_to_storage.txvga_gain, ret);
    ret = otp_read(&data_to_storage.fofs, buf, OTP_FOFS);
    print_data(&data_to_storage.fofs, ret);
    ret = otp_read(data_to_storage.txp_diff, buf, OTP_TXP_DIFF);
    print_data(data_to_storage.txp_diff, ret);
    ret = otp_read(data_to_storage.mac_addr, buf, OTP_MAC_ADDR);
    print_data(data_to_storage.mac_addr, ret);

out:
    finalize_otp_uio();
    return 0;
}
#endif