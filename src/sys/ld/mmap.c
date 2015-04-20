#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>

enum {
	MADV_PGCOLOR = 12,
};
static int ncolors = 0; // disabled

__attribute__((constructor))
void getcolors(void)
{
	const char *str = getenv("PGCOLOR");
	if (str)
		ncolors = strtol(str, NULL, 10);
	printf("--> using %d colors\n", ncolors);
}

void * mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	void* (*func)(void *, size_t , int , int , int , off_t);
	void *rv;
	int m;

	func = dlsym(RTLD_NEXT, "mmap");
	if (func == mmap)
		exit(EXIT_FAILURE);

	rv = func(addr, len, prot, flags, fd, offset);
	if (rv == MAP_FAILED)
		return rv;
	if (prot & PROT_NONE)
		return rv;
	if ((prot & MAP_ANON) == 0)
		return rv;
	if ((prot & MAP_PRIVATE) == 0)
		return rv;

	m = mcontrol(rv, len, MADV_PGCOLOR, ncolors);
	if (m)
		exit(errno);

	return rv;
}


