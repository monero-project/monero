#!/usr/bin/python

import sys
import subprocess

print 'running: ', sys.argv[1]
S0 = subprocess.check_output(sys.argv[1], stderr=subprocess.STDOUT)
print 'running: ', sys.argv[2]
S1 = subprocess.check_output(sys.argv[2], stderr=subprocess.STDOUT)
print 'comparing'
if S0 != S1:
  sys.exit(1)
sys.exit(0)
