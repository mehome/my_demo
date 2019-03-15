/*
 * Copyright (C) 2012-2012 Montage, Inc. All rights reserved.
 *
 * Description:
 * Montage high-speed USB SOC DR module device controller driver.
 * This can be found on warriors cpus.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#undef VERBOSE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/dmapool.h>
#include <linux/delay.h>
#include <linux/of_device.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/dma.h>

#define USB_BURSTSIZE_56_BYTES
#define EP0_BUF_32BYTES_CACHELINE_ALIGN
#define UNCACHED_ADDR(x)   ( ((u32) (x) & 0x1fffffffUL) | 0xA0000000UL )
/*
usb: prime status stage once data stage has primed
- For Control Read transfer, the ACK handshake on an IN transaction
may be corrupted, so the device may not receive the ACK for data
stage, the complete irq will not occur at this situation.
Therefore, we need to move prime status stage from complete irq
routine to the place where the data stage has just primed, or the
host will never get ACK for status stage.
The above issue has been described at USB2.0 spec chapter 8.5.3.3.

- After adding prime status stage just after prime the data stage,
there is a potential problem when the status dTD is added before the data stage
has primed by hardware. The reason is the device's dTD descriptor has NO direction bit,
if data stage (IN) prime hasn't finished, the status stage(OUT)
dTD will be added at data stage dTD's Next dTD Pointer, so when the data stage
transfer has finished, the status dTD will be primed as IN by hardware,
then the host will never receive ACK from the device side for status stage.
*/
#define PRIME_STATUS_PATCH

#include "mt_usb2_udc.h"

#define	DRIVER_DESC	"Montage High-Speed USB SOC Device Controller driver"
#define	DRIVER_AUTHOR	"Montage"
#define	DRIVER_VERSION	"Dec 18, 2012"

#define	DMA_ADDR_INVALID	(~(dma_addr_t)0)

static const char driver_name[] = "mt-udc";
static const char driver_desc[] = DRIVER_DESC;

static struct usb_dr_device __iomem *dr_regs;

//static struct usb_sys_interface __iomem *usb_sys_regs;

/* it is initialized in probe()  */
#ifdef CONFIG_PANTHER_USB_DUAL_ROLE
struct mt_udc *udc_controller = NULL;
#else
static struct mt_udc *udc_controller = NULL;
#endif

static const struct usb_endpoint_descriptor
mt_ep0_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	0,
	.bmAttributes =		USB_ENDPOINT_XFER_CONTROL,
	.wMaxPacketSize =	USB_MAX_CTRL_PAYLOAD,
};

static void mt_ep_fifo_flush(struct usb_ep *_ep);

#define mt_readl(addr)		readl(addr)
#define mt_writel(val32, addr) writel(val32, addr)
//#define mt_writel(val32, addr)  {printk("addr=0x%08x val=0x%08x\n", addr, val32);writel(val32, addr);}
#define cpu_to_hc32(x)		cpu_to_le32(x)
#define hc32_to_cpu(x)		le32_to_cpu(x)

/********************************************************************
 *	Internal Used Function
********************************************************************/
/*-----------------------------------------------------------------
 * done() - retire a request; caller blocked irqs
 * @status : request status to be set, only works when
 *	request is still in progress.
 *--------------------------------------------------------------*/
static void done(struct mt_ep *ep, struct mt_req *req, int status)
__releases(ep->udc->lock)
__acquires(ep->udc->lock)
{
	struct mt_udc *udc = NULL;
	unsigned char stopped = ep->stopped;
	struct ep_td_struct *curr_td, *next_td;
	int j;

	udc = (struct mt_udc *)ep->udc;
	/* Removed the req from mt_ep->queue */
	list_del_init(&req->queue);

	/* req.status should be set as -EINPROGRESS in ep_queue() */
	if (req->req.status == -EINPROGRESS)
		req->req.status = status;
	else
		status = req->req.status;

	/* Free dtd for the request */
	next_td = req->head;
	for (j = 0; j < req->dtd_count; j++) {
		curr_td = next_td;
		if (j != req->dtd_count - 1) {
			next_td = curr_td->next_td_virt;
		}
		dma_pool_free(udc->td_pool, curr_td, curr_td->td_dma);
	}

	usb_gadget_unmap_request(&ep->udc->gadget, &req->req, ep_is_in(ep));

	if (status && (status != -ESHUTDOWN))
		VDBG("complete %s req %p stat %d len %u/%u",
			ep->ep.name, &req->req, status,
			req->req.actual, req->req.length);

	ep->stopped = 1;

	spin_unlock(&ep->udc->lock);

#if !defined(USB_BURSTSIZE_56_BYTES)
	if(req->req.align_buf && (0==req->req.is_in))
	{
		//printk(" -> %d %d\n", req->req.length, req->req.actual);
		if(req->req.length)
			memcpy(req->req.buf, req->req.align_buf, req->req.length);
		kfree(req->req.align_buf);
		req->req.align_buf = 0;
	}
#endif

	usb_gadget_giveback_request(&ep->ep, &req->req);

	spin_lock(&ep->udc->lock);
	ep->stopped = stopped;
}

/*-----------------------------------------------------------------
 * nuke(): delete all requests related to this ep
 * called with spinlock held
 *--------------------------------------------------------------*/
static void nuke(struct mt_ep *ep, int status)
{
	ep->stopped = 1;

	/* Flush fifo */
	mt_ep_fifo_flush(&ep->ep);

	/* Whether this eq has request linked */
	while (!list_empty(&ep->queue)) {
		struct mt_req *req = NULL;

		req = list_entry(ep->queue.next, struct mt_req, queue);
		done(ep, req, status);
	}
}

/*------------------------------------------------------------------
	Internal Hardware related function
 ------------------------------------------------------------------*/

static int dr_controller_setup(struct mt_udc *udc)
{
	unsigned int tmp = 0, portctrl = 0;
	unsigned long timeout;
#define MT_UDC_RESET_TIMEOUT 1000

	/* Stop and reset the usb controller */
	tmp = mt_readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	mt_writel(tmp, &dr_regs->usbcmd);

	tmp = mt_readl(&dr_regs->usbcmd);
	tmp |= USB_CMD_CTRL_RESET;
	mt_writel(tmp, &dr_regs->usbcmd);

	/* Wait for reset to complete */
	timeout = jiffies + MT_UDC_RESET_TIMEOUT;
	while (mt_readl(&dr_regs->usbcmd) & USB_CMD_CTRL_RESET) {
		if (time_after(jiffies, timeout)) {
			ERR("udc reset timeout!\n");
			return -ETIMEDOUT;
		}
		cpu_relax();
	}

	/* Set the controller as device mode */
	tmp = mt_readl(&dr_regs->usbmode);
	tmp &= ~USB_MODE_CTRL_MODE_MASK;	/* clear mode bits */
	tmp |= USB_MODE_CTRL_MODE_DEVICE;
	/* Disable Setup Lockout */
	tmp |= USB_MODE_SETUP_LOCK_OFF;


	mt_writel(tmp, &dr_regs->usbmode);

	/* Clear the setup status */
	mt_writel(0, &dr_regs->usbsts);

	tmp = udc->ep_qh_dma;
	tmp &= USB_EP_LIST_ADDRESS_MASK;
	mt_writel(tmp, &dr_regs->endpointlistaddr);

	VDBG("vir[qh_base] is %p phy[qh_base] is 0x%8x reg is 0x%8x",
		udc->ep_qh, (int)tmp,
		mt_readl(&dr_regs->endpointlistaddr));
	
	/* Config PHY interface */
	portctrl = mt_readl(&dr_regs->portsc1);
	portctrl &= ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
	switch (udc->phy_mode) {
	case MT_USB2_PHY_ULPI:
		portctrl |= PORTSCX_PTS_ULPI;
		break;
	case MT_USB2_PHY_UTMI_WIDE:
		//portctrl |= PORTSCX_PTW_16BIT;
		/* fall through */
	case MT_USB2_PHY_UTMI:
		portctrl |= PORTSCX_PTS_UTMI;
		break;
	case MT_USB2_PHY_SERIAL:
		portctrl |= PORTSCX_PTS_FSLS;
		break;
	default:
		return -EINVAL;
	}
	mt_writel(portctrl, &dr_regs->portsc1);

	return 0;
}

/* Enable DR irq and set controller to run state */
static void dr_controller_run(struct mt_udc *udc)
{
	u32 temp;

	/* Enable DR irq reg */
	temp = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN
		| USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN
		| USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;

	mt_writel(temp, &dr_regs->usbintr);

	/* Clear stopped bit */
	udc->stopped = 0;

	/* Set the controller as device mode */
	temp = mt_readl(&dr_regs->usbmode);
	temp |= USB_MODE_CTRL_MODE_DEVICE;
	mt_writel(temp, &dr_regs->usbmode);

	/* Set controller to Run */
	temp = mt_readl(&dr_regs->usbcmd);
	temp |= USB_CMD_RUN_STOP;
	mt_writel(temp, &dr_regs->usbcmd);
}

static void dr_controller_stop(struct mt_udc *udc)
{
	unsigned int tmp;

	pr_debug("%s\n", __func__);

	/* if we're in OTG mode, and the Host is currently using the port,
	 * stop now and don't rip the controller out from under the
	 * ehci driver
	 */
	if (udc->gadget.is_otg) {
		if (!(mt_readl(&dr_regs->otgsc) & OTGSC_STS_USB_ID)) {
			pr_debug("udc: Leaving early\n");
			return;
		}
	}

	/* disable all INTR */
	mt_writel(0, &dr_regs->usbintr);

	/* Set stopped bit for isr */
	udc->stopped = 1;

	/* disable IO output */
/*	usb_sys_regs->control = 0; */

	/* set controller to Stop */
	tmp = mt_readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	mt_writel(tmp, &dr_regs->usbcmd);
}

static void dr_ep_setup(unsigned char ep_num, unsigned char dir,
			unsigned char ep_type)
{
	unsigned int tmp_epctrl = 0;

	tmp_epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);
	if (dir) {
		if (ep_num)
			tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
		tmp_epctrl |= EPCTRL_TX_ENABLE;
		tmp_epctrl &= ~EPCTRL_TX_TYPE;
		tmp_epctrl |= ((unsigned int)(ep_type)
				<< EPCTRL_TX_EP_TYPE_SHIFT);
	} else {
		if (ep_num)
			tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
		tmp_epctrl |= EPCTRL_RX_ENABLE;
		tmp_epctrl &= ~EPCTRL_RX_TYPE;
		tmp_epctrl |= ((unsigned int)(ep_type)
				<< EPCTRL_RX_EP_TYPE_SHIFT);
	}

	mt_writel(tmp_epctrl, &dr_regs->endptctrl[ep_num]);
}

static void
dr_ep_change_stall(unsigned char ep_num, unsigned char dir, int value)
{
	u32 tmp_epctrl = 0;

	tmp_epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);

	if (value) {
		/* set the stall bit */
		if (dir)
			tmp_epctrl |= EPCTRL_TX_EP_STALL;
		else
			tmp_epctrl |= EPCTRL_RX_EP_STALL;
	} else {
		/* clear the stall bit and reset data toggle */
		if (dir) {
			tmp_epctrl &= ~EPCTRL_TX_EP_STALL;
			tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
		} else {
			tmp_epctrl &= ~EPCTRL_RX_EP_STALL;
			tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
		}
	}
	mt_writel(tmp_epctrl, &dr_regs->endptctrl[ep_num]);
}

/* Get stall status of a specific ep
   Return: 0: not stalled; 1:stalled */
static int dr_ep_get_stall(unsigned char ep_num, unsigned char dir)
{
	u32 epctrl;

	epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);
	if (dir)
		return (epctrl & EPCTRL_TX_EP_STALL) ? 1 : 0;
	else
		return (epctrl & EPCTRL_RX_EP_STALL) ? 1 : 0;
}

/********************************************************************
	Internal Structure Build up functions
********************************************************************/

/*------------------------------------------------------------------
* struct_ep_qh_setup(): set the Endpoint Capabilites field of QH
 * @zlt: Zero Length Termination Select (1: disable; 0: enable)
 * @mult: Mult field
 ------------------------------------------------------------------*/
static void struct_ep_qh_setup(struct mt_udc *udc, unsigned char ep_num,
		unsigned char dir, unsigned char ep_type,
		unsigned int max_pkt_len,
		unsigned int zlt, unsigned char mult)
{
	struct ep_queue_head *p_QH = &udc->ep_qh[2 * ep_num + dir];
	unsigned int tmp = 0;

	/* set the Endpoint Capabilites in QH */
	switch (ep_type) {
	case USB_ENDPOINT_XFER_CONTROL:
		/* Interrupt On Setup (IOS). for control ep  */
		tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
			| EP_QUEUE_HEAD_IOS;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
			| (mult << EP_QUEUE_HEAD_MULT_POS);
		break;
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		tmp = max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS;
		break;
	default:
		VDBG("error ep type is %d", ep_type);
		return;
	}
	if (zlt)
		tmp |= EP_QUEUE_HEAD_ZLT_SEL;

	p_QH->max_pkt_length = cpu_to_hc32(tmp);
	p_QH->next_dtd_ptr = 1;
	p_QH->size_ioc_int_sts = 0;
}

/* Setup qh structure and ep register for ep0. */
static void ep0_setup(struct mt_udc *udc)
{
	/* the intialization of an ep includes: fields in QH, Regs,
	 * mt_ep struct */
	 /*! Steven modified on version 1.01
	   */
	  /* ep0 set zlt 1 no force zlp*/
	struct_ep_qh_setup(udc, 0, USB_RECV, USB_ENDPOINT_XFER_CONTROL,
			USB_MAX_CTRL_PAYLOAD, 1, 0);
	struct_ep_qh_setup(udc, 0, USB_SEND, USB_ENDPOINT_XFER_CONTROL,
			USB_MAX_CTRL_PAYLOAD, 1, 0);
	dr_ep_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL);
	dr_ep_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL);

	return;

}

/***********************************************************************
		Endpoint Management Functions
***********************************************************************/

/*-------------------------------------------------------------------------
 * when configurations are set, or when interface settings change
 * for example the do_set_interface() in gadget layer,
 * the driver will enable or disable the relevant endpoints
 * ep0 doesn't use this routine. It is always enabled.
-------------------------------------------------------------------------*/
static int mt_ep_enable(struct usb_ep *_ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct mt_udc *udc = NULL;
	struct mt_ep *ep = NULL;
	unsigned short max = 0;
	unsigned char mult = 0, zlt;
	int retval = -EINVAL;
	unsigned long flags = 0;

	ep = container_of(_ep, struct mt_ep, ep);

	/* catch various bogus parameters */
	if (!_ep || !desc
			|| (desc->bDescriptorType != USB_DT_ENDPOINT))
		return -EINVAL;

	udc = ep->udc;

	if (!udc->driver || (udc->gadget.speed == USB_SPEED_UNKNOWN))
		return -ESHUTDOWN;

	max = usb_endpoint_maxp(desc);

	/* Disable automatic zlp generation.  Driver is responsible to indicate
	 * explicitly through req->req.zero.  This is needed to enable multi-td
	 * request. */
	zlt = 1;

	/* Assume the max packet size from gadget is always correct */
	switch (desc->bmAttributes & 0x03) {
	case USB_ENDPOINT_XFER_CONTROL:
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		/* mult = 0.  Execute N Transactions as demonstrated by
		 * the USB variable length packet protocol where N is
		 * computed using the Maximum Packet Length (dQH) and
		 * the Total Bytes field (dTD) */
		mult = 0;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		/* Calculate transactions needed for high bandwidth iso */
		mult = (unsigned char)(1 + ((max >> 11) & 0x03));
		/*! Steven modified on version 1.02
		  */
		max = max & 0x7ff;	/* bit 0~10 */
		/* 3 transactions at most */
		if (mult > 3)
			goto en_done;
		break;
	default:
		goto en_done;
	}

	spin_lock_irqsave(&udc->lock, flags);
	ep->ep.maxpacket = max;
	ep->ep.desc = desc;
	ep->stopped = 0;

	/* Controller related setup */
	/* Init EPx Queue Head (Ep Capabilites field in QH
	 * according to max, zlt, mult) */
	struct_ep_qh_setup(udc, (unsigned char) ep_index(ep),
			(unsigned char) ((desc->bEndpointAddress & USB_DIR_IN)
					?  USB_SEND : USB_RECV),
			(unsigned char) (desc->bmAttributes
					& USB_ENDPOINT_XFERTYPE_MASK),
			max, zlt, mult);

	/* Init endpoint ctrl register */
	dr_ep_setup((unsigned char) ep_index(ep),
			(unsigned char) ((desc->bEndpointAddress & USB_DIR_IN)
					? USB_SEND : USB_RECV),
			(unsigned char) (desc->bmAttributes
					& USB_ENDPOINT_XFERTYPE_MASK));

	spin_unlock_irqrestore(&udc->lock, flags);
	retval = 0;

	VDBG("enabled %s (ep%d%s) maxpacket %d",ep->ep.name,
			ep->ep.desc->bEndpointAddress & 0x0f,
			(desc->bEndpointAddress & USB_DIR_IN)
				? "in" : "out", max);
en_done:
	return retval;
}

/*---------------------------------------------------------------------
 * @ep : the ep being unconfigured. May not be ep0
 * Any pending and uncomplete req will complete with status (-ESHUTDOWN)
*---------------------------------------------------------------------*/
static int mt_ep_disable(struct usb_ep *_ep)
{
	struct mt_udc *udc = NULL;
	struct mt_ep *ep = NULL;
	unsigned long flags = 0;
	u32 epctrl;
	int ep_num;

	ep = container_of(_ep, struct mt_ep, ep);
	if (!_ep || !ep->ep.desc) {
		VDBG("%s not enabled", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	/* disable ep on controller */
	ep_num = ep_index(ep);
	epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep)) {
		epctrl &= ~(EPCTRL_TX_ENABLE | EPCTRL_TX_TYPE);
		epctrl |= EPCTRL_EP_TYPE_BULK << EPCTRL_TX_EP_TYPE_SHIFT;
	} else {
		epctrl &= ~(EPCTRL_RX_ENABLE | EPCTRL_TX_TYPE);
		epctrl |= EPCTRL_EP_TYPE_BULK << EPCTRL_RX_EP_TYPE_SHIFT;
	}
	mt_writel(epctrl, &dr_regs->endptctrl[ep_num]);

	udc = (struct mt_udc *)ep->udc;
	spin_lock_irqsave(&udc->lock, flags);

	/* nuke all pending requests (does flush) */
	nuke(ep, -ESHUTDOWN);

	ep->ep.desc = NULL;
	ep->stopped = 1;
	spin_unlock_irqrestore(&udc->lock, flags);

	VDBG("disabled %s OK", _ep->name);
	return 0;
}

/*---------------------------------------------------------------------
 * allocate a request object used by this endpoint
 * the main operation is to insert the req->queue to the eq->queue
 * Returns the request, or null if one could not be allocated
*---------------------------------------------------------------------*/
static struct usb_request *
mt_alloc_request(struct usb_ep *_ep, gfp_t gfp_flags)
{
	struct mt_req *req = NULL;

	req = kzalloc(sizeof *req, gfp_flags);
	if (!req)
		return NULL;

	req->req.dma = DMA_ADDR_INVALID;
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void mt_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct mt_req *req = NULL;

	req = container_of(_req, struct mt_req, req);

	if (_req)
		kfree(req);
}

/* Actually add a dTD chain to an empty dQH and let go */
static void mt_prime_ep(struct mt_ep *ep, struct ep_td_struct *td)
{
	struct ep_queue_head *qh = get_qh_by_ep(ep);

	/* Write dQH next pointer and terminate bit to 0 */
	qh->next_dtd_ptr = cpu_to_hc32(td->td_dma
			& EP_QUEUE_HEAD_NEXT_POINTER_MASK);

	/* Clear active and halt bit */
	qh->size_ioc_int_sts &= cpu_to_hc32(~(EP_QUEUE_HEAD_STATUS_ACTIVE
					| EP_QUEUE_HEAD_STATUS_HALT));

	/* Ensure that updates to the QH will occur before priming. */
	wmb();

	/* Prime endpoint by writing correct bit to ENDPTPRIME */
	mt_writel(ep_is_in(ep) ? (1 << (ep_index(ep) + 16))
			: (1 << (ep_index(ep))), &dr_regs->endpointprime);
}

/* Add dTD chain to the dQH of an EP */
static void mt_queue_td(struct mt_ep *ep, struct mt_req *req)
{
	u32 temp, bitmask, tmp_stat;

	/* VDBG("QH addr Register 0x%8x", dr_regs->endpointlistaddr);
	VDBG("ep_qh[%d] addr is 0x%8x", i, (u32)&(ep->udc->ep_qh[i])); */

	bitmask = ep_is_in(ep)
		? (1 << (ep_index(ep) + 16))
		: (1 << (ep_index(ep)));

	/* check if the pipe is empty */
#if defined(PRIME_STATUS_PATCH)
	if (!(list_empty(&ep->queue)) && !(ep_index(ep) == 0)) {
#else
	if (!(list_empty(&ep->queue))) {
#endif
		/* Add td to the end */
		struct mt_req *lastreq;
		lastreq = list_entry(ep->queue.prev, struct mt_req, queue);
		lastreq->tail->next_td_ptr =
			cpu_to_hc32(req->head->td_dma & DTD_ADDR_MASK);
		/* Ensure dTD's next dtd pointer to be updated */
		wmb();
		/* Read prime bit, if 1 goto done */
		if (mt_readl(&dr_regs->endpointprime) & bitmask)
			return;

		do {
			/* Set ATDTW bit in USBCMD */
			temp = mt_readl(&dr_regs->usbcmd);
			mt_writel(temp | USB_CMD_ATDTW, &dr_regs->usbcmd);

			/* Read correct status bit */
			tmp_stat = mt_readl(&dr_regs->endptstatus) & bitmask;

		} while (!(mt_readl(&dr_regs->usbcmd) & USB_CMD_ATDTW));

		/* Write ATDTW bit to 0 */
		temp = mt_readl(&dr_regs->usbcmd);
		mt_writel(temp & ~USB_CMD_ATDTW, &dr_regs->usbcmd);

		if (tmp_stat)
			return;
	}

	mt_prime_ep(ep, req->head);
}

/* Fill in the dTD structure
 * @req: request that the transfer belongs to
 * @length: return actually data length of the dTD
 * @dma: return dma address of the dTD
 * @is_last: return flag if it is the last dTD of the request
 * return: pointer to the built dTD */
static struct ep_td_struct *mt_build_dtd(struct mt_req *req, unsigned *length,
		dma_addr_t *dma, int *is_last, gfp_t gfp_flags)
{
	u32 swap_temp;
	struct ep_td_struct *dtd;

	/* how big will this transfer be? */
	*length = min(req->req.length - req->req.actual,
			(unsigned)EP_MAX_LENGTH_TRANSFER);

	dtd = dma_pool_alloc(udc_controller->td_pool, gfp_flags, dma);
	if (dtd == NULL)
		return dtd;

	dtd->td_dma = *dma;
	/* Clear reserved field */
	swap_temp = hc32_to_cpu(dtd->size_ioc_sts);
	swap_temp &= ~DTD_RESERVED_FIELDS;
	dtd->size_ioc_sts = cpu_to_hc32(swap_temp);

	/* Init all of buffer page pointers */
	swap_temp = (u32) (req->req.dma + req->req.actual);
	dtd->buff_ptr0 = cpu_to_hc32(swap_temp);
	dtd->buff_ptr1 = cpu_to_hc32(swap_temp + 0x1000);
	dtd->buff_ptr2 = cpu_to_hc32(swap_temp + 0x2000);
	dtd->buff_ptr3 = cpu_to_hc32(swap_temp + 0x3000);
	dtd->buff_ptr4 = cpu_to_hc32(swap_temp + 0x4000);

	req->req.actual += *length;

	/* zlp is needed if req->req.zero is set */
	if (req->req.zero) {
		if (*length == 0 || (*length % req->ep->ep.maxpacket) != 0)
			*is_last = 1;
		else
			*is_last = 0;
	} else if (req->req.length == req->req.actual)
		*is_last = 1;
	else
		*is_last = 0;

	if ((*is_last) == 0)
		VDBG("multi-dtd request!");
	/*!
	   Steven modified on version 1.01
	  */
	/* Initialize the actual value of req*/  
	else
		req->req.actual = 0;
	
	/* Fill in the transfer size; set active bit */
	swap_temp = ((*length << DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE);

	/* Enable interrupt for the last dtd of a request */
	if (*is_last && !req->req.no_interrupt)
		swap_temp |= DTD_IOC;

	dtd->size_ioc_sts = cpu_to_hc32(swap_temp);

	mb();

	VDBG("length = %d address= 0x%x", *length, (int)*dma);

	return dtd;
}

/* Generate dtd chain for a request */
static int mt_req_to_dtd(struct mt_req *req, gfp_t gfp_flags)
{
	unsigned	count;
	int		is_last;
	int		is_first =1;
	struct ep_td_struct	*last_dtd = NULL, *dtd;
	dma_addr_t dma;

	do {
		dtd = mt_build_dtd(req, &count, &dma, &is_last, gfp_flags);
		if (dtd == NULL)
			return -ENOMEM;

		if (is_first) {
			is_first = 0;
			req->head = dtd;
		} else {
			last_dtd->next_td_ptr = cpu_to_hc32(dma);
			last_dtd->next_td_virt = dtd;
		}
		last_dtd = dtd;

		req->dtd_count++;
	} while (!is_last);

	dtd->next_td_ptr = cpu_to_hc32(DTD_NEXT_TERMINATE);

	req->tail = dtd;

	return 0;
}

/* queues (submits) an I/O request to an endpoint */
static int
mt_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct mt_ep *ep = container_of(_ep, struct mt_ep, ep);
	struct mt_req *req = container_of(_req, struct mt_req, req);
	struct mt_udc *udc;
	unsigned long flags;
	int ret;

	/*! Steven add this on version 1.01
	  */
	int zlflag = 0;

	/* catch various bogus parameters */
	if (!_req || !req->req.complete || !req->req.buf
			|| !list_empty(&req->queue)) {
		VDBG("%s, bad params", __func__);
		return -EINVAL;
	}
	if (unlikely(!_ep || !ep->ep.desc)) {
		VDBG("%s, bad ep", __func__);
		return -EINVAL;
	}
	if (usb_endpoint_xfer_isoc(ep->ep.desc)) {
#if 0//mask for multi test
		if (req->req.length > ep->ep.maxpacket)
			return -EMSGSIZE;
#endif
	}

	udc = ep->udc;
	if (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	req->ep = ep;

	/* map virtual address to hardware */
	/*! Steven modified on version 1.01
	 */
	/* workaround dma map error when req->req.length is 0*/	
	if (req->req.length == 0) {
		zlflag = 1;
		req->req.length++;
	}

	ret = usb_gadget_map_request(&ep->udc->gadget, &req->req, ep_is_in(ep));
	if (ret)
		return ret;

#if !defined(USB_BURSTSIZE_56_BYTES)
    if ((unsigned int )req->req.buf & 0x00000003)
    {
        if(req->req.num_sgs)
            return -ENOMEM;

        req->req.align_buf = kmalloc(req->req.length + 32, GFP_ATOMIC);
        if(NULL==req->req.align_buf)
            return -ENOMEM;

        if(ep_is_in(ep))
        {
            req->req.is_in = 1;
            memcpy(req->req.align_buf, req->req.buf, req->req.length);

            req->req.dma = dma_map_single(ep->udc->gadget.dev.parent, req->req.align_buf
                                          , req->req.length, DMA_TO_DEVICE);
        }
        else
        {
            req->req.is_in = 0;
            req->req.dma = dma_map_single(ep->udc->gadget.dev.parent, req->req.align_buf
                                          , req->req.length, DMA_FROM_DEVICE);
        }
    }
    else
    {
        req->req.align_buf = 0;
    }
#endif

	if (zlflag && (req->req.length == 1)) {
		zlflag = 0;
		req->req.length = 0;
	}

	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->dtd_count = 0;

	/* build dtds and push them to device queue */
	if (!mt_req_to_dtd(req, gfp_flags)) {
		spin_lock_irqsave(&udc->lock, flags);
		mt_queue_td(ep, req);
	} else {
#if !defined(USB_BURSTSIZE_56_BYTES)
        if(req->req.align_buf)
        {
            kfree(req->req.align_buf);
            req->req.align_buf = 0;
        }
#endif
		return -ENOMEM;
	}

#if !defined(PRIME_STATUS_PATCH)
    /* Update ep0 state */
	if ((ep_index(ep) == 0))
		udc->ep0_state = DATA_STATE_XMIT;
#endif

	/* irq handler advances the queue */
	if (req != NULL)
		list_add_tail(&req->queue, &ep->queue);
	spin_unlock_irqrestore(&udc->lock, flags);

	return 0;
}

/* dequeues (cancels, unlinks) an I/O request from an endpoint */
static int mt_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct mt_ep *ep = container_of(_ep, struct mt_ep, ep);
	struct mt_req *req;
	unsigned long flags;
	int ep_num, stopped, ret = 0;
	u32 epctrl;

	if (!_ep || !_req)
		return -EINVAL;

	spin_lock_irqsave(&ep->udc->lock, flags);
	stopped = ep->stopped;

	/* Stop the ep before we deal with the queue */
	ep->stopped = 1;
	ep_num = ep_index(ep);
	epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep))
		epctrl &= ~EPCTRL_TX_ENABLE;
	else
		epctrl &= ~EPCTRL_RX_ENABLE;
	mt_writel(epctrl, &dr_regs->endptctrl[ep_num]);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		ret = -EINVAL;
		goto out;
	}

	/* The request is in progress, or completed but not dequeued */
	if (ep->queue.next == &req->queue) {
		_req->status = -ECONNRESET;
		mt_ep_fifo_flush(_ep);	/* flush current transfer */

		/* The request isn't the last request in this ep queue */
		if (req->queue.next != &ep->queue) {
			struct mt_req *next_req;

			next_req = list_entry(req->queue.next, struct mt_req,
					queue);

			/* prime with dTD of next request */
			mt_prime_ep(ep, next_req->head);
		}
	/* The request hasn't been processed, patch up the TD chain */
	} else {
		struct mt_req *prev_req;

		prev_req = list_entry(req->queue.prev, struct mt_req, queue);
		prev_req->tail->next_td_ptr = req->tail->next_td_ptr;
	}

	done(ep, req, -ECONNRESET);

	/* Enable EP */
out:	epctrl = mt_readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep))
		epctrl |= EPCTRL_TX_ENABLE;
	else
		epctrl |= EPCTRL_RX_ENABLE;
	mt_writel(epctrl, &dr_regs->endptctrl[ep_num]);
	ep->stopped = stopped;

	spin_unlock_irqrestore(&ep->udc->lock, flags);
	return ret;
}

/*-------------------------------------------------------------------------*/

/*-----------------------------------------------------------------
 * modify the endpoint halt feature
 * @ep: the non-isochronous endpoint being stalled
 * @value: 1--set halt  0--clear halt
 * Returns zero, or a negative error code.
*----------------------------------------------------------------*/
static int mt_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct mt_ep *ep = NULL;
	unsigned long flags = 0;
	int status = -EOPNOTSUPP;	/* operation not supported */
	unsigned char ep_dir = 0, ep_num = 0;
	struct mt_udc *udc = NULL;

	ep = container_of(_ep, struct mt_ep, ep);
	udc = ep->udc;
	if (!_ep || !ep->ep.desc) {
		status = -EINVAL;
		goto out;
	}

	if (usb_endpoint_xfer_isoc(ep->ep.desc)) {
		status = -EOPNOTSUPP;
		goto out;
	}

	/* Attempt to halt IN ep will fail if any transfer requests
	 * are still queue */
#if 0//mask for usbtest case t13
	if (value && ep_is_in(ep) && !list_empty(&ep->queue)) {
		status = -EAGAIN;
		goto out;
	}
#endif

	status = 0;
	ep_dir = ep_is_in(ep) ? USB_SEND : USB_RECV;
	ep_num = (unsigned char)(ep_index(ep));
	spin_lock_irqsave(&ep->udc->lock, flags);
	dr_ep_change_stall(ep_num, ep_dir, value);
	spin_unlock_irqrestore(&ep->udc->lock, flags);

	if (ep_index(ep) == 0) {
		udc->ep0_state = WAIT_FOR_SETUP;
		udc->ep0_dir = 0;
	}
out:
	VDBG(" %s %s halt stat %d", ep->ep.name,
			value ?  "set" : "clear", status);

	return status;
}

static int mt_ep_fifo_status(struct usb_ep *_ep)
{
	struct mt_ep *ep;
	struct mt_udc *udc;
	int size = 0;
	u32 bitmask;
	struct ep_queue_head *qh;

	ep = container_of(_ep, struct mt_ep, ep);
	if (!_ep || (!ep->ep.desc && ep_index(ep) != 0))
		return -ENODEV;

	udc = (struct mt_udc *)ep->udc;

	if (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	qh = get_qh_by_ep(ep);

	bitmask = (ep_is_in(ep)) ? (1 << (ep_index(ep) + 16)) :
	    (1 << (ep_index(ep)));

	if (mt_readl(&dr_regs->endptstatus) & bitmask)
		size = (qh->size_ioc_int_sts & DTD_PACKET_SIZE)
		    >> DTD_LENGTH_BIT_POS;

	pr_debug("%s %u\n", __func__, size);
	return size;
}

static void mt_ep_fifo_flush(struct usb_ep *_ep)
{
	struct mt_ep *ep;
	int ep_num, ep_dir;
	u32 bits;
	unsigned long timeout;
#define FSL_UDC_FLUSH_TIMEOUT 1000

	if (!_ep) {
		return;
	} else {
		ep = container_of(_ep, struct mt_ep, ep);
		if (!ep->ep.desc)
			return;
	}
	ep_num = ep_index(ep);
	ep_dir = ep_is_in(ep) ? USB_SEND : USB_RECV;

	if (ep_num == 0)
		bits = (1 << 16) | 1;
	else if (ep_dir == USB_SEND)
		bits = 1 << (16 + ep_num);
	else
		bits = 1 << ep_num;

	timeout = jiffies + FSL_UDC_FLUSH_TIMEOUT;
	do {
		mt_writel(bits, &dr_regs->endptflush);

		/* Wait until flush complete */
		while (mt_readl(&dr_regs->endptflush)) {
			if (time_after(jiffies, timeout)) {
				ERR("ep flush timeout\n");
				return;
			}
			cpu_relax();
		}
		/* See if we need to flush again */
	} while (mt_readl(&dr_regs->endptstatus) & bits);
}

static struct usb_ep_ops mt_ep_ops = {
	.enable = mt_ep_enable,
	.disable = mt_ep_disable,

	.alloc_request = mt_alloc_request,
	.free_request = mt_free_request,

	.queue = mt_ep_queue,
	.dequeue = mt_ep_dequeue,

	.set_halt = mt_ep_set_halt,
	.fifo_status = mt_ep_fifo_status,
	.fifo_flush = mt_ep_fifo_flush,	/* flush fifo */
};

/*-------------------------------------------------------------------------
		Gadget Driver Layer Operations
-------------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * Get the current frame number (from DR frame_index Reg )
 *----------------------------------------------------------------------*/
static int mt_get_frame(struct usb_gadget *gadget)
{
	return (int)(mt_readl(&dr_regs->frindex) & USB_FRINDEX_MASKS);
}

/*-----------------------------------------------------------------------
 * Tries to wake up the host connected to this gadget
 -----------------------------------------------------------------------*/
static int mt_wakeup(struct usb_gadget *gadget)
{
	struct mt_udc *udc = container_of(gadget, struct mt_udc, gadget);
	u32 portsc;

	/* Remote wakeup feature not enabled by host */
	if (!udc->remote_wakeup)
		return -ENOTSUPP;

	portsc = mt_readl(&dr_regs->portsc1);
	/* not suspended? */
	if (!(portsc & PORTSCX_PORT_SUSPEND))
		return 0;
	/* trigger force resume */
	portsc |= PORTSCX_PORT_FORCE_RESUME;
	mt_writel(portsc, &dr_regs->portsc1);
	return 0;
}

static int can_pullup(struct mt_udc *udc)
{
	return udc->driver && udc->softconnect && udc->vbus_active;
}

/* Notify controller that VBUS is powered, Called by whatever
   detects VBUS sessions */
static int mt_vbus_session(struct usb_gadget *gadget, int is_active)
{
	struct mt_udc	*udc;
	unsigned long	flags;

	udc = container_of(gadget, struct mt_udc, gadget);
	spin_lock_irqsave(&udc->lock, flags);
	VDBG("VBUS %s", is_active ? "on" : "off");
	udc->vbus_active = (is_active != 0);
	if (can_pullup(udc))
		mt_writel((mt_readl(&dr_regs->usbcmd) | USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	else
		mt_writel((mt_readl(&dr_regs->usbcmd) & ~USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

/* constrain controller's VBUS power usage
 * This call is used by gadget drivers during SET_CONFIGURATION calls,
 * reporting how much power the device may consume.  For example, this
 * could affect how quickly batteries are recharged.
 *
 * Returns zero on success, else negative errno.
 */
static int mt_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	struct mt_udc *udc;

	udc = container_of(gadget, struct mt_udc, gadget);
	if (!IS_ERR_OR_NULL(udc->transceiver))
		return usb_phy_set_power(udc->transceiver, mA);
	return -ENOTSUPP;
}

/* Change Data+ pullup status
 * this func is used by usb_gadget_connect/disconnet
 */
static int mt_pullup(struct usb_gadget *gadget, int is_on)
{
	struct mt_udc *udc;

	udc = container_of(gadget, struct mt_udc, gadget);

	if (!udc->vbus_active)
		return -EOPNOTSUPP;

	udc->softconnect = (is_on != 0);
	if (can_pullup(udc))
		mt_writel((mt_readl(&dr_regs->usbcmd) | USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	else
		mt_writel((mt_readl(&dr_regs->usbcmd) & ~USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);

	return 0;
}

static int mt_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver);
static int mt_udc_stop(struct usb_gadget *g);

static const struct usb_gadget_ops mt_gadget_ops = {
	.get_frame = mt_get_frame,
	.wakeup = mt_wakeup,
/*	.set_selfpowered = mt_set_selfpowered,	*/ /* Always selfpowered */
	.vbus_session = mt_vbus_session,
	.vbus_draw = mt_vbus_draw,
	.pullup = mt_pullup,
	.udc_start = mt_udc_start,
	.udc_stop = mt_udc_stop,
};

/*
 * Empty complete function used by this driver to fill in the req->complete
 * field when creating a request since the complete field is mandatory.
 */
static void mt_noop_complete(struct usb_ep *ep, struct usb_request *req) { }

/* Set protocol stall on ep0, protocol stall will automatically be cleared
   on new transaction */
static void ep0stall(struct mt_udc *udc)
{
	u32 tmp;

	/* must set tx and rx to stall at the same time */
	tmp = mt_readl(&dr_regs->endptctrl[0]);
	tmp |= EPCTRL_TX_EP_STALL | EPCTRL_RX_EP_STALL;
	mt_writel(tmp, &dr_regs->endptctrl[0]);
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->ep0_dir = 0;
}

/* Prime a status phase for ep0 */
static int ep0_prime_status(struct mt_udc *udc, int direction)
{
	struct mt_req *req = udc->status_req;
	struct mt_ep *ep;
	int ret;

	if (direction == EP_DIR_IN)
		udc->ep0_dir = USB_DIR_IN;
	else
		udc->ep0_dir = USB_DIR_OUT;

	ep = &udc->eps[0];
#if defined(PRIME_STATUS_PATCH)
	if (udc->ep0_state != DATA_STATE_XMIT)
		udc->ep0_state = WAIT_FOR_OUT_STATUS;
#else
	udc->ep0_state = WAIT_FOR_OUT_STATUS;
#endif

	req->ep = ep;
	req->req.length = 0;
	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->req.complete = mt_noop_complete;
	req->dtd_count = 0;

	ret = usb_gadget_map_request(&ep->udc->gadget, &req->req, ep_is_in(ep));
	if (ret)
		return ret;

	if (mt_req_to_dtd(req, GFP_ATOMIC) == 0)
		mt_queue_td(ep, req);
	else
		return -ENOMEM;

	list_add_tail(&req->queue, &ep->queue);

	return 0;
}

static void udc_reset_ep_queue(struct mt_udc *udc, u8 pipe)
{
	struct mt_ep *ep = get_ep_by_pipe(udc, pipe);

	if (ep->name)
		nuke(ep, -ESHUTDOWN);
}

/*
 * ch9 Set address
 */
static void ch9setaddress(struct mt_udc *udc, u16 value, u16 index, u16 length)
{
	/* Save the new address to device struct */
	udc->device_address = (u8) value;
	/* Update usb state */
	udc->usb_state = USB_STATE_ADDRESS;
	/* Status phase */
	if (ep0_prime_status(udc, EP_DIR_IN))
		ep0stall(udc);
}

/*
 * ch9 Get status
 */
static void ch9getstatus(struct mt_udc *udc, u8 request_type, u16 value,
		u16 index, u16 length)
{
	u16 tmp = 0;		/* Status, cpu endian */
	struct mt_req *req;
	struct mt_ep *ep;
	int ret;

	ep = &udc->eps[0];

	if ((request_type & USB_RECIP_MASK) == USB_RECIP_DEVICE) {
		/* Get device status */
		tmp = udc->gadget.is_selfpowered;
		tmp |= udc->remote_wakeup << USB_DEVICE_REMOTE_WAKEUP;
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
		/* Get interface status */
		/* We don't have interface information in udc driver */
		tmp = 0;
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
		/* Get endpoint status */
		struct mt_ep *target_ep;

		target_ep = get_ep_by_pipe(udc, get_pipe_by_windex(index));

		/* stall if endpoint doesn't exist */
		if (!target_ep->ep.desc)
			goto stall;
		tmp = dr_ep_get_stall(ep_index(target_ep), ep_is_in(target_ep))
				<< USB_ENDPOINT_HALT;
	}

	udc->ep0_dir = USB_DIR_IN;
#if defined(PRIME_STATUS_PATCH)
	req = udc->ch9getstatus_req;
#else
	/* Borrow the per device status_req */
	req = udc->status_req;
#endif
	/* Fill in the reqest structure */
	*((u16 *) req->req.buf) = cpu_to_le16(tmp);

	req->ep = ep;
	req->req.length = 2;
	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->req.complete = mt_noop_complete;
	req->dtd_count = 0;

	ret = usb_gadget_map_request(&ep->udc->gadget, &req->req, ep_is_in(ep));
	if (ret)
		goto stall;

	/* prime the data phase */
	if ((mt_req_to_dtd(req, GFP_ATOMIC) == 0))
		mt_queue_td(ep, req);
	else			/* no mem */
		goto stall;

	list_add_tail(&req->queue, &ep->queue);
	udc->ep0_state = DATA_STATE_XMIT;
#if defined(PRIME_STATUS_PATCH)
	if (ep0_prime_status(udc, EP_DIR_OUT))
		ep0stall(udc);
#endif

	return;
stall:
	ep0stall(udc);
}

static void setup_received_irq(struct mt_udc *udc,
		struct usb_ctrlrequest *setup)
__releases(udc->lock)
__acquires(udc->lock)
{
	u16 wValue = le16_to_cpu(setup->wValue);
	u16 wIndex = le16_to_cpu(setup->wIndex);
	u16 wLength = le16_to_cpu(setup->wLength);

	udc_reset_ep_queue(udc, 0);

	/* We process some stardard setup requests here */
	switch (setup->bRequest) {
	case USB_REQ_GET_STATUS:
		/* Data+Status phase from udc */
		if ((setup->bRequestType & (USB_DIR_IN | USB_TYPE_MASK))
					!= (USB_DIR_IN | USB_TYPE_STANDARD))
			break;
		ch9getstatus(udc, setup->bRequestType, wValue, wIndex, wLength);
		return;

	case USB_REQ_SET_ADDRESS:
		/* Status phase from udc */
		if (setup->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD
						| USB_RECIP_DEVICE))
			break;
		ch9setaddress(udc, wValue, wIndex, wLength);
		return;

	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
		/* Status phase from udc */
	{
		int rc = -EOPNOTSUPP;
		u16 ptc = 0;

		if ((setup->bRequestType & (USB_RECIP_MASK | USB_TYPE_MASK))
				== (USB_RECIP_ENDPOINT | USB_TYPE_STANDARD)) {
			int pipe = get_pipe_by_windex(wIndex);
			struct mt_ep *ep;

			if (wValue != 0 || wLength != 0 || pipe >= udc->max_ep)
				break;
			ep = get_ep_by_pipe(udc, pipe);

			spin_unlock(&udc->lock);
			rc = mt_ep_set_halt(&ep->ep,
					(setup->bRequest == USB_REQ_SET_FEATURE)
						? 1 : 0);
			spin_lock(&udc->lock);

		} else if ((setup->bRequestType & (USB_RECIP_MASK
				| USB_TYPE_MASK)) == (USB_RECIP_DEVICE
				| USB_TYPE_STANDARD)) {
			/* Note: The driver has not include OTG support yet.
			 * This will be set when OTG support is added */
			if (wValue == USB_DEVICE_TEST_MODE)
				ptc = wIndex >> 8;
			else if (gadget_is_otg(&udc->gadget)) {
				if (setup->bRequest ==
				    USB_DEVICE_B_HNP_ENABLE)
					udc->gadget.b_hnp_enable = 1;
				else if (setup->bRequest ==
					 USB_DEVICE_A_HNP_SUPPORT)
					udc->gadget.a_hnp_support = 1;
				else if (setup->bRequest ==
					 USB_DEVICE_A_ALT_HNP_SUPPORT)
					udc->gadget.a_alt_hnp_support = 1;
			}
			rc = 0;
		} else
			break;

		if (rc == 0) {
			if (ep0_prime_status(udc, EP_DIR_IN))
				ep0stall(udc);
		}
		if (ptc) {
			u32 tmp;

			mdelay(10);
			tmp = mt_readl(&dr_regs->portsc1) | (ptc << 16);
			mt_writel(tmp, &dr_regs->portsc1);
			printk(KERN_INFO "udc: switch to test mode %d.\n", ptc);
		}

		return;
	}

	default:
		break;
	}

	/* Requests handled by gadget */
	if (wLength) {
		/* Data phase from gadget, status phase from udc */
		udc->ep0_dir = (setup->bRequestType & USB_DIR_IN)
				?  USB_DIR_IN : USB_DIR_OUT;
		spin_unlock(&udc->lock);
		if (udc->driver->setup(&udc->gadget,
				&udc->local_setup_buff) < 0)
			ep0stall(udc);
		spin_lock(&udc->lock);
		udc->ep0_state = (setup->bRequestType & USB_DIR_IN)
				?  DATA_STATE_XMIT : DATA_STATE_RECV;
#if defined(PRIME_STATUS_PATCH)
		/*
		 * If the data stage is IN, send status prime immediately.
		 * See 2.0 Spec chapter 8.5.3.3 for detail.
		 */
		if (udc->ep0_state == DATA_STATE_XMIT)
			if (ep0_prime_status(udc, EP_DIR_OUT))
				ep0stall(udc);
#endif
	} else {
		/* No data phase, IN status from gadget */
		udc->ep0_dir = USB_DIR_IN;
		spin_unlock(&udc->lock);
		if (udc->driver->setup(&udc->gadget,
				&udc->local_setup_buff) < 0)
			ep0stall(udc);
		spin_lock(&udc->lock);
		udc->ep0_state = WAIT_FOR_OUT_STATUS;
	}
}

/* Process request for Data or Status phase of ep0
 * prime status phase if needed */
static void ep0_req_complete(struct mt_udc *udc, struct mt_ep *ep0,
		struct mt_req *req)
{
	if (udc->usb_state == USB_STATE_ADDRESS) {
		/* Set the new address */
		u32 new_address = (u32) udc->device_address;
		mt_writel(new_address << USB_DEVICE_ADDRESS_BIT_POS,
				&dr_regs->deviceaddr);
	}

	done(ep0, req, 0);

	switch (udc->ep0_state) {
	case DATA_STATE_XMIT:
#if defined(PRIME_STATUS_PATCH)
		/* already primed at setup_received_irq */
		udc->ep0_state = WAIT_FOR_OUT_STATUS;
#else
		/* receive status phase */
		if (ep0_prime_status(udc, EP_DIR_OUT))
			ep0stall(udc);
#endif
		break;
	case DATA_STATE_RECV:
		/* send status phase */
		if (ep0_prime_status(udc, EP_DIR_IN))
			ep0stall(udc);
		break;
	case WAIT_FOR_OUT_STATUS:
		udc->ep0_state = WAIT_FOR_SETUP;
		break;
	case WAIT_FOR_SETUP:
		ERR("Unexpect ep0 packets\n");
		break;
	default:
		ep0stall(udc);
		break;
	}
}

/* Tripwire mechanism to ensure a setup packet payload is extracted without
 * being corrupted by another incoming setup packet */
static void tripwire_handler(struct mt_udc *udc, u8 ep_num, u8 *buffer_ptr)
{
	u32 temp;
	struct ep_queue_head *qh;

	qh = &udc->ep_qh[ep_num * 2 + EP_DIR_OUT];

	/* Clear bit in ENDPTSETUPSTAT */
	temp = mt_readl(&dr_regs->endptsetupstat);
	mt_writel(temp | (1 << ep_num), &dr_regs->endptsetupstat);

	/* while a hazard exists when setup package arrives */
	do {
		/* Set Setup Tripwire */
		temp = mt_readl(&dr_regs->usbcmd);
		mt_writel(temp | USB_CMD_SUTW, &dr_regs->usbcmd);

		/* Copy the setup packet to local buffer */
		memcpy(buffer_ptr, (u8 *) qh->setup_buffer, 8);
	} while (!(mt_readl(&dr_regs->usbcmd) & USB_CMD_SUTW));

	/* Clear Setup Tripwire */
	temp = mt_readl(&dr_regs->usbcmd);
	mt_writel(temp & ~USB_CMD_SUTW, &dr_regs->usbcmd);
}

/* process-ep_req(): free the completed Tds for this req */
static int process_ep_req(struct mt_udc *udc, int pipe,
		struct mt_req *curr_req)
{
	struct ep_td_struct *curr_td;
	int	td_complete, actual, remaining_length, j, tmp;
	int	status = 0;
	int	errors = 0;
	struct  ep_queue_head *curr_qh = &udc->ep_qh[pipe];
	int direction = pipe % 2;

	curr_td = curr_req->head;
	td_complete = 0;
	actual = curr_req->req.length;

	for (j = 0; j < curr_req->dtd_count; j++) {
		remaining_length = (hc32_to_cpu(curr_td->size_ioc_sts)
					& DTD_PACKET_SIZE)
				>> DTD_LENGTH_BIT_POS;
		actual -= remaining_length;

		errors = hc32_to_cpu(curr_td->size_ioc_sts);
		if (errors & DTD_ERROR_MASK) {
			if (errors & DTD_STATUS_HALTED) {
				ERR("dTD error %08x QH=%d\n", errors, pipe);
				/* Clear the errors and Halt condition */
				tmp = hc32_to_cpu(curr_qh->size_ioc_int_sts);
				tmp &= ~errors;
				curr_qh->size_ioc_int_sts = cpu_to_hc32(tmp);
				status = -EPIPE;
				/* FIXME: continue with next queued TD? */

				break;
			}
			if (errors & DTD_STATUS_DATA_BUFF_ERR) {
				VDBG("Transfer overflow");
				status = -EPROTO;
				break;
			} else if (errors & DTD_STATUS_TRANSACTION_ERR) {
				VDBG("ISO error");
				status = -EILSEQ;
				break;
			} else
				ERR("Unknown error has occurred (0x%x)!\n",
					errors);

		} else if (hc32_to_cpu(curr_td->size_ioc_sts)
				& DTD_STATUS_ACTIVE) {
			VDBG("Request not complete");
			status = REQ_UNCOMPLETE;
			return status;
		} else if (remaining_length) {
			if (direction) {
				VDBG("Transmit dTD remaining length not zero");
				status = -EPROTO;
				break;
			} else {
				td_complete++;
				break;
			}
		} else {
			td_complete++;
			VDBG("dTD transmitted successful");
		}

		if (j != curr_req->dtd_count - 1)
			curr_td = (struct ep_td_struct *)curr_td->next_td_virt;
	}

	if (status)
		return status;

	curr_req->req.actual = actual;

	return 0;
}

/* Process a DTD completion interrupt */
static void dtd_complete_irq(struct mt_udc *udc)
{
	u32 bit_pos;
	int i, ep_num, direction, bit_mask, status;
	struct mt_ep *curr_ep;
	struct mt_req *curr_req, *temp_req;

	/* Clear the bits in the register */
	bit_pos = mt_readl(&dr_regs->endptcomplete);
	mt_writel(bit_pos, &dr_regs->endptcomplete);

	if (!bit_pos)
		return;

	for (i = 0; i < udc->max_ep; i++) {
		ep_num = i >> 1;
		direction = i % 2;

		bit_mask = 1 << (ep_num + 16 * direction);

		if (!(bit_pos & bit_mask))
			continue;

		curr_ep = get_ep_by_pipe(udc, i);

		/* If the ep is configured */
		if (curr_ep->name == NULL) {
			WARNING("Invalid EP?");
			continue;
		}

		/* process the req queue until an uncomplete request */
		list_for_each_entry_safe(curr_req, temp_req, &curr_ep->queue,
				queue) {
			status = process_ep_req(udc, i, curr_req);

			VDBG("status of process_ep_req= %d, ep = %d",
					status, ep_num);
			if (status == REQ_UNCOMPLETE)
				break;
			/* write back status to req */
			curr_req->req.status = status;

			if (ep_num == 0) {
				ep0_req_complete(udc, curr_ep, curr_req);
				break;
			} else
				done(curr_ep, curr_req, status);
		}
	}
}

static inline enum usb_device_speed portscx_device_speed(u32 reg)
{
	switch (reg & PORTSCX_PORT_SPEED_MASK) {
	case PORTSCX_PORT_SPEED_HIGH:
		return USB_SPEED_HIGH;
	case PORTSCX_PORT_SPEED_FULL:
		return USB_SPEED_FULL;
	case PORTSCX_PORT_SPEED_LOW:
		return USB_SPEED_LOW;
	default:
		return USB_SPEED_UNKNOWN;
	}
}

/* Process a port change interrupt */
static void port_change_irq(struct mt_udc *udc)
{
	if (udc->bus_reset)
		udc->bus_reset = 0;

	/* Bus resetting is finished */
	if (!(mt_readl(&dr_regs->portsc1) & PORTSCX_PORT_RESET))
		/* Get the speed */
		udc->gadget.speed =
			portscx_device_speed(mt_readl(&dr_regs->portsc1));

	/* Update USB state */
	if (!udc->resume_state)
		udc->usb_state = USB_STATE_DEFAULT;
}

/* Process suspend interrupt */
static void suspend_irq(struct mt_udc *udc)
{
	udc->resume_state = udc->usb_state;
	udc->usb_state = USB_STATE_SUSPENDED;

	/* report suspend to the driver, serial.c does not support this */
	if (udc->driver->suspend)
		udc->driver->suspend(&udc->gadget);
}

static void bus_resume(struct mt_udc *udc)
{
	udc->usb_state = udc->resume_state;
	udc->resume_state = 0;

	/* report resume to the driver, serial.c does not support this */
	if (udc->driver->resume)
		udc->driver->resume(&udc->gadget);
}

/* Clear up all ep queues */
static int reset_queues(struct mt_udc *udc, bool bus_reset)
{
	u8 pipe;

	for (pipe = 0; pipe < udc->max_pipes; pipe++)
		udc_reset_ep_queue(udc, pipe);

	/* report disconnect; the driver is already quiesced */
	spin_unlock(&udc->lock);
	if (bus_reset)
		usb_gadget_udc_reset(&udc->gadget, udc->driver);
	else
		udc->driver->disconnect(&udc->gadget);
	spin_lock(&udc->lock);

	return 0;
}

/* Process reset interrupt */
static void reset_irq(struct mt_udc *udc)
{
	u32 temp;
	unsigned long timeout;

	/* Clear the device address */
	temp = mt_readl(&dr_regs->deviceaddr);
	mt_writel(temp & ~USB_DEVICE_ADDRESS_MASK, &dr_regs->deviceaddr);

	udc->device_address = 0;

	/* Clear usb state */
	udc->resume_state = 0;
	udc->ep0_dir = 0;
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->remote_wakeup = 0;	/* default to 0 on reset */
	udc->gadget.b_hnp_enable = 0;
	udc->gadget.a_hnp_support = 0;
	udc->gadget.a_alt_hnp_support = 0;

	/* Clear all the setup token semaphores */
	temp = mt_readl(&dr_regs->endptsetupstat);
	mt_writel(temp, &dr_regs->endptsetupstat);

	/* Clear all the endpoint complete status bits */
	temp = mt_readl(&dr_regs->endptcomplete);
	mt_writel(temp, &dr_regs->endptcomplete);

	timeout = jiffies + 100;
	while (mt_readl(&dr_regs->endpointprime)) {
		/* Wait until all endptprime bits cleared */
		if (time_after(jiffies, timeout)) {
			ERR("Timeout for reset\n");
			break;
		}
		cpu_relax();
	}

	/* Write 1s to the flush register */
	mt_writel(0xffffffff, &dr_regs->endptflush);

	if (mt_readl(&dr_regs->portsc1) & PORTSCX_PORT_RESET) {
		VDBG("Bus reset");
		/* Bus is reseting */
		udc->bus_reset = 1;
		/* Reset all the queues, include XD, dTD, EP queue
		 * head and TR Queue */
		reset_queues(udc, true);
		udc->usb_state = USB_STATE_DEFAULT;
	} else {
		VDBG("Controller reset");
		/* initialize usb hw reg except for regs for EP, not
		 * touch usbintr reg */
		dr_controller_setup(udc);

		/* Reset all internal used Queues */
		reset_queues(udc, false);

		ep0_setup(udc);

		/* Enable DR IRQ reg, Set Run bit, change udc state */
		dr_controller_run(udc);
		udc->usb_state = USB_STATE_ATTACHED;
	}
}

/*
 * USB device controller interrupt handler
 */
static irqreturn_t mt_udc_irq(int irq, void *_udc)
{
	struct mt_udc *udc = _udc;
	u32 irq_src;
	irqreturn_t status = IRQ_NONE;
	unsigned long flags;

	/* Disable ISR for OTG host mode */
	if (udc->stopped)
		return IRQ_NONE;
	spin_lock_irqsave(&udc->lock, flags);
	irq_src = mt_readl(&dr_regs->usbsts) & mt_readl(&dr_regs->usbintr);
	/* Clear notification bits */
	mt_writel(irq_src, &dr_regs->usbsts);

	/* VDBG("irq_src [0x%8x]", irq_src); */

	/* Need to resume? */
	if (udc->usb_state == USB_STATE_SUSPENDED)
		if ((mt_readl(&dr_regs->portsc1) & PORTSCX_PORT_SUSPEND) == 0)
			bus_resume(udc);

	/* USB Interrupt */
	if (irq_src & USB_STS_INT) {
		VDBG("Packet int");
		/* Setup package, we only support ep0 as control ep */
		if (mt_readl(&dr_regs->endptsetupstat) & EP_SETUP_STATUS_EP0) {
			tripwire_handler(udc, 0,
					(u8 *) (&udc->local_setup_buff));
			setup_received_irq(udc, &udc->local_setup_buff);
			status = IRQ_HANDLED;
		}

		/* completion of dtd */
		if (mt_readl(&dr_regs->endptcomplete)) {
			dtd_complete_irq(udc);
			status = IRQ_HANDLED;
		}
	}

	/* SOF (for ISO transfer) */
	if (irq_src & USB_STS_SOF) {
		status = IRQ_HANDLED;
	}

	/* Port Change */
	if (irq_src & USB_STS_PORT_CHANGE) {
		port_change_irq(udc);
		status = IRQ_HANDLED;
	}

	/* Reset Received */
	if (irq_src & USB_STS_RESET) {
		VDBG("reset int");
		reset_irq(udc);
		status = IRQ_HANDLED;
	}

	/* Sleep Enable (Suspend) */
	if (irq_src & USB_STS_SUSPEND) {
		suspend_irq(udc);
		status = IRQ_HANDLED;
	}

	if (irq_src & (USB_STS_ERR | USB_STS_SYS_ERR)) {
		VDBG("Error IRQ %x\n", irq_src);
	}

	spin_unlock_irqrestore(&udc->lock, flags);
	return status;
}

/*----------------------------------------------------------------*
 * Hook to gadget drivers
 * Called by initialization code of gadget drivers
*----------------------------------------------------------------*/
static int mt_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	int retval = 0;
	unsigned long flags = 0;

	/* lock is needed but whether should use this lock or another */
	spin_lock_irqsave(&udc_controller->lock, flags);

	driver->driver.bus = NULL;
	/* hook up the driver */
	udc_controller->driver = driver;
	spin_unlock_irqrestore(&udc_controller->lock, flags);
	g->is_selfpowered = 1;

	if (!IS_ERR_OR_NULL(udc_controller->transceiver)) {
		/* Suspend the controller until OTG enable it */
		udc_controller->stopped = 1;
		printk(KERN_INFO "Suspend udc for OTG auto detect\n");

		/* connect to bus through transceiver */
		if (!IS_ERR_OR_NULL(udc_controller->transceiver)) {
			retval = otg_set_peripheral(
					udc_controller->transceiver->otg,
						    &udc_controller->gadget);
			if (retval < 0) {
				ERR("can't bind to transceiver\n");
				udc_controller->driver = NULL;
				return retval;
			}
		}
	} else {
		/* Enable DR IRQ reg and set USBCMD reg Run bit */
		dr_controller_run(udc_controller);
		udc_controller->usb_state = USB_STATE_ATTACHED;
		udc_controller->ep0_state = WAIT_FOR_SETUP;
		udc_controller->ep0_dir = 0;
	}

	return retval;
}

/* Disconnect from gadget driver */
static int mt_udc_stop(struct usb_gadget *g)
{
	struct mt_ep *loop_ep;
	unsigned long flags;

	if (!IS_ERR_OR_NULL(udc_controller->transceiver))
		otg_set_peripheral(udc_controller->transceiver->otg, NULL);

	/* stop DR, disable intr */
	dr_controller_stop(udc_controller);

	/* in fact, no needed */
	udc_controller->usb_state = USB_STATE_ATTACHED;
	udc_controller->ep0_state = WAIT_FOR_SETUP;
	udc_controller->ep0_dir = 0;

	/* stand operation */
	spin_lock_irqsave(&udc_controller->lock, flags);
	udc_controller->gadget.speed = USB_SPEED_UNKNOWN;
	nuke(&udc_controller->eps[0], -ESHUTDOWN);
	list_for_each_entry(loop_ep, &udc_controller->gadget.ep_list,
			ep.ep_list)
		nuke(loop_ep, -ESHUTDOWN);
	spin_unlock_irqrestore(&udc_controller->lock, flags);

	udc_controller->driver = NULL;

	return 0;
}

/*-------------------------------------------------------------------------
		PROC File System Support
-------------------------------------------------------------------------*/
#ifdef CONFIG_USB_GADGET_DEBUG_FILES

#include <linux/seq_file.h>

static const char proc_filename[] = "driver/mt_udc";

static int mt_proc_read(struct seq_file *m, void *v)
{
	unsigned long flags;
	int i;
	u32 tmp_reg;
	struct mt_ep *ep = NULL;
	struct mt_req *req;

	struct mt_udc *udc = udc_controller;

	spin_lock_irqsave(&udc->lock, flags);

	/* ------basic driver information ---- */
	seq_printf(m,
			DRIVER_DESC "\n"
			"%s version: %s\n"
			"Gadget driver: %s\n\n",
			driver_name, DRIVER_VERSION,
			udc->driver ? udc->driver->driver.name : "(none)");

	/* ------ DR Registers ----- */
	tmp_reg = mt_readl(&dr_regs->usbcmd);
	seq_printf(m,
			"USBCMD reg:\n"
			"SetupTW: %d\n"
			"Run/Stop: %s\n\n",
			(tmp_reg & USB_CMD_SUTW) ? 1 : 0,
			(tmp_reg & USB_CMD_RUN_STOP) ? "Run" : "Stop");

	tmp_reg = mt_readl(&dr_regs->usbsts);
	seq_printf(m,
			"USB Status Reg:\n"
			"Dr Suspend: %d Reset Received: %d System Error: %s "
			"USB Error Interrupt: %s\n\n",
			(tmp_reg & USB_STS_SUSPEND) ? 1 : 0,
			(tmp_reg & USB_STS_RESET) ? 1 : 0,
			(tmp_reg & USB_STS_SYS_ERR) ? "Err" : "Normal",
			(tmp_reg & USB_STS_ERR) ? "Err detected" : "No err");

	tmp_reg = mt_readl(&dr_regs->usbintr);
	seq_printf(m,
			"USB Interrupt Enable Reg:\n"
			"Sleep Enable: %d SOF Received Enable: %d "
			"Reset Enable: %d\n"
			"System Error Enable: %d "
			"Port Change Dectected Enable: %d\n"
			"USB Error Intr Enable: %d USB Intr Enable: %d\n\n",
			(tmp_reg & USB_INTR_DEVICE_SUSPEND) ? 1 : 0,
			(tmp_reg & USB_INTR_SOF_EN) ? 1 : 0,
			(tmp_reg & USB_INTR_RESET_EN) ? 1 : 0,
			(tmp_reg & USB_INTR_SYS_ERR_EN) ? 1 : 0,
			(tmp_reg & USB_INTR_PTC_DETECT_EN) ? 1 : 0,
			(tmp_reg & USB_INTR_ERR_INT_EN) ? 1 : 0,
			(tmp_reg & USB_INTR_INT_EN) ? 1 : 0);

	tmp_reg = mt_readl(&dr_regs->frindex);
	seq_printf(m,
			"USB Frame Index Reg: Frame Number is 0x%x\n\n",
			(tmp_reg & USB_FRINDEX_MASKS));

	tmp_reg = mt_readl(&dr_regs->deviceaddr);
	seq_printf(m,
			"USB Device Address Reg: Device Addr is 0x%x\n\n",
			(tmp_reg & USB_DEVICE_ADDRESS_MASK));

	tmp_reg = mt_readl(&dr_regs->endpointlistaddr);
	seq_printf(m,
			"USB Endpoint List Address Reg: "
			"Device Addr is 0x%x\n\n",
			(tmp_reg & USB_EP_LIST_ADDRESS_MASK));

	tmp_reg = mt_readl(&dr_regs->portsc1);
	seq_printf(m,
		"USB Port Status&Control Reg:\n"
		"Port Transceiver Type : %s Port Speed: %s\n"
		"PHY Low Power Suspend: %s Port Reset: %s "
		"Port Suspend Mode: %s\n"
		"Over-current Change: %s "
		"Port Enable/Disable Change: %s\n"
		"Port Enabled/Disabled: %s "
		"Current Connect Status: %s\n\n", ( {
			const char *s;
			switch (tmp_reg & PORTSCX_PTS_FSLS) {
			case PORTSCX_PTS_UTMI:
				s = "UTMI"; break;
			case PORTSCX_PTS_ULPI:
				s = "ULPI "; break;
			case PORTSCX_PTS_FSLS:
				s = "FS/LS Serial"; break;
			default:
				s = "None"; break;
			}
			s;} ),
		usb_speed_string(portscx_device_speed(tmp_reg)),
		(tmp_reg & PORTSCX_PHY_LOW_POWER_SPD) ?
		"Normal PHY mode" : "Low power mode",
		(tmp_reg & PORTSCX_PORT_RESET) ? "In Reset" :
		"Not in Reset",
		(tmp_reg & PORTSCX_PORT_SUSPEND) ? "In " : "Not in",
		(tmp_reg & PORTSCX_OVER_CURRENT_CHG) ? "Dected" :
		"No",
		(tmp_reg & PORTSCX_PORT_EN_DIS_CHANGE) ? "Disable" :
		"Not change",
		(tmp_reg & PORTSCX_PORT_ENABLE) ? "Enable" :
		"Not correct",
		(tmp_reg & PORTSCX_CURRENT_CONNECT_STATUS) ?
		"Attached" : "Not-Att");

	tmp_reg = mt_readl(&dr_regs->usbmode);
	seq_printf(m,
			"USB Mode Reg: Controller Mode is: %s\n\n", ( {
				const char *s;
				switch (tmp_reg & USB_MODE_CTRL_MODE_HOST) {
				case USB_MODE_CTRL_MODE_IDLE:
					s = "Idle"; break;
				case USB_MODE_CTRL_MODE_DEVICE:
					s = "Device Controller"; break;
				case USB_MODE_CTRL_MODE_HOST:
					s = "Host Controller"; break;
				default:
					s = "None"; break;
				}
				s;
			} ));

	tmp_reg = mt_readl(&dr_regs->endptsetupstat);
	seq_printf(m,
			"Endpoint Setup Status Reg: SETUP on ep 0x%x\n\n",
			(tmp_reg & EP_SETUP_STATUS_MASK));

	for (i = 0; i < udc->max_ep / 2; i++) {
		tmp_reg = mt_readl(&dr_regs->endptctrl[i]);
		seq_printf(m, "EP Ctrl Reg [0x%x]: = [0x%x]\n", i, tmp_reg);
	}
	tmp_reg = mt_readl(&dr_regs->endpointprime);
	seq_printf(m, "EP Prime Reg = [0x%x]\n\n", tmp_reg);

#if 0
	if (udc->pdata->have_sysif_regs) {
		tmp_reg = usb_sys_regs->snoop1;
		seq_printf(m, "Snoop1 Reg : = [0x%x]\n\n", tmp_reg);

		tmp_reg = usb_sys_regs->control;
		seq_printf(m, "General Control Reg : = [0x%x]\n\n", tmp_reg);
	}
#endif

	/* ------mt_udc, mt_ep, mt_request structure information ----- */
	ep = &udc->eps[0];
	seq_printf(m, "For %s Maxpkt is 0x%x index is 0x%x\n",
			ep->ep.name, ep_maxpacket(ep), ep_index(ep));

	if (list_empty(&ep->queue)) {
		seq_puts(m, "its req queue is empty\n\n");
	} else {
		list_for_each_entry(req, &ep->queue, queue) {
			seq_printf(m,
				"req %p actual 0x%x length 0x%x buf %p\n",
				&req->req, req->req.actual,
				req->req.length, req->req.buf);
		}
	}
	/* other gadget->eplist ep */
	list_for_each_entry(ep, &udc->gadget.ep_list, ep.ep_list) {
		if (ep->ep.desc) {
			seq_printf(m,
					"\nFor %s Maxpkt is 0x%x "
					"index is 0x%x\n",
					ep->ep.name, ep_maxpacket(ep),
					ep_index(ep));

			if (list_empty(&ep->queue)) {
				seq_puts(m, "its req queue is empty\n\n");
			} else {
				list_for_each_entry(req, &ep->queue, queue) {
					seq_printf(m,
						"req %p actual 0x%x length "
						"0x%x  buf %p\n",
						&req->req, req->req.actual,
						req->req.length, req->req.buf);
				}	/* end for each_entry of ep req */
			}	/* end for else */
		}	/* end for if(ep->queue) */
	}	/* end (ep->desc) */

	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

/*
 * seq_file wrappers for procfile show routines.
 */
static int mt_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_proc_read, NULL);
}

static const struct file_operations mt_proc_fops = {
	.open		= mt_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#define create_proc_file()	proc_create(proc_filename, 0, NULL, &mt_proc_fops)
#define remove_proc_file()	remove_proc_entry(proc_filename, NULL)

#else				/* !CONFIG_USB_GADGET_DEBUG_FILES */

#define create_proc_file()	do {} while (0)
#define remove_proc_file()	do {} while (0)

#endif				/* CONFIG_USB_GADGET_DEBUG_FILES */

/*-------------------------------------------------------------------------*/

/* Release udc structures */
static void mt_udc_release(struct device *dev)
{
	complete(udc_controller->done);
	dma_free_coherent(dev->parent, udc_controller->ep_qh_size,
			udc_controller->ep_qh, udc_controller->ep_qh_dma);
	kfree(udc_controller);
}

/******************************************************************
	Internal structure setup functions
*******************************************************************/
/*------------------------------------------------------------------
 * init resource for globle controller
 * Return the udc handle on success or NULL on failure
 ------------------------------------------------------------------*/
#if defined(EP0_BUF_32BYTES_CACHELINE_ALIGN)
u8 ep0_buf[32] __attribute__((aligned(32)));
#endif
static int struct_udc_setup(struct mt_udc *udc,
		struct platform_device *pdev)
{
	//struct mt_usb2_platform_data *pdata;
	size_t size;

	//pdata = dev_get_platdata(&pdev->dev);
	udc->phy_mode = MT_USB2_PHY_UTMI;//pdata->phy_mode;

	udc->eps = kzalloc(sizeof(struct mt_ep) * udc->max_ep, GFP_KERNEL);
	if (!udc->eps)
		return -1;

	/* initialized QHs, take care of alignment */
	size = udc->max_ep * sizeof(struct ep_queue_head);
	if (size < QH_ALIGNMENT)
		size = QH_ALIGNMENT;
	else if ((size % QH_ALIGNMENT) != 0) {
		size += QH_ALIGNMENT + 1;
		size &= ~(QH_ALIGNMENT - 1);
	}
	udc->ep_qh = dma_alloc_coherent(&pdev->dev, size,
					&udc->ep_qh_dma, GFP_KERNEL);
	if (!udc->ep_qh) {
		ERR("malloc QHs for udc failed\n");
		kfree(udc->eps);
		return -1;
	}

	udc->ep_qh_size = size;

	/* Initialize ep0 status request structure */
	/* FIXME: mt_alloc_request() ignores ep argument */
	udc->status_req = container_of(mt_alloc_request(NULL, GFP_KERNEL),
			struct mt_req, req);
#if defined(EP0_BUF_32BYTES_CACHELINE_ALIGN)
	dma_cache_wback_inv((unsigned long) ep0_buf, 32);
	udc->status_req->req.buf = UNCACHED_ADDR(ep0_buf);
#else
	/* allocate a small amount of memory to get valid address */
	udc->status_req->req.buf = kmalloc(8, GFP_KERNEL);
#endif

#if defined(PRIME_STATUS_PATCH)
	udc->ch9getstatus_req = container_of(mt_alloc_request(NULL, GFP_KERNEL),
			struct mt_req, req);
	udc->ch9getstatus_req->req.buf = kmalloc(L1_CACHE_BYTES, GFP_KERNEL);
#endif

	udc->resume_state = USB_STATE_NOTATTACHED;
	udc->usb_state = USB_STATE_POWERED;
	udc->ep0_dir = 0;
	udc->remote_wakeup = 0;	/* default to 0 on reset */

	return 0;
}

/*----------------------------------------------------------------
 * Setup the mt_ep struct for eps
 * Link mt_ep->ep to gadget->ep_list
 * ep0out is not used so do nothing here
 * ep0in should be taken care
 *--------------------------------------------------------------*/
static int struct_ep_setup(struct mt_udc *udc, unsigned char index,
		char *name, int link)
{
	struct mt_ep *ep = &udc->eps[index];

	ep->udc = udc;
	strcpy(ep->name, name);
	ep->ep.name = ep->name;

	ep->ep.ops = &mt_ep_ops;
	ep->stopped = 0;

	if (index == 0) {
		ep->ep.caps.type_control = true;
	} else {
		ep->ep.caps.type_iso = true;
		ep->ep.caps.type_bulk = true;
		ep->ep.caps.type_int = true;
	}

	if (index & 1)
		ep->ep.caps.dir_in = true;
	else
		ep->ep.caps.dir_out = true;

	/* for ep0: maxP defined in desc
	 * for other eps, maxP is set by epautoconfig() called by gadget layer
	 */
	usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);

	/* the queue lists any req for this ep */
	INIT_LIST_HEAD(&ep->queue);

	/* gagdet.ep_list used for ep_autoconfig so no ep0 */
	if (link)
		list_add_tail(&ep->ep.ep_list, &udc->gadget.ep_list);
	ep->gadget = &udc->gadget;
	ep->qh = &udc->ep_qh[index];

	return 0;
}

/* Driver probe function
 * all intialization operations implemented here except enabling usb_intr reg
 * board setup should have been done in the platform code
 */
static int mt_udc_probe(struct platform_device *pdev)
{
	struct mt_usb2_platform_data *pdata;
	struct resource *res;
	int ret = -ENODEV;
	unsigned int i;
	
	udc_controller = kzalloc(sizeof(struct mt_udc), GFP_KERNEL);
	if (udc_controller == NULL)
		return -ENOMEM;

	pdata = dev_get_platdata(&pdev->dev);
	udc_controller->pdata = pdata;
	spin_lock_init(&udc_controller->lock);
	udc_controller->stopped = 1;


	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENXIO;
		goto err_kfree;
	}

        if (!request_mem_region(res->start, resource_size(res),
    					driver_name)) {
            ERR("request mem region for %s failed\n", pdev->name);
            ret = -EBUSY;
            goto err_kfree;
        }

	//dr_regs = ioremap(res->start, resource_size(res));
	dr_regs = (struct usb_dr_device __iomem *)res->start;
	if (!dr_regs) {
		ret = -ENOMEM;
		goto err_release_mem_region;
	}

	pdata->regs = (void __iomem *)dr_regs;

	/*
	 * do platform specific init: check the clock, grab/config pins, etc.
	 */
	if (pdata->init && pdata->init(pdev)) {
		ret = -ENODEV;
		goto err_iounmap_noclk;
	}


#if 0
	if (pdata->have_sysif_regs)
		usb_sys_regs = (void *)dr_regs + USB_DR_SYS_OFFSET;
			((u32)dr_regs + USB_DR_SYS_OFFSET);*/
#endif

	/* Initialize USB clocks */
	ret = mt_udc_clk_init(pdev);
	if (ret < 0)
		goto err_iounmap_noclk;

#if defined(USB_BURSTSIZE_56_BYTES)
	mt_writel( 0x0E0EUL, (void *)(res->start + 0x60));
#endif

	/* Read Device Controller Capability Parameters register */
	udc_controller->max_ep = USB_MAX_EP_NUM * 2;
	VDBG("max_ep:%d", udc_controller->max_ep);

	udc_controller->irq = platform_get_irq(pdev, 0);
	if (!udc_controller->irq) {
		ret = -ENODEV;
		goto err_iounmap;
	}

	VDBG("irq:%d", udc_controller->irq);
	
#ifdef CONFIG_PANTHER_USB_DUAL_ROLE
	ret = request_irq(udc_controller->irq, mt_udc_irq, IRQF_SHARED,
			driver_name, udc_controller);
#else
	ret = request_irq(udc_controller->irq, mt_udc_irq, 0,
			driver_name, udc_controller);
#endif
	if (ret != 0) {
		ERR("cannot request irq %d err %d\n",
				udc_controller->irq, ret);
		goto err_iounmap;
	}

	/* Initialize the udc structure including QH member and other member */
	if (struct_udc_setup(udc_controller, pdev)) {
		ERR("Can't initialize udc data structure\n");
		ret = -ENOMEM;
		goto err_free_irq;
	}

	/* initialize usb hw reg except for regs for EP,
	 * leave usbintr reg untouched */
	dr_controller_setup(udc_controller);

	ret = mt_udc_clk_finalize(pdev);
	if (ret)
		goto err_free_irq;

	/* Setup gadget structure */
	udc_controller->gadget.ops = &mt_gadget_ops;
	udc_controller->gadget.max_speed = USB_SPEED_HIGH;
	udc_controller->gadget.ep0 = &udc_controller->eps[0].ep;
	INIT_LIST_HEAD(&udc_controller->gadget.ep_list);
	udc_controller->gadget.speed = USB_SPEED_UNKNOWN;
	udc_controller->gadget.name = driver_name;

	/* Setup gadget.dev and register with kernel */
	dev_set_name(&udc_controller->gadget.dev, "gadget");
	udc_controller->gadget.dev.of_node = pdev->dev.of_node;


	/* setup QH and epctrl for ep0 */
	ep0_setup(udc_controller);

	/* setup udc->eps[] for ep0 */
	struct_ep_setup(udc_controller, 0, "ep0", 0);
	/* for ep0: the desc defined here;
	 * for other eps, gadget layer called ep_enable with defined desc
	 */
	udc_controller->eps[0].ep.desc = &mt_ep0_desc;
	usb_ep_set_maxpacket_limit(&udc_controller->eps[0].ep,
				   USB_MAX_CTRL_PAYLOAD);

	/* setup the udc->eps[] for non-control endpoints and link
	 * to gadget.ep_list */
	for (i = 1; i < (int)(udc_controller->max_ep / 2); i++) {
		char name[14];

		sprintf(name, "ep%dout", i);
		struct_ep_setup(udc_controller, i * 2, name, 1);
		sprintf(name, "ep%din", i);
		struct_ep_setup(udc_controller, i * 2 + 1, name, 1);
	}

	/* use dma_pool for TD management */
	udc_controller->td_pool = dma_pool_create("udc_td", &pdev->dev,
			sizeof(struct ep_td_struct),
			DTD_ALIGNMENT, UDC_DMA_BOUNDARY);
	if (udc_controller->td_pool == NULL) {
		ret = -ENOMEM;
		goto err_free_irq;
	}

	ret = usb_add_gadget_udc_release(&pdev->dev, &udc_controller->gadget,
			mt_udc_release);
	if (ret)
		goto err_del_udc;

	create_proc_file();
	return 0;

err_del_udc:
	dma_pool_destroy(udc_controller->td_pool);
err_free_irq:
	free_irq(udc_controller->irq, udc_controller);
err_iounmap:
	if (pdata->exit)
		pdata->exit(pdev);
	mt_udc_clk_release();
err_iounmap_noclk:
	//iounmap(dr_regs);
err_release_mem_region:
    release_mem_region(res->start, resource_size(res));
err_kfree:
	kfree(udc_controller);
	udc_controller = NULL;
	return ret;
}

/* Driver removal function
 * Free resources and finish pending transactions
 */
static int mt_udc_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct mt_usb2_platform_data *pdata = dev_get_platdata(&pdev->dev);

	DECLARE_COMPLETION_ONSTACK(done);

	if (!udc_controller)
		return -ENODEV;

	udc_controller->done = &done;
	usb_del_gadget_udc(&udc_controller->gadget);

	mt_udc_clk_release();

	/* DR has been stopped in usb_gadget_unregister_driver() */
	remove_proc_file();

#if !defined(EP0_BUF_32BYTES_CACHELINE_ALIGN)
	/* Free allocated memory */
	kfree(udc_controller->status_req->req.buf);
#endif
	kfree(udc_controller->status_req);
#if defined(PRIME_STATUS_PATCH)
	kfree(udc_controller->ch9getstatus_req->req.buf);
	kfree(udc_controller->ch9getstatus_req);
#endif
	kfree(udc_controller->eps);

	dma_pool_destroy(udc_controller->td_pool);
	free_irq(udc_controller->irq, udc_controller);
	iounmap(dr_regs);

		release_mem_region(res->start, resource_size(res));

	/* free udc --wait for the release() finished */
	wait_for_completion(&done);

	/*
	 * do platform specific un-initialization:
	 * release iomux pins, etc.
	 */
	if (pdata->exit)
		pdata->exit(pdev);

	return 0;
}

/*-----------------------------------------------------------------
 * Modify Power management attributes
 * Used by OTG statemachine to disable gadget temporarily
 -----------------------------------------------------------------*/
static int mt_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
	dr_controller_stop(udc_controller);
	return 0;
}

/*-----------------------------------------------------------------
 * Invoked on USB resume. May be called in_interrupt.
 * Here we start the DR controller and enable the irq
 *-----------------------------------------------------------------*/
static int mt_udc_resume(struct platform_device *pdev)
{
	/* Enable DR irq reg and set controller Run */
	if (udc_controller->stopped) {
		dr_controller_setup(udc_controller);
		dr_controller_run(udc_controller);
	}
	udc_controller->usb_state = USB_STATE_ATTACHED;
	udc_controller->ep0_state = WAIT_FOR_SETUP;
	udc_controller->ep0_dir = 0;
	return 0;
}

static int mt_udc_otg_suspend(struct device *dev, pm_message_t state)
{
	struct mt_udc *udc = udc_controller;
	u32 mode, usbcmd;

	mode = mt_readl(&dr_regs->usbmode) & USB_MODE_CTRL_MODE_MASK;

	pr_debug("%s(): mode 0x%x stopped %d\n", __func__, mode, udc->stopped);

	/*
	 * If the controller is already stopped, then this must be a
	 * PM suspend.  Remember this fact, so that we will leave the
	 * controller stopped at PM resume time.
	 */
	if (udc->stopped) {
		pr_debug("gadget already stopped, leaving early\n");
		udc->already_stopped = 1;
		return 0;
	}

	if (mode != USB_MODE_CTRL_MODE_DEVICE) {
		pr_debug("gadget not in device mode, leaving early\n");
		return 0;
	}

	/* stop the controller */
	usbcmd = mt_readl(&dr_regs->usbcmd) & ~USB_CMD_RUN_STOP;
	mt_writel(usbcmd, &dr_regs->usbcmd);

	udc->stopped = 1;

	pr_info("USB Gadget suspended\n");

	return 0;
}

static int mt_udc_otg_resume(struct device *dev)
{
	pr_debug("%s(): stopped %d  already_stopped %d\n", __func__,
		 udc_controller->stopped, udc_controller->already_stopped);

	/*
	 * If the controller was stopped at suspend time, then
	 * don't resume it now.
	 */
	if (udc_controller->already_stopped) {
		udc_controller->already_stopped = 0;
		pr_debug("gadget was already stopped, leaving early\n");
		return 0;
	}

	pr_info("USB Gadget resume\n");

	return mt_udc_resume(NULL);
}
/*-------------------------------------------------------------------------
	Register entry point for the peripheral controller driver
--------------------------------------------------------------------------*/
static const struct platform_device_id mt_udc_devtype[] = {
	{
		.name = "mt-udc",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, mt_udc_devtype);
static struct platform_driver udc_driver = {
	.remove		= mt_udc_remove,
	.id_table	= mt_udc_devtype,
	/* these suspend and resume are not usb suspend and resume */
	.suspend = mt_udc_suspend,
	.resume  = mt_udc_resume,
	.driver  = {
		.name = (char *)driver_name,
        /* udc suspend/resume called from OTG driver */
        .suspend = mt_udc_otg_suspend,
        .resume  = mt_udc_otg_resume,
	},
};

module_platform_driver_probe(udc_driver, mt_udc_probe);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt-udc");
