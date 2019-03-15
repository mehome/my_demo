DIR=$(shell pwd) 
CONFIG_PACKAGE_IOT_ENABLE_ULE=y
CONFIG_PACKAGE_IOT_ENABLE_LVMENG=y
DIR_LIB = ../lib
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
CLIENT := turingClient
endif

SERVER := turingIot

TARGET	:=  $(SERVER) 
CONFIG_IOT_ENABLE_OPUS=y
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
TARGET += $(CLIENT)
endif

DIR_INC += -I$(CURDIR) -I$(CURDIR)/event  -I$(CURDIR)/iot -I$(CURDIR)/alert   -I$(CURDIR)/capture  
DIR_INC += -I$(CURDIR)/amr  -I$(CURDIR)/turing -I$(CURDIR)/common  -I$(CURDIR)/include  #-I$(CURDIR)/log 
DIR_INC += -I$(CURDIR)/column   -I$(CURDIR)/collect  -I$(CURDIR)/function  -I$(CURDIR)/playlist -I$(CURDIR)/curlmulti 
DIR_INC += -I$(CURDIR)/wechat  
DIR_INC += -I$(CURDIR)/wavplay   
DIR_INC += -I$(CURDIR)/mpg123
DIR_INC += -I$(CURDIR)/sinvoice

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
DIR_INC += -I$(CURDIR)/huabei 
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_TTPOD),y)
DIR_INC += -I$(CURDIR)/ttpod  
endif

ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
DIR_INC += -I$(CURDIR)/curlmulti 
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_COMMSAT),y)
DIR_INC += -I$(CURDIR)/commsat 
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
DIR_INC += -I$(CURDIR)/huabei 
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
DIR_INC += -I$(CURDIR)/migu
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
DIR_INC += -I$(CURDIR)/opus 
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
DIR_INC += -I$(CURDIR)/ule
DIR_INC += -I$(CURDIR)/iflytek
endif
#CC = mips-openwrt-linux-gcc


CFLAGS += $(DIR_INC) -DUSE_MSG_QUEUE
LDFLAGS += -ljson-c -lcurl -luci -lasound -lpthread  -ldl -lrt -lopencore-amrnb  -lwakeup
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
LDFLAGS += -lmigu
endif

ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
LDFLAGS += -lopus
endif

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

PLAYER_OBJS += $(patsubst %.c,%.o,$(wildcard playlist/*.c))
IOT_OBJS += $(patsubst %.c,%.o,$(wildcard iot/*.c))
ALERT_OBJS += $(patsubst %.c,%.o,$(wildcard alert/*.c))
CAPTURE_OBJS += $(patsubst %.c,%.o,$(wildcard capture/*.c))
AMR_OBJS += $(patsubst %.c,%.o,$(wildcard amr/*.c))
TURING_OBJS += $(patsubst %.c,%.o,$(wildcard turing/*.c))
#LOG_OBJS += $(patsubst %.c,%.o,$(wildcard log/*.c))
COMMON_OBJS += $(patsubst %.c,%.o,$(wildcard common/*.c))
EVENT_OBJS += $(patsubst %.c,%.o,$(wildcard event/*.c))
COLLECT_OBJS += $(patsubst %.c,%.o,$(wildcard collect/*.c))
COLUMN_OBJS += $(patsubst %.c,%.o,$(wildcard column/*.c))
FUNCTION_OBJS += $(patsubst %.c,%.o,$(wildcard function/*.c))
WAVPLAY_OBJS += $(patsubst %.c,%.o,$(wildcard wavplay/*.c))
MPG123_OBJS += $(patsubst %.c,%.o,$(wildcard mpg123/*.c))
WECHAT_OBJS += $(patsubst %.c,%.o,$(wildcard wechat/*.c))
PLAYLIST_OBJS += $(patsubst %.c,%.o,$(wildcard playlist/*.c))
SINVOICE_OBJS += $(patsubst %.c,%.o,$(wildcard sinvoice/*.c))

ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
MULTI_OBJS += $(patsubst %.c,%.o,$(wildcard curlmulti/*.c))
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_COMMSAT),y)
COMMSAT_OBJS += $(patsubst %.c,%.o,$(wildcard commsat/*.c))
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_TTPOD),y)
TTPOD_OBJS += $(patsubst %.c,%.o,$(wildcard ttpod/*.c))
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
HUABEI_OBJS += $(patsubst %.c,%.o,$(wildcard huabei/*.c))
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
MIGU_OBJS += $(patsubst %.c,%.o,$(wildcard migu/*.c))
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
OPUS_OBJS += $(patsubst %.c,%.o,$(wildcard opus/*.c))
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
ULE_OBJS += $(patsubst %.c,%.o,$(wildcard ule/*.c))
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK),y)
ULE_OBJS += $(patsubst %.c,%.o,$(wildcard iflytek/*.c))
endif
endif
SERVER_OBJS += $(OBJECTS) $(CAPTURE_OBJS)  $(IOT_OBJS)  $(LOG_OBJS) $(PLAYER_OBJS)
SERVER_OBJS += $(COMMON_OBJS) $(ALERT_OBJS) $(AMR_OBJS) $(TURING_OBJS) $(EVENT_OBJS) 
SERVER_OBJS += $(COLLECT_OBJS) $(COLUMN_OBJS) $(FUNCTION_OBJS) $(PLAYLIST_OBJS) 
SERVER_OBJS += $(CONFIG_OBJS) $(WECHAT_OBJS) $(WAVPLAY_OBJS) $(MPG123_OBJS) $(SINVOICE_OBJS)

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
SERVER_OBJS += $(HUABEI_OBJS)
endif


ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
SERVER_OBJS += $(MULTI_OBJS)
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_COMMSAT),y)
SERVER_OBJS += $(COMMSAT_OBJS)
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
SERVER_OBJS += $(MIGU_OBJS)
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_TTPOD),y)
SERVER_OBJS += $(TTPOD)
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
SERVER_OBJS += $(OPUS_OBJS)
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
SERVER_OBJS += $(ULE_OBJS)
endif
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
CLIENT_ALL_OBJS := $(patsubst %.c,%.o,$(wildcard client/*.c))
ifneq ($(CONFIG_IOT_ENABLE_OPUS), y)
OPUS_CLIENT_OBJS = client/opus.o
CLIENT_OBJS += $(filter-out $(OPUS_CLIENT_OBJS) ,$(CLIENT_ALL_OBJS))

endif

CLIENT_OBJS += $(TEST_OBJS)
CLIENT_OBJS += $(patsubst %.c,%.o,$(wildcard common/*.c))
#CLIENT_OBJS += $(patsubst %.c,%.o,$(wildcard log/*.c))
CLIENT_OBJS += $(patsubst %.c,%.o,$(wildcard config/*.c))
endif

ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
CLIENT_LDFLAGS += -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcdb -luci -ljson-c -lm
CLIENT_LDFLAGS += -lmyutils -lmpg123  -lout123
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
CLIENT_LDFLAGS += -lopus
endif
all: $(TARGET) 
$(SERVER) : $(SERVER_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS)
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
$(CLIENT) : $(CLIENT_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(CLIENT_LDFLAGS)
endif
%.o : %.c
	$(CC) -c -Os $(CFLAGS) $^ -o $@
clean:
	@rm -f *.o $(TARGET)

.PHONY:clean


