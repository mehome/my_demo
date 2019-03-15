#include <dragonite_main.h>
#include <net/mac80211.h>
#include <mac_ctrl.h>
#include <tx.h>

#include "dragonite_common.h"
#include "mac_regs.h"

void dragonite_handle_power_saving(struct mac80211_dragonite_data *data)
{
	MAC_INFO* info = data->mac_info;
	struct dragonite_sta_priv *sp;
	acq *q;
	unsigned long splock_irqflags;
	u32 ps_raw_stat_bm, ps_stat_bm, bss_ps_stat, handle_ps_bm;
	int i, j;

	ps_raw_stat_bm = MACREG_READ32(PS_RAW_STAT_BM);
	ps_stat_bm = MACREG_READ32(PS_STAT_BM);
	bss_ps_stat = MACREG_READ32(BSS_PS_STAT);

	dragonite_ps_lock();

	handle_ps_bm = ps_stat_bm - (ps_raw_stat_bm & info->sta_ps_on);

	if(ps_stat_bm)/* ps status bitmap be changed or anyone ps before */
	{
		for(i = 0; i < info->sta_idx_msb; i++)
		{
			if(handle_ps_bm & (0x1 << i))/* this station ps status change */
			{
				info->sta_ps_recover &= ~(0x1 << i);

				if(ps_stat_bm & (0x1 << i))/* this station ps on */
				{
					info->sta_ps_off_indicate &= ~(0x1 << i);

					sp = idx_to_sp(data, (u8)i);
					if(sp==NULL) {
						continue;
					}

					if(info->sta_ps_on & (0x1 << i))
					{
						/* ps on handle before */
					}
					else
					{
						info->sta_ps_on |= (0x1 << i);

						DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "station no.%d ps on\n", i);

						dragonite_tx_swq_to_psq(data, i);

						dragonite_tx_ps_buffer_adjust(data, i);

						spin_lock_irqsave(&sp->splock, splock_irqflags);

						if(IS_AMPDU(sp))
						{
							for(j = 0; j < TID_MAX; j++)
							{
								if((q = SINGLE_Q(sp, j)) != NULL)
								{
									tx_acq_detach_async(info, q);
								}
							}
						}
						spin_unlock_irqrestore(&sp->splock, splock_irqflags);			
					}
				}
				if(!(ps_raw_stat_bm & (0x1 << i)))/* this station current ps off */
				{
					if(ps_stat_bm & (0x1 << i))
					{
						info->sta_ps_off_indicate |= (0x1 << i);
					}
				}
			}
		}
	}
	dragonite_ps_unlock();

	dragonite_bss_ps_lock();

	for(i = 0; i < MAX_BSSIDS; i++)
	{
		if(bss_ps_stat & (0x1 << i))
		{
			info->bss_ps_status[i] = DRG_BSS_PS_ON;

			if(info->bcq_status[i] == DRG_BCQ_DISABLE)
			{
				info->bcq_status[i] = DRG_BCQ_ENABLE;

				DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "bss no.%d enable to use bcq\n", i);

				dragonite_tx_bmcq_to_bcq(data, i);
			}
		}
	}
	dragonite_bss_ps_unlock();
}

void dragonite_handle_power_saving_off(struct mac80211_dragonite_data *data)
{
    MAC_INFO* info = data->mac_info;
	tx_descriptor *descr;
    volatile struct list_head *lptr;
    struct dragonite_tx_descr *dragonite_descr;
    struct dragonite_sta_priv *sp;
    acq *q;
    int sta_index , bss_index, tid, qid;
    bool acq_busy;
    u32 ps_stat_bm, ps_raw_stat_bm, current_bss_ps_stat;
    unsigned long splock_irqflags;
    u16 send_bar_start_ssn[TID_MAX];

    dragonite_ps_lock();

    if(info->sta_ps_off_indicate) {

        for(sta_index = 0; sta_index < info->sta_idx_msb; sta_index++) {

            acq_busy = false;
            if(info->sta_ps_off_indicate & (0x1 << sta_index)) {
                sp = idx_to_sp(data, (u8) sta_index);
                if(sp==NULL) {
                    info->sta_ps_off_indicate &= ~(0x1 << sta_index);
                    continue;
                }
                if(sp->tx_hw_pending_skb_count>0)
                {
                    /* tx not done complete, reject ps off */
                    DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "tx not done complete, reject ps off %d\n", sp->tx_hw_pending_skb_count);
                    acq_busy = true;
                }

                /* check single queue at request status or not*/
                spin_lock_irqsave(&sp->splock, splock_irqflags);
                if(IS_AMPDU(sp))
                {
                    for(tid = 0; tid < TID_MAX; tid++) {

                        if((q = SINGLE_Q(sp, tid)) != NULL) {/* means ampdu action now */

                            if(tx_acq_cmd_req(info, q))
                            {
                                tx_acq_detach_async(info, q);/* detach agagin */

                                acq_busy = true;

                                break;
                            }
                        }
                    }
                }
                spin_unlock_irqrestore(&sp->splock, splock_irqflags);
                if(acq_busy) {
                    /* ps off not completed */
                    DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "acq busy, reject ps off\n");
                }
                else
                {
                    info->sta_ps_off_indicate &= ~(0x1 << sta_index);
                    ps_stat_bm = ~(0x1 << sta_index); /* mask all and write 0 clear then update PS_STAT_BM*/
                    DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "station no.%d try to ps off\n", sta_index);
                    MACREG_WRITE32(PS_STAT_BM, ps_stat_bm);
                    ps_raw_stat_bm = MACREG_READ32(PS_RAW_STAT_BM);
                    ps_stat_bm = MACREG_READ32(PS_STAT_BM);
                    if(ps_stat_bm & (0x1 << sta_index)) {
                        continue;
                    }
                    DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "station no.%d ps off\n", sta_index);
                    /* TODO: move ps q to normal q(include mixed q and single q), and re-schedule it */
                    dragonite_tx_swq_to_gpq(data, sta_index);

                    memset((void *) send_bar_start_ssn, INVALID_SSN, sizeof(u16) * TID_MAX);

                    /* clean up single q entry table status */

                    spin_lock_irqsave(&sp->splock, splock_irqflags);
                    if(IS_AMPDU(sp))
                    {
                        for(tid = 0; tid < TID_MAX; tid++) {

                            if((q = SINGLE_Q(sp, tid)) != NULL) {/* means ampdu action now */
           
                                memset((void *)&q->acqe[0], 0, ACQE_SIZE * q->queue_size);
                                memset((void *)&q->acqe[q->queue_size], 0, ACQE_SIZE * q->queue_size);

                                if(sp->tx_sw_single_queue[tid].qhead != NULL)
                                {
                                    lptr = sp->tx_sw_single_queue[tid].qhead;
                                    dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
                                    descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);

                                    if(descr->sn != PREV_AGGR_SSN(sp, tid))
                                    {
                                        send_bar_start_ssn[tid] = descr->sn;
                                    }
                                    tx_single_acq_start(q, descr->sn);
                                }
                                else
                                {    
                                    if(AGGR_SSN(sp, tid) != PREV_AGGR_SSN(sp, tid)) {

                                        send_bar_start_ssn[tid] = AGGR_SSN(sp, tid);
                                    }
                                    tx_single_acq_start(q, AGGR_SSN(sp, tid));
                                }
                            }
                        }
                    }
                    spin_unlock_irqrestore(&sp->splock, splock_irqflags);
                    info->sta_ps_on &= ~(0x1 << sta_index);

                    info->sta_ps_poll_request &= ~(0x1 << sta_index);

                    for(tid = 0; tid < TID_MAX; tid++) {

                        if(!IS_INVALID_SSN(send_bar_start_ssn[tid]))
                        {
                            dragonite_tx_send_ba_request(data, sta_index, tid, send_bar_start_ssn[tid]);
                        }

                        if(sp->tx_sw_single_queue[tid].qhead != NULL) {
                            dragonite_tx_schedule(data, NOT_USE_VIF_IDX, sta_index, tid);
                        }
                    }
                    for(qid = 0; qid < ACQ_NUM - 1; qid++){
                        if(data->tx_sw_mixed_queue[qid].qhead != NULL) {
                            dragonite_tx_schedule(data, NOT_USE_VIF_IDX, -1, qid);
                        }
                    }
                }
            }
        }
    }
    dragonite_ps_unlock();
    dragonite_bss_ps_lock();
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
    dragonite_pm_lock_lock();
    if(!data->poweroff)
        current_bss_ps_stat = MACREG_READ32(BSS_PS_STAT);
    else
	current_bss_ps_stat = 0;
    dragonite_pm_lock_unlock();
#else
    current_bss_ps_stat = MACREG_READ32(BSS_PS_STAT);
#endif
    for(bss_index = 0; bss_index < MAX_BSSIDS; bss_index++) {
        if(info->sta_ps_recover)
        {
            // not to handle bcq when sta ps unknown
            break;
        }

        if(current_bss_ps_stat & (0x1 << bss_index))
        {
        }
        else
        {
            info->bss_ps_status[bss_index] = DRG_BSS_PS_OFF;
        }
    }
    dragonite_bss_ps_unlock();
}

int dragonite_add_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif)
{
    char flag;
    int index;
    int addr_index;
    MAC_INFO* info = data->mac_info;
    bool sta_mode = dragonite_vif_is_sta_mode(vif);
    sta_basic_info bcap;
    int i;
    struct dragonite_vif_priv *vp = (void *) vif->drv_priv;
    struct dragonite_sta_priv *sp = (void *) sta->drv_priv;
    sta_cap_tbl* captbl;
    int ret = 0;

    bcap.val = 0;
    flag = 0;

    dragonite_mac_lock();

    if(sta_mode)
    {
        index = wmac_addr_lookup_engine_find(sta->addr, (int)&addr_index, 0, flag | IN_DS_TBL);
    }
    else
    {
        index = wmac_addr_lookup_engine_find(sta->addr, (int)&addr_index, 0, flag);
    }

    if (index >= 0)/*find the duplicate, ignore*/
    {
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "duplicated MAC address\n");
        ret = -EEXIST;
        goto exit;
    }

    index = 0;
    for(i=0;i<32;i++)//find the available addr index
    {
        if(!(info->sta_idx_bitmap & (0x01UL << i)))
        {
            index = i;
            break;
        }
    }

    if(i != index)
    {
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "lookup engine is full !!\n");
        ret = -EIO;
        goto exit;
    }

    memset(sp->tx_sw_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));
    memset(sp->tx_sw_single_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));
    memset(sp->tx_hw_single_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));
    memset(sp->tx_sw_ps_single_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));

    if(sta_mode)
    {
        sp->sta_mode = true;
    }
    else
    {
        sp->sta_mode = false;
    }
    bcap.bit.vld = 1;
    bcap.bit.bssid = vp->bssid_idx;

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    vp->tx_pm_bit = 0;
#endif
    vp->ts_err_mute = false;
    vp->ts_err_count = 0;

    sp->stacap_index = index;
    captbl = &info->sta_cap_tbls[index];
    captbl->bssid = vp->bssid_idx;
    captbl->aid = sta->aid;

    sp->rekey_id = 0;

    //printk("BSS INDEX = %d\n", vp->bssid_idx);

    if((vp->bssinfo->cipher_mode == CIPHER_TYPE_WEP40) || (vp->bssinfo->cipher_mode == CIPHER_TYPE_WEP104))
    {
        DRG_DBG("wep encryption\n");
        bcap.bit.wep_defkey = 1;
        bcap.bit.cipher = 1;

        captbl->wep_defkey = 1;
        captbl->cipher_mode = vp->bssinfo->cipher_mode;
        //vp->bssinfo->wep_defkey_refcount++;
    }
    else if(vp->bssinfo->cipher_mode != 0)
    {
        DRG_DBG("tkip or ccmp encryption\n");
        //bcap.bit.cipher = 1;
        captbl->cipher_mode = vp->bssinfo->cipher_mode;
    }

    if(sta_mode)
    {
        addr_index = wmac_addr_lookup_engine_update(info, sta->addr, 0, (bcap.val << 8) | index, flag | IN_DS_TBL);
    }
    else
    {
        addr_index = wmac_addr_lookup_engine_update(info, sta->addr, 0, (bcap.val << 8) | index, flag);
    }
    sp->addr_index = addr_index;
    memcpy(sp->mac_addr, sta->addr, ETH_ALEN);
    captbl->addr_index = addr_index;
    sp->sta_binfo.val = (bcap.val << 8) | index;
    info->sta_idx_bitmap |= (0x01UL << index);
    data->sta_priv_addr[index] = (u32) sp;

    for(i = 0; i < MAX_STA_NUM; i++)
    {
        if(info->sta_idx_bitmap & (0x01UL << (31 - i)))
        {
            info->sta_idx_msb = MAX_STA_NUM - i;
            break;
        }
    }

    MACREG_UPDATE32(BSS0_STA_BITMAP + (vp->bssid_idx << 2), (0x01UL << index), (0x01UL << index));

    sp->ssn = 0;

    spin_lock_init(&sp->splock);

    if((info->sta_ps_on | info->sta_ps_off_indicate | info->sta_ps_poll_request | info->sta_ps_recover) & (0x01UL << index))
    {
	DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "station has dirty ps status\n");
    }

    dragonite_mac_unlock();

    return ret;

exit:
    dragonite_mac_unlock();

    return ret;
}
int dragonite_remove_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif)
{
    char flag;
    int index;
    int addr_index;
    MAC_INFO* info = data->mac_info;
    bool sta_mode = dragonite_vif_is_sta_mode(vif);
    struct dragonite_vif_priv *vp = (void *) vif->drv_priv;
    struct dragonite_sta_priv *sp = (void *) sta->drv_priv;
    sta_cap_tbl* captbl;
    u32 ps_stat_bm;
    int sta_idx, ret = 0;

    flag = 0;

    dragonite_mac_lock();

    if(sta_mode)
    {

        index = wmac_addr_lookup_engine_find(sta->addr, (int)&addr_index, 0, flag | IN_DS_TBL);
    }
    else
    {
        index = wmac_addr_lookup_engine_find(sta->addr, (int)&addr_index, 0, flag);
    }

    if (index < 0)/*can't find it*/
    {
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "remove station fail, can not find in lookup engine\n");
        ret = -EIO;
        goto exit;
    }

    /* remove all pending q from this station */
    dragonite_tx_detach_node(data, sp);

    data->sta_priv_addr[index] = 0;

    captbl = &info->sta_cap_tbls[index];

    memset(captbl, 0, sizeof(sta_cap_tbl));

    MACREG_UPDATE32(BSS0_STA_BITMAP + (vp->bssid_idx << 2), 0, (0x01UL << index));

    if(sta_mode)
    {
        wmac_addr_lookup_engine_update(info, sta->addr, 0, 0, flag | IN_DS_TBL);
    }
    else
    {
        wmac_addr_lookup_engine_update(info, sta->addr, 0, 0, flag);
    }

    info->sta_idx_bitmap &= ~(0x01UL << index);

    for(sta_idx = 0; sta_idx < MAX_STA_NUM; sta_idx++)
    {
        if(info->sta_idx_bitmap & (0x01UL << ((MAX_STA_NUM - 1) - sta_idx)))
        {
            info->sta_idx_msb = MAX_STA_NUM - sta_idx;
            break;
        }
    }

    dragonite_mac_unlock();

    dragonite_ps_lock();

    ps_stat_bm = ~(0x01UL << index); /* mask all and write 0 clear then update PS_STAT_BM*/

    MACREG_WRITE32(PS_STAT_BM, ps_stat_bm);

    info->sta_ps_on &= ~(0x01UL << index);
    info->sta_ps_off_indicate &= ~(0x01UL << index);
    info->sta_ps_poll_request &= ~(0x01UL << index);
    info->sta_ps_recover &= ~(0x1UL << index);

    dragonite_ps_unlock();

    return ret;

exit:
    dragonite_mac_unlock();

    return ret;
}
int dragonite_change_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif)
{
	return 0;
}

void dragonite_recover_station(struct mac80211_dragonite_data *data, u8 sta_idx)
{
    char flag;
    int addr_index, tid;
    MAC_INFO *info = data->mac_info;
    struct dragonite_sta_priv *sp;
    sta_cap_tbl *captbl;
    sta_basic_info bcap;

    flag = 0;
    bcap.val = 0;

    dragonite_mac_lock();

    ASSERT((sta_idx < MAX_STA_NUM), "dragonite_recover_station: sta_idx is out of range\n");
    if(!(info->sta_idx_bitmap & (0x1UL << sta_idx)))
    {
        goto EXIT;
    }

    sp = idx_to_sp(data, sta_idx);
    captbl = &(info->sta_cap_tbls[sta_idx]);
    ASSERT((sp != NULL), "dragonite_recover_station: sp is NULL\n");
    ASSERT((captbl != NULL), "dragonite_recover_station: captbl is NULL\n");

    bcap.val = (sp->sta_binfo.val) >> 8;

    for(tid = 0; tid < MAX_QOS_TIDS; tid++)
    {
	if(bcap.field.rx_ampdu_bitmap & (0x1 << tid))
	{
            mac_invalidate_ampdu_scoreboard_cache(sp->stacap_index, tid);
            while(MACREG_READ32(AMPDU_BMG_CTRL) & BMG_CLEAR)
                DRG_DBG("waiting on scoreboard flush, STA index %d, TID %d\n", sp->stacap_index, tid);
	}
    }
    if(sp->sta_mode)
    {
        addr_index = wmac_addr_lookup_engine_update(info, sp->mac_addr, 0, sp->sta_binfo.val, flag | IN_DS_TBL);
    }
    else
    {
        addr_index = wmac_addr_lookup_engine_update(info, sp->mac_addr, 0, sp->sta_binfo.val, flag);
    }

    MACREG_UPDATE32(BSS0_STA_BITMAP + ((captbl->bssid) << 2), (0x01UL << (sp->stacap_index)),
                    (0x01UL << (sp->stacap_index)));

EXIT:
    dragonite_mac_unlock();

    return;
}
