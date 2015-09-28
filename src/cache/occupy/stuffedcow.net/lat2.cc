/* 
  Henry Wong <henry@stuffedcow.net>
  http://blog.stuffedcow.net/2013/01/ivb-cache-replacement/
  
  Dual pointer chase. Reuse each cache line exactly once before eviction.
  
  2014-10-14
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
inline unsigned long long int rdtsc()
{
	unsigned int lo, hi;

	__asm__ volatile (".byte 0x0f, 0x31" : "=a" (lo), "=d" (hi));
	return (long long)(((unsigned long long)hi)<<32LL) | (unsigned long long) lo;
}

void **array;
void * dummy;

#define MIN_ITS	1024
#define SPEED 67108864 
//33554432


enum _tests {
	TEST_RANDLAT = 0
};

struct _options
{
	bool hugetlb;
	long long sizelimit;
	enum _tests test;
	int cycle_len;
	int stepahead;
} opts;

static inline unsigned long long my_rand (unsigned long long limit)
{
	return ((unsigned long long)(((unsigned long long)rand()<<48)^((unsigned long long)rand()<<32)^((unsigned long long)rand()<<16)^(unsigned long long)rand())) % limit;
}

static double run_randlat(long long size, int ITS)
{
	static long long last_size = 1;
	static bool fastmode = false;
	static void* pos = &array[0];
	volatile static void* pos2 = &array[0];

	double clocks_per_it;
	
	if (size <= last_size) 
	{
		printf ("Panic: size must be increasing\n");
		return 0;
	}
	
	if (ITS <= MIN_ITS)
		fastmode = true;

	if (last_size == 1)	//first iteration
		array[0] = &array[0];
	for (long long i=last_size;i<size;i++)
		array[i] = &array[i];
		
	if (!fastmode)
	{
		int cycle_length = (opts.cycle_len >= 1 ? opts.cycle_len : 1);
	
		//Use a variation on Sattolo's algorithm to incrementally generate a random cyclic permutation that increases in size each time.
		for (long long i=size-1;i>=last_size;i--)
		{
			if (i < cycle_length) continue;
			unsigned int k = my_rand(i/cycle_length) * cycle_length + (i%cycle_length);
			void* temp = array[i];
			array[i] = array[k];
			array[k] = temp;
		}
		register void *j = pos, *k = pos;
		putchar (' '); fflush(stdout);

		// Dual pointer chase with some offset to foil adaptive cache replacement policy, if any. 
		// Touches each address twice with some delay between them to make
		// adaptive replacement policy think there is some reuse.
		// Assume latency-bound, not bandwidth-bound.
		// > 256K/ cycle_len but smaller than ~6M/cycle_len
		for (int i=0;i<opts.stepahead; i++)
			k = *(void**)k;

		long long start = rdtsc();
		
		for (int i=ITS;i;i--)
		{
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;
			k = *(void**)k;
			j = *(void**)j;			
			k = *(void**)k;
		}	
		
		long long stop = rdtsc();
		clocks_per_it =  (double)(stop-start)/(ITS*16);
		pos = j;
		pos2 = k;
	}
	else
	{
		//Alternative method with more error for HD swapping. Error = ~6-20 cycles?
		
		static void *addrs[MIN_ITS*16];
		for (int i=0;i<MIN_ITS*16;i++)
			addrs[i] = array + my_rand(size);
		putchar (' '); fflush(stdout);

		long long start = rdtsc();
		
		register long long j = 0;
		for (int i=(MIN_ITS-1)*16;i>=0;i-=16)
		{
			j &= *(long long*)addrs[i+j];
			j &= *(long long*)addrs[i+1+j];
			j &= *(long long*)addrs[i+2+j];
			j &= *(long long*)addrs[i+3+j];
			j &= *(long long*)addrs[i+4+j];
			j &= *(long long*)addrs[i+5+j];
			j &= *(long long*)addrs[i+6+j];
			j &= *(long long*)addrs[i+7+j];
			j &= *(long long*)addrs[i+8+j];
			j &= *(long long*)addrs[i+9+j];
			j &= *(long long*)addrs[i+10+j];
			j &= *(long long*)addrs[i+11+j];
			j &= *(long long*)addrs[i+12+j];
			j &= *(long long*)addrs[i+13+j];
			j &= *(long long*)addrs[i+14+j];
			j &= *(long long*)addrs[i+15+j];
		}	
		
		long long stop = rdtsc();
		clocks_per_it =  (double)(stop-start)/(MIN_ITS*16);
		dummy = (void*)j;
	}

	last_size = size;

	
	//printf ("%5d KB: clocks = %f\n", size*(sizeof(void*))/1024, (double)(stop-start)/(ITS<<4), j);
	
	
	printf ("%lld\t%f\n", size*sizeof(void*), clocks_per_it, dummy);	//passing dummy to prevent optimization
	fflush(stdout);
	return clocks_per_it;
}   


static double runtest(struct _options opts, int size, int ITS)
{
	sched_yield();
	switch (opts.test)
	{
		case TEST_RANDLAT: return run_randlat(size, ITS);
		default: printf ("Invalid test %d\n", opts.test);
	}
	return 0.;
}



void parse_args (int argc, char ** argv, struct _options &opts)
{
#define NEXTARG do { if (++i >= argc) { printf ("argument required for '%s'\n", argv[i-1]); exit(1); }} while(0)
	//First argument must be array size.
	sscanf (argv[1], "%lli", &opts.sizelimit);
	
	
	for (int i=2;i<argc;i++)
	{
		if (!strcmp ("hugetlb", argv[i]))
		{
			opts.hugetlb = true;
		}
		else if (!strcmp ("-t", argv[i]))
		{
			NEXTARG;
			if (!strcmp (argv[i], "randlat"))
				opts.test = TEST_RANDLAT;
			else
			{
				printf ("-t invalid test\n");
				printf ("Must be one of: randlat\n");
				exit (1);
			}
		}
		else if (!strcmp ("-cyclelen", argv[i]))
		{
			NEXTARG;
			opts.cycle_len = 0;
			sscanf (argv[i], "%i", &opts.cycle_len);
			if (opts.cycle_len == 0)
			{
				printf ("-cyclelen <cycle length>\n");
				exit(1);
			}
		}
		else if (!strcmp("-seed", argv[i]))
		{
			NEXTARG;
			int seed = 0;
			sscanf (argv[i], "%i", &seed);
			srand(seed);
		}
		else
		{
			printf ("Invalid argument '%s'\n", argv[i]);
			exit (1);
		}
	}
	
}
#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
int main(int argc, char ** argv)
{
	if (argc < 2) {
		printf ("Usage: lat <array size> [hugetlb]\n");
		return 1;
	}
	
	memset(&opts, 0, sizeof(opts));
	
	opts.cycle_len = 1;
	parse_args(argc, argv, opts);
	
	switch (opts.test)
	{
		case TEST_RANDLAT: printf ("Random access latency test\n"); break;
		default: printf ("invalid test\n"); return 1;
	}
	

	int fd;
	
	printf ("Size limit = %lld bytes\n", opts.sizelimit);
	
	if (opts.hugetlb)
	{
		fd = open ("/mnt/test/tempfile", O_CREAT | O_RDWR, 0755);
		if (fd < 0) { printf ("Failed to open /mnt/test/tempfile\n"); return 1;}
		printf ("Using hugepages\n");
		array = (void**)mmap (0, opts.sizelimit, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		//array = (void**)mmap (0, opts.sizelimit, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_HUGETLB|MAP_ANONYMOUS, 0, 0);
		if (array == MAP_FAILED) 
		{
			printf ("mmap failed (%d). hugetlbfs not mounted in /mnt/test? Array not a multiple of page size?\n", errno);
			perror("mmap");
			unlink("/mnt/test/tempfile");
			return 1;
		}
	}
	else
	{
		printf ("Using default page size\n");
		array = new void*[opts.sizelimit/sizeof(void*)];	
	}
	
	// Hard-coded for SNB/IVB cache hierarchy. Want to hit L3 cache.
	opts.stepahead = 524288/(opts.cycle_len*sizeof(void*));
	if (opts.stepahead < 10) opts.stepahead = 10;
	
	int its = SPEED;
	//704643072  
	//512 8
	for (long long size = 512, step=8; size  <= opts.sizelimit/sizeof(void*); size = size + step )
	{
		double cpi = runtest(opts, size, its);
		if (!((size-1) & size)) step <<=1;
		its = (int)(SPEED/cpi);
		if (its < MIN_ITS) its = MIN_ITS;
	}

	
	if (opts.hugetlb)
	{
		munmap (array, opts.sizelimit);
		unlink ("/mnt/test/tempfile");
		close (fd);
	}
	else
	{
		delete[] array;
	}
	
}   
