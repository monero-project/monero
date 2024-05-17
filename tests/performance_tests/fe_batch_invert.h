// Copyright (c) 2024, The Monero Project
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

#include "crypto/crypto.h"

template<bool batched>
class test_fe_batch_invert
{
public:
  static const size_t loop_count = 50;
  static const size_t n_elems = 1000;

  bool init()
  {
    m_fes = (fe *) malloc(n_elems * sizeof(fe));

    for (std::size_t i = 0; i < n_elems; ++i)
    {
      crypto::secret_key r;
      crypto::random32_unbiased((unsigned char*)r.data);

      ge_p3 point;
      ge_scalarmult_base(&point, (unsigned char*)r.data);

      memcpy(m_fes[i], &point.Y, sizeof(fe));
    }

    return true;
  }

  bool test()
  {
    fe *inv_fes = (fe *) malloc(n_elems * sizeof(fe));

    if (batched)
      fe_batch_invert(inv_fes, m_fes, n_elems);
    else
    {
      for (std::size_t i = 0; i < n_elems; ++i)
        fe_invert(inv_fes[i], m_fes[i]);
    }

    free(inv_fes);

    return true;
  }

private:
  fe *m_fes;
};
