#!/usr/bin/env python3
# Simulate a maximal block attack on the Monero network
# This uses the scheme proposed by ArticMine
# Written by Sarang Nother
# Copyright (c) 2019-2025, The Monero Project
import sys
import math

MEDIAN_WINDOW_SMALL = 100 # number of recent blocks for median computation
MEDIAN_WINDOW_BIG = 5000
MULTIPLIER_BIG = 50.0
MEDIAN_THRESHOLD = 300*1000 # initial value for median (scaled kB -> B)
lcg_seed = 0
embw = MEDIAN_THRESHOLD
ltembw = MEDIAN_THRESHOLD

weights = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_SMALL # weights of recent blocks (B), with index -1 most recent
lt_weights = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_BIG # long-term weights

# see contrib/epee/include/misc_language.h, get_mid
def get_mid(a, b):
    return (a//2) + (b//2) + ((a - 2*(a//2)) + (b - 2*(b//2)))//2;

# Compute the median of a list
def get_median(vec):
    if len(vec) == 1:
        return vec[0]
    temp = sorted(vec)
    n = len(temp) // 2
    if len(temp) % 2 == 1:
        return temp[n]
    else:
        return get_mid(temp[n-1], temp[n])

def LCG():
  global lcg_seed
  lcg_seed = (lcg_seed * 0x100000001b3 + 0xcbf29ce484222325) & 0xffffffff
  return lcg_seed

def run(t, blocks):
  global embw
  global ltembw

  weights = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_SMALL # weights of recent blocks (B), with index -1 most recent
  lt_weights = [MEDIAN_THRESHOLD]*MEDIAN_WINDOW_BIG # long-term weights

  for block in range(blocks):
      # determine the long-term effective weight
      ltmedian = get_median(lt_weights[-MEDIAN_WINDOW_BIG:])
      ltembw = max(MEDIAN_THRESHOLD,ltmedian)

      # determine the effective weight
      stmedian = get_median(weights[-MEDIAN_WINDOW_SMALL:])
      embw = min(max(ltembw,stmedian),int(MULTIPLIER_BIG*ltembw))

      # drop the lowest values
      weights = weights[1:]
      lt_weights = lt_weights[1:]

      # add a block of max weight
      if t == 0:
        max_weight = 2 * embw
      elif t == 1:
        r = LCG()
        max_weight = int(90 + r % 500000 + 250000 + math.sin(block / 200.) * 350000)
        if max_weight < 90: max_weight = 90
      elif t == 2:
        max_weight = 90
      else:
        sys.exit(1)
      weights.append(max_weight)
      lt_weights.append(min(max(max_weight, ltembw * 10 // 17),int(ltembw + int(ltembw * 7 / 10))))

      #print "H %u, r %u, BW %u, EMBW %u, LTBW %u, LTEMBW %u, ltmedian %u" % (block, r, max_weight, embw, lt_weights[-1], ltembw, ltmedian)
      print("H %u, BW %u, EMBW %u, LTBW %u" % (block, max_weight, embw, lt_weights[-1]))

run(0, 2 * MEDIAN_WINDOW_BIG)
run(1, 9 * MEDIAN_WINDOW_BIG)
run(2, 1 * MEDIAN_WINDOW_BIG)
