#!/bin/env dls-python
from pkg_resources import require
require('numpy')
require('matplotlib')

import sys, os
import matplotlib as mpl
from pylab import *

if len(sys.argv) < 1:
    print "### ERROR: not enough arguments. Please supply an input file ###"

fptr = open(sys.argv[1])

datafile=fptr.readline().strip()
dataset=fptr.readline().strip()

config=""
result=""
scans = []
datasetsize = None
algorithm = None
for line in fptr.readlines():
    if not datasetsize and line.startswith(' Dataset='):
        d = dict( [kv.strip().split('=') for kv in line.split('\t')])
        datasetsize = double(d['Dataset'].rstrip('MB'))
    if line.startswith('CONFIG:'): config=line
    elif line.startswith('RESULT:'):
        result = line
        
        # Mash all the config and result key-value pairs into a dictionary
        d = dict( [kv.strip().split('=') for kv in result.split('\t')[1:]] )
        d.update( dict([kv.strip().split('=') for kv in config.split('\t')[1:]] ))
        #scans.append( d )
        if not algorithm:
            algorithm = d['algo']
        
        scans.append( (d['threads'], int(d['level']), double(d['Ratio']), double(d['Datarate'].rstrip('MB/s'))) )
        
fptr.close()

# Analyse and plot results
#for s in scans: print s

#scans = array(scans)
#print scans.shape
#scans = scans.reshape((2, 10, 3))
#print scans

results = dict()
for thread,level,ratio, datarate in scans:
    if not results.has_key(thread):
        results.update({thread: [(level,ratio, datarate)]})
    else:
        results[thread].append((level,ratio,datarate))

legends=[]
markers= [ 's', 'o', 'v', '^', '+', 'x', '>', '<', '.', ',' ]
for i,threadcount in enumerate(results):
    #print zip(*results[threadcount])
    legends.append('%s threads'%threadcount)
    xy = zip(*results[threadcount])
    x=xy[1]
    y=xy[2]
    
    plot(x,y, marker=markers[i])

plot([1,6],[1280, 1280*6], linestyle='-.', color='black')
ylim(0,12000)
xlim(0,None)
text(1.1, 1280+50, "10 GigE")    
axhline(1280, linewidth=3, linestyle='-.', color='black')

text(0.5, 10000, "dataset: %.2fGB\nalgorithm: %s"%(datasetsize/1024., algorithm ))

title(datafile)
legend(legends, loc='best')
xlabel('Compresssion ratio')
ylabel('Speed (MB/s)')
grid(True)
    
show()

