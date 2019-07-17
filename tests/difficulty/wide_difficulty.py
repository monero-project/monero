#!/usr/bin/env python

from __future__ import print_function
import sys
import subprocess

python = sys.argv[1]
py = sys.argv[2]
c = sys.argv[3]
data = sys.argv[4]

first = python + " " + py + " > " + data
second = [c, '--wide', data]

try:
  print('running: ', first)
  subprocess.check_call(first, shell=True)
  print('running: ', second)
  subprocess.check_call(second)
except:
  sys.exit(1)

