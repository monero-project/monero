#!/usr/bin/env python

from __future__ import print_function
import sys
import subprocess

if len(sys.argv) == 4:
  first = [sys.argv[1], sys.argv[2]]
  second = [sys.argv[3]]
else:
  first = [sys.argv[1]]
  second = [sys.argv[2]]

print('running: ', first)
S0 = subprocess.check_output(first, stderr=subprocess.STDOUT)
print('running: ', second)
S1 = subprocess.check_output(second, stderr=subprocess.STDOUT)
print('comparing')
if S0 != S1:
  sys.exit(1)
sys.exit(0)
