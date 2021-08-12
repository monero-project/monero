// Copyright (c) 2019-2021, The Monero Project
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

#pragma once

#include <cstdint>

#include "misc_language.h"


namespace cryptonote
{

  template<class type_vec_type>
  type_vec_type median(std::vector<type_vec_type> &v)
  {
    if(v.empty())
      return boost::value_initialized<type_vec_type>();
    if(v.size() == 1)
      return v[0];

    size_t n = (v.size()) / 2;
    std::sort(v.begin(), v.end());
    //nth_element(v.begin(), v.begin()+n-1, v.end());
    if(v.size()%2)
    {//1, 3, 5...
      return v[n];
    }else 
    {//2, 4, 6...
      return get_mid<type_vec_type>(v[n-1],v[n]);
    }
  }

  // binomial coefficient: n choose k
  // returns '0' if:
  // - n < k
  // - result overflows std::int32_t
  // - on unknown error
  std::int32_t n_choose_k(const std::uint32_t n, const std::uint32_t k);

}
