// Copyright (c) 2016-2023, The Monero Project
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

#include <boost/optional/optional.hpp>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include "byte_slice.h"
#include "crypto/hash.h"
#include "cryptonote_config.h"

namespace cryptonote
{
class block;
class core;

namespace rpc
{

struct output_distribution_data
{
  std::vector<std::uint64_t> distribution;
  std::uint64_t start_height;
  std::uint64_t base;

  bool operator==(const output_distribution_data& other) const
  {
    return distribution == other.distribution
      && start_height == other.start_height
      && base == other.base
    ;
  }
};

class RpcHandler
{
  public:
    RpcHandler() { }
    virtual ~RpcHandler() { }

    virtual epee::byte_slice handle(std::string&& request) = 0;

    static boost::optional<output_distribution_data>
      get_output_distribution(const std::function<bool(uint64_t, uint64_t, uint64_t, uint64_t&, std::vector<uint64_t>&, uint64_t&)> &f, uint64_t amount, uint64_t from_height, uint64_t to_height, const std::function<crypto::hash(uint64_t)> &get_hash, bool cumulative, uint64_t blockchain_height);
};

class CoinbaseOutputDistributionCache
{
public:
  using get_rct_cb_dist_f = std::function<bool(uint64_t, uint64_t, uint64_t&, std::vector<uint64_t>&, uint64_t&)>;
  using get_block_id_by_height_f = std::function<crypto::hash(uint64_t)>;

  template <class GetRCTCoinbaseDistFunc, class GetBlockIdByHeightFunc>
  CoinbaseOutputDistributionCache
  (
    network_type net_type,
    GetRCTCoinbaseDistFunc&& grcd,
    GetBlockIdByHeightFunc&& gbibhf
  )
    : m_num_cb_outs_per_block()
    , m_height_begin()
    , m_last_1_hash()
    , m_last_10_hash()
    , m_last_100_hash()
    , m_net_type(net_type)
    , m_get_rct_cb_dist(grcd)
    , m_get_block_id_by_height(gbibhf)
    , m_mutex()
  {}

  bool get_coinbase_output_distribution
  (
    uint64_t start_height, // inclusive
    uint64_t stop_height, // inclusive
    uint64_t& true_start_height,
    std::vector<uint64_t>& num_cb_outs_per_block,
    uint64_t& base,
    bool* only_used_cache = nullptr
  );

private:
  uint64_t height_end() const // exclusive
  {
    return m_height_begin + m_num_cb_outs_per_block.size();
  };

  void rollback_for_reorgs();

  void save_current_checkpoints();

  std::vector<uint64_t> m_num_cb_outs_per_block; // NOT cumulative like RCT offsets
  uint64_t m_height_begin;
  crypto::hash m_last_1_hash;
  crypto::hash m_last_10_hash;
  crypto::hash m_last_100_hash;
  network_type m_net_type;

  get_rct_cb_dist_f m_get_rct_cb_dist;
  get_block_id_by_height_f m_get_block_id_by_height;

  std::mutex m_mutex;
};

}  // rpc

}  // cryptonote
