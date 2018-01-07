// Copyright (c) 2017-2018, The Monero Project
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
// Most of this file is originally copyright (c) 2017 Raymond Chen, Microsoft
// This algorithm is adapted from Raymond Chen's code:
// https://blogs.msdn.microsoft.com/oldnewthing/20170109-00/?p=95145

#pragma once

#include <vector>
#include <functional>
#include "misc_log_ex.h"

namespace tools
{

template<typename F>
void apply_permutation(std::vector<size_t> permutation, const F &swap)
{
  //sanity check
  for (size_t n = 0; n < permutation.size(); ++n)
    CHECK_AND_ASSERT_THROW_MES(std::find(permutation.begin(), permutation.end(), n) != permutation.end(), "Bad permutation");

  for (size_t i = 0; i < permutation.size(); ++i)
  {
    size_t current = i;
    while (i != permutation[current])
    {
      size_t next = permutation[current];
      swap(current, next);
      permutation[current] = current;
      current = next;
    }
    permutation[current] = current;
  }
}

template<typename T>
void apply_permutation(const std::vector<size_t> &permutation, std::vector<T> &v)
{
  CHECK_AND_ASSERT_THROW_MES(permutation.size() == v.size(), "Mismatched vector sizes");
  apply_permutation(permutation, [&v](size_t i0, size_t i1){ std::swap(v[i0], v[i1]); });
}

}
