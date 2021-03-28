CFLAGS=-std=gnu99 -Wall -Wextra

all: traceroute

traceroute: traceroute.o send.o recieve.o

traceroute.o: traceroute.c send.c recieve.c

clean:
	rm -rf *.o

distclean:
	rm -rf *.o *.c