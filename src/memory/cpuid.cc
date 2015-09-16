#include <iostream>
#include <string.h>

static const char *CPUID2[] = {
    [0x00] = "General Null descriptor, this byte contains no information",
    [0x01] = "TLB: Instruction TLB: 4 KByte pages, 4-way set associative, 32 entries",
    [0x02] = "TLB: Instruction TLB: 4 MByte pages, fully associative, 2 entries",
    [0x03] = "TLB: Data TLB: 4 KByte pages, 4-way set associative, 64 entries",
    [0x04] = "TLB: Data TLB: 4 MByte pages, 4-way set associative, 8 entries",
    [0x05] = "TLB: Data TLB1: 4 MByte pages, 4-way set associative, 32 entries",
    [0x06] = "Cache: 1st-level instruction cache: 8 KBytes, 4-way set associative, 32 byte line size",
    [0x08] = "Cache: 1st-level instruction cache: 16 KBytes, 4-way set associative, 32 byte line size",
    [0x09] = "Cache: 1st-level instruction cache: 32KBytes, 4-way set associative, 64 byte line size",
    [0x0A] = "Cache: 1st-level data cache: 8 KBytes, 2-way set associative, 32 byte line size",
    [0x0B] = "TLB: Instruction TLB: 4 MByte pages, 4-way set associative, 4 entries",
    [0x0C] = "Cache: 1st-level data cache: 16 KBytes, 4-way set associative, 32 byte line size",
    [0x0D] = "Cache: 1st-level data cache: 16 KBytes, 4-way set associative, 64 byte line size",
    [0x0E] = "Cache: 1st-level data cache: 24 KBytes, 6-way set associative, 64 byte line size",
    [0x1D] = "Cache: 2nd-level cache: 128 KBytes, 2-way set associative, 64 byte line size",
    [0x21] = "Cache: 2nd-level cache: 256 KBytes, 8-way set associative, 64 byte line size",
    [0x22] = "Cache: 3rd-level cache: 512 KBytes, 4-way set associative, 64 byte line size, 2 lines per sector",
    [0x23] = "Cache: 3rd-level cache: 1 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x24] = "Cache: 2nd-level cache: 1 MBytes, 16-way set associative, 64 byte line size",
    [0x25] = "Cache: 3rd-level cache: 2 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x29] = "Cache: 3rd-level cache: 4 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x2C] = "Cache: 1st-level data cache: 32 KBytes, 8-way set associative, 64 byte line size",
    [0x30] = "Cache: 1st-level instruction cache: 32 KBytes, 8-way set associative, 64 byte line size",
    [0x40] = "Cache: No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache",
    [0x41] = "Cache: 2nd-level cache: 128 KBytes, 4-way set associative, 32 byte line size",
    [0x42] = "Cache: 2nd-level cache: 256 KBytes, 4-way set associative, 32 byte line size",
    [0x43] = "Cache: 2nd-level cache: 512 KBytes, 4-way set associative, 32 byte line size",
    [0x44] = "Cache: 2nd-level cache: 1 MByte, 4-way set associative, 32 byte line size",
    [0x45] = "Cache: 2nd-level cache: 2 MByte, 4-way set associative, 32 byte line size",
    [0x46] = "Cache: 3rd-level cache: 4 MByte, 4-way set associative, 64 byte line size",
    [0x47] = "Cache: 3rd-level cache: 8 MByte, 8-way set associative, 64 byte line size",
    [0x48] = "Cache: 2nd-level cache: 3MByte, 12-way set associative, 64 byte line size",
    [0x49] = "Cache: 3rd-level cache: 4MB, 16-way set associative, 64-byte line size (Intel Xeon processor MP, Family 0FH, Model 06H); 2nd-level cache: 4 MByte, 16-way set associative, 64 byte line size",
    [0x4A] = "Cache: 3rd-level cache: 6MByte, 12-way set associative, 64 byte line size",
    [0x4B] = "Cache: 3rd-level cache: 8MByte, 16-way set associative, 64 byte line size",
    [0x4C] = "Cache: 3rd-level cache: 12MByte, 12-way set associative, 64 byte line size",
    [0x4D] = "Cache: 3rd-level cache: 16MByte, 16-way set associative, 64 byte line size",
    [0x4E] = "Cache: 2nd-level cache: 6MByte, 24-way set associative, 64 byte line size",
    [0x4F] = "TLB: Instruction TLB: 4 KByte pages, 32 entries",
    [0x50] = "TLB: Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 64 entries",
    [0x51] = "TLB: Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 128 entries",
    [0x52] = "TLB: Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 256 entries",
    [0x55] = "TLB: Instruction TLB: 2-MByte or 4-MByte pages, fully associative, 7 entries",
    [0x56] = "TLB: Data TLB0: 4 MByte pages, 4-way set associative, 16 entries",
    [0x57] = "TLB: Data TLB0: 4 KByte pages, 4-way associative, 16 entries",
    [0x59] = "TLB: Data TLB0: 4 KByte pages, fully associative, 16 entries",
    [0x5A] = "TLB: Data TLB0: 2-MByte or 4 MByte pages, 4-way set associative, 32 entries",
    [0x5B] = "TLB: Data TLB: 4 KByte and 4 MByte pages, 64 entries",
    [0x5C] = "TLB: Data TLB: 4 KByte and 4 MByte pages,128 entries",
    [0x5D] = "TLB: Data TLB: 4 KByte and 4 MByte pages,256 entries",
    [0x60] = "Cache: 1st-level data cache: 16 KByte, 8-way set associative, 64 byte line size",
    [0x61] = "TLB: Instruction TLB: 4 KByte pages, fully associative, 48 entries",
    [0x63] = "TLB: Data TLB: 1 GByte pages, 4-way set associative, 4 entries",
    [0x66] = "Cache: 1st-level data cache: 8 KByte, 4-way set associative, 64 byte line size",
    [0x67] = "Cache: 1st-level data cache: 16 KByte, 4-way set associative, 64 byte line size",
    [0x68] = "Cache: 1st-level data cache: 32 KByte, 4-way set associative, 64 byte line size",
    [0x70] = "Cache: Trace cache: 12 K-μop, 8-way set associative",
    [0x71] = "Cache: Trace cache: 16 K-μop, 8-way set associative",
    [0x72] = "Cache: Trace cache: 32 K-μop, 8-way set associative",
    [0x76] = "TLB: Instruction TLB: 2M/4M pages, fully associative, 8 entries",
    [0x78] = "Cache: 2nd-level cache: 1 MByte, 4-way set associative, 64byte line size",
    [0x79] = "Cache: 2nd-level cache: 128 KByte, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x7A] = "Cache: 2nd-level cache: 256 KByte, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x7B] = "Cache: 2nd-level cache: 512 KByte, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x7C] = "Cache: 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size, 2 lines per sector",
    [0x7D] = "Cache: 2nd-level cache: 2 MByte, 8-way set associative, 64byte line size",
    [0x7F] = "Cache: 2nd-level cache: 512 KByte, 2-way set associative, 64-byte line size",
    [0x80] = "Cache: 2nd-level cache: 512 KByte, 8-way set associative, 64-byte line size",
    [0x82] = "Cache: 2nd-level cache: 256 KByte, 8-way set associative, 32 byte line size",
    [0x83] = "Cache: 2nd-level cache: 512 KByte, 8-way set associative, 32 byte line size",
    [0x84] = "Cache: 2nd-level cache: 1 MByte, 8-way set associative, 32 byte line size",
    [0x85] = "Cache: 2nd-level cache: 2 MByte, 8-way set associative, 32 byte line size",
    [0x86] = "Cache: 2nd-level cache: 512 KByte, 4-way set associative, 64 byte line size",
    [0x87] = "Cache: 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size",
    [0xA0] = "DTLB: DTLB: 4k pages, fully associative, 32 entries",
    [0xB0] = "TLB: Instruction TLB: 4 KByte pages, 4-way set associative, 128 entries",
    [0xB1] = "TLB: Instruction TLB: 2M pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries",
    [0xB2] = "TLB: Instruction TLB: 4KByte pages, 4-way set associative, 64 entries",
    [0xB3] = "TLB: Data TLB: 4 KByte pages, 4-way set associative, 128 entries",
    [0xB4] = "TLB: Data TLB1: 4 KByte pages, 4-way associative, 256 entries",
    [0xB5] = "TLB: Instruction TLB: 4KByte pages, 8-way set associative, 64 entries",
    [0xB6] = "TLB: Instruction TLB: 4KByte pages, 8-way set associative, 128 entries",
    [0xBA] = "TLB: Data TLB1: 4 KByte pages, 4-way associative, 64 entries",
    [0xC0] = "TLB: Data TLB: 4 KByte and 4 MByte pages, 4-way associative, 8 entries",
    [0xC1] = "STLB: Shared 2nd-Level TLB: 4 KByte/2MByte pages, 8-way associative, 1024 entries",
    [0xC2] = "DTLB: DTLB: 4 KByte/2 MByte pages, 4-way associative, 16 entries",
    [0xC3] = "STLB: Shared 2nd-Level TLB: 4 KByte /2 MByte pages, 6-way associative, 1536 entries. Also 1GBbyte pages, 4-way, 16 entries.",
    [0xCA] = "STLB: Shared 2nd-Level TLB: 4 KByte pages, 4-way associative, 512 entries",
    [0xD0] = "Cache: 3rd-level cache: 512 KByte, 4-way set associative, 64 byte line size",
    [0xD1] = "Cache: 3rd-level cache: 1 MByte, 4-way set associative, 64 byte line size",
    [0xD2] = "Cache: 3rd-level cache: 2 MByte, 4-way set associative, 64 byte line size",
    [0xD6] = "Cache: 3rd-level cache: 1 MByte, 8-way set associative, 64 byte line size",
    [0xD7] = "Cache: 3rd-level cache: 2 MByte, 8-way set associative, 64 byte line size",
    [0xD8] = "Cache: 3rd-level cache: 4 MByte, 8-way set associative, 64 byte line size",
    [0xDC] = "Cache: 3rd-level cache: 1.5 MByte, 12-way set associative, 64 byte line size",
    [0xDD] = "Cache: 3rd-level cache: 3 MByte, 12-way set associative, 64 byte line size",
    [0xDE] = "Cache: 3rd-level cache: 6 MByte, 12-way set associative, 64 byte line size",
    [0xE2] = "Cache: 3rd-level cache: 2 MByte, 16-way set associative, 64 byte line size",
    [0xE3] = "Cache: 3rd-level cache: 4 MByte, 16-way set associative, 64 byte line size",
    [0xE4] = "Cache: 3rd-level cache: 8 MByte, 16-way set associative, 64 byte line size",
    [0xEA] = "Cache: 3rd-level cache: 12MByte, 24-way set associative, 64 byte line size",
    [0xEB] = "Cache: 3rd-level cache: 18MByte, 24-way set associative, 64 byte line size",
    [0xEC] = "Cache: 3rd-level cache: 24MByte, 24-way set associative, 64 byte line size",
    [0xF0] = "Prefetch: 64-Byte prefetching",
    [0xF1] = "Prefetch: 128-Byte prefetching",
    [0xFF] = "General CPUID leaf 2 does not report cache descriptor information, use CPUID leaf 4 to query cache parameters",
};

// ebx bits when cpuid(eax=7)
// some omitted because don't care
static const char *EXTFEAT[] = {
    [0]  = "FSGBASE",
    [1]  = "TSC Adjust MSR",
    [2]  = "(reserved)",
    [3]  = "BMI1",
    [4]  = "HLE",
    [5]  = "AVX2",
    [6]  = "(reserved)",
    [7]  = "Supervisor-mode Exec Prevention",
    [8]  = "BMI2",
    [9]  = "Enhanced REP MOVSB",
    [10] = "INVPCID",
    [11] = "Restricted Transational Memory",
    [12] = "Platform QoS Monitoring",
    [13] = "Deprecates FPU CS/DS",
    [14] = "Memory Protection Extension (MPX)",
    [15] = "Platform QoS Enforcement",
    [16] = "(reserved)",
    [17] = "(reserved)",
    [18] = "RDSEED",
    [19] = "ADX",
    [20] = "Supervisor-mode Access Prevention",
    [21] = "(reserved)",
    [22] = "(reserved)",
    [23] = "(reserved)",
    [24] = "(reserved)",
    [25] = "Processor Trace",
    [26] = "(reserved)",
    [27] = "(reserved)",
    [28] = "(reserved)",
    [29] = "(reserved)",
    [30] = "(reserved)",
    [31] = "(reserved)",
};

struct regs
{
    unsigned int eax, ebx, ecx, edx;
};

static inline void
cpuid(unsigned int func, struct regs &r)
{
    __asm__("cpuid \n\t"
            : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
            : "a"(func), "b"(r.ebx), "c"(r.ecx), "d"(r.edx)
            :
            );
}

void qos_monitoring(void)
{
    struct regs r;
    memset(&r, 0, sizeof(r));

    r.ecx = 0;
    cpuid(0xF, r);

    printf("\t\tMax RMID = %u\n", r.ebx);
    if ((r.edx >> 1) & 0x1)
        printf("\t\tL3 Cache QoS Monitoring\n");
}

void qos_enforce(void)
{
}

void features(void)
{
    struct regs r;
    memset(&r, 0, sizeof(r));

    r.ecx = 0;
    cpuid(7, r);

    printf("\n\nFeature list\n");
    for (int i = 0; i < 32; i++) {
        if ((r.ebx >> i) & 0x1) {
            printf("\t%s\n", EXTFEAT[i]);
            if (i == 12)
                qos_monitoring();
            if (i == 15)
                qos_enforce();
        }
    }
}

void cpuid2(void)
{
    struct regs r;

    memset(&r, 0, sizeof(r));
    cpuid(2, r);

    if ((0xff & r.eax) != 0x01)
        return;
    for (int i = 0; i < 4; i++) {
        unsigned int v(0),idx;
        switch (i) {
            case 0: v = r.eax; break;
            case 1: v = r.ebx; break;
            case 2: v = r.ecx; break;
            case 3: v = r.edx; break;
            default: abort();
        }

        if (0x80000000 & v)
            continue;

        idx = (0xff000000 & v) >> 24;
        if (idx > 0)
            printf("%s\n", CPUID2[idx]);

        idx = (0x00ff0000 & v) >> 16;
        if (idx > 0)
            printf("%s\n", CPUID2[idx]);

        idx = (0x0000ff00 & v) >> 8;
        if (idx > 0)
            printf("%s\n", CPUID2[idx]);

        if (i == 0)
            continue;
        idx = (0x000000ff & v);
        if (idx > 0)
            printf("%s\n", CPUID2[idx]);
    }
}

static const char *CPUID4_EAX[] = {
    [0] = "Null - No more caches",
    [1] = "Data cache",
    [2] = "Instruction cache",
    [3] = "Unified cache",
    // bits 4-31 reserved
};

// Leaf 04H output depends on the initial value in ECX
void cpuid4(void)
{
    struct regs r;
    int val;

    printf("Legend\n"
            "    [1] Maximum number of addressable IDs\n"
            "        for logical processors sharing this cache\n"
            "    [2] Maximum number of addressable IDs\n"
            "        for processor cores in the physical package\n"
            "    [3] 0: WBINVD/INVD from threads sharing this\n"
            "           cache acts upon lower level caches for\n"
            "           threads sharing this cache.\n"
            "        1: WBINVD/INVD is not guaranteed to act upon\n"
            "           lower level caches of non-originating threads\n"
            "           sharing this cache.\n"
            "\n"
            );

    memset(&r, 0, sizeof(r));
    int ecx = 0;
    while (1) {
        r.ecx = ecx++;
        cpuid(4, r);

        // check if a cache type is specified
        if (!(val = (r.eax) & 0x1f))
            break;

        printf("---- cache ----\n");

        // parse EAX
        printf("type:               %s\n", CPUID4_EAX[val]);
        val = (r.eax >> 5) & 0x7;
        printf("level:              %d\n", val);
        val = (r.eax >> 8) & 0x1;
        printf("self-init:          %d\n", val);
        val = (r.eax >> 9) & 0x1;
        printf("full ass:           %d\n", val);
        // bits 13-10 reserved
        val = (r.eax >> 14) & 0xfff;
        printf("max id:             %d [1]\n", val+1);
        val = (r.eax >> 26) & 0x3f;
        printf("max id:             %d [2]\n", val+1);

        // parse EBX
        val = (r.ebx) & 0xfff;
        printf("line size:          %d\n", val+1);
        val = (r.ebx >> 12) & 0x3ff;
        printf("line partitions:    %d\n", val+1);
        val = (r.ebx >> 22) & 0x3ff;
        printf("ways of ass:        %d\n", val+1);

        // parse ECX
        val = (r.ecx) & 0xffffffff;
        printf("number of sets:     %d\n", val+1);

        // parse EDX
        val = (r.edx) & 0x1;
        printf("wb invalidate:      %d [3]\n", val);
        val = (r.edx >> 1) & 0x1;
        printf("cache incl:         %d (%sinclusive)\n",
                val, (val ? "" : "not "));
        val = (r.edx >> 2) & 0x1;
        printf("complex index:      %d (%s)\n",
                val, (val ? "complex indexing" : "direct-mapped"));
        // bits 31-03 reserved
    }

}

int main(int argc __unused, char *argv[] __unused)
{
    cpuid2();
    cpuid4();
    features();
    return 0;
}
