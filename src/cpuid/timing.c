// Benchmark latency of various methods to learn the CPU ID.
//
// CPUID gives the initial APIC ID
// RDTSCP gives the "processor ID"
// RDPID is the same as RDTSCP but doesn't give the clock
//
// RDPID not available in Haswell

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/mman.h>

#define always_inline \
	__attribute__((always_inline))

typedef unsigned long ul;

static always_inline
uint64_t rdtscp(void)
{
    uint64_t rax = 0, rdx = 0;
    __asm__ __volatile__ ("rdtscp"
            : "=a" (rax), "=d" (rdx)
            : : "memory");
    return ((rdx << 32) | rax);
}

// extract just the ID; not used for timing
static always_inline
uint64_t rdtscp_id(void)
{
    uint64_t rax = 0, rcx = 0, rdx = 0;
    __asm__ __volatile__ ("rdtscp"
            : "=a" (rax), "=d" (rdx), "=c"(rcx)
            : : "memory");
    return rcx; // processor ID
}

static always_inline
uint64_t cpuid(void)
{
	unsigned int leaf = 0;
	unsigned int subleaf = 0;
    unsigned int eax=1, ebx=0, ecx=0, edx=0;
    __asm__ __volatile__ ("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(leaf), "b"(0), "c"(subleaf), "d"(0)
            :
            "memory");
    return (ebx >> 24) & 0xff; // inital APIC ID
}

void bench() {
    const int iters = 1<<22;

    ul now = rdtscp();
    for (int i = 0; i < iters; i++)
        __asm__ __volatile__ ("nop":::"memory");
    ul end = rdtscp();
    ul loop_cycles = (end-now);

    now = rdtscp();
    for (int i = 0; i < iters; i++)
        (void)cpuid();
    end = rdtscp();
    ul cpuid_cycles = (end-now);

    now = rdtscp();
    for (int i = 0; i < iters; i++)
        rdtscp_id();
    end = rdtscp();
    ul rdtscp_cycles = (end-now);

    asm volatile ("sfence");
    printf("        cpuid: %.2f\n",
            (float)(cpuid_cycles-loop_cycles) / (float)iters);
    printf("       rdtscp: %.2f\n",
            (float)(rdtscp_cycles-loop_cycles) / (float)iters);
}

int main() {
    bench();
}
