#! /usr/bin/env python3
import os
import sys
import subprocess as sp

if len(sys.argv) < 1:
    print("Error: specify command to monitor")
    sys.exit(1)

counters = 'DTLB_MISSES_WALK_CYCLES:PMC0' + \
            ',ITLB_MISSES_WALK_CYCLES:PMC1' + \
            ',UNCORE_CLOCKTICKS:UPMCFIX'

cmd = [ 'likwid-perfctr', '-C', '2', '-O']
cmd.extend(['-g', counters])
cmd.extend(sys.argv[1:])

text = sp.check_output(cmd, universal_newlines=True)

lines = text.split('\n')
n = len(lines)
i = 0
while i < n:
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
    if 'WALK_CYCLES' in lines[i]:
        fields = lines[i].split(',')
        for f in fields[2:]:
            if f != '':
                cycles += float(f)
    elif 'CLOCKTICKS' in lines[i]:
        fields = lines[i].split(',')
        for f in fields[2:]:
            if f != '':
                ticks += float(f)
    i += 1

print('cycles ticks ratio')
print(str(cycles) + ' ' + str(ticks) \
        + ' ' + str(cycles/ticks))

