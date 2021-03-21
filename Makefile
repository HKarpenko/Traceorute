CC=gcc
CFLAGS=-c -Wall -Wextra
LDFLAGS=
SOURCES=traceroute.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=traceroute

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o

distclean:
	rm -rf *.o *.c