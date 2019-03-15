/*
 * =====================================================================================
 *
 *       Filename:  netutils.c
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

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "netutils.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

/*
 * Convert Ethernet address string representation to binary data
 * @param       a       string in xx:xx:xx:xx:xx:xx notation
 * @param       e       binary data
 * @return      TRUE if conversion was successful and FALSE otherwise
 */
int ether_atoe(const char *a, unsigned char *e)
{
    char *c = (char *)a;
    int i = 0;

    memset(e, 0, ETHER_ADDR_LEN);
    for (;;) {
        e[i++] = (unsigned char)strtoul(c, &c, 16);
        if (!*c++ || i == ETHER_ADDR_LEN)
            break;
    }
    return (i == ETHER_ADDR_LEN);
}

/*
 * Convert Ethernet address binary data to string representation
 * @param       e       binary data
 * @param       a       string in xx:xx:xx:xx:xx:xx notation
 * @return      a
 */
char *ether_etoa(const unsigned char *e, char *a)
{
    char *c = a;
    int i;

    for (i = 0; i < ETHER_ADDR_LEN; i++) {
        if (i)
            *c++ = ':';
        c += sprintf(c, "%02X", e[i] & 0xff);
    }
    return a;
}

/*
 * Search a string backwards for a set of characters
 * This is the reverse version of strspn()
 *
 * @param       s       string to search backwards
 * @param       accept  set of chars for which to search
 * @return      number of characters in the trailing segment of s 
 *              which consist only of characters from accept.
 */
static size_t sh_strrspn(const char *s, const char *accept)
{
    const char *p;
    size_t accept_len = strlen(accept);
    int i;

    if (s[0] == '\0')
        return 0;

    p = s + (strlen(s) - 1);
    i = 0;

    do {
        if (memchr(accept, *p, accept_len) == NULL)
            break;
        p--;
        i++;
    }
    while (p != s);

    return i;
}

int get_ifname_unit(const char *ifname, int *unit, int *subunit)
{
    const char digits[] = "0123456789";
    char str[64];
    char *p;
    size_t ifname_len = strlen(ifname);
    size_t len;
    long val;

    if (unit)
        *unit = -1;
    if (subunit)
        *subunit = -1;

    if (ifname_len + 1 > sizeof(str))
        return -1;

    strcpy(str, ifname);

    /*
     * find the trailing digit chars 
     */
    len = sh_strrspn(str, digits);

    /*
     * fail if there were no trailing digits 
     */
    if (len == 0)
        return -1;

    /*
     * point to the beginning of the last integer and convert 
     */
    p = str + (ifname_len - len);
    val = strtol(p, NULL, 10);

    /*
     * if we are at the beginning of the string, or the previous character is 
     * not a '.', then we have the unit number and we are done parsing 
     */
    if (p == str || p[-1] != '.') {
        if (unit)
            *unit = val;
        return 0;
    } else {
        if (subunit)
            *subunit = val;
    }

    /*
     * chop off the '.NNN' and get the unit number 
     */
    p--;
    p[0] = '\0';

    /*
     * find the trailing digit chars 
     */
    len = sh_strrspn(str, digits);

    /*
     * fail if there were no trailing digits 
     */
    if (len == 0)
        return -1;

    /*
     * point to the beginning of the last integer and convert 
     */
    p = p - len;
    val = strtol(p, NULL, 10);

    /*
     * save the unit number 
     */
    if (unit)
        *unit = val;

    return 0;
}

int get_ifname_ether_addr(const char *ifname, unsigned char *mac, int maclen)
{
    struct ifreq ifr;
    int hwlen = 0;
    int s;

    memset (&ifr, 0, sizeof (struct ifreq));
    strlcpy (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        return EXIT_FAILURE;
    }
    if (ioctl (s, SIOCGIFHWADDR, &ifr) < 0) {
        close (s);
        return EXIT_FAILURE;
    }   
    close (s);
            
    switch (ifr.ifr_hwaddr.sa_family) {
        case ARPHRD_ETHER:
        case ARPHRD_IEEE802:
            hwlen = ETHER_ADDR_LEN;
            break;
        case ARPHRD_IEEE1394:
            hwlen = EUI64_ADDR_LEN;
            break;
        case ARPHRD_INFINIBAND:
            hwlen = INFINIBAND_ADDR_LEN;
            break;
        default:
            logger (LOG_ERR, "%s: interface is not Ethernet, FireWire, InfiniBand or Token Ring", __func__);
            return EXIT_FAILURE;
    }

    if (mac && maclen >= hwlen) {
        memcpy (mac, ifr.ifr_hwaddr.sa_data, hwlen);
        return EXIT_SUCCESS;
    }

    logger (LOG_ERR, "%s: buffer size [%d] is too small for [%d]", __func__, maclen, hwlen);
    return EXIT_FAILURE;
}

static int get_ifname_ether_info(const char *ifname, char *ip, int iplen, int sioc)
{
    char addrbuf[INET6_ADDRSTRLEN+1];
    struct ifreq ifr;
    int ifrlen;
    struct sockaddr_in  *sin;
    struct sockaddr_in6 *sin6;
    int s;

    memset (&ifr, 0, sizeof (struct ifreq));
    strlcpy (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        return EXIT_FAILURE;
    }
    if (ioctl (s, sioc, &ifr, &ifrlen) < 0) {
        close (s);
        return EXIT_FAILURE;
    }   
    close (s);
            
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin6 =(struct sockaddr_in6 *)sin;

    switch( sin->sin_family ) {
        case AF_INET6:
            if (!inet_ntop( AF_INET6, (void *)&(sin6->sin6_addr), addrbuf, sizeof(addrbuf) )) {
                return EXIT_FAILURE;
            }
            break;
        case AF_INET:
            if (!inet_ntop( AF_INET, (void *)&(sin->sin_addr), addrbuf, sizeof(addrbuf) )) {
                return EXIT_FAILURE;
            }
            break;
        default:
            return EXIT_FAILURE;
    }

    if (ip && iplen > strlen(addrbuf)) {
        sprintf(ip, "%s", addrbuf);
        return EXIT_SUCCESS;
    }

    logger (LOG_ERR, "%s: buffer size [%d] is too small for [%d]", __func__, iplen, strlen(addrbuf));
    return EXIT_FAILURE;
}

int get_ifname_ether_ip(const char *ifname, char *ip, int iplen)
{
    return get_ifname_ether_info(ifname, ip, iplen, SIOCGIFADDR);
}

int get_ifname_ether_mask(const char *ifname, char *mask, int masklen)
{
    return get_ifname_ether_info(ifname, mask, masklen, SIOCGIFNETMASK);
}

void set_mac(const char *ifname, const char *hwaddr)
{
    int sfd;
    struct ifreq ifr;
    int up;

    memset (&ifr, 0, sizeof (struct ifreq));
    if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        logger (LOG_ERR, "%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
        return;
    }

    strcpy(ifr.ifr_name, ifname);

    up = 0;
    if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
        if ((up = ifr.ifr_flags & IFF_UP) != 0) {
            ifr.ifr_flags &= ~IFF_UP;
            if (ioctl(sfd, SIOCSIFFLAGS, &ifr) != 0) {
                logger (LOG_ERR, "%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
            }
        }
    }
    else {
        logger (LOG_ERR, "%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
    }

    ether_atoe(hwaddr, (unsigned char *)&ifr.ifr_hwaddr.sa_data);

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    if (ioctl(sfd, SIOCSIFHWADDR, &ifr) == -1) {
        logger (LOG_ERR, "Error setting %s address\n", ifname);
    }

    if (up) {
        if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
            ifr.ifr_flags |= IFF_UP|IFF_RUNNING;
            if (ioctl(sfd, SIOCSIFFLAGS, &ifr) == -1) {
                logger (LOG_ERR, "%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
            }
        }
    }

    close(sfd);
}

static int __ifconfig(char *name, int flags, char *addr, char *netmask, int *mtu)
{
    int s;
    struct ifreq ifr;
    struct in_addr in_addr, in_netmask, in_broadaddr;

    memset (&ifr, 0, sizeof (struct ifreq));
    /* Open a raw socket to the kernel */
    if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        goto err;

    /* Set interface name */
    strncpy(ifr.ifr_name, name, 16);

    /* Set interface mtu */
    if (mtu) {
        ifr.ifr_mtu = *mtu;
        if (ioctl(s, SIOCSIFMTU, &ifr) < 0)
            goto err;
    }

    /* Set interface flags */
    ifr.ifr_flags = flags;
    if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
        goto err;

    /* Set IP address */
    if (addr) {
        inet_aton(addr, &in_addr);
        sin_addr(&ifr.ifr_addr).s_addr = in_addr.s_addr;
        ifr.ifr_addr.sa_family = AF_INET;
        if (ioctl(s, SIOCSIFADDR, &ifr) < 0)
            goto err;
    }

    /* Set IP netmask and broadcast */
    if (addr && netmask) {
        inet_aton(netmask, &in_netmask);
        sin_addr(&ifr.ifr_netmask).s_addr = in_netmask.s_addr;
        ifr.ifr_netmask.sa_family = AF_INET;
        if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0)
            goto err;

        in_broadaddr.s_addr = (in_addr.s_addr & in_netmask.s_addr) | ~in_netmask.s_addr;
        sin_addr(&ifr.ifr_broadaddr).s_addr = in_broadaddr.s_addr;
        ifr.ifr_broadaddr.sa_family = AF_INET;
        if (ioctl(s, SIOCSIFBRDADDR, &ifr) < 0)
            goto err;
    }

    close(s);

    return 0;

    err:
    close(s);
    perror(name);
    return errno;
}

int ifconfig(char *name, int flags, char *addr, char *netmask)
{
    return __ifconfig(name, flags, addr, netmask, NULL);
}

int ifconfig2(char *name, int flags, char *addr, char *netmask, int mtu)
{
    return __ifconfig(name, flags, addr, netmask, &mtu);
}

/******** vconfig ******************************************************/
static int __vconfig(struct vlan_ioctl_args *ifr)
{
    int s;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return EXIT_FAILURE;
    }
    ioctl(s, SIOCSIFVLAN, ifr);
    close(s);

    return EXIT_SUCCESS;
}

int vconfig_add(char *ifname, int vid)
{
    struct vlan_ioctl_args ifr;

    if ((!ifname) || (vid < 0) || (vid > (VLAN_GROUP_ARRAY_LEN - 1))) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = ADD_VLAN_CMD;
    strncpy(ifr.device1, ifname, IFNAMSIZ);
    ifr.u.VID = vid;

    return __vconfig(&ifr);
}

int vconfig_rem(char *vname)
{
    struct vlan_ioctl_args ifr;

    if (!vname) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = DEL_VLAN_CMD;
    strncpy(ifr.device1, vname, IFNAMSIZ);

    return __vconfig(&ifr);
}

int vconfig_set_flag(char *ifname, unsigned int flag, short qos)
{
    struct vlan_ioctl_args ifr;

    if ((!ifname) || (flag < 0) || (flag > 1) || (qos < 0) || (qos > 7)) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = SET_VLAN_FLAG_CMD;
    strncpy(ifr.device1, ifname, IFNAMSIZ);
    ifr.u.flag = flag;
    ifr.vlan_qos = qos;

    return __vconfig(&ifr);
}

int vconfig_set_egress_map(char *vname, unsigned int skb_priority, short qos)
{
    struct vlan_ioctl_args ifr;

    if ((!vname) || (qos < 0) || (qos > 7)) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
    strncpy(ifr.device1, vname, IFNAMSIZ);
    ifr.u.skb_priority = skb_priority;
    ifr.vlan_qos = qos;

    return __vconfig(&ifr);
}

int vconfig_set_ingress_map(char *vname, unsigned int skb_priority, short qos)
{
    struct vlan_ioctl_args ifr;

    if ((!vname) || (qos < 0) || (qos > 7)) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = SET_VLAN_INGRESS_PRIORITY_CMD;
    strncpy(ifr.device1, vname, IFNAMSIZ);
    ifr.u.skb_priority = skb_priority;
    ifr.vlan_qos = qos;

    return __vconfig(&ifr);
}

int vconfig_set_name_type(unsigned int name_type)
{
    struct vlan_ioctl_args ifr;

    if ((name_type != VLAN_NAME_TYPE_PLUS_VID) && 
        (name_type != VLAN_NAME_TYPE_RAW_PLUS_VID) &&
        (name_type != VLAN_NAME_TYPE_PLUS_VID_NO_PAD) &&
        (name_type != VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD)) {
        return EXIT_FAILURE;
    } 

    memset(&ifr, 0, sizeof(ifr));
    ifr.cmd = SET_VLAN_NAME_TYPE_CMD;
    ifr.u.name_type = name_type;

    return __vconfig(&ifr);
}
/******** vconfig end **************************************************/

/******** brctl ********************************************************/
static int __brctl_br(char *br, unsigned long params, int addbr)
{
    int s;
    unsigned long arg[4];
    int ret = EXIT_SUCCESS;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return EXIT_FAILURE;
    }

    arg[0] = ((addbr) ? BRCTL_ADD_BRIDGE : BRCTL_DEL_BRIDGE);
    arg[1] = (unsigned long) br;
    arg[2] = params;
    arg[3] = 0;
    if (ioctl(s, SIOCSIFBR, arg) < 0) {
        ret = EXIT_FAILURE;
    }

    close(s);

    return ret;
}

static int __brctl_brif(char *br, char *brif, int addbrif)
{
    int s;
    unsigned long arg[4];
    struct ifreq ifr;
    int if_index;
    int ret = EXIT_SUCCESS;

    memset (&ifr, 0, sizeof (struct ifreq));
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return EXIT_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, br, IFNAMSIZ);
    if_index = if_nametoindex(brif);
    ifr.ifr_ifindex = if_index;

    if (!if_index) {
        ret = EXIT_FAILURE;
    }

    if ((if_index) && ioctl(s, ((addbrif) ? SIOCBRADDIF : SIOCBRDELIF), &ifr) < 0) {
        arg[0] = ((addbrif) ? BRCTL_ADD_IF : BRCTL_DEL_IF);
        arg[1] = if_index;
        arg[2] = 0;
        arg[3] = 0;
        ifr.ifr_data = (char *) &arg;
        if (ioctl(s, SIOCDEVPRIVATE, &ifr) < 0) {
            ret = EXIT_FAILURE;
        }
    }

    close(s);

    return ret;
}

int brctl_addbr(char *br, unsigned long params)
{
    if (!br) {
        return EXIT_FAILURE;
    }
    return __brctl_br(br, params, 1);
}

int brctl_delbr(char *br)
{
    if (!br) {
        return EXIT_FAILURE;
    }
    return __brctl_br(br, 0, 0);
}

int brctl_addif(char *br, char *brif)
{
    if (!br || !brif) {
        return EXIT_FAILURE;
    }
    return __brctl_brif(br, brif, 1);
}

int brctl_delif(char *br, char *brif)
{
    if (!br || !brif) {
        return EXIT_FAILURE;
    }
    return __brctl_brif(br, brif, 0);
}
/******** brctl end ****************************************************/

static int route_manip(int cmd, char *name, int metric, char *dst, char *gateway, char *genmask)
{
    int s;
    struct rtentry rt;

    /* Open a raw socket to the kernel */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        goto err;

    /* Fill in rtentry */
    memset(&rt, 0, sizeof(rt));
    if (dst)
        inet_aton(dst, &sin_addr(&rt.rt_dst));
    if (gateway)
        inet_aton(gateway, &sin_addr(&rt.rt_gateway));
    if (genmask)
        inet_aton(genmask, &sin_addr(&rt.rt_genmask));
    rt.rt_metric = metric;
    rt.rt_flags = RTF_UP;
    if (sin_addr(&rt.rt_gateway).s_addr)
        rt.rt_flags |= RTF_GATEWAY;
    if (sin_addr(&rt.rt_genmask).s_addr == INADDR_BROADCAST)
        rt.rt_flags |= RTF_HOST;
    rt.rt_dev = name;

    /* Force address family to AF_INET */
    rt.rt_dst.sa_family = AF_INET;
    rt.rt_gateway.sa_family = AF_INET;
    rt.rt_genmask.sa_family = AF_INET;

    if (ioctl(s, cmd, &rt) < 0)
        goto err;

    close(s);
    return 0;

    err:
    close(s);
    perror(name);
    return errno;
}

int route_add(char *name, int metric, char *dst, char *gateway, char *genmask)
{
    return route_manip(SIOCADDRT, name, metric, dst, gateway, genmask);
}

int route_del(char *name, int metric, char *dst, char *gateway, char *genmask)
{
    return route_manip(SIOCDELRT, name, metric, dst, gateway, genmask);
}

static void __attribute__ ((unused)) __route_printRoute(struct route_info *rtInfo)
{
    if (rtInfo->dstAddr.s_addr != 0) {
        logger (LOG_INFO, "Destination address: %s", (char *)inet_ntoa(rtInfo->dstAddr));
    }
    else {
        logger (LOG_INFO, "Destination address: *.*.*.*");
    }
    if (rtInfo->gateWay.s_addr != 0) {
        logger (LOG_INFO, "Gateway address: %s", (char *)inet_ntoa(rtInfo->gateWay));
    }
    else {
        logger (LOG_INFO, "Gateway address: *.*.*.*");
    }
    logger (LOG_INFO, "Interface name: %s", rtInfo->ifName);
    if (rtInfo->srcAddr.s_addr != 0) {
        logger (LOG_INFO, "Source address: %s", (char *)inet_ntoa(rtInfo->srcAddr));
    }
    else {
        logger (LOG_INFO, "Source address: *.*.*.*");
    }
}

static int __route_findRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *dst)
{
    struct in_addr dst_addr;
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

    if (inet_aton(dst, &dst_addr) == 0) {
        return EXIT_FAILURE;
    }

    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
        return EXIT_FAILURE;
    }
   
    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen)) {
        switch (rtAttr->rta_type) {
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
                break;
            case RTA_GATEWAY:
                rtInfo->gateWay = *(struct in_addr *)RTA_DATA(rtAttr);
                break;
            case RTA_PREFSRC:
                rtInfo->srcAddr = *(struct in_addr *)RTA_DATA(rtAttr);
                break;
            case RTA_DST:
                rtInfo->dstAddr = *(struct in_addr *)RTA_DATA(rtAttr);
                break;
        }
    }

#if 0
    __route_printRoute(rtInfo);
#endif

    if (rtInfo->dstAddr.s_addr == dst_addr.s_addr) {
        return EXIT_SUCCESS;
    }
    else {
        return EXIT_FAILURE;
    }
}

static int __route_readNlSock(int sockFd, char *bufPtr, int bufLen, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do {
        if ((readLen = recv(sockFd, bufPtr, bufLen - msgLen, 0)) < 0) {
            break;
        }
        nlHdr = (struct nlmsghdr *)bufPtr;
        if ((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
            break;
        }
        if (nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        }
        else {
            bufPtr += readLen;
            msgLen += readLen;
        }
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;
}

static int __route_get_route_info(char *dst, struct route_info *rtInfo)
{
    static int msgSeq = 0;
    struct nlmsghdr *nlMsg;
    char *msgBuf = NULL;
    int msgLen = 1024;
    int len, sock = 0;
    int ret = EXIT_FAILURE;

    if (!rtInfo || !dst || (*dst == 0)) {
        goto err;
    }
    
    msgBuf = (char *)malloc(msgLen);
    if (!msgBuf) {
        perror("route malloc: ");
        goto err;
    }

    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) <0) {
        perror("route Socket Creation: ");
        goto err;
    }

    memset(msgBuf, 0, msgLen);
    nlMsg = (struct nlmsghdr *)msgBuf;
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlMsg->nlmsg_type = RTM_GETROUTE;
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    nlMsg->nlmsg_seq = msgSeq++;
    nlMsg->nlmsg_pid = getpid();

    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        perror("route Write To Socket Failed: ");
        goto err;
    }
    if ((len = __route_readNlSock(sock, msgBuf, msgLen, msgSeq, getpid())) < 0) {
        perror("route Read From Socket Failed: ");
        goto err;
    }

    for (; NLMSG_OK(nlMsg,len); nlMsg = NLMSG_NEXT(nlMsg,len)) {
        memset(rtInfo, 0, sizeof(struct route_info));
        if ((ret = __route_findRoutes(nlMsg, rtInfo, dst)) == EXIT_SUCCESS) {
            break;
        }
    }

err:
    if (sock) {
        close(sock);
    }
    if (msgBuf) {
        free(msgBuf);
    }

    return ret;
}

int route_info(char *dst, struct route_info *rtInfo)
{
    return __route_get_route_info(dst, rtInfo);
}

int route_gw_info(struct route_info *rtInfo)
{
    return route_info("0.0.0.0", rtInfo);
}

/* configure loopback interface */
void config_loopback(void)
{
    /* Bring up loopback interface */
    ifconfig("lo", IFUP, "127.0.0.1", "255.0.0.0");

    /* Add to routing table */
    route_add("lo", 0, "127.0.0.0", "0.0.0.0", "255.0.0.0");
}

int iface_exist(char *ifname)
{
    struct if_nameindex *ifp, *ifpsave;
    int found = 0;

    ifpsave = ifp = if_nameindex();

    if (!ifp) {
        return 0;
    }

    while (ifp->if_index) {
        if (strcmp(ifp->if_name, ifname) == 0) {
            found = 1;
            break;
        }
        ifp++;
    }

    if_freenameindex(ifpsave);

    return found;
}
