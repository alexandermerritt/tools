// order apparently matters among these header files
#include <sys/cdefs.h>
#include <sys/types.h>

#include <sys/module.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/kernel.h>

#include <sys/lock.h>
#include <sys/sx.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <machine/pmap.h>
#include <vm/vm_map.h>

#include <sys/mutex.h>
#include <sys/proc.h>

static void walk(void)
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

static int event_handler(struct module *module, int event, void *arg)
{
    int e = 0;
    switch (event) {
        case MOD_LOAD: {
                           walk();
                       } break;
        case MOD_UNLOAD: {
                         } break;
        default: {
                     e = EOPNOTSUPP;
                 } break;
    }
    return e;
}

struct moduledata mod_conf = {
    .name = "color",
    .evhand = event_handler,
    .priv = NULL
};

DECLARE_MODULE(color, mod_conf, SI_SUB_DRIVERS, SI_ORDER_ANY);

