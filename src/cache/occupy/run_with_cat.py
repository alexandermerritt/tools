#! /usr/bin/env python

# Run occupy with varying working set sizes and varying LLC sizes to measure
# impact of cache size restriction on cache line access latencies.

import subprocess as sp

llc = 15 # MiB
maxcos = 0xfffff
mincos = 0x3

def change_cos(cos, cos_n):
    cmd = ['sudo', '../cat/pqos/pqos', '-e']
    cmd.append('llc:' + str(cos_n) + '=' + hex(cos))
    sp.check_call(cmd)

wss = 1 # MiB
while wss <= (llc+1):
    cos = maxcos
    while cos >= mincos:
        change_cos(cos, 0)
        cmd = ['./occupy', str(wss << 20),
                str(wss << 20), str(1) ]
        out = sp.check_output(cmd).strip()
        out += ' ' + hex(cos)
        out += ' ' + str(maxcos * (bin(cos).count('1')/20.))
        print(out)
        cos = cos >> 1
    wss += 1

