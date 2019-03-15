MTDM = mtdm
MTC = mtc
MTC_UDHCPCS = mt_udhcpcs
PRE_INIT = preinit
TARGET = $(MTDM) $(MTC) $(PRE_INIT) $(MTC_UDHCPCS)

#LIBS        = `pkg-config --libs --cflags libugui`

PRE_INIT_OBJS = preinit.o
MTC_OBJS = mtc.o mtdm_client.o
MTC_UDHCPCS_OBJS = mtc_udhcpcs.o
MTDM_OBJS  = $(filter-out $(MTC_OBJS) $(PRE_INIT_OBJS) $(MTC_UDHCPCS_OBJS) ,$(patsubst %.c,%.o,$(wildcard *.c)))
MTDM_OBJS += $(patsubst %.c,%.o,$(wildcard sys/*.c))
MTDM_OBJS += $(patsubst %.c,%.o,$(wildcard serv/*.c))

all: $(TARGET) WDK

WDK:
	@echo "Building WDK ..."
	make -C wdk

$(MTDM): $(MTDM_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(MTC): $(MTC_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(PRE_INIT): $(PRE_INIT_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(MTC_UDHCPCS): $(MTC_UDHCPCS_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET) 


