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

#include <vector>
#include <unordered_map>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "string_tools.h"
#include "cryptonote_protocol_defs.h"
#include "common/pruning.h"
#include "block_queue.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn.block_queue"

namespace std {
  static_assert(sizeof(size_t) <= sizeof(boost::uuids::uuid), "boost::uuids::uuid too small");
  template<> struct hash<boost::uuids::uuid> {
    std::size_t operator()(const boost::uuids::uuid &_v) const {
      return reinterpret_cast<const std::size_t &>(_v);
    }
  };
}

namespace cryptonote
{

void block_queue::add_blocks(uint64_t height, std::vector<cryptonote::block_complete_entry> bcel, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, float rate, size_t size)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  std::vector<crypto::hash> hashes;
  bool has_hashes = remove_span(height, &hashes);
  blocks.insert(span(height, std::move(bcel), connection_id, addr, rate, size));
  if (has_hashes)
  {
    for (const crypto::hash &h: hashes)
    {
      requested_hashes.insert(h);
      have_blocks.insert(h);
    }
    set_span_hashes(height, connection_id, hashes);
  }
}

void block_queue::add_blocks(uint64_t height, uint64_t nblocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, boost::posix_time::ptime time)
{
  CHECK_AND_ASSERT_THROW_MES(nblocks > 0, "Empty span");
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  blocks.insert(span(height, nblocks, connection_id, addr, time));
}

void block_queue::flush_spans(const boost::uuids::uuid &connection_id, bool all)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  block_map::iterator i = blocks.begin();
  while (i != blocks.end())
  {
    block_map::iterator j = i++;
    if (j->connection_id == connection_id && (all || j->blocks.size() == 0))
    {
      erase_block(j);
    }
  }
}

void block_queue::erase_block(block_map::iterator j)
{
  CHECK_AND_ASSERT_THROW_MES(j != blocks.end(), "Invalid iterator");
  for (const crypto::hash &h: j->hashes)
  {
    requested_hashes.erase(h);
    have_blocks.erase(h);
  }
  blocks.erase(j);
}

void block_queue::flush_stale_spans(const std::set<boost::uuids::uuid> &live_connections)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  block_map::iterator i = blocks.begin();
  while (i != blocks.end())
  {
    block_map::iterator j = i++;
    if (j->blocks.empty() && live_connections.find(j->connection_id) == live_connections.end())
    {
      erase_block(j);
    }
  }
}

bool block_queue::remove_span(uint64_t start_block_height, std::vector<crypto::hash> *hashes)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); ++i)
  {
    if (i->start_block_height == start_block_height)
    {
      if (hashes)
        *hashes = std::move(i->hashes);
      erase_block(i);
      return true;
    }
  }
  return false;
}

void block_queue::remove_spans(const boost::uuids::uuid &connection_id, uint64_t start_block_height)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); )
  {
    block_map::iterator j = i++;
    if (j->connection_id == connection_id && j->start_block_height <= start_block_height)
    {
      erase_block(j);
    }
  }
}

uint64_t block_queue::get_max_block_height() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  uint64_t height = 0;
  for (const auto &span: blocks)
  {
    const uint64_t h = span.start_block_height + span.nblocks - 1;
    if (h > height)
      height = h;
  }
  return height;
}

uint64_t block_queue::get_next_needed_height(uint64_t blockchain_height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return blockchain_height;
  uint64_t last_needed_height = blockchain_height;
  bool first = true;
  for (const auto &span: blocks)
  {
    if (span.start_block_height + span.nblocks - 1 < blockchain_height)
      continue;
    if (span.start_block_height != last_needed_height || (first && span.blocks.empty()))
      return last_needed_height;
    last_needed_height = span.start_block_height + span.nblocks;
    first = false;
  }
  return last_needed_height;
}

void block_queue::print() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  MDEBUG("Block queue has " << blocks.size() << " spans");
  for (const auto &span: blocks)
    MDEBUG("  " << span.start_block_height << " - " << (span.start_block_height+span.nblocks-1) << " (" << span.nblocks << ") - " << (span.blocks.empty() ? "scheduled" : "filled    ") << "  " << span.connection_id << " (" << ((unsigned)(span.rate*10/1024.f))/10.f << " kB/s)");
}

std::string block_queue::get_overview(uint64_t blockchain_height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return "[]";
  block_map::const_iterator i = blocks.begin();
  std::string s = std::string("[");
  uint64_t expected = blockchain_height;
  while (i != blocks.end())
  {
    if (expected > i->start_block_height)
    {
      s += "<";
    }
    else
    {
      if (expected < i->start_block_height)
        s += std::string(std::max((uint64_t)1, (i->start_block_height - expected) / (i->nblocks ? i->nblocks : 1)), '_');
      s += i->blocks.empty() ? "." : i->start_block_height == blockchain_height ? "m" : "o";
      expected = i->start_block_height + i->nblocks;
    }
    ++i;
  }
  s += "]";
  return s;
}

inline bool block_queue::requested_internal(const crypto::hash &hash) const
{
  return requested_hashes.find(hash) != requested_hashes.end();
}

bool block_queue::requested(const crypto::hash &hash) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  return requested_internal(hash);
}

bool block_queue::have(const crypto::hash &hash) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  return have_blocks.find(hash) != have_blocks.end();
}

std::pair<uint64_t, uint64_t> block_queue::reserve_span(uint64_t first_block_height, uint64_t last_block_height, uint64_t max_blocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, bool sync_pruned_blocks, uint32_t local_pruning_seed, uint32_t pruning_seed, uint64_t blockchain_height, const std::vector<std::pair<crypto::hash, uint64_t>> &block_hashes, boost::posix_time::ptime time)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);

  MDEBUG("reserve_span: first_block_height " << first_block_height << ", last_block_height " << last_block_height
      << ", max " << max_blocks << ", peer seed " << epee::string_tools::to_string_hex(pruning_seed) << ", blockchain_height " <<
      blockchain_height << ", block hashes size " << block_hashes.size() << ", local seed " << epee::string_tools::to_string_hex(local_pruning_seed)
      << ", sync_pruned_blocks " << sync_pruned_blocks);
  if (last_block_height < first_block_height || max_blocks == 0)
  {
    MDEBUG("reserve_span: early out: first_block_height " << first_block_height << ", last_block_height " << last_block_height << ", max_blocks " << max_blocks);
    return std::make_pair(0, 0);
  }
  if (block_hashes.size() > last_block_height)
  {
    MDEBUG("reserve_span: more block hashes than fit within last_block_height: " << block_hashes.size() << " and " << last_block_height);
    return std::make_pair(0, 0);
  }

  // skip everything we've already requested
  uint64_t span_start_height = last_block_height - block_hashes.size() + 1;
  std::vector<std::pair<crypto::hash, uint64_t>>::const_iterator i = block_hashes.begin();
  while (i != block_hashes.end() && requested_internal((*i).first))
  {
    ++i;
    ++span_start_height;
  }

  if (!sync_pruned_blocks)
  {
    // if the peer's pruned for the starting block and its unpruned stripe comes next, start downloading from there
    const uint32_t next_unpruned_height = tools::get_next_unpruned_block_height(span_start_height, blockchain_height, pruning_seed);
    MDEBUG("reserve_span: next_unpruned_height " << next_unpruned_height << " from " << span_start_height << " and seed "
        << epee::string_tools::to_string_hex(pruning_seed) << ", limit " << span_start_height + CRYPTONOTE_PRUNING_STRIPE_SIZE);
    if (next_unpruned_height > span_start_height && next_unpruned_height < span_start_height + CRYPTONOTE_PRUNING_STRIPE_SIZE)
    {
      MDEBUG("We can download from next span: ideal height " << span_start_height << ", next unpruned height " << next_unpruned_height <<
          "(+" << next_unpruned_height - span_start_height << "), current seed " << pruning_seed);
      span_start_height = next_unpruned_height;
    }
  }
  MDEBUG("span_start_height: " <<span_start_height);
  const uint64_t block_hashes_start_height = last_block_height - block_hashes.size() + 1;
  if (span_start_height >= block_hashes.size() + block_hashes_start_height)
  {
    MDEBUG("Out of hashes, cannot reserve");
    return std::make_pair(0, 0);
  }

  i = block_hashes.begin() + span_start_height - block_hashes_start_height;
  while (i != block_hashes.end() && requested_internal((*i).first))
  {
    ++i;
    ++span_start_height;
  }

  uint64_t span_length = 0;
  std::vector<crypto::hash> hashes;
  bool first_is_pruned = sync_pruned_blocks && !tools::has_unpruned_block(span_start_height + span_length, blockchain_height, local_pruning_seed);
  while (i != block_hashes.end() && span_length < max_blocks && (sync_pruned_blocks || tools::has_unpruned_block(span_start_height + span_length, blockchain_height, pruning_seed)))
  {
    // if we want to sync pruned blocks, stop at the first block for which we need full data
    if (sync_pruned_blocks && first_is_pruned == tools::has_unpruned_block(span_start_height + span_length, blockchain_height, local_pruning_seed))
    {
      MDEBUG("Stopping at " << span_start_height + span_length << " for peer on stripe " << tools::get_pruning_stripe(pruning_seed) << " as we need full data for " << tools::get_pruning_stripe(local_pruning_seed));
      break;
    }
    hashes.push_back((*i).first);
    ++i;
    ++span_length;
  }
  if (span_length == 0)
  {
    MDEBUG("span_length 0, cannot reserve");
    return std::make_pair(0, 0);
  }
  MDEBUG("Reserving span " << span_start_height << " - " << (span_start_height + span_length - 1) << " for " << connection_id);
  add_blocks(span_start_height, span_length, connection_id, addr, time);
  set_span_hashes(span_start_height, connection_id, hashes);
  return std::make_pair(span_start_height, span_length);
}

std::pair<uint64_t, uint64_t> block_queue::get_next_span_if_scheduled(std::vector<crypto::hash> &hashes, boost::uuids::uuid &connection_id, boost::posix_time::ptime &time) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return std::make_pair(0, 0);
  block_map::const_iterator i = blocks.begin();
  if (i == blocks.end())
    return std::make_pair(0, 0);
  if (!i->blocks.empty())
    return std::make_pair(0, 0);
  hashes = i->hashes;
  connection_id = i->connection_id;
  time = i->time;
  return std::make_pair(i->start_block_height, i->nblocks);
}

void block_queue::reset_next_span_time(boost::posix_time::ptime t)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  CHECK_AND_ASSERT_THROW_MES(!blocks.empty(), "No next span to reset time");
  block_map::iterator i = blocks.begin();
  CHECK_AND_ASSERT_THROW_MES(i != blocks.end(), "No next span to reset time");
  CHECK_AND_ASSERT_THROW_MES(i->blocks.empty(), "Next span is not empty");
  (boost::posix_time::ptime&)i->time = t; // sod off, time doesn't influence sorting
}

void block_queue::set_span_hashes(uint64_t start_height, const boost::uuids::uuid &connection_id, std::vector<crypto::hash> hashes)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); ++i)
  {
    if (i->start_block_height == start_height && i->connection_id == connection_id)
    {
      span s = *i;
      erase_block(i);
      s.hashes = std::move(hashes);
      for (const crypto::hash &h: s.hashes)
        requested_hashes.insert(h);
      blocks.insert(s);
      return;
    }
  }
}

bool block_queue::get_next_span(uint64_t &height, std::vector<cryptonote::block_complete_entry> &bcel, boost::uuids::uuid &connection_id, epee::net_utils::network_address &addr, bool filled) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return false;
  block_map::const_iterator i = blocks.begin();
  for (; i != blocks.end(); ++i)
  {
    if (!filled || !i->blocks.empty())
    {
      height = i->start_block_height;
      bcel = i->blocks;
      connection_id = i->connection_id;
      addr = i->origin;
      return true;
    }
  }
  return false;
}

bool block_queue::has_next_span(const boost::uuids::uuid &connection_id, bool &filled, boost::posix_time::ptime &time) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return false;
  block_map::const_iterator i = blocks.begin();
  if (i == blocks.end())
    return false;
  if (i->connection_id != connection_id)
    return false;
  filled = !i->blocks.empty();
  time = i->time;
  return true;
}

bool block_queue::has_next_span(uint64_t height, bool &filled, boost::posix_time::ptime &time, boost::uuids::uuid &connection_id) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return false;
  block_map::const_iterator i = blocks.begin();
  if (i == blocks.end())
    return false;
  if (i->start_block_height > height)
    return false;
  filled = !i->blocks.empty();
  time = i->time;
  connection_id = i->connection_id;
  return true;
}

size_t block_queue::get_data_size() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  size_t size = 0;
  for (const auto &span: blocks)
    size += span.size;
  return size;
}

size_t block_queue::get_num_filled_spans_prefix() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);

  if (blocks.empty())
    return 0;
  block_map::const_iterator i = blocks.begin();
  size_t size = 0;
  while (i != blocks.end() && !i->blocks.empty())
  {
    ++i;
    ++size;
  }
  return size;
}

size_t block_queue::get_num_filled_spans() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  size_t size = 0;
  for (const auto &span: blocks)
  if (!span.blocks.empty())
    ++size;
  return size;
}

crypto::hash block_queue::get_last_known_hash(const boost::uuids::uuid &connection_id) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  crypto::hash hash = crypto::null_hash;
  uint64_t highest_height = 0;
  for (const auto &span: blocks)
  {
    if (span.connection_id != connection_id)
      continue;
    uint64_t h = span.start_block_height + span.nblocks - 1;
    if (h > highest_height && span.hashes.size() == span.nblocks)
    {
      hash = span.hashes.back();
      highest_height = h;
    }
  }
  return hash;
}

bool block_queue::has_spans(const boost::uuids::uuid &connection_id) const
{
  for (const auto &span: blocks)
  {
    if (span.connection_id == connection_id)
      return true;
  }
  return false;
}

float block_queue::get_speed(const boost::uuids::uuid &connection_id) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  std::unordered_map<boost::uuids::uuid, float> speeds;
  for (const auto &span: blocks)
  {
    if (span.blocks.empty())
      continue;
    // note that the average below does not average over the whole set, but over the
    // previous pseudo average and the latest rate: this gives much more importance
    // to the latest measurements, which is fine here
    std::unordered_map<boost::uuids::uuid, float>::iterator i = speeds.find(span.connection_id);
    if (i == speeds.end())
      speeds.insert(std::make_pair(span.connection_id, span.rate));
    else
      i->second = (i->second + span.rate) / 2;
  }
  float conn_rate = -1, best_rate = 0;
  for (const auto &i: speeds)
  {
    if (i.first == connection_id)
      conn_rate = i.second;
    if (i.second > best_rate)
      best_rate = i.second;
  }

  if (conn_rate <= 0)
    return 1.0f; // not found, assume good speed
  if (best_rate == 0)
    return 1.0f; // everything dead ? Can't happen, but let's trap anyway

  const float speed = conn_rate / best_rate;
  MTRACE(" Relative speed for " << connection_id << ": " << speed << " (" << conn_rate << "/" << best_rate);
  return speed;
}

float block_queue::get_download_rate(const boost::uuids::uuid &connection_id) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  float conn_rate = -1.f;
  for (const auto &span: blocks)
  {
    if (span.blocks.empty())
      continue;
    if (span.connection_id != connection_id)
      continue;
    // note that the average below does not average over the whole set, but over the
    // previous pseudo average and the latest rate: this gives much more importance
    // to the latest measurements, which is fine here
    if (conn_rate < 0.f)
      conn_rate = span.rate;
    else
      conn_rate = (conn_rate + span.rate) / 2;
  }

  if (conn_rate < 0)
    conn_rate = 0.0f;
  MTRACE("Download rate for " << connection_id << ": " << conn_rate << " b/s");
  return conn_rate;
}

bool block_queue::foreach(std::function<bool(const span&)> f) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  block_map::const_iterator i = blocks.begin();
  while (i != blocks.end())
    if (!f(*i++))
      return false;
  return true;
}

}
