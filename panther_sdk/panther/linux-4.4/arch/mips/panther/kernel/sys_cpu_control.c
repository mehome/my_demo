#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <linux/compiler.h>
#include <linux/smp.h>

#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/hardirq.h>
#include <asm/mmu_context.h>
#include <asm/time.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/mips_mt.h>

//char throttle_buf[L1_CACHE_BYTES] __attribute__ ((__aligned__(L1_CACHE_BYTES)));
extern asmlinkage void __r4k_wait(void);

struct cpu_control_in
{
	int cpu;
	int command;
	unsigned long arg[8];
};

struct cpu_control_out
{
	unsigned long result[8];
};

#define CPU_CONTROL_CMD_THROTTLE	1
#define CPU_CONTROL_CMD_COP0_READ	2
#define CPU_CONTROL_CMD_COP0_WRITE	3

SYSCALL_DEFINE4(cpu_control, char __user * , in, int , in_len, char __user *, out, int, out_len)
{
	int retval = -EFAULT;
	struct cpu_control_in ctrl_in;
	struct cpu_control_out ctrl_out;
	int i;

	memset(&ctrl_in, 0, sizeof(struct cpu_control_in));
	memset(&ctrl_out, 0, sizeof(struct cpu_control_out));

	if(in_len)
	{
		if (copy_from_user(&ctrl_in, in, in_len))
			goto error_out;
	}

	if(ctrl_in.command==CPU_CONTROL_CMD_THROTTLE)
	{
		for(i=0;i<ctrl_in.arg[0];i++)
		{
			#if 1
				__r4k_wait();
			#else
				__asm__ __volatile__  (
					"lui $t1, 0x9000\n\t"
					"cache 0x14, 0x7fe0($t1)\n\t"
					"lw  $t2, 0x7fe0($t1)\n\t"
					"addu $t2, $t1, $t2\n\t"
					"cache 0x14, 0x7fe0($t1)\n\t"
					"lw  $t2, 0x7fe0($t1)\n\t"
					"addu $t2, $t1, $t2\n\t"
					"cache 0x14, 0x7fe0($t1)\n\t"
					"lw  $t2, 0x7fe0($t1)\n\t"
					"addu $t2, $t1, $t2\n\t"
					"cache 0x14, 0x7fe0($t1)\n\t"
					"lw  $t2, 0x7fe0($t1)\n\t"
					"addu $t2, $t1, $t2\n\t"
					"cache 0x14, 0x7fe0($t1)\n\t"
					"lw  $t2, 0x7fe0($t1)\n\t"
					"addu $t2, $t1, $t2\n\t"
					 : : : "t1", "t2");
			#endif
		}

		retval = 0;
	}
	else if(ctrl_in.command==CPU_CONTROL_CMD_COP0_READ)
	{
		if((ctrl_in.arg[0]==16)&&(ctrl_in.arg[1]==7))
		{
			ctrl_out.result[0] = read_c0_config7();
		}
		else if((ctrl_in.arg[0]==0)&&(ctrl_in.arg[1]==1))
		{
			ctrl_out.result[0] = __read_32bit_c0_register($0, 1);
		}
		else if((ctrl_in.arg[0]==1)&&(ctrl_in.arg[1]==7))
		{
			ctrl_out.result[0] = __read_32bit_c0_register($1, 7);
		}
		else if((ctrl_in.arg[0]==3)&&(ctrl_in.arg[1]==7))
		{
			ctrl_out.result[0] = __read_32bit_c0_register($3, 7);
		}

		retval = 0;
	}
	else if(ctrl_in.command==CPU_CONTROL_CMD_COP0_WRITE)
	{
		if((ctrl_in.arg[0]==16)&&(ctrl_in.arg[1]==7))
		{
			write_c0_config7(ctrl_in.arg[2]);
			ctrl_out.result[0] = read_c0_config7();
		}
		else if((ctrl_in.arg[0]==0)&&(ctrl_in.arg[1]==1))
		{
			__write_32bit_c0_register($0, 1, ctrl_in.arg[2]);
			ctrl_out.result[0] = __read_32bit_c0_register($0, 1);
		}
		else if((ctrl_in.arg[0]==1)&&(ctrl_in.arg[1]==7))
		{
			__write_32bit_c0_register($1, 7, ctrl_in.arg[2]);
			ctrl_out.result[0] = __read_32bit_c0_register($1, 7);
		}
		else if((ctrl_in.arg[0]==3)&&(ctrl_in.arg[1]==7))
		{
			__write_32bit_c0_register($3, 7, ctrl_in.arg[2]);
			ctrl_out.result[0] = __read_32bit_c0_register($3, 7);
		}

		retval = 0;
	}


	if(0==retval)
	{
		if(out_len)
		{
			if (copy_to_user(out, &ctrl_out, out_len))
				goto error_out;
		}
	}

	return retval;

error_out:
	retval = -EFAULT;
	return retval;
}

