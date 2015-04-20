all:	mmap.so

mmap.o:	mmap.c
	gcc -Wall -Werror -fPIC -c mmap.c

mmap.so:	mmap.o
	gcc -shared -o libmmap.so mmap.o

clean:
	rm -f libmmap.so mmap.o

