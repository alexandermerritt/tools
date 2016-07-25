#include <assert.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITERS    (1ul<<5)
#define SIZE     (1ul<<31)
#define NSEC2SEC 1000000000ul

typedef unsigned long ul;
typedef struct { ul pad[8]; } cl;

static ul barrier;
static ul accum_bw;
static ul nthreads;

static inline
void arrive_wait() {
    __atomic_sub_fetch(&barrier, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(&barrier, __ATOMIC_SEQ_CST) > 0)
        ;
}

static inline
void depart_wait() {
    __atomic_add_fetch(&barrier, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(&barrier, __ATOMIC_SEQ_CST) < nthreads)
        ;
}

static inline
ul now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec*NSEC2SEC + t.tv_nsec);
}

void work() {
    ul *array = malloc(SIZE);
    assert(array);
    memset(array, 0xdb, SIZE);
    volatile cl *p = (cl*) &array[0];
    arrive_wait();
    ul start = now();
    // TODO update this to use SIMD
    for (ul n = 0; n < ITERS; n++) {
        for (ul i = 0; i < (SIZE>>6); i+=(1ul<<3)) {
            p[i+0].pad[0];
            p[i+1].pad[0];
            p[i+2].pad[0];
            p[i+3].pad[0];
            p[i+4].pad[0];
            p[i+5].pad[0];
            p[i+6].pad[0];
            p[i+7].pad[0];
        }
    }
    depart_wait();
    ul dur = now()-start;
    float sec = (float)dur / NSEC2SEC;
    ul ops = ITERS * (SIZE>>6); // each is 64B
    ul bw = (ops<<6)/sec;
    __atomic_add_fetch(&accum_bw, bw, __ATOMIC_SEQ_CST);
}

long env_threads() {
    if (!getenv("OMP_NUM_THREADS")) {
        fprintf(stderr, "Specify OMP_NUM_THREADS\n");
        exit(EXIT_FAILURE);
    }
    long nthreads = strtol(getenv("OMP_NUM_THREADS"), NULL, 10);
    if (nthreads <= 0) {
        fprintf(stderr, "Invalid OMP_NUM_THREADS: %ld\n", nthreads);
        exit(EXIT_FAILURE);
    }
    return nthreads;
}

int main() {
    nthreads = env_threads();
    __atomic_store_n(&barrier, nthreads, __ATOMIC_SEQ_CST);
    __atomic_store_n(&accum_bw, 0ul, __ATOMIC_SEQ_CST);
    printf("using %lu threads\n", nthreads);
#pragma omp parallel
    {
        work();
    }
    ul bw = __atomic_load_n(&accum_bw, __ATOMIC_SEQ_CST);
    printf("bandwidth: %.2lf GBps\n", bw/(float)(1ul<<30));
    return 0;
}
