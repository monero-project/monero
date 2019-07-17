#!/usr/bin/python
# Simulate a maximal block attack on the Monero network
# This uses the scheme proposed by ArticMine
# Written by Sarang Nother
# Copyright (c) 2019 The Monero Project
import sys
import math

MEDIAN_WINDOW_SMALL = 100 # number of recent blocks for median computation
MEDIAN_WINDOW_BIG = 5000
MULTIPLIER_BIG = 20.0
MEDIAN_THRESHOLD = 20*1000 # initial value for median (scaled kB -> B)
lcg_seed = 0
embw = MEDIAN_THRESHOLD
ltembw = MEDIAN_THRESHOLD

sizes = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_SMALL # sizes of recent blocks (B), with index -1 most recent
lt_sizes = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_BIG # long-term sizes

# Compute the median of a list
def get_median(vec):
    #temp = vec
    temp = sorted(vec)
    if len(temp) % 2 == 1:
        return temp[len(temp)//2]
    else:
        return int((temp[len(temp)//2]+temp[len(temp)//2-1])//2)

def LCG():
  global lcg_seed
  lcg_seed = (lcg_seed * 0x100000001b3 + 0xcbf29ce484222325) & 0xffffffff
  return lcg_seed

def run(t, blocks):
  global embw
  global ltembw

  sizes = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_SMALL # sizes of recent blocks (B), with index -1 most recent
  lt_sizes = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_BIG # long-term sizes

  for block in range(blocks):
      # determine the long-term effective size
      ltmedian = get_median(lt_sizes[-MEDIAN_WINDOW_BIG:])
      ltembw = max(MEDIAN_THRESHOLD,ltmedian)

      # determine the effective size
      stmedian = get_median(sizes[-MEDIAN_WINDOW_SMALL:])
      embw = min(max(MEDIAN_THRESHOLD,stmedian),int(MULTIPLIER_BIG*ltembw))

      # drop the lowest values
      sizes = sizes[1:]
      lt_sizes = lt_sizes[1:]

      # add a block of max size
      if t == 0:
        max_size = 2 * embw
      elif t == 1:
        r = LCG()
        max_size = int(240 + r % 33333 + 16666 + math.sin(block / 200.) * 23333)
        if max_size < 240: max_size = 240
      elif t == 2:
        max_size = 240
      else:
        sys.exit(1)
      sizes.append(max_size)
      lt_sizes.append(min(max_size,int(ltembw + int(ltembw * 2 / 50))))

      #print "H %u, r %u, BW %u, EMBW %u, LTBW %u, LTEMBW %u, ltmedian %u" % (block, r, max_size, embw, lt_sizes[-1], ltembw, ltmedian)
      print "H %u, BW %u, EMBW %u, LTBW %u" % (block, max_size, embw, lt_sizes[-1])

run(0, 2 * MEDIAN_WINDOW_BIG)
run(1, 9 * MEDIAN_WINDOW_BIG)
run(2, 1 * MEDIAN_WINDOW_BIG)
