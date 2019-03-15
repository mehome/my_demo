#include <dragonite_main.h>
#include <mac.h>
#include <mac_ctrl.h>
#include <mac_tables.h>

#include "dragonite_common.h"

static int mac_set_group_key_entry(int cipher_type, int sta_mode, struct ieee80211_key_conf *key, cipher_key *gkey, int rekey_id)
{
    if((cipher_type == CIPHER_TYPE_WEP40) || (cipher_type == CIPHER_TYPE_WEP104))
    {
		if(key->keyidx == 0)
		{
			gkey->wep_def_key.cipher_type = cipher_type;
            memcpy( (u8 *) gkey->wep_def_key.key, (u8 *) key->key, key->keylen);
		}
		else if(key->keyidx == 1)
		{
			gkey->wep_def_key.cipher_type1 = cipher_type;
            memcpy( (u8 *) gkey->wep_def_key.key1, (u8 *) key->key, key->keylen);
		}
		else if(key->keyidx == 2)
		{
			gkey->wep_def_key.cipher_type2 = cipher_type;
            memcpy( (u8 *) gkey->wep_def_key.key2, (u8 *) key->key, key->keylen);
		}
		else if(key->keyidx == 3)
		{
			gkey->wep_def_key.cipher_type3 = cipher_type;
            memcpy( (u8 *) gkey->wep_def_key.key3, (u8 *) key->key, key->keylen);
		}

		gkey->wep_def_key.iv = 0;
    }
    else if(cipher_type == CIPHER_TYPE_TKIP)
    {
        gkey->tkip_pair_key.cipher_type = cipher_type;
        //gkey->tkip_group_key.txtsc_lo = 1;
        //gkey->tkip_group_key.txtsc_hi = 0;
        gkey->tkip_pair_key.txkeyid = key->keyidx;		/* shall be 1~3 for group key */

		gkey->tkip_pair_key.rekey_id = rekey_id;

        memcpy( (u8 *) gkey->tkip_pair_key.key, (u8 *) key->key, 16);

        memcpy( (u8 *) gkey->tkip_pair_key.txmickey, (u8 *) &key->key[16], 8);
        memcpy( (u8 *) gkey->tkip_pair_key.rxmickey, (u8 *) &key->key[24], 8);
    }
    else if(cipher_type == CIPHER_TYPE_CCMP)
    {
        gkey->ccmp_pair_key.cipher_type = cipher_type;
        //gkey->ccmp_group_key.txpn_lo = 1;
        //gkey->ccmp_group_key.txpn_hi = 0;
        gkey->ccmp_pair_key.txkeyid = key->keyidx;		/* shall be 1~3 for group key */

		gkey->ccmp_pair_key.rekey_id = rekey_id;

        memcpy( (u8 *) gkey->ccmp_group_key.key, (u8 *) key->key, key->keylen);
    }
#if 0
    else if(cipher_type == CIPHER_TYPE_WAPI)
    {
        gkey->wapi_group_key.cipher_type = cipher_type;
        gkey->wapi_group_key.txpn[0] = WAPI_PN_DWORD;
        gkey->wapi_group_key.txpn[1] = WAPI_PN_DWORD;
        gkey->wapi_group_key.txpn[2] = WAPI_PN_DWORD;
        gkey->wapi_group_key.txpn[3] = WAPI_PN_DWORD;
        gkey->wapi_group_key.txkeyid = key->keyidx % 2;

        memcpy( (u8 *) gkey->wapi_group_key.key, (u8 *) key->key, WAPI_CIPHER_KEY_LEN);
        memcpy( (u8 *) gkey->wapi_group_key.mickey, (u8 *) &key->key[WAPI_CIPHER_KEY_LEN], WAPI_MICKEY_LEN);
    }
#endif
    return 0;
}

static int mac_set_pairwise_key_entry(int cipher_type, int sta_mode, struct ieee80211_key_conf *key, cipher_key *pkey, int rekey_id)
{
	if((cipher_type == CIPHER_TYPE_WEP40) || (cipher_type == CIPHER_TYPE_WEP104))
	{
		pkey->wep_km_key.cipher_type = cipher_type;
		pkey->wep_km_key.iv = 0;

		memcpy( (u8 *) pkey->wep_km_key.key, (u8 *) key->key, key->keylen);
	}
	else if(cipher_type == CIPHER_TYPE_TKIP)
	{
		pkey->tkip_pair_key.cipher_type = cipher_type;
		//pkey->tkip_pair_key.txtsc_lo = 1;
		//pkey->tkip_pair_key.txtsc_hi = 0;
		//pkey->tkip_pair_key.txkeyid = 0;

		pkey->tkip_pair_key.rekey_id = rekey_id;

		memcpy( (u8 *) pkey->tkip_pair_key.key, (u8 *) &key->key, 16);

		memcpy( (u8 *) pkey->tkip_pair_key.txmickey, (u8 *) &key->key[16], 8);
		memcpy( (u8 *) pkey->tkip_pair_key.rxmickey, (u8 *) &key->key[24], 8);
/*
#if defined(ARTHUR_AMPDU_TX)
		pkey->tkip_pair_key.id++;
#endif
*/
	}
	else if(cipher_type == CIPHER_TYPE_CCMP)
	{
		pkey->ccmp_pair_key.cipher_type = cipher_type;
		//pkey->ccmp_pair_key.txpn_lo = 1;
		//pkey->ccmp_pair_key.txpn_hi = 0;
		//pkey->ccmp_pair_key.txkeyid = 0;
		 
		pkey->ccmp_pair_key.rekey_id = rekey_id;

		memcpy( (u8 *) pkey->ccmp_pair_key.key, (u8 *) key->key, key->keylen);
/*
#if defined(ARTHUR_AMPDU_TX)
		pkey->ccmp_pair_key.id++;
#endif
*/
	}
#if 0
	else if(cipher_type == CIPHER_TYPE_WAPI)
	{
		pkey->wapi_pair_key.cipher_type = cipher_type;
		if(sta_mode)
			pkey->wapi_pair_key.txpn[0] = WAPI_PN_DWORD;
		else
			pkey->wapi_pair_key.txpn[0] = WAPI_PN_DWORD + 1;
		pkey->wapi_pair_key.txpn[1] = WAPI_PN_DWORD;
		pkey->wapi_pair_key.txpn[2] = WAPI_PN_DWORD;
		pkey->wapi_pair_key.txpn[3] = WAPI_PN_DWORD;
		pkey->wapi_pair_key.txkeyid = key->keyidx;

		memcpy( (u8 *) pkey->wapi_pair_key.key, (u8 *) key->key, WAPI_CIPHER_KEY_LEN);
		memcpy( (u8 *) pkey->wapi_pair_key.mickey, (u8 *) &key->key[WAPI_CIPHER_KEY_LEN], WAPI_MICKEY_LEN);

#if defined(ARTHUR_AMPDU_TX)
		pkey->wapi_pair_key.id++;
#endif
	}
#endif
	return 0;
}
int dragonite_key_config(struct mac80211_dragonite_data *data, struct ieee80211_vif *vif,
		struct ieee80211_sta *sta, struct ieee80211_key_conf *key)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct dragonite_sta_priv *sp;
	MAC_INFO* info = data->mac_info;
	int cipher_type;
	bool sta_mode = dragonite_vif_is_sta_mode(vif);
	int sta_cap_tbl_index;
	int flag;
	int addr_index;
	sta_basic_info bcap;
	sta_cap_tbl* captbl;
	flag=0;
	bcap.val = 0;

	switch(key->cipher) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			if(key->keylen == 5) {
				cipher_type = CIPHER_TYPE_WEP40;
			}
			else if(key->keylen == 13) {
				cipher_type = CIPHER_TYPE_WEP104;
			}
			else
				return -EOPNOTSUPP;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			cipher_type = CIPHER_TYPE_TKIP;
			break;
		case WLAN_CIPHER_SUITE_AES_CMAC:
			key->flags |= IEEE80211_KEY_FLAG_SW_MGMT_TX;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			cipher_type = CIPHER_TYPE_CCMP;
			key->flags |= IEEE80211_KEY_FLAG_SW_MGMT_TX;
			break;
		default:
			return -EOPNOTSUPP;
			break;
	}

	if(key->cipher == WLAN_CIPHER_SUITE_AES_CMAC)
	{
		goto exit;
	}
	if (!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE))//group key
	{
#if defined(DRAGONITE_MLME_DEBUG)
        printk(KERN_CRIT "set group key bssid idx %d, sta_mode %d\n", vp->bssid_idx, sta_mode);
#else
        DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "set group key bssid idx %d, sta_mode %d\n", vp->bssid_idx, sta_mode);
#endif
        DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "cipher_type %08x icv_len %d iv_len %d flags %x keyidx %d keylen %d\n",
               key->cipher, key->icv_len, key->iv_len, key->flags, key->keyidx, key->keylen);

		DRG_NOTICE_DUMP(DRAGONITE_DEBUG_ENCRPT_FLAG, "key: ", key->key, key->keylen);

#if defined(CONFIG_MAC80211_MESH) && !defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
		if(vif->type == NL80211_IFTYPE_MESH_POINT)
		{
			return -EOPNOTSUPP;
		}
#endif
		if(vif->type == NL80211_IFTYPE_AP)
		{
			/* force multicast frame(CCMP and TKIP) use software en/decrypt to work around bcq handle PN wrong */
			if((cipher_type == CIPHER_TYPE_TKIP) || (cipher_type == CIPHER_TYPE_CCMP))
			{
				return -EOPNOTSUPP;
			}
		}
		dragonite_mac_lock();

#if defined(CONFIG_MAC80211_MESH) && defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
		if(vif->type == NL80211_IFTYPE_MESH_POINT)
		{
			key->hw_key_idx = vp->bssid_idx + MAC_EXT_BSSIDS;
		}
		else
		{
			key->hw_key_idx = vp->bssid_idx;
		}
#else
		key->hw_key_idx = vp->bssid_idx;
#endif

		mac_rekey_start(key->hw_key_idx, REKEY_TYPE_GKEY);

		if(vp->rekey_id == 0)
		{
			vp->rekey_id = 1;
		}
		else if(vp->rekey_id == 1)
		{
			vp->rekey_id = 0;
		}

		memset(&vp->tsc[0], 0, 16);

		mac_set_group_key_entry(cipher_type, sta_mode, key, &data->mac_info->group_keys[key->hw_key_idx], vp->rekey_id);

		mac_rekey_done();

		vp->bssinfo->cipher_mode = cipher_type;

		if(sta_mode)//update lookup engine
		{
			if((cipher_type == CIPHER_TYPE_WEP40) || (cipher_type == CIPHER_TYPE_WEP104))
			{
				sta_cap_tbl_index = wmac_addr_lookup_engine_find(vp->bssinfo->associated_bssid, (int)&addr_index, &bcap.val, flag | IN_DS_TBL);
				captbl = &info->sta_cap_tbls[sta_cap_tbl_index];
				captbl->cipher_mode = cipher_type;
				captbl->wep_defkey = 1;
				bcap.val = bcap.val >> 8;
				bcap.bit.cipher |= 1;
				bcap.bit.wep_defkey |= 1;
				addr_index = wmac_addr_lookup_engine_update(info, 0, addr_index, (bcap.val << 8) | sta_cap_tbl_index, flag | IN_DS_TBL | BY_ADDR_IDX);
			}
		}

		dragonite_mac_unlock();
	}
	else//pairwise key
	{
		sp = (void *)sta->drv_priv;

#if defined(DRAGONITE_MLME_DEBUG)
        printk(KERN_CRIT "set pair key sta index %d, sta_mode %d\n", sp->stacap_index, sta_mode);
#else
        DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "set pair key sta index %d, sta_mode %d\n", sp->stacap_index, sta_mode);
#endif
        DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "cipher type %08x icv_len %d iv_len %d flags %x keyidx %d keylen %d\n",
               key->cipher, key->icv_len, key->iv_len, key->flags, key->keyidx, key->keylen);

		DRG_NOTICE_DUMP(DRAGONITE_DEBUG_ENCRPT_FLAG, "key: ", key->key, key->keylen);

		dragonite_mac_lock();

		key->hw_key_idx = sp->stacap_index;

		mac_rekey_start(key->hw_key_idx, REKEY_TYPE_PKEY);

		if(sp->rekey_id == 0)
		{
			sp->rekey_id = 1;
		}
		else if(sp->rekey_id == 1)
		{
			sp->rekey_id = 0;
		}

		memset(&sp->tsc[0], 0, 16);

		mac_set_pairwise_key_entry(cipher_type, sta_mode, key, &data->mac_info->private_keys[key->hw_key_idx], sp->rekey_id);

		captbl = &info->sta_cap_tbls[key->hw_key_idx];
		captbl->cipher_mode = cipher_type;

 		bcap.val = sp->sta_binfo.val;
 		bcap.val = (bcap.val >> 8);
 		bcap.bit.cipher |= 1;
 		bcap.bit.bssid = vp->bssid_idx;
 		if(sta_mode)
 		{
 			addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, flag | IN_DS_TBL | BY_ADDR_IDX);
 		}
 		else
 		{
 			addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, flag | BY_ADDR_IDX);
 		}

 		sp->sta_binfo.val = (bcap.val << 8) | sp->stacap_index;
 		sp->addr_index = addr_index;

		mac_rekey_done();

		dragonite_mac_unlock();

		DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "Set key finished\n");
	}

exit:
	return 0;

}
int dragonite_key_delete(struct mac80211_dragonite_data *data, struct ieee80211_vif *vif, 
		struct ieee80211_key_conf *key)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	bool sta_mode = dragonite_vif_is_sta_mode(vif);

#if defined(DRAGONITE_MLME_DEBUG)
    printk(KERN_CRIT "delete key bssid idx %d, sta_mode %d\n", vp->bssid_idx, sta_mode);
#else
    DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "delete key bssid idx %d, sta_mode %d\n", vp->bssid_idx, sta_mode);
#endif
    DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "cipher_type %08x icv_len %d iv_len %d flags %x keyidx %d keylen %d\n",
           key->cipher, key->icv_len, key->iv_len, key->flags, key->keyidx, key->keylen);

	//DRG_NOTICE_DUMP(DRAGONITE_DEBUG_ENCRPT_FLAG, "key: ", key->key, key->keylen);

	if(key->cipher == WLAN_CIPHER_SUITE_AES_CMAC)
	{
		goto exit;
	}

	if (!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE))//delete group key
	{
		vp->bssinfo->cipher_mode = CIPHER_TYPE_NONE;

		dragonite_mac_lock();

		mac_rekey_start(key->hw_key_idx, REKEY_TYPE_GKEY);

		memset(&data->mac_info->group_keys[key->hw_key_idx], 0, sizeof(cipher_key));

        mac_rekey_done();

		dragonite_mac_unlock();
	}
	else
	{
		data->mac_info->sta_cap_tbls[key->hw_key_idx].cipher_mode = CIPHER_TYPE_NONE;

		dragonite_mac_lock();

        mac_rekey_start(key->hw_key_idx, REKEY_TYPE_PKEY);

		memset(&data->mac_info->private_keys[key->hw_key_idx], 0, sizeof(cipher_key));

		mac_rekey_done();

		dragonite_mac_unlock();
	}
exit:
	return 0;
}
