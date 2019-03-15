export CFLAGS := -I$(shell pwd)/include $(CFLAGS)
export LDFLAGS := -L$(shell pwd)/lib $(LDFLAGS)

.PHONY: all clean 
all:
	mkdir -p lib
	mkdir -p include
	$(MAKE) -C http_lib clean
	$(MAKE) -C http_lib
	cp http_lib/*.so lib
	cp http_lib/*.h include
ifeq ($(WIFIAUDIO_DEBUG), y)
	cp -rf http_lib/include/* include/
	cp -rf http_lib/openwrt_lib/* lib/
endif
	mkdir -p bin
	mkdir -p www/cgi-bin
	$(MAKE) -C httpapi
	cp httpapi/*.cgi www/cgi-bin/
	cp httpapi/WIFIAudio_DownloadBurn bin/
	cp httpapi/WIFIAudio_LightControl_LedMatrixScreen bin/
	$(MAKE) -C postmusiclist
	cp postmusiclist/*.cgi www/cgi-bin/
	$(MAKE) -C Lenovo_cgi
ifeq ($(WIFIAUDIO_DEBUG), y)
	cp Lenovo_cgi/*.cgi www/cgi-bin/
else
	cp Lenovo_cgi/companionInfo www/cgi-bin/
	cp Lenovo_cgi/deviceInfo www/cgi-bin/
endif
	$(MAKE) -C htmlupdate/cgi-bin/
	cp htmlupdate/cgi-bin/*.cgi www/cgi-bin/
	cp htmlupdate/xzx.html www/
	cp -rf htmlupdate/css www/
	cp -rf htmlupdate/js www/

clean:
	-$(MAKE) clean -C http_lib
	-$(MAKE) clean -C httpapi
	-$(MAKE) clean -C postmusiclist
	-$(MAKE) clean -C Lenovo_cgi
	-$(MAKE) clean -C htmlupdate/cgi-bin
	-$(RM) -rf lib
	-$(RM) -rf include
	-$(RM) -rf bin
	-$(RM) -rf www
	
