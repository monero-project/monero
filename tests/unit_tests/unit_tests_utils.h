// Copyright (c) 2014-2024, The Monero Project
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

#include <atomic>
#include <boost/filesystem.hpp>

#include "fcmp_pp/fcmp_pp_types.h"

namespace unit_test
{
  extern boost::filesystem::path data_dir;

  class call_counter
  {
  public:
    call_counter()
      : m_counter(0)
    {
    }

    void inc() volatile
    {
      // memory_order_relaxed is enough for call counter
      m_counter.fetch_add(1, std::memory_order_relaxed);
    }

    size_t get() volatile const
    {
      return m_counter.load(std::memory_order_relaxed);
    }

    void reset() volatile
    {
      m_counter.store(0, std::memory_order_relaxed);
    }

  private:
    std::atomic<size_t> m_counter;
  };

  bool write_fcmp_pp_verify_input_to_file(
    const std::size_t n_inputs,
    const crypto::hash &signable_tx_hash,
    const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    const uint8_t n_layers,
    const crypto::ec_point &tree_root_bytes,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images);

  bool read_fcmp_pp_verify_input_from_file(
    const std::size_t n_inputs,
    crypto::hash &signable_tx_hash,
    fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    uint8_t &n_layers,
    fcmp_pp::TreeRootShared &tree_root,
    std::vector<crypto::ec_point> &pseudo_outs,
    std::vector<crypto::key_image> &key_images);
}

# define ASSERT_EQ_MAP(val, map, key) \
  do { \
    auto found = map.find(key); \
    ASSERT_TRUE(found != map.end()); \
    ASSERT_EQ(val, found->second); \
  } while (false)
