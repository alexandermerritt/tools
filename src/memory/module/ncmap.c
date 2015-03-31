#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");

#ifdef CONFIG_X86
#else
#error Architecture not supported.
#endif

// Rename your program to this or use symlink to have this module
// trigger for your program.
static const char name[] = "ncmap__";

//
// Utility functions
//

static inline int check(unsigned long val)
{
    // VM_MERGEABLE is highest known meaningful bit in vm_flags
    unsigned long mask = ~((VM_MERGEABLE << 1) - 1);
    if (val & mask)
        return 0;
    // should have READ but not both WR and EX
    if (!(val & VM_READ))
        return 0;
    if ((val & VM_WRITE) && (val & VM_EXEC))
        return 0;
    return 1;
}

// reverse-engineered guesses for the register meanings:
//      si = address    dx = len    cx = vm_flags
unsigned long * find_vmflags(struct pt_regs *regs)
{
    if (check(regs->cx))
        return &regs->cx;
    return NULL;
}

static inline int filter_process(void)
{
    int ret = strncmp(current->comm, name, strlen(name));
    return (ret != 0);
}

void print_regs(struct pt_regs *regs)
{
    printk(KERN_INFO "regs:\n"
            "   ax      0x%lx\n"
            "   bp      0x%lx\n"
            "   bx      0x%lx\n"
            "   cs      0x%lx\n"
            "   cx      0x%lx\n"
            "   di      0x%lx\n"
            "   dx      0x%lx\n"
            "   flags   0x%lx\n"
            "   ip      0x%lx\n"
            "   orig_ax 0x%lx\n"
            "   r10     0x%lx\n"
            "   r11     0x%lx\n"
            "   r12     0x%lx\n"
            "   r13     0x%lx\n"
            "   r14     0x%lx\n"
            "   r15     0x%lx\n"
            "   r8      0x%lx\n"
            "   r9      0x%lx\n"
            "   si      0x%lx\n"
            "   sp      0x%lx\n"
            "   ss      0x%lx\n",
        regs->ax, regs->bp, regs->bx,
        regs->cs, regs->cx, regs->di,
        regs->dx, regs->flags, regs->ip,
        regs->orig_ax, regs->r10, regs->r11,
        regs->r12, regs->r13, regs->r14,
        regs->r15, regs->r8, regs->r9,
        regs->si, regs->sp, regs->ss);
}

// 
// Probe handlers
//

static int entry__mmap_region(
        struct kretprobe_instance *ri,
        struct pt_regs *regs)
{
    unsigned long *vm_flags;

    if (!ri->task->mm)
        goto nope;
    if (filter_process())
        goto nope;

    // printk(KERN_INFO "---------- %s ------------\n", __func__);
    // print_regs(regs);

    vm_flags = find_vmflags(regs);
    if (!vm_flags) {
        printk(KERN_INFO "vmflags not found\n");
        goto nope;
    }

    // Skip these
    if (*vm_flags & VM_EXEC)
        goto nope;
    if (*vm_flags & VM_SHARED)
        goto nope;

    // Indicate region gets dedicated VMA
    // printk(KERN_INFO "marking 0x%lx special\n", regs->si);
    *vm_flags |= VM_SPECIAL;

    return 0;

nope:
    return 1;
}

#if 0
// jprobe - verify VM_SPECIAL was set in vm_flags during the kretprobe
// entry handler above. Only needed to verify regs->cx holds vm_flags.
static struct vm_area_struct *entry__vma_merge(struct mm_struct *mm,
        struct vm_area_struct *prev, unsigned long addr,
        unsigned long end, unsigned long vm_flags,
        struct anon_vma *anon_vma, struct file *file,
        pgoff_t pgoff, struct mempolicy *policy)
{
    int marked = 0;

    if (!current->mm || filter_process())
        goto out;

    // printk(KERN_INFO "---------- %s ------------\n", __func__);

    // printk(KERN_INFO "vm_flags: 0x%lx\n", vm_flags);
    if (vm_flags & (VM_EXEC | VM_SHARED)) {
        // printk(KERN_INFO "OOPS: got area exec or shared\n");
        goto out;
    }

    marked = (VM_SPECIAL == (vm_flags & VM_SPECIAL));
    // printk(KERN_INFO "pid %d tgid %d addr 0x%lx%s marked special\n",
            // current->pid, current->tgid, addr, (marked ? "" : " NOT"));
    if (!marked)
        goto out;

out:
    jprobe_return();
    return 0;
}
#endif

static int exit__mmap_region(
        struct kretprobe_instance *ri,
        struct pt_regs *regs)
{
    unsigned long addr;
    struct vm_area_struct *vma;

    // printk(KERN_INFO "---------- %s ------------\n", __func__);
    // print_regs(regs);

    // Return value to mmap_region
    addr = regs_return_value(regs);
    if (!addr)
        return 0;

    // mmap_region added the vma to the process so we can look it up
    vma = find_vma(current->mm, addr);
    if (!vma)
        return 0;

    // printk(KERN_INFO "vm_flags: 0x%lx\n", vma->vm_flags);

    // Since we skipped these region types earlier, this handler
    // shouldn't trigger on them.
    if (vma->vm_flags & (VM_EXEC | VM_SHARED)) {
        printk(KERN_WARNING "exit triggered on EXEC or SHARED vma\n");
        return 0;
    }

    // Only want anonymous.
    if (vma->vm_file)
        return 0;

    // Validate the bits we set in the entry handler actually held.
    if (VM_SPECIAL != (vma->vm_flags & VM_SPECIAL)) {
        printk(KERN_INFO "pid %d tgid %d addr 0x%lx not marked special\n",
                current->pid, current->tgid, addr);
    }

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    return 0;
}

//
// Probe structures
//

#if 0
static struct jprobe jp_vma_merge = {
    .entry = entry__vma_merge,
    .kp = {
        .symbol_name = "vma_merge",
    },
};
#endif

static struct kretprobe krp_mmap_region = {
    .handler = exit__mmap_region,
    .entry_handler = entry__mmap_region,
    .data_size = 0,
    .maxactive = 16,
    .kp = {
        .symbol_name = "mmap_region",
    },
};

//
// Module routines
//

static int ncmap_init(void)
{
    int ret;

#if 0
    ret = register_jprobe(&jp_vma_merge);
    if (ret < 0) {
        printk(KERN_INFO "register_jprobe failed: %d\n", ret);
        return ret;
    }
#endif

    ret = register_kretprobe(&krp_mmap_region);
    if (ret < 0) {
        printk(KERN_INFO "register_kretprobe failed: %d\n", ret);
        return ret;
    }
    return 0;
}

static void ncmap_exit(void)
{
#if 0
    // only used to verify VM_SPECIAL
    unregister_jprobe(&jp_vma_merge);
#endif

    unregister_kretprobe(&krp_mmap_region);

    printk(KERN_INFO "missed %d instances of %s\n",
            krp_mmap_region.nmissed,
            krp_mmap_region.kp.symbol_name);
}

module_init(ncmap_init);
module_exit(ncmap_exit);
