//
// Intel Manual 4.10.4 talks about TLB invalidation
//

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <deque>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <iostream>
#include <random>

using namespace std;

enum
{
    PAGE_SHIFT = 12, // XXX set this (12, 21, 30)
};

struct page
{
    union {
        char fill[1<<PAGE_SHIFT];
        long i;
    } u;
};

// return nanoseconds
static size_t clockdiff(struct timespec &t1, struct timespec &t2)
{
    size_t time1 = (t1.tv_sec * 1e9 + t1.tv_nsec);
    size_t time2 = (t2.tv_sec * 1e9 + t2.tv_nsec);
    return (time2 - time1);
}

static int pincpu(int cpu)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    size_t cpusetsize = sizeof(cpu_set_t);
    if (sched_setaffinity(getpid(), cpusetsize, &mask))
        return 1;
    return 0;
}

// returns ns elapsed
static size_t _runtest(void *area, size_t ws, size_t iters)
{
    struct page *pagearea = static_cast<struct page*>(area);
    struct timespec start,end;

    std::random_device rd;
    std::mt19937_64 m(rd());
    std::uniform_int_distribution<int> dis(0,ws-1);

    // get some random sequence to use
    int *idxs = new int[ws];
    for (size_t pgidx = 0; pgidx < ws; pgidx++)
        idxs[pgidx] = pgidx;
    for (size_t pgidx = 0; pgidx < ws; pgidx++)
        swap(idxs[dis(m)], idxs[dis(m)]);

    volatile long i; // MUST BE volatile if compiler has -O > 0
    clock_gettime(CLOCK_REALTIME, &start);
    for (size_t iter = 0; iter < iters; iter++)
        for (size_t pgidx = 0; pgidx < ws; pgidx++)
            i = pagearea[idxs[pgidx]].u.i;
    clock_gettime(CLOCK_REALTIME, &end);

    delete [] idxs;
    return clockdiff(start,end);
}

// shiva D-TLB and STLB combined can translate ~9MB window
static void runtest(void)
{
    const size_t areasz     = (1UL << 30);
    const size_t pgs_from   = 1;
    const size_t pgs_to     = (areasz >> PAGE_SHIFT);
    const size_t pgs_incr   = (PAGE_SHIFT == 12 ? 16 : 1);
    const size_t iters      = (PAGE_SHIFT == 12 ? 64 : 1024);

    const int    huge       = 0; //MAP_HUGETLB;

    if (pincpu(0))
        exit(EXIT_FAILURE);

    void *addr = mmap(nullptr, areasz, PROT_READ|PROT_WRITE,
            MAP_ANON|MAP_PRIVATE|MAP_POPULATE|MAP_LOCKED|huge, -1, 0);
    if (MAP_FAILED == addr) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    cout << "pgs ns1k" << endl;
    size_t ns;
    for (size_t ws = pgs_from; ws < pgs_to; ws += pgs_incr) {
        ns = _runtest(addr, ws, iters);
        cout << ws << " " << (ns * 1024 / (ws * iters)) << endl;
    }

    munmap(addr, areasz);
}

int main(void)
{
    runtest();
    return 0;
}
