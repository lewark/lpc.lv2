OBJS = lpc.o lpc_plugin.o
CC=gcc
CPP=g++
INCLUDES=
CFLAGS=-c -s -O3 -ffast-math -fPIC
#-g for debug
#-s -O3 -ffast-math for release
LIBS=

lpc_plugin.so: $(OBJS) manifest.ttl
	$(CPP) -shared -o $@ $(OBJS) $(LIBS)
	
.o: $*.h

.c.o: $*.h $*.c
	$(CC) $(CFLAGS) $*.c

.cpp.o: $*.h $*.cpp
	$(CPP) $(CFLAGS) $*.cpp

manifest.ttl:
	sed "s/@LIB_EXT@/.so/" manifest.ttl.in > manifest.ttl

.PHONY: clean install uninstall

clean: 
	rm -f lpc_plugin.so *~ *.o manifest.ttl

install: manifest.ttl lpc_plugin.so
	mkdir -p ~/.lv2/lpc_plugin.lv2
	cp manifest.ttl lpc_plugin.ttl lpc_plugin.so ~/.lv2/lpc_plugin.lv2

uninstall:
	rm ~/.lv2/lpc_plugin.lv2/*.so
	rm ~/.lv2/lpc_plugin.lv2/*.ttl
	rmdir ~/.lv2/lpc_plugin.lv2
