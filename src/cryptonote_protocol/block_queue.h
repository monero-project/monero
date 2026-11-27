// Copyright (c) 2017-2024, The Monero Project
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

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <boost/optional.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/uuid/uuid.hpp>
#include "net/net_utils_base.h"
#include "crypto/crypto.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn.block_queue"

namespace cryptonote
{
  struct block_complete_entry;

  class block_queue
  {
  public:
    struct span
    {
      uint64_t start_block_height;
      crypto::hash parent{}; // hash preceding the span on its peer's chain
      std::vector<crypto::hash> hashes;
      std::vector<cryptonote::block_complete_entry> blocks;
      boost::uuids::uuid connection_id;
      uint64_t nblocks;
      float rate;
      size_t size;
      uint64_t sync_size; // includes prevalidated weights for pruned blocks
      mutable boost::posix_time::ptime time;
      epee::net_utils::network_address origin{};

      span(uint64_t start_block_height, std::vector<cryptonote::block_complete_entry> blocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, float rate, size_t size, uint64_t sync_size):
        start_block_height(start_block_height), blocks(std::move(blocks)), connection_id(connection_id), nblocks(this->blocks.size()), rate(rate), size(size), sync_size(sync_size), time(boost::date_time::min_date_time), origin(addr) {}
      span(uint64_t start_block_height, uint64_t nblocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, boost::posix_time::ptime time):
        start_block_height(start_block_height), connection_id(connection_id), nblocks(nblocks), rate(0.0f), size(0), sync_size(0), time(time), origin(addr) {}

      bool operator<(const span &s) const { return start_block_height < s.start_block_height; }
      bool operator<(uint64_t height) const { return start_block_height < height; }
      friend bool operator<(uint64_t height, const span &s) { return height < s.start_block_height; }
    };
    typedef std::multiset<span, std::less<>> block_map;

    struct scheduled_span
    {
      explicit scheduled_span(const span &s):
        start_block_height(s.start_block_height), nblocks(s.nblocks), connection_id(s.connection_id), time(s.time) {}
      uint64_t start_block_height;
      uint64_t nblocks;
      boost::uuids::uuid connection_id;
      boost::posix_time::ptime time;
    };

    struct download_limit
    {
      uint64_t chain_height;
      bool first_block_connects;
    };

  public:
    void add_blocks(uint64_t height, std::vector<cryptonote::block_complete_entry> bcel, std::vector<crypto::hash> hashes, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, float rate, size_t size, uint64_t sync_size, const crypto::hash &parent);
    bool add_blocks(uint64_t height, uint64_t nblocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, boost::posix_time::ptime time = boost::date_time::min_date_time);
    void flush_spans(const boost::uuids::uuid &connection_id, bool all = false);
    void flush_stale_spans(const std::set<boost::uuids::uuid> &live_connections);
    bool remove_span(uint64_t start_block_height);
    void remove_span(uint64_t height, const boost::uuids::uuid &connection_id, const crypto::hash &last_hash);
    uint64_t get_max_block_height() const;
    std::string get_overview(uint64_t blockchain_height) const;
    std::pair<uint64_t, uint64_t> reserve_span(uint64_t first_block_height, uint64_t last_block_height, uint64_t max_blocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, bool sync_pruned_blocks, uint32_t local_pruning_seed, uint32_t pruning_seed, uint64_t blockchain_height, const std::vector<std::pair<crypto::hash, uint64_t>> &block_hashes, boost::posix_time::ptime time = boost::posix_time::microsec_clock::universal_time(), boost::optional<download_limit> limit = boost::none, boost::optional<crypto::hash> first_parent = boost::none);
    uint64_t get_next_needed_height(uint64_t blockchain_height) const;
    boost::optional<scheduled_span> get_next_span_if_scheduled(uint64_t height) const;
    boost::optional<scheduled_span> get_scheduled_span_at(uint64_t height) const;
    bool claim_span(const scheduled_span &scheduled, uint64_t height, const std::vector<crypto::hash> &hashes, const crypto::hash &parent, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr);
    boost::optional<span> get_next_span(const std::unordered_set<crypto::hash> &waiting_parents = {}) const;
    bool has_next_span(const boost::uuids::uuid &connection_id, bool &filled, boost::posix_time::ptime &time) const;
    bool has_next_span(uint64_t height, bool &filled, boost::posix_time::ptime &time, boost::uuids::uuid &connection_id) const;
    bool has_filled_span(uint64_t blockchain_height) const;
    size_t get_data_size() const;
    size_t get_num_filled_spans() const;
    uint64_t get_num_filled_blocks() const;
    uint64_t get_max_block_size_average() const;
    float get_speed(const boost::uuids::uuid &connection_id) const;
    bool foreach(std::function<bool(const span&)> f) const;
    bool requested(const crypto::hash &hash) const;
    bool requested(const crypto::hash &hash, uint64_t height) const;
    bool have(const crypto::hash &hash) const;
    std::uint64_t have_height(const crypto::hash &hash) const;

  private:
    block_map::const_iterator find_span(uint64_t height) const;
    void insert_span(span s);
    void erase_block(block_map::iterator j);
    inline bool requested_internal(const crypto::hash &hash) const;

  private:
    block_map blocks;
    mutable boost::recursive_mutex mutex;
    std::unordered_multiset<crypto::hash> requested_hashes;
    std::unordered_multimap<crypto::hash, std::uint64_t> have_blocks;
  };
}
