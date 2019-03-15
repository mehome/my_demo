#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#define MY_DEST_MAC0	0xff
#define MY_DEST_MAC1	0xff
#define MY_DEST_MAC2	0xff
#define MY_DEST_MAC3	0xff
#define MY_DEST_MAC4	0xff
#define MY_DEST_MAC5	0xff

#define DEFAULT_IF	"mon0"
#define BUF_SIZ		4096

#define ETH_ALEN 6
#define HEADROOM  256

#define RADIOTAP_TSFT  0
#define RADIOTAP_FLAGS 1

#define RADIOTAP_F_WEP        0x04
#define RADIOTAP_F_FCS_FAILED 0x40

#define TX_REPEAT_MAGIC 88888888

struct radiotap_hdr {
    uint8_t  version;
    uint8_t  pad;
    uint16_t length;
    uint32_t present;
} __attribute__((packed));

static int encap_action_frame(uint8_t *buf, size_t pos)
{
    buf[0] = 0x0; /* type and subtype */
    buf[1] = 0x0;
    buf[2] = 0x0; /* duration */
    buf[3] = 0x0;

    return pos + 4;
}

static int encap_radiotap(uint8_t *buf, size_t pos)
{
    struct radiotap_hdr *rthdr = (void*)buf;

    rthdr->version = 0;
    rthdr->pad = 0;
    rthdr->length = htole16(sizeof *rthdr + 1);
    rthdr->present = htole32(1 << RADIOTAP_FLAGS);
    buf[sizeof *rthdr] = RADIOTAP_F_WEP;
    return (pos + (sizeof *rthdr + 1));
}

void send_pkt(int cnt, int length, int burst)
{
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
	int tx_len, send_count, burst_time;
	unsigned int iteration = 0;

	/* Initialize send count, length, burst */
	if(cnt == TX_REPEAT_MAGIC)
		send_count = -1;
	else
		send_count = cnt;

	if(length < 50)
		tx_len = 50;
	else
		tx_len = length;

	if(burst < 1)
		burst_time = 1;
	else
		burst_time = burst;

	strcpy(ifName, DEFAULT_IF);
again:
	if ((sockfd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL))) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);

	int pos = 0;
	uint8_t *buf = sendbuf;

	pos = encap_radiotap(buf, pos);

	pos = encap_action_frame(buf+pos, pos);

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

	while((send_count > 0) || (send_count == -1))
	{
		if(iteration % burst_time == 0)
			usleep(10 * 1000);

		if (sendto(sockfd, sendbuf, tx_len, MSG_DONTWAIT, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
			goto error;

		if(send_count != -1)
			send_count--;

		iteration++;
	}
	close(sockfd);

	return 0;

error:
	if((send_count > 0) || (send_count == -1))
	{
		close(sockfd);
		goto again;
	}
}
