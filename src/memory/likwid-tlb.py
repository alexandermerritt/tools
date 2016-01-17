#! /usr/bin/env python
import os
import sys
import subprocess as sp

if len(sys.argv) < 1:
    print("Error: specify command to monitor")
    sys.exit(1)

# westmere
#counters = 'DTLB_MISSES_WALK_CYCLES:PMC0' + \
#            ',ITLB_MISSES_WALK_CYCLES:PMC1' + \
#            ',UNCORE_CLOCKTICKS:UPMCFIX'

# ivb-e
counters = 'DTLB_LOAD_MISSES_WALK_DURATION:PMC0,' + \
           'DTLB_STORE_MISSES_WALK_DURATION:PMC1,' + \
           'ITLB_MISSES_WALK_DURATION:PMC2'

cmd = [ 'likwid-perfctr', '-C', '0', '-O']
cmd.extend(['-g', counters])
cmd.extend(sys.argv[1:])

text = sp.check_output(cmd, universal_newlines=True)

lines = text.split('\n')
n = len(lines)
i = 0
while i < n:
    if 'Gups:' in lines[i]:
        gups = lines[i].split()[-1]
    if 'Event,' not in lines[i]:
        i += 1
        continue
    break

if i >= n or 'Event,' not in lines[i]:
    print('bad output')
    print(text)
    sys.exit(1)

colnames = lines[i].split(',')
i += 1
cycles = 0.0
ticks = 0.0
while i < n:
    if 'WALK_DURATION' in lines[i]:
        fields = lines[i].split(',')
        for f in fields[2:]:
            if f != '':
                cycles += float(f)
    elif 'CPU_CLK_UNHALTED_REF' in lines[i]:
        fields = lines[i].split(',')
        for f in fields[2:]:
            if f != '':
                ticks += float(f)
    i += 1

print('cycles ticks ratio gups')
print(str(cycles) + ' ' + str(ticks) \
        + ' ' + str(cycles/ticks)) \
        + ' ' + str(gups)

