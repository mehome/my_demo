/*
 * =====================================================================================
 *
 *       Filename:  netutils.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2016 04:34:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __NETUTILS_H__
#define __NETUTILS_H__

#include "logger.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <linux/if_bridge.h>

#define EUI64_ADDR_LEN          8
#define INFINIBAND_ADDR_LEN     20

/* Linux 2.4 doesn't define this */
#ifndef ARPHRD_IEEE1394
#  define ARPHRD_IEEE1394       24
#endif  

/* The BSD's don't define this yet */
#ifndef ARPHRD_INFINIBAND
#  define ARPHRD_INFINIBAND     27
#endif

/******** vconfig ******************************************************/
enum vlan_ioctl_cmds {
    ADD_VLAN_CMD,
    DEL_VLAN_CMD,
    SET_VLAN_INGRESS_PRIORITY_CMD,
    SET_VLAN_EGRESS_PRIORITY_CMD,
    GET_VLAN_INGRESS_PRIORITY_CMD,
    GET_VLAN_EGRESS_PRIORITY_CMD,
    SET_VLAN_NAME_TYPE_CMD,
    SET_VLAN_FLAG_CMD
};
enum vlan_name_types {
    VLAN_NAME_TYPE_PLUS_VID,
    VLAN_NAME_TYPE_RAW_PLUS_VID,
    VLAN_NAME_TYPE_PLUS_VID_NO_PAD,
    VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD,
    VLAN_NAME_TYPE_HIGHEST
};

struct vlan_ioctl_args {
    int cmd;
    char device1[24];

    union {
        char device2[24];
        int VID;
        unsigned int skb_priority;
        unsigned int name_type;
        unsigned int bind_type;
        unsigned int flag;
    } u;

    short vlan_qos;
};

#define VLAN_GROUP_ARRAY_LEN 4096
#define SIOCSIFVLAN 0x8983

#ifndef IFNAMSIZ
enum { IFNAMSIZ = 16 };
#endif
/******** vconfig end **************************************************/

/******** brctl ********************************************************/
/* Private Argv for brctl in the driver */
#define ARGV_NO_FLOOD_MC        0x0001
#define ARGV_FLOOD_UNKNOW_UC    0x0002
/******** brctl end ****************************************************/

struct route_info{
    struct in_addr dstAddr;
    struct in_addr srcAddr;
    struct in_addr gateWay;
    char ifName[IF_NAMESIZE];
};

/*
 * Convert Ethernet address string representation to binary data
 * @param       a       string in xx:xx:xx:xx:xx:xx notation
 * @param       e       binary data
 * @return      TRUE if conversion was successful and FALSE otherwise
 */
extern int ether_atoe(const char *a, unsigned char *e);
extern char *ether_etoa(const unsigned char *e, char *a);
extern int get_ifname_unit(const char *ifname, int *unit, int *subunit);
extern int get_ifname_ether_addr(const char *ifname, unsigned char *mac, int maclen);
extern int get_ifname_ether_ip(const char *ifname, char *ip, int iplen);
extern int get_ifname_ether_mask(const char *ifname, char *mask, int masklen);

extern void set_mac(const char *ifname, const char *hwaddr);
extern int ifconfig(char *name, int flags, char *addr, char *netmask);
extern int ifconfig2(char *name, int flags, char *addr, char *netmask, int mtu);

extern int vconfig_add(char *ifname, int vid);
extern int vconfig_rem(char *vname);
extern int vconfig_set_flag(char *ifname, unsigned int flag, short qos);
extern int vconfig_set_egress_map(char *vname, unsigned int skb_priority, short qos);
extern int vconfig_set_ingress_map(char *vname, unsigned int skb_priority, short qos);
extern int vconfig_set_name_type(unsigned int name_type);

extern int brctl_addbr(char *br, unsigned long params);
extern int brctl_delbr(char *br);
extern int brctl_addif(char *br, char *brif);
extern int brctl_delif(char *br, char *brif);

extern int route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
extern int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
extern int route_info(char *dst, struct route_info *rtInfo);
extern int route_gw_info(struct route_info *rtInfo);

extern void config_loopback(void);

extern int iface_exist(char *ifname);
#define IFUP   (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define IFDOWN (IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

#endif /* __NETUTILS_H__ */
