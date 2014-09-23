all:	code

.PHONY:	all

CC=clang
CFLAGS=-Wall -Werror -Wno-unused-variable -g
LDFLAGS=-libverbs

ib.o:	ib.c
	$(CC) $(CFLAGS) $^ -c -o $@

code:	code.c ib.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	ln -sf $@ server
	ln -sf $@ client

clean:
	rm -f code server client *.o *~
