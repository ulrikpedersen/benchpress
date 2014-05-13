#!/bin/env dls-python

import sys, os

if len(sys.argv) < 1:
    print "### ERROR: not enough arguments. Please supply an input file ###"

fptr = open(sys.argv[1])

datafile=fptr.readline().strip()
dataset=fptr.readline().strip()

config=""
result=""
scans = []
for line in fptr.readlines():
    if line.startswith('CONFIG:'): config=line
    elif line.startswith('RESULT:'):
        result = line
        
        # Mash all the config and result key-value pairs into a dictionary
        d = dict( [kv.strip().split('=') for kv in result.split('\t')[1:]] )
        d.update( dict([kv.strip().split('=') for kv in config.split('\t')[1:]] ))
        scans.append( d )
        
fptr.close()

# Analyse and plot results
print scans

