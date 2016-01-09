/*
 * tlbtest.cc
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// c includes
#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <likwid.h>

// c++ includes
#include <iostream>
#include <random>

#define PAGE_SHIFT  (12)
#define MMAP_SZ     (1UL << 20)
#define MMAP_PGS    (MMAP_SZ >> PAGE_SHIFT)

using namespace std;

int pin(int cpu)
{
    int err = 0;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    if ((err = sched_setaffinity(getpid(), sizeof(mask), &mask)))
        perror("sched_setaffinity");
    return err;
}

struct page12 {
    union {
        unsigned int idx;
        char pad[1 << 12];
    } u;
};

int tlb(size_t bytes, size_t page_shift)
{
    struct timespec t1, t2;

    LIKWID_MARKER_THREADINIT;

    unsigned int flags = MAP_PRIVATE|MAP_ANONYMOUS;
    flags |= MAP_LOCKED|MAP_POPULATE; // locked may require sudo

    if (pin(0))
        return 1;

    cout << "# mmap" << endl;
    //LIKWID_MARKER_START("region_mmap");
    void *addr = mmap(NULL,bytes,PROT_READ|PROT_WRITE,flags,-1,0);
    if (MAP_FAILED == addr) {
        perror("mmap");
        return 1;
    }
    //LIKWID_MARKER_STOP("region_mmap");

    unsigned int npages = (bytes >> page_shift) - 1;
    unsigned int *pidx = new unsigned int[npages];
    if (!pidx)
        return 1;

    // generate random sequence of 
    cout << "# generating idx" << endl;
    //LIKWID_MARKER_START("region_rand");
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<unsigned long> dist(0, npages);
    for (unsigned int pg = 0; pg < npages; pg++)
        pidx[pg] = (npages - pg - 1);
    cout << "# swapping idx" << endl;
    for (unsigned int pg = 0; pg < (npages << 6); pg++)
        swap(pidx[dist(gen)], pidx[dist(gen)]);
    //LIKWID_MARKER_STOP("region_rand");

    // put the indexes into the area directly
    cout << "# injecting idx" << endl;
    //LIKWID_MARKER_START("region_inject");
    volatile struct page12 *pages = (struct page12 *)addr;
    for (unsigned int pg = 0; pg < npages; pg++)
        pages[pg].u.idx = pidx[pg];
    //LIKWID_MARKER_STOP("region_inject");

    delete pidx;
    pidx = nullptr;

    // perform the test
    cout << "# executing..." << endl;
    LIKWID_MARKER_START("region_exec");
    clock_gettime(CLOCK_REALTIME, &t1);
    unsigned int niters = (npages << 5);
    if (niters < 4096)
        niters = 4096;
    unsigned int next = 0;
#define TOUCH   next = pages[next].u.idx
#define N2      TOUCH ; TOUCH
#define N8      N2;N2;N2;N2
#define N64     N8;N8;N8;N8;N8;N8;N8;N8
    for (unsigned int i = 0; i < niters; i++) {
        N64;
    }
    clock_gettime(CLOCK_REALTIME, &t2);
    LIKWID_MARKER_STOP("region_exec");

    size_t ns = (t2.tv_sec * 1e9 + t2.tv_nsec)
        - (t1.tv_sec * 1e9 + t1.tv_nsec);
    cout << (ns/(double)niters) << endl;

    munmap(addr, bytes);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s npages pinYN\n", *argv);
        return 1;
    }

    unsigned long npages = strtol(argv[1], NULL, 10);
    bool pin = (argv[2][0] == 'y');

    LIKWID_MARKER_INIT;

    if (pin) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        if (sched_setaffinity(getpid(), sizeof(mask), &mask))
            return 1;
    }

    if (tlb(npages << 12, 12)) return 1;

    LIKWID_MARKER_CLOSE;

    return 0;
}
