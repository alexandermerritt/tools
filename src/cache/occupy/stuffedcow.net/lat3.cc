/* 
  Henry Wong <henry@stuffedcow.net>
  http://blog.stuffedcow.net/2013/01/ivb-cache-replacement/
  
  Scan resistance: Pointer chase a small and big array and see whether
    the cache prefers caching the small array.
   Must use -rripmode with -cyclelen parameters.
   Must use hugetlb (small pages randomizes upper address bits)
   Must use array size > 4194304*sizeof(void*) (See line 68)
   
   Use cyclelen = 16384 for 64bit, 32768 for 32bit (128KB)
   Use startoffset to target one of the dedicated sets.
   lat3 536870912 hugetlb -cyclelen 16384 -rripmode -startoffset ___
  
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
//#define SPEED 67108864 
#define SPEED 33554432


enum _tests {
	TEST_RANDLAT = 0
};

struct _options
{
	bool hugetlb;
	long long sizelimit;
	long long sizelimit2;
	enum _tests test;
	int cycle_len;
	unsigned long long start_offset, stop_offset;
	bool offset_mode;
	bool random_offset_mode;
	bool rrip_mode;
} opts;

static inline unsigned long long my_rand (unsigned long long limit)
{
	return ((unsigned long long)(((unsigned long long)rand()<<48)^((unsigned long long)rand()<<32)^((unsigned long long)rand()<<16)^(unsigned long long)rand())) % limit;
}

void *pos = 0, *pos2 = 0;;

static double run_randlat(long long size, long long size2, int ITS)
{
	size = (size-1) | 1;	// size must be odd
	size2 &= ~1;	// size2 must be even

	static long long last_size = 1, last_size2 = 0;	// size for odd indices, size2 for even indices
	static bool fastmode = false;
	
	// Two arrays. One located within the first 16M/32M (4M*sizeof(void*)), the other occupying the rest of the array.
	if (!pos) pos = &array[opts.start_offset+4194304];
	if (!pos2) pos2 = &array[opts.start_offset+1];

	double clocks_per_it;
	
	if (size < last_size) 
	{
		printf ("Panic: size must be non-decreasing (%lld, %lld)\n", size, last_size);
		return 0;
	}
	if (size2 < last_size2) 
	{
		printf ("Panic: size2 must be non-decreasing (%lld, %lld)\n", size2, last_size2);
		return 0;
	}
	
	fastmode = false;	// Slow mode code has been removed.

	long long max_last_size = last_size > last_size2 ? last_size : last_size2;
	long long max_size = size > size2 ? size : size2;
	if (max_last_size == 1)	//first iteration
		array[0] = &array[0];
	for (long long i=max_last_size;i<max_size;i++)
		array[i] = &array[i];
		
	if (!fastmode)
	{
		int cycle_length = (opts.cycle_len >= 1 ? opts.cycle_len : 1);
	
		//Use a variation on Sattolo's algorithm to incrementally generate a random cyclic permutation that increases in size each time.
		for (long long i=size-1;i>=last_size;i-=2)
		{
			if (i < cycle_length+4194304) continue;
			unsigned int k = my_rand((i-4194304)/cycle_length) * cycle_length + 4194304 + (i%cycle_length);
			void* temp = array[i];
			array[i] = array[k];
			array[k] = temp;
		}
		for (long long i=size2-1;i>=last_size2;i-=2)
		{
			if (i < cycle_length) continue;
			unsigned int k = my_rand(i/cycle_length) * cycle_length + (i%cycle_length);
			void* temp = array[i];
			array[i] = array[k];
			array[k] = temp;
		}		
		
		register void* j = pos, *k=pos2;
		putchar (' '); fflush(stdout);

		for (int i=0;i<20; i++)
		{
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
			k = *(void**)k;
		}

		long long start = rdtsc();
		
		for (int i=ITS;i;i--)
		{
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");
			j = *(void**)j; asm ("lfence");
			k = *(void**)k; asm ("lfence");

		}	
		
		long long stop = rdtsc();
		clocks_per_it =  (double)(stop-start)/(ITS*16);
		pos = j;
		pos2 = k;
	}


	last_size = size;
	last_size2 = size2;

	//printf ("pos = %p, pos2 = %p\n", pos, pos2);
	
	//printf ("%5d KB: clocks = %f\n", size*(sizeof(void*))/1024, (double)(stop-start)/(ITS<<4), j);
	
	
	if (opts.offset_mode)
		printf ("%f\n", clocks_per_it, dummy);	//passing dummy to prevent optimization
	else
		printf ("%lld,%lld\t%f\n", size*sizeof(void*), size2*sizeof(void*), clocks_per_it, dummy);	//passing dummy to prevent optimization
	fflush(stdout);
	return clocks_per_it;
}   


static double runtest(struct _options opts, int size, int ITS)
{
	sched_yield();
	switch (opts.test)
	{
		case TEST_RANDLAT: return run_randlat(size, size, ITS);
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
		else if (!strcmp ("-startoffset", argv[i]))
		{
			NEXTARG;
			sscanf (argv[i], "%llu", &opts.start_offset);
		}
		else if (!strcmp ("-stopoffset", argv[i]))
		{
			NEXTARG;
			sscanf (argv[i], "%llu", &opts.stop_offset);
		}
		else if (!strcmp ("-offsetmode", argv[i]))
		{
			opts.offset_mode = true;	// Run only maximum size, and sweep offset from startoffset to stopoffset
		}
		else if (!strcmp ("-randoffsetmode", argv[i]))
		{
			opts.random_offset_mode = true;	// Run only maximum size, and sweep offset from startoffset to stopoffset
		}
		else if (!strcmp ("-rripmode", argv[i]))
		{
			opts.rrip_mode = true;		// fancy dual pointer chase while sweeping the *size* of one of the cycles.
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
	
	parse_args(argc, argv, opts);

	if (opts.stop_offset == 0) opts.stop_offset = opts.cycle_len;
	
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
	
	printf ("Allocated array at %p\n", array);
	
	for (long long i=0;i<opts.sizelimit/sizeof(void*);i++)
		array[i] = &array[i];

	
	int its = SPEED;
	//704643072  
	//512 8 <-- default
	
	if (opts.offset_mode)
	{
		printf ("Broken, do not use\n");
		long long size = opts.sizelimit/sizeof(void*);
		its = SPEED/64;
				
		// Sweep offsets.
		for (long long i = opts.start_offset ; i < opts.stop_offset; i+= 64/sizeof(void*))	//hard-coded for 64-byte cache lines
		{
			printf ("%lld\t", i);
			pos = &array[i];
			double cpi = runtest(opts, size, its);
			its = (int)(SPEED/cpi);
			if (its < MIN_ITS) its = MIN_ITS;
		}
		
		for (long long i = opts.stop_offset ; i > opts.start_offset; i-= 64/sizeof(void*))	//hard-coded for 64-byte cache lines
		{
			printf ("%lld\t", i);
			pos = &array[i];
			double cpi = runtest(opts, size, its);
			its = (int)(SPEED/cpi);
			if (its < MIN_ITS) its = MIN_ITS;
		}		
	}
	else if (opts.random_offset_mode)
	{
		printf ("Broken, do not use\n");
		long long size = opts.sizelimit/sizeof(void*);
		its = SPEED/64;
				
		for (long long i = opts.start_offset ; i < opts.stop_offset; i+= 64/sizeof(void*))	//hard-coded for 64-byte cache lines
		while (true)
		{
			long long i = my_rand(opts.stop_offset);
			printf ("%lld\t", i);
			pos = &array[i];
			double cpi = runtest(opts, size, its);
			its = (int)(SPEED/cpi);
			if (its < MIN_ITS) its = MIN_ITS;
		}
	}
	else if (opts.rrip_mode)
	{
		long long size = opts.sizelimit/sizeof(void*);
		long long sizelim = opts.sizelimit/sizeof(void*);
		if (sizelim > 16777216/sizeof(void*)) sizelim = 16777216/sizeof(void*);
		its = SPEED/64;
				
		for (long long i = opts.cycle_len ; i < sizelim; i+= opts.cycle_len)	//hard-coded for 64-byte cache lines
		{
			double cpi = run_randlat(size, i, its);
			its = (int)(SPEED/cpi);
			if (its < MIN_ITS) its = MIN_ITS;
		}
	}
	else
	{
		printf ("Broken, do not use\n");
		for (long long size = 512, step=8; size  <= opts.sizelimit/sizeof(void*); size = size + step )
		{
			double cpi = runtest(opts, size, its);
			if (!((size-1) & size)) step <<=1;
			its = (int)(SPEED/cpi);
			if (its < MIN_ITS) its = MIN_ITS;
		}
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
