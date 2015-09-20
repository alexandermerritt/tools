CC := gcc
CFLAGS := -O2 -std=c99
LDFLAGS :=

all:	occupy

occupy.o:	occupy.c makefile
	$(CC) $(CFLAGS) $< -c -o $@

occupy:	occupy.o
	$(CC) $< -o $@ $(LDFLAGS) 

clean:
	rm -f occupy *.o

