#! /usr/bin/env python
# Report sizes of all maps by procsses in their virtual address space.

import os
import re

re_pid = re.compile('^[0-9]+')
#re_map = re.compile('^([0-9]+|Size|Rss)')
re_map = re.compile('^Size')

def pids():
    dirs = os.listdir('/proc/')
    dirs2 = []
    for d in dirs:
        if re_pid.match(d):
            dirs2.append(d)
    return dirs2

def mapsizes(pid):
    sizes = [] # region sizes
    pname = '/proc/' + pid + '/smaps'
    try:
        f = open(pname, 'r')
        lines = f.readlines()
    except:
        return
    for line in lines:
        line = line.strip()
        if re_map.match(line):
            # size reported in kB
            sz = long(line.split()[1]) * 1024
            sizes.append( sz )
    f.close()
    return sizes

print('pid nregions pgsize waste')
def analyze(pid,sizes,pgsize):
    waste = 0
    for sz in sizes:
        waste += pgsize - (sz % pgsize)
    print(pid + ' ' + str(len(sizes)) + ' '
            + str(pgsize) + ' ' + str(waste))

for pid in pids():
    sizes = mapsizes(pid)
    if not sizes:
        continue
    analyze(pid, sizes, 2**12)
    analyze(pid, sizes, 2**21)


