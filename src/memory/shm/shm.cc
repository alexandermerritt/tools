#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
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

struct page2m
{
    union {
        char fluff[1<<20];
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
    int nthreads = 8;
    size_t pages_per = (npages / nthreads);
    thread **threads = new thread*[nthreads];
    for (int t = 0; t < nthreads; t++)
        threads[t] = new thread(touch, pages, (t*pages_per), pages_per);
    for (int t = 0; t < nthreads; t++)
        threads[t]->join();
}

int runshm(unsigned int perms, unsigned int flags,
        off64_t start, off64_t end, int niter)
{
    const char sname[] = "/shm-lg";
    size_t *map_times(nullptr);
    size_t *unmap_times(nullptr);
    struct timespec t1, t2;
    size_t diff = 0;
    void *addr;
    int fd;

    int page_shift = 12;
    //start = end = (1UL << 39) - (1UL << 33);
    //start = (1UL << page_shift); end = (253957UL << page_shift);
    start = (1UL << page_shift); end = (1UL<<page_shift);
    printf("# start %ld end %ld\n", start, end);

    flags |= MAP_NORESERVE;

    int tidx = 0;
    map_times = new size_t[niter];
    if (!map_times)
        return 1;
    unmap_times = new size_t[niter];
    if (!unmap_times)
        return 1;

    fd = shm_open(sname, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd < 0)
        return 1;

    printf("# prefaulting %lu bytes (%lu pages)..\n",
            end, (end >> page_shift));
    if (ftruncate(fd, end))
        return 1;
    unsigned int f = MAP_SHARED|MAP_NORESERVE;
    struct page *pages =
        (struct page*)mmap(nullptr, end, PROT_READ|PROT_WRITE, f, fd, 0);
    if (MAP_FAILED == pages) {
        perror("mmap");
        return 1;
    }
    //parallel_touch((struct page*)addr, end>>page_shift);
    for (off64_t pg = 0; pg < (end>>page_shift); pg++) {
        printf("."); fflush(stdout);
        pages[pg].u.value = 0xdead + pg;
    }
    printf("\n");
    munmap(pages, end);

    printf("# running..\n");
    for (off64_t len = start; len <= end; len <<= 1) {
        printf("%lu", len);
        tidx = 0;
        for (int i = 0; i < niter; i++) {
            clock_gettime(CLOCK_REALTIME, &t1);
            addr = mmap(nullptr, len, perms, flags, fd, 0);
            clock_gettime(CLOCK_REALTIME, &t2);
            if (MAP_FAILED == addr) {
                perror("mmap");
                return 1;
            }
            diff = (t2.tv_sec * 1e9 + t2.tv_nsec) -
                (t1.tv_sec * 1e9 + t1.tv_nsec);
            map_times[tidx] = diff;

            clock_gettime(CLOCK_REALTIME, &t1);
            munmap(addr, len);
            clock_gettime(CLOCK_REALTIME, &t2);
            if (MAP_FAILED == addr)
                return 1;
            diff = (t2.tv_sec * 1e9 + t2.tv_nsec) -
                (t1.tv_sec * 1e9 + t1.tv_nsec);
            unmap_times[tidx] = diff;

            tidx++;
        }
        float map_mean = 0, unmap_mean = 0;
        for (int i = 0; i < niter; i++) {
            map_mean += map_times[i];
            unmap_mean += unmap_times[i];
        }
        map_mean /= (float)niter;
        unmap_mean /= (float)niter;
        printf(" %1.2f %1.2f\n", map_mean, unmap_mean);
        //if ((diff / 1e9) >= 1.0f)
            //niter = 1;
    }

    delete [] map_times;
    delete [] unmap_times;
    close(fd);
    //shm_unlink(sname);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    string cmd(argv[1]);
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
    if (start == 0 || end == 0)
        return 1;

    if (runshm(perms, flags, start, end, niter))
        return 1;

    return 0;
}

