/*
 * =====================================================================================
 *
 *       Filename:  wlutils.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/16/2016 10:38:05 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __WLUTILS_H__
#define __WLUTILS_H__

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#define SCAN_DEVICE "sta0"

#define WLAN_CAPABILITY_PRIVACY	(1<<4)
#define SELECTOR_LEN    4

#define SCAN_AP_NOT_FOUND	0
#define SCAN_AP_FOUND		1
#define SCAN_AP_PASS_ERROR	2
#define SCAN_AP_SEC_ERROR	3

#define SCAN_RETRY_CNT	5
#define SCANRESULT "/tmp/scanres"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

struct handler_args {
	const char *group;
	int id;
};

enum {
	AUTH_CIPHER_NONE = 0,
	AUTH_CIPHER_WEP40,
	AUTH_CIPHER_WEP104,
	AUTH_CIPHER_TKIP,
	AUTH_CIPHER_CCMP,
	AUTH_CIPHER_SMS4,
	AUTH_CIPHER_AES_128_CMAC,
};

#define RSN_CIPHER_SUITE_TKIP   OUI_SUITETYPE(0x00, 0x0f, 0xac, 2)
#define RSN_CIPHER_SUITE_CCMP   OUI_SUITETYPE(0x00, 0x0f, 0xac, 4)
#define WPA_CIPHER_SUITE_TKIP   OUI_SUITETYPE(0x00, 0x50, 0xf2, 2)
#define WPA_CIPHER_SUITE_CCMP   OUI_SUITETYPE(0x00, 0x50, 0xf2, 4)

#define OUI_SUITETYPE(a, b, c, d) \
        ((((__u32) (a)) << 24) | (((__u32) (b)) << 16) | (((__u32) (c)) << 8) | \
        (__u32) (d))

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

struct bss_info {
	char mac_addr[20];
	int signal;
};

struct scan_params {
	FILE *fd;
	char *ssid;
	int done;
	int aborted;
};

struct wifi_scan_result {
	int sec;
	int gcipher;
	int pcipher;
};

unsigned int wifi_scan(void);
unsigned int wifi_set_ssid_pwd(char *ssid, char *pass);

#endif /* __WLUTILS_H__ */
