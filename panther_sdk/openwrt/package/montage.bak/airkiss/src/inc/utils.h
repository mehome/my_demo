#define MONTAGE_AIRKISS_VERSION "Version 2.0"
#define MONITOR_DEVICE "mon"
#define VERIFY_DEVICE "br1"
#define WIFIOFFSET	18

#define RECOVERTIME	15
#define NEIBORTIME	5

#define AIRKISSSTOPTIME	60
#define AIRKISSPORT	10000
#define AIRKISSTRYCOUNT	100
#define AIRKISSFILE	"/tmp/airkiss"	

#define AIRKISS_STANDBY		0x0
#define AIRKISS_ONGOING		0x1
#define AIRKISS_DONE		0x2
#define AIRKISS_VERIFIED	0x3
#define AIRKISS_TIMEOUT		0x4

enum dbglevel {
	INFO=1,
	ERROR,
	WARN,
};

struct mon_airkiss {
	bool is_ap_lock;
	bool is_chan_lock;
	bool is_airkiss_complete;
	bool is_wifi_scan;
	int stay_channel_tick;
	int lock_ap_tick;
	unsigned int current_channel;
	unsigned int current_channel_offset;
	unsigned int primary_channel;
	unsigned int second_channel_offset;
	pthread_mutex_t lock;
	pthread_t rcv_thread_id;
	//pcap_t *handle;
	int sockfd;
};

int start_airkiss(struct mon_airkiss *);
static void monitor_interface_add(void);
static void monitor_interface_del(void);
