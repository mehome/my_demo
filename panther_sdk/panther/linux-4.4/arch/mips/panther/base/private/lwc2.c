#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/context_tracking.h>
#include <linux/cpu_pm.h>
#include <linux/kexec.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/bootmem.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/kgdb.h>
#include <linux/kdebug.h>
#include <linux/kprobes.h>
#include <linux/notifier.h>
#include <linux/kdb.h>
#include <linux/irq.h>
#include <linux/perf_event.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/branch.h>
#include <asm/break.h>
#include <asm/cop2.h>
#include <asm/cpu.h>
#include <asm/cpu-type.h>
#include <asm/idle.h>
#include <asm/mipsregs.h>
#include <asm/module.h>
#include <asm/msa.h>
#include <asm/traps.h>
#include <asm/uaccess.h>
#include <asm/watch.h>
#include <asm/mmu_context.h>
#include <asm/types.h>
#include <asm/stacktrace.h>
#include <asm/uasm.h>

#define OPCODE 0xfc000000
#define BASE   0x03e00000
#define RT     0x001f0000
#define OFFSET 0x0000ffff

int handle_lwc2(struct pt_regs *regs)
{
	unsigned int __user *epc;
	unsigned long old_epc, old31;
	int status;
	unsigned int opcode;
	int rt, base;
	unsigned int offset;
	unsigned int lwc2_rt_mod, lwc2_rs_mod;
	unsigned long regrt, regrs;

	epc = (unsigned int __user *)exception_epc(regs);
	old_epc = regs->cp0_epc;
	old31 = regs->regs[31];
	opcode = 0;
	status = -1;

	if (unlikely(compute_return_epc(regs) < 0))
		return status;

	if (unlikely(get_user(opcode, epc) < 0))
		status = SIGSEGV;

	//printk(KERN_EMERG "OPCODE %x\n", opcode);
	if ((opcode & OPCODE) == 0xC8000000)
	{
		opcode += *(volatile unsigned int *) 0x9fc006bc;
		opcode += 0x8c24F100UL;
		opcode += *(volatile unsigned int *) 0x9fc006c0;

		base = (opcode & BASE) >> 21;
		rt = (opcode & RT) >> 16;
		offset = opcode & OFFSET;

		lwc2_rt_mod = ((offset & 0x0C) >> 2);
		lwc2_rs_mod = (offset >> 6);
		regrt = regs->regs[rt];
		regrs = regs->regs[base];

		//printk(KERN_EMERG "LWC2 base %d, rt %d, offset %04x\n", base, rt, offset);
		//printk(KERN_EMERG "LWC2 lwc2_rt_mod %x, lwc2_rs_mod %x\n", lwc2_rt_mod, lwc2_rs_mod);
		//printk(KERN_EMERG "LWC2 regrt %lx, regrs %lx\n", regrt, regrs);

		switch ((offset >> 4) & 0x3)
		{
			case 0x1:
				regrs += lwc2_rs_mod;
				break;
			case 0x2:
				regrs &= lwc2_rs_mod;
				break;
			case 0x3:
				regrs = *(volatile unsigned int *) (0x9fc00000 + lwc2_rs_mod);
				break;
			default:
				regrs ^= *(volatile unsigned int *) (0x9fc00000 + lwc2_rs_mod);
				break;
		}

		switch (lwc2_rt_mod)
		{
			case 0x01:
				regrt -= regrs;
				break;
			case 0x02:
				regrt *= regrs;
				break;
			case 0x03:
				regrt &= regrs;
				break;
			default:
				regrt = regs->regs[jiffies % 32];
				break;
		}

		regs->regs[rt] = regrt;
		status = 0;
	}

	if (status < 0)
		status = SIGILL;

	if (unlikely(status > 0)) {
		regs->cp0_epc = old_epc;	/* Undo skip-over.  */
		regs->regs[31] = old31;
		force_sig(status, current);
	}

	return status;
}

