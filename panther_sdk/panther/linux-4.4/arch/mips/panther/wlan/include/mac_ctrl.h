/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*!
*   \file mac_ctrl.h
*   \brief
*   \author Montage
*/

#ifndef __MAC_CTRL_H__
#define __MAC_CTRL_H__

#include "panther_debug.h"
#include "mac.h"

#define TX_DESC_HEADROOM 	144	/* no head romm require (in sram : headroom + wtb + frame) */

#define MAC_ACK_POLICY_NORMAL_ACK			0
#define MAC_ACK_POLICY_NOACK				1

enum {
	BW40MHZ_SCN = 0,	/* no secondary channel is present */
	BW40MHZ_SCA = 1,	/* secondary channel is above the primary channel */
	BW40MHZ_SCB = 3,	/* secondary channel is below the primary channel */
	BW40MHZ_AUTO = 4,	/* auto select secondary channel */
};

/* channel offset */
#define CH_OFFSET_20		0
#define CH_OFFSET_40		1
#define CH_OFFSET_20U		2
#define CH_OFFSET_20L		3

#define WMAC_WAIT_FOREVER(con, msg) \
	do { \
		unsigned int cnt = 0; \
		while ((con)) { \
			cnt++; \
			if (cnt == 0xffffffff) { \
				break; \
			} else if(cnt == 0xffff) { \
				DRG_DBG(msg"?????????????System maybe enter Busy Loop????????????\n"); \
			} \
		} \
		if (cnt >= 0xffff) DRG_DBG("Loop stop, cnt=%d\n", cnt); \
	} while(0)

/* address lookup options */
#define BY_ADDR_IDX			0x1
#define IN_DS_TBL			0x2
/* address lookup error code */
#define LOOKUP_ERR			(-1)
#define LOOKUP_WRONG_TBL	(-2)

void mac_invalidate_ampdu_scoreboard_cache(int sta_index, int tid);
int wmac_addr_lookup_engine_find(u8 *addr, int addr_index, int *basic_cap, char flag);
int wmac_addr_lookup_engine_update(MAC_INFO *info, u8 *addr, int addr_index, u32 basic_cap, u8 flag);
int wmac_addr_lookup_engine_flush(MAC_INFO *info);
int mac_program_addr_lookup_engine(MAC_INFO* info);

void mac_rekey_start(int index, int type);
void mac_rekey_done(void);

int enable_mac_interrupts(void);
int disable_mac_interrupts(void);

int dragonite_mac_start(MAC_INFO* info);
int dragonite_mac_stop(MAC_INFO* info);

int dragonite_mac_tx_rx_enable(void);
int dragonite_mac_tx_rx_disable(void);

int program_mac_registers(MAC_INFO* info);
void mac_program_bssid(u8 *macaddr, int bss_idx, int role, int timer_index);

void mac_apply_default_wmm_ap_parameters(void);
void mac_apply_default_wmm_station_parameters(void);

int reset_mac_registers(void);
#endif // __MAC_CTRL_H__

