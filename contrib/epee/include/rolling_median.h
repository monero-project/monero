// Copyright (c) 2019, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Adapted from source by AShelly:
// Copyright (c) 2011 ashelly.myopenid.com, licenced under the MIT licence
// https://stackoverflow.com/questions/5527437/rolling-median-in-c-turlach-implementation
// https://stackoverflow.com/questions/1309263/rolling-median-algorithm-in-c
// https://ideone.com/XPbl6

#pragma once

#include <stdlib.h>
#include <stdint.h>

namespace epee
{
namespace misc_utils
{

template<typename Item>
struct rolling_median_t
{
private:
  Item* data;  //circular queue of values
  int*  pos;   //index into `heap` for each value
  int*  heap;  //max/median/min heap holding indexes into `data`.
  int   N;     //allocated size.
  int   idx;   //position in circular queue
  int   minCt; //count of items in min heap
  int   maxCt; //count of items in max heap
  int   sz;    //count of items in heap

private:

  //returns true if heap[i] < heap[j]
  bool mmless(int i, int j) const
  {
    return data[heap[i]] < data[heap[j]];
  }

  //swaps items i&j in heap, maintains indexes
  bool mmexchange(int i, int j)
  {
    const int t = heap[i];
    heap[i] = heap[j];
    heap[j] = t;
    pos[heap[i]] = i;
    pos[heap[j]] = j;
    return 1;
  }

  //swaps items i&j if i<j;  returns true if swapped
  bool mmCmpExch(int i, int j)
  {
     return mmless(i, j) && mmexchange(i, j);
  }

  //maintains minheap property for all items below i.
  void minSortDown(int i)
  {
    for (i *= 2; i <= minCt; i *= 2)
    {
       if (i < minCt && mmless(i + 1, i))
         ++i;
       if (!mmCmpExch(i, i / 2))
         break;
    }
  }

  //maintains maxheap property for all items below i. (negative indexes)
  void maxSortDown(int i)
  {
    for (i *= 2; i >= -maxCt; i *= 2)
    {
      if (i > -maxCt && mmless(i, i - 1))
        --i;
      if (!mmCmpExch(i / 2, i))
        break;
    }
  }

  //maintains minheap property for all items above i, including median
  //returns true if median changed
  bool minSortUp(int i)
  {
    while (i > 0 && mmCmpExch(i, i / 2))
      i /= 2;
    return i == 0;
  }

  //maintains maxheap property for all items above i, including median
  //returns true if median changed
  bool maxSortUp(int i)
  {
     while (i < 0 && mmCmpExch(i / 2, i))
       i /= 2;
     return i == 0;
  }

protected:
  rolling_median_t &operator=(const rolling_median_t&) = delete;
  rolling_median_t(const rolling_median_t&) = delete;

public:
  //creates new rolling_median_t: to calculate `nItems` running median. 
  rolling_median_t(size_t N): N(N)
  {
    int size = N * (sizeof(Item) + sizeof(int) * 2);
    data = (Item*)malloc(size);
    pos = (int*) (data + N);
    heap = pos + N + (N / 2); //points to middle of storage.
    clear();
  }

  rolling_median_t(rolling_median_t &&m)
  {
    free(data);
    memcpy(this, &m, sizeof(rolling_median_t));
    m.data = NULL;
  }
  rolling_median_t &operator=(rolling_median_t &&m)
  {
    free(data);
    memcpy(this, &m, sizeof(rolling_median_t));
    m.data = NULL;
    return *this;
  }

  ~rolling_median_t()
  {
    free(data);
  }

  void clear()
  {
    idx = 0;
    minCt = 0;
    maxCt = 0;
    sz = 0;
    int nItems = N;
    while (nItems--)  //set up initial heap fill pattern: median,max,min,max,...
    {
      pos[nItems] = ((nItems + 1) / 2) * ((nItems & 1) ? -1 : 1);
      heap[pos[nItems]] = nItems;
    }
  }

  int size() const
  {
    return sz;
  }

  //Inserts item, maintains median in O(lg nItems)
  void insert(Item v)
  {
    int p = pos[idx];
    Item old = data[idx];
    data[idx] = v;
    idx = (idx + 1) % N;
    sz = std::min<int>(sz + 1, N);
    if (p > 0)         //new item is in minHeap
    {
      if (minCt < (N - 1) / 2)
      {
        ++minCt;
      }
      else if (v > old)
      {
        minSortDown(p);
        return;
      }
      if (minSortUp(p) && mmCmpExch(0, -1))
        maxSortDown(-1);
    }
    else if (p < 0)   //new item is in maxheap
    {
      if (maxCt < N / 2)
      {
        ++maxCt;
      }
      else if (v < old)
      {
        maxSortDown(p);
        return;
      }
      if (maxSortUp(p) && minCt && mmCmpExch(1, 0))
        minSortDown(1);
    }
    else //new item is at median
    {
      if (maxCt && maxSortUp(-1))
        maxSortDown(-1);
      if (minCt && minSortUp(1))
        minSortDown(1);
    }
  }

  //returns median item (or average of 2 when item count is even)
  Item median() const
  {
    Item v = data[heap[0]];
    if (minCt < maxCt)
    {
      v = (v + data[heap[-1]]) / 2;
    }
    return v;
  }
};

}
}
