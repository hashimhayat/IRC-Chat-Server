# This makefile does NOT create the sircd for you.
# You'll want to compile debug.o into sircd to get
# the debugging functions.
# We suggest adding three targets:  all, clean, test

CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -O2 -DDEBUG

LDFLAGS	 	= 
DEFS 	 	=
LIB		= -lpthread

all:	sircd

sircd:	sircd.c rtlib.c irc_proto.c debug.c	
	$(CC) $(DEFS) $(CFLAGS) -c sircd.c
	$(CC) $(DEFS) $(CFLAGS) -c rtlib.c
	$(CC) $(DEFS) $(CFLAGS) -c irc_proto.c
	$(CC) $(DEFS) $(CFLAGS) -c debug.c

	$(LD) -o $@ $(LDFLAGS) sircd.o rtlib.o irc_proto.o debug.o $(LIB)

clean:
	rm -f *.o
