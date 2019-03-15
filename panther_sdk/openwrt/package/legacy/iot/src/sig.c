#include "sig.h"

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#include "myutils/debug.h"
#include "common.h"

static int shuttingDown = 0;


static void sig_child(void)
{
	pid_t pid;
	while ((pid = waitpid((pid_t) - 1, 0, WNOHANG)) > 0) {
		debug("pid:%d exit",pid);
		;
	}
}

static void sig_shutdown(void)
{
	info("sig_shutdown");
	if (shuttingDown)
		return;
	shuttingDown = 1;
	PidUnLock();
#ifdef USE_UPLOAD_AMR
	QueueDeinit();
#endif
	AlertDeinit();
	MpdDeinit();
	exit(-1);
}
static int signal_ignore()
{
	
}
static void signal_alarm()
{
    wav_play2("/tmp/res/sinvoice_timeout.wav");
    cdb_set_int("$sinvoice_start",0);
    //sinvoice_finish();
    //stop_record_data();
}

static void sig_handler(int foo, siginfo_t * bar, void *baz)
{
	debug("Sig:%d\n",foo);
	switch (foo) 
    {
		case SIGCHLD:
			sig_child();
			break;
		case SIGUSR1:
			//sig_shutdown();
			break;
		case SIGUSR2:
			//sig_shutdown();
			break;
		case SIGKILL:
			sig_shutdown();
			break;
		case SIGABRT :
			signal_ignore();
			break;
		case SIGALRM :
			signal_alarm();
			break;
		default:
			debug("Unknow sig:%d\n",foo);
			break;
	}
}



#ifdef      ENABLE_BACKSTRACE

#include <signal.h>
#include <memory.h>
#if 1
#include <ucontext.h>
#else
#include <asm-generic/ucontext.h>
#endif
#include <dlfcn.h>


#define CALL_TRACE_MAX_SIZE (10)
#define MIPS_ADDI_SP_SP_XXXX (0x23bd0000) /* instruction code for addi sp, sp, xxxx */
#define MIPS_ADDIU_SP_SP_XXXX (0x27bd0000) /* instruction code for addiu sp, sp, xxxx */
#define MIPS_SW_RA_XXXX_SP (0xafbf0000) /* instruction code for sw ra, xxxx(sp) */
#define MIPS_SD_RA_XXXX_SP (0xffbf0000) /* instruction code for sd ra ,xxxx(sp) */
#define MIPS_ADDU_RA_ZERO_ZERO (0x0000f821) /* instruction code for addu ra, zero, zero */

static int backtrace_mips32(void **buffer, int size, struct ucontext *uc)
{
	unsigned long *addr = NULL;
	unsigned long *ra = NULL;
	unsigned long *sp = NULL;
	unsigned long *pc = NULL;
	size_t ra_offset = 0;
	size_t stack_size = 0;
	int depth = 0;

	if (size == 0)
	{
		return 0;
	}

	if (buffer == NULL || size < 0 || uc == NULL)
	{
		return -1;
	}

#if 1
	pc = (unsigned long *)(unsigned long)uc->uc_mcontext.pc;
	ra = (unsigned long *)(unsigned long)uc->uc_mcontext.gregs[31];
	sp = (unsigned long *)(unsigned long)uc->uc_mcontext.gregs[29];
#else
	pc=(unsigned long*)(unsigned long)uc->uc_mcontext.sc_pc;
	ra = (unsigned long *)(unsigned long)uc->uc_mcontext.sc_regs[31];
	sp = (unsigned long *)(unsigned long)uc->uc_mcontext.sc_regs[29];
#endif	

	buffer[0] = pc;

	if ( size == 1 )
	{
		return 1;
	}

	for ( addr = pc; !ra_offset || !stack_size; --addr )
	{
		if ( ((*addr) & 0xffff0000) == MIPS_SW_RA_XXXX_SP || ((*addr) & 0xffff0000) == MIPS_SD_RA_XXXX_SP )
		{
			ra_offset = (short)((*addr) & 0xffff);
		}
		else if ( ((*addr) & 0xffff0000) == MIPS_ADDIU_SP_SP_XXXX || ((*addr) & 0xffff0000) == MIPS_ADDI_SP_SP_XXXX )
		{
			stack_size = abs((short)((*addr) & 0xffff));
		}
		else if ( (*addr) == MIPS_ADDU_RA_ZERO_ZERO )
		{
			return 1;
		}
	}

	if ( ra_offset > 0 )
	{
		ra = (unsigned long *)(*(unsigned long *)((unsigned long)sp + ra_offset));
	}

	if ( stack_size > 0 )
	{
		sp = (unsigned long *)((unsigned long)sp + stack_size);
	}

	for (depth = 1; depth < size && ra; ++depth)
	{
		buffer[depth] = ra;

		ra_offset = 0;
		stack_size = 0;

		for (addr = ra; !ra_offset || !stack_size; --addr)
		{
			if ( ((*addr) & 0xffff0000) == MIPS_SW_RA_XXXX_SP || ((*addr) & 0xffff0000) == MIPS_SD_RA_XXXX_SP )
			{
				ra_offset = (short)((*addr) & 0xffff);
			}
			else if ( ((*addr) & 0xffff0000) == MIPS_ADDIU_SP_SP_XXXX || ((*addr) & 0xffff0000) == MIPS_ADDI_SP_SP_XXXX )
			{
				stack_size = abs((short)((*addr) & 0xffff));
			}
			else if ( (*addr) == MIPS_ADDU_RA_ZERO_ZERO )
			{
				return depth + 1;
			}
		}

		ra = (unsigned long *)(*(unsigned long *)((unsigned long)sp + ra_offset));
		sp = (unsigned long *)((unsigned long)sp + stack_size);
	}

	return depth;
}

typedef struct
{
	const char *dli_fname;
	void *dli_fbase;
	const char *dli_sname;
	void *dli_saddr;
} Dl_info;

extern int dladdr(void *address, Dl_info *dlip);

static void print_backtrace_mips32(void **array, int size)
{
	FILE *fp = NULL;
	int i;
	Dl_info info;
	int status;

	fp = fopen("/proc/self/maps", "r");

	printf("size = %d\n", size );
	if( fp ) {
		for (i = 0; i < size && 0 < (unsigned int) array[i]; i++) {
			char line[1024];
			int found = 0;

			rewind(fp);
			
			memset(line, 0 , sizeof(line));
			while (fgets(line, sizeof(line), fp)) {
				char lib[1024];
				void *start, *end;
				unsigned int offset;
				
				int n = sscanf(line, "%p-%p %*s %x %*s %*d %s", &start, &end, &offset, lib);
				if (n == 4 && array[i] >= start && array[i] < end) {
					if (array[i] < (void*)0x10000000)
						offset = (unsigned int)array[i];
					else
						offset += array[i] - start;

					printf("#%d [%08x] in %-20s ", i, offset, lib);
					status = dladdr (array[i], &info);
					if (status && info.dli_fname && info.dli_fname[0] != '\0')
					{
						printf("< %s+0x%x >\r\n", info.dli_sname, (unsigned int)((unsigned int)array[i] - (unsigned int)info.dli_saddr));
					}

					found = 1;
					break;
				}
			}
			if (!found)
				printf("#%d [%08x]\n", i, (unsigned)array[i]);
		}
		fclose(fp);
	}
}

static void signalHandler( int sig, siginfo_t* siginfo, void* uc)
{
	void* allocRec[100];
	int num=0;
	num = backtrace_mips32(allocRec, 100, (struct ucontext *)uc);
	print_backtrace_mips32(allocRec, num);
	signal(sig, SIG_DFL);
}
#endif
void signal_setup(void)
{
	// mask off all signals before creating threads.
	// this way we control which thread gets which signals.
	// for now, we don't care which thread gets the following.
	sigset_t set;
	sigfillset(&set);
	
	sigdelset(&set, SIGALRM);
	sigdelset(&set, SIGCHLD);
	sigdelset(&set, SIGUSR2);
	sigdelset(&set, SIGUSR1);
	sigdelset(&set, SIGKILL);
	sigdelset(&set, SIGABRT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	// setting this to SIG_IGN would prevent signalling any threads.

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;

	

	sa.sa_sigaction = &sig_handler;
	sigaction(SIGUSR1, &sa, NULL);

	sa.sa_sigaction = &sig_handler;
	sigaction(SIGCHLD, &sa, NULL);
	
	sa.sa_sigaction = &sig_handler;
	sigaction(SIGUSR2, &sa, NULL);

	sa.sa_sigaction = &sig_handler;
	sigaction(SIGKILL, &sa, NULL);
	
	sa.sa_sigaction = &sig_handler;
	sigaction(SIGABRT , &sa, NULL);
	
	sa.sa_sigaction = &sig_handler;
	sigaction(SIGALRM , &sa, NULL);

#ifdef      ENABLE_BACKSTRACE
	sa.sa_sigaction = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
#endif	

}

