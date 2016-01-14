#! /usr/bin/env python2

# Plot parallel page fault

import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import sys, math, csv

import brewer2mpl

bmap = brewer2mpl.get_map('YlOrRd', 'sequential', 5)
colors = bmap.mpl_colors[::-1]

plt.rc('legend', **{'fontsize': 11})
matplotlib.rcParams['figure.figsize'] = 5.0, 3.2
matplotlib.rcParams['figure.figsize'] = 5.0, 3.2

DELIM = ' '

# As tuple, returns (x,y) where
# x: (int list) log base 2 of no. threads
# y: (float list) seconds
def import_data(filename, llen, lencol, xcol, ycol):
    xs = []
    ys = []
    with open(filename) as f:
        reader = csv.reader(f, delimiter=DELIM)
        reader.next() # skip column headers
        for row in reader:
            if long(row[lencol]) != llen:
                continue
            xs.append(math.log(float(row[xcol]), 2))
            ys.append(float(row[ycol]))
    return (xs,ys)

def simpleaxis(ax):
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()
    h, l = ax1.get_legend_handles_labels()
    #h1 = [ h[2], h[0], h[1] ]
    #l1 = [ l[2], l[0], l[1] ]

    leg = ax1.legend(reversed(h), reversed(l), loc='best', frameon=False, ncol=2)
    # Alternative: Small box
    # leg.get_frame().set_linewidth(0.5)
    #ax1.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(xlabel))

fig = plt.figure()
ax1 = fig.add_subplot(1,1,1)
ax1.set_xlabel('No. Threads (log_2)')
ax1.set_ylabel('Time [sec]')

# plt.xlim( (,) )
# plt.ylim( (,) )
# plt.xticks( tics, map(lambda x: 2**x, tics) )

LINE_TYPES = [ '--', '-' ]
MARKERS = [ '*','o','s','p','D']
ALPHA = 1.0
MARKERSIZE = 9

item = 0
lab = '1 TiB'
(xval,yval) = import_data('../pfault.smaug', 2**40, 1, 0, 4)
ax1.semilogy(xval, yval,
        #LINE_TYPES[item],
        marker=MARKERS[item], markersize=MARKERSIZE,
        color=colors[item], alpha=ALPHA,
        label=lab)

item += 1
lab = '512 GiB'
(xval,yval) = import_data('../pfault.smaug', 2**39, 1, 0, 4)
ax1.semilogy(xval, yval,
        #LINE_TYPES[item],
        marker=MARKERS[item], markersize=MARKERSIZE,
        color=colors[item], alpha=ALPHA,
        label=lab)

item += 1
lab = '256 GiB'
(xval,yval) = import_data('../pfault.smaug', 2**38, 1, 0, 4)
ax1.semilogy(xval, yval,
        #LINE_TYPES[item],
        marker=MARKERS[item], markersize=MARKERSIZE,
        color=colors[item], alpha=ALPHA,
        label=lab)

item += 1
lab = '128 GiB'
(xval,yval) = import_data('../pfault.smaug', 2**37, 1, 0, 4)
ax1.semilogy(xval, yval,
        #LINE_TYPES[item],
        marker=MARKERS[item], markersize=MARKERSIZE,
        color=colors[item], alpha=ALPHA,
        label=lab)

item += 1
lab = '64 GiB'
(xval,yval) = import_data('../pfault.smaug', 2**36, 1, 0, 4)
ax1.semilogy(xval, yval,
        #LINE_TYPES[item],
        marker=MARKERS[item], markersize=MARKERSIZE,
        color=colors[item], alpha=ALPHA,
        label=lab)

simpleaxis(ax1)
plt.grid(True, which='major', axis='y',
        color='gray', linestyle='--', linewidth=0.5)
plt.savefig('ppfault.pdf', format='pdf', dpi=900, bbox_inches='tight')

fig.clf()
