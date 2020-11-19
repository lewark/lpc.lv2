OBJS = lpc.o lpc_plugin.o
CC=gcc
CPP=g++
INCLUDES=
CPPFLAGS=-c -s -O3 -ffast-math -fPIC
#-g for debug
#-s -O3 -ffast-math for release
LIBS=
INSTALLDIR=~/.lv2

lpc_plugin.so: $(OBJS) manifest.ttl
	$(CPP) -shared -o $@ $(OBJS) $(LIBS)

manifest.ttl:
	sed "s/@LIB_EXT@/.so/" manifest.ttl.in > manifest.ttl

.PHONY: clean install uninstall validate

clean: 
	rm -f lpc_plugin.so *~ *.o manifest.ttl

install: manifest.ttl lpc_plugin.so
	mkdir -p $(INSTALLDIR)/lpc.lv2
	cp manifest.ttl lpc_plugin.ttl lpc_plugin.so $(INSTALLDIR)/lpc.lv2

uninstall:
	rm $(INSTALLDIR)/lpc.lv2/*.so
	rm $(INSTALLDIR)/lpc.lv2/*.ttl
	rmdir $(INSTALLDIR)/lpc.lv2

validate: manifest.ttl
	sord_validate -l $(shell find -L /usr/include/lv2 /usr/lib/lv2/schemas.lv2 -type f -name '*.ttl') manifest.ttl lpc_plugin.ttl
