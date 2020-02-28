OBJS = lpc.o lpc_plugin.o
CC=gcc
CPP=g++
INCLUDES=
CFLAGS=-c -g -fPIC
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

clean: 
	rm -f lpc_plugin.so *~ *.o manifest.ttl
