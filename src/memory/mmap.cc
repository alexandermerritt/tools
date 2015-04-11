#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <iostream>

using namespace std;

#define LEN (1<<29)

#ifdef __linux__
int pin(int cpu)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    return sched_setaffinity(getpid(), sizeof(mask), &mask);
}
#endif

int main(void)
{
    uint32_t flags = MAP_PRIVATE|MAP_ANONYMOUS;
#ifdef __unix__
    flags |= MAP_PREFAULT_READ;
    //flags |= MAP_ALIGNED_SUPER;
#elif __linux__
    flags |= MAP_POPULATE;
#else
#error Unsupported OS
#endif

    printf("Waiting before mapping...\n");
    sleep(10);
    printf("running...\n");

    void *addr = mmap(NULL, LEN, PROT_READ|PROT_WRITE,
            flags, -1, 0);
    if (MAP_FAILED == addr) {
        perror("mmap");
        return 1;
    }

#ifdef __linux__
    if (pin(0))
        return 1;
#endif

    struct timespec t1, t2;
    clock_gettime(CLOCK_REALTIME, &t1);
    volatile long v;
    long *vals = (long*)addr;
    for (int x = 0; x < (1<<5); x++)
        for (int i = 0; i < (LEN >> 3); i++)
            v = vals[i];
    clock_gettime(CLOCK_REALTIME, &t2);

    long diff = (t2.tv_sec * 1e9 + t2.tv_nsec) -
        (t1.tv_sec * 1e9 + t1.tv_nsec);

    printf("%ld\n", (diff / 1000));
    sleep(20);

    munmap(addr, LEN);
    return 0;
}
