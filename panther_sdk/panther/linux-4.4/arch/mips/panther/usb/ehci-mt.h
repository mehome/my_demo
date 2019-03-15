#ifndef _MONTAGE_EHCI_H_
#define _MONTAGE_EHCI_H_

/************** USB EHCI ******************/
#ifdef CONFIG_PANTHER
#include <asm/mach-panther/panther.h>
#endif
#define USB_EHCI_MT_REG_BASE		USB_BASE

#define USB_ID_REG			(USB_EHCI_MT_REG_BASE+0x00)
#define USB_HWGENERAL_REG		(USB_EHCI_MT_REG_BASE+0x04)
#define USB_HWHOST_REG			(USB_EHCI_MT_REG_BASE+0x08)
#define USB_HWTXBUF_REG			(USB_EHCI_MT_REG_BASE+0x10)
#define USB_HWRXBUF_REG			(USB_EHCI_MT_REG_BASE+0x14)

#define USB_GPTIMER0LD_REG		(USB_EHCI_MT_REG_BASE+0x180)
#define USB_GPTIMER0CTRL_REG		(USB_EHCI_MT_REG_BASE+0x184)
#define USB_GPTIMER1LD_REG		(USB_EHCI_MT_REG_BASE+0x188)
#define USB_GPTIMER1CTRL_REG		(USB_EHCI_MT_REG_BASE+0x18C)

#define USB_HST_CAPABILITY_REG		(USB_EHCI_MT_REG_BASE+0x20)
#define USB_HCSPARAMS_REG		(USB_HST_CAPABILITY_REG+0x04)
#define USB_HCCPARAMS_REG		(USB_HST_CAPABILITY_REG+0x08)

#define USB_USBCMD_REG			(USB_EHCI_MT_REG_BASE+0x40)
#define USB_USBSTS_REG			(USB_USBCMD_REG+0x04)
#define USB_USBINTR_REG			(USB_USBCMD_REG+0x08)
#define USB_FRINDEX_REG			(USB_USBCMD_REG+0x0C)
#define USB_PERIODICLISTBASE_REG	(USB_USBCMD_REG+0x14)
#define USB_ASYNCLISTADDR_REG		(USB_USBCMD_REG+0x18)
#define USB_BURSTSIZE_REG		(USB_USBCMD_REG+0x20)
#define USB_TXFILLTUNING_REG		(USB_USBCMD_REG+0x24)
#define USB_CONFIG_REG			(USB_USBCMD_REG+0x40)
#define USB_PORTSCx_REG			(USB_USBCMD_REG+0x44)

#define USB_USBMODE_REG			(USB_USBCMD_REG+0x68)

#ifdef MT_DEBUG
#include <linux/delay.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/irq.h>

/* cpu pipeline flush */
void static inline mt_sync(void)
{
	__asm__ volatile ("sync");
}

void static inline mt_sync_udelay(int us)
{
	__asm__ volatile ("sync");
	udelay(us);
}

void static inline mt_sync_delay(int ms)
{
	__asm__ volatile ("sync");
	mdelay(ms);
}

void static inline mt_writeb(u8 val, unsigned long reg)
{
	*(volatile u8 *)reg = val;
	*(volatile u8 *)reg = val;
}

void static inline mt_writew(u16 val, unsigned long reg)
{
	*(volatile u16 *)reg = val;
	*(volatile u16 *)reg = val;
}

void static inline mt_writel(u32 val, unsigned long reg)
{
	*(volatile u32 *)reg = val;
//	*(volatile u32 *)reg = val;
}

static inline u8 mt_readb(unsigned long reg)
{
	u8 tmp;

	tmp = *(volatile u8 *)reg;
	tmp = *(volatile u8 *)reg;
	return tmp;
}

static inline u16 mt_readw(unsigned long reg)
{
	u16 tmp;

	tmp = *(volatile u16 *)reg;
	tmp = *(volatile u16 *)reg;
	return tmp;
}

static inline u32 mt_readl(unsigned long reg)
{
	u32 tmp;

	tmp = *(volatile u32 *)reg;
	tmp = *(volatile u32 *)reg;
	return tmp;
}
#endif

#endif //#ifndef _MONTAGE_EHCI_H_
