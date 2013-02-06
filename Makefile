CC=gcc
CFLAGS=-g -O2 `pkg-config --cflags gtk+-2.0`
LDFLAGS=-lX11 `pkg-config --libs gtk+-2.0`

cliplog: cliplog.c
	gcc ${CFLAGS} cliplog.c ${LDFLAGS} -o cliplog

clean:
	rm cliplog
