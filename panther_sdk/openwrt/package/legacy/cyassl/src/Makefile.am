SUBDIRS = src ctaocrypt
EXTRA_DIST = cyassl.dsp cyassl.dsw certs/*.pem certs/*.der certs/*.txt \
    lib/dummy cyassl.sln cyassl.vcproj cyassl-iphone.xcodeproj/project.pbxproj \
    doc/*.pdf

basic:
	cd src; $(MAKE); cd ../

openssl-links:
	cd lib; ln -s ../src/.libs/libcyassl.a libcrypto.a; \
	ln -s ../src/.libs/libcyassl.a libssl.a; \
	ln -s ../src/.libs/libcyassl.a libcyassl.a; cd ../

install:
	$(mkinstalldirs) $(DESTDIR)$(includedir)/cyassl $(DESTDIR)$(libdir); \
	cp -fpR include/* $(DESTDIR)$(includedir)/cyassl; \
	make openssl-links; \
	cp -fpR src/.libs/libcyassl.{a,so*} src/libcyassl.la $(DESTDIR)$(libdir)
