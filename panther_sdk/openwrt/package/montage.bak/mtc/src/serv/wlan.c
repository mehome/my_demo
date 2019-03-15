#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

#define IFPLUGD_NAME             "ifplugd"
#define IFPLUGD_PROC             "/usr/bin/"IFPLUGD_NAME
#define IFPLUGD_STA0_PIDFILE     "/var/run/"IFPLUGD_NAME".sta0.pid"

extern MtdmData *mtdm;

//#define HAVE_MULTIPLE_BSSID

#define GPIO_PATH              "/sys/devices/platform/cta-gpio/gpio"

#define HOSTAPD_CLI_ACTIONFILE "/tmp/hostapd_cli_action"
#define HOSTAPD_CLI_PIDFILE    "/var/run/hostapd_cli.pid"
#define HOSTAPD_CLI_CMDLINE    "-a "HOSTAPD_CLI_ACTIONFILE
#define HOSTAPD_CLI_BIN        "/usr/sbin/hostapd_cli"

#define HOSTAPD_CFGFILE        "/var/run/hostapd.conf"
#define HOSTAPD_PIDFILE        "/var/run/hostapd.pid"
#define HOSTAPD_ACLFILE        "/var/run/hostapd.acl"
#define HOSTAPD_CMDLINE        "-B -P "HOSTAPD_PIDFILE" "HOSTAPD_CFGFILE
#define HOSTAPD_BIN            "/usr/sbin/hostapd"
#define HOSTAPD_MAX_CHK_CNT    5
#ifdef SPEEDUP_WLAN_CONF
#define HOSTAPD_CFGFILE_TMP     "/var/run/hostapd.conf.tmp"
#endif

#define WPASUP_CFGFILE         "/var/run/wpa.conf"
#define WPASUP_PIDFILE         "/var/run/wpa.pid"
#define WPASUP_HCTRL           "-H /var/run/hostapd/wlan0"
#if defined(__PANTHER__)
#define WPASUP_CMDLINE         "-B -P "WPASUP_PIDFILE" -i %s -b%s -Dnl80211 -c "WPASUP_CFGFILE
#else
#define WPASUP_CMDLINE         "-B -P "WPASUP_PIDFILE" -i %s -Dnl80211 -c "WPASUP_CFGFILE
#endif
#define WPASUP_BIN             "/usr/sbin/wpa_supplicant"
#define WPASUP_MAX_CHK_CNT     5
#ifdef SPEEDUP_WLAN_CONF
#define WPASUP_CFGFILE_TMP     "/var/run/wpa.conf.tmp"
#endif

#define P2P_PIDFILE            "/var/run/p2p.pid"

#define PROCWLA                "/proc/wla"

#define AUTH_CAP_NONE          (0)
#define AUTH_CAP_OPEN          (1)
#define AUTH_CAP_SHARED        (2)
#define AUTH_CAP_WEP           (4)
#define AUTH_CAP_WPA           (8)
#define AUTH_CAP_WPA2          (16)
#define AUTH_CAP_WPS           (32)
#define AUTH_CAP_WAPI          (128)

#define AUTH_CAP_CIPHER_NONE   (0)
#define AUTH_CAP_CIPHER_WEP40  (1)
#define AUTH_CAP_CIPHER_WEP104 (2)
#define AUTH_CAP_CIPHER_TKIP   (4)
#define AUTH_CAP_CIPHER_CCMP   (8)
#define AUTH_CAP_CIPHER_WAPI   (16)

#define BB_MODE_B              (1)
#define BB_MODE_G              (2)
#define BB_MODE_BG             (3)
#define BB_MODE_N              (4)
#define BB_MODE_BGN            (7)

int mtdm_sleep(int sec)
{
    sleep(sec);
    return 0;
}

//wds rule $wl_wds1='gkey=1&en=1&mac=00:12:0e:b5:5b:10&cipher=1&kidx=0'
static void config_wds(void)
{
    char wl_wds[MSBUF_LEN] = { 0 };
    char macaddr[MBUF_LEN] = { 0 };
    char wl_bss_wep_key[MSBUF_LEN] = { 0 };
    char wl_bss_wep_keyname[USBUF_LEN];
    char *ptr;
    char *argv[10];
    char *en, *mac, *cipher, *kidx;
    int num;
    int rule;
    int i, j;
    u32 idx;

    for (i=1, rule=0;;i++, rule++) {
        if (cdb_get_array_str("$wl_wds", i, wl_wds, sizeof(wl_wds), NULL) == NULL) {
            break;
        }
        else if (strlen(wl_wds) == 0) {
            break;
        }
        else {
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_wds%d is [%s]", __func__, i, wl_wds);

            num = str2argv(wl_wds, argv, '&');
            if((num < 5)                 || !str_arg(argv, "gkey=") || 
               !str_arg(argv, "en=")     || !str_arg(argv, "mac=")  || 
               !str_arg(argv, "cipher=") || !str_arg(argv, "kidx=")) {
               MTDM_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                continue;
            }

            en     = str_arg(argv, "en=");
            mac    = str_arg(argv, "mac=");
            cipher = str_arg(argv, "cipher=");
            kidx   = str_arg(argv, "kidx=");

            MTDM_LOGGER(this, LOG_INFO, "%s: en is [%s]", __func__, en);
            MTDM_LOGGER(this, LOG_INFO, "%s: mac is [%s]", __func__, mac);
            MTDM_LOGGER(this, LOG_INFO, "%s: cipher is [%s]", __func__, cipher);
            MTDM_LOGGER(this, LOG_INFO, "%s: kidx is [%s]", __func__, kidx);

            for (j=0, ptr=mac; *ptr!=0; ptr++) {
                if (*ptr != ':') {
                    macaddr[j++] = *ptr;
                }
            }
            macaddr[j] = 0;
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_peer_addr.%d=0x%s ", rule, macaddr);
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_cipher_type.%d=%s ", rule, cipher);
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_keyid.%d=%s ", rule, kidx);

            if (!strcmp(cipher, "1") || !strcmp(cipher, "2")) {
                idx = strtoul(kidx, NULL, 0) + 1; 
                sprintf(wl_bss_wep_keyname, "$wl_bss_wep_%dkey", idx);
                if (cdb_get_array_str(wl_bss_wep_keyname, 
                    idx, wl_bss_wep_key, sizeof(wl_bss_wep_key), NULL) != NULL) {
                    f_write_sprintf(PROCWLA, 0, 0, "config.wds_key%d.%d=%s ", kidx, rule, wl_bss_wep_key);
                }
                else {
                    MTDM_LOGGER(this, LOG_ERR, "%s: Can't get %s for wl_wds", __func__, wl_bss_wep_keyname);
                }
            }
            else {
                // FIXME: I don't know what's purpose for this script, skip wait implement
#if  0
config_get wl_bss_wpa_psk1 wl_bss_wpa_psk1
MSG=`wpa_passphrase $ssid $wl_bss_wpa_psk1`
PSK=`echo $MSG | awk -F" " '{ printf $4 }'`
eval $PSK
echo "config.wds_key0=${psk}" > $PROC
#endif
                MTDM_LOGGER(this, LOG_INFO, "%s: wait for implement here(line:%d)", __func__, __LINE__);
            }
        }
    }
}

//macf rule $wl_macf1='en=1&smac=a0:11:22:33:44:51&desc=My iPhone'
static void config_hostapd_acl(int wl_macf_mode)
{
    char wl_macf[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *en, *smac;
    int num;
    int i;

    unlink(HOSTAPD_ACLFILE);

    if ((wl_macf_mode == 1) || (wl_macf_mode == 2)) {
        for (i=1;;i++) {
            if (cdb_get_array_str("$wl_macf", i, wl_macf, sizeof(wl_macf), NULL) == NULL) {
                break;
            }
            else if (strlen(wl_macf) == 0) {
                break;
            }
            else {
                MTDM_LOGGER(this, LOG_INFO, "%s: wl_macf%d is [%s]", __func__, i, wl_macf);

                num = str2argv(wl_macf, argv, '&');
                if((num < 3)               || !str_arg(argv, "en=") || 
                   !str_arg(argv, "smac=") || !str_arg(argv, "desc=")) { 
                    MTDM_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                    continue;
                }
                en   = str_arg(argv, "en=");
                smac = str_arg(argv, "smac=");

                MTDM_LOGGER(this, LOG_INFO, "%s: en is [%s]", __func__, en);
                MTDM_LOGGER(this, LOG_INFO, "%s: smac is [%s]", __func__, smac);

                if (!strcmp(en, "1")) {
                    f_write_string(HOSTAPD_ACLFILE, smac, FW_APPEND | FW_NEWLINE, 0);
                }
            }
        }
        f_write_string(HOSTAPD_ACLFILE, NULL, FW_APPEND | FW_NEWLINE, 0);
    }
}

static void config_general(void)
{
    int wl_bb_mode, wl_2choff, wl_sgi, wl_preamble;
    int wl_bss_int;
    int wl_twinsz, wl_rwinsz;
    int wl_sprate;

    // AP/BSS isolated
    wl_bss_int = cdb_get_int("$wl_ap_isolated", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.wl_ap_isolated=%d ", wl_bss_int);
    wl_bss_int = cdb_get_int("$wl_bss_isolated", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.wl_bss_isolated=%d ", wl_bss_int);

    // AMPDU
    wl_bss_int = cdb_get_int("$wl_no_ampdu", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.no_ampdu=%d ", wl_bss_int);
    wl_bss_int = cdb_get_int("$wl_ampdu_tx_mask", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.ampdu_tx_mask=%d ", wl_bss_int);
    wl_bss_int = cdb_get_int("$wl_recovery", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.recovery=%d ", wl_bss_int);
	
    // AMPDU aggregation number
    wl_twinsz = cdb_get_int("$wl_tx_winsz", 8);
    f_write_sprintf(PROCWLA, 0, 0, "tx_winsz=%d ", wl_twinsz);
    wl_rwinsz = cdb_get_int("$wl_rx_winsz", 8);
    f_write_sprintf(PROCWLA, 0, 0, "rx_winsz=%d ", wl_rwinsz);

    // Specific tx rate setup
    wl_sprate = cdb_get_int("$wl_tx_sprate", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.spr=%d ", wl_sprate);

    // special config
    // cwnd and aifs
    wl_bss_int = cdb_get_int("$wl_cw_aifs", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.wl_cw_aifs=%d ", wl_bss_int);

    // wds with ampdu
    wl_bss_int = cdb_get_int("$wl_wds_ampdu", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.wl_wds_ampdu=%d ", wl_bss_int);

    // Fix rate
    wl_bss_int = cdb_get_int("$wl_tx_rate", 0);
    f_write_sprintf(PROCWLA, 0, 0, "config.fix_txrate=%d ", wl_bss_int);

    // Fix rate flags
    wl_bss_int = 0;
    wl_bb_mode = cdb_get_int("$wl_bb_mode", 0);
    wl_2choff = cdb_get_int("$wl_2choff", 0);
    wl_sgi = cdb_get_int("$wl_sgi", 0);
    wl_preamble = cdb_get_int("$wl_preamble", 0);
    if (wl_bb_mode >= BB_MODE_N) {
        if (wl_2choff > 0) {
            wl_bss_int |= (1 << 2);
        }
        if (wl_sgi == 1) {
            wl_bss_int |= (1 << 0);
            if (wl_2choff > 0) {
                wl_bss_int |= (1 << 1);
            }
        }
    }
    if (wl_preamble == 0) {
        wl_bss_int |= (1 << 4);
    }
    f_write_sprintf(PROCWLA, 0, 0, "config.rate_flags=%d ", wl_bss_int);

    if (mtdm->rule.HOSTAPD == 1) {
        wl_bss_int = cdb_get_int("$wl_forward_mode", 0);
        f_write_sprintf(PROCWLA, 0, 0, "config.wl_forward_mode=%d ", wl_bss_int);

        wl_bss_int = cdb_get_int("$wl_tx_pwr", 0);
        f_write_sprintf(PROCWLA, 0, 0, "config.wl_tx_pwr=%d ", wl_bss_int);

        if (cdb_get_int("$wl_protection", 0) == 1) {
            f_write_sprintf(PROCWLA, 0, 0, "tx_protect=1 ");
        }
        else {
            f_write_sprintf(PROCWLA, 0, 0, "tx_protect=0 ");
        }

        f_write_sprintf(PROCWLA, 0, 0, "config.wds_mode=0 ");

        wl_bss_int = cdb_get_int("$wl_wds_mode", 0);
        if ((wl_bss_int == 1) || (wl_bss_int == 2)) {
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_mode=%d ", wl_bss_int);
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_peer_addr.0=0 ");
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_peer_addr.1=0 ");
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_peer_addr.2=0 ");
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_peer_addr.3=0 ");

            config_wds();
        }
        else {
            f_write_sprintf(PROCWLA, 0, 0, "config.wds_mode=0 ");
        }

        // Power Saving
        wl_bss_int = cdb_get_int("$wl_ps_mode", 0);
        f_write_sprintf(PROCWLA, 0, 0, "config.pw_save=%d ", wl_bss_int);
        if (wl_bss_int == 2) {
            f_write_sprintf(PROCWLA, 0, 0, "config.uapsd=1 ");
        }
        else {
            f_write_sprintf(PROCWLA, 0, 0, "config.uapsd=0 ");
        }
    }
    if (mtdm->rule.WPASUP == 1) {
        wl_bss_int = cdb_get_int("$wl_forward_mode", 0);
        f_write_sprintf(PROCWLA, 0, 0, "config.wl_forward_mode=%d ", wl_bss_int);
        
        if ((cdb_get_int("$wl_bss_role2", 0) & 256) > 0) {
            f_write_sprintf(PROCWLA, 0, 0, "config.sta_mat=1 ");
        }
        else {
            f_write_sprintf(PROCWLA, 0, 0, "config.sta_mat=0 ");
        }
    }
}

//wl_aplist_info=bssid=xxx&sec=xxx&cipher=&last=&mode=&tc=&ssid=
//wl_aplist_pass=xxx
static void config_ap_list(FILE *fp)
{
    char wl_aplist_info[MSBUF_LEN] = { 0 };
    char wl_aplist_pass[MSBUF_LEN] = { 0 };
    char wl_aplist_info_ssid[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *bssid, *sec, *cipher, *last, *ssid;
    char *pass = wl_aplist_pass;
    int num;
    int i;

    for (i=1;i<11;i++) {
        if (cdb_get_array_str("$wl_aplist_info", i, wl_aplist_info, sizeof(wl_aplist_info), NULL) == NULL) {
            break;
        }
        else if (strlen(wl_aplist_info) == 0) {
            break;
        }
        else {
            cdb_get_array_str("$wl_aplist_pass", i, wl_aplist_pass, sizeof(wl_aplist_pass), "");
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_info%d is [%s]", __func__, i, wl_aplist_info);
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_pass%d is [%s]", __func__, i, wl_aplist_pass);

            memcpy(wl_aplist_info_ssid, wl_aplist_info, sizeof(wl_aplist_info_ssid));
            num = str2argv(wl_aplist_info, argv, '&');
            if((num < 7)               || !str_arg(argv, "bssid=")  || 
               !str_arg(argv, "sec=")  || !str_arg(argv, "cipher=") || 
               !str_arg(argv, "last=") || !str_arg(argv, "mode=")   || 
               !str_arg(argv, "tc=")   || !str_arg(argv, "ssid=")) {
                MTDM_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                continue;
            }
            bssid  = str_arg(argv, "bssid=");
            sec    = str_arg(argv, "sec=");
            cipher = str_arg(argv, "cipher=");
            last   = str_arg(argv, "last=");
            ssid   = str_arg(argv, "ssid=");
            ssid   = wl_aplist_info_ssid + (ssid - wl_aplist_info);
            if (strlen(ssid) == 0) {
                MTDM_LOGGER(this, LOG_INFO, "%s: skip, ssid is empty", __func__);
                continue;
            }

            MTDM_LOGGER(this, LOG_INFO, "%s: bssid  is [%s]", __func__, bssid);
            MTDM_LOGGER(this, LOG_INFO, "%s: sec    is [%s]", __func__, sec);
            MTDM_LOGGER(this, LOG_INFO, "%s: cipher is [%s]", __func__, cipher);
            MTDM_LOGGER(this, LOG_INFO, "%s: last   is [%s]", __func__, last);
            MTDM_LOGGER(this, LOG_INFO, "%s: ssid   is [%s]", __func__, ssid);

            fprintf(fp, "network={ \n");
            fprintf(fp, "ssid=\"%s\"\n", ssid);
            fprintf(fp, "bssid=%s\n", bssid);
            #ifdef SPEEDUP_WLAN_CONF
            int len;
            fprintf(fp, "scan_ssid=1\n");
            fprintf(fp, "key_mgmt=NONE WPA-PSK\n");
            fprintf(fp, "pairwise=TKIP CCMP\n");
            if (strlen(pass) == 64) {
                fprintf(fp, "psk=%s\n", pass);
            }
            else {
                fprintf(fp, "psk=\"%s\"\n", pass);
            }
            fprintf(fp, "wep_tx_keyidx=0\n");
            len = strlen(pass);
            if ((len == 5) || (len == 13)) {
                fprintf(fp, "wep_key0=\"%s\"\n", pass);
                fprintf(fp, "wep_key1=\"%s\"\n", pass);
                fprintf(fp, "wep_key2=\"%s\"\n", pass);
                fprintf(fp, "wep_key3=\"%s\"\n", pass);
            }
            else {
                fprintf(fp, "wep_key0=%s\n", pass);
                fprintf(fp, "wep_key1=%s\n", pass);
                fprintf(fp, "wep_key2=%s\n", pass);
                fprintf(fp, "wep_key3=%s\n", pass);
            }
            #else
            if (!strcmp(sec, "none")) {
                fprintf(fp, "key_mgmt=NONE\n");
            }
            else if (!strcmp(sec, "wep")) {
		int len;
                fprintf(fp, "key_mgmt=NONE\n");
                len = strlen(pass);
                if ((len == 5) || (len == 13)) {
			fprintf(fp, "wep_key0=\"%s\"\n", pass);
                	fprintf(fp, "wep_key1=\"%s\"\n", pass);
                	fprintf(fp, "wep_key2=\"%s\"\n", pass);
                	fprintf(fp, "wep_key3=\"%s\"\n", pass);
                }
                else {
			fprintf(fp, "wep_key0=%s\n", pass);
                	fprintf(fp, "wep_key1=%s\n", pass);
                	fprintf(fp, "wep_key2=%s\n", pass);
                	fprintf(fp, "wep_key3=%s\n", pass);
                }
                fprintf(fp, "wep_tx_keyidx=0\n");
            }
            else {
                fprintf(fp, "key_mgmt=WPA-PSK\n");
                fprintf(fp, "pairwise=%s\n", cipher);
                if (strlen(pass) == 64) {
                    fprintf(fp, "psk=%s\n", pass);
                }
                else {
                    fprintf(fp, "psk=\"%s\"\n", pass);
                }
            }
            #endif
            if (!strcmp(last, "1")) {
                fprintf(fp, "priority=1\n");
            }

            if(cdb_get_int("$wl_sta_no_sgi", 0)) {
                fprintf(fp, "disable_sgi=1\n");
            }
            if(cdb_get_int("$wl_sta_20_only", 0)) {
                fprintf(fp, "disable_ht40=1\n");
            }
            fprintf(fp, "}\n");

            if (cdb_get_int("$omi_result", 0) == OMNICFG_R_CONNECTING) {
                /* it is under omnicfg apply, only add one rule into aplist
                 */
                break;
            }
        }
    }
}

static void config_bss_cipher(FILE *fp, int bss, WLRole role)
{
    char wl_bss_bssid2[MBUF_LEN] = { 0 };
    char wl_bss_bssid3[MBUF_LEN] = { 0 };
    int wl_bss_sec_type = cdb_get_array_int("$wl_bss_sec_type", bss, 0);

    MTDM_LOGGER(this, LOG_INFO, "%s: config bss[%d] role[%d]", __func__, bss, role);
    MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_sec_type%d is [%d]", __func__, bss, wl_bss_sec_type);

    if (bss != 1) {
        if (role == wlRole_AP) {
            if (cdb_get_array_str("$wl_bss_bssid", 3, wl_bss_bssid3, sizeof(wl_bss_bssid3), NULL) == NULL) {
                MTDM_LOGGER(this, LOG_ERR, "%s: wl_bss_bssid3 is unknown", __func__);
            }
            else {
                MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_bssid3 is [%s]", __func__, wl_bss_bssid3);
                fprintf(fp, "bssid=%s\n", wl_bss_bssid3);
            }
        }
        else if (role == wlRole_STA) {
            if (cdb_get_array_str("$wl_bss_bssid", 2, wl_bss_bssid2, sizeof(wl_bss_bssid2), NULL) == NULL) {
                MTDM_LOGGER(this, LOG_ERR, "%s: wl_bss_bssid2 is unknown", __func__);
            }
            else {
                MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_bssid2 is [%s]", __func__, wl_bss_bssid2);
                fprintf(fp, "bssid=%s\n", wl_bss_bssid2);
            }
        }
    }
    if (wl_bss_sec_type != 0) {
        if (wl_bss_sec_type & AUTH_CAP_WPS) {
            if (role == wlRole_AP) {
                char wl_wps_def_pin[SSBUF_LEN] = { 0 };
                int wl_bss_wps_state = cdb_get_array_int("$wl_bss_wps_state", bss, 0);

                fprintf(fp, "wps_state=%d\n", wl_bss_wps_state);
                fprintf(fp, "eap_server=1\n");
//                fprintf(fp, "ap_setup_locked=1\n");
                fprintf(fp, "device_name=Cheetah AP\n");
                fprintf(fp, "manufacturer=Montage\n");
                fprintf(fp, "model_name=Cheetah\n");
                fprintf(fp, "model_number=3280\n");
                fprintf(fp, "serial_number=12345\n");
                fprintf(fp, "device_type=6-0050F204-1\n");
                fprintf(fp, "os_version=01020300\n");
                fprintf(fp, "config_methods=label virtual_display virtual_push_button keypad\n");
                fprintf(fp, "upnp_iface=br0\n");

                cdb_get_str("$wl_wps_def_pin", wl_wps_def_pin, sizeof(wl_wps_def_pin), "");
                if (strlen(wl_wps_def_pin) == 0) {
                    if (boot_cdb_get("pin", wl_wps_def_pin) == 1) {
                        cdb_set("$wl_wps_def_pin", wl_wps_def_pin);
                    }
                }
                MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_wps_state%d is [%d]", __func__, bss, wl_bss_wps_state);
                MTDM_LOGGER(this, LOG_INFO, "%s: wl_wps_def_pin is [%s]", __func__, wl_wps_def_pin);
                fprintf(fp, "ap_pin=%s\n", wl_wps_def_pin);
            }
            else {
                if ((wl_bss_sec_type & ~AUTH_CAP_WPS) <= AUTH_CAP_OPEN) {
                    fprintf(fp, "key_mgmt=NONE\n");
                }
            }
        }

        if (wl_bss_sec_type & AUTH_CAP_WEP) {
            char wl_bss_wep_key[MSBUF_LEN] = { 0 };
            char wl_bss_wep_keyname[USBUF_LEN];
            int wl_bss_wep_index = cdb_get_array_int("$wl_bss_wep_index", bss, 0);
            int len, i;
            if (role == wlRole_AP) {
                fprintf(fp, "wep_default_key=%d\n", wl_bss_wep_index);
            }
            else {
                fprintf(fp, "key_mgmt=NONE\n");
                fprintf(fp, "wep_tx_keyidx=%d\n", wl_bss_wep_index);
            }
            MTDM_LOGGER(this, LOG_INFO, "%s: WEP:", __func__);
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_wep_index%d is [%d]", __func__, bss, wl_bss_wep_index);
            for (i=1;i<5;i++) {
                sprintf(wl_bss_wep_keyname, "$wl_bss_wep_%dkey", i);
                if (cdb_get_array_str(wl_bss_wep_keyname, 
                    bss, wl_bss_wep_key, sizeof(wl_bss_wep_key), NULL) != NULL) {
                    len = strlen(wl_bss_wep_key);
                    if ((len == 5) || (len == 13)) {
                        fprintf(fp, "wep_key%d=\"%s\"\n", (i-1), wl_bss_wep_key);
                    }
                    else {
                        fprintf(fp, "wep_key%d=%s\n", (i-1), wl_bss_wep_key);
                    }
                    MTDM_LOGGER(this, LOG_INFO, "%s: %s is [%s]", __func__, wl_bss_wep_keyname, wl_bss_wep_key);
                }
            }
            if ((role == wlRole_AP) && (wl_bss_sec_type & AUTH_CAP_SHARED)) {
                fprintf(fp, "auth_algs=2\n");
            }
        }

        if (wl_bss_sec_type & (AUTH_CAP_WPA | AUTH_CAP_WPA2)) {
            char wl_bss_wpa_psk[MSBUF_LEN] = { 0 };
            int wl_bss_wpa_rekey = cdb_get_array_int("$wl_bss_wpa_rekey", bss, 0);
            int wl_bss_cipher = cdb_get_array_int("$wl_bss_cipher", bss, 0);
            int wl_bss_key_mgt = cdb_get_array_int("$wl_bss_key_mgt", bss, 0);
            cdb_get_array_str("$wl_bss_wpa_psk", bss, wl_bss_wpa_psk, sizeof(wl_bss_wpa_psk), "");
            if (wl_bss_key_mgt > 0) {
                fprintf(fp, "ieee80211x=1\n");
                fprintf(fp, "eapol_key_index_workaround=0\n");
                fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");
            }
            else {
                if (role == wlRole_AP) {
                    fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
                }
                else {
                    fprintf(fp, "key_mgmt=WPA-PSK\n");
                }
            }
            if (role == wlRole_AP) {
                if ((wl_bss_sec_type & (AUTH_CAP_WPA | AUTH_CAP_WPA2)) == (AUTH_CAP_WPA | AUTH_CAP_WPA2)) {
                    fprintf(fp, "wpa=3\n");
                }
                else if (wl_bss_sec_type & AUTH_CAP_WPA2) {
                    fprintf(fp, "wpa=2\n");
                }
                else if (wl_bss_sec_type & AUTH_CAP_WPA) {
                    fprintf(fp, "wpa=1\n");
                }
                fprintf(fp, "wpa_pairwise=");
                if (wl_bss_cipher & AUTH_CAP_CIPHER_TKIP) {
                    fprintf(fp, "TKIP ");
                }
                if (wl_bss_cipher & AUTH_CAP_CIPHER_CCMP) {
                    fprintf(fp, "CCMP ");
                }
                fprintf(fp, "\n");
            }
            else {
                fprintf(fp, "pairwise=");
                if (wl_bss_cipher & AUTH_CAP_CIPHER_TKIP) {
                    fprintf(fp, "TKIP ");
                }
                if (wl_bss_cipher & AUTH_CAP_CIPHER_CCMP) {
                    fprintf(fp, "CCMP ");
                }
                fprintf(fp, "\n");
            }
            if (wl_bss_key_mgt > 0) {
                char wl_bss_radius_svr[MSBUF_LEN] = { 0 };
                char wl_bss_radius_svr_key[MSBUF_LEN] = { 0 };
                int wl_bss_radius_svr_port = cdb_get_array_int("$wl_bss_radius_svr_port", bss, 0);
                cdb_get_array_str("$wl_bss_radius_svr", bss, wl_bss_radius_svr, sizeof(wl_bss_radius_svr), "");
                cdb_get_array_str("$wl_bss_radius_svr_key", bss, wl_bss_radius_svr_key, sizeof(wl_bss_radius_svr_key), "");
                fprintf(fp, "own_ip_addr=127.0.0.1\n");
                fprintf(fp, "auth_server_addr=%s\n", wl_bss_radius_svr);
                fprintf(fp, "auth_server_port=%d\n", wl_bss_radius_svr_port);
                fprintf(fp, "auth_server_shared_secret=%s\n", wl_bss_radius_svr_key);
            }
            else {
                if (role == wlRole_AP) {
                    if (strlen(wl_bss_wpa_psk) == 64) {
                        fprintf(fp, "wpa_psk=%s\n", wl_bss_wpa_psk);
                    }
                    else {
                        fprintf(fp, "wpa_passphrase=%s\n", wl_bss_wpa_psk);
                    }
                }
                else {
                    if (strlen(wl_bss_wpa_psk) == 64) {
                        fprintf(fp, "psk=%s\n", wl_bss_wpa_psk);
                    }
                    else {
                        fprintf(fp, "psk=\"%s\"\n", wl_bss_wpa_psk);
                    }
                }
            }
            if (role == wlRole_AP) {
                fprintf(fp, "wpa_group_rekey=%d\n", wl_bss_wpa_rekey);
            }
            MTDM_LOGGER(this, LOG_INFO, "%s: %s:", __func__, (wl_bss_sec_type & AUTH_CAP_WPA) ? "WPA" : "WPA2");
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_cipher%d is [%d]", __func__, bss, wl_bss_cipher);
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_key_mgt%d is [%d]", __func__, bss, wl_bss_key_mgt);
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_wpa_psk%d is [%s]", __func__, bss, wl_bss_wpa_psk);
            MTDM_LOGGER(this, LOG_INFO, "%s: wpa_group_rekey%d is [%d]", __func__, bss, wl_bss_wpa_rekey);
        }

        if (wl_bss_sec_type & AUTH_CAP_WAPI) {
            char wl_bss_wpa_psk[MSBUF_LEN] = { 0 };
            int wl_bss_wpa_rekey = cdb_get_array_int("$wl_bss_wpa_rekey", bss, 0);
            cdb_get_array_str("$wl_bss_wpa_psk", bss, wl_bss_wpa_psk, sizeof(wl_bss_wpa_psk), "");
            fprintf(fp, "wapi=1\n");
            if (role == wlRole_AP) {
                fprintf(fp, "wpa_passphrase=%s\n", wl_bss_wpa_psk);
                fprintf(fp, "wpa_group_rekey=%d\n", wl_bss_wpa_rekey);
            }
            else {
                fprintf(fp, "psk=\"%s\"\n", wl_bss_wpa_psk);
                // Unneccessary apply ptk/group rekey parameter to STA, 
                // rekey mechanism created by AP.
            }
            MTDM_LOGGER(this, LOG_INFO, "%s: WAPI:", __func__);
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_wpa_psk%d is [%s]", __func__, bss, wl_bss_wpa_psk);
            MTDM_LOGGER(this, LOG_INFO, "%s: wpa_group_rekey%d is [%d]", __func__, bss, wl_bss_wpa_rekey);
        }
    }
    else {
        if (role == wlRole_STA) {
            fprintf(fp, "key_mgmt=NONE\n");
        }
    }
}

#ifdef SPEEDUP_WLAN_CONF
#include <errno.h>
#define PROCESS_NOT_EXISTED 0
/*
    Return pid if wpa_supplicant is running.
    otherwise, return 0.
*/
static int check_process_running(char *pid_file)
{
    int ret = PROCESS_NOT_EXISTED;
    if( f_exists(pid_file) ) {
        int pid = read_pid_file(pid_file);
        if (pid == UNKNOWN) {
            return PROCESS_NOT_EXISTED;
        }
        /*
           http://lib.ru/UNIXFAQ/unixprogrfaq.txt Section 1.9
           send sig 0 via kill to check if running.
         */
        ret = kill(pid, 0);
        if (ret == 0) {
            printf("pid existed\n\n");
            return pid;
        }
        else if(ret == -1 && errno == ESRCH) {
            return PROCESS_NOT_EXISTED;
        }
        else {
            MTDM_LOGGER(this, LOG_WARNING, "%s: with problem [%d]", __func__, errno);
            // should we try to kill hostapd?
            // we should not enter this section
            return PROCESS_NOT_EXISTED;
        }
    }
    return PROCESS_NOT_EXISTED;
}

static int mac80211_wpasup_setup_empty_file(void) {
    FILE *fp;
    fp = fopen(WPASUP_CFGFILE_TMP, "w");
    MTDM_LOGGER(this, LOG_INFO, "%s:[%d]", __func__, __LINE__);
    if (fp) {
        fclose(fp);
        rename(WPASUP_CFGFILE_TMP, WPASUP_CFGFILE);
    }
    return 0;
}

#endif
static int check_hostapd_terminated(void)
{
    int cnt = HOSTAPD_MAX_CHK_CNT;

    if( f_exists(HOSTAPD_PIDFILE) ) {
        while (1) {
            int pid = read_pid_file(HOSTAPD_PIDFILE);
            if ((cnt == 0) || (pid == UNKNOWN) || (kill(pid, SIGTERM) != 0))
                break;
            sleep(1);
            MTDM_LOGGER(this, LOG_INFO, "%s: (%d)hostapd pid [%d]", __func__, cnt--, pid);
        }
    }
    return 0;
}

static int check_wpasup_terminated(void)
{
    int cnt = WPASUP_MAX_CHK_CNT;

    if( f_exists(WPASUP_PIDFILE) ) {
        while (1) {
            int pid = read_pid_file(WPASUP_PIDFILE);
            if ((cnt == 0) || (pid == UNKNOWN) || (kill(pid, SIGTERM) != 0))
                break;
            sleep(1);
            MTDM_LOGGER(this, LOG_INFO, "%s: (%d)wpa_supplicant pid [%d]", __func__, cnt--, pid);
        }
    }
    return 0;
}

static int mac80211_hostapd_setup_base(void)
{
    char wl_bss_ssid1[MSBUF_LEN] = { 0 };
    char wl_region[USBUF_LEN] = { 0 };
    char hostapd_acl[MSBUF_LEN] = { 0 };
    char basic_rates[USBUF_LEN] = { 0 };
    char base_cfg[LSBUF_LEN] = { 0 };
    char ht_capab[SSBUF_LEN] = { 0 };
    char bss1_cfg[SSBUF_LEN] = { 0 };
#ifdef HAVE_MULTIPLE_BSSID
    char bss2_cfg[SSBUF_LEN] = { 0 };
#endif
    char hw_mode[USBUF_LEN] = { 0 };
    int wl_beacon_listen = cdb_get_int("$wl_beacon_listen", -1);
    int wl_macf_mode = cdb_get_int("$wl_macf_mode", 0);
    int wl_preamble = 0;
    int wl_bb_mode = 0;
    int wl_channel = 0;
    int wl_2choff = 0;
    int wl_sgi = 0;
    int wl_tbtt = 0;
    int wl_dtim = 0;
    int wl_apsd = 0;
    int wl_frag = 0;
    int wl_rts = 0;

    unsigned char val[6];
    char buf[MSBUF_LEN] = { 0 };
    char *ptr;
    FILE *fp;
    int num;
    int i;

    cdb_get_str("$wl_region", wl_region, sizeof(wl_region), "CN");

    cdb_get_str("$wl_bss_ssid1", wl_bss_ssid1, sizeof(wl_bss_ssid1), "");
    if(0==strlen(wl_bss_ssid1))
    {
        cdb_get_str("$wl_ap_name_prefix", wl_bss_ssid1, sizeof(wl_bss_ssid1), "Panther_");
        cdb_get_str("$wl_ap_name_rule", buf, sizeof(buf), "3");

        my_mac2val(val, mtdm->boot.MAC0);
        num = strtoul(buf, NULL, 0);
        if(num > sizeof(val))
           num = sizeof(val);
        if(num > 0)
        {
            ptr = &wl_bss_ssid1[strlen(wl_bss_ssid1)];
            for(i=(sizeof(val)-num);i<sizeof(val);i++)
            {
                sprintf(ptr, "%2.2x", val[i]);
                ptr+=2;
            }
        }
    }
    
    //Channel
    wl_channel = cdb_get_int("$wl_channel", 0);

    // basic_rates
    num = cdb_get_int("$wl_basic_rates", 0);
    if ((num == 0) || (num == 127)) {
        sprintf(basic_rates, "10 20 55 60 110 120 240");
    }
    else if (num == 3) {
        sprintf(basic_rates, "10 20");
    }
    else if (num == 16) {
        sprintf(basic_rates, "10 20 55 110");
    }

    // wl_apsd
    if ((wl_apsd = cdb_get_int("$wl_ps_mode", 0)) == 2) {
        wl_apsd = 1;
    }
    else {
        wl_apsd = 0;
    }

    // wl_preamble
    if ((wl_preamble = cdb_get_int("$wl_preamble", 1)) == 0) {
        wl_preamble = 1;
    }
    else {
        wl_preamble = 0;
    }

    // wl_tbtt
    if ((wl_tbtt = cdb_get_int("$wl_tbtt", 100)) < 15) {
        wl_tbtt = 15;
    }
    else if (wl_tbtt > 65535) {
        wl_tbtt = 65535;
    }

    // wl_dtim
    if ((wl_dtim = cdb_get_int("$wl_dtim", 2)) < 1) {
        wl_dtim = 1;
    }
    else if (wl_dtim > 255) {
        wl_dtim = 255;
    }

    // wl_rts
    if ((wl_rts = cdb_get_int("$wl_rts", 2347)) < 256) {
        wl_rts = 256;
    }
    else if (wl_rts > 2347) {
        wl_rts = 2347;
    }

    // wl_frag
    if ((wl_frag = cdb_get_int("$wl_frag", 2346)) < 256) {
        wl_frag = 256;
    }
    else if (wl_frag > 2346) {
        wl_frag = 2346;
    }

    // base_cfg, ht_capab
    if ((wl_bb_mode = cdb_get_int("$wl_bb_mode", BB_MODE_G)) == BB_MODE_B) {
        sprintf(hw_mode, "b");
    }
    else {
        sprintf(hw_mode, "g");
    }
    ptr = base_cfg;
    if (wl_bb_mode >= BB_MODE_N) {
        wl_2choff = cdb_get_int("$wl_2choff", 0);
        wl_sgi = cdb_get_int("$wl_sgi", 0);

        ptr = ht_capab;
        switch (wl_2choff) {
            case 0:
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20]");
                }
                break;
            case 1:
                ptr += sprintf(ptr, "[HT40+]");
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            case 2:
                ptr += sprintf(ptr, "[HT40-]");
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            case 3:
                if (wl_channel == 0) {
                    ptr += sprintf(ptr, "[HT40+]");
                }
                else {
                    ptr += sprintf(ptr, "[HT40-AUTO]");
                }
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            default:
                break;
        }

        if (cdb_get_int("$wl_ht_mode", 0) == 1) {
            ptr += sprintf(ptr, "[GF]");
        }
        //ptr += sprintf(ptr, "[SMPS-STATIC]");


        ptr = base_cfg;
        ptr += sprintf(ptr, "ieee80211n=1\n");
        ptr += sprintf(ptr, "ht_capab=%s\n", ht_capab);
        if (cdb_get_int("$wl_2040_coex", 0) == 1) {
            ptr += sprintf(ptr, "noscan=0\n");
            ptr += sprintf(ptr, "2040_coex_en=1\n");
        }
        else if (wl_beacon_listen != 1) {
            ptr += sprintf(ptr, "noscan=1\n");
        }
    }
    // ptr here, it should be point to base_cfg
    if (wl_beacon_listen != -1) {
        ptr += sprintf(ptr, "obss_survey_en=%d\n", wl_beacon_listen);
    }

    // bss1_cfg, wds
    ptr = bss1_cfg;
    if (cdb_get_int("$wl_wds_mode", 0) == 1) {
        ptr += sprintf(ptr, "ssid=___wds_bridge___\n");
        ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
    }
    else {
        ptr += sprintf(ptr, "ssid=%s\n", wl_bss_ssid1);
        if (cdb_get_int("$wl_bss_ssid_hidden1", 0) == 1) {
            ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
        }
        else {
            ptr += sprintf(ptr, "ignore_broadcast_ssid=0\n");
        }
    }

#ifdef HAVE_MULTIPLE_BSSID
    // bss2_cfg, multiple bssid
    ptr = bss2_cfg;
    if (cdb_get_int("$wl_bss_enable3", 0) == 1) {
        if (cdb_get_int("$wl_bss_role3", 0) & wlRole_AP) {
            cdb_get_array_str("$wl_bss_ssid", 3, ssid, sizeof(ssid), "");
            ptr += sprintf(ptr, "bss=wlan1\n");
            ptr += sprintf(ptr, "ssid=%s\n", ssid);
            if (cdb_get_int("$wl_bss_ssid_hidden3", 0) == 1) {
                ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
            }
#warning "not implement yet"
            config_bss_cipher(0, 3, wlRole_AP); 
        }
    }
#endif

    // hostapd_acl
    ptr = hostapd_acl;
    if (wl_macf_mode == 0) {
        ptr += sprintf(ptr, "macaddr_acl=0\n");
    }
    else if (wl_macf_mode == 1) {
        ptr += sprintf(ptr, "macaddr_acl=0\n");
        ptr += sprintf(ptr, "deny_mac_file=%s\n", HOSTAPD_ACLFILE);
    }
    else if (wl_macf_mode == 2) {
        ptr += sprintf(ptr, "macaddr_acl=1\n");
        ptr += sprintf(ptr, "accept_mac_file=%s\n", HOSTAPD_ACLFILE);
    }
    config_hostapd_acl(wl_macf_mode);

    // really 
    #ifdef SPEEDUP_WLAN_CONF
    fp = fopen(HOSTAPD_CFGFILE_TMP, "w");
    #else
    fp = fopen(HOSTAPD_CFGFILE, "w");
    #endif
    if (fp) {
        fprintf(fp, "interface=%s\n", mtdm->rule.AP);
        fprintf(fp, "driver=nl80211\n");
        fprintf(fp, "fragm_threshold=%d\n", wl_frag);
        fprintf(fp, "rts_threshold=%d\n", wl_rts);
        fprintf(fp, "beacon_int=%d\n", wl_tbtt);
        fprintf(fp, "dtim_period=%d\n", wl_dtim);
        fprintf(fp, "preamble=%d\n", wl_preamble);
        fprintf(fp, "uapsd_advertisement_enabled=%d\n", wl_apsd);
        fprintf(fp, "channel=%d\n", wl_channel);
        fprintf(fp, "hw_mode=%s\n", hw_mode);
        fprintf(fp, "basic_rates=%s\n", basic_rates);
        fprintf(fp, "%s\n", hostapd_acl);
        fprintf(fp, "wmm_enabled=%d\n", cdb_get_int("$wl_bss_wmm_enable1", 0));
        fprintf(fp, "wmm_ac_bk_cwmin=4\n");
        fprintf(fp, "wmm_ac_bk_cwmax=10\n");
        fprintf(fp, "wmm_ac_bk_aifs=7\n");
        fprintf(fp, "wmm_ac_bk_txop_limit=0\n");
        fprintf(fp, "wmm_ac_bk_acm=0\n");
        fprintf(fp, "wmm_ac_be_aifs=3\n");
        fprintf(fp, "wmm_ac_be_cwmin=4\n");
        fprintf(fp, "wmm_ac_be_cwmax=10\n");
        fprintf(fp, "wmm_ac_be_txop_limit=0\n");
        fprintf(fp, "wmm_ac_be_acm=0\n");
        fprintf(fp, "wmm_ac_vi_aifs=2\n");
        fprintf(fp, "wmm_ac_vi_cwmin=3\n");
        fprintf(fp, "wmm_ac_vi_cwmax=4\n");
        fprintf(fp, "wmm_ac_vi_txop_limit=94\n");
        fprintf(fp, "wmm_ac_vi_acm=0\n");
        fprintf(fp, "wmm_ac_vo_aifs=2\n");
        fprintf(fp, "wmm_ac_vo_cwmin=2\n");
        fprintf(fp, "wmm_ac_vo_cwmax=3\n");
        fprintf(fp, "wmm_ac_vo_txop_limit=47\n");
        fprintf(fp, "wmm_ac_vo_acm=0\n");
        fprintf(fp, "tx_queue_data3_aifs=7\n");
        fprintf(fp, "tx_queue_data3_cwmin=15\n");
        fprintf(fp, "tx_queue_data3_cwmax=1023\n");
        fprintf(fp, "tx_queue_data3_burst=0\n");
        fprintf(fp, "tx_queue_data2_aifs=3\n");
        fprintf(fp, "tx_queue_data2_cwmin=15\n");
        fprintf(fp, "tx_queue_data2_cwmax=63\n");
        fprintf(fp, "tx_queue_data2_burst=0\n");
        fprintf(fp, "tx_queue_data1_aifs=1\n");
        fprintf(fp, "tx_queue_data1_cwmin=7\n");
        fprintf(fp, "tx_queue_data1_cwmax=15\n");
        fprintf(fp, "tx_queue_data1_burst=3.0\n");
        fprintf(fp, "tx_queue_data0_aifs=1\n");
        fprintf(fp, "tx_queue_data0_cwmin=3\n");
        fprintf(fp, "tx_queue_data0_cwmax=7\n");
        fprintf(fp, "tx_queue_data0_burst=1.5\n");
        fprintf(fp, "country_code=%s\n", wl_region);
        fprintf(fp, "ieee80211d=1\n");
        fprintf(fp, "%s\n", base_cfg);
        fprintf(fp, "%s\n", bss1_cfg);
        fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
        config_bss_cipher(fp, 1, wlRole_AP);
#ifdef HAVE_MULTIPLE_BSSID
        fprintf(fp, "%s\n", bss2_cfg);
        config_bss_cipher(fp, 3, wlRole_AP);
#endif
#ifdef CONFIG_WLAN_AP_INACTIVITY_TIME_30
        fprintf(fp, "ap_max_inactivity=30\n");
#endif
        fclose(fp);
        #ifdef SPEEDUP_WLAN_CONF
        rename(HOSTAPD_CFGFILE_TMP, HOSTAPD_CFGFILE);
        #endif
    }
    else {
        MTDM_LOGGER(this, LOG_ERR, "%s: open [%s] fail", __func__, HOSTAPD_CFGFILE);
    }

    return 0;
}

static int mac80211_wpasup_setup_base(void)
{
    char wl_bss_ssid2[MSBUF_LEN] = { 0 };
    char wl_region[USBUF_LEN] = { 0 };
    u8 scan_interval = (mtdm->rule.OMODE == opMode_wi) ? 5 : 0;
    FILE *fp;
#ifdef SPEEDUP_WLAN_CONF
	char wl_passphase[MSBUF_LEN] = { 0 };
    int len;
#endif
    cdb_get_str("$wl_region", wl_region, sizeof(wl_region), "US");
    MTDM_LOGGER(this, LOG_INFO, "%s: wl_region is [%s]", __func__, wl_region);

    if (cdb_get_int("$wl_aplist_en", 0) == 1) {
        MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_en is enabled", __func__);

        exec_cmd("aplist");
        #ifdef SPEEDUP_WLAN_CONF
        fp = fopen(WPASUP_CFGFILE_TMP, "w");
        #else
        fp = fopen(WPASUP_CFGFILE, "w");
        #endif
        if (fp) {
            fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
            config_ap_list(fp);
            fprintf(fp, "scan_interval=%u\n", scan_interval);
            fprintf(fp, "country=%s\n", wl_region);
            fclose(fp);
            #ifdef SPEEDUP_WLAN_CONF
            rename(WPASUP_CFGFILE_TMP, WPASUP_CFGFILE);
            #endif
        }
    }
    else {
        MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_en is disabled", __func__);

        if (cdb_get_array_str("$wl_bss_ssid", 2, wl_bss_ssid2, sizeof(wl_bss_ssid2), NULL) == NULL) {
            MTDM_LOGGER(this, LOG_ERR, "%s: wl_bss_ssid2 is unknown", __func__);
        }
        else {
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_bss_ssid2 is [%s]", __func__, wl_bss_ssid2);
        }

#ifdef SPEEDUP_WLAN_CONF
        if (cdb_get_array_str("$wl_passphase", 2, wl_passphase, sizeof(wl_passphase), NULL) == NULL) {
            MTDM_LOGGER(this, LOG_ERR, "%s: wl_passphase is unknown", __func__);
        } else {
            MTDM_LOGGER(this, LOG_INFO, "%s: wl_passphase is [%s]", __func__, wl_passphase);
        }

        fp = fopen(WPASUP_CFGFILE_TMP, "w");
#else
        fp = fopen(WPASUP_CFGFILE, "w");
#endif
        if (fp) {
            fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
            fprintf(fp, "network={\n");
            fprintf(fp, "ssid=\"%s\"\n", wl_bss_ssid2);
#ifndef SPEEDUP_WLAN_CONF
            config_bss_cipher(fp, 2, wlRole_STA);
#else
            fprintf(fp, "scan_ssid=1\n");
            fprintf(fp, "bssid=\n");
            len = strlen(wl_passphase);
            fprintf(fp, "key_mgmt=NONE WPA-PSK\n");
            fprintf(fp, "pairwise=TKIP CCMP\n");
            if (len == 64) {
                fprintf(fp, "psk=%s\n", wl_passphase);
            } else {
                fprintf(fp, "psk=\"%s\"\n", wl_passphase);
            }
            fprintf(fp, "wep_tx_keyidx=0\n");
            if ((len == 0) || (len == 5) || (len == 13)) {
                fprintf(fp, "wep_key0=\"%s\"\n", wl_passphase);
                fprintf(fp, "wep_key1=\"%s\"\n", wl_passphase);
                fprintf(fp, "wep_key2=\"%s\"\n", wl_passphase);
                fprintf(fp, "wep_key3=\"%s\"\n", wl_passphase);
            } else {
                fprintf(fp, "wep_key0=%s\n", wl_passphase);
                fprintf(fp, "wep_key1=%s\n", wl_passphase);
                fprintf(fp, "wep_key2=%s\n", wl_passphase);
                fprintf(fp, "wep_key3=%s\n", wl_passphase);
            }
#endif
            if(cdb_get_int("$wl_sta_no_sgi", 0)) {
                fprintf(fp, "disable_sgi=1\n");
            }
            if(cdb_get_int("$wl_sta_20_only", 0)) {
                fprintf(fp, "disable_ht40=1\n");
            }
            fprintf(fp, "}\n");
            fprintf(fp, "scan_interval=%u\n", scan_interval);
            fprintf(fp, "country=%s\n", wl_region);
            fclose(fp);
            #ifdef SPEEDUP_WLAN_CONF
            rename(WPASUP_CFGFILE_TMP, WPASUP_CFGFILE);
            #endif
        }
        else {
            MTDM_LOGGER(this, LOG_ERR, "%s: open [%s] fail", __func__, WPASUP_CFGFILE);
        }
    }

    return 0;
}

static int mac80211_p2p_setup_base(void)
{
    FILE *fp;

    fp = fopen(WPASUP_CFGFILE, "w");
    if (fp) {
        fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
        fprintf(fp, "ap_scan=1\n");
        fprintf(fp, "fast_reauth=1\n");
        fprintf(fp, "device_name=montage_p2p\n");
        fprintf(fp, "device_type=1-0050F204-1\n");
        fprintf(fp, "config_methods= virtual_display virtual_push_button keypad\n");
        fprintf(fp, "p2p_listen_reg_class=81\n");
        fprintf(fp, "p2p_listen_channel=11\n");
        fprintf(fp, "p2p_oper_reg_class=81\n");
        fprintf(fp, "p2p_oper_channel=11\n");
        fclose(fp);
    }
    return 0;
}

static void aplist_rollback_ap(int sData)
{
    if (sData) {
        /* it is under omnicfg apply
         * monitor omnicfg finish, if fail, to remove the first rule in aplist
         */
        if (cdb_get_int("$omi_result", 0) == OMNICFG_R_CONNECTING) {
            TimerMod(timerMtdmAPList, 1);
        }
        else {
            /* reserve time to update wanif_state, if wan is connected
             */
            sleep(3);

            if (cdb_get_int("$wanif_state", Wanif_State_DISCONNECTED) != Wanif_State_CONNECTED) {
                MTDM_LOGGER(this, LOG_INFO, "%s: Remove aplist first rule, omnicfg fail", __func__);
                exec_cmd("aplist r 1");
            }
            else {
                char wl_region[USBUF_LEN] = { 0 };
                u8 scan_interval = (mtdm->rule.OMODE == opMode_wi) ? 5 : 0;
                FILE *fp;
                cdb_get_str("$wl_region", wl_region, sizeof(wl_region), "US");
                fp = fopen(WPASUP_CFGFILE, "w");
                if (fp) {
                    fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
                    config_ap_list(fp);
                    fprintf(fp, "scan_interval=%u\n", scan_interval);
                    fprintf(fp, "country=%s\n", wl_region);
                    fclose(fp);
                }
                mtdm->misc.reconf_wpa = 1;
                MTDM_LOGGER(this, LOG_INFO, "%s: Re-create new wpa.conf for aplist", __func__);
                MTDM_LOGGER(this, LOG_INFO, "%s: Timeout wan is connected, omnicfg ok", __func__);
            }
        }
    }
    else {
        if (cdb_get_int("$wanif_state", Wanif_State_DISCONNECTED) != Wanif_State_CONNECTED) {
            if (cdb_get_int("$op_work_mode", opMode_ap) != opMode_ap) {
#if !defined(__PANTHER__)   /* these code were removed for avoiding rollback to AP mode */
                MTDM_LOGGER(this, LOG_INFO, "%s: Timeout rollback to AP mode", __func__);
                cdb_set_int("$op_work_mode", opMode_ap);
                #ifdef SPEEDUP_WLAN_CONF
                mac80211_wpasup_setup_empty_file();
                #endif
                exec_cmd("mtc commit");
#endif
            }
            else {
                MTDM_LOGGER(this, LOG_INFO, "%s: Timeout now AP mode", __func__);
                #ifdef SPEEDUP_WLAN_CONF
                mtdm_wlhup();
                #endif
            }
        }
        else {
            MTDM_LOGGER(this, LOG_INFO, "%s: Timeout wan is connected", __func__);
        }
    }
}

void aplist_monitor_timer_enable(int from_omnicfg, int sec, char *msg)
{
    #ifdef SPEEDUP_WLAN_CONF
    if (cdb_get_int("$wl_aplist_en", 0) == 1 && mtdm->rule.OMODE != opMode_ap) {
    #else
    if (cdb_get_int("$wl_aplist_en", 0) == 1) {
    #endif
        MTDM_LOGGER(this, LOG_INFO, "%s: Run aplist timer %d seconds", msg, sec);
        TimerDel(timerMtdmAPList);
        TimerAdd(timerMtdmAPList, aplist_rollback_ap, from_omnicfg, sec);
    }
}

static int hostapd_cli_start(void)
{
    char wl_mac_gpio[SSBUF_LEN];
    char mypath[SSBUF_LEN];
    char mac[MBUF_LEN];
    u8 en, gpio;
    u8 val[6];
    FILE *fp;

//  $wl_mac_gpio=en=0&mac=00:32:80:00:11:22&gpio=11
    if (cdb_get_str("$wl_mac_gpio", wl_mac_gpio, sizeof(wl_mac_gpio), NULL) == NULL) {
        MTDM_LOGGER(this, LOG_INFO, "%s: No wl_mac_gpio cdb", __func__);
        return 2;
    }
    if (sscanf(wl_mac_gpio, "en=%hhu;mac=%hhx:%hhx:%hhx:%hhx:%hhx:%hhx;gpio=%hhu", 
            &en, &val[0], &val[1], &val[2], &val[3], &val[4], &val[5], &gpio) != 8) {
        MTDM_LOGGER(this, LOG_INFO, "%s: No wl_mac_gpio cdb", __func__);
        return 2;
    }
    if ( f_exists(HOSTAPD_CLI_PIDFILE) ) {
        MTDM_LOGGER(this, LOG_INFO, "%s: Already run pid, can't start again", __func__);
        return 1;
    }
    // 0 : disable, 1 : active high, 2 : active low
    if (en == 0) {
        return 0;
    }
    sprintf(mac, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x", val[0], val[1], val[2], val[3], val[4], val[5]);
    if (gpio >= 33) {
        MTDM_LOGGER(this, LOG_ERR, "%s: Incorrect setting: \"en=%d;mac=%s;gpio=%d\"", __func__, en, mac, gpio);
        return 2;
    }
    MTDM_LOGGER(this, LOG_INFO, "%s: Setup GPIO%d for %s", __func__, gpio, mac);
    sprintf(mypath, "%s/gpio%d", GPIO_PATH, gpio);
    if ( !f_isdir(mypath) ) {
        exec_cmd2("echo %d > /sys/class/gpio/export", gpio);
    }
    exec_cmd2("echo out > /sys/class/gpio/gpio%d/direction", gpio);
    exec_cmd2("echo %d > /sys/class/gpio/gpio%d/value", (en & 1) ? 1 : 0, gpio);

    fp = fopen(HOSTAPD_CLI_ACTIONFILE, "w");
    if (fp) {
        fprintf(fp, "#!/bin/ash\n");
        fprintf(fp, "#\n");
        fprintf(fp, "# cdb: en=%d;mac=%s;gpio=%d\n", en, mac, gpio);
        fprintf(fp, "#\n");
        fprintf(fp, "mode=%d\n", (en & 1) ? 1 : 0);
        fprintf(fp, "[ \"$3\" = \"`echo %s | tr A-Z a-z`\" ] && {\n", mac);
        fprintf(fp, "\t[ \"$2\" = \"AP-STA-CONNECTED\" ] && sw=0\n");
        fprintf(fp, "\t[ \"$2\" = \"AP-STA-DISCONNECTED\" ] && sw=1\n");
        fprintf(fp, "\ton=$(( $sw ^ $mode ))\n");
        fprintf(fp, "\techo $on > /sys/class/gpio/gpio%d/value\n", gpio);
        fprintf(fp, "}\n");
        fclose(fp);
        exec_cmd2("chmod a+x "HOSTAPD_CLI_ACTIONFILE);
        start_stop_daemon(HOSTAPD_CLI_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, HOSTAPD_CLI_PIDFILE, 
            "%s", HOSTAPD_CLI_CMDLINE);
        return 0;
    }
    else {
        MTDM_LOGGER(this, LOG_ERR, "%s: Create %s is failed", __func__, HOSTAPD_CLI_ACTIONFILE);
        return 2;
    }
}

static int start(void)
{
    int wl_enable = cdb_get_int("$wl_enable", 0);

    config_general();

    if ((mtdm->rule.OMODE != opMode_p2p)) {
        if(!iface_exist(mtdm->rule.STA)) {
            exec_cmd2("iw dev wlan0 interface add sta0 type managed");
            set_mac("sta0", mtdm->rule.STAMAC);
        }
    }
    if (wl_enable == 1) {
        if (mtdm->rule.HOSTAPD == 1) {
            mac80211_hostapd_setup_base();
#ifdef CONFIG_PACKAGE_omnicfg
            if (is_directmode()) {
                pid_t pid;
                if ((pid = get_pid_by_pidfile(HOSTAPD_PIDFILE, HOSTAPD_BIN)) != -1) {
                    kill(pid, SIGUSR2);
                }
            }
            else {
			#ifndef SPEEDUP_WLAN_CONF
            /* Pure AP mode need to make sure wpa_supplicant stop */
            if((mtdm->rule.OMODE == opMode_db) || (mtdm->rule.OMODE == opMode_ap) || (mtdm->rule.OMODE == opMode_wr))
                check_wpasup_terminated();
			#endif
            check_hostapd_terminated();

            MTDM_LOGGER(this, LOG_INFO, "%s: Run [%s]", __func__, HOSTAPD_BIN);
            start_stop_daemon(HOSTAPD_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                "%s", HOSTAPD_CMDLINE);

            }
#else
			#ifndef SPEEDUP_WLAN_CONF
            /* Pure AP mode need to make sure wpa_supplicant stop */
            if((mtdm->rule.OMODE == opMode_db) || (mtdm->rule.OMODE == opMode_ap) || (mtdm->rule.OMODE == opMode_wr))
                check_wpasup_terminated();
			#endif
            check_hostapd_terminated();

            MTDM_LOGGER(this, LOG_INFO, "%s: Run [%s]", __func__, HOSTAPD_BIN);
            start_stop_daemon(HOSTAPD_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                "%s", HOSTAPD_CMDLINE);
#endif
            exec_cmd2("brctl addif %s %s", mtdm->rule.BRAP, mtdm->rule.AP);
#ifdef HAVE_MULTIPLE_BSSID
            /* Multiple BSSID */
            if (0) {
                int wl_bss_enable3 = cdb_get_int("$wl_bss_enable3", 0);
                int wl_bss_role3 = cdb_get_int("$wl_bss_role3", wlRole_NONE);
                if ((wl_bss_enable3 == 1) && (wl_bss_role3 == wlRole_AP)) {
                    exec_cmd2("brctl addif %s wlan1", mtdm->rule.BRAP);
                }
            }
#endif
            hostapd_cli_start();
        }
        if (mtdm->rule.WPASUP == 1) {
            if (mtdm->rule.OMODE == opMode_p2p) {
                mac80211_p2p_setup_base();
            }
            #ifdef SPEEDUP_WLAN_CONF
            else if (mtdm->rule.OMODE == opMode_ap) {
                mac80211_wpasup_setup_empty_file();
				if(cdb_get_int("$network_caller", 0) == 0){
                	exec_cmd("/lib/wdk/omnicfg voice not_connected");
					cdb_set_int("$network_caller", 1); 
				}
            }
            #endif
            else {
                mac80211_wpasup_setup_base();
            }
#ifndef SPEEDUP_WLAN_CONF
            /* Pure Client mode need to make sure hostapd stop */
            if(mtdm->rule.OMODE == opMode_client)
                check_hostapd_terminated();
#endif
            check_wpasup_terminated();

#ifdef CONFIG_PACKAGE_omnicfg
            if (cdb_get_int("$omi_result", 0) == OMNICFG_R_CONNECTING) {
                /* under omnicfg apply, monitor it util omnicfg is finished
                 */
                aplist_monitor_timer_enable(1, 1, "wifi connecting(omnicfg)");
            }
            else {
                /* NOT under omnicfg apply, it is used in aplist
                 * start a timer of 20 seconds to determine rollback AP mode
                 */
                aplist_monitor_timer_enable(0, 20, "wifi connecting");
            }
#endif

            MTDM_LOGGER(this, LOG_INFO, "%s: Run [%s]", __func__, WPASUP_BIN);
            #ifdef SPEEDUP_WLAN_CONF
            if ((mtdm->rule.OMODE == opMode_ap) || 
                ((mtdm->rule.OMODE >= opMode_wi) && (mtdm->rule.OMODE <= opMode_br))) {
            #else
            if ((mtdm->rule.OMODE >= opMode_wi) && (mtdm->rule.OMODE <= opMode_br)) {
            #endif
                start_stop_daemon(WPASUP_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
#if defined(__PANTHER__)
                    WPASUP_CMDLINE" "WPASUP_HCTRL, mtdm->rule.STA, mtdm->rule.WANB);
#else
                    WPASUP_CMDLINE" "WPASUP_HCTRL, mtdm->rule.STA);
#endif
            }
            else {
                start_stop_daemon(WPASUP_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
#if defined(__PANTHER__)
                    WPASUP_CMDLINE" "WPASUP_HCTRL, mtdm->rule.STA, mtdm->rule.WANB);
#else
                    WPASUP_CMDLINE" "WPASUP_HCTRL, mtdm->rule.STA);
#endif
            }

            if ((mtdm->rule.OMODE != opMode_rp) && (mtdm->rule.OMODE != opMode_br) && 
                (mtdm->rule.OMODE != opMode_p2p)) {
                exec_cmd2("brctl addif %s %s", mtdm->rule.BRSTA, mtdm->rule.STA);
            }
            // P2P mode call the p2p_cli to handle the p2p progress
            if (mtdm->rule.OMODE == opMode_p2p) {
                start_stop_daemon("/usr/sbin/p2p_cli", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                    "-B -p %s", P2P_PIDFILE);
            }
        }else if(cdb_get_int("$network_caller", 0) == 0) {
			exec_cmd("/lib/wdk/omnicfg voice not_connected");
			cdb_set_int("$network_caller", 1); 
        }
    }

    return 0;
}

static int hostapd_cli_stop(void)
{
    char buf[BUF_LEN_8] = { 0 };
    u8 gpio;

    if( f_exists(HOSTAPD_CLI_ACTIONFILE) ) {
        exec_cmd3(buf, sizeof(buf), "sed -n 's/# cdb: //p' "HOSTAPD_CLI_ACTIONFILE" | awk -F';' '{gsub(/.*gpio=/,\"\",$0); print $0}'");
        if (buf[0] != 0) {
            gpio = strtoul(buf, NULL, 0); 
            exec_cmd2("echo in > /sys/class/gpio/gpio%d/direction", gpio);
            exec_cmd2("echo %d > /sys/class/gpio/unexport", gpio);
            MTDM_LOGGER(this, LOG_INFO, "%s: remove wl_mac_gpio, gpio=[%d]", __func__, gpio);
        }
        unlink(HOSTAPD_CLI_ACTIONFILE);
    }
    if( f_exists(HOSTAPD_CLI_PIDFILE) ) {
        start_stop_daemon(HOSTAPD_CLI_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, HOSTAPD_CLI_PIDFILE, NULL);
        unlink(HOSTAPD_CLI_PIDFILE);
    }

    return 0;
}

static int stop(void)
{
	cdb_set_int("$omi_result", 0);
//	exec_cmd2("cdb commit && /usr/bin/uartdfifo.sh failed");
//	exec_cmd2("uartdfifo.sh AlexaAlerEnd ; killall -9 aplay ; killall -9 AlexaRequest");
    hostapd_cli_stop();

#ifdef HAVE_MULTIPLE_BSSID
    /* Multiple BSSID */
    if (0) {
        int wl_bss_enable3 = cdb_get_int("$wl_bss_enable3", 0);
        int wl_bss_role3 = cdb_get_int("$wl_bss_role3", wlRole_NONE);

        if ((wl_bss_enable3 == 1) && (wl_bss_role3 == wlRole_AP)) {
            ifconfig("wlan1", IFDOWN, "0.0.0.0", NULL);
            exec_cmd2("brctl delif %s wlan1", mtdm->rule.BRAP);
        }
    }
#endif

    if ((mtdm->rule.AP[0] != 0) && (mtdm->rule.BRAP[0] != 0) && f_exists(HOSTAPD_PIDFILE)) {
#ifdef CONFIG_PACKAGE_omnicfg
        if (!is_directmode()) {

        exec_cmd2("brctl delif %s %s", mtdm->rule.BRAP, mtdm->rule.AP);

        }
#else
        exec_cmd2("brctl delif %s %s", mtdm->rule.BRAP, mtdm->rule.AP);
#endif
    }
    if ((mtdm->rule.STA[0] != 0) && (mtdm->rule.BRSTA[0] != 0)) {
        exec_cmd2("brctl delif %s %s", mtdm->rule.BRSTA, mtdm->rule.STA);
    }
    if (mtdm->rule.HOSTAPD == 1) {
#ifdef CONFIG_PACKAGE_omnicfg
        if (!is_directmode()) {

        start_stop_daemon(HOSTAPD_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, HOSTAPD_PIDFILE, NULL);

        }
#else
        start_stop_daemon(HOSTAPD_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, HOSTAPD_PIDFILE, NULL);
#endif
    }
    if (mtdm->rule.WPASUP == 1) {
        start_stop_daemon(WPASUP_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, WPASUP_PIDFILE, NULL);
#ifdef CONFIG_MONTAGE_OIPOD_BOARD
        ifconfig(mtdm->rule.BRSTA, IFDOWN, "0.0.0.0", NULL);
#endif
    }
    if (mtdm->rule.OMODE == opMode_p2p) {
        // P2P mode stop the p2p_cli
        start_stop_daemon("/usr/sbin/p2p_cli", SSDF_STOP_DAEMON, 0, SIGTERM, P2P_PIDFILE, NULL);
    }

    sleep(1);
    return 0;
}

#ifdef SPEEDUP_WLAN_CONF
#if defined(__PANTHER__)
#if !defined(OMNICFG_NAME)
#define OMNICFG_NAME    "omnicfg"
#define OMNICFG_BIN     "/lib/wdk/"OMNICFG_NAME
#endif
static void sniffer_config_timeout_handler(int sData)
{
    int omicfg_sniffer_enable;

    omicfg_sniffer_enable = cdb_get_int("$omicfg_sniffer_enable", 0);
    if(1==omicfg_sniffer_enable)
    {
        // stop smrtcfg
        exec_cmd("wd smrtcfg stop");
        cdb_set_int("$omicfg_sniffer_enable", 0);
    }

    // store fail log when sniffer timeout
    exec_cmd(OMNICFG_BIN" sniffer_timeout");
    // restart omnicfg
    exec_cmd("mtc wl restart");
}

static void omnicfg_reset_timeout(void)
{
    int target_timeout, time_now;
    int omicfg_sniffer_enable;
    int omicfg_sniffer_timeout;

    TimerDel(timerMtdmSnifferTimeout);

    omicfg_sniffer_enable = cdb_get_int("$omicfg_sniffer_enable", 0);
    omicfg_sniffer_timeout = cdb_get_int("$omicfg_sniffer_timeout", 0);

    if (omicfg_sniffer_enable)
    {
        if (omicfg_sniffer_timeout)
        {
            if ((time_now = uptime())>=0)
            {
                target_timeout = time_now + omicfg_sniffer_timeout;
            }
            else
            {
                target_timeout = 0;
            }
            cdb_set_int("$omi_target_timeout", target_timeout);

            TimerAdd(timerMtdmSnifferTimeout, sniffer_config_timeout_handler, 0, omicfg_sniffer_timeout);
        }
    }
}
#endif
int mtdm_wlhup(void)
{
    int mode = cdb_get_int("$op_work_mode", opMode_ap);
    int amode = cdb_get_int("$airkiss_state", 0);
    int pid_wpa = PROCESS_NOT_EXISTED;
    int pid_hostapd = PROCESS_NOT_EXISTED;
    pid_wpa = check_process_running(WPASUP_PIDFILE);
    pid_hostapd = check_process_running(HOSTAPD_PIDFILE);

    cdb_service_clear_change("op");
    cdb_service_clear_change("airkiss");
    cdb_service_clear_change("wl");

    if (PROCESS_NOT_EXISTED != pid_wpa && PROCESS_NOT_EXISTED != pid_hostapd) {
        mtdm->rule.OMODE = mode;
        mac80211_hostapd_setup_base();
        kill(pid_hostapd, SIGHUP);
#if defined(__PANTHER__)
        omnicfg_reset_timeout();
#endif
        if (mode == opMode_ap) {
            mac80211_wpasup_setup_empty_file();
        } else {
            mac80211_wpasup_setup_base();
        }

        #ifdef CONFIG_PACKAGE_omnicfg
        if((amode & 3) > 0) {
            if(amode == 1)
                ;
            else if(amode == 2)
                aplist_monitor_timer_enable(0, 5, "(HUP)wifi connecting(AirKiss)");
        }
        else {
            if (cdb_get_int("$omi_result", 0) == OMNICFG_R_CONNECTING) {
                /* under omnicfg apply, monitor it util omnicfg is finished
                 */
                aplist_monitor_timer_enable(1, 1, "(HUP)wifi connecting(omnicfg)");
            }
            else {
                /* NOT under omnicfg apply, it is used in aplist
                 * start a timer of 20 seconds to determine rollback AP mode
                 */
                aplist_monitor_timer_enable(0, 20, "(HUP)wifi connecting");
            }
        }
        #endif

        kill(pid_wpa, SIGHUP);
    }

    //printf("##### %d, %d \n", mode, pid_wpa);
    return 0;
}
#endif

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int wlan_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

