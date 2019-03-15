/*
 * Montage EHCI HCD (Host Controller Driver) for USB.
 */

#include <linux/platform_device.h>
#include "ehci-mt.h"

#ifdef MT_DEBUG
#include <linux/list.h>
#include <linux/usb.h>
#include "../core/hcd.h"
#include "ehci.h"
#endif

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>
#include <asm/mach-panther/reg.h>

extern int usb_disabled(void);

#define USB_BURSTSIZE_56_BYTES

#if !defined(USB_BURSTSIZE_56_BYTES)

#define MT_USB_DMA_ALIGN 4

struct dma_aligned_buffer {
	void *kmalloc_ptr;
	void *old_xfer_buffer;
	u8 data[0];
};

static void free_dma_aligned_buffer(struct urb *urb)
{
	struct dma_aligned_buffer *temp;
	size_t length;

	if (!(urb->transfer_flags & URB_ALIGNED_TEMP_BUFFER))
		return;

	temp = container_of(urb->transfer_buffer,
		struct dma_aligned_buffer, data);

	if (usb_urb_dir_in(urb)) {
		if (usb_pipeisoc(urb->pipe))
			length = urb->transfer_buffer_length;
		else
			length = urb->actual_length;

		if(temp->old_xfer_buffer && length)
		{
			memcpy(temp->old_xfer_buffer, temp->data, length);
			//dma_cache_wback_inv((unsigned long)temp->old_xfer_buffer, length);
		}
	}
	urb->transfer_buffer = temp->old_xfer_buffer;
	kfree(temp->kmalloc_ptr);

	urb->transfer_flags &= ~URB_ALIGNED_TEMP_BUFFER;
}

static int alloc_dma_aligned_buffer(struct urb *urb, gfp_t mem_flags)
{
	struct dma_aligned_buffer *temp, *kmalloc_ptr;
	size_t kmalloc_size;

	if (urb->num_sgs || urb->sg ||
	    urb->transfer_buffer_length == 0 ||
	    !((uintptr_t)urb->transfer_buffer & (MT_USB_DMA_ALIGN - 1)))
		return 0;

	/* Allocate a buffer with enough padding for alignment */
	kmalloc_size = urb->transfer_buffer_length +
		sizeof(struct dma_aligned_buffer) + L1_CACHE_BYTES - 1;

	kmalloc_ptr = kmalloc(kmalloc_size, mem_flags);
	if (!kmalloc_ptr)
		return -ENOMEM;

	/* Position our struct dma_aligned_buffer such that data is aligned */
	temp = PTR_ALIGN(kmalloc_ptr + 1, L1_CACHE_BYTES) - 1;
	temp->kmalloc_ptr = kmalloc_ptr;
	temp->old_xfer_buffer = urb->transfer_buffer;
	if (usb_urb_dir_out(urb))
		memcpy(temp->data, urb->transfer_buffer,
		       urb->transfer_buffer_length);
	urb->transfer_buffer = temp->data;

	urb->transfer_flags |= URB_ALIGNED_TEMP_BUFFER;

	return 0;
}

static int mt_ehci_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
				      gfp_t mem_flags)
{
	int ret;

	ret = alloc_dma_aligned_buffer(urb, mem_flags);
	if (ret)
		return ret;

	ret = usb_hcd_map_urb_for_dma(hcd, urb, mem_flags);
	if (ret)
		free_dma_aligned_buffer(urb);

	return ret;
}

static void mt_ehci_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	usb_hcd_unmap_urb_for_dma(hcd, urb);
	free_dma_aligned_buffer(urb);
}

#endif

static int ehci_hcd_mt_init(struct usb_hcd *hcd)
{
	int retval = 0;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	ehci->caps = (struct ehci_caps __iomem *)(hcd->regs + 0x20);
	ehci->regs = (struct ehci_regs __iomem *)(hcd->regs + 0x40);
	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	hcd->has_tt = 1;
	ehci_reset(ehci);

#if defined(USB_BURSTSIZE_56_BYTES)
    REG_UPDATE32( (hcd->regs + 0x60), 0x0E0E, 0xFFFFUL);
#endif

	retval = ehci_init(hcd);
	if(retval)
		return retval;

	//ehci_port_power(ehci, 0);

	return retval;
}

static const struct hc_driver ehci_mt_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "MT EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2 | HCD_BH,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ehci_hcd_mt_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,

#if !defined(USB_BURSTSIZE_56_BYTES)
	.map_urb_for_dma		= mt_ehci_map_urb_for_dma,
	.unmap_urb_for_dma		= mt_ehci_unmap_urb_for_dma,
#endif
};

#ifdef MT_DEBUG
void ehci_hcd_mt_dump(void)
{
	printk("hc_capbase[0x%x]\n", mt_readl(USB_HST_CAPABILITY_REG));
	printk("hcs_params[0x%x]\n", mt_readl(USB_HCSPARAMS_REG));
	printk("hcc_params[0x%x]\n", mt_readl(USB_HCCPARAMS_REG));

	printk("command[0x%x]\n", mt_readl(USB_USBCMD_REG));
	printk("status[0x%x]\n", mt_readl(USB_USBSTS_REG));
	printk("intr_enable[0x%x]\n", mt_readl(USB_USBINTR_REG));
	printk("frame_index[0x%x]\n", mt_readl(USB_FRINDEX_REG));
	printk("segment[0x%x]\n", mt_readl(USB_FRINDEX_REG+4));
	printk("frame_list[0x%x]\n", mt_readl(USB_PERIODICLISTBASE_REG));
	printk("async_next[0x%x]\n", mt_readl(USB_ASYNCLISTADDR_REG));
	printk("configured_flag[0x%x]\n", mt_readl(USB_CONFIG_REG));
	printk("port_status[0x%x]\n", mt_readl(USB_PORTSCx_REG));
}

void ehci_async_dump(struct ehci_qh *qh_start)
{
	u32 i = 0;
	struct list_head	*entry, *tmp;
	struct ehci_qtd	*qtd;

	printk("qh[%d]: 0x%x\n", i, (u32)qh_start);
	printk("  hw_next[0x%x]\n", qh_start[i].hw_next);
	printk("  hw_info1[0x%x]\n", qh_start[i].hw_info1);
	printk("  hw_info2[0x%x]\n", qh_start[i].hw_info2);
	printk("  hw_current[0x%x]\n", qh_start[i].hw_current);
	printk("  hw_qtd_next[0x%x]\n", qh_start[i].hw_qtd_next);
	printk("  hw_alt_next[0x%x]\n", qh_start[i].hw_alt_next);
	printk("  hw_token[0x%x]\n", qh_start[i].hw_token);
	printk("  hw_buf[0x%x]\n", qh_start[i].hw_buf[0]);

	list_for_each_safe (entry, tmp, &qh_start->qtd_list)
	{
		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		printk("qtd[%d]: 0x%x\n", i, (u32)qtd);
		printk("  hw_next[0x%x]\n", qtd->hw_next);
		printk("  hw_alt_next[0x%x]\n", qtd->hw_alt_next);
		printk("  hw_token[0x%x]\n", qtd->hw_token);
		printk("  hw_buf[0x%x]\n", qtd->hw_buf[0]);
		i += 1;
	}
}
#endif

static int ehci_hcd_mt_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct resource *res;
	int irq;
	int retval;

	//printk("enter ehci_hcd_mt_drv_probe\n");
	if (POWERCTL_STATIC_OFF==pmu_get_usb_powercfg())
		return -ENODEV;

	if (usb_disabled())
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev,
				"Found HC with no IRQ. Check %s setup!\n",
				dev_name(&pdev->dev));
		return -ENODEV;
	}
	irq = res->start;
	hcd = usb_create_hcd(&ehci_mt_hc_driver, &pdev->dev, "mt-ehci");
	if (!hcd) {
		retval = -ENOMEM;
		goto fail_create_hcd;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
				"Found HC with no register addr. Check %s setup!\n",
				dev_name(&pdev->dev));
		retval = -ENODEV;
		goto fail_request_resource;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = res->end - res->start + 1;
	printk("hcd register start[0x%x], end[0x%x], size[0x%x]\n",
	  (u32)hcd->rsrc_start, pdev->resource[0].end, (u32)hcd->rsrc_len);

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		retval = -EBUSY;
		goto fail_request_resource;
	}

	hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		retval = -EFAULT;
		goto fail_ioremap;
	}

#ifdef CONFIG_PANTHER_USB_DUAL_ROLE
	retval = usb_add_hcd(hcd, irq, IRQF_SHARED);
#else
	retval = usb_add_hcd(hcd, irq, 0);
#endif
	if (retval)
		goto fail_add_hcd;

	return retval;

fail_add_hcd:
	iounmap(hcd->regs);
fail_ioremap:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
fail_request_resource:
	usb_put_hcd(hcd);
fail_create_hcd:
	dev_err(&pdev->dev, "init %s fail, %d\n", dev_name(&pdev->dev), retval);

	return retval;
}

static int ehci_hcd_mt_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);

	return 0;
}

#ifdef CONFIG_PM
static int ehci_hcd_mt_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc;

	printk("*******ehci_hcd_mt_drv_suspend!!!!!!*******\n");
	rc = 0;

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW) {
		ehci_halt(ehci);
		ehci_reset(ehci);
	}

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	// could save FLADJ in case of Vmtx power loss
	// ... we'd only use it to handle clock skew

	return rc;
}


static int ehci_hcd_mt_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	// maybe restore FLADJ

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	/* If CF is still set, we maintained PCI Vmtx power.
	 * Just undo the effect of ehci_pci_suspend().
	 */
	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	//if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	//ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}

#else
#define ehci_hcd_mt_drv_suspend NULL
#define ehci_hcd_mt_drv_resume NULL
#endif

static struct platform_driver ehci_hcd_mt_driver = {
	.probe		= ehci_hcd_mt_drv_probe,
	.remove		= ehci_hcd_mt_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_mt_drv_suspend,
	.resume		= ehci_hcd_mt_drv_resume,
	.driver = {
		.name	= "mt-ehci",
		.owner	= THIS_MODULE,
	}
};

MODULE_ALIAS("platform:mt-ehci");
