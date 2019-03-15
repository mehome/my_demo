#include <dragonite_main.h>
#include <linux/skbuff.h>
#include <skb.h>
#include <net/mac80211.h>
#include <linux/etherdevice.h>
#include <mac_ctrl.h>
#include <tx.h>

#include "panther_debug.h"
#include "dragonite_common.h"

const u8 broadcast_addr[ETH_ALEN] =	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#if defined (DRAGONITE_TX_SKB_CHECKER)
static DEFINE_SPINLOCK(drg_tx_skb_lock);
#endif
static inline int DRAGONITE_SEQ_TO_SN(__le16 seq)
{
#if defined(__BIG_ENDIAN)
    return ((seq & 0xF000) >> 12) + ((seq & 0x00FF) << 4);
#else
	return (seq & IEEE80211_SCTL_SEQ) >> 4;
#endif
}
static inline int DRAGONITE_SN_TO_SEQ(u16 sn)
{
#if defined(__BIG_ENDIAN)
    return ((sn & 0xF) << 12) + ((sn & 0xFF0) >> 4);
#else
	return (sn & (IEEE80211_SCTL_SEQ >> 4)) << 4;
#endif
}
static inline int DRAGONITE_GET_AID(__le16 aid)
{
#if defined(__BIG_ENDIAN)
    return ((aid & 0xFF00) >> 8) + ((aid & 0x0003) << 8);
#else
	return aid & 0x03FF;
#endif
}
static void *dragonite_parse_beacon_get_tim(struct sk_buff *skb, int *tim_len)
{
	/* Check whether the Beacon frame has DTIM indicating buffered bc/mc */
	struct ieee80211_mgmt *mgmt;
	u8 *pos, *end, id, elen;
	struct ieee80211_tim_ie *tim;

	mgmt = (struct ieee80211_mgmt *)skb->data;
	pos = mgmt->u.beacon.variable;
	end = skb->data + skb->len;

	while (pos + 2 < end) {
		id = *pos++;
		elen = *pos++;
		if (pos + elen > end)
			break;

		if (id == WLAN_EID_TIM) {
			if (elen < sizeof(*tim))
				break;
			tim = (struct ieee80211_tim_ie *) pos;
            *tim_len = elen;
			return tim;
		}

		pos += elen;
	}

	return NULL;
}
void dragonite_dump_tx_skb(struct sk_buff *skb)
{
    int l;
    int dump_len;
	int rx_dump_maxlen = 60;
    unsigned char *pdata = skb->data;

    dump_len = (skb->len > rx_dump_maxlen) ? rx_dump_maxlen : skb->len;
    printk("                       %s TX (%d)\n                          ", "WiFi", skb->len);
    
    for(l=0;l<dump_len;l++) 
    {
            printk("%02x ", pdata[l]);
            if((l%16)==15) 
                    printk("\n                          ");
    }
    printk("\n");
}
void dragonite_dump_rx_skb(struct sk_buff *skb)
{
    int l;
    int dump_len;
        int rx_dump_maxlen = 60;
    unsigned char *pdata = skb->data;

    dump_len = (skb->len > rx_dump_maxlen) ? rx_dump_maxlen : skb->len;
    printk("                       %s RX (%d)\n                          ", "WiFi", skb->len);

    for(l=0;l<dump_len;l++)
    {
            printk("%02x ", pdata[l]);
            if((l%16)==15)
                    printk("\n                          ");
    }
    printk("\n");
}
/* return nonzero value on parsing error */
/* please reset/preset skbinfo.flags before calling the function */
int dragonite_parse_rx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo)
{
    MAC_INFO* info = data->mac_info;
	struct ieee80211_hdr *wifi_hdr = (struct ieee80211_hdr *) skb->data;
    struct ieee80211_pspoll *pspoll;
    char flag;
    int index;
    int addr_index;
    int hdrlen;
    __le16 fc;
    u8 *payload;
    u16 ethertype;
    unsigned long irqflags;
    u16 aid;
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    struct dragonite_vif_priv *vp;
    int idx;
    struct ieee80211_mgmt *mgmt;
    int baselen;
    struct ieee80211_tim_ie *tim;
    int tim_len;
    bool aid0_hit, aid_hit;
#endif
    flag = 0;
    index = 0;
    irqflags = 0;

    if(skb->len < 10)
    {
        /* Should not happen; just a sanity check for addr1 use */
        return -EINVAL;
    }

    hdrlen = ieee80211_hdrlen(wifi_hdr->frame_control);
    if(hdrlen == 0)
        return -EINVAL;
    fc = wifi_hdr->frame_control;

    skbinfo->header_length = hdrlen;

    if(is_multicast_ether_addr(wifi_hdr->addr1))
    {
        skbinfo->flags |= SKB_INFO_FLAGS_MULTICAST_FRAME;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        if(ether_addr_equal(wifi_hdr->addr1, broadcast_addr))
#else
        if(0==compare_ether_addr(wifi_hdr->addr1, broadcast_addr))
#endif
        {
            skbinfo->flags |= SKB_INFO_FLAGS_BROADCAST_FRAME;
        }
    }

    if(ieee80211_is_data(fc))
    {
        skbinfo->flags |= SKB_INFO_FLAGS_DATA_FRAME;

        if(ieee80211_is_nullfunc(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_NULL_FUNCTION;
        }
        else
        {
            if(skb->len >= (hdrlen + sizeof(rfc1042_header) + 2))
            {
                payload = (u8 *) &skb->data[hdrlen];

                if(0==memcmp(payload, rfc1042_header, sizeof(rfc1042_header)))
                {
                    ethertype = ((payload[6] << 8) | (payload[7]));

#if defined(CONFIG_WLA_WAPI)
                    if((ethertype == ETH_P_PAE) || (ethertype == ETH_P_WAPI_PAE))
#else
                    if(ethertype == ETH_P_PAE)
#endif
                    {
                        skbinfo->flags |= SKB_INFO_FLAGS_PAE_FRAME;
                    }
                }
            }
        }
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
        if(is_multicast_ether_addr(wifi_hdr->addr1))
        {
            dragonite_pm_lock_lock();
            if(data->ps_flags & PS_WAIT_FOR_MULTICAST)
            {
                for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
                {
                    vp = idx_to_vp(data, idx);
                    if(!vp)
                        continue;
                    if(vp->bssinfo->dut_role == ROLE_STA)
                    {
                        if(ether_addr_equal(wifi_hdr->addr2, vp->bssinfo->associated_bssid))
                        {
                            DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "MATCH MULTICAST\n");
                            if(!ieee80211_has_moredata(fc))
                            {
                                DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "MATCH MULTICAST NO MORE DATA\n");
                                data->ps_flags &= ~PS_WAIT_FOR_MULTICAST;
                            }
                        }
                        break;
                    }
                }
            }
            dragonite_pm_lock_unlock();
        }
#endif
        if(is_multicast_ether_addr(wifi_hdr->addr3))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_DA_IS_MULTICAST;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
            if(ether_addr_equal(wifi_hdr->addr3, broadcast_addr))
#else
            if(0==compare_ether_addr(wifi_hdr->addr3, broadcast_addr))
#endif
            {
                skbinfo->flags |= SKB_INFO_FLAGS_DA_IS_BROADCAST;
            }
        }
        else
        {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
            if(ether_addr_equal(wifi_hdr->addr3, wifi_hdr->addr1))
#else
            if(0==compare_ether_addr(wifi_hdr->addr3, wifi_hdr->addr1))
#endif
            {
                skbinfo->flags |= SKB_INFO_FLAGS_DA_IS_BSSID;
            }
        }

        if(ieee80211_has_a4(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_WDS_FRAME;
        }

        if(ieee80211_has_protected(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_PROTECTED_FRAME;
        }
    }
    else
    {
        if(ieee80211_is_beacon(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_BEACON_FRAME;
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
            dragonite_pm_lock_lock();
            if(data->ps_flags & PS_WAIT_FOR_BEACON)
            {
                for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
                {
                    vp = idx_to_vp(data, idx);
                    if(!vp)
                        continue;
                    if(vp->bssinfo->dut_role == ROLE_STA)
                    {
                        mgmt = (struct ieee80211_mgmt *) skb->data;
                        baselen = (u8 *) mgmt->u.beacon.variable - (u8 *) mgmt;
                        /* check size */
                        if(baselen > skb->len)
                            break;
                        /* compare bssid */
                        if(ether_addr_equal(mgmt->bssid, vp->bssinfo->associated_bssid))
                        {
                            DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "GET BEACON\n");
                            tim = (struct ieee80211_tim_ie *) dragonite_parse_beacon_get_tim(skb, &tim_len);

                            if(!tim)
                            {
                                break;
                            }
                            /* check aid0 and aid */
                            aid_hit = ieee80211_check_tim(tim, tim_len, vp->aid);

                            if(tim->dtim_count == 0)
                            {
                                aid0_hit = (tim->bitmap_ctrl & 0x01);
                            }
                            else
                            {
                                aid0_hit = false;
                            }

                            if((aid0_hit == false) && (aid_hit == false))
                            {
                                data->ps_flags &= ~PS_WAIT_FOR_BEACON;
                            }
                            else if((aid0_hit == true) && (aid_hit == false))
                            {
                                DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "WAIT MULTICAST\n");
                                data->ps_flags &= ~PS_WAIT_FOR_BEACON;
                                data->ps_flags |= PS_WAIT_FOR_MULTICAST;
                            }
                            else
                            {
                                DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "WAIT UNICAST\n");
                                data->ps_flags &= ~PS_WAIT_FOR_BEACON;
                                data->ps_flags |= PS_FORCE_WAKE_UP;
                            }
                        }
                        break;
                    }
                }
            }
            dragonite_pm_lock_unlock();
#endif
        }

        if(ieee80211_is_action(fc))
            skbinfo->flags |= SKB_INFO_FLAGS_ACTION_FRAME;

        if(ieee80211_is_pspoll(fc))
        {
            if(ieee80211_has_pm(wifi_hdr->frame_control))
            {
                /* check station in PS mode or not, if true then get aid, then config pspoll bitmap */
                pspoll = (struct ieee80211_pspoll *) skb->data;
                aid = DRAGONITE_GET_AID(pspoll->aid);
                if(aid > IEEE80211_MAX_AID)
                {
                    return -EINVAL;
                }

                dragonite_mac_lock();

                index = wmac_addr_lookup_engine_find(pspoll->ta, (int)&addr_index, 0, flag);

                dragonite_mac_unlock();

                if(index < 0) /* can not find out this station */
                {
                    return -EINVAL;
                }
                if(info->sta_cap_tbls[index].aid != aid)
                {
                    DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "receive ps poll, aid not match, ps-poll aid = %04x, sta_cap aid = %04x\n", 
                           aid, info->sta_cap_tbls[index].aid);
                    return -EINVAL;
                }
                info->sta_ps_poll_request |= (0x1 << index);
                DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "Trigger station %d ps-poll\n", index);
            }
        }
    }

    return 0;
}

extern int dragonite_dynamic_ps;
#if defined(CONFIG_MAC80211_MESH) || defined(DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
int dragonite_parse_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo, struct ieee80211_vif *vif)
#else
int dragonite_parse_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo)
#endif
{
    struct ieee80211_hdr *wifi_hdr = (struct ieee80211_hdr *) skb->data;

    int hdrlen;
    u8 *payload;
    u16 ethertype = 0;
    __le16 fc;
    u8 *qc;
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
#endif

    hdrlen = ieee80211_hdrlen(wifi_hdr->frame_control);

    if(hdrlen == 0)
        return -EINVAL;

    skbinfo->ssn = DRAGONITE_SEQ_TO_SN(wifi_hdr->seq_ctrl);
    //printk("parse seq %04x\n", skbinfo->ssn);

    fc = wifi_hdr->frame_control;

    skbinfo->header_length = hdrlen;

    skbinfo->tid = EDCA_LEGACY_FRAME_TID;

    /* The generic rule is that only non-EAPOL non-NULL DATA packet need encryption */
    if(ieee80211_is_data(fc))
    {
        skbinfo->flags |= SKB_INFO_FLAGS_DATA_FRAME;

        if(ieee80211_is_nullfunc(fc))
        {
            skbinfo->tid = EDCA_HIGHEST_PRIORITY_TID;
            skbinfo->flags |= SKB_INFO_FLAGS_FORCE_SECURITY_OFF;
        }
        else
        {
            /* We are searching for EAPOL frames, and judge if it need to be encrypted or not */
            if(skb->len >= (hdrlen + sizeof(rfc1042_header) + 2))
            {
                payload = (u8 *) &skb->data[hdrlen];

                if(0==memcmp(payload, rfc1042_header, sizeof(rfc1042_header)))
                {
                    ethertype = ((payload[6] << 8) | (payload[7]));

#if defined(CONFIG_WLA_WAPI)
                    if((ethertype == ETH_P_PAE) || (ethertype == ETH_P_WAPI_PAE))
#else
                    if(ethertype == ETH_P_PAE)
#endif
                    {
                        skbinfo->flags |= SKB_INFO_FLAGS_PAE_FRAME;
                        skbinfo->flags |= SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION;

                        skbinfo->flags |= SKB_INFO_FLAGS_SPECIAL_DATA_FRAME;

                        /* some eapol packet might need encrypt (e.g. group key negotiation after pairwair key setup) */
                        /* we honor the protected frame bit to judge if encryption is needed */
                        if(!ieee80211_has_protected(fc))
                        {    

                            skbinfo->flags |= SKB_INFO_FLAGS_FORCE_SECURITY_OFF;
                        }
                        else
                        {
                            /* remove the protected bit, as HW will add it back if it is really encrypted */
                            wifi_hdr->frame_control &= ~cpu_to_le16(IEEE80211_FCTL_PROTECTED);
                        }
                    }
                    else if(ethertype == ETH_P_ARP)
                    {
                        skbinfo->flags |= SKB_INFO_FLAGS_SPECIAL_DATA_FRAME;
                    }

                }
            }
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
#if defined(CONFIG_WLA_WAPI)
            if((ethertype != ETH_P_PAE) && (ethertype != ETH_P_WAPI_PAE))
#else
            if(ethertype != ETH_P_PAE)
#endif
            {
                dragonite_pm_lock_lock();
                if(dragonite_dynamic_ps)
                {
                    if(vp->tx_pm_bit)
                    {
                        if(time_is_before_jiffies(vp->tx_pm_bit_prev_jiffies + msecs_to_jiffies(10 * 1000)))
                        {
                            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Tx pm bit retain more than 10s\n");
                        }
                        DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "Tx attach pm bit\n");
                        fc |= cpu_to_le16(IEEE80211_FCTL_PM);
                        wifi_hdr->frame_control = fc;
                        skbinfo->flags |= SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION;
                    }
                }
                dragonite_pm_lock_unlock();
            }
#endif
        }

        if(ieee80211_is_data_qos(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_QOS_FRAME;

            qc = ieee80211_get_qos_ctl(wifi_hdr);
            if(skbinfo->flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME)
            {
                /* replace with tid 7 */
                skb->queue_mapping = IEEE80211_AC_VO;
                skb->priority = EDCA_HIGHEST_PRIORITY_TID;
                qc[0] = 0x0007;
                skbinfo->tid = EDCA_HIGHEST_PRIORITY_TID;
            }
            else
            {
                skbinfo->tid = qc[0] & IEEE80211_QOS_CTL_TID_MASK;      
            }
        }
        else
        {
            if(skbinfo->flags & SKB_INFO_FLAGS_PAE_FRAME)
            {
                /* use highest priority for PAE packet (legacy only; we are not changing Q-DATA priority) */
                skbinfo->tid = EDCA_HIGHEST_PRIORITY_TID;
            }
            skbinfo->flags |= SKB_INFO_FLAGS_USE_GLOBAL_SEQUENCE_NUMBER;
            skbinfo->flags |= SKB_INFO_FLAGS_NO_BA_SESSION;
        }
    }
    else if(ieee80211_is_mgmt(fc))
    {
        skbinfo->tid = EDCA_HIGHEST_PRIORITY_TID;
        #if 0 /* TODO : change this solution */
        if(ieee80211_is_action(fc) || ieee80211_is_disassoc(fc) 
           || ieee80211_is_deauth(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_BUFFERABLE_MGMT_FRAME;
        }
        #else
        if(ieee80211_is_action(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_BUFFERABLE_MGMT_FRAME;
        }
        #endif
        else
        {
            skbinfo->flags |= SKB_INFO_FLAGS_MGMT_FRAME;
        }

        /* indicate hardware to fill timestamp */
        if(ieee80211_is_probe_resp(fc))
        {
            skbinfo->flags |= SKB_INFO_FLAGS_INSERT_TIMESTAMP;
        }
    }
    else
    {
        DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "control frame, drop it !!\n");
        dragonite_dump_tx_skb(skb);
        return -EINVAL;
    }
    if(is_multicast_ether_addr(wifi_hdr->addr1))
    {
#if defined(CONFIG_MAC80211_MESH) && !defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
        if(vif)
        {
            if(vif->type == NL80211_IFTYPE_MESH_POINT)
            {
                /* software makes multicast frame encryption in mesh network */
                skbinfo->flags |= SKB_INFO_FLAGS_FORCE_SECURITY_OFF;
            }
        }
#endif
        skbinfo->flags |= SKB_INFO_FLAGS_MULTICAST_FRAME;
        skbinfo->flags |= SKB_INFO_FLAGS_NO_BA_SESSION;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        if(ether_addr_equal(wifi_hdr->addr1, broadcast_addr))
#else
        if(0==compare_ether_addr(wifi_hdr->addr1, broadcast_addr))
#endif
        {
            skbinfo->flags |= SKB_INFO_FLAGS_BROADCAST_FRAME;
        }
    }

    return 0;
}
void dragonite_tx_replace_ssn(struct sk_buff *skb, u16 ssn)
{
   struct ieee80211_hdr *wifi_hdr = (struct ieee80211_hdr *) skb->data;

   wifi_hdr->seq_ctrl = (__le16) DRAGONITE_SN_TO_SEQ(ssn);
}
void drg_check_in_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb)
{
    int mis_align;
    struct dragonite_tx_descr *dragonite_descr;
#if defined (DRAGONITE_TX_SKB_CHECKER)
    volatile struct list_head *lptr;
    unsigned long irqflags;
#endif

    mis_align = ((unsigned long)(skb->data - TXDESCR_SIZE) % L1_CACHE_BYTES);

    dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR(((skb->data - TXDESCR_SIZE) - mis_align) - DRAGONITE_TXDESCR_SIZE);

    dragonite_descr->ts_tstamp = (jiffies / HZ);

#if defined (DRAGONITE_TX_SKB_CHECKER)
    lptr = (volatile struct list_head *) &dragonite_descr->skb_list;

    spin_lock_irqsave(&drg_tx_skb_lock, irqflags);

    skb_list_add(lptr, data->tx_skb_queue);

    spin_unlock_irqrestore(&drg_tx_skb_lock, irqflags);
#endif
}
void drg_check_out_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, bool success)
{
    struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
#if defined (DRAGONITE_TX_SKB_CHECKER)
    int mis_align;
    struct dragonite_tx_descr *dragonite_descr;
    volatile struct list_head *lptr;
    unsigned long irqflags;

    mis_align = ((unsigned long)(skb->data - TXDESCR_SIZE) % L1_CACHE_BYTES);

    dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR(((skb->data - TXDESCR_SIZE) - mis_align) - DRAGONITE_TXDESCR_SIZE);

    lptr = (volatile struct list_head *) &dragonite_descr->skb_list;

    spin_lock_irqsave(&drg_tx_skb_lock, irqflags);

    if(data->tx_skb_queue->qhead != NULL)
    {
        skb_list_del(lptr, data->tx_skb_queue);
    }
    else
    {
        panic("DRAGONITE: drg_hdl_tx_skb skb error !!");
    }

    spin_unlock_irqrestore(&drg_tx_skb_lock, irqflags);
#endif

    if(success || (txi->flags & IEEE80211_TX_CTL_REQ_TX_STATUS))
    {
        ieee80211_tx_status_irqsafe(data->hw, skb);
    }
    else
    {
        dev_kfree_skb_any(skb);
    }
}
#if defined (DRAGONITE_TX_SKB_CHECKER)
void skb_list_trace_timeout(struct mac80211_dragonite_data *data, u32 wakeup_t)
{
    unsigned long irqflags;
    int skb_timeout_count = 0;
    volatile struct list_head *hptr, *lptr;
    struct dragonite_tx_descr *dragonite_descr;

    spin_lock_irqsave(&drg_tx_skb_lock, irqflags);

    hptr = data->tx_skb_queue->qhead;
    if(hptr != NULL)
    {
        lptr = hptr;
        do
        {
            dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32) lptr - ((sizeof(struct list_head)) * 2));

            if((wakeup_t - dragonite_descr->ts_tstamp) > DRG_TX_SKB_TIMEOUT_TOLERANT)
            {
                skb_timeout_count++;
            }
            lptr = lptr->next;
        }while(hptr != lptr);
    }

    if(skb_timeout_count)
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Tx skb timeout over %d s, timeout skb count %d, total skb count %d\n", 
               DRG_TX_SKB_TIMEOUT_TOLERANT, skb_timeout_count, data->tx_skb_queue->qcnt);

    spin_unlock_irqrestore(&drg_tx_skb_lock, irqflags);
}
#endif
