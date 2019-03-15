#include "globalParam.h"
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "alert/AlertHead.h"

AlertManager *alertManager;

extern UINT8 g_byDebugFlag;
extern pthread_t CurlHttpsRequest_Pthread;
extern pthread_t StatusMonitor_Pthread;
extern pthread_t DownchannelRequest_Pthread;
extern pthread_t getMpdStatus_Pthread;
extern pthread_t CaptureAudio_Pthread;
extern pthread_t Mp3player_Pthread;
extern pthread_t WakeUp_Pthread;

#ifdef ALEXA_FF
extern pthread_t WakeUp_Pthread;
#endif

static void thread_wait(void)
{
#ifdef ALEXA_FF
	if (WakeUp_Pthread != 0)
	{
		pthread_join(WakeUp_Pthread, NULL);
		DEBUG_PRINTF("WakeUp_Pthread end...\n");
	}
#endif

	if (StatusMonitor_Pthread != 0)
	{
		pthread_join(StatusMonitor_Pthread, NULL);
		DEBUG_PRINTF("StatusMonitor_Pthread end...\n");
	}

	if (CurlHttpsRequest_Pthread != 0)
	{
		pthread_join(CurlHttpsRequest_Pthread, NULL);
		DEBUG_PRINTF("CurlHttpsRequest_Pthread end...\n");
	}

	if (DownchannelRequest_Pthread != 0)
	{
		pthread_join(DownchannelRequest_Pthread, NULL);
		DEBUG_PRINTF("DownchannelRequest_Pthread end...\n");
	}

	if (getMpdStatus_Pthread != 0)
	{
		pthread_join(getMpdStatus_Pthread, NULL);
		DEBUG_PRINTF("getMpdStatus_Pthread end...\n");
	}

	if (CaptureAudio_Pthread != 0)
	{
		pthread_join(CaptureAudio_Pthread, NULL);
		DEBUG_PRINTF("CaptureAudio_Pthread end...\n");
	}
	
	if (Mp3player_Pthread != 0)
	{
		pthread_join(Mp3player_Pthread, NULL);
		DEBUG_PRINTF("Mp3player_Pthread end...\n");
	}
}

static int waitWifiConnect(void)
{
#if 0
	int iStatus = 0;
	char *pWifiConnectStatus;

	while (1)
	{
		sleep(1);
		pWifiConnectStatus = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wificonnectstatus");
		if(NULL != pWifiConnectStatus) 
		{
			iStatus = atoi(pWifiConnectStatus);
			free(pWifiConnectStatus);
			pWifiConnectStatus = NULL;
			if (3 == iStatus)
			{
				DEBUG_PRINTF("xzxwifiaudio.config.amazon_loginStatus is %d", iStatus);
				DEBUG_PRINTF(" ");
				DEBUG_PRINTF(" ");
				DEBUG_PRINTF("*****************************************");
				DEBUG_PRINTF("*                                       *");
				DEBUG_PRINTF("*   wifiConnectSucceed, start alexa...  *");
				DEBUG_PRINTF("*                                       *");
				DEBUG_PRINTF("*                       V3.0 2017.02.21 *");
				DEBUG_PRINTF("*****************************************");
				return 0;
			}
		}
	}
#else
	DEBUG_PRINTF(" ");
	DEBUG_PRINTF(" ");
	DEBUG_PRINTF("*****************************************");
	DEBUG_PRINTF("*                                       *");
	DEBUG_PRINTF("*   wifiConnectSucceed, start Alexa...  *");
	DEBUG_PRINTF("*                                       *");
	DEBUG_PRINTF("*                       C-Chip Team     *");
	DEBUG_PRINTF("*                       V2.1 2016.12.13 *");
	DEBUG_PRINTF("*****************************************");
#endif
}

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

int backtrace_mips32(void **buffer, int size, struct ucontext *uc)
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

void signalHandler( int sig, siginfo_t* siginfo, void* uc)
{
	void* allocRec[100];
	int num=0;
	num = backtrace_mips32(allocRec, 100, (struct ucontext *)uc);
	print_backtrace_mips32(allocRec, num);
	signal(sig, SIG_DFL);
}

#define LOCKFILE "/var/run/AlexaRequest.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int lockfile(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return(fcntl(fd, F_SETLK, &fl));
}

int alexa_running(void)
{
	int fd;
	char buf[16];
	
	int Speech = cdb_get_int("$Speech_recognition", 4);
	if (Speech != 3)
	{
		printf("Speech is not alexa :%d\n", Speech);
		//exit(-1);
	}
	fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	if (fd < 0) {
		printf("can't open %s: %s, alexa is running.", LOCKFILE, strerror(errno));
		exit(1);
	}
	if (lockfile(fd) < 0) {
	if (errno == EACCES || errno == EAGAIN) {
		printf("can't lock %s: %s, alexa is running.", LOCKFILE, strerror(errno));
		close(fd);
		exit(1);
	}
		printf("can't lock %s: %s, alexa is running.", LOCKFILE, strerror(errno));
		exit(1);
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);
	return 0;
}

int Alert_Init(void)
{
	alertManager = alertmanager_new();
	if(NULL == alertManager)
	{
		DEBUG_PRINTF("alertmanager_new failed ");
	}

	if(crontab_load_from_disk(alertManager) != 0)
	{
		DEBUG_PRINTF("crontab_load_from_disk failed ");
	}

	//Alert_Pthread = alertmanager_schedulers(alertManager);
	//if(0 == Alert_Pthread) {
	//		DEBUG_PRINTF("alertmanager_schedulers failed ");
	//}
}

static void StarSigaction(void)
{
	struct sigaction sa;
	char a;

	sa.sa_sigaction = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
}

int Alert_Reload(void) 
{
	if(alertManager)
		crontab_load_from_disk(alertManager) ;
}

extern size_t g_playttsaudio_chunk_bytes;
int main(int argc, char* argv[])
{
	alexa_running();

	StarSigaction();

	if ((2 == argc) && (0 == strcmp("-v", argv[1])))
	{
		g_byDebugFlag = DEBUG_ON;
	}

	waitWifiConnect();

	SystemPara_Init();

	Alert_Init();

#ifdef ALEXA_FF
	CreateWakeupPthread();
#endif

	CreateGetMpdStatusPthread();

	CreateCapturePthread();

	CreateUPStatusMonitorPthread();

	CreateDownchannelRequestPthread();

	CreateCurlHttpsRequestPthread();

	CreateMp3PlayerPthread();

	thread_wait();

	return 0;
}


