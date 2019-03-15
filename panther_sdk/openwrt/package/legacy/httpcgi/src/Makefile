export CFLAGS
export LDFLAGS

.PHONY: all install clean 
all:
	$(MAKE) -C command
	${MAKE} -C amazon
	
install:
	${MAKE} -C command install
	${MAKE} -C amazon install
	${MAKE} -C html install
	
clean:
	-$(MAKE) clean -C command
	-$(MAKE) clean -C amazon
	
