/*=============================================================================+
|                                                                              |
| Copyright 2012, 2017                                                         |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*
*   \file cta_eth.c
*   \brief  Cheetah ethernet driver.
*   \author Montage
*/
/*=============================================================================+
| Included Files                                                               |
+=============================================================================*/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <net/ip.h>

#include <linux/irqreturn.h>
#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/mach-panther/cta_eth.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/common.h>
#include <asm/mach-panther/cta_hnat.h>
#include <asm/mach-panther/cta_switch.h>
#include <linux/if_vlan.h>
#include <../net/8021q/vlan.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>

static struct dentry *eth_debug_dir = NULL;

static DEFINE_SPINLOCK(panther_eth_lock);

unsigned short ip_cksum(unsigned short *p, unsigned int len);
/*=============================================================================+
| Define                                                                       |
+=============================================================================*/
#define hal_delay_us(n)				udelay(n)
#define diag_printf					printk
#define cyg_drv_interrupt_unmask(n)	enable_irq(n)
#define cyg_drv_interrupt_mask(n)	disable_irq(n)

#define CTA_RX_CKSUM_OFFLOAD
//#define ETH_DBG

#if defined(CONFIG_PANTHER_FPGA)
#define MCR_SMI_DIV				0x0d
#else
#define MCR_SMI_DIV				0x50
#endif
#define MCR_INIT	((MCR_SMI_DIV<<MCR_SMI_S) | MCR_BWCTRL_TRIGGER|MCR_BWCTRL_DROP| \
					MCR_BYBASS_LOOKUP|	/* bypass lookup */ \
					MCR_TXDMA_RETRY_EN|MCR_TX_FLOWCTRL|MCR_LINKUP|MCR_FLOWCTRL)
					//MCR_TXDMA_RETRY_EN|MCR_TX_FLOWCTRL|MCR_LINKUP)

#define MDROPS_INIT				0x1e01007b
#define MIM_INIT				0x33
#define MRXCR_INIT				0x8201004 //defulat=0x8321004
#define ML3_INIT				0x071505dc

#define DC_FLUSH_TX(a,l)		dma_cache_wback_inv((unsigned long) a, (unsigned long) l)
#define DC_FLUSH_RX(a,l)		dma_cache_inv((unsigned long) a,(unsigned long) l)
#define PHYTOVA(a)				ca_addr(a)
#define DESC_W1_TO_VBUF(w1) 	ca_addr((w1)&0x1fffffc0)
#define DESC_W1_TO_VPKT(w1) 	ca_addr((w1)&0x1fffffff)

#define NUM_MBUF	 			(NUM_RX_DESC+24+1+NUM_TX_DESC)

#define DESC_SZ					(NUM_RX_DESC*sizeof(rxdesc_t) \
								+ NUM_TX_DESC*sizeof(txdesc_t) + NUM_HW_DESC*sizeof(rxdesc_t))

#define CTA_ETH_IRQ		IRQ_HWNAT

#ifdef ETH_DBG
#define DBG_ERR					__BIT(0)
#define DBG_INFO				__BIT(1)
#define DBG_INIT				__BIT(2)
#define DBG_INITMORE			__BIT(3)
#define DBG_TX					__BIT(4)
#define DBG_TXMORE				__BIT(5)
#define DBG_RX					__BIT(6)
#define DBG_RXMORE				__BIT(7)
#define DBG_IOCTL				__BIT(8)
#define DBG_ISR					__BIT(9)
#define DBG_DSR					__BIT(10)
#define DBG_DELIVER				__BIT(11)
#define DBG_PHY					__BIT(12)
#define DBG_TXMORE2				__BIT(13)
#define DBG_RXMORE2				__BIT(14)
#define DBG_HNAT				__BIT(15)

#define DBG_MASK_INIT			(DBG_ERR)
#define DBG_MASK 				cta_eth_debug_lvl
#define DBG_INFO_MASK			(DBG_ERR)

#define DBGPRINTF(_type, fmt, ...) \
	do { \
		if ((_type) & DBG_INFO_MASK) \
			diag_printf(fmt, ## __VA_ARGS__); \
	} while(0)

#else

#define DBGPRINTF(_type, fmt, ...) \
	do { } while(0)
#endif

#ifndef CYG_ASSERT
#define CYG_ASSERTC(test)		do {} while(0)
#define CYG_ASSERT(test,msg)	do {} while(0)
#endif

/* the SMI control is seperated from HWNAT in IP3280 */
#define GPIO_SMI_DIV			0x20b8
#define GPIO_SMI_CR				0x20b4 // offset from MAC's base
/* SMI bank register */
#define SMI_DATA          0x00
#define SMI_CLK_RST       0x04 /* [15:0]:read_data */
#define EPHY_ADDR         0x08

#define RX_CACHE_FLUSH_SIZE		0x620 //1568
#define MAX_PKT_DUMP			64

#define HWD_RDY					(1<<31)
#define HWD_WR					(1<<28)
#define HWD_IDX_S				18		// which entry (0~23)
#define HWD_HWI_S				16		//which half-word (0~3)

/*=============================================================================+
| Variables                                                                    |
+=============================================================================*/
/* Global variables */
const struct ethtool_ops cta_ethtool_ops;
const static char *eth_ethdrv_version = "1.0";

cta_eth_t cta_eth;
cta_eth_t *pcta_eth = 0;
#if defined(ETH_DBG)
int cta_eth_debug_lvl = DBG_MASK_INIT;
#endif

static bufdesc_t g_txbd[NUM_TX_DESC] __attribute__ ((aligned (0x20)));
// descriptor must be aligned to 0x20 to optimize performance
static unsigned char g_desc[DESC_SZ] __attribute__ ((aligned (0x20)));

static struct net_device *g_eth_dev = NULL;

int port_link_status[2] = { 0, 0 };
static const u8 broadcast_addr[ETH_ALEN] =  { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

extern int get_args(const char *string, char *argvs[]);

extern int force_vo;
/*=============================================================================+
| Function Prototypes                                                          |
+=============================================================================*/

//#define USE_NAPI
#define USE_TASKLET
//#define USE_WORKQUEUE

#if defined(USE_TASKLET)
//#define INTERRUPT_MITIGATION
#define DETECT_A1_AND_APPLY_TX_RATE_LIMIT
#endif

#if defined(INTERRUPT_MITIGATION)
static int rx_count;
static int tx_done_count;
static int intr_count;
static unsigned long last_jiffies;
static struct hrtimer intr_timer;
static int intr_tolerant_count = 2000;
static int intr_minigation_ticks = 2000;
#endif

//#define USED_VLAN
//#define BRIDGE_FILTER
#define ETH_DEV_INIT			1

#ifdef BRIDGE_FILTER
static char mymac[6];
int cta_eth_filter_mode;
#endif

struct tasklet_struct   tasklet;    /* tasklet for dsr */
if_cta_eth_t * if_cta_eth_g[MAX_IF_NUM];

static int cta_eth_rxint(rxctrl_t *prxc, int budget);
#ifndef USE_NAPI
static void cta_eth_deliver(struct net_device *sc);
#endif
#ifdef USE_NAPI
#define CTA_ETH_NAPI_WEIGHT 8
static int cta_eth_poll(struct napi_struct *napi, int budget);
#endif

static irqreturn_t cta_eth_isr(int irq, void *data);
void cta_eth_txint(txctrl_t *ptxc, int wait_free);
int cta_set_mac(struct net_device *dev, void *addr);
int eth_drv_recv1(struct net_device *dev, unsigned char * data,  int total_len, char **newbuf, void *hw_nat, unsigned int w0, unsigned int w1);
char * eth_drv_getbuf(void);
void eth_drv_freebuf(char *buf);

static int eth_start(struct net_device *dev);
static int eth_stop( struct net_device *dev );
int eth_send(struct sk_buff *skb, struct net_device *dev );
static int eth_ioctl( struct net_device *dev, struct ifreq *ifr, int cmd );

int cta_hnat_sdmz_rx_check(if_cta_eth_t *pifcta_eth, char *buf, char *pkt, unsigned int w0, unsigned int w1, int len);
void cta_hnat_sdmz_tx_check(char *hwadrp);

void cta_eth_flowctrl(int onoff);

static int cta_eth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd);
static int cta_eth_set_settings(struct net_device *dev, struct ethtool_cmd *cmd);
/*=============================================================================+
| Extern Function/Variables                                                    |
+=============================================================================*/

/*=============================================================================+
| Functions                                                                    |
+=============================================================================*/

int wan_interface_port(void)
{
	if(cta_switch_cfg[1] & CSW_CTRL_IF_TYPE_WAN)
		return 1;

	if(cta_switch_cfg[0] & CSW_CTRL_IF_TYPE_WAN)
		return 0;

	return -1;
}

int lan_interface_port(void)
{
	if(cta_switch_cfg[1] & CSW_CTRL_IF_TYPE_LAN)
		return 1;

	if(cta_switch_cfg[0] & CSW_CTRL_IF_TYPE_LAN)
		return 0;

	return -1;
}

int wan_interface_vlan_id(void)
{
	if(cta_switch_cfg[1] & CSW_CTRL_IF_TYPE_WAN)
		return swcfg_p1_vlan_id();

	if(cta_switch_cfg[0] & CSW_CTRL_IF_TYPE_WAN)
		return swcfg_p0_vlan_id();

	return 4095;
}

int lan_interface_vlan_id(void)
{
	if(cta_switch_cfg[1] & CSW_CTRL_IF_TYPE_LAN)
		return swcfg_p1_vlan_id();

	if(cta_switch_cfg[0] & CSW_CTRL_IF_TYPE_LAN)
		return swcfg_p0_vlan_id();

	return 4095;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_mdio:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_mdio(short phy,short reg,unsigned short *pval,int wr)
{
	unsigned long v;
	int internal = 0;

	if(phy & CSW_CTRL_PHY_INTERNAL)
		internal = 1;

	phy &= CSW_CTRL_PHY_ADDR;

	if(internal)
		phy ^= MPHY_EXT;

	v = MPHY_TR | (phy << MPHY_PA_S) | (reg << MPHY_RA_S);
	if(wr)
		v |= MPHY_WR | *pval;
	SMIREG_WRITE32(SMI_DATA, v);
	/* delay for h/w, DON'T remove */
	hal_delay_us(1);
	for(;;)
	{
		if(MPHY_TR & (v=SMIREG_READ32(SMI_DATA)))
		{
			break;
		}
	}
	v = SMIREG_READ32(SMI_CLK_RST);
	if (!wr)
		*pval=(v&0xffff);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_mdio_rd:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
unsigned short cta_eth_mdio_rd(short phy,short reg)
{
	unsigned short rd;
	cta_eth_mdio(phy,reg,&rd,0);
	return rd;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_mdio_wr:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_mdio_wr(short phy,short reg,unsigned short val)
{
	cta_eth_mdio(phy,reg,&val,1);
#if defined(ETH_DBG)
	if (DBG_INFO_MASK & DBG_PHY)
	{
		unsigned short rd;

		rd=cta_eth_mdio_rd(phy,reg);
		DBGPRINTF(DBG_PHY, "phy%02x,reg%02x,wr:%04x\n", phy,(int)reg,(unsigned int)val);
		if (rd!=val)
			diag_printf("cta_eth_mdio_wr(%02x,%02x,%04x)==> not match, rd:%x\n\n",
						phy, (int)reg, (unsigned int) val, (unsigned int)rd);
	}
#endif
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_reset:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_phy_reset(unsigned short phy, unsigned int id)
{
	unsigned short val;

	DBGPRINTF(DBG_PHY, "%s: \n", __func__);
	val = cta_eth_mdio_rd(phy, 0);
	cta_eth_mdio_wr(phy, 0, val | 0x8000);

	if(id==PHYID_MONT_EPHY)
	{
		pcta_eth->ephy.anc_fail=0;
#if defined(CONFIG_EPHY_FPGA)
		val = phy_status(phy);
		if(val & PHY_ANC)
		{
			DBGPRINTF(DBG_PHY, "%s: EPHY error: phy_status(%04x)\n", __func__, val);
		}
#endif
	}
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_isolate:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_phy_isolate(unsigned short phy, unsigned int isolate)
{
	/* Electrically isolate PHY from RMII. */
	unsigned short val;

	val = cta_eth_mdio_rd(phy, 0);
	if(isolate)
		cta_eth_mdio_wr(phy, 0, val | 0x0400);
	else
		cta_eth_mdio_wr(phy, 0, val & ~0x0400);
}

#if 0
/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_synchronization:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_phy_synchronization(unsigned short phy)
{
	unsigned short tmp;
	/*
	^...Avoid rx crc error once link partner is RTL8168B/8111B family
	^...PCI-E Gigabit Ethernet NIC
	 */
	tmp = cta_eth_mdio_rd(phy, 31);
	tmp &= 0x7;
	/* Swap to page 2 */
	cta_eth_mdio_wr(phy, 31, 2);
	/* Restart timing synchronize and equalizer for 100BASE-TX */
	cta_eth_mdio_wr(phy, 16, (cta_eth_mdio_rd(phy, 16)|0x8000));
	hal_delay_us(3000);			/* delay for h/w, DON'T remove */
	cta_eth_mdio_wr(phy, 16, (cta_eth_mdio_rd(phy, 16)&0x7fff));
	/* Swap to page 0 */
	cta_eth_mdio_wr(phy, 31, tmp);
}
#endif

/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_monitor:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
#if 0
void cta_eth_phy_monitor(unsigned short phy, unsigned int id, unsigned short cap)
{
	if(id!=PHYID_MONT_EPHY)
		return;

	if(cap & PHY_LINK)
	{
		unsigned int j;
		/* 20131211 */
		if(cap & PHY_100M)
		{
			volatile unsigned short p20x17, p20x15, p40x14;
			int p20x17_ok=0, p20x15_ok=0, p40x14_ok=0;
			/* Swap to page 2 */
			cta_eth_mdio_wr(phy, 31, 2);
			/* Read 0x17 reg */
			p20x17=cta_eth_mdio_rd(phy, 23);
			if(((p20x17 > 0x0000) && (p20x17 < 0x00ff)) || ((p20x17 > 0xff00) && (p20x17 < 0xffff)))
			{
				p20x17_ok=1;
				DBGPRINTF(DBG_PHY, "%s: EPHY check: page(2) reg(0x17) ok, val(%08x)\n",
							__func__, p20x17);
			}
			else
			{
				DBGPRINTF(DBG_PHY, "%s: EPHY error: page(2) reg(0x17) fail!!, val(%08x) cnt(%d)\n",
							__func__, p20x17, ++pcta_eth->ephy.dsp_p20x17_fail);
			}

			p20x15=cta_eth_mdio_rd(phy, 21);
			p20x15 &= ~0x7e00;		/* bit[14:9]: clear index */
			p20x15 |= 0x0800;		/* bit[14:9]: index=4 */
			cta_eth_mdio_wr(phy, 21, p20x15);
			/* Read 0x15 reg */
			p20x15=cta_eth_mdio_rd(phy, 21);
			p20x15 &= 0x00ff;		/* bit[7:0]: snr value */
			if(((p20x15 > 0x0010) && (p20x15 < 0x007f)))
			{
				p20x15_ok=1;
				DBGPRINTF(DBG_PHY, "%s: EPHY check: page(2) reg(0x15) ok, val(%08x)\n",
							__func__, p20x15);
			}
			else
			{
				DBGPRINTF(DBG_PHY, "%s: EPHY error: page(2) reg(0x15) fail!!, val(%08x) cnt(%d)\n",
							__func__, p20x15, ++pcta_eth->ephy.dsp_p20x15_fail);
			}

			/* 20131213 */
			/* Wait 1ms */
			for(j=0; j<0x8000; j++);
			/* Swap to page 4 */
			cta_eth_mdio_wr(phy, 31, 4);
			/* Read 0x14 reg */
			p40x14=cta_eth_mdio_rd(phy, 20);
			/* Wait 800ms */
			for(j=0; j<0x17e0000; j++);
			/* Read 0x14 reg 2nd */
			p40x14=cta_eth_mdio_rd(phy, 20);
			/* Check bit[15:14] */
			if(0x8000 == (p40x14 & 0xc000))
			{
				p40x14_ok=1;
				DBGPRINTF(DBG_PHY, "%s: EPHY check: page(4) reg(0x14) ok, val(%08x)\n",
							__func__, p40x14);
			}
			else
			{
				DBGPRINTF(DBG_PHY, "%s: EPHY error: page(4) reg(0x14) fail!!, val(%08x) cnt(%d)\n",
							__func__, p40x14, ++pcta_eth->ephy.dsp_p40x14_fail);
			}

			/* Swap to page 0 */
			cta_eth_mdio_wr(phy, 31, 0);
			if((!p20x17_ok) || (!p20x15_ok) || (!p40x14_ok))
			{
				DBGPRINTF(DBG_PHY, "%s: EPHY error: DSP fail, reset EPHY(%d)\n",
							__func__, ++pcta_eth->ephy.dsp_fail);
				cta_eth_phy_reset(phy, id);
				return;
			}
		}
		pcta_eth->ephy.rst_unlock = 1;
	}
	else
	{
		if(pcta_eth->ephy.rst_unlock == 1)
			cta_eth_phy_reset(phy, id);
	}
}
#else
int cta_eth_phy_monitor(unsigned short phy)
{
	volatile unsigned short val;
	int ret, j;

	if (cta_switch_monitor_status() == 0)
		return 0;

	ret = 0;
#if defined(CONFIG_EPHY_FPGA)
	/* 20131211 */
	/* Swap to page 2 */
	cta_eth_mdio_wr(phy, 31, 2);
	/* Read 0x17 reg */
	val = cta_eth_mdio_rd(phy, 23);
	if (((val > 0x0000) && (val < 0x00ff)) ||
		((val > 0xff00) && (val < 0xffff))) {
		;
	} else {
		cta_switch_inc_phy_p20x17();
		ret = 1;
		goto exit;
	}

	val = cta_eth_mdio_rd(phy, 21);
	val &= ~0x7e00;		/* bit[14:9]: clear index */
	val |= 0x0800;		/* bit[14:9]: index=4 */
	cta_eth_mdio_wr(phy, 21, val);
	/* Read 0x15 reg */
	val = cta_eth_mdio_rd(phy, 21);
	val &= 0x00ff;		/* bit[7:0}: snr value */
	if (((val > 0x0010) && (val < 0x007f))) {
		;
	} else {
		cta_switch_inc_phy_p20x15();
		ret = 1;
		goto exit;
	}
#endif

	/* 20131213 */
	/* Wait 1ms */
	for(j=0; j<0x8000; j++);
	/* Swap to page 4 */

	cta_eth_mdio_wr(phy, 31, 4);
	/* Read 0x14 reg */
	val = cta_eth_mdio_rd(phy, 20);

	/* Wait 800ms */
	for(j=0; j<0x17e0000; j++);
	/* Read 0x14 reg 2nd */
	val = cta_eth_mdio_rd(phy, 20);

	/* Check bit[15:14] */
	if (0x8000 == (val & 0xc000)) {
		;
	} else {
		cta_switch_inc_phy_p40x14();
		ret = 1;
		goto exit;
	}
exit:
	/* Swap to page 0 */
	cta_eth_mdio_wr(phy, 31, 0);
	return ret;
}
#endif

#if defined(CONFIG_PANTHER_FPGA)

//#define POLLING_TIMER
#if defined(POLLING_TIMER)
#define PHY_POLLING_INTERVAL        ( HZ )
static struct timer_list phy_polling_timer;
#endif

#endif

/*!-----------------------------------------------------------------------------
 *        cta_eth_hwreset:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_hwreset(void)
{
	DBGPRINTF(DBG_INIT, "cta_eth_hwreset\n");
	ETH_REG32(MSRST) = 0xfff;
	ETH_REG32(MSRST) = 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_txd_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_txd_init(txctrl_t *ptxc)
{
	/* Initialize the transmit buffer descriptors. */
	DBGPRINTF(DBG_INIT, "ifcta_eth: init %d txd at %08x\n", ptxc->num_txd,  (unsigned int) ptxc->ptxd);
	/* zero all tx desc */
	memset((void *) ptxc->ptxd, 0, ptxc->num_txd * sizeof(txdesc_t));
	/* Init buffer descriptor  */
	memset((void *) ptxc->ptxbuf, 0, ptxc->num_txd * sizeof(bufdesc_t));

	/*  Mark end of ring at the last tx descriptor  */
	ptxc->ptxd[ptxc->num_txd-1].w0 = DS0_END_OF_RING;

	ptxc->num_freetxd = ptxc->num_txd;
	ptxc->tx_head = ptxc->tx_tail = 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_rxd_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_rxd_init(rxctrl_t *prxc)
{
	/* Initialize the receive buffer descriptors. */
	int i;
	rxdesc_t *prxd;

	DBGPRINTF(DBG_INIT, "ifcta_eth: init %d rxd at %08x\n",
			prxc->num_rxd, (unsigned int) prxc->prxd);

	prxd = prxc->prxd;
	for (i = prxc->num_rxd; i>0; i--, prxd++)
	{
		prxd->w1 = 0;
		prxd->w0 = DS0_RX_OWNBIT;
	}
	(--prxd)->w0 = DS0_END_OF_RING|DS0_RX_OWNBIT;

	prxc->num_freerxd = prxc->num_rxd;
	prxc->rx_head = prxc->rx_tail = 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_txd_free:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_txd_free(txctrl_t *ptxc)
{
	int i;
	void *buf;
	bufdesc_t *ptxbuf = &ptxc->ptxbuf[0];

	/* free buffer */
	for(i=0; i < ptxc->num_txd; i++,ptxbuf++)
	{
		if ((buf=ptxbuf->buf))
		{
			eth_drv_freebuf(buf);
		}
	}
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_rx_start:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_rx_start(void)
{
	ETH_REG32(MCR) |= (MCR_RXDMA_EN | MCR_RXMAC_EN);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_tx_start:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_tx_start(void)
{
	ETH_REG32(MCR) |= (MCR_TXDMA_EN | MCR_TXMAC_EN);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_hwd_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_hwd_init(void)
{
	unsigned short i,j;
	unsigned long x;

	for(i=0;i<24;i++)
	{
		for (j=0; j < 4; j++)
		{
			while (HWD_RDY & ETH_REG32(HWDESC));
			x = HWD_RDY|HWD_WR |((unsigned long)i<<HWD_IDX_S) | ((unsigned long)j<<HWD_HWI_S) ;
			if (j==0)
			{
				x |= ((unsigned long)DS0_RX_OWNBIT >>16)  |
					((i==23)?  (DS0_END_OF_RING >>16) : 0 ) ;
			}
			//diag_printf("x=%08x\n", x);
			ETH_REG32(HWDESC) = x;
		}
	}
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_hwinit:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_hwinit(void)
{
	DBGPRINTF(DBG_INIT, "%s\n", __func__);

	/*  descriptor base  */
	MREG(MHWDB) = (unsigned int) pcta_eth->hw.prxd;
	MREG(MTXDB) = (unsigned int) pcta_eth->tx.ptxd;
	MREG(MSWDB) = (unsigned int) pcta_eth->rx.prxd;
	MREG(MAC_RXCR) = 0x08401004;

#if defined(CONFIG_PANTHER_FPGA)
	MREG(MFP) = 0xcffff;
#else
	//ETH_REG32(MFP) = 0x2dffff;
	MREG(MFP) = 0x10ffff;
#endif

	ETH_REG32(MVL01) = 0x10020003;
	ETH_REG32(MVL23) = 0x10000000;
	ETH_REG32(MVL45) = 0x10000000;
	ETH_REG32(MVL67) = 0x10000000;

	MREG(MCR)	= MCR_INIT;
	MREG(MRXCR) = MRXCR_INIT;
	MREG(MAUTO) = 0x81ff0030;
	MREG(MDROPS)= MDROPS_INIT;
	MREG(MMEMB) = 0;
	MREG(ML3)	= ML3_INIT;
#if defined(CONFIG_TODO)
	MREG(MTMII) = 0xc0000000;		//bit[31] 1:send nibble pause to switch if run out software desc.
									//bit[30] 1:turn on TMII TX machanism.
#endif

	/* Clear all pending interrupts */
	MREG(MIC)	= MREG(MIS);
	MREG(MPKTSZ)= 0x00fa05fe;		//lookup serch time, unit:clk.
	MREG(MTW) 	= 0x00000f08;		//tx retry number

	/*  Enable interrupt  */
	// enable in cta_eth_if_start
	//ETH_REG32(MIM) &= ~MIM_INIT;

	DBGPRINTF(DBG_INIT, "\nInit switch, LAN(VID 4095):p%d, WAN(VID 0): p%d\n",
							PORT_ETH1_IDX, PORT_ETH0_IDX);

	printk("LAN port:%d(VID %d), WAN port:%d(VID %d)\n",
		   lan_interface_port(), lan_interface_vlan_id(), wan_interface_port(), wan_interface_vlan_id());

	if((cta_switch_cfg[1] & CSW_CTRL_PHY_INTERNAL) && (cta_switch_cfg[1] & CSW_CTRL_IF_ENABLE))
	{
		printk("Internal PHY addr: %d\n", (int)(swcfg_p1_phy_addr() & CSW_CTRL_PHY_ADDR));
		SMIREG_WRITE32(EPHY_ADDR, (swcfg_p1_phy_addr() & CSW_CTRL_PHY_ADDR));
	}

	/* New switch architecture*/
	cta_switch_init(1);

	/* HW NAT initialize */
	cta_hnat_init();
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_buf_free:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int cta_eth_buf_free(void)
{
	unsigned long cb,nb,*p;

	if (0 ==(cb = ETH_REG32(MBUFB)))
	{
		DBGPRINTF(DBG_INIT, "%s MBUF=0,can't feee\n",__func__);
		return -1;
	}
	for (;cb;)
	{
		if (3 & (unsigned long)cb )
		{
			DBGPRINTF(DBG_INIT, "%s unaligned BUF=%08lx,can't free\n",__func__, cb);
			break;
		}
		p = (unsigned long*)nonca_addr((cb<<8)>>8);
		nb = *p; // next
		eth_drv_freebuf((void*)(PHYTOVA((cb<<8)>>8)-BUF_HW_OFS)) ;
		cb = nb;
	}
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_buf_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int cta_eth_buf_init(void)
{
	unsigned long first, cb, nb;
	int i;

	if (!(cb = first = (unsigned long)eth_drv_getbuf()))
	{
		goto no_buf;
	}
	ETH_REG32(MBUFB) = virtophy(first)+BUF_HW_OFS ;
	printk("cta_eth_buf_init:%d\n", NUM_MBUF);
	for (i = 0; i < NUM_MBUF-1; i++)
	{
		if (!(nb = (unsigned long)eth_drv_getbuf()))
		{
			goto free;
		}
		/* current buf's head point to next buf's head */
		*(unsigned long *)(nonca_addr(cb)+BUF_HW_OFS) = virtophy(nb)+BUF_HW_OFS;
		/* saved the pointer (MSB 16bits) of buffer */
		//*(unsigned char*)(nonca_addr(cb)+BUF_MSB_OFS) = ((unsigned int)cb)>>24 ;
		cb = nb ;
	}
	*(unsigned long *)(nonca_addr(cb)+BUF_HW_OFS) = 0;
	ETH_REG32(MLBUF) = virtophy(cb)+BUF_HW_OFS;
	DBGPRINTF(DBG_INIT, "%s: MBUF=%x, MLBUF=%x\n", __func__, ETH_REG32(MBUFB), ETH_REG32(MLBUF));

	return 0;

free:
	cta_eth_buf_free();
no_buf:
	DBGPRINTF(DBG_INIT, "%s:no buf\n",__func__);
	return -1;
}

/*!-----------------------------------------------------------------------------
 *        cta_smi_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_smi_init(void)
{
	SMIREG_WRITE32(SMI_CLK_RST, MCR_SMI_DIV << MCR_SMI_S);
	DBGPRINTF(DBG_PHY,"SMI_CR=%lx\n", (SMI_BASE+SMI_CLK_RST));
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_init(void)
{
	/*
		This function attaches the Ethernet interface to the OS. It also calls various driver
		initialization functions to allocate drivers resources and initialize the device.
	*/
	pcta_eth = &cta_eth;

	DBGPRINTF(DBG_INIT, "%s\n", __func__);

	cta_smi_init(); // !!!!! What is this

#if defined(CONFIG_TODO)
#if !defined(__ECOS) && (CHEETAH_VERSION_CODE >= CHEETAH_VERSION(2,0))
	/* reset HNAT & Switch */
	preempt_disable();

	/* to reset Switch, Wifi/HNAT/Switch clocks need to be turn on */
	/* turn on WIFI clock */
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) & (~0x100UL));
	/* turn on HNAT clock */
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) & (~0x200UL));
	/* turn on Switch clock */
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) & (~0x800UL));

	/* reset HNAT */
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) | 0x2UL);
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) & (~0x2UL));
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) | 0x2UL);

	/* reset Switch */
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) | 0x8UL);
	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) & (~0x8UL));

	mdelay(10);

	ETH_REG32(HW_RESET) = (ETH_REG32(HW_RESET) | 0x8UL);
	preempt_enable();
#endif
#endif

	/* Make sure the uncached memory area is not cached!! */
	HAL_DCACHE_FLUSH(g_desc, DESC_SZ);

	/* Reset switch hardware */
	cta_eth_hwreset(); // NEED MODIFY ???

	//cta_eth_phyinit();
	pcta_eth->tx.ptxd = (txdesc_t *) nonca_addr((unsigned int)g_desc);
	pcta_eth->rx.prxd = (rxdesc_t *)(pcta_eth->tx.ptxd + NUM_TX_DESC);
	pcta_eth->hw.prxd = (rxdesc_t *)(pcta_eth->rx.prxd + NUM_RX_DESC);

	pcta_eth->tx.ptxbuf = &g_txbd[0];
	pcta_eth->tx.num_txd = NUM_TX_DESC;
	pcta_eth->rx.num_rxd = NUM_RX_DESC;
	pcta_eth->hw.num_rxd = NUM_HW_DESC;

	if (cta_eth_buf_init())
		return -1;

	cta_eth_txd_init(&pcta_eth->tx);
	cta_eth_rxd_init(&pcta_eth->rx);
	cta_eth_rxd_init(&pcta_eth->hw);
	cta_eth_hwd_init();

	pcta_eth->vector = CTA_ETH_IRQ;

#ifdef USE_TASKLET
	tasklet_init(&tasklet, (void *)cta_eth_deliver, 0);
#endif
	/* request non-shared interrupt */
	if(request_irq(pcta_eth->vector, (irq_handler_t)&cta_eth_isr,
					0, "ethernet", pcta_eth) != 0)
	{
		diag_printf( "%s: irq %d request fail\n", __func__, pcta_eth->vector );
		return -ENODEV;
	}
	/* create IRQ handler with IRQ disabled, mimic the eCos behavior */
	disable_irq(pcta_eth->vector);
	/* Initialize hardware */
	cta_eth_hwinit();

	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_if_start:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_if_start(int unit)
{
	if_cta_eth_t *pifcta_eth;
	unsigned short if_bit;

	pifcta_eth = pcta_eth->if_array[unit];
#if defined(CONFIG_PANTHER)
	if(unit == 0)
		pifcta_eth->phy_addr = switch_port_to_phy_addr(PORT_ETH1_IDX);
	else
		pifcta_eth->phy_addr = switch_port_to_phy_addr(PORT_ETH0_IDX);
#else
	if(unit == 0)
		pifcta_eth->phy_addr = 0;
	else
		pifcta_eth->phy_addr = 4;
#endif
	CYG_ASSERTC(pifcta_eth != 0);

	if_bit = 1 << unit;

	DBGPRINTF(DBG_INIT, "if321x_if_start(%d)\n", unit);
	if(pcta_eth->act_ifmask & if_bit)
		return;

	if(pcta_eth->act_ifmask == 0)
	{
		pcta_eth->sc_main = pifcta_eth->sc;
		//TODO , add initial code

		/*  Enable interrupt  */
		ETH_REG32(MIM) &= ~MIM_INIT;
		cyg_drv_interrupt_unmask(pcta_eth->vector);
	}
	pcta_eth->act_ifmask |= if_bit;
	DBGPRINTF(DBG_INIT, "if321x_if_start: act_ifmask=%x\n", pcta_eth->act_ifmask);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_rx_stop:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_rx_stop(rxctrl_t *prxc)
{
	int i;
	rxdesc_t *prxd;
	void *bufp;

	ETH_REG32(MCR) &= ~(MCR_RXDMA_EN | MCR_RXMAC_EN);
	for(i = 0; i < prxc->num_rxd; i++)
	{
		prxd = prxc->prxd + i;
		if(((DS0_RX_OWNBIT | DS0_TX_OWNBIT) >> 16) & *(unsigned short *) prxd)
		{
			prxd->w0 &= ~(DS0_RX_OWNBIT | DS0_TX_OWNBIT);
			prxd->w0 |= DS0_DROP_OF_PACKET;
			if(prxd->w1&0x0fffffff)
			{
				bufp = (void*)DESC_W1_TO_VBUF(prxd->w1);
				eth_drv_freebuf(bufp);
			}
		}
	}
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_tx_stop:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_tx_stop(txctrl_t *ptxc)
{
	int i;
	txdesc_t *ptxd;

	ETH_REG32(MCR) &= ~(MCR_TXDMA_EN | MCR_TXMAC_EN);
	for(i = 0; i < ptxc->num_txd; i++)
	{
		ptxd = ptxc->ptxd + i;
		if(((DS0_RX_OWNBIT | DS0_TX_OWNBIT) >> 16) & *(unsigned short *) ptxd)
		{
			ptxd->w0 &= ~(DS0_RX_OWNBIT | DS0_TX_OWNBIT);
			ptxd->w0 |= DS0_DROP_OF_PACKET;
		}
	}
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_if_stop:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_if_stop(int unit)
{
	if_cta_eth_t *pifcta_eth;
	unsigned short if_bit;

	pifcta_eth = pcta_eth->if_array[unit];
	if_bit = 1 << unit;

	DBGPRINTF(DBG_INIT, "cta_eth_if_stop(%d) ifp=%x act_ifmask=%x\n",
				unit, (int) pifcta_eth, (int) pcta_eth->act_ifmask);
	CYG_ASSERTC(pifcta_eth != 0);
	if (!(pcta_eth->act_ifmask & if_bit))
		return;

	if ((pcta_eth->act_ifmask &= ~if_bit) == 0)
	{
#if defined(USE_TASKLET)
#if defined(INTERRUPT_MITIGATION)
		hrtimer_cancel(&intr_timer);
#endif
		//tasklet_disable(&tasklet);

		/* Disable interrupt */
		ETH_REG32(MIM)|= MIM_INIT;
		cyg_drv_interrupt_mask(pcta_eth->vector);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10);

		if(0)
		{
			cta_eth_rx_stop(&pcta_eth->rx);
			cta_eth_tx_stop(&pcta_eth->tx);
			cta_eth_txd_free(&pcta_eth->tx);
		}
#else
		/* Disable interrupt */
		ETH_REG32(MIM)|= MIM_INIT;
		cyg_drv_interrupt_mask(pcta_eth->vector);
		if (0)
		{
			cta_eth_rx_stop(&pcta_eth->rx);
			cta_eth_tx_stop(&pcta_eth->tx);
			cta_eth_txd_free(&pcta_eth->tx);
		}
		diag_printf("Don't free NOW!\n");
#endif
		pcta_eth->sc_main = 0;
		return;
	}
	DBGPRINTF(DBG_INIT, "cheetah_if_stop: return act_ifmask=%x\n", pcta_eth->act_ifmask);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_add_if:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_add_if(int unit, if_cta_eth_t *pifcta_eth)
{
	int status = -1;

	if(unit <0 || unit >= MAX_IF_NUM)
		return -1;

	if(pcta_eth->if_array[unit] == 0)
	{
		pcta_eth->if_array[unit] = pifcta_eth;
		pifcta_eth->to_vlan = ((unit|0x8) << 8);
		pcta_eth->if_cnt++;
		status = 0;
	}

	return status;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_del_if:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_del_if(int unit)
{
//		if_cta_eth_t *pifcta_eth;

	if(unit < 0 || unit >= MAX_IF_NUM)
		return -1;

	DBGPRINTF(DBG_INIT, "cta_eth_del_if(%d)\n", unit);
//		pifcta_eth = pcta_eth->if_array[unit];
//		CYG_ASSERTC(pifcta_eth != 0);
    CYG_ASSERTC(pcta_eth->if_array[unit] != 0);
	pcta_eth->if_array[unit] = 0;

	return 0;
}

#if defined(INTERRUPT_MITIGATION)
static enum hrtimer_restart cta_eth_tasklet_schedule(struct hrtimer *timer)
{
	tasklet_schedule(&tasklet);
	return HRTIMER_NORESTART;
}
#endif

/*!-----------------------------------------------------------------------------
 *        cta_eth_isr:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static irqreturn_t cta_eth_isr(int irq, void *data)
{
#if defined(INTERRUPT_MITIGATION)
	int delayed_tasklet_schedule = 0;
#endif

	//struct net_device *dev = (struct net_device *)dev_id;
	ETH_REG32(MIM)|= MIM_INIT;

#ifdef USE_TASKLET

#if defined(INTERRUPT_MITIGATION)
	if((((rx_count+tx_done_count)>intr_tolerant_count)||(intr_count>intr_tolerant_count))
	    && ((last_jiffies==jiffies)||((last_jiffies+1)==jiffies)))
	{
		hrtimer_start(&intr_timer, ktime_set(0, intr_minigation_ticks*1000), HRTIMER_MODE_REL);
		delayed_tasklet_schedule = 1;
	}

	if(jiffies != last_jiffies)
	{
		//printk("(%d %d %d)\n", intr_count, rx_count, tx_done_count);
		last_jiffies = jiffies;
		rx_count = 0;
		tx_done_count = 0;
		intr_count = 0;
	}

	intr_count++;
	if(delayed_tasklet_schedule)
		return IRQ_HANDLED;
#endif

	tasklet_schedule(&tasklet);
#endif
#ifdef  USE_NAPI
	struct napi_struct *napi = &pcta_eth->cta_napi;
	if(likely(napi_schedule_prep(napi)))
		__napi_schedule(napi);
#endif
	return IRQ_HANDLED;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_rxint:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_rxint(rxctrl_t *prxc, int budget)
{
	/* The receive interrupt service handler. */
	unsigned int w0, w1;
	unsigned char *nbufp, *bufp;
	unsigned char *pkt;
	rxdesc_t *prxd;
	short frame_len=0;
	if_cta_eth_t *pifcta_eth;
	int loop;

	prxd = prxc->prxd;

	DBGPRINTF(DBG_RXMORE, "rxint, rxc=%x\n", (unsigned int) prxc);

	prxd = &prxc->prxd[prxc->rx_head];
	w0 = prxd->w0;
	w1 = prxd->w1;
	loop=0;

	while (!(w0 & (DS0_RX_OWNBIT)) && (loop < budget))
	{
#if defined(INTERRUPT_MITIGATION)
		rx_count++;
#endif
		loop++;
		DBGPRINTF(DBG_RXMORE, "rxint, DS0 w0=%x w1=%x\n",  w0, w1);
		pkt = 0;
		/* For statistics, locate the interface first */
		DBGPRINTF(DBG_RXMORE, "vl=%x\n", RX_VIDX(w0));
		if ((pifcta_eth = pcta_eth->if_array[0]) == 0)
		{
			/* The port has been released. */
			/* Drop the packet  */
			prxd->w1=((w1>>6)<<6) + BUF_HW_OFS;
			//pifcta_eth->sc->stats.rx_dropped++;
			goto next;
		}

		if(w0 & DS0_DROP_OF_PACKET)
		{
			pifcta_eth->sc->stats.rx_errors++;
			pifcta_eth->sc->stats.rx_dropped++;

#if 1
            {
                int i;
                unsigned char *p;

                printk("======== ETH Drop: %x %x ========\n", w0, w1);

                p = (unsigned char *) w1;

                for(i=0;i<128;i++)
                {
                    printk("%02x ", p[i]);
                    if(15==(i%16))
                        printk("\n");
                }
                printk("======== ======= ======= ========\n");
            }
#endif

			if(w0 & DS0_ECRC)
			{
				pifcta_eth->sc->stats.rx_crc_errors++;
				pifcta_eth->sc->stats.rx_frame_errors++;
			}

			if(w0 & (DS0_L2LE | DS0_FOR))
				pifcta_eth->sc->stats.rx_frame_errors++;

			//if(w0 & (DS0_EIPCS|DS0_EL4CS))

			/*  Drop the packet  */
			prxd->w1=((w1>>6)<<6) + BUF_HW_OFS;
			goto next;
		}

		frame_len = RX_FRAMELEN(w0);

		pifcta_eth->sc->stats.rx_bytes += frame_len;
		pifcta_eth->sc->stats.rx_packets++;

#if 0
		if(w0 & (DS0_ISBC | DS0_ISMC))
			pifcta_eth->sc->stats.multicast++;
#endif

		prxc->rx_pktcnt++;
		/* Indicate the packet to the upper layer */
		if((pifcta_eth->flags & PRIVIFFG_ACTIVE)
			&& (0!=(nbufp=(char*)eth_drv_getbuf())))
		{
			pkt = (unsigned char*)DESC_W1_TO_VPKT(w1);
			bufp =  (unsigned char*)DESC_W1_TO_VBUF(w1);
			DC_FLUSH_RX(nbufp, RX_CACHE_FLUSH_SIZE);
			nbufp += BUF_HW_OFS ;
			prxd->w1 = (unsigned long)nbufp;
		}
		else
		{
			prxd->w1=((w1>>6)<<6) + BUF_HW_OFS;
			pifcta_eth->sc->stats.rx_dropped++;
			pifcta_eth->sc->stats.rx_fifo_errors++;
		}
next:
		/* Release the buffer to hardware */
		if (++prxc->rx_head == prxc->num_rxd)
		{
			prxd->w0 = DS0_RX_OWNBIT | DS0_END_OF_RING;
			prxc->rx_head = 0;
		}
		else
			prxd->w0 = DS0_RX_OWNBIT;

		/* dispatch to upper layer */
		if(pkt != 0)
		{
#if defined(CONFIG_TODO)
			int hw_nat;
			//diag_dump_buf_16bit(bufp+0x40,64);
			//printf("hash[0]:%x, hash[1]:%x\n", *(int *)(bufp+4), *(int *)(bufp+12));

			if((hw_nat = cta_hnat_rx_check(bufp, pkt, w0, w1)) < 0)
			{
				eth_drv_freebuf(bufp);
				goto next_desc;
			}
#endif
#if defined(CONFIG_TODO)
			if(cta_hnat_sdmz_rx_check(pifcta_eth, bufp, pkt, w0, w1, frame_len) == 0)
				eth_drv_recv1(pifcta_eth->sc, pkt, (short)frame_len,
							(char**)&bufp, (void *)hw_nat, w0, w1); //4byte VLAN
#else
				eth_drv_recv1(pifcta_eth->sc, pkt, (short)frame_len,
							(char**)&bufp, (void *)NULL, w0, w1); //4byte VLAN
#endif
		}
#if defined(CONFIG_TODO)
next_desc:
#endif
		prxd = &prxc->prxd[prxc->rx_head];
		w0 = prxd->w0;
		w1 = prxd->w1;
	}

	DBGPRINTF(DBG_RXMORE, "rxint end, prxc=%x\n", (unsigned int)prxc);
	return loop;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_txint:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_txint(txctrl_t *ptxc, int wait_free)
{
	/* The transmit interrupt service handler. */
	volatile txdesc_t *ptxd;
	bufdesc_t *ptxbuf;

	DBGPRINTF(DBG_TXMORE, "txint, txc=%x\n", (unsigned int) ptxc);

	ptxd = ptxc->ptxd;
	ptxbuf = ptxc->ptxbuf;

	if(wait_free && (ptxd[ptxc->tx_tail].w0 & DS0_TX_OWNBIT))
	{
		int i = 0;
		int j;
		diag_printf("txbusy %s(%d) tail=%d head=%d\n", __func__, __LINE__, ptxc->tx_head, ptxc->tx_tail);
		for (j=0;j < ptxc->num_txd;j++)
		{
				if((j&3)==0)
					diag_printf("\n");
				diag_printf("%08x-%08x ", ptxd[j].w0, ptxd[j].w1 );
		}
		while((ptxd[ptxc->tx_tail].w0&DS0_TX_OWNBIT))
		{
			i++;
		}
		diag_printf("i=%d\n", i);
	}
	while((!(ptxd[ptxc->tx_tail].w0 & DS0_TX_OWNBIT)) && ptxd[ptxc->tx_tail].w1)
	{
#if defined(INTERRUPT_MITIGATION)
		tx_done_count++;
#endif

		DBGPRINTF(DBG_TX, "TD: %p, %x, %x, %d\n", &ptxd[ptxc->tx_tail], ptxd[ptxc->tx_tail].w0,
							ptxd[ptxc->tx_tail].w1, ptxc->num_freetxd);
		DBGPRINTF(DBG_TX, ",%x\n", (unsigned int)ptxbuf[ptxc->tx_tail].buf);
		if (ptxbuf[ptxc->tx_tail].buf != 0)
		{
			ptxc->tx_ok++;

			//if(g_eth_dev)
			//    g_eth_dev->trans_start = jiffies;

			if ((unsigned int)ptxbuf[ptxc->tx_tail].buf >= 0x80000000)
			{
#if 1
				dev_kfree_skb_any(ptxbuf[ptxc->tx_tail].buf);
#else
#ifdef	USE_TASKLET
				dev_kfree_skb(ptxbuf[ptxc->tx_tail].buf);
#else
				dev_kfree_skb_irq(ptxbuf[ptxc->tx_tail].buf);
#endif
#endif
			}

			ptxbuf[ptxc->tx_tail].sc = 0;
			ptxbuf[ptxc->tx_tail].buf = 0;
            ptxd[ptxc->tx_tail].w1 = 0;
		}

		if (++ptxc->tx_tail == ptxc->num_txd)
			ptxc->tx_tail = 0;
	}

	DBGPRINTF(DBG_TXMORE, "txint end, txc=%x\n", (unsigned int) ptxc);
}

/*!-----------------------------------------------------------------------------
 *        eth_rx_mode:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void eth_rx_mode(struct net_device *dev)
{
	/* received include address un-matched */
	if(dev->flags & IFF_PROMISC)
		ETH_REG32(MDROPS) &= ~(1<<7);
	return;
}

static struct net_device_stats *eth_get_stats(struct net_device *dev)
{
	return &dev->stats;
}

static void eth_tx_timeout(struct net_device *dev)
{
	printk("%s:Ethernet transmit timed out\n", dev->name);

	//dev->trans_start = jiffies;
	dev->stats.tx_errors++;
}

/*!-----------------------------------------------------------------------------
 *        eth_vlan_rx_add_vid:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_vlan_rx_add_vid(struct net_device *dev, __be16 proto, unsigned short vid)
{
    /* not implement yet! */
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        eth_vlan_rx_kill_vid:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_vlan_rx_kill_vid(struct net_device *dev, __be16 proto, u16 vid)
{
    /* not implement yet! */
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        eth_start:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_start(struct net_device *dev)
{
	if_cta_eth_t *pifcta_eth;

	pifcta_eth = (if_cta_eth_t *) netdev_priv(dev);

	CYG_ASSERTC(pifcta_eth->sc == dev);

	DBGPRINTF(DBG_INFO, "eth_start(%d)\n", pifcta_eth->unit);

	if(!(pifcta_eth->flags & PRIVIFFG_ACTIVE))
	{
		pifcta_eth->flags |= PRIVIFFG_ACTIVE;
		cta_eth_if_start(pifcta_eth->unit);
	}

	cta_eth_rx_start();
	cta_eth_tx_start();

	memcpy(pifcta_eth->macaddr, dev->dev_addr, 6);
#if 0
	cta_eth_setmac(pifcta_eth->unit, pifcta_eth->macaddr);
#endif

#ifdef USE_NAPI
	napi_enable(&(cta_eth.cta_napi));
#endif

	netif_start_queue (dev);

	DBGPRINTF(DBG_INFO, "eth_start(%d): done\n", pifcta_eth->unit);
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        eth_stop:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_stop( struct net_device *dev )
{
	if_cta_eth_t *pifcta_eth;

	pifcta_eth = (if_cta_eth_t *) netdev_priv(dev);

	CYG_ASSERTC(pifcta_eth->sc == sc);
	DBGPRINTF(DBG_INFO, "eth_stop(%d)\n", pifcta_eth->unit);
#ifdef USE_NAPI
	napi_disable(&(cta_eth.cta_napi));
#endif

	netif_stop_queue(dev);
	if(pifcta_eth->flags & PRIVIFFG_ACTIVE)
	{
		pifcta_eth->flags &= ~PRIVIFFG_ACTIVE;
		cta_eth_if_stop(pifcta_eth->unit);
	}

	DBGPRINTF(DBG_INFO, "eth_stop(%d): done\n", pifcta_eth->unit);
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        eth_ioctl:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_ioctl( struct net_device *dev, struct ifreq *ifr, int cmd)
{
	if_cta_eth_t *pdp= (if_cta_eth_t*) netdev_priv(dev);
	struct mii_ioctl_data *data_ptr = (struct mii_ioctl_data *) &(ifr->ifr_data);
	//DBGPRINTF(DBG_INFO, "eth_ioctl(%d):cmd=%x ifr=%x\n", pifcta_eth->unit, cmd, (int)ifr);
	switch(cmd)
	{
	case SIOCGMIIPHY:
		data_ptr->phy_id = pdp->phy_addr & 0x1f;
		break;
	case SIOCGMIIREG:
		cta_eth_mdio(pdp->phy_addr,data_ptr->reg_num & 0x1f,(&(data_ptr->val_out)),0);
		break;
	case SIOCSMIIREG:
		cta_eth_mdio(pdp->phy_addr,data_ptr->reg_num & 0x1f,&(data_ptr->val_out),1);
		break;
	case SIOCSIFHWADDR:
		printk("GOT YOU\n");
		break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_set_mac:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int eth_set_mac_address(struct net_device *dev, void *addr)
{
    struct sockaddr *p = addr;
    unsigned char *mac = p->sa_data;
    int i;

    if(!is_valid_ether_addr(mac))
        return -EADDRNOTAVAIL;

    for(i = 0; i < 6; i++)
    {
        dev->dev_addr[i] = mac[i];
    }

    return 0;
}

static int eth_vlan_set_mac_address(struct net_device *dev, void *addr)
{
	unsigned int mac_hi, mac_lo;
	struct sockaddr *p = addr;
	unsigned char *mac = p->sa_data;
	struct vlan_dev_priv *vlan = vlan_dev_priv(dev);

	if(!is_valid_ether_addr(mac))
		return -EADDRNOTAVAIL;

	/* To do: sync with switch flow ctrl mac */
	mac_hi = (mac[0] << 8) | mac[1] ;
	mac_lo = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

	if(vlan->vlan_id == wan_interface_vlan_id())
	{
		ETH_REG32(MSA10) = mac_hi;
		ETH_REG32(MSA11) = mac_lo;
#if defined(CONFIG_TODO)
		register_macaddr(1,mac);
#endif
	}
	else if(vlan->vlan_id == lan_interface_vlan_id())
	{
		ETH_REG32(MSA00) = mac_hi;
		ETH_REG32(MSA01) = mac_lo;
#if defined(CONFIG_TODO)
		register_macaddr(1,mac);
#endif
	}
	else
	{
		printk("eth_vlan_set_mac: unknown vlan device\n");
		return -EINVAL;
	}

	return 0;
}

#ifdef BRIDGE_FILTER
/*!-----------------------------------------------------------------------------
 *        cta_eth_rx_filter:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int cta_eth_rx_filter(char *data)
{
	static char all_ones[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	/* broadcast */
	if(!memcmp(data, all_ones, 6))
		return 0;
	/* multicast */
	if(data[0] == 0x01)
		return 0;
	/* myself, TODO: partial match */
	if(!memcmp(mymac, data, 5))
		return 0;
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_filter:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int cta_eth_filter(struct sk_buff *skb, char *data, int tx)
{
	if( cta_eth_filter_mode == 0) //skip?
		return 0;

	if(tx)
	{
		return 0;
		/* if not from our self, drop */
		if(!memcmp(mymac, data+6, 5 ))
			return 0;
	}
	else
	{
#ifdef USED_VLAN
		/* DHCP discover/request from WAN port, drop */
		if ((*(data+15) == 0x01) && (*(data+27) == 0x11) && (*(data+39) == 0x44))
			goto drop;
#endif
		if(!cta_eth_rx_filter(data))
			return 0;
	}
drop:
	dev_kfree_skb_any(skb);
	return 1;
}
#endif

/*!-----------------------------------------------------------------------------
 *        eth_send:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
u32 ptxd_desc_rdata;
#if defined(DETECT_A1_AND_APPLY_TX_RATE_LIMIT)
int __eth_send(struct sk_buff *skb, struct net_device *dev)
#else
int eth_send(struct sk_buff *skb, struct net_device *dev )
#endif
{
	/*	This function transmits the given frame into the DMA engine of the
		Ethernet controller. If the transmit data path is enable, the frame
		will be transmit at once.
		sc: the pointer for the soft control of Ethernet driver.
		sg_list: the packet which is specified via a ¡§scatter-gather¡¨ list.
		sg_len: number of packet scatter in the list.
		total_len: the packet total length.
		key: the packet address for upper layer rele
	*/
#if defined(ETH_DBG)
	if_cta_eth_t *pifcta_eth = (if_cta_eth_t *) netdev_priv(dev);
#endif
	txctrl_t *ptxc = &pcta_eth->tx;
	volatile txdesc_t *ptxd_start;
	int total_len = skb->len;
	unsigned char vidx=0;
	unsigned short vid ;
#if defined(CONFIG_TODO)
	semi_cb_t *semi_cb = SKB2SEMICB(skb);
#endif

	unsigned long flags;

	CYG_ASSERT(pifcta_eth->sc == dev, "Invalid device pointer");

	DBGPRINTF(DBG_TX, "eth_send(%d) key=%08x, len=%d\n",
			 pifcta_eth->unit, (unsigned int)skb, total_len);

#ifdef BRIDGE_FILTER
	if(cta_eth_filter(skb, skb->data, 1))
		return 0;
#endif

#if defined(ETH_DBG)
	if(DBG_TXMORE2 & DBG_MASK)
	{
		int j;
		int dlen = (skb->len > MAX_PKT_DUMP)? MAX_PKT_DUMP : skb->len;

		printk(">>> %s skb->data=%x , len=%x", __func__, (int)skb->data, skb->len);
		for(j=0;j<dlen;j++)
		{
			if(0==(j&0x1f))
				printk("\n%03x:", j);
			printk(" %02x",skb->data[j]&0xff);
		}
		printk("\n");
	}
#endif

	if(total_len > 1518)
	{
		dev->stats.tx_dropped++;
		dev->stats.tx_packets++;
		dev->stats.tx_errors++;
		goto drop_tx;
	}

	ptxc->tx_pktcnt ++;

	if(skb_vlan_tag_present(skb)>=0)
	{
		vid = skb_vlan_tag_get(skb);
		if(wan_interface_vlan_id()==vid)
			vidx = 1;
	}

#if defined(CONFIG_TODO)
	if(force_vo) {
		struct iphdr *ip;
		int hlen;
		ip = (struct iphdr *)(((char *)skb->data) + 14);
		hlen = ip->ihl << 2;
		ip->tos = 0xe0;
		ip->check = 0;
		ip->check = ip_cksum((unsigned short *)ip, hlen);
	}
#endif

#if defined(CONFIG_TODO)
	if(!IS_HWMOD(semi_cb))
	{
		cta_hnat_sdmz_tx_check(skb->data);
		cta_hnat_learn_mac(skb->data, 1, vidx);
	}
#endif
	DC_FLUSH_TX((void *)skb->data, skb->len);

	skb->dev = dev;

	/* frame len can not less than ETH_MIN_FRAMELEN */
	if(total_len < ETH_MIN_FRAMELEN)
	{
		/* then reinit total len */
		total_len += (ETH_MIN_FRAMELEN - total_len);
	}

	spin_lock_irqsave(&panther_eth_lock, flags);

	ptxd_start = &ptxc->ptxd[ptxc->tx_head];

	if((ptxd_start->w0 & DS0_TX_OWNBIT) || ptxd_start->w1)
	{
		dev->stats.tx_packets++;
		dev->stats.tx_dropped++;
		dev->stats.tx_fifo_errors++;
		dev->stats.tx_errors++;
		spin_unlock_irqrestore(&panther_eth_lock, flags);
		goto drop_tx;
	}

	dev->trans_start = jiffies;
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	ptxc->ptxbuf[ptxc->tx_head].sc = dev;
	ptxc->ptxbuf[ptxc->tx_head].buf = (void *) skb;

	if (++ptxc->tx_head == ptxc->num_txd)
		ptxc->tx_head = 0;

#if defined(CONFIG_TODO)
	/* Use one buffers per descriptor */
	if(IS_HWMOD(semi_cb))
	{
		ptxd_start->w1 = semi_cb->w1;
		ptxd_start->w0 = semi_cb->w0 |((0x8|vidx)<<8) | (ptxd_start->w0 & DS0_END_OF_RING);
	}
	else
#endif
	{
		ptxd_start->w1 = virtophy(skb->data);
		ptxd_start->w0 = (ptxd_start->w0 & DS0_END_OF_RING)
					| (total_len<<DS0_LEN_S)
					| (DS0_TX_OWNBIT |DS0_DF)
					| ((0x8|vidx)<<8);
	}

	ptxd_desc_rdata = *((volatile u32 *)&ptxd_start->w0);

	//printk("%p, %x, %x, %d\n",ptxd_start,ptxd_start->w0,ptxd_start->w1,ptxc->num_freetxd);
	/* Kick the hardware to start transmitting */
	ETH_REG32(MTT) = 1;

	spin_unlock_irqrestore(&panther_eth_lock, flags);

	return NETDEV_TX_OK;

drop_tx:
	/* No tx resource for this frame */
	DBGPRINTF(DBG_TX, "eth_send(%d) drop frame %08lx\n",
			 pifcta_eth->unit, (unsigned long)skb);
	/* Drop the frame now */
	ptxc->tx_drop++;
#if 0
	return NETDEV_TX_BUSY;
#else
#if defined(DETECT_A1_AND_APPLY_TX_RATE_LIMIT)
	if(chip_revision>=2)
		dev_kfree_skb(skb);
	else
		dev_kfree_skb_irq(skb);
#else
	dev_kfree_skb(skb);
#endif
	return NETDEV_TX_OK;
#endif
}

#if defined(DETECT_A1_AND_APPLY_TX_RATE_LIMIT)

#include <linux/list.h>

struct list_head tx_pending_list = LIST_HEAD_INIT(tx_pending_list);
struct tx_pending_element
{
	struct list_head list;
	struct sk_buff *skb;
	struct net_device *dev;
};

#define ETH_MAX_PACKET_LEN              1540
#define MAX_ETH_PACKET_QUEUE_DEPTH      200
#define ETH_MIN_INTER_PACKET_TIME_NS    125000 //((1000*1000*1000)/((100*1000*1000/8)/1540))

static int eth_packet_queue_depth;
int64_t ifg = ETH_MIN_INTER_PACKET_TIME_NS;

static struct hrtimer timer_tx;
static enum hrtimer_restart handle_tx(struct hrtimer* timer);
void cta_eth_tx_poll(void)
{
	unsigned long flags;
	struct list_head *element;
	struct tx_pending_element *txe;

	spin_lock_irqsave(&panther_eth_lock, flags);
	if(!list_empty(&tx_pending_list))
	{
		element = tx_pending_list.next;
		txe = list_entry(element, struct tx_pending_element, list);
		eth_packet_queue_depth--;
		if(eth_packet_queue_depth < 0)
			panic("Wrong eth_packet_queue_depth");
		list_del(element);
		spin_unlock_irqrestore(&panther_eth_lock, flags);
		__eth_send(txe->skb, txe->dev);
	}
	else
	{
		spin_unlock_irqrestore(&panther_eth_lock, flags);
	}
}

static enum hrtimer_restart handle_tx(struct hrtimer* timer)
{
	ktime_t current_time = ktime_get();
	ktime_t next_time = ktime_add_ns(current_time, ifg);

	cta_eth_tx_poll();

	current_time = ktime_get();
	hrtimer_forward(&timer_tx, current_time, ktime_sub(next_time, current_time));

	return HRTIMER_RESTART;
}

int eth_send(struct sk_buff *skb, struct net_device *dev)
{
	if(chip_revision>=2)
	{
		return __eth_send(skb, dev);
	}
	else
	{
		unsigned long flags;
		struct tx_pending_element *txe;

		txe = (struct tx_pending_element *) &skb->cb;

		memset(txe, 0, sizeof(struct tx_pending_element));
		txe->skb = skb;
		txe->dev = dev;

		spin_lock_irqsave(&panther_eth_lock, flags);
		if(eth_packet_queue_depth<MAX_ETH_PACKET_QUEUE_DEPTH)
		{
			eth_packet_queue_depth++;
			list_add_tail((struct list_head *) &txe->list, &tx_pending_list);
			spin_unlock_irqrestore(&panther_eth_lock, flags);
		}
		else
		{
			spin_unlock_irqrestore(&panther_eth_lock, flags);
			dev_kfree_skb(skb);
		}

		return NETDEV_TX_OK;
	}
}
#endif

#ifndef USE_NAPI
/*!-----------------------------------------------------------------------------
 *        cta_eth_deliver:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_deliver(struct net_device *sc)
{
	unsigned int int_status;

	int_status = ETH_REG32(MIS);
	/* Ack all interrupts */
	ETH_REG32(MIC) = int_status;

	DBGPRINTF(DBG_DELIVER, "cta_eth_deliver. istatus=%x\n",
				int_status);


	do{
		if(int_status & MIS_SQE)
		{
			//DBGPRINTF(DBG_INIT, "SQE\n\n");
			ETH_REG32(MAUTO)|=(1<<2);
		}
		if(int_status & MIS_HQE)
		{
			//DBGPRINTF(DBG_INIT, "HQE\n\n");
		}

		if(int_status & MIE_BE)
		{
			//DBGPRINTF(DBG_INIT, "BE\n\n");
			ETH_REG32(MAUTO)|=(1<<0);
		}

		if(int_status & MIS_TX)
		{
			cta_eth_txint(&pcta_eth->tx, 0);
		}

		if(int_status & MIS_RX)
		{
			cta_eth_rxint(&pcta_eth->rx, NUM_RX_DESC);
		}
	} while(0);
	/*
		in case of something pending to do,
		re-schedule next tasklet to continue
	*/
	int_status=ETH_REG32(MIS);
	if(int_status & (MIM_INIT))
	{
		/* we re-schedule a tasklet later */
#ifdef USE_TASKLET
#if defined(INTERRUPT_MITIGATION)
		ETH_REG32(MIM)&= ~MIM_INIT;
#else
		tasklet_hi_schedule(&tasklet);
#endif
#endif
	}
	else
		ETH_REG32(MIM)&= ~MIM_INIT;
}
#endif

#ifdef USE_NAPI
/*!-----------------------------------------------------------------------------
 *        cta_eth_poll:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_poll(struct napi_struct *napi, int budget)
{
	unsigned int work_done=0, work_to_do = budget;//min(budget, dev->quota);
	unsigned int int_status;

	int_status = ETH_REG32(MIS);
	/* Ack all interrupts */
	ETH_REG32(MIC) = int_status;

//	DBGPRINTF(DBG_DELIVER, "%s MIS=%x\n", __func__, int_status);
#if 0
	if(int_status & MIS_SW)
	{
		cml_eth_swint();
	}
#endif
	if(int_status & MIS_SQE)
	{
		//DBG_LOG(DBG_INIT, "SQE\n\n");
		ETH_REG32(MAUTO)|=(1<<2);
	}

	if(int_status & MIS_HQE)
	{
		//DBG_LOG(DBG_INIT, "HQE\n\n");
	}

	if(int_status & MIE_BE)
	{
		//DBG_LOG(DBG_INIT, "BE\n\n");
		ETH_REG32(MAUTO)|=(1<<0);
	}
	if(int_status & MIS_TX)
	{
		cta_eth_txint(&pcta_eth->tx, 0);
	}
	if(int_status & MIS_RX)
	{
		work_done = cta_eth_rxint(&pcta_eth->rx, work_to_do);
	}
	if(work_done < work_to_do)
	{
		napi_complete(napi);
		ETH_REG32(MIM) &= ~MIM_INIT;
	}

	return work_done;
}
#endif

/*!-----------------------------------------------------------------------------
 *        cta_eth_flowctrl:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_flowctrl(int onoff)
{
	if(pcta_eth->flowctrl_mode == 0)
		return;
	if(onoff)
	{
		/* send pause frame */
		ETH_REG32(MFC1) = 0x2;
		pcta_eth->flowctrl_state = 1;
		diag_printf("send pause!!%x\n", ETH_REG32(MFC1));
	}
	else if(pcta_eth->flowctrl_state)
	{
		/* send clear pause frame */
		ETH_REG32(MFC1) = 0x4;
		pcta_eth->flowctrl_state = 0;
		diag_printf("send clear pause!!\n");
	}
}

#if (defined(CONFIG_PANTHER_WLAN) && defined(CONFIG_BRIDGE))
extern int forward_eth_skb(struct sk_buff *skb);
#endif
/*!-----------------------------------------------------------------------------
 *        eth_drv_recv1:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int eth_drv_recv1(struct net_device *dev, unsigned char *data, int len,
					char **nbufp, void *hw_nat, unsigned int w0, unsigned int w1)
{
	struct sk_buff *skb;
	unsigned int * buf = *((unsigned int**)nbufp);
	int rc;

	skb = (struct sk_buff*) buf[BUF_SW_OFS/4];
#if 1 //DEBUG
	if(0x08 != (((unsigned int)skb)>>28))
	{
		diag_printf("%s: skb is lost: %x\n",__func__, (unsigned int)skb);
		return -1;
	}
#endif

	skb->data = skb->tail = data;
	skb_put(skb,len);

#if defined(ETH_DBG)
	if(DBG_RXMORE2 & DBG_MASK)
	{
		int j;
		int dlen = (skb->len > MAX_PKT_DUMP)? MAX_PKT_DUMP : skb->len;
		printk("<<< %s skb->data=%x , len=%x", __func__, (int)skb->data, skb->len);

		for(j=0;j<dlen;j++)
		{
				if(0==(j&0x1f))
					printk("\n%03x:", j);
				printk(" %02x",skb->data[j]&0xff);
		}
		printk("\n");
	}
#endif

#ifdef BRIDGE_FILTER
	if(cta_eth_filter(skb, data, 0))
	{
		return 0;
	}
#endif

#if defined(CONFIG_PANTHER_WLAN) && defined(CONFIG_BRIDGE)
	/* Use this for ether to WiFi */
    if(NET_RX_DROP == forward_eth_skb(skb))
    {
        return 0;
    }
#endif

	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);    /* set packet type */
#if defined(CTA_RX_CKSUM_OFFLOAD)
	skb->ip_summed = CHECKSUM_UNNECESSARY;       /* don't check it */
#endif
#if defined(CONFIG_TODO)
	cta_hnat_set_semicb((unsigned long)SKB2SEMICB(skb), hw_nat, w0, w1);
#endif

#ifdef  USE_NAPI
	rc = netif_receive_skb(skb);
#else
	rc = netif_rx(skb);
#endif

	return rc;
}

/*!-----------------------------------------------------------------------------
 *        eth_drv_getbuf:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
char *eth_drv_getbuf()
{
	struct sk_buff *skb;

#if defined (CONFIG_PANTHER_SKB_RECYCLE_2K)
	skb = skbmgr_dev_alloc_skb2k();
#else
	skb = __dev_alloc_skb(1664, GFP_ATOMIC | GFP_DMA);
#endif

	if(NULL==skb)   return NULL;

	skb->data = (unsigned char*)(((unsigned int)(skb->data+0x3f)) & ~0x3f);
	*((unsigned int*) skb->data) = (unsigned int)skb ;
	DC_FLUSH_TX(skb->data, sizeof(unsigned int));

	return skb->data;
}

/*!-----------------------------------------------------------------------------
 *        eth_drv_freebuf:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void eth_drv_freebuf(char *buf)
{
	unsigned int *wbuf;
	struct sk_buff *skb;

	wbuf = (unsigned int*)(((unsigned int)buf) & ~0x3f);
	if(0x08 ==(wbuf[BUF_SW_OFS>>2]>>28))
	{
		skb = (struct sk_buff*)wbuf[BUF_SW_OFS>>2];
		kfree_skb(skb);
	}
	else
	{
		printk("%s , skb head missing:%x , buf=%x\n",
				__func__, wbuf[BUF_SW_OFS>>2], (unsigned int)buf);
	}
}

/*!-----------------------------------------------------------------------------
 *        an_cmd:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int an_cmd(int phy_addr, unsigned short val, unsigned short cap)
{
	unsigned short phy_setting, ctl=0;

	if(cap & (PHY_CAP_AN|PHY_CAP_100F|PHY_CAP_10F|PHY_CAP_100F|PHY_CAP_100H))
	{
		phy_setting = cta_eth_mdio_rd(phy_addr, 0);			// ctrl
		if(cap & PHY_CAP_AN)
			ctl |= PHYR_CTL_AN;
		if(cap & (PHY_CAP_10F))
			ctl |= PHYR_CTL_DUPLEX;
		if(cap & (PHY_CAP_100H))
			ctl |= PHYR_CTL_SPEED;
		if(val)
			phy_setting |= ctl;
		else
			phy_setting &= ~ctl;
		cta_eth_mdio_wr(phy_addr, 0, phy_setting);
	}
	if(cap & (PHY_CAP_PAUSE|PHY_CAP_100F|PHY_CAP_10F|PHY_CAP_100F|PHY_CAP_100H))
	{
		phy_setting = cta_eth_mdio_rd(phy_addr, 4);			// mine
		if(val)
			phy_setting |= cap;
		else
			phy_setting &= ~cap;
		cta_eth_mdio_wr(phy_addr, 4, (phy_setting & (~PHY_CAP_AN))); // do not write PHY_CAP_AN to hardware!!
	}

	{
#if defined(CONFIG_EPHY_FPGA)
		unsigned int id;
		if((id=cta_switch_phyid(phy_addr))==PHYID_MONT_EPHY)
			cta_eth_phy_reset(phy_addr, id);
		else
#endif
		{
			phy_setting = cta_eth_mdio_rd(phy_addr, 0);
			cta_eth_mdio_wr(phy_addr, 0, phy_setting | 0x200);	// AN
		}
	}
	return CLI_OK;
}

/*!-----------------------------------------------------------------------------
 *        phy_status:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
short phy_status(short phy_addr)
{
	short ret = 0;
	short phy_setting,ctl,sts;

	sts = cta_eth_mdio_rd(phy_addr, 1);

	if(sts & PHYR_ST_AN)
		ret |= PHY_ANS;

	if(sts & PHYR_ST_LINK)
		ret |= PHY_LINK;
	else
	{
#if 0
		/* To debug */
		printk("%s: phy_addr(%d) sts(%x) debug(%x)\n", __func__, phy_addr, sts, cta_eth_mdio_rd(phy_addr, 1));
#endif
		goto ret_;
	}
	/* AN Complete */
	if(sts & PHYR_ST_ANC)
	{
		ret |= PHY_ANC;
		phy_setting = cta_eth_mdio_rd(phy_addr , 4);		// mine
		ctl = cta_eth_mdio_rd(phy_addr , 5);		// link parter
		if((cta_switch_phyid(phy_addr)==PHYID_MONT_EPHY) && (ctl==0))
		{
			sts = cta_eth_mdio_rd(phy_addr, 31);
			sts &= 0x7;
			/* Swap to page 0 */
			cta_eth_mdio_wr(phy_addr, 31, 0);
			ctl = cta_eth_mdio_rd(phy_addr , 19);	// result of parallel detection
			cta_eth_mdio_wr(phy_addr, 31, sts);
			if( ctl & 0x10)
				ret |= PHY_100M;
			if( ctl & 0x8)
				ret |= PHY_FDX;
		}
		else
		{
			phy_setting = 0x3e0 & (phy_setting & ctl);
			if(PHY_CAP_100F & phy_setting)
				ret |= (PHY_100M|PHY_FDX);
			else
			if(PHY_CAP_100H & phy_setting)
				ret |= (PHY_100M);
			else
			if(PHY_CAP_10F & phy_setting)
				ret |= (PHY_FDX);
		}
	}
	else // force
	{
		ctl = cta_eth_mdio_rd(phy_addr, 0);
		if(ctl & PHYR_CTL_SPEED)
			ret |= PHY_100M;
		if(ctl & PHYR_CTL_DUPLEX)
			ret |= PHY_FDX;
	}
ret_:
	return ret;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_link_status:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
short cta_eth_link_status(int phy)
{
	short status = 0, link = 0;

	status = cta_eth_mdio_rd(phy, 1);
//	DBG_MSG(DEBUG_PHY, "port(%d) status=%x\n", phy, (unsigned int) status);
	link = status & (1 << 2);
	return (link?1 : 0);
}

/*!-----------------------------------------------------------------------------
 *        mdio_cmd:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int mdio_cmd(int argc, char *argv[])
{
	int set = 0;
	int adr = 0;
	int reg;
	int val;

	if(argc < 1)
		goto help;
	sscanf(argv[0], "%d", &adr);
	if(argc > 1)
	{
		if(1 != sscanf(argv[1], "%x", &reg))
			goto help;
	}
	else
	{
		printk("phy%d regs", adr);
		for(reg = 0; reg < 32; reg++)
		{
			val = cta_eth_mdio_rd(adr, reg) & 0xffff;
			if ((reg % 8) == 0)
				printk("\n%02X: ", reg);
			printk("%04x ", val);
		}
		printk("\n");
		return 0;
	}

	if(argc > 2)
	{
		if(1 != sscanf(argv[2], "%x", &val))
			goto help;
		cta_eth_mdio_wr(adr, reg, val);
		set++;
	}
	else
	{
		val = cta_eth_mdio_rd(adr, reg) & 0xffff;
	}
	printk("%sphy%d reg 0x%02x=%04x\n", set ? "SET " : "", adr, reg, val);
	return 0;
help:
	printk("mdio [adr] [reg] [val]\n\r");
	return -1;
}

#if defined(INTERRUPT_MITIGATION)
int cta_eth_set_isr_maxcnt(unsigned int maxcnt)
{
	printk("eth irq tolerant %d per jiffies\n", maxcnt);
	intr_tolerant_count = maxcnt;
	return 0;
}

int cta_eth_isr_mintigation_ctrl(unsigned int ticks)
{
	printk("eth irq mintigation delay %d (us)\n", ticks);
	intr_minigation_ticks = ticks;
	return 0;
}
#endif

/*!-----------------------------------------------------------------------------
 *        eth_cal_txrx:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int eth_cal_txrx(int port, unsigned int txclk, unsigned int rxclk)
{
	cta_switch_txrx_ctrl(port, txclk, rxclk);
	cta_switch_port_attach(port, 0, 1);
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_8021q_ctrl:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int cta_eth_8021q_ctrl(unsigned int val)
{
	int vid, idx_8021q, sll, ofs, msk;

	/* Set vlan table of hnat */
	vid = (val & MASK_8021Q_VID) >> 16;
	idx_8021q = (val & MASK_BSSID_IDX) >> 8;
	if(val & FLG_8021Q_OUTB)
		vid |= (1<<12);

	//ETH_DHWV(idx_8021q) = (unsigned short)vid;					/* store in hw vlan table */
	sll = (idx_8021q%2)?16:0; 									/* shift or not */
	msk = (idx_8021q%2)?0xffff0000:0xffff; 						/* mask MSB or LSB */
	ofs = (idx_8021q/2)<<2;										/* offset */

	MREG((MVL01 + ofs)) &= ~msk;
	MREG((MVL01 + ofs)) |= (vid<<sll);
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
unsigned int cta_eth_phy_init(unsigned char p)
{
	unsigned int id;

	id = (cta_eth_mdio_rd(p, 2) << 16) | cta_eth_mdio_rd(p, 3);
	if(id==PHYID_ICPLUS_IP101G)
	{
		/* Config IP101G to version 1.2 */
		cta_eth_mdio_wr(p, 20, 16);
		cta_eth_mdio_wr(p, 16, 0x1006);
#if defined(CONFIG_PANTHER_FPGA)
		/* Config IP101G RXC driving current = 12.96mA */
		cta_eth_mdio_wr(p, 20, 4);
		cta_eth_mdio_wr(p, 22, 0xa000);
		/* Config IP101G TXC driving current = 12.96mA */
		cta_eth_mdio_wr(p, 20, 16);
		cta_eth_mdio_wr(p, 27, 0x0015);
		/* Config IP101G RXD driving current = 12.96mA */
		cta_eth_mdio_wr(p, 20, 16);
		cta_eth_mdio_wr(p, 26, 0x5b6d);
#endif
		/* Restore control and capability */
		cta_eth_mdio_wr(p, 4, cta_switch_phybcap(p));
		cta_eth_mdio_wr(p, 0, cta_switch_phyctrl(p)|0x8000);
	}
	else if(id==PHYID_MONT_EPHY)
	{
#if defined(CONFIG_PANTHER_FPGA)
#if defined(CONFIG_EPHY_FPGA)
		/* Swap to page 2 */
		cta_eth_mdio_wr(p, 31, 2);
		/* DSP initial val */
		cta_eth_mdio_wr(p, 18, 0x8975);
		cta_eth_mdio_wr(p, 19, 0xba60);
#else
		/* Swap to page 1 */
		cta_eth_mdio_wr(p, 31, 1);
		cta_eth_mdio_wr(p, 16, 0xb5a0);
		/* AFE tx control: enable termination impedance calibration */
		cta_eth_mdio_wr(p, 17, 0xa528);
#if defined(CONFIG_EPHY_GLITCH_PATCH)
		cta_eth_mdio_wr(p, 18, (cta_eth_mdio_rd(p, 18)&0x07ff));
		cta_eth_mdio_wr(p, 18, (cta_eth_mdio_rd(p, 18)|0x1800));
#endif
		cta_eth_mdio_wr(p, 19, 0xa4d8);
		/* AFE rx control */
		cta_eth_mdio_wr(p, 20, 0x3780);
		/* ADC VCM = 1.00 */
		cta_eth_mdio_wr(p, 22, (cta_eth_mdio_rd(p, 22)|0x6000));
#if defined(CONFIG_EPHY_GLITCH_PATCH)
		cta_eth_mdio_wr(p, 23, (cta_eth_mdio_rd(p, 23)&0xe01f));
		cta_eth_mdio_wr(p, 23, (cta_eth_mdio_rd(p, 23)|0x1500));
#endif
		/* Swap to page 2 */
		cta_eth_mdio_wr(p, 31, 2);
 		/* AGC thresholds, org=0x4030 */
 		cta_eth_mdio_wr(p, 17, 0x8059);
		/* DSP initial val */
		cta_eth_mdio_wr(p, 18, 0x8975);
		cta_eth_mdio_wr(p, 19, 0xba60);
 		/* Swap to page 4 */
 		cta_eth_mdio_wr(p, 31, 4);
 		/* 10T signal detection time control, org=0x5aa0 */
 		cta_eth_mdio_wr(p, 18, 0x5a40);
#endif
		/* Swap to page 0 */
		cta_eth_mdio_wr(p, 31, 0);
		/* RMII V1.2 */
		cta_eth_mdio_wr(p, 19, (cta_eth_mdio_rd(p, 19)|0x0040));
		/* Enable MDIX */
		cta_eth_mdio_wr(p, 20, (cta_eth_mdio_rd(p, 20)|0x3000));
#else

		//Digital Settings
		//Page 0
		cta_eth_mdio_wr(p, 0x1f, 0x0);
		//Enable Polarity reverse, Auto-MDIX enable
		cta_eth_mdio_wr(p, 0x14, 0x6000);

		//Page 2
		cta_eth_mdio_wr(p, 0x1f, 0x2);
		//agc pulse upper/lower threshold;
		cta_eth_mdio_wr(p, 0x11, 0x8059);
		//100BTX receiver control register 3, TRL loop filter gain
		cta_eth_mdio_wr(p, 0x12, 0x8975);
		//100BTX receiver control register 4, equlizer step 0,
		cta_eth_mdio_wr(p, 0x13, 0x6a60);

		//Page 4
		cta_eth_mdio_wr(p, 0x1f, 0x4);
		//singal detection time control
		cta_eth_mdio_wr(p, 0x12, 0x5a40);

		//Analog Settings
		//Page 1
		cta_eth_mdio_wr(p, 0x1f, 0x1);
		//TX Swing and Low Power Mode,lpmode_100, txamp_100
		cta_eth_mdio_wr(p, 0x10, 0xb5a0);
		//TX Common Mode, txvcm, rterm_ext, using calibration results
		cta_eth_mdio_wr(p, 0x11, 0xa528);
		//Set PGA_test_en[1:0] to 1 for correct working mode
		cta_eth_mdio_wr(p, 0x1b, 0x00c0);
		//AFE Clock gen control register 2
		cta_eth_mdio_wr(p, 0x18, 0xf400);
		//RX controll regster 2, sd_level, unsd_level
		cta_eth_mdio_wr(p, 0x13, 0xa4d8);
		//RX controll regster 3, lpf_ctune_cal, rx_lpf
		cta_eth_mdio_wr(p, 0x14, 0x3780);
		//RX controll regster 4, lpf_ctune_ext_en, using external value
		cta_eth_mdio_wr(p, 0x15, 0xb600);
		//RX controll regster 5, RX Common mode
		cta_eth_mdio_wr(p, 0x16, 0xf900);

		//Page 0
		cta_eth_mdio_wr(p, 0x1f, 0x0);
		// set bit[15]=1 to reset, bit[13]=1 set 100Mb/s, bit[12]=1 enable auto-negotiation process.
		cta_eth_mdio_wr(p, 0x00, 0xb100);
#endif
	}
	else
	{
		printk("%s(%d): Unknow phy id:%x p:%d\n", __func__, __LINE__, id, p);
	}
	return id;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_phy_loopback:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
void cta_eth_phy_loopback(unsigned int p, unsigned int action)
{
	unsigned short val;

	val = cta_eth_mdio_rd(p, 0);
	if(action)
	{
		/* Enable loopback, disable AN, force 100M full mode */
		val |= 0x6100;
		val &= ~0x1000;
		cta_eth_mdio_wr(p, 0, val);
	}
	else
	{
		/* Disable loopback, restore capability and restart AN */
		val &= ~0x6100;
		val |= (0x200 | cta_switch_phyctrl(p));
		cta_eth_mdio_wr(p, 0, val);
	}
}

/*!-----------------------------------------------------------------------------
 *        eth_cmd:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
int eth_cmd(int argc, char *argv[])
{
	char *val;

	if(argc < 1)
	{
		goto done;
	}

	if(!strcmp(argv[0], "mdio"))
	{
		mdio_cmd(argc-1, &argv[1]);
		goto done;
	}
	if(!strcmp(argv[0], "loopback"))
	{
		unsigned int p;
		if (argc < 3)
		{
			goto done;
		}
		sscanf(argv[1], "%d", &p);

		if(!strcmp(argv[2], "start"))
			cta_eth_phy_loopback(p,1);
		else
		if(!strcmp(argv[2], "stop"))
		{
			cta_eth_phy_loopback(p,0);
			cta_switch_port_detach(p);
		}

		goto done;
	}
	if(!strcmp(argv[0], "calibrate"))
	{
		unsigned int port;
		unsigned int txclk;
		unsigned int rxclk;
		if (argc < 4)
		{
			goto done;
		}
		sscanf(argv[1], "%d", &port);
		sscanf(argv[2], "%d", &txclk);
		sscanf(argv[3], "%d", &rxclk);
		eth_cal_txrx(port,txclk,rxclk);
		goto done;
	}
	if(!strcmp(argv[0], "sclk"))
	{
		cta_switch_sclk_ctrl();
		goto done;
	}
	if(!strcmp(argv[0], "an"))
	{
		cta_an();
		goto done;
	}
	if(!strcmp(argv[0], "8021q"))
	{
		unsigned int data;
		sscanf(argv[1], "%x", &data);
		cta_eth_8021q_ctrl(data);
		cta_switch_8021q_ctrl(data);
		goto done;
	}
	if(!strcmp(argv[0], "bssid"))
	{
		unsigned int data;
		sscanf(argv[1], "%x", &data);
		cta_switch_bssid_ctrl(data);
		goto done;
	}
	if(!strcmp(argv[0], "epvid"))
	{
		unsigned int data;
		sscanf(argv[1], "%x", &data);
		cta_switch_pvid_ctrl(data);
		goto done;
	}
	if(!strcmp(argv[0], "wmode"))
	{
		unsigned int data;
		sscanf(argv[1], "%d", &data);
		cta_switch_work_mode(data);
		goto done;
	}
	if(!strcmp(argv[0], "swctl"))
	{
		unsigned int data;
		sscanf(argv[1], "%d", &data);
		cta_switch_state_ctrl(data);
		goto done;
	}
#if defined(DETECT_A1_AND_APPLY_TX_RATE_LIMIT)
	if(!strcmp(argv[0], "ifg"))
	{
		unsigned int data;
		sscanf(argv[1], "%d", &data);
		ifg = data;
		printk("ifg = %lld\n", ifg);
		goto done;
	}
#endif
#if defined(INTERRUPT_MITIGATION)
	if(!strcmp(argv[0], "irqmaxcnt"))
	{
		unsigned int data;
		sscanf(argv[1], "%d", &data);
		cta_eth_set_isr_maxcnt(data);
		goto done;
	}
	if(!strcmp(argv[0], "irqtimer"))
	{
		unsigned int data;
		sscanf(argv[1], "%d", &data);
		cta_eth_isr_mintigation_ctrl(data);
		goto done;
	}
#endif
	if(0==(val=strchr(argv[0],'=')))
	{
		printk("%s: invalid command '%s'\n", __func__, argv[0]);
		goto done;
	}
	*val++=0;
done:
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        debugfs_br_write_state:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int debugfs_br_write_state(void *priv, u64 val)
{
	/* write hw bridge old status */
	cta_switch_brg_write_state((u8)val);
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        debugfs_br_read_state:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int debugfs_br_read_state(void *priv, u64 *val)
{
	/* read hw bridge old status */
	*val = cta_switch_brg_read_state();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_br_state, debugfs_br_read_state, debugfs_br_write_state, "%llu\n");

/*!-----------------------------------------------------------------------------
 *        debugfs_br_get:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int debugfs_br_get(void *priv, u64 *val)
{
	/* hw bridge status */
	*val = cta_switch_brg_state();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_br, debugfs_br_get, NULL, "%llu\n");

/*!-----------------------------------------------------------------------------
 *        debugfs_pp_get:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int debugfs_pp_get(void *priv, u64 *val)
{
	/* polling phy status */
	*val = cta_switch_pp_state();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_pp, debugfs_pp_get, NULL, "%llu\n");

#ifdef CONFIG_PROC_FS
/*!-----------------------------------------------------------------------------
 *        proc_eth_write:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int proc_eth_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char *buf;
	int argc;
	char *argv[8];

	buf = kmalloc(300, GFP_KERNEL);
	if(NULL==buf)
		return -ENOMEM;

	if(count > 0 && count < 299)
	{
		if(copy_from_user(buf, buffer, count))
			return -EFAULT;
		if(buf[count-1]=='\n')
			buf[count-1]='\0';
		buf[count]='\0';
		argc = get_args( (const char *)buf , argv );
		eth_cmd(argc, argv); //rc=eth_cmd(argc, argv);
	}

	kfree(buf);

	return count;
}
#endif

static int cta_eth_port_map(struct net_device *dev)
{
#define CTA_ETH_DEFAULT_PORT 1   /* default port is P1 */
    int port = CTA_ETH_DEFAULT_PORT;
    struct vlan_dev_priv *vlan;
    if_cta_eth_t *pifcta_eth;

    pifcta_eth = (if_cta_eth_t *) netdev_priv(dev);

    if((dev->priv_flags & IFF_802_1Q_VLAN) == 0)
    {
        /* default port is P1 */
    }
    else
    {
        vlan = vlan_dev_priv(dev);

        if(vlan->vlan_id == swcfg_p1_vlan_id())
        {
            port = 1;
        }
        else if(vlan->vlan_id == swcfg_p0_vlan_id())
        {
            port = 0;
        }
        else
        {
            //printk("cta_eth_port_map: unknown vlan device VID:%d\n", vlan->vlan_id);
            port = -1;
        }
    }

    return port;
}

static int cta_eth_phy_map(struct net_device *dev)
{
    int port;

    port = cta_eth_port_map(dev);
    if(0>port)
        return -1;

    return switch_port_to_phy_addr(port);
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_get_an:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static u32 cta_eth_get_an(int phy, u16 addr)
{
	u32 result = 0;
	int advert;

	advert = cta_eth_mdio_rd(phy, addr);
	if(advert & LPA_LPACK)
		result |= ADVERTISED_Autoneg;
	if(advert & ADVERTISE_10HALF)
		result |= ADVERTISED_10baseT_Half;
	if(advert & ADVERTISE_10FULL)
		result |= ADVERTISED_10baseT_Full;
	if(advert & ADVERTISE_100HALF)
		result |= ADVERTISED_100baseT_Half;
	if(advert & ADVERTISE_100FULL)
		result |= ADVERTISED_100baseT_Full;
	if(advert & ADVERTISE_PAUSE_CAP)
		result |= ADVERTISED_Pause;

	return result;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_get_settings:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	int phy;
	u16 bmcr, bmsr;
	u32 nego;

	phy = cta_eth_phy_map(dev);
	if(0>phy)
        return -EINVAL;

	cmd->supported =
		(SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
		SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
		SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII |
		SUPPORTED_Pause);

	/* only supports twisted-pair */
	cmd->port = PORT_MII;

	/* only supports internal transceiver */
	cmd->transceiver = XCVR_INTERNAL;

	/* this isn't fully supported at higher layers */
	cmd->phy_address = phy;
//	cmd->mdio_support = MDIO_SUPPORTS_C22;

	cmd->advertising = ADVERTISED_TP | ADVERTISED_MII;

	bmcr = cta_eth_mdio_rd(phy, MII_BMCR);
	bmsr = cta_eth_mdio_rd(phy, MII_BMSR);
	if(bmcr & BMCR_ANENABLE)
	{
		cmd->advertising |= ADVERTISED_Autoneg;
		cmd->autoneg = AUTONEG_ENABLE;

		cmd->advertising |= cta_eth_get_an(phy, MII_ADVERTISE);

		if(bmsr & BMSR_ANEGCOMPLETE) {
			cmd->lp_advertising = cta_eth_get_an(phy, MII_LPA);
		} else {
			cmd->lp_advertising = 0;
		}

		nego = cmd->advertising & cmd->lp_advertising;

		if(nego & (ADVERTISED_100baseT_Full |
			ADVERTISED_100baseT_Half))
		{
			cmd->speed = SPEED_100;
			cmd->duplex = !!(nego & ADVERTISED_100baseT_Full);
		} else {
			cmd->speed = SPEED_10;
			cmd->duplex = !!(nego & ADVERTISED_10baseT_Full);
		}
	}
	else
	{
		cmd->autoneg = AUTONEG_DISABLE;
		cmd->speed = ((bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10);
		cmd->duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
	}

	/* ignore maxtxpkt, maxrxpkt for now */

	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_set_settings:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	int phy;

	if(cmd->speed != SPEED_10 && cmd->speed != SPEED_100)
		return -EINVAL;
	if(cmd->duplex != DUPLEX_HALF && cmd->duplex != DUPLEX_FULL)
		return -EINVAL;
	if(cmd->port != PORT_MII)
		return -EINVAL;
	if(cmd->phy_address >= 2)
		return -EINVAL;
	if(cmd->autoneg != AUTONEG_DISABLE && cmd->autoneg != AUTONEG_ENABLE)
		return -EINVAL;

	phy = cta_eth_phy_map(dev);
	if(0>phy)
		return -EINVAL;

	/* ignore supported, maxtxpkt, maxrxpkt */
	if(cmd->autoneg == AUTONEG_ENABLE)
	{
		u32 bmcr, advert, tmp;

		if ((cmd->advertising & (ADVERTISED_10baseT_Half |
					  ADVERTISED_10baseT_Full |
					  ADVERTISED_100baseT_Half |
					  ADVERTISED_100baseT_Full)) == 0)
			return -EINVAL;

		/* advertise only what has been requested */
		advert = cta_eth_mdio_rd(phy, MII_ADVERTISE);
		tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP);
		if (cmd->advertising & ADVERTISED_10baseT_Half)
			tmp |= ADVERTISE_10HALF;
		if (cmd->advertising & ADVERTISED_10baseT_Full)
			tmp |= ADVERTISE_10FULL;
		if (cmd->advertising & ADVERTISED_100baseT_Half)
			tmp |= ADVERTISE_100HALF;
		if (cmd->advertising & ADVERTISED_100baseT_Full)
			tmp |= ADVERTISE_100FULL;
		if (cmd->advertising & ADVERTISED_Pause)
			tmp |= ADVERTISE_PAUSE_CAP;
		if (advert != tmp) {
			cta_eth_mdio_wr(phy, MII_ADVERTISE, tmp);
		}

		/* turn on autonegotiation, and force a renegotiate */
		bmcr = cta_eth_mdio_rd(phy, MII_BMCR);
		bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
		cta_eth_mdio_wr(phy, MII_BMCR, bmcr);
	}
	else
	{
		u32 bmcr, tmp;

		/* turn off auto negotiation, set speed and duplexity */
		bmcr = cta_eth_mdio_rd(phy, MII_BMCR);
		tmp = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 | BMCR_FULLDPLX);
		if(cmd->speed == SPEED_100)
			tmp |= BMCR_SPEED100;
		if(cmd->duplex == DUPLEX_FULL)
			tmp |= BMCR_FULLDPLX;
		if(bmcr != tmp)
			cta_eth_mdio_wr(phy, MII_BMCR, tmp);
	}
	return 0;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_get_link:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static u32 cta_eth_get_link(struct net_device *dev)
{
	int phy;
	int port;
	unsigned short cap, link;
	unsigned int id;

	/* Stop polling phy */
	if(!cta_switch_pp_state())
		return 1;

	phy = cta_eth_phy_map(dev);
	if(0>phy)
		return -EINVAL;

	port = cta_eth_port_map(dev);

	/* first, a dummy read, needed to latch some MII phys */
	cta_eth_mdio_rd(phy,MII_BMSR);
	cap = phy_status(phy);
	link = cap & PHY_LINK;
	id = cta_switch_phyid(phy);
	if((port_link_status[port] == 0) && link)
	{
		cta_switch_port_ctrl(port, 1);
		cta_switch_led_action(port, link);
		if(id==PHYID_MONT_EPHY)
		{
			if ((cap & (PHY_LINK|PHY_100M)) == (PHY_LINK|PHY_100M))
			{
				if (cta_eth_phy_monitor(phy))
					cta_eth_phy_reset(phy, id);
			}
			cta_eth_phy_isolate(phy, 0);
		}
	}
	else if((port_link_status[port] & PHY_LINK) && (link==0))
	{
		cta_switch_port_ctrl(port, 0);
		if(id==PHYID_MONT_EPHY)
		{
			/* Isolate EPHY is a workaround of AMD AM79C97 NIC */
			cta_eth_phy_isolate(phy, 1);
			cta_eth_phy_reset(phy, id);
		}
	}
	if(id==PHYID_MONT_EPHY)
	{
		if((cap & (PHY_ANC|PHY_LINK))==0)
		{
			/*
				Once EPHY enable MDIX, the time of link-up will
				increase if link partner disable AN
			*/
			if(pcta_eth->ephy.anc_fail==20)
			{
				/*
					EPHY may link fail once link partner's AN is disabled,
					so we reset EPHY while link is down
				*/
				DBGPRINTF(DBG_PHY, "%s: EPHY error: AN-complete =link status =0, reset EPHY(%d)\n",
									__func__, cta_switch_inc_phy_anerr());
				cta_eth_phy_isolate(phy, 1);
				cta_eth_phy_reset(phy, id);
			}
			else
				pcta_eth->ephy.anc_fail++;
		}
#if 0
		/*
		 This action makes descambler seed is not ok,
		 Fred recommends disable it. (2015/11/23)
		 */
		if(cta_switch_crc_monitor(phy))
			cta_eth_phy_synchronization(phy);
#endif
#if defined(CONFIG_EPHY_FPGA)
		{
			/* Debug mdio */
			unsigned short val=0,tmp;
			if(pcta_eth->ephy.mdio_fail==0xffff)
				pcta_eth->ephy.mdio_fail=0;

			tmp = cta_eth_mdio_rd(phy, 31);
			tmp &= 0x7;
			/* Swap to page 1 */
			cta_eth_mdio_wr(phy, 31, 1);
			/* register read/write test */
			cta_eth_mdio_wr(phy, 23, pcta_eth->ephy.mdio_fail);
			val = cta_eth_mdio_rd(phy, 23);
			/* Swap to orig page */
			cta_eth_mdio_wr(phy, 31, tmp);

			if(pcta_eth->ephy.mdio_fail!= val)
			{
				DBGPRINTF(DBG_PHY, "%s: EPHY error: mdio  reg(%d), wr(%04x) rd(%04x)\n",
							__func__, 23, pcta_eth->ephy.mdio_fail, val);
			}
			pcta_eth->ephy.mdio_fail++;
		}
#endif
	}
	port_link_status[port] = link;
	return link;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_nway_reset:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int cta_eth_nway_reset(struct net_device *dev)
{
	int id, ret = -EINVAL;
	short tmp;

	id = cta_eth_phy_map(dev);
	if(0>id)
		return -EINVAL;

	tmp = cta_eth_mdio_rd(id,MII_BMCR);
	if(tmp & BMCR_ANENABLE)
	{
		cta_eth_mdio_wr(id,MII_BMCR,tmp|BMCR_ANRESTART);
		ret = 0;
	}
	return ret;
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_sw_monitor:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_sw_monitor(unsigned long id)
{
	u8 rc;

	rc = cta_switch_monitor();
	if(rc == 2)
	{
		printk(KERN_EMERG "Resetting internel Ethernet PHY\n");
		cta_eth_phy_reset(swcfg_p1_phy_addr(), PHYID_MONT_EPHY);
	}
	else if(rc)
	{
		cta_switch_ethernet_forward_to_wifi(0);
		cta_switch_ethernet_forward_to_wifi(1);
	}

	mod_timer(&pcta_eth->timer, jiffies + (HZ/10));
}

/*!-----------------------------------------------------------------------------
 *        cta_eth_get_drvinfo:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void cta_eth_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "Panther Ethernet");
	strcpy(info->version, "1.0");
	strcpy(info->fw_version, "1.0");
	strcpy(info->bus_info, "RMII");
}

static int eth_vlan_register_dev(struct net_device *dev, struct net_device *vdev, u16 vid)
{
    vdev->priv_flags |= IFF_ETH_DEV;
    vdev->ethtool_ops = &cta_ethtool_ops;

    return 0;
}

static struct net_device_ops eth_netdev_ops = {
	.ndo_open               = eth_start,
	.ndo_stop               = eth_stop,
	.ndo_start_xmit         = eth_send,
	.ndo_set_rx_mode        = eth_rx_mode,
	.ndo_do_ioctl           = eth_ioctl,
	.ndo_set_mac_address    = eth_set_mac_address,
	.ndo_vlan_rx_add_vid    = eth_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid   = eth_vlan_rx_kill_vid,
	.ndo_vlan_register_dev  = eth_vlan_register_dev,
	.ndo_vlan_dev_set_mac_address = eth_vlan_set_mac_address,
	.ndo_get_stats          = eth_get_stats,
	.ndo_tx_timeout         = eth_tx_timeout,
};

const struct ethtool_ops cta_ethtool_ops = {
	.get_drvinfo		= cta_eth_get_drvinfo,
	.get_settings		= cta_eth_get_settings,
	.set_settings		= cta_eth_set_settings,
	.get_link			= cta_eth_get_link,
	.nway_reset			= cta_eth_nway_reset,
};

/*!-----------------------------------------------------------------------------
 *        mon_eth_drv_init:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int mon_eth_drv_init(int unit)
{
	if_cta_eth_t *pifcta_eth;
	int rc;
	struct net_device *dev;

	/* dev and priv zeroed in alloc_etherdev */
	dev = alloc_etherdev(sizeof (if_cta_eth_t));
	if(dev == NULL)
	{
		printk("%s Unable to alloc new net device\n",__func__);
		return -ENOMEM;
	}
	sprintf(dev->name, "eth%d", unit);
	pifcta_eth = (if_cta_eth_t *) netdev_priv(dev);
	dev->base_addr = MAC_BASE;
	dev->irq = IRQ_HWNAT;
	dev->ethtool_ops = &cta_ethtool_ops; //@ethtool
	pifcta_eth->unit=unit;

	DBGPRINTF(DBG_INFO, "Montage ethernet drv init: dev=%x, unit=%d\n",
				(unsigned int) dev, unit);

	/* need to notice !!! */
	if((pcta_eth == 0)  && cta_eth_init() != 0)
	{
		DBGPRINTF(DBG_ERR, "eth_drv_init: failed!\n");
		return -1;
	}
	pifcta_eth->sc = dev;
	dev->netdev_ops = &eth_netdev_ops;
	rc = register_netdev (dev);
	if(rc)
		goto err_out1;

	/* need to notice !!! */
	if(cta_eth_add_if(unit, pifcta_eth) != 0)
		goto err_out2;

	random_ether_addr(pifcta_eth->macaddr);

	pifcta_eth->macaddr[5] += (char) unit ;
#if 0
	cta_eth_setmac(unit, pifcta_eth->macaddr);						/* Need to modify */
#endif
	memcpy( dev->dev_addr, pifcta_eth->macaddr, dev->addr_len );	/* Set MAC address */

    dev->features |= NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_CTAG_TX;
	dev->priv_flags |= IFF_ETH_DEV;
	g_eth_dev = dev;

#ifdef USE_NAPI
	netif_napi_add(dev, &(cta_eth.cta_napi), cta_eth_poll, CTA_ETH_NAPI_WEIGHT);
#endif

#if defined(INTERRUPT_MITIGATION)
	//init_timer(&intr_timer);
    hrtimer_init(&intr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	intr_timer.function = cta_eth_tasklet_schedule;
#endif

	init_timer(&pcta_eth->timer);
	pcta_eth->timer.function = cta_eth_sw_monitor;
	pcta_eth->timer.expires = jiffies + 10;
	add_timer(&pcta_eth->timer);

#if defined(DETECT_A1_AND_APPLY_TX_RATE_LIMIT)
	if(chip_revision<2)
	{
		hrtimer_init(&timer_tx, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		timer_tx.function = &handle_tx;
		hrtimer_start(&timer_tx, ktime_set(0, 0), HRTIMER_MODE_REL);
	}
#endif

	return 0;
err_out2:
	cta_eth_del_if(unit);
err_out1:
	DBGPRINTF(DBG_ERR, "eth_drv_init: failed 2!\n");
	return -1;
}

static const struct file_operations eth_fops = {
	.write		= proc_eth_write,
};

/*!-----------------------------------------------------------------------------
 *        eth_init_module:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static int __init eth_init_module(void)
{
	static int probed = 0;
	int rc;
	int i;

    // check if ethernet module existed
    ETH_REG32(MVL01) = 0x12341234;
    if (ETH_REG32(MVL01) != 0x12341234) {
        ETH_REG32(MVL01) = 0;
        printk("[%s:%d] can not find this module\n", __func__, __LINE__);
        return -ENODEV;
    }

    ETH_REG32(MVL01) = 0;

#ifdef CONFIG_PROC_FS
	if(!proc_create("eth", S_IWUSR | S_IRUGO, NULL, &eth_fops))
		return -ENOMEM;
#endif

#ifdef CONFIG_CHEETAH_ETH_DEBUGFS
	if(!eth_debug_dir)
	{
		eth_debug_dir = debugfs_create_dir("eth", NULL);

		debugfs_create_file("br_state", S_IRWXUGO, eth_debug_dir, NULL, &fops_br_state);
		debugfs_create_file("br", S_IRWXUGO, eth_debug_dir, NULL, &fops_br);
		debugfs_create_file("pp", S_IRWXUGO, eth_debug_dir, NULL, &fops_pp);
	}
#endif

#if defined(CONFIG_TODO)
	/* release hnat(switch port2) and switch module clk */
	GPREG(SWRST) &= ~(PAUSE_SW | PAUSE_HNAT);
	/* reset hnat and switch port2 */
	GPREG(SWRST) &= ~SWRST_HNAT;
	for (i=0; i < 200; i++);
	GPREG(SWRST) |= SWRST_HNAT;
#endif

	if(probed)
		return -ENODEV;
	probed = 1;

	printk(KERN_INFO "Ethernet driver version %s\n", eth_ethdrv_version);

	for(i=0;i<ETH_DEV_INIT; i++)
	{
		rc = mon_eth_drv_init(i);
		if(rc)
			break;
	}

	return 0;
}

/*!-----------------------------------------------------------------------------
 *        eth_cleanup_module:
 *
 *      \brief
 *      \param
 *      \return
 +----------------------------------------------------------------------------*/
static void __exit eth_cleanup_module(void)
{
	int i;
	struct net_device * dev;

	printk(KERN_INFO "Ethernet driver unloaded\n");
	del_timer(&pcta_eth->timer);
#ifdef USE_TASKLET

#if defined(INTERRUPT_MITIGATION)
	hrtimer_cancel(&intr_timer);
#endif
	tasklet_kill(&tasklet);
#endif
	g_eth_dev = NULL;
	for(i=0;i< ETH_DEV_INIT; i++)
	{
		if(if_cta_eth_g[i]==0)
			continue;
		if(0==(dev=if_cta_eth_g[i]->sc))
			continue;
		if(pcta_eth->sc_main==dev)
			free_irq(dev->irq, dev);

		/* the priv data is part of allocated  dev */

		unregister_netdev(dev);
		kfree(dev);
		kfree(if_cta_eth_g[i]);
		if_cta_eth_g[i]=0;
	}
#if defined(CONFIG_TODO)
	/* reset hnat and switch port2 */
	GPREG(SWRST) &= ~SWRST_HNAT;
	for (i=0; i < 200; i++);
	GPREG(SWRST) |= SWRST_HNAT;
	/* stop hnat(switch port2) and switch module clk */
	GPREG(SWRST) &= ~(PAUSE_SW | PAUSE_HNAT);
#endif
#ifdef CONFIG_PROC_FS
	if(eth_debug_dir)
	{
		remove_proc_entry("eth", NULL);
		debugfs_remove_recursive(eth_debug_dir);
		eth_debug_dir = NULL;
	}
#endif
}
module_init(eth_init_module);
module_exit(eth_cleanup_module);
