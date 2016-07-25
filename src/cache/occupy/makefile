CC := gcc
CFLAGS := -O3 -std=gnu11
CFLAGS += -march=native -mtune=native
CFLAGS += -msse4.2 -mavx2 -fomit-frame-pointer
LDFLAGS :=

all:	occupy array-stride

occupy.o:	occupy.c makefile
	$(CC) $(CFLAGS) $< -c -o $@

occupy:	occupy.o
	$(CC) $< -o $@ $(LDFLAGS) 

array-stride:	array-stride.c makefile
	$(CC) $< -o $@ $(CFLAGS) -fopenmp -lrt

clean:
	rm -f occupy *.o array-stride

