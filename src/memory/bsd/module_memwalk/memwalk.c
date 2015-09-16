#if !defined(__unix__)
#error Compiles only on *BSD systems.
#endif

// order apparently matters among these header files
#include <sys/cdefs.h>
#include <sys/types.h>

#include <sys/module.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/kernel.h>

#include <sys/lock.h>
#if defined(__DragonFly__)
#include <sys/spinlock2.h>
#include <vm/bitops.h>
#elif defined(__FreeBSD__)
#include <sys/sx.h>
#endif

#include <vm/vm.h>
#include <vm/pmap.h>
#include <machine/pmap.h>
#include <vm/vm_map.h>

#include <sys/mutex.h>
#include <sys/proc.h>

void testbitset(void);
void print_pmap(struct proc *);
void dump_pq_free(void);

#if defined(__DragonFly__)
enum action
{
	PRT_VMSPACE = 1,
	PRT_PMAP,
	DUMP_PQ_FREE,
	TEST_BITSET,
};

#define PROC_WATCH_NAME		"map"
#define PROC_WATCH_NAME_LEN	3

static inline int is_proc(struct proc *proc)
{
	int rv;
	rv = strncmp(proc->p_comm,
			PROC_WATCH_NAME,
			PROC_WATCH_NAME_LEN);
	return !rv;
}

static inline char map_type_char(vm_maptype_t t)
{
	switch (t) {
		case VM_MAPTYPE_UNSPECIFIED: return 'U';
		case VM_MAPTYPE_NORMAL: return 'N';
		case VM_MAPTYPE_VPAGETABLE: return 'V';
		case VM_MAPTYPE_SUBMAP: return 'S';
		case VM_MAPTYPE_UKSMAP: return 'K';
		default: return 'X';
	}
}

static void print_vmspace(struct vmspace *vms)
{
	struct vm_map *map = &vms->vm_map;
	vm_map_lock_read(map);
	struct vm_map_entry *entry = map->header.next;
	int n = map->nentries;
	uprintf("    start end maptype mapobj\n");
	while (n-- > 0 && entry && entry != &map->header) {
		uprintf("    0x%016lx 0x%016lx %c %p\n",
				entry->start, entry->end,
				map_type_char(entry->maptype),
				entry->object.map_object);
		entry = entry->next;
	}
	vm_map_unlock_read(map);
}

void _walk_rb_pmap(struct pv_entry *entry);
void walk_rb_pmap(struct pmap *pmap);

void _walk_rb_pmap(struct pv_entry *entry)
{
	struct vm_page *m;
	static long long firstn = 10;
	if (--firstn <= 0)
		return;
	if (entry && entry->pv_entry.rbe_left)
		_walk_rb_pmap(entry->pv_entry.rbe_left);
	m = entry->pv_m;
	uprintf("%p\n", (void*)m->phys_addr);
	if (entry && entry->pv_entry.rbe_right)
		_walk_rb_pmap(entry->pv_entry.rbe_right);
}

// must hold locks
void walk_rb_pmap(struct pmap *pmap)
{
	_walk_rb_pmap(pmap->pm_pvroot.rbh_root);
}

void print_pmap(struct proc *proc)
{
	struct pmap *pmap = &proc->p_vmspace->vm_pmap;

	// in pmap_pt the pm_spin lock is held when manipulating the
	// rbtree (there are three locks in pmap! which to use?)
	// pm_token is held when incrementing pm_count, and pmap_scan
	// pmap_scan first locks pm_token, then briefly spin_lock's
	// the pm_spin to touch the rbtree

	lwkt_gettoken(&pmap->pm_token);
	spin_lock(&pmap->pm_spin);
	walk_rb_pmap(pmap);
	spin_unlock(&pmap->pm_spin);
	lwkt_reltoken(&pmap->pm_token);
}

static int proc_prtpmap(struct proc *proc, void *data)
{
	static int found = 0;
	if (found)
		return 0;
	if (proc->p_stat == SZOMB)
		return 0;
	PHOLD(proc);
	if (is_proc(proc)) {
		found = 1;
		print_pmap(proc);
	}
	PRELE(proc);
	return 0;
}

static int proc_prtvmspace(struct proc *proc, void *data)
{
	static int found = 0;
	if (found)
		return 0;
	if (proc->p_stat == SZOMB)
		return 0;
	PHOLD(proc);
	if (!is_proc(proc))
		goto _unlock;
	uprintf("%s %d\n", proc->p_comm, proc->p_pid);
	print_vmspace(proc->p_vmspace);
	found = 1;
_unlock:;
	PRELE(proc);
	return 0;
}

void dump_pq_free(void)
{
	int qid = PQ_FREE;
	int q, *cnt = vm_page_queues[qid].cnt;
	uprintf("q cnt lcnt\n");
	for (q = 0; q < PQ_L2_SIZE; q++)
		uprintf("%d %d %d\n", q, *cnt,
				vm_page_queues[qid+q].lcnt);
}

static void dodf(enum action a)
{
	switch (a) {
		case PRT_VMSPACE:
			allproc_scan(proc_prtvmspace, NULL);
			break;
		case PRT_PMAP:
			allproc_scan(proc_prtpmap, NULL);
			break;
		case DUMP_PQ_FREE:
			dump_pq_free();
			break;
		case TEST_BITSET:
			testbitset();
			break;
		default: break;
	}
}

typedef unsigned char uchar;

void testbitset(void)
{
	char msg[256];
	const int n = 4;
	int i;
	uchar set[n];
	for (i = 0; i < 32; i++) {
		memset(set, 0, sizeof(set));
		set_bit(set, n, i);
		ksnprintf(msg, 256, "0x%02x%02x%02x%02x",
				set[0], set[1], set[2], set[3]);
		uprintf("%s set: %d\n", msg, bit_isset(set, n, i));

		clear_bit(set, n, i);
		ksnprintf(msg, 256, "0x%02x%02x%02x%02x",
				set[0], set[1], set[2], set[3]);
		uprintf("%s set: %d\n", msg, bit_isset(set, n, i));
	}

	uprintf("setting 0 4 7 8 23\n");
	set_bit(set, n, 0);
	set_bit(set, n, 4);
	set_bit(set, n, 7);
	set_bit(set, n, 8);
	set_bit(set, n, 23);
	ksnprintf(msg, 256, "0x%02x%02x%02x%02x",
			set[0], set[1], set[2], set[3]);
	uprintf("%s\n", msg);
	for (i = 0; i < 7; i++) { /* 0 and 7 should return -1 */
		uprintf("%d th bit set, idx = %d\n", i,
				nth_bitset(set, n, i));
	}
}
#endif	/* __DragonFly__ */

#if defined(__FreeBSD__)
static void dofreebsd(void)
{
	struct proc *proc = NULL;
	struct vm_map_entry *entry;
	struct vm_map *vmap;
	struct pmap *pmap;
	struct pv_chunk *pc;
	struct vmspace *vms;
	int n, i, b, pvper;

	sx_slock(&proctree_lock);
	FOREACH_PROC_IN_SYSTEM(proc) {
		if (0 == strncmp(proc->p_comm, "mmap", 4))
			break;
	}

	if (!proc) {
		uprintf("    No mmap found\n");
		sx_sunlock(&proctree_lock);
		return;
	}

	PROC_LOCK(proc);
	sx_sunlock(&proctree_lock);

	vms = proc->p_vmspace;
	vmap = &vms->vm_map;
	pmap = &vms->vm_pmap;

	n = vmap->nentries;
	entry = &vmap->header;
	printf("--- %06d vmap ---\n", proc->p_pid);
	while (n-- > 0 && !!entry) {
		printf("    %lu [0x%lx,0x%lx) \n",
				entry->end-entry->start,
				entry->start, entry->end);
		entry = entry->next;
	}

	printf("--- %06d pmap ---\n", proc->p_pid);
	pvper = sizeof(pc->pc_map) << 3; // bits per entry in bitmap
	TAILQ_FOREACH(pc, &pmap->pm_pvchunk, pc_list) {
		for (i = 0; i < _NPCM; i++) {
			for (b = 0; b < pvper; b++) {
				if (0 == (pc->pc_map[i] & (1<<b)))
					printf("    0x%lx\n",
							pc->pc_pventry[i * pvper + b].pv_va);
			}
		}
	}

	PROC_UNLOCK(proc);
}
#endif	/* __FreeBSD__ */

static void doaction(void)
{
#if defined(__FreeBSD__)
	dofreebsd();
#elif defined(__DragonFly__)
	dodf(TEST_BITSET);
#endif
}

static int event_handler(struct module *module, int event, void *arg)
{
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			doaction();
			break;
		case MOD_UNLOAD:
			break;
		default:
			e = EOPNOTSUPP;
			break;
	}
	return e;
}

struct moduledata mod_conf = {
	.name = "memwalk",
	.evhand = event_handler,
	.priv = NULL
};

DECLARE_MODULE(memwalk, mod_conf, SI_SUB_DRIVERS, SI_ORDER_ANY);

