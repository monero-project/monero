// Copyright (c) 2018, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <iostream>
#include <vector>

namespace tools {

uint64_t combinations_count(uint32_t k, uint32_t n);

template<typename T>
class Combinator {
public:
  Combinator(const std::vector<T>& v) : origin(v) { }

  std::vector<std::vector<T>> combine(size_t k);

private:
  void doCombine(size_t from, size_t k);

  std::vector<T> origin;
  std::vector<std::vector<T>> combinations;
  std::vector<size_t> current;
};

template<typename T>
std::vector<std::vector<T>> Combinator<T>::combine(size_t k)
{
  if (k > origin.size())
  {
    throw std::runtime_error("k must be smaller than elements number");
  }

  if (k == 0)
  {
    throw std::runtime_error("k must be greater than zero");
  }

  combinations.clear();
  doCombine(0, k);
  return combinations;
}

template<typename T>
void Combinator<T>::doCombine(size_t from, size_t k)
{
  current.push_back(0);

  for (size_t i = from; i <= origin.size() - k; ++i)
  {
    current.back() = i;

    if (k > 1) {
      doCombine(i + 1, k - 1);
    } else {
      std::vector<T> comb;
      for (auto ind: current) {
        comb.push_back(origin[ind]);
      }
      combinations.push_back(comb);
    }
  }

  current.pop_back();
}

} //namespace tools
