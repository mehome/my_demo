#common makefile header
#CC=mipsel-openwrt-linux-gcc
#LD=mipsel-openwrt-linux-ld
DIR_INC = ./include
DIR_SRC = ./src
DIR_MAD = ./resample
export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y
TARGET	=   AlexaRequest 

CROSS_COMPILE = 
CFLAGS += -Wunused-function -Wunused-but-set-variable -g  -I$(DIR_INC) -I$(DIR_MAD) -I$(STAGING_DIR)/usr/include
LIBS = 
#   
LDFLAGS += -lcdb -lssl -lcurl -luci -lcrypto -lpthread -ljson-c -lasound -lmad -lnghttp2 -lmpdclient -lrt -lm  -ldl
LDFLAGS += -lubox -lm -L./wakeup/lib -lthf
LDFLAGS += -lavsclient -lcchip
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

#ALERT_TEST_SRCS = alert/alert_api.c alert/Alert.c  alert/AlertEventListener.c alert/AlertScheduler.c alert/Crontab.c alert/RequestFactory.c \
#							alert/AlertManager.c alert/AlertsFileDataStore.c alert/AlertsStatePayload.c alert/AlertHandler.c \
#							alert/hashmap.c alert/hashmap_itr.c alert/hashset.c alert/hashset_itr.c alert/list_iterator.c \
#							alert/List.c alert/list_node.c  
#ALERT_TEST_OBJS := $(patsubst alert/%.c,alert/%.o,$(wildcard *.c))

#### alert 
ALERT_SRCS=$(shell ls ./alert/*.c)							
ALERT_OBJS = $(ALERT_SRCS:.c=.o)

###### token
#TOKEN_SRCS=$(shell ls ./token/*.c)
#TOKEN_OBJS = $(TOKEN_SRCS:.c=.o)

###### mpc
MPC_SRCS=$(shell ls ./mpc/*.c)
MPC_OBJS = $(MPC_SRCS:.c=.o)

###### Queue
QUEUE_SRCS=$(shell ls ./Queue/*.c)
QUEUE_OBJS = $(QUEUE_SRCS:.c=.o)

###### capture
CAPTURE_SRCS=$(shell ls ./capture/*.c)
CAPTURE_OBJS = $(CAPTURE_SRCS:.c=.o)

###### mp3player
MP3PLAYER_SRCS=$(shell ls ./mp3player/*.c)
MP3PLAYER_OBJS = $(MP3PLAYER_SRCS:.c=.o)

###### wavplay
WAVPLAYER_SRCS=$(shell ls ./wavplay/*.c)
WAVPLAYER_OBJS = $(WAVPLAYER_SRCS:.c=.o)

WAKEUP_SRCS=$(shell ls ./wakeup/*.c)
WAKEUP_OBJS = $(WAKEUP_SRCS:.c=.o)
###### wakeup
#$(if $(CONFIG_ALEXA_SUPPORT_FF),WAKEUP_SRCS=$(shell ls ./wakeup/*.c)WAKEUP_OBJS=$(WAKEUP_SRCS:.c=.o))

###### authDelegat
AUTHDELEGATE_SRCS=$(shell ls ./authDelegate/*.c)
AUTHDELEGATE_OBJS = $(AUTHDELEGATE_SRCS:.c=.o)

all: $(TARGET)
    $(if $(BUILD_FF_FLAG), @echo "abcd123")


$(TARGET): $(OBJECTS) $(ALERT_OBJS) $(QUEUE_OBJS) $(MPC_OBJS) $(CAPTURE_OBJS) $(MP3PLAYER_OBJS) $(WAVPLAYER_OBJS) $(WAKEUP_OBJS) $(AUTHDELEGATE_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean

