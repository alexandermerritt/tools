#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <iostream>

#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int pin(int cpu)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    return sched_setaffinity(getpid(), sizeof(mask), &mask);
}

int dommap(size_t len, size_t nreg, size_t niter,
        unsigned int flags, unsigned int perms)
{
    struct timespec t1, t2;
    long diff;

    if (pin(0))
        return 1;

    size_t next = 0;
    void **addrs = new void* [len];
    if (!addrs)
        return 1;

    printf("total_ns len nregions niter r w pop nores\n");
    for (unsigned int i = 0; i < niter; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        for (unsigned int i = 0; i < nreg; i++)
            if ((addrs[next++] = mmap(NULL, len, perms, flags, -1, 0))
                    == MAP_FAILED)
                return 1;
        clock_gettime(CLOCK_REALTIME, &t2);
        diff = (t2.tv_sec * 1e9 + t2.tv_nsec) -
            (t1.tv_sec * 1e9 + t1.tv_nsec);
        int npages = (nreg * (len >> 12));
        printf("%ld %lu %lu %lu %d %d %d %d\n", diff,
                len, nreg, niter,
                !!(PROT_READ & perms),
                !!(PROT_WRITE & perms),
                !!(MAP_POPULATE & flags),
                !!(MAP_NORESERVE & flags));
        for (unsigned int pg = 0; pg < nreg; pg++)
            munmap(addrs[--next], len);
    }

    return 0;
}

int runmmap(void)
{
    unsigned int flags, perms;
    size_t len, nreg, niter;

    len   = (1UL << 20);
    nreg  = (1UL << 12);
    niter = 1;

    flags  = MAP_PRIVATE | MAP_ANONYMOUS;
    flags |= MAP_POPULATE;
    //flags |= MAP_UNINITIALIZED;
    flags |= MAP_NORESERVE;

    perms = PROT_READ | PROT_WRITE;

    if (dommap(len, nreg, niter, flags, perms))
        return 1;

    return 0;
}

// TODO how to use O_DIRECT?
size_t * dorunmmapfs(const char *fname, off64_t len, unsigned int perms, unsigned int flags, int niter)
{
    struct timespec t1, t2;
    size_t diff;
    void *addr;

    int tidx = 0;
    size_t *times = new size_t[niter];
    if (!times)
        return nullptr;

        int fd = open(fname, O_RDWR|O_CREAT|O_LARGEFILE, S_IRUSR|S_IWUSR);
        if (fd < 0)
            return nullptr;

    for (int i = 0; i < niter; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        addr = mmap(nullptr, len, perms, flags, fd, 0);
        if (MAP_FAILED == addr)
            return nullptr;
        clock_gettime(CLOCK_REALTIME, &t2);

        diff = (t2.tv_sec * 1e9 + t2.tv_nsec) -
            (t1.tv_sec * 1e9 + t1.tv_nsec);
        times[tidx++] = diff;

        //munmap(addr, len);
    }
        close(fd);

    return times;
}

// Create a file on disk first, large enough to be mapped (e.g. 32G)
//      dd if=/dev/zero of=/tmp/map_me bs=1M count=$((1<<15))
int runmmapfs(void)
{
    unsigned int perms = 0;
    unsigned int flags = 0;
    size_t *times(nullptr);
    const char fname[] = "/tmp/map_me";

    perms = PROT_READ | PROT_WRITE;
    //perms = PROT_READ;
    //perms = PROT_WRITE;
    //perms = PROT_NONE;

    //flags |= MAP_PRIVATE;
    flags |= MAP_SHARED;
    flags |= MAP_POPULATE;
    flags |= MAP_NORESERVE;

    int niter = 16;
    const off64_t start = (1<<30), end = (1UL<<32);

    for (off64_t len = start; len <= end; len <<= 1) {
        if (!(times = dorunmmapfs(fname, len, perms, flags, niter)))
            return 1;
        printf("%lu", len);
        for (int i = 0; i < niter; i++)
            printf(" %lu", times[i]);
        printf("\n");
        delete [] times;
    }

    return 0;
}

int main(void)
{
    //if (runmmap())
        //return 1;

    if (runmmapfs())
        return 1;

    return 0;
}

