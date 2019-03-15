/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
#if defined(CONFIG_BRIDGE)
#define RX_HAS_VLAN_TAG

#include <linux/if_ether.h>
#include <mac_ctrl.h>
//#include <net/dsfield.h>
#include <linux/netdevice.h>
#include "skb.h"
#if defined(RX_HAS_VLAN_TAG)
#include <linux/if_vlan.h>
#endif
#include <wla_mat.h>

int enable_swbr_shortcut = 1;
int enable_swbr_debug = 0;
#define __DBG(fmt, args...)     do {            \
    if (enable_swbr_debug) {                    \
        printk("%s: "fmt, __func__, ## args);   \
    }                                           \
} while(0);
static const u8 broadcast_addr[ETH_ALEN] =  { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

extern struct mac80211_dragonite_data *global_data;
#define WIFI_RX_SKB     (1)
int parse_ethernet_rx_skb(struct sk_buff *skb, struct skb_info *skbinfo, int wifi_rx)
{
    struct ethhdr *eth_hdr;
    int bss_idx;

    if(skb->len < ETH_HLEN)
    {
        return -EINVAL;
    }

    skbinfo->da_index = -1;
    skbinfo->header_length = ETH_HLEN;
    skbinfo->flags = 0;

    eth_hdr = (struct ethhdr* ) skb->data;

    if(is_multicast_ether_addr(eth_hdr->h_dest))
    {
        skbinfo->flags |= SKB_INFO_FLAGS_DA_IS_MULTICAST;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        if(ether_addr_equal(eth_hdr->h_dest, broadcast_addr))
#else
        if(0==compare_ether_addr(eth_hdr->h_dest, broadcast_addr))
#endif
        {
            skbinfo->flags |= SKB_INFO_FLAGS_DA_IS_BROADCAST;
        }
    }
    else
    {
        dragonite_mac_lock();

        skbinfo->da_index = wmac_addr_lookup_engine_find(eth_hdr->h_dest, 0, 0, 0);

        dragonite_mac_unlock();

        if(wifi_rx)
        {
            for(bss_idx = 0; bss_idx < MAC_MAX_BSSIDS; bss_idx++)
            {
                if(0 == memcmp(skb->data, global_data->dragonite_bss_info[bss_idx].MAC_ADDR, 6))
                {
                    break;
                }
            }
            if(bss_idx >= MAC_MAX_BSSIDS)
            {
                __DBG("!!!Warning: da no match with any bss\n");
            }
            else
            {
                if(ROLE_STA == global_data->dragonite_bss_info[bss_idx].dut_role)
                {
                    skbinfo->flags |= SKB_INFO_FLAGS_STA_WIFI_RX;
                }
            }
        }
    }

    return 0;
}

extern int wifi_tx_enable;
extern u32 drg_wifi_sniffer;
extern struct net_device *wlan_dev_g;
extern struct net_device *sta_dev_g;
extern struct net_device *lan_dev_g;
struct ieee80211_sub_if_data;
extern u16 ieee80211_select_queue(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb);
extern void skb_set_queue_mapping(struct sk_buff *skb, u16 queue_mapping);
extern netdev_tx_t ieee80211_subif_start_xmit(struct sk_buff *skb, struct net_device *dev);
void forward_to_wifi_tx(struct sk_buff *skb, struct net_device *dev)
{
    ehdr *ef;
    u16 ethertype;

#if defined(RX_HAS_VLAN_TAG)
    memmove((skb->data + VLAN_HLEN), skb->data, ETH_ALEN * 2);
    skb_pull(skb, VLAN_HLEN);
#endif
    if(sta_dev_g)
    {
        ef = (ehdr *)((int)skb->data);
        mat_handle_insert(ef, dev->dev_addr);
        __DBG("mat_handle_insert\n");
    }

    ethertype = (skb->data[12] << 8) | skb->data[13];
    skb->protocol = htons(ethertype);
    skb_set_network_header(skb, ETH_HLEN);
    skb_set_queue_mapping(skb, ieee80211_select_queue(netdev_priv(dev), skb));
    ieee80211_subif_start_xmit(skb, dev);

    return;
}

extern int dragonite_dynamic_ps;
extern int wifi_hw_supported;
int forward_eth_skb(struct sk_buff *skb)
{
    int ret=NET_RX_SUCCESS;

    struct skb_info skbinfo;
    struct sk_buff *new_skb;

    new_skb = NULL;

    __DBG("eth rx\n");

    if(0 == enable_swbr_shortcut)
    {
        goto EXIT;
    }

    if(dragonite_dynamic_ps)
    {
        goto EXIT;
    }

    if(0 == wifi_hw_supported)
    {
        goto EXIT;
    }

    if((0 == wifi_tx_enable) || (drg_wifi_sniffer == 1))
    {
        goto EXIT;
    }

    if(NULL == lan_dev_g)
    {
        __DBG("No lan_dev_g\n");
        goto EXIT;
    }

    if((NULL == wlan_dev_g) 
       && (NULL == sta_dev_g))
    {
        __DBG("no wlan_dev_g also no sta_dev_g\n");
        goto EXIT;
    }

    if(0 != parse_ethernet_rx_skb(skb, &skbinfo, !WIFI_RX_SKB))
    {
        goto EXIT;
    }

    if(sta_dev_g
       ||(0 == (skbinfo.flags & SKB_INFO_FLAGS_DA_IS_MULTICAST)))
    {
        if(wlan_dev_g && (0 > skbinfo.da_index))
            goto EXIT;

        if(wlan_dev_g)
        {
            if(0 == memcmp(skb->data,wlan_dev_g->dev_addr,6))
                goto EXIT;
        }
        else
        {
            if(0 == memcmp(skb->data,sta_dev_g->dev_addr,6))
                goto EXIT;
        }
        /* unicast, either TX to some associated STA or just drop it */
        if(skbinfo.flags & SKB_INFO_FLAGS_DA_IS_MULTICAST)
        {
            ASSERT(sta_dev_g, "forward_eth_skb: non-sta forward gc\n");
            ret = NET_RX_SUCCESS;
            new_skb = skb_copy(skb, GFP_ATOMIC);
            if(NULL == new_skb)
            {
                printk("forward_eth_skb: cannot alloc new skb");
            }
            else
            {
                __DBG("groupcast, forward to wifi and cpu\n");
                if(wlan_dev_g)
                {
                    forward_to_wifi_tx(new_skb, wlan_dev_g);
                }
                else
                {
                    forward_to_wifi_tx(new_skb, sta_dev_g);
                }
            }
        }
        else
        {
            ret = NET_RX_DROP;
            __DBG("unicast, forward to wifi\n");
            if(wlan_dev_g)
            {
                forward_to_wifi_tx(skb, wlan_dev_g);
            }
            else
            {
                forward_to_wifi_tx(skb, sta_dev_g);
            }
        }
    }
EXIT:
    return ret;
}

extern int eth_send(struct sk_buff *skb, struct net_device *dev);
extern int lan_interface_vlan_id(void);
int forward_wifi_skb(struct sk_buff * skb)
{
    int ret=NET_RX_SUCCESS;
    struct skb_info skbinfo;
    ehdr *ef;

    __DBG("wifi rx\n");
    if(0 == enable_swbr_shortcut)
    {
        goto EXIT;
    }

    if(dragonite_dynamic_ps)
    {
        goto EXIT;
    }
    if(NULL == lan_dev_g)
    {
        __DBG("No lan_dev_g\n");
        goto EXIT;
    }

    if(0 != parse_ethernet_rx_skb(skb, &skbinfo, WIFI_RX_SKB))
    {
        goto EXIT;
    }

    if(0 != (skbinfo.flags & SKB_INFO_FLAGS_DA_IS_MULTICAST))
    {
        goto EXIT;
    }

    if(0 == (skbinfo.flags & SKB_INFO_FLAGS_STA_WIFI_RX))
    {
        if(NULL == wlan_dev_g)
        {
            // avoid panic for shortcut before add "wlan0" to bridge
            __DBG("No wlan_dev_g\n");
            goto EXIT;
        }
        if(0 == memcmp(skb->data,wlan_dev_g->dev_addr,6))
        {
            __DBG("send to wlan0, forward to cpu\n");
            goto EXIT;
        }
    }
    else
    {
        if(NULL == sta_dev_g)
        {
            __DBG("No sta_dev_g\n");
            goto EXIT;
        }
        ef = (ehdr *)((int)skb->data);
        mat_handle_lookup(ef);
        __DBG("da after mat_handle_lookup: %02x:%02x:%02x:%02x:%02x:%02x\n",
               skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4], skb->data[5]);

        if(0 == memcmp(skb->data,sta_dev_g->dev_addr,6))
        {
            __DBG("send to sta0, forward to cpu\n");
            goto EXIT;
        }
    }

    ret=NET_RX_DROP;
    skb->vlan_tci = lan_interface_vlan_id();
    __DBG("send to eth, forward to lan\n");
    eth_send(skb, lan_dev_g);
EXIT:
    return ret;
}
#endif // CONFIG_BRIDGE
