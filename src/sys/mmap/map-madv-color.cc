#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <list>

#define NPAGES		32
#define LEN		(NPAGES << 12)
#define MADV_PGCOLOR	12

#define A2P(a)		((a)<<12)

using namespace std;

int map(pid_t pid, list<string> &lines)
{
	char str[256];
	lines.clear();
	snprintf(str, 256, "/proc/%d/map", pid);
	FILE *fp = fopen(str, "r");
	if (!fp)
		return 1;
	while (fgets(str, 256, fp))
		lines.push_back(string(str));
	fclose(fp);
	return 0;
}

int dump_pmap(pid_t pid)
{
	char str[256];
	snprintf(str, 256, "/proc/%d/pmap", pid);
	FILE *fp = fopen(str, "r");
	if (!fp)
		return 1;
	printf(" --- pmap \n");
	while (fgets(str, 256, fp))
		cout << str;
	fclose(fp);
	return 0;
}

void print(list<string> &maps, void *_addr)
{
	long long other, addr = (long long)_addr;
	list<string>::iterator iter = maps.begin();
	while (iter != maps.end()) {
		other = strtoll(iter->data(), 0, 16);
		if (addr && addr == other) {
			cout << *iter;
			break;
		} else if (!addr) {
			cout << *iter;
		}
		iter++;
	}
}

int test_mcontrol(void *addr, int ncolors)
{
	list<string> maps;
	pid_t pid = getpid();
	int pg;

	// XXX NOTE enable this portion so the app can test itself
	// otherwise, use the sys/ld/libmmap.so preloaded to force control
#if 0
	printf(" --- mcontrol PGCOLOR\n");
	int rv = mcontrol(addr, LEN, MADV_PGCOLOR, ncolors);
	if (rv) {
		perror("mcontrol");
		return 1;
	}
#endif

	map(pid, maps); print(maps, NULL);
	printf(" --- touching\n");
	*((int*)addr) = 0xdeadbeef;
	map(pid, maps); print(maps, NULL);
	dump_pmap(pid);

	printf(" --- touching all\n");
	for (pg = 0; pg < NPAGES; pg++) {
		*((int*)((uintptr_t)addr + A2P(pg))) = 0xdeadbeef;
	}
	map(pid, maps); print(maps, NULL);
	dump_pmap(pid);

	printf(" --- (mmap returned %p)\n", addr);
	printf(" --- (pid %d)\n", getpid());
	return 0;
}

int main(int argc, char *argv[])
{
	void *addr;
	list<string> maps;
	pid_t pid = getpid();

	if (argc < 2)
		return 1;
	int ncolors = strtol(argv[1], NULL, 10);

	printf(" --- full map at main()\n");
	map(pid, maps); print(maps, NULL);

	uint32_t flags = MAP_PRIVATE|MAP_ANON|MAP_NOCORE;
	addr = mmap(NULL, LEN, PROT_READ|PROT_WRITE, flags, -1, 0);
	if (MAP_FAILED == addr)
		return 1;

	printf(" --- mmap returned %p\n", addr);
	map(pid, maps); print(maps, NULL);

	return test_mcontrol(addr, ncolors);

	printf(" --- full map after first mmap\n");
	map(pid, maps); print(maps, NULL);

	printf(" --- mmap at %p\n", addr);
	map(pid, maps); print(maps, addr);

	printf(" --- more mmaps\n");
	// observe whether each region expands existing region
	mmap(NULL, LEN, PROT_READ|PROT_WRITE, flags, -1, 0);
	map(pid, maps); print(maps, addr);

	mmap(NULL, LEN, PROT_READ|PROT_WRITE, flags, -1, 0);
	map(pid, maps); print(maps, addr);

	mmap(NULL, LEN, PROT_READ|PROT_WRITE, flags, -1, 0);
	map(pid, maps); print(maps, addr);

	// trigger page faults
	// i.  creates vm_object assigned to area
	// ii. resident page count increases
	printf(" --- touching\n");
	int i;
	for (i = 0; i < 8; i++) {
		*((char*)addr + (i<<12)) = 0;
		map(pid, maps); print(maps, addr);
	}

	sleep(10);

	// clear mappings
	printf(" --- clear mappings\n");
	madvise(addr, 1*LEN, MADV_INVAL);
	map(pid, maps); print(maps, addr);
	// fault in again
	printf(" --- touching\n");
	for (i = 0; i < 8; i++) {
		*((char*)addr + (i<<12)) = 0;
		map(pid, maps); print(maps, addr);
	}

	// test random
	printf(" --- clear mappings\n");
	madvise(addr, 1*LEN, MADV_INVAL);
	map(pid, maps); print(maps, addr);
	printf(" --- set rand\n");
	madvise(addr, 1*LEN, MADV_RANDOM);
	map(pid, maps); print(maps, NULL);
	printf(" --- touching\n");
	*((char*)addr) = 0;
	map(pid, maps); print(maps, NULL);
	return 0;
}
