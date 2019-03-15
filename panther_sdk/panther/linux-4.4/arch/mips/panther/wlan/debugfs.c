/*=============================================================================+
|                                                                              |
| Copyright 2012                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*!
*   \file
*   \brief
*   \author Montage
*/
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/synclink.h>
#include <net/mac80211.h>
#include <os_compat.h>
#include <rf.h>
#include <ip301.h>
#include <rfc.h>
//#include <wla_bb.h>
#include <bb.h>
#include <mac.h>
#include <dragonite_main.h>
#include <mac_ctrl.h>
#include <chip.h>
#include "mac_regs.h"
#include "performance.h"
//#include <linux_wla.h>
//#include <asm/mach-panther/panther.h>
//#include <asm/mach-panther/common.h>
//#include <wlan_def.h>
//#include <wla_mat.h>

//#include <asm/bug.h>

#if defined(CONFIG_PANTHER_INTERNAL_DEBUGGER)
#include <asm/mach-panther/idb.h>
#endif

#undef TKIP_COUNTERMEASURE
static struct mac80211_dragonite_data *wlan_data;
extern struct macaddr_pool maddr_tables;
extern char *curfunc;
//extern struct mat_entry *(mat_hash_tbl[MAT_HASH_TABLE_SIZE]);
int force_vo;
static char buf[300];
#define MAX_ARGV 8
struct seq_file *global_psf;
#if defined(CONFIG_PANTHER_INTERNAL_DEBUGGER)
static int wl_print(struct seq_file *m, const char *fmt, ...)
{
    va_list args;
    int len;
    int r;

    if (!m)
    {
        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);
    }
    else
    {
        if (m->count < m->size)
        {
            va_start(args, fmt);
            len = vsnprintf(m->buf + m->count, m->size - m->count, fmt, args);
            va_end(args);
            if (m->count + len < m->size)
            {
                m->count += len;
                return 0;
            }
        }
        m->count = m->size;
        return -1;
    }
    return r;
}

void idb_dump_buf_32bit(struct seq_file *m, unsigned int *p, u32 s)
{
    int i;
    while ((int)s > 0)
    {
        for (i = 0; i < 4; i++)
        {
            if (i < (int)s/4)
            {
                wl_print(m, "%08X ",p[i]);
            }
            else
            {
                wl_print(m, "         ");
            }
        }
        wl_print(m, "\n");
        s -= 16;
        p += 4;
    }
}

void idb_dump_buf_16bit(struct seq_file *m, unsigned short *p, u32 s)
{
    int i;
    while ((int)s > 0)
    {
        for (i = 0; i < 8; i++)
        {
            if (i < (int)s/2)
            {
                wl_print(m, "%04X ", p[i] );
                if (i == 3) wl_print(m, " ");
            }
            else
            {
                wl_print(m, "     ");
            }
        }
        wl_print(m, "\n");
        s -= 16;
        p += 8;
    }
}

void idb_dump_buf_8bit(struct seq_file *m, unsigned char *p, u32 s)
{
    int i, c;
    while ((int)s > 0)
    {
        for (i = 0; i < 16;  i++)
        {
            if (i < (int)s)
            {
                wl_print(m, "%02X ", p[i] & 0xFF);
            }
            else
            {
                wl_print(m, "   ");
            }
            if (i == 7) wl_print(m, " ");
        }
        wl_print(m, " |");
        for (i = 0; i < 16;  i++)
        {
            if (i < (int)s)
            {
                c = p[i] & 0xFF;
                if ((c < 0x20) || (c >= 0x7F)) c = '.';
            }
            else
            {
                c = ' ';
            }
            wl_print(m, "%c", c);
        }
        wl_print(m, "|\n");
        s -= 16;
        p += 16;
    }
}

void reg_dump(struct seq_file *s, unsigned char *addr,int size)
{
    unsigned char *caddr;

    caddr = addr;
    while (caddr < (addr + size))
    {
        if ((((int)caddr % 16) == 0) || caddr == addr)
            wl_print(s, "\n%08x: ", (unsigned int)caddr);

        wl_print(s, "%08x ", *((volatile unsigned int *) caddr));
        caddr += 4;
    }
}

char *macaddr_ntoa(char *mac)
{
    static char buf[18];
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0] & 0xff, mac[1] & 0xff, mac[2] & 0xff,
            mac[3] & 0xff, mac[4] & 0xff, mac[5] & 0xff);
    return buf;
}

void dump_bcap(struct seq_file *s, int index, char is_ds)
{
    char addr[6];
    int bcap = 0;

    memset(addr, 0, 6);
    wmac_addr_lookup_engine_find(addr, index, &bcap, BY_ADDR_IDX|(is_ds ? IN_DS_TBL:0));

    wl_print(s, "[%d] addr=%s, bcap=%08x\n", index, macaddr_ntoa((char *)&addr), bcap);
}

void dump_sta(struct seq_file *s, MAC_INFO* info, int sta_idx)
{
    sta_cap_tbl *captbl = &(info->sta_cap_tbls[sta_idx]);

    wl_print(s, "sta_idx=%d, 0x%x\n", sta_idx, (unsigned int)captbl);

    wl_print(s, "wep_defkey=%d\n",  captbl->wep_defkey);
    wl_print(s, "cipher_mode=%d\n", captbl->cipher_mode);
    wl_print(s, "bssid=%d\n",       captbl->bssid);
    wl_print(s, "aid=%d\n",         captbl->aid);
    wl_print(s, "addr_index=%d\n",  captbl->addr_index);
}

#if 0
void dump_list(struct seq_file *s, MAC_INFO* info, short head, short tail, short low, short high)
{
    u32 i;
    buf_header *bhdr;
    u8 free[FASTPATH_BUFFER_HEADER_POOL_SIZE+SW_BUFFER_HEADER_POOL_SIZE];

    wl_print(s, "******freelist:%d, %d\n", head, tail);

    while (head != tail)
    {
        bhdr = idx_to_bhdr(info, head);
        wl_print(s, "->[%d]\n", head);
        idb_dump_buf_32bit(s, (unsigned int *)bhdr, 8);

        if ((head < low) || (head >=high))
            wl_print(s, "!!!!!!!!!!!!!!illegal Range!!!!!!!!!!!!\n");
        free[head] = 1;
        head = bhdr->next_index; 
    }
    wl_print(s, "******used:\n");

    for (i = low; i < high; i++)
    {
        if (!free[i])
        {
            wl_print(s, "??[%d]\n", i);
            idb_dump_buf_32bit(s, (unsigned int *)idx_to_bhdr(info, i), 8); 
        }
    }
}

void dump_bts(struct seq_file *s, MAC_INFO* info, int sta_idx)
{
    struct wsta_cb *cb;
    struct bts_cb *bcb;
    struct bts_rate *br;
    u32 i;

    cb = info->wcb_tbls[sta_idx];
    if (cb == 0)
        return;
    bcb = &cb->bts;

    for (i = 0; i < 4; i++)
    {
        wl_print(s, "[%d|%d] ", cb->cur_tx_rate[i].rate_idx, cb->cur_tx_rate[i].bts_rates);
    }
    wl_print(s, "\n[ridx]      TP Prob        TS   success/attemp\n");
    for (i = 0; i < bcb->rates_len; i++)
    {
        br = &bcb->rates[i];
        wl_print(s, "[%2d] %9d  %3d %10d  %4d/%-4d\n", br->rate_idx, br->tp, br->avg_prob, br->timestamp, br->success, br->attempts);
    }

}

void dump_bc_qinfo(struct seq_file *s, MAC_INFO* info)
{
    bcast_qinfo *qinfos = info->bcast_qinfos;
    int i;

    for (i = 0; i < MAC_MAX_BSSIDS; i++)
    {
        wl_print(s, "BSS:%d\n", i);
        idb_dump_buf_16bit(s, (unsigned short *)&qinfos[i], sizeof(bcast_qinfo));
    }
}

void dump_addr_cache(struct seq_file *s, MAC_INFO* info)
{
    int idx, val;
    for (idx = 0; idx < 32; idx++)
    {
        MACREG_WRITE32(0x86c, (idx << 2)|0x1);
        val = MACREG_READ32(0x86c);
        wl_print(s, "[%d], val=%x\n", idx, val);
    }
}

void dump_wcb(struct seq_file *s, MAC_INFO* info, int sta_idx)
{
    struct wsta_cb *cb;

    cb = info->wcb_tbls[sta_idx];

    if (cb == 0)
        return;

    wl_print(s, "WCB:0x%x\n", (unsigned int)cb); 
    idb_dump_buf_8bit(s, (unsigned char *)cb, sizeof(struct wsta_cb));

}

void dump_ba(struct seq_file *s, MAC_INFO* info, int sta_idx)
{
#if defined(WLA_AMPDU_RX_SW_REORDER)
    int j;
    struct wsta_cb *cb;
    struct rx_ba_session *ba;

    cb = info->wcb_tbls[sta_idx];
    if (cb == 0)
        return;
    wl_print(s, "RX:\n");   
    for (j = 0; j < 8; j++)
    {
        ba = (struct rx_ba_session *)cb->ba_rx[j];
        if (ba)
        {
            wl_print(s, "   [%d]ba=%x, size=%d, start=%d, num=%d, head=%x, tail=%x\n", j, ba, ba->win_size, ba->win_start, ba->stored_num, ba->reorder_q_head, ba->reorder_q_tail);
        }
    }
#endif
#if defined(WLA_AMPDU_RX_HW_REORDER)
    int regval, i;
    MACREG_WRITE32(AMPDU_BMG_CTRL, VARIABLE_TABLE_1_CACHE_FLUSH | VARIABLE_TABLE_0_CACHE_FLUSH);    
    for (i = 0; i < MAX_QOS_TIDS; i++)
    {
        if (info->reorder_buf_mapping_tbl[(sta_idx * MAX_QOS_TIDS) + i] != 0)
        {
            wl_print(s, "TID:%d\n", i);
            idb_dump_buf_16bit(s, (unsigned short *)info->reorder_buf_mapping_tbl[(sta_idx * MAX_QOS_TIDS) + i], sizeof(ampdu_reorder_buf));
        }
    }

    wl_print(s, "TX:\n");
    MACREG_WRITE32(PSBA_PS_UCAST_TIM_CR, PS_UCAST_TIM_CMD | sta_idx);   
    while (MACREG_READ32(PSBA_PS_UCAST_TIM_CR) & PS_UCAST_TIM_CMD)
        ;
    regval = MACREG_READ32(PSBA_PS_UCAST_TIM_SR); 
    wl_print(s, "state=%08x\n",  regval);
    wl_print(s, "sta_qinfo_tbls:\n");
    idb_dump_buf_16bit(s, (unsigned short *)&info->sta_qinfo_tbls[sta_idx], sizeof(sta_qinfo_tbl));
    if (info->sta_qinfo_tbls[sta_idx].qinfo_addr)
    {
        ucast_qinfo *qinfos = (ucast_qinfo *)NONCACHED_ADDR(info->sta_qinfo_tbls[sta_idx].qinfo_addr);
        for (i = 0; i < 8; i++)
        {
            wl_print(s, "TID:%d\n", i);
            idb_dump_buf_16bit(s, (unsigned short *)&qinfos[i], sizeof(ucast_qinfo));
        }       
    }
#endif
}
#endif
mm_segment_t oldfs;
void initkernelenv(void)
{ 
    oldfs = get_fs(); 
    set_fs(KERNEL_DS); 
} 

void dinitkernelenv(void)
{
    set_fs(oldfs); 
}

struct file *openfile(char *path,int flag,int mode)
{
    struct file *fp; 

    fp=filp_open(path, flag, 0); 

    if (IS_ERR(fp))
        return NULL;
    else
        return fp; 
}

int writefile(struct file *fp,char *buf,int writelen) 
{ 
    if (fp->f_op && fp->f_op->write)
        return fp->f_op->write(fp, buf, writelen, &fp->f_pos);
    else
        return -1; 
} 

int readfile(struct file *fp, char *buf, int readlen)
{
    if (fp->f_op && fp->f_op->read)
        return fp->f_op->read(fp, buf, readlen, &fp->f_pos);
    else
        return -1;
}

int closefile(struct file *fp) 
{
    filp_close(fp,NULL); 
    return 0; 
}

#ifdef TKIP_COUNTERMEASURE
void dump_ptk(struct seq_file *s, MAC_INFO *info, int cmd)
{
    int i=0, j=0;
    struct wsta_cb *wcb;
    cipher_key *hwkey;
    u8 buf[TKIP_KEY_LEN + TKIP_MICKEY_LEN + TKIP_MICKEY_LEN];
    //int cmd = atoi(argv[1]); /* 0: dump, 1: set rx_mic, 2: set tx_mix */


    for (i = 0; i < info->sta_cap_tbl_count; i++)
    {
        //bss = &(my_ap_dev->bss[i]);
        wcb = info->wcb_tbls[i];

        if (wcb == NULL)
            continue;

        if (wcb->hw_cap_ready == 0)
            continue;

        hwkey = IDX_TO_PRIVATE_KEY(wcb->captbl_idx);

        if (hwkey->tkip_key.cipher_type != CIPHER_TYPE_TKIP)
            continue;

        memcpy(buf, hwkey->tkip_key.key, TKIP_KEY_LEN);
        memcpy(&buf[TKIP_KEY_LEN], hwkey->tkip_key.txmickey,  TKIP_MICKEY_LEN);
        memcpy(&buf[TKIP_KEY_LEN+TKIP_MICKEY_LEN], hwkey->tkip_key.rxmickey,  TKIP_MICKEY_LEN);

        switch (cmd)
        {
        case 1:
            buf[TKIP_KEY_LEN+TKIP_MICKEY_LEN+6] = 0xAA;
            buf[TKIP_KEY_LEN+TKIP_MICKEY_LEN+7] = 0x55;
            wla_cipher_key(wcb, CIPHER_TYPE_TKIP, KEY_TYPE_PAIRWISE_KEY, buf, 0, 0);
            break;
        case 2:
            buf[TKIP_KEY_LEN+6] = 0xAA;
            buf[TKIP_KEY_LEN+7] = 0x55;
            wla_cipher_key(wcb, CIPHER_TYPE_TKIP, KEY_TYPE_PAIRWISE_KEY, buf, 0, 0);
            break;
        case 3:
            buf[0] = 0xAA;
            buf[1] = 0x55;
            wla_cipher_key(wcb, CIPHER_TYPE_TKIP, KEY_TYPE_PAIRWISE_KEY, buf, 0, 0);
            break;
        default:
            wl_print(s, "*** wcb idx =%d ****\n", wcb->captbl_idx);
            wl_print(s, "tk1 = ");
            for (j = 0; j < 16; j++)
            {
                wl_print(s, "%02x", buf[j]);
            }
            wl_print(s, "\n");

            wl_print(s, "tx_mic_key = ");
            for (j = TKIP_KEY_LEN; j < TKIP_KEY_LEN+TKIP_MICKEY_LEN; j++)
            {
                wl_print(s, "%02x", buf[j]);
            }
            wl_print(s, "\n");

            wl_print(s, "rx_mic_key = ");
            for (j = TKIP_KEY_LEN+TKIP_MICKEY_LEN; j < TKIP_KEY_LEN+TKIP_MICKEY_LEN+TKIP_MICKEY_LEN; j++)
            {
                wl_print(s, "%02x", buf[j]);
            }
            wl_print(s, "\n");
            break;
        }
    }
}
#endif // TKIP_COUNTERMEASURE
#if 0
static int cipher_suite_to_bit(const u8 *s)
{
    u32 suite = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];

    if (suite== RSN_CIPHER_SUITE_TKIP)
        return 1 << AUTH_CIPHER_TKIP;
    if (suite == RSN_CIPHER_SUITE_CCMP)
        return 1 << AUTH_CIPHER_CCMP;
    if (suite == WPA_CIPHER_SUITE_TKIP)
        return 1 << AUTH_CIPHER_TKIP;
    if (suite == WPA_CIPHER_SUITE_CCMP)
        return 1 << AUTH_CIPHER_CCMP;

    return 0;
}


int wlan_parse_wpa_rsn_ie(const u8 *start, u32 len, u32 *pcipher, u32 *gcipher)
{
    u8 *pos;
    u32 i, count, left;

    pos = (u8 *) start + 2;
    left = len - 2;

    if (left < SELECTOR_LEN)
        return -1;

    /* get group cipher suite */
    *gcipher |= cipher_suite_to_bit(pos);

    pos += SELECTOR_LEN;
    left -= SELECTOR_LEN;

    /* get pairwise cipher suite list */
    count = (pos[1] << 8) | pos[0];
    pos += 2;
    left -= 2;
    if (count == 0 || left < count * SELECTOR_LEN)
    {
        return -1;
    }
    for (i = 0; i < count; i++)
    {
        *pcipher |= cipher_suite_to_bit(pos);

        pos += SELECTOR_LEN;
        left -= SELECTOR_LEN;
    }

    return 0;
}
#endif

void exec_bb(int argc, char *argv[])
{
    unsigned long value;
    int i;

    if (argc < 2)
    {
        goto Usage;
    }

    if (!strcmp("alldump", argv[1]))
    {
        for (i = 0; i <= 0xff; i++)
        {
            //value = bb_register_read(i);
            value = bb_register_read(0, i);
            wl_print(global_psf, "1-%02x-%02x\n", i, (unsigned int) value);
        }
    }
    else if (!strcmp("agcdump", argv[1]))
    {
        for (i = 0; i < 20; i++)
        {
            //bb_register_write(0x10, i);
            //value = bb_register_read(0x11);
            value = bb_register_read(1, i);
            wl_print(global_psf, "1-10-%02x\n", i);
            wl_print(global_psf, "1-11-%02x\n", (unsigned int)value);
        }
    }
    else if (!strcmp("groupdump", argv[1]))
    {
        int group = 0;
        int val;

        if (argc == 3)
        {
            sscanf(argv[2],"%d", &group);
        }
        if ((group >= 0) && (group <3))
        {
            for (i = 0; i <= 0xff; i++)
            {
                val = bb_register_read(group, i);
                wl_print(global_psf, "%d-%02x-%02x\n",
                         group, i, (unsigned int) val);
            }
        }
    }
    else if (!strcmp("r", argv[1]))
    {
        int reg;
        int val;
        int group;
        if (argc == 4)
        {
            sscanf(argv[2],"%d", &group);
            sscanf(argv[3],"%x", &reg);
            val = bb_register_read(group, reg);
            if(group == 0)
            {
                wl_print(global_psf, "BB%x = %02x\n",
                         (unsigned int)reg, (unsigned int) val);
            }
            else if(group == 1)
            {
                wl_print(global_psf, "BB11 = %02x\n",
                         (unsigned int) val);
            }
            else if(group == 2)
            {
                wl_print(global_psf, "BB13 = %02x\n",
                         (unsigned int) val);
            }
            else if(group == 3)
            {
                wl_print(global_psf, "BB15 = %02x\n",
                         (unsigned int) val);
            }
        }

    }
    else if (!strcmp("w", argv[1]))
    {
        int reg;
        int wval, rval, group;
        if (argc == 5)
        {
            sscanf(argv[2],"%d", &group);
            sscanf(argv[3],"%x", &reg);
            sscanf(argv[4],"%x", &wval);
            bb_register_write(group, reg, wval);
            rval = bb_register_read(group, reg);
            if(group == 0)
            {
                wl_print(global_psf, "SET BB%x = %02x\n",
                         (unsigned int) rval);
            }
            else if(group == 1)
            {
                wl_print(global_psf, "SET BB11 = %02x\n",
                         (unsigned int) rval);
            }
            else if(group == 2)
            {
                wl_print(global_psf, "SET BB13 = %02x\n",
                         (unsigned int) rval);
            }
            else if(group == 3)
            {
                wl_print(global_psf, "SET BB15 = %02x\n",
                         (unsigned int) rval);
            }
        }
    }
    else
    {
        goto Usage;
    }
    return;

Usage:
    wl_print(global_psf, "Usage:\n"
             "\tbb r [group] [reg]\n"
             "\tbb w [group] [reg] [val]\n"
             "\tbb alldump\t(dump all register)\n"
             "\tbb agcdump\t(dump agc table)\n"
             "\tbb groupdump [group]\n");
}

void exec_rf(int argc, char *argv[])
{
    if (argc < 2)
    {
        goto Usage;
    }

    if (!strcmp("r", argv[1]))
    {
        int v;
        if (argc != 3)
            goto Usage;
        sscanf(argv[2], "%x", &v);
        wl_print(global_psf, "read reg %x=0x%x\n", v, rf_read(v));
    }
    else if (!strcmp("w", argv[1]))
    {
        int v, v1;
        if (argc != 4)
            goto Usage;
        sscanf(argv[2], "%x", &v);
        sscanf(argv[3], "%x", &v1);
        wl_print(global_psf, "write reg %d=0x%x\n", v, v1);
        rf_write(v, v1);
    }
    else if (!strcmp("alldump", argv[1]))
    {
        int v;
        for (v = 0; v <= 0x1f; v++)
            wl_print(global_psf, "reg %x=0x%x\n", v, rf_read(v));
    }
    else
    {
        goto Usage;
    }
    return;

Usage:
    wl_print(global_psf, "Usage:\n"
             "\trf r [reg]\n"
             "\trf w [reg] [val]\n"
             "\trf alldump\t(dump all register)\n"
             "\tchan <idx> <bw> 0:none 1:upper 3:below\n"
            );
}

void exec_chan(int argc, char *argv[])
{
    int chan, bw;
    if (argc != 3)
    {
        goto Usage;
    }
    sscanf(argv[1], "%d", &chan);
    sscanf(argv[2], "%d", &bw);

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
    }
    else if (!(chan >= 1 && chan <= 14))
    {
        wl_print(global_psf, "wrong channel\n");
        goto Usage;
    }
    else if (!(bw >= 0 && bw <= 3))
    {
        wl_print(global_psf, "wrong bandwidth\n");
        goto Usage;
    }
    else
    {
        dragonite_set_channel(chan, bw);
    }

    return;
Usage:
    wl_print(global_psf, "Usage:\n"
             "\tchan [channel_num(1~14)] [bw(0~3)]\n"
            );
}

void exec_smrtcfg(int argc, char *argv[])
{
    if (!strcmp("start", argv[1]))
        omnicfg_start();
    else if (!strcmp("stop", argv[1]))
        omnicfg_stop();
    else
    {
        wl_print(global_psf, "Usage:\n"
                 "\tsmrtcfg start\n"
                 "\tsmrtcfg stop\n"
                );
    }
}
void exec_airkiss(int argc, char *argv[])
{
    if (!strcmp("start", argv[1]))
        airkiss_start();
    else if (!strcmp("stop", argv[1]))
        airkiss_stop();
    else
    {
        wl_print(global_psf, "Usage:\n"
                 "\tairkiss start\n"
                 "\tairkiss stop\n"
                );
    }
}

void exec_chaninfo(int argc, char *argv[])
{
    u8 ch, bw;
    dragonite_get_channel(&ch, &bw);
    wl_print(global_psf, "Current channel %d\n"
             "Current bandwidth %d\n",
             ch, bw
            );
}

void exec_bcap(int argc, char *argv[])
{
    int tbl_idx = 0, i;
    char addr[6] = {0};
    MAC_INFO *info = wlan_data->mac_info; 

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    if (argc >= 2)
    {
        sscanf(argv[1], "%d", &tbl_idx);
        if (argc == 3)
        {
            int bcap_val;
            sscanf(argv[2], "%x", &bcap_val);
            wmac_addr_lookup_engine_update(info, addr, tbl_idx, bcap_val, BY_ADDR_IDX);
        }
        dump_bcap(global_psf, tbl_idx, 0);
    }
    else
    {
        /* dump all */
        for (i = 0; i < MAX_STA_NUM; i++)
        {
            dump_bcap(global_psf, i, 0);
            // see resource.c __ext_sta_table[MAX_STA_NUM][10]
            // info->ext_sta_table = UNCACHED(wifisram->__ext_sta_table[MAX_STA_NUM][10])
            idb_dump_buf_16bit(global_psf, (unsigned short *)(info->ext_sta_table + (i * 10)), 10);
        }
    }
}

void exec_umac(int argc, char *argv[])
{
    reg_dump(global_psf, (unsigned char *)(UMAC_REG_BASE), 0x100);
    wl_print(global_psf, "\nTX:");
    reg_dump(global_psf, (unsigned char *)(UMAC_REG_BASE + UMAC_TX_CNTL), 0x100);
    wl_print(global_psf, "\nRX:");
    reg_dump(global_psf, (unsigned char *)(UMAC_REG_BASE + BA_LIFE_TIME_REG), 0x100); /*BA_LIFE_TIME_REG is first reg about RX*/
//    wl_print(global_psf, "\nETH:");
//  reg_dump(global_psf, (unsigned char *)0xaf009800, 0x100);
    wl_print(global_psf, "\nSEC:");
    reg_dump(global_psf, (unsigned char *)(UMAC_REG_BASE + SEC_REG_OFFSET), 0x20);
    wl_print(global_psf, "\n");
}

void exec_lmac(int argc, char *argv[])
{
    reg_dump(global_psf, (unsigned char *)(UMAC_REG_BASE + LMAC_REG_OFFSET), 0x200);
    wl_print(global_psf, "\n");
}

void exec_gkey(int argc, char *argv[])
{
    MAC_INFO *info = wlan_data->mac_info;
    int tbl_idx = 0;
    int size = MAC_MAX_BSSIDS;
#if defined(CONFIG_MAC80211_MESH) && defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
    size = MAC_MAX_BSSIDS + MAC_EXT_BSSIDS;
#endif

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    if (argc == 2)
        sscanf(argv[1],"%d",&tbl_idx);

    if (tbl_idx >= size || tbl_idx < 0)
    {
        wl_print(global_psf, "invalid index[%d]\n", tbl_idx);
        return;
    }

    wl_print(global_psf, "group_keys[%d]\n", tbl_idx);
    idb_dump_buf_32bit(global_psf, (unsigned int *)&info->group_keys[tbl_idx], sizeof(cipher_key));
}

void exec_pkey(int argc, char *argv[])
{
    MAC_INFO *info = wlan_data->mac_info;
    int size = MAC_MAX_BSSIDS;
    int tbl_idx = 0;

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    if (argc == 2)
        sscanf(argv[1],"%d",&tbl_idx);

    if (tbl_idx >= size || tbl_idx < 0)
    {
        wl_print(global_psf, "invalid index[%d]\n", tbl_idx);
        return;
    }

    wl_print(global_psf, "private_keys[%d]\n", tbl_idx);
    idb_dump_buf_32bit(global_psf, (unsigned int *)&info->private_keys[tbl_idx], sizeof(cipher_key));
}

void exec_rxfilter(int argc, char *argv[])
{

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    if (!strcmp("get", argv[1]))
        wl_print(global_psf, "rx_filter %d\n", wlan_data->rx_filter);
    else if (!strcmp("set", argv[1]) && argc == 3)
        sscanf(argv[2],"%d",&wlan_data->rx_filter);
    else
    {
        wl_print(global_psf, "Usage:\n"
                 "\trxfilter set [num]\n"
                 "\trxfilter get\n"
                );
    }
}

void exec_sta(int argc, char *argv[])
{
    MAC_INFO *info = wlan_data->mac_info;
    int tbl_idx = 0;

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    if (argc == 2)
    {
        sscanf(argv[1],"%d",&tbl_idx);
        if(tbl_idx >= info->sta_idx_msb)
        {
            wl_print(global_psf, "Invalid index (0~%d)\n", info->sta_idx_msb);
            return;
        }
        dump_sta(global_psf, info, tbl_idx);
    }
    else
    {
        for (; tbl_idx < info->sta_idx_msb; tbl_idx++)
        {
            dump_sta(global_psf, info, tbl_idx);
        }
    }
}

void exec_vif(int argc, char *argv[])
{
    struct dragonite_vif_priv *vp;
    struct __bss_info *bss_info;
    int idx;

    /* see definition of role in mac.h */
    char *role[6] = { "NONE", "AP", "STA", "IBSS", "P2PC", "P2PGO"};

    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }

    for (idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(wlan_data, (u8) idx);
        if (vp == NULL)
        {
            continue;
        }
        wl_print(global_psf, "vif_priv:\nidx\tmagic\tbssid\n");
        bss_info = vp->bssinfo;
        wl_print(global_psf, "%d\t%d\t%s\n",
                 idx,
                 vp->magic,
                 vp->bssid
                );
        wl_print(global_psf, "bss_info:\nrole\tcipher_mode\tmac_addr\tassociate_ssid\n");

        wl_print(global_psf, "%s\t%d\t%s",
                 role[bss_info->dut_role],
                 bss_info->cipher_mode,
                 bss_info->MAC_ADDR
                 );
        if (bss_info->dut_role == ROLE_STA)
        {
            wl_print(global_psf, "\t%s", bss_info->associated_bssid);
        }
        wl_print(global_psf, "\n");
    }
}

void exec_rxdesc(int argc, char *argv[])
{
    MAC_INFO *info = wlan_data->mac_info;
    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }
    wl_print(global_psf, "Start Address:%p rx_descr_count=%d End Address:%x\n",
             info->rx_descriptors, info->rx_descr_count,
             (unsigned int)(info->rx_descriptors + info->rx_descr_count));
    idb_dump_buf_32bit(global_psf, (unsigned int *)info->rx_descriptors,
                       info->rx_descr_count * sizeof(rx_descriptor));
}

void exec_txdesc(int argc, char *argv[])
{

}

/* request for rx phy info, will print target match */
extern int dragonite_rx_phy_info_dump;
void exec_rxphyinfo(int argc, char *argv[])
{
    if (!wlan_data->started)
    {
        wl_print(global_psf, "WLAN not started yet\n");
        return;
    }
    dragonite_rx_phy_info_dump = 1;
}

void exec_rxphyinfo_off(int argc, char *argv[])
{
    dragonite_rx_phy_info_dump = 0;
}

extern int dragonite_force_txrate_idx;
void exec_txrate(int argc, char *argv[])
{
    int idx;

    if(argc != 2)
        return;

    sscanf(argv[1], "%d", &idx);
    if(idx >= 0 && idx <= 20)
        dragonite_force_txrate_idx = idx;
}

extern int dragonite_force_disable_sniffer_function;
void exec_sniffer_off(int argc, char *argv[])
{
    int disable;

    if(argc != 2)
        return;

    sscanf(argv[1], "%d", &disable);
    if(disable)
    {
        wl_print(global_psf, "disable sniffer function\n");
        dragonite_force_disable_sniffer_function = 1;
    }
    else
    {
        wl_print(global_psf, "enable sniffer function\n");
        dragonite_force_disable_sniffer_function = 0;
    }
}

#if defined(BB_NOTCH_FILTER_TEST)
extern int bb_notch_filter_test_enable;
#endif
void exec_bbtest(int argc, char *argv[])
{
    int enable;

    if(argc != 2)
        return;

    sscanf(argv[1], "%d", &enable);
    if(enable)
    {
        wl_print(global_psf, "enable notch filter\n");
#if defined(BB_NOTCH_FILTER_TEST)
        bb_notch_filter_test_enable = 1;
#endif
    }
    else
    {
        wl_print(global_psf, "disable notch filter\n");
#if defined(BB_NOTCH_FILTER_TEST)
        bb_notch_filter_test_enable = 0;
#endif
    }
}

typedef void (*exec_proc_t)(int argc, char *argv[]);

typedef struct _debugfs_proc_t
{
    const char *name;
    const char *help;
    exec_proc_t handler;
} debugfs_proc_t;

static debugfs_proc_t proc_table[] =
{
    { "bb",         "\tbb\t\tregister read/write/dump\n",           exec_bb},
    { "rf",         "\trf\t\tregister read/write/dump\n",           exec_rf},
    { "chan",       "\tchan\t\tnumber bandwidth change\n",          exec_chan},
    { "chaninfo",   "\tchaninfo\tshow channel and bandwidth\n",     exec_chaninfo},
    { "bcap",       "\tbcap\t\tdump bcap\n",                        exec_bcap},
    { "umac",       "\tumac\t\tdump umac\n",                        exec_umac},
    { "lmac",       "\tlmac\t\tdump lmac\n",                        exec_lmac},
    { "gkey",       "\tgkey\t\tdump group keys\n",                  exec_gkey},
    { "pkey",       "\tpkey\t\tdump private keys\n",                exec_pkey},
    { "rxfilter",   "\trxfilter\tset/get\n",                        exec_rxfilter},
    { "sta",        "\tsta\t\tdump info about station\t\n",         exec_sta},
    { "vif",        "\tvif\t\tdump info about virtual interface\n", exec_vif},
    { "rxdesc",     "\trxdesc\t\tdump info about rx descriptor\n",  exec_rxdesc},
    { "txdesc",     "\ttxdesc\t\tdump info about tx descriptor\n",  exec_txdesc},
    { "rxphyinfo",  "\trxphyinfo\tdump phy rssi/snr/rate\n",        exec_rxphyinfo},
    { "rxphyinfo_off","\trxphyinfo_off\tdisable dump info\n",       exec_rxphyinfo_off},
    { "txrate",     "\ttxrate\t\tset tx rate idx\n",                exec_txrate},
    { "bbtest",     "\tbbtest\t\tset notch filter\n",               exec_bbtest},
    { "sniffer_off","\tsniffer_off\tdisable sniffer function\n",    exec_sniffer_off},
#if defined(DRAGONITE_ENABLE_OMNICFG)
    { "smrtcfg",    NULL,                                   exec_smrtcfg},
#endif
#if defined(DRAGONITE_ENABLE_AIRKISS)
    { "airkiss",    NULL,                                   exec_airkiss},
#endif
};

static void print_help(struct seq_file *s)
{
    int i = 0;
    debugfs_proc_t *item;
    wl_print(s, "Parm:\n");
    for (i = 0; i < ARRAY_SIZE(proc_table); ++i)
    {
        item = &proc_table[i];
        if (item->help != NULL)
        {
            wl_print(s, item->help);
        }
    }
    return;
}

static int wifidump_func(int argc, char *argv[])
{
    struct seq_file *s = global_psf;
    int i;

    if (1 > argc)
    {
        print_help(s);
        return 0;
    }

    for (i = 0; i < ARRAY_SIZE(proc_table); ++i)
    {
        debugfs_proc_t *item;
        item = &proc_table[i];
        if (!strcmp(argv[0], item->name))
        {
            item->handler(argc, argv);
            break;
        }
    }

    if (i >= ARRAY_SIZE(proc_table))
    {
        print_help(s);
    }

    return 0;
}

extern int get_args (const char *string, char *argvs[]);
struct idb_command idb_wifidump_cmd =
{
    .cmdline = "wd",
    .help_msg = "wd                         Dump WiFi informations", 
    .func = wifidump_func,
};
#endif

#if defined(CONFIG_PANTHER_INTERNAL_DEBUGGER)
static int wl_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    memset(buf, 0, 300);

    if (count > 0 && count < 299)
    {
        if (copy_from_user(buf, buffer, count))
            return -EFAULT;
        buf[count-1]='\0';
    }

    return count;
}

static int wl_show(struct seq_file *s, void *priv)
{
#if 0
    MAC_INFO *info = WLA_MAC_INFO;
    struct wla_softc *sc = info->sc;
#endif
    int rc;
    int argc ;
    char * argv[MAX_ARGV] ;

    //sc->seq_file = s;
    global_psf = s;
    argc = get_args( (const char *)buf , argv );
    rc=wifidump_func(argc, argv);
    //sc->seq_file = NULL;
    global_psf = NULL;

    return 0;
}

static int wl_open(struct inode *inode, struct file *file)
{
    int ret;

    ret = single_open(file, wl_show, NULL);

    return ret;
}
static const struct file_operations wl_fops = {
    .open       = wl_open,
    .read       = seq_read,
    .write      = wl_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};
#endif

int wla_debug_init(void *wla_sc)
{
#if defined(CONFIG_PANTHER_INTERNAL_DEBUGGER)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
    struct proc_dir_entry *res;
#endif

    register_idb_command(&idb_wifidump_cmd);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    if (!proc_create("wd", S_IWUSR | S_IRUGO, NULL, &wl_fops))
        return -EIO;

#else
    res = create_proc_entry("wd", S_IWUSR | S_IRUGO, NULL);
    if (!res)
        return -ENOMEM;

    res->proc_fops = &wl_fops;
#endif
#endif

    wlan_data = wla_sc;

    return 0;
}

void wla_debug_exit(void)
{
#if defined(CONFIG_PANTHER_INTERNAL_DEBUGGER)
    unregister_idb_command(&idb_wifidump_cmd);
    remove_proc_entry("wd",NULL);
#endif
}
