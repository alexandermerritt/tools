#! /usr/bin/env python
import os
import subprocess as sp

cmd = ['likwid-perfctr',
        '-O',
        '-C', '0',
        '-m',
        '-M', '1',
        '-g',
        'DTLB_LOAD_MISSES_WALK_CYCLES:PMC0,DTLB_MISSES_WALK_CYCLES:PMC1,ITLB_MISSES_WALK_CYCLES:PMC2',
        './tlbtest'
        ]

ret = sp.check_output(cmd, universal_newlines=True)
lines = ret.split('\n')
for line in lines:
    print(line)

