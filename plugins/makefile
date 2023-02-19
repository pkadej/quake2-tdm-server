# Makefile to use with linux gcc compiler
CC=gcc
CFLAGS=-DC_ONLY -D__linux__ -Dstricmp=strcasecmp -O6 -ffast-math \
	-funroll-loops -fexpensive-optimizations

DO_SHLIB_CC=$(CC) $(CFLAGS) -fPIC -o $@ -c $<

OBJS=main.o irc_mainthread.o irc_threads.o

../tdm_irc.so : $(OBJS)
	$(CC) $(CFLAGS) -lpthread -shared -o $@ $(OBJS)
	
main.o : main.c
	$(DO_SHLIB_CC)
	
irc_mainthread.o : irc_mainthread.c
	$(DO_SHLIB_CC)
	
irc_threads.o :	irc_threads.c
	$(DO_SHLIB_CC)
