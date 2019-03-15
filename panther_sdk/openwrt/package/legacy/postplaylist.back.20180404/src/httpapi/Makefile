LIBRARYS += -lhttp_json -lhttp_fifo -lhttp_ret_json -lhttp_sem -lhttp_uci

CGI_INC = WIFIAudio_GetCommand.h  WIFIAudio_SystemCommand.h WIFIAudio_Debug.h WIFIAudio_LightControl.h WIFIAudio_LightControlFormat.h WIFIAudio_LightControlUart.h WIFIAudio_ChannelCompare.h
CGI_SRC = WIFIAudio_httpapi.c WIFIAudio_GetCommand.c  WIFIAudio_SystemCommand.c WIFIAudio_LightControl.c WIFIAudio_LightControlFormat.c WIFIAudio_LightControlUart.c WIFIAudio_ChannelCompare.c
CGI_TARGET = httpapi.cgi

DOWNLOADBURN_INC = 
DOWNLOADBURN_SRC = WIFIAudio_DownloadBurn.c
DOWNLOADBURN_TARGET = WIFIAudio_DownloadBurn

LEDMATRIXSCREEN_INC =   WIFIAudio_LightControl.h WIFIAudio_LightControlFormat.h WIFIAudio_LightControlUart.h
LEDMATRIXSCREEN_SRC = WIFIAudio_LightControl_LedMatrixScreen.c  WIFIAudio_LightControl.c WIFIAudio_LightControlFormat.c WIFIAudio_LightControlUart.c
LEDMATRIXSCREEN_TARGET = WIFIAudio_LightControl_LedMatrixScreen

all:	$(CGI_TARGET) $(DOWNLOADBURN_TARGET) $(LEDMATRIXSCREEN_TARGET)

$(CGI_TARGET):	$(CGI_SRC) $(CGI_INC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(DOWNLOADBURN_TARGET):	$(DOWNLOADBURN_SRC) $(DOWNLOADBURN_INC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(LEDMATRIXSCREEN_TARGET):	$(LEDMATRIXSCREEN_SRC) $(LEDMATRIXSCREEN_INC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
.PHONY:	clean
clean:
	$(RM) $(CGI_TARGET) $(DOWNLOADBURN_TARGET) $(LEDMATRIXSCREEN_TARGET)

