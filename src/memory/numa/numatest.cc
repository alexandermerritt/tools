/*
 * Small microbenchmark to evaluate NUMA latencies on large-memory
 * servers.
 *
 * Requires use of the ncmap.c kernel module to disable CPU caches on
 * R/W mmap regions.
 *
 * This benchmark is so simple because caches are disabled.
 * Prefetchers won't be used, no need to try and disable them, or
 * out-smart them with a random-access linked-list traversal.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// c includes
#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>

// c++ includes
#include <iostream>

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

int vma(void)
{
    const int ncpus = 480;
    const unsigned long niters = 100000UL;
    struct timespec t1, t2;

    unsigned int flags = MAP_PRIVATE|MAP_ANONYMOUS;
    flags |= MAP_LOCKED|MAP_POPULATE; // locked may require sudo

    void *addr = mmap(NULL,MMAP_SZ,PROT_READ|PROT_WRITE,flags,-1,0);
    if (MAP_FAILED == addr) {
        perror("mmap");
        return 1;
    }

    cout << "cpu ns" << endl;
    for (int cpu = 0; cpu < ncpus; cpu++) {
        if (pin(cpu))
            return 1;

        clock_gettime(CLOCK_REALTIME, &t1);
        volatile unsigned long v;
        for (unsigned long i = 0; i < niters; i++)
            v = *((volatile unsigned long*)addr);
        clock_gettime(CLOCK_REALTIME, &t2);

        size_t ns = (t2.tv_sec * 1e9 + t2.tv_nsec)
            - (t1.tv_sec * 1e9 + t1.tv_nsec);
        cout << cpu << " " << (ns/(double)niters) << endl;

        (void)v; // hush compiler
    }

    munmap(addr, MMAP_SZ);
    return 0;
}

// may also want to configure a simple run for use with likwid to
// measure CPU counters

int main(void)
{
    if (vma()) return 1;
    //if (bw()) return 1;
    return 0;
}
