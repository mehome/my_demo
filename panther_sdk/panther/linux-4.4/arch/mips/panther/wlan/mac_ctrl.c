#include <linux/jiffies.h>
#include "mac_ctrl.h"
#include "mac_regs.h"
#include "tx.h"
#include "panther_debug.h"
#include "performance.h"

int wifi_hw_supported = 0;
/* proper lock mechanism from caller is presumed */
void mac_invalidate_ampdu_scoreboard_cache(int sta_index, int tid)
{
	DRG_DBG("mac_invalidate_ampdu_scoreboard_cache index %d tid %d\n", sta_index, tid);
	MACREG_WRITE32(AMPDU_BMG_CTRL, (BMG_CLEAR | (tid << 8) | (sta_index)));
}

int wmac_addr_lookup_engine_find(u8 *addr, int addr_index, int *basic_cap, char flag)
{
	u32 val, cmd;

	cmd = LU_TRIGGER;
	if (!(flag & IN_DS_TBL))
		cmd |= LU_CMD_SEL_STA_TABLE;

	if (flag & BY_ADDR_IDX) {
		cmd |= (( addr_index << 5 ) & 0x00001FE0UL )|LU_CMD_READ_BY_INDEX;
	} else {
		MACREG_WRITE32(LUT_ADDR0, (addr[0] << 8) | (addr[1]));
		MACREG_WRITE32(LUT_ADDR1, (addr[2] << 24) | (addr[3]  << 16) | (addr[4] << 8) | (addr[5]));
		cmd |= LU_CMD_SEARCH_BY_ADDR;
	}

	MACREG_WRITE32(LUT_CMD, cmd);
	WMAC_WAIT_FOREVER((0 == (MACREG_READ32(LUT_CMD) & LU_DONE)),
					"wmac_addr_lookup_engine_find: waiting idle\n");

	val = MACREG_READ32(LUR_INDX_ADDR);

	if (!(val & LUR_INDX_ADDR_CMD_SUCCEED))
		return LOOKUP_ERR;

	if (flag & BY_ADDR_IDX) {
		if (addr) {
			addr[0] = (u8) ((val & 0x0000FF00UL) >> 8);
			addr[1] = (u8) (val & 0x000000FFUL);

			val = MACREG_READ32(LUR_ADDR);
			addr[2] = (u8) ((val & 0xFF000000UL) >> 24);
			addr[3] = (u8) ((val & 0x00FF0000UL) >> 16);
			addr[4] = (u8) ((val & 0x0000FF00UL) >> 8);
			addr[5] = (u8) (val & 0x000000FFUL);
		}
	} else {
		u32 mask;
		if (flag & IN_DS_TBL) {
			if ((val & LUR_INDX_ADDR_DS_HIT) == 0)
				return LOOKUP_WRONG_TBL;
			else
				mask = 0x000f0000;
		} else {
			if ((val & LUR_INDX_ADDR_STA_HIT) == 0)
				return LOOKUP_WRONG_TBL;
			else
				mask = 0x00ff0000;
		}

		if (addr_index) {
			int *addr_p = (int *)addr_index;
			*addr_p = ((val & mask) >> 16);
		}
	}

	val = MACREG_READ32(LUR_BASIC_CAP);
	if (basic_cap)
		*basic_cap = val;

	return (val & 0x000000FFUL);
}

/* return the hash index */
int wmac_addr_lookup_engine_update(MAC_INFO *info, u8 *addr, int addr_index, u32 basic_cap, u8 flag)
{
	u32 val, cmd;
	unsigned char ci_cmd;

	cmd = LU_TRIGGER;

	if (flag & IN_DS_TBL) {
		/* check DS index range */
		if (flag & BY_ADDR_IDX) {
			if (addr_index >= MAX_DS_NUM)
				return -1;
		}
	} else {
		/* check STA index range */
		if (flag & BY_ADDR_IDX) {
			if(addr_index >= MAX_STA_NUM)
				return -1;
		}
		cmd |= LU_CMD_SEL_STA_TABLE;
	}

	MACREG_WRITE32(LUBASIC_CAP, basic_cap);

	/* add or delete basic capbility by mac address */
	if (flag & BY_ADDR_IDX) {
		/* non-value addr means update or delete basic capbility by address index */
		if (!basic_cap) {
			MACREG_WRITE32(LUT_ADDR0, 0);
			MACREG_WRITE32(LUT_ADDR1, 0);
		} else {
			u8 raddr[6];
			if (wmac_addr_lookup_engine_find(raddr, addr_index, 0, flag) < 0) {
				return LOOKUP_ERR;
			}
			MACREG_WRITE32(LUT_ADDR0, (raddr[0] << 8) | (raddr[1]));
			MACREG_WRITE32(LUT_ADDR1, (raddr[2] << 24) | (raddr[3]  << 16) | (raddr[4] << 8) | (raddr[5]));
		}

		if (flag & IN_DS_TBL)
			cmd |= ((addr_index << 5)| LU_CMD_UPDATE_DS_BY_INDEX);
		else
			cmd |= ((addr_index << 5)| LU_CMD_UPDATE_STA_BY_INDEX);
	} else {
		MACREG_WRITE32(LUT_ADDR0, (addr[0] << 8) | (addr[1]));
		MACREG_WRITE32(LUT_ADDR1, (addr[2] << 24) | (addr[3]  << 16) | (addr[4] << 8) | (addr[5]));
		if (flag & IN_DS_TBL)
			cmd |= (basic_cap ? LU_CMD_INSERT_INTO_DS_BY_ADDR : LU_CMD_UPDATE_INTO_DS_BY_ADDR);
		else
			cmd |= (basic_cap ? LU_CMD_INSERT_INTO_STA_BY_ADDR : LU_CMD_UPDATE_INTO_STA_BY_ADDR);
	}

	MACREG_WRITE32(LUT_CMD, cmd);

	WMAC_WAIT_FOREVER((0 == (MACREG_READ32(LUT_CMD) & LU_DONE)),
					"wmac_addr_lookup_engine_update: waiting idle\n");

	val = MACREG_READ32(LUR_INDX_ADDR);

	if (val & LUR_INDX_ADDR_CMD_SUCCEED) {
		val = ((val & 0x00FF0000) >> 16);
		/* CHEETAH needs to update station's total number */
		ci_cmd = cmd & LU_CMD_MASK;
		if (basic_cap) {
			if (ci_cmd == LU_CMD_INSERT_INTO_STA_BY_ADDR)
				info->sta_tbl_count++;
			else if (ci_cmd == LU_CMD_INSERT_INTO_DS_BY_ADDR)
				info->ds_tbl_count++;
		} else {
			if (flag & IN_DS_TBL)
				info->ds_tbl_count--;
			else
				info->sta_tbl_count--;
		}
	} else {
		val = -1;
	}

	/* Don't adjust  STA_DS_TABLE_CFG dynamically to avoid cache has incorrect value */
	MACREG_UPDATE32(STA_DS_TABLE_CFG, info->sta_tbl_count|((info->ds_tbl_count & 0x1f) << 9), STA_DS_TABLE_MAX_STA_SEARCH|STA_DS_TABLE_MAX_DS_SEARCH);

	return val;
}

int wmac_addr_lookup_engine_flush(MAC_INFO *info)
{

	WLA_DBG(INFO, "flush_mac_addr_lookup_engine\n");

	/* flush external sta/ds tables */
	MACREG_UPDATE32(STA_DS_TABLE_CFG, STA_DS_TABLE_DS_CLR|STA_DS_TABLE_STA_CLR,
							STA_DS_TABLE_DS_CLR|STA_DS_TABLE_STA_CLR|
							STA_DS_TABLE_MAX_STA_SEARCH|STA_DS_TABLE_MAX_DS_SEARCH);

	/* flush lookup entries */
	MACREG_WRITE32(LUT_CMD, LU_TRIGGER| LU_FLUSH_STA_TABLE);
	/* polling done bit */
	WMAC_WAIT_FOREVER((0 == (MACREG_READ32(LUT_CMD) & LU_DONE)),
					"wmac_addr_lookup_engine_flush(1): waiting idle\n");

	MACREG_WRITE32(LUT_CMD, LU_TRIGGER| LU_FLUSH_DS_TABLE);
	/* polling done bit */
	WMAC_WAIT_FOREVER((0 == (MACREG_READ32(LUT_CMD) & LU_DONE)),
					"wmac_addr_lookup_engine_flush(2): waiting idle\n");

#if 0
	if (info->ext_sta_tbls)
		memset(info->ext_sta_tbls, 0, sizeof(sta_basic_info)*MAX_ADDR_TABLE_ENTRIES);
	if (info->ext_ds_tbls)
		memset(info->ext_ds_tbls, 0, sizeof(sta_basic_info)*MAX_DS_TABLE_ENTRIES);
	info->sta_tbl_count = info->ds_tbl_count = 0;
#endif

	return 0;
}

int mac_addr_lookup_engine_flush(void)
{
    unsigned long last_jiffies;
	int ret = 0;

    DRG_DBG("flush_mac_addr_lookup_engine\n");

    /* flush lookup entries */
    MACREG_WRITE32(LUT_CMD, LU_TRIGGER| LU_FLUSH_STA_TABLE);

    last_jiffies = jiffies;
    while(LU_DONE != (MACREG_READ32(LUT_CMD) & LU_DONE))
    {
        if(time_after(jiffies, (last_jiffies + HZ/20)))
        {
            DRG_DBG("flush STA table failed\n");
			ret = -ENODEV;
            break;
        }
    }

    MACREG_WRITE32(LUT_CMD, LU_TRIGGER| LU_FLUSH_DS_TABLE);

    last_jiffies = jiffies;
    while(LU_DONE != (MACREG_READ32(LUT_CMD) & LU_DONE))
    {
        if(time_after(jiffies, (last_jiffies + HZ/20)))
        {
            DRG_DBG("flush DS table failed\n");
			ret = -ENODEV;
            break;
        }
    }

    DRG_DBG("flush_mac_addr_lookup_engine...Done.\n");

    return ret;
}

int mac_program_addr_lookup_engine(MAC_INFO* info)
{
    DRG_DBG("mac_program_addr_lookup_engine\n");

    if(0==MACREG_READ32(EXT_STA_TABLE_ADDR))
        MACREG_WRITE32(EXT_STA_TABLE_ADDR, PHYSICAL_ADDR(info->ext_sta_table));

    if(0==MACREG_READ32(EXT_DS_TABLE_ADDR))
        MACREG_WRITE32(EXT_DS_TABLE_ADDR, PHYSICAL_ADDR(info->ext_ds_table));

    MACREG_UPDATE32(STA_DS_TABLE_CFG, STA_DS_TABLE_CFG_DONE, STA_DS_TABLE_CFG_DONE);

    /* set max sta/ds table max. search */
    MACREG_UPDATE32(STA_DS_TABLE_CFG, 0, STA_DS_TABLE_MAX_DS_SEARCH | STA_DS_TABLE_MAX_STA_SEARCH);

    MACREG_UPDATE32(STA_DS_TABLE_CFG, (0x1f << 16), STA_DS_TABLE_HASH_MASK);
    MACREG_UPDATE32(STA_DS_TABLE_CFG, (0x01a << 24), STA_DS_TABLE_INT_STA_NO); //STA 27 AP 4

    return 0;
}

int mac_cw_value_to_hw_encoding(int cw_val)
{
	int encoding;

	if(cw_val>=1023)
		encoding = 10;
	else if(cw_val>=511)
		encoding = 9;
	else if(cw_val>=255)
		encoding = 8;
	else if(cw_val>=127)
		encoding = 7;
	else if(cw_val>=63)
		encoding = 6;
	else if(cw_val>=31)
		encoding = 5;
	else if(cw_val>=15)
		encoding = 4;
	else if(cw_val>=7)
		encoding = 3;
	else if(cw_val>=3)
		encoding = 2;
	else if(cw_val>=1)
		encoding = 1;
	else
		encoding = 0;

	return encoding;
}
void mac_apply_ac_parameters(int ac, int cw_min, int cw_max, int aifs, int txop)
{
        if(ac == AC_BK_QID)
        {
                if((cw_min >= 0) && (cw_max >= 0))
                {
                        MACREG_UPDATE32(CW_SET, (mac_cw_value_to_hw_encoding(cw_max) << 4) | (mac_cw_value_to_hw_encoding(cw_min)),
                                                CW_SET_BK_CWMIN | CW_SET_BK_CWMAX);
                }

                if(aifs >= 0)
                {
                        MACREG_UPDATE32(AIFS_SET, (aifs << 16), AIFS_SET_AIFSN_BK);
                }

                if(txop >= 0)
                {
                        MACREG_UPDATE32(TXOP_LIMIT, (txop), TXOP_LIMIT_BK);
                }
        }
        else if(ac == AC_BE_QID)
        {
                if((cw_min >= 0) && (cw_max >= 0))
                {
                        MACREG_UPDATE32(CW_SET, (mac_cw_value_to_hw_encoding(cw_max) << 12) | (mac_cw_value_to_hw_encoding(cw_min) << 8),
                                                CW_SET_BE_CWMIN | CW_SET_BE_CWMAX);
                }

                if(aifs >= 0)
                {
                        MACREG_UPDATE32(AIFS_SET, (aifs << 20), AIFS_SET_AIFSN_BE);
                }

                if(txop >= 0)
                {
                        MACREG_UPDATE32(TXOP_LIMIT, (txop << 8), TXOP_LIMIT_BE);
                }
        }
        else if(ac == AC_VI_QID)
        {
                if((cw_min >= 0) && (cw_max >= 0))
                {
                        MACREG_UPDATE32(CW_SET, (mac_cw_value_to_hw_encoding(cw_max) << 20) | (mac_cw_value_to_hw_encoding(cw_min) << 16),
                                                CW_SET_VI_CWMIN | CW_SET_VI_CWMAX);
                }

                if(aifs >= 0)
                {
                        MACREG_UPDATE32(AIFS_SET, (aifs << 24), AIFS_SET_AIFSN_VI);
                }

                if(txop >= 0)
                {
                        MACREG_UPDATE32(TXOP_LIMIT, (txop << 16), TXOP_LIMIT_VI);
                }
        }
        else if(ac == AC_VO_QID)
        {
                if((cw_min >= 0) && (cw_max >= 0))
                {
                        MACREG_UPDATE32(CW_SET, (mac_cw_value_to_hw_encoding(cw_max) << 28) | (mac_cw_value_to_hw_encoding(cw_min) << 24),
                                                        CW_SET_VO_CWMIN | CW_SET_VO_CWMAX);
                }

                if(aifs >= 0)
                {
                        MACREG_UPDATE32(AIFS_SET, (aifs << 28), AIFS_SET_AIFSN_VO);
                }

                if(txop >= 0)
                {
                        MACREG_UPDATE32(TXOP_LIMIT, (txop << 24), TXOP_LIMIT_VO);
                }
        }
}
void mac_apply_default_wmm_ap_parameters(void)
{
	printk(KERN_EMERG "DRAGONITE: CONFIG EDCA PARAMETERS TO AP MODE\n");
    mac_apply_ac_parameters(AC_VO_QID, 3,     7,  1,  47);
    mac_apply_ac_parameters(AC_VI_QID, 7,    15,  1,  94);
    mac_apply_ac_parameters(AC_BE_QID, 15,   63,  3,   0);
    mac_apply_ac_parameters(AC_BK_QID, 15, 1023,  7,   0);

    MACREG_UPDATE32(AIFS_SET, 0x2, AIFS_SET_AIFSN_LEGACY);
}

void mac_apply_default_wmm_station_parameters(void)
{
	printk(KERN_EMERG "DRAGONITE: CONFIG EDCA PARAMETERS TO STATION MODE\n");
    mac_apply_ac_parameters(AC_VO_QID, 3,     7,  2,  47);
    mac_apply_ac_parameters(AC_VI_QID, 7,    15,  2,  94);
    mac_apply_ac_parameters(AC_BE_QID, 15, 1023,  3,   0);
    mac_apply_ac_parameters(AC_BK_QID, 15, 1023,  7,   0);

    MACREG_UPDATE32(AIFS_SET, 0x2, AIFS_SET_AIFSN_LEGACY);
}

int program_mac_registers(MAC_INFO* info)
{
    MACREG_WRITE32(SEC_GPKEY_BA, PHYSICAL_ADDR(info->group_keys));
    MACREG_WRITE32(SEC_PVKEY_BA, PHYSICAL_ADDR(info->private_keys));

    MACREG_WRITE32(SLOTTIME_SET, 0x0205);

    MACREG_WRITE32(RXDSC_BADDR, PHYSICAL_ADDR(info->rx_descriptors));

    /* Set WIFI/Ethernet MAX Length to Maximum */
    MACREG_WRITE32(MAX_WIFI_LEN,0xffff);
    MACREG_WRITE32(MAX_ETHER_LEN,0xffff);

    MACREG_WRITE32(RX_MPDU_MAXSIZE, 4096);

    MACREG_WRITE32(SWBL_BADDR, PHYSICAL_ADDR(info->buf_headers));

    MACREG_WRITE32(SWB_HT, (((u32) 0x0000 << 16)|((u16)info->rx_buffer_hdr_count - 1)));

    MACREG_WRITE32(RX_BLOCK_SIZE, (info->sw_path_bufsize << 16 ) | info->fastpath_bufsize);
    MACREG_WRITE32(BUFF_POOL_CNTL, 1);

    /* Set LLC offset to 80 bytes to prevent wifi header corruptted by packet SN, RSC */
    MACREG_WRITE32(MAC_LLC_OFFSET, 0x50);

    MACREG_WRITE32(RX_ACK_POLICY, RX_ACK_POLICY_AMPDU_BA|RX_PLCP_LEN_CHK|RX_AMPDU_REORDER_ENABLE);

    MACREG_WRITE32(ERR_EN, 0xf3);  // Enable WIFI RX fast path 

    MACREG_UPDATE32(BEACON_SETTING2, SW_CONTROL_SENDBAR_RATE , SW_CONTROL_SENDBAR_RATE); //Enable SW decide rate of BAR packet

    MACREG_UPDATE32(RTSCTS_SET, LMAC_RTS_CTS_THRESHOLD, LMAC_RTS_CTS_THRESHOLD);

    MACREG_UPDATE32(SECENG_CTRL, SECENG_CTRL_REKEY_ERR_DROP, SECENG_CTRL_GKEY_RSC_ERR_DROP|SECENG_CTRL_KEYID_ERR_DROP|SECENG_CTRL_REKEY_ERR_DROP);

    MACREG_UPDATE32(SW_DES_NO, PKT_DES_NO, PKT_DES_NO); // Switch rx pkt descr to length 40 for bb team debug

    MACREG_UPDATE32(TS_REG_TIMER_ON, REG_TIMER_ON, REG_TIMER_ON); // enable tsf even lmac timer disable

    panther_throughput_config();

    return 0;
}
int dragonite_mac_tx_rx_enable(void)
{

    MACREG_WRITE32(UMAC_TX_CNTL, (((0x4 << 4) & UMAC_TX_CNTL_TXINFO_LEN)
                                | UMAC_TX_CNTL_LU_CACHE_EN | UMAC_TX_CNTL_TX_ENABLE));

    MACREG_WRITE32(UPRX_EN, 1);

    return 0;
}
int dragonite_mac_tx_rx_disable(void)
{
    /* Do not close UPRX_EN, it will casue unstable network */
    //MACREG_WRITE32(UPRX_EN, 0);

    MACREG_UPDATE32(UMAC_TX_CNTL, 0, UMAC_TX_CNTL_LU_CACHE_EN | UMAC_TX_CNTL_TX_ENABLE);

    return 0;
}
int dragonite_mac_start(MAC_INFO* info)
{
    /* XXX: set fragment threshold, move somewhere else later */
    // MACREG_UPDATE32(TXFRAG_CNTL, 100, 0x0000ffffUL);

	DRG_DBG("Mac start\n");

    /* enable LMAC timer*/
    MACREG_UPDATE32(LMAC_CNTL, 0x1, LMAC_CNTL_TSTART);

    enable_mac_interrupts();

    return 0;
}

int dragonite_mac_stop(MAC_INFO* info)
{
    disable_mac_interrupts();

    MACREG_UPDATE32(LMAC_CNTL, 0x0, LMAC_CNTL_TSTART);

	return 0;
}
int enable_mac_interrupts(void)
{
    MACREG_WRITE32(MAC_INT_MASK_REG, ~(MAC_INT_TSF | MAC_INT_SW_RX | MAC_INT_RX_DESCR_FULL |
									   MAC_INT_ACQ_TX_DONE | MAC_INT_ACQ_CMD_SWITCH | MAC_INT_ACQ_RETRY_FAIL));

    return 0;
}
int disable_mac_interrupts(void)
{
    MACREG_WRITE32(MAC_INT_MASK_REG, MAC_INT_FULL_MASK);           

    return 0;
}
void mac_program_bssid(u8 *macaddr, int bss_idx, int role, int timer_index)
{
    u32 reg_val = 0;
    u32 reg2_val;

    switch (role)
    {
       case ROLE_STA:
           reg_val = (BSSID_INFO_EN |  BSSID_INFO_MODE_STA);
           break;
       case ROLE_AP:
           reg_val = (BSSID_INFO_EN |  BSSID_INFO_MODE_AP);
           break;
       case ROLE_IBSS:
           reg_val = (BSSID_INFO_EN |  BSSID_INFO_MODE_IBSS);
           break;
       case ROLE_P2PC:
           reg_val = (BSSID_INFO_EN |  BSSID_INFO_MODE_STA | BSSID_INFO_P2P);
           break;
       case ROLE_P2PGO:
           reg_val = (BSSID_INFO_EN |  BSSID_INFO_MODE_AP | BSSID_INFO_P2P);
           break;
       default:
           DRG_DBG("BSS(%d) not enabled\n", bss_idx);
           reg_val = 0;
           break;
    }

    reg_val |= (((timer_index << 24) & BSSID_INFO_TIMER_INDEX) | ((macaddr[0] << 8) | macaddr[1]));
    reg2_val = ((macaddr[2] << 24) | (macaddr[3] << 16) | (macaddr[4] << 8) | macaddr[5]);

    if (bss_idx==0)
    {
        MACREG_WRITE32(BSSID0_INFO2, reg2_val);
        MACREG_WRITE32(BSSID0_INFO, reg_val);
    }
    else if (bss_idx==1)
    {
        MACREG_WRITE32(BSSID1_INFO2, reg2_val);
        MACREG_WRITE32(BSSID1_INFO, reg_val);
    }
    else if (bss_idx==2)
    {
        MACREG_WRITE32(BSSID2_INFO2, reg2_val);
        MACREG_WRITE32(BSSID2_INFO, reg_val);
    }
    else if (bss_idx==3) {
        MACREG_WRITE32(BSSID3_INFO2, reg2_val);
        MACREG_WRITE32(BSSID3_INFO, reg_val);
    }
}

int reset_mac_registers(void)
{
    if(0 == mac_addr_lookup_engine_flush())
	{
		wifi_hw_supported = 1;
	}

    MACREG_WRITE32(MAC_INT_MASK_REG, 0x0000ffff);       /* mask all interrupts */
    MACREG_WRITE32(MAC_INT_CLR, 0x0000ffff);       /* clear all interrupt status  */

    return 0;
}

void mac_rekey_start(int index, int type)
{
    int key_ctrl = 0;

    key_ctrl = (((index << 16) & SEC_KEY_CTRL_STA_IDX) | SEC_KEY_CTRL_REKEY_REQ );

    if(type == REKEY_TYPE_GKEY) {
        key_ctrl |= SEC_KEY_CTRL_STA_IDX_GLOBAL;
    }

    MACREG_WRITE32(SEC_KEY_CTRL, key_ctrl);
}

void mac_rekey_done()
{
    MACREG_WRITE32(SEC_KEY_CTRL, 0);
}
