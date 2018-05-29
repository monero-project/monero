// Copyright (c) 2017, The Monero Project
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
// Adapted from Python code by Sarang Noether

#pragma once

#ifndef MULTIEXP_H
#define MULTIEXP_H

#include <vector>
#include "crypto/crypto.h"
#include "rctTypes.h"
#include "misc_log_ex.h"

namespace rct
{

struct MultiexpData {
  rct::key scalar;
  ge_p3 point;

  MultiexpData() {}
  MultiexpData(const rct::key &s, const ge_p3 &p): scalar(s), point(p) {}
  MultiexpData(const rct::key &s, const rct::key &p): scalar(s)
  {
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&point, p.bytes) == 0, "ge_frombytes_vartime failed");
  }
};

struct straus_cached_data;
struct pippenger_cached_data;

rct::key bos_coster_heap_conv(std::vector<MultiexpData> data);
rct::key bos_coster_heap_conv_robust(std::vector<MultiexpData> data);
std::shared_ptr<straus_cached_data> straus_init_cache(const std::vector<MultiexpData> &data, size_t N =0);
size_t straus_get_cache_size(const std::shared_ptr<straus_cached_data> &cache);
rct::key straus(const std::vector<MultiexpData> &data, const std::shared_ptr<straus_cached_data> &cache = NULL, size_t STEP = 0);
std::shared_ptr<pippenger_cached_data> pippenger_init_cache(const std::vector<MultiexpData> &data, size_t N =0);
size_t pippenger_get_cache_size(const std::shared_ptr<pippenger_cached_data> &cache);
size_t get_pippenger_c(size_t N);
rct::key pippenger(const std::vector<MultiexpData> &data, const std::shared_ptr<pippenger_cached_data> &cache = NULL, size_t c = 0);

}

#endif
