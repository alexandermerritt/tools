//
// Intel Manual 4.10.4 talks about TLB invalidation
//
// Tell Linux to make large pages available:
//      echo N > /proc/sys/vm/nr_hugepages
//

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <deque>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sched.h>
#include <iostream>
#include <random>

using namespace std;

const size_t MAX_MEM = (1UL << 33);

//
// Structures
//

enum
{
    PAGE_4K = 12,
    PAGE_2M = 21,
    // TODO PAGE_1G = 30,
};

struct page4k
{
    union {
        char fill[1<<12];
        long i;
    } u;
};

struct page2m
{
    union {
        char fill[1<<21];
        long i;
    } u;
};

struct page1g
{
    union {
        char fill[1<<30];
        long i;
    } u;
};

//
// Functions
//

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
template <typename PG_TYPE>
static size_t _runtest(void *area, size_t area_sz, size_t ws, size_t iters)
{
    PG_TYPE *pg_area = static_cast<PG_TYPE*>(area);
    struct timespec start,end;

    if ((ws * sizeof(PG_TYPE)) > area_sz)
        throw runtime_error("area size too small for working set");

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
            i = pg_area[idxs[pgidx]].u.i;
    clock_gettime(CLOCK_REALTIME, &end);

    delete [] idxs;
    return clockdiff(start,end);
}

// shiva D-TLB and STLB combined can translate ~9MB window
template <int PAGE_ORDER>
static void runtest(int wset_low, int wset_high, int pgs_incr)
{
    const size_t pg_sz      = (1UL << PAGE_ORDER);
    const size_t area_sz    = (wset_high * pg_sz);
    //const size_t pgs_incr   = 1; //(PAGE_ORDER > 12 ? 1 : 16);
    const size_t iters      = 512; //(PAGE_ORDER > 12 ? 1024 : 64);
    const int report_per    = 1024;

    // XXX linux doesn't do 1G pages yet
    const size_t huge       = (PAGE_ORDER == PAGE_2M ? MAP_HUGETLB : 0);

    if ((wset_low % pgs_incr) || (wset_high % pgs_incr))
        throw runtime_error("working set low/high"
                " must be multiple of increment");

    if (pincpu(0))
        throw runtime_error("could not pin cpu");

    void *area = mmap(nullptr, area_sz, PROT_READ|PROT_WRITE,
            MAP_ANON|MAP_PRIVATE|MAP_POPULATE|MAP_LOCKED|huge, -1, 0);
    if (MAP_FAILED == area) {
        perror("mmap");
        if (huge)
            cerr << "Note: 2mb pages requires linux reserve"
                " pages for allocation:" << endl
                << "       set this via echo N "
                "> /proc/sys/vm/nr_hugepages" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "pgs time_ns" << endl;
    size_t ns;
    for (int ws = wset_low; ws <= wset_high; ws += pgs_incr) {
        switch (PAGE_ORDER) {
            case PAGE_4K: {
                ns = _runtest<page4k>(area, area_sz, ws, iters);
            } break;
            case PAGE_2M: {
                ns = _runtest<page2m>(area, area_sz, ws, iters);
            } break;
            // TODO case PAGE_1G:
            default:
                assert(!"page order invalid");
        }
        cout << ws << " " << (ns * report_per / (ws * iters)) << endl;
    }

    munmap(area, area_sz);
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        cerr << "Usage: " << *argv << " "
            << "page_order pages_low "
            "pages_high pages_increment" << endl;
        return 1;
    }

    const int order     = atoi(argv[1]);
    const int pgs_low   = atoi(argv[2]);
    const int pgs_high  = atoi(argv[3]);
    const int pgs_incr  = atoi(argv[4]);

    if (order < 1 || pgs_low < 1 || pgs_high < 1 || pgs_incr < 1)
        return 1;
    if (pgs_low > pgs_high)
        return 1;
    if ((pgs_high * (1UL << order)) > MAX_MEM)
        return 1;

    if (order == PAGE_4K)
        runtest<PAGE_4K>(pgs_low, pgs_high, pgs_incr);
    else if (order == PAGE_2M)
        runtest<PAGE_2M>(pgs_low, pgs_high, pgs_incr);
    else
        return 1;

    return 0;
}
