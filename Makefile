CFLAGS=-std=gnu99 -Wall -Wextra

all: traceroute

traceroute: traceroute.o

traceroute.o: traceroute.c

clean:
	rm -rf *.o

distclean:
	rm -rf *.o *.c