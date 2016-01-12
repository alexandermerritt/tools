// scalability of "alloc" and "release" of physical memory

#include <iostream>
#include <exception>
#include <thread>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <omp.h>

using namespace std;

typedef volatile long* vlp;

static inline long
diff(struct timespec c1, struct timespec c2)
{
    return (c2.tv_sec*1e9+c2.tv_nsec)
        - (c1.tv_sec*1e9+c1.tv_nsec);
}

void initomp(int nthreads)
{
    omp_set_num_threads(nthreads);
#pragma omp parallel for
    for (long i = 0; i < (1L<<20); i++)
        (void)i;
}

void pfault(int nthreads, size_t len, size_t pgshift)
{
    struct timespec c1,c2;
    omp_set_num_threads(nthreads);

    cout << "# nthreads " << nthreads
        << " len " << len
        << " pgshift " << pgshift
        << endl;
    cout << "len pgshift time(ns) time(s) rate(ns/pg)" << endl;

    const int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    const int prot  = PROT_READ | PROT_WRITE;
    void *mem = mmap(NULL, len, prot, flags, -1, 0);
    if (mem == MAP_FAILED)
        throw runtime_error(strerror(errno));

    clock_gettime(CLOCK_MONOTONIC, &c1); // TODO per-thread
#pragma omp parallel
    {
#pragma omp for
        for (size_t pg = 0; pg < (len>>pgshift); pg++) {
            *((vlp)((uintptr_t)mem + (pg<<pgshift))) = 42L;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &c2);
    long d= diff(c1,c2);
    printf("% 16ld % 3ld % 16ld % 8lf % 8ld\n",
            len, pgshift, d, (d/1e9),
            (d/(len>>pgshift)) );

    munmap(mem, len);
}

int main(int narg, char *args[])
{
    if (narg != 3)
        return 1;
    int nthreads = strtol(args[1], NULL, 10);
    size_t len   = strtoll(args[2], NULL, 10);
    initomp(nthreads);
    pfault(nthreads, len, 12);
    return 0;
}
