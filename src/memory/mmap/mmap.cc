#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <iostream>
#include <thread>

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

struct page
{
    union {
        char fluff[1<<12];
        long value;
    } u;
};

void touch(struct page *pages, off64_t start, size_t npages)
{
    printf("thread start %lu npages %lu\n", start, npages);
    for (size_t pg = start; pg < (start+npages); pg++)
        pages[pg].u.value = 0xdead + pg;
}

void parallel_touch(struct page *pages, size_t npages)
{
    int nthreads = 1;
    size_t pages_per = (npages / nthreads);
    thread **threads = new thread*[nthreads];
    for (int t = 0; t < nthreads; t++)
        threads[t] = new thread(touch, pages, (t*pages_per), pages_per);
    for (int t = 0; t < nthreads; t++)
        threads[t]->join();
}

int dommap(size_t len, size_t nreg, size_t niter,
        unsigned int flags, unsigned int perms)
{
    struct timespec t1, t2;
    long diff;

    if (pin(0))
        return 1;

    printf("# attempting %lu bytes\n", len);
    size_t next = 0;
    //void **addrs = new void* [len];
    //if (!addrs)
        //return 1;

    len = (254254UL << 20);

    struct page *pages = (struct page*)mmap(NULL, len, perms, flags, -1, 0);
    if (pages == MAP_FAILED)
        return 1;
    //parallel_touch(pages, (len >> 20));
    //for (size_t pg = 0; pg < (len >> 20); pg++)
        //pages[pg].u.value = 0xdead + pg;

    return 0;
    printf("total_ns len nregions niter r w pop nores\n");
    for (unsigned int i = 0; i < niter; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        for (unsigned int i = 0; i < nreg; i++)
            //if ((addrs[next++] = mmap(NULL, len, perms, flags, -1, 0))
            if ((mmap(NULL, len, perms, flags, -1, 0)) == MAP_FAILED)
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
        //for (unsigned int pg = 0; pg < nreg; pg++)
            //munmap(addrs[--next], len);
    }

    printf("sleeping..\n");
    sleep(3600);
    return 0;
}

int runmmap(void)
{
    unsigned int flags, perms;
    size_t len, nreg, niter;

    len   = (1UL << 38);
    //nreg  = (1UL << 12);
    nreg  = 1;
    niter = 1;

    flags  = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
    //flags |= MAP_POPULATE;
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
        clock_gettime(CLOCK_REALTIME, &t2);
        if (MAP_FAILED == addr)
            return nullptr;

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
int runmmapfs(unsigned int perms, unsigned int flags,
        off64_t start, off64_t end, int niter)
{
    size_t *times(nullptr);
    const char fname[] = "/tmp/map_me";

    flags |= MAP_NORESERVE;

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

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    string cmd(argv[1]);
    if ("anon" == cmd) {
        if (runmmap())
            return 1;
    } else if ("file" == cmd) {
        unsigned int perms = 0;
        unsigned int flags = 0;
        int niter = 1;
        off64_t start = 0, end = 0;
        for (int i = 2; i < argc; i++) {
            string arg(argv[i]);
            if ("r" == arg)
                perms |= PROT_READ;
            else if ("w" == arg)
                perms |= PROT_WRITE;
            else if ("shared" == arg)
                flags |= MAP_SHARED;
            else if ("private" == arg)
                flags |= MAP_PRIVATE;
            else if ("pop" == arg)
                flags |= MAP_POPULATE;
            else if ('S' == arg[0])
                start = 1UL<<strtol(&arg[1], nullptr, 10);
            else if ('E' == arg[0])
                end = 1UL<<strtol(&arg[1], nullptr, 10);
            else if ('I' == arg[0])
                niter = strtol(&arg[1], nullptr, 10);
            else
                return 1;
        }

        if (runmmapfs(perms, flags, start, end, niter))
            return 1;
    } else
        return 1;

    return 0;
}

