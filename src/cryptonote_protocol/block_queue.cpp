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

#include <algorithm>
#include <iterator>
#include <vector>
#include <unordered_map>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include "string_tools.h"
#include "cryptonote_config.h"
#include "cryptonote_protocol_defs.h"
#include "common/pruning.h"
#include "block_queue.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn.block_queue"

namespace cryptonote
{

void block_queue::add_blocks(uint64_t height, std::vector<cryptonote::block_complete_entry> bcel, std::vector<crypto::hash> hashes, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, float rate, size_t size, uint64_t sync_size, const crypto::hash &parent)
{
  CHECK_AND_ASSERT_THROW_MES(!bcel.empty() && bcel.size() == hashes.size(), "Invalid filled span hashes");
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (auto i = blocks.lower_bound(height); i != blocks.end() && i->start_block_height - height < hashes.size(); )
  {
    const auto existing = i++;
    const size_t offset = existing->start_block_height - height;
    // A partial or competing reply leaves the owner's request in flight.
    if (existing->blocks.empty() && existing->nblocks <= hashes.size() - offset &&
        std::equal(existing->hashes.begin(), existing->hashes.end(), hashes.begin() + offset))
      erase_block(existing);
  }
  span s(height, std::move(bcel), connection_id, addr, rate, size, sync_size);
  s.parent = parent;
  s.hashes = std::move(hashes);
  insert_span(std::move(s));
}

bool block_queue::add_blocks(uint64_t height, uint64_t nblocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, boost::posix_time::ptime time)
{
  CHECK_AND_ASSERT_THROW_MES(nblocks > 0, "Empty span");
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.find(height) != blocks.end())
    return false;
  blocks.emplace(height, nblocks, connection_id, addr, time);
  return true;
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

void block_queue::insert_span(span s)
{
  const span &added = *blocks.insert(std::move(s));
  for (std::size_t i = 0; i < added.hashes.size(); ++i)
  {
    requested_hashes.insert(added.hashes[i]);
    if (!added.blocks.empty())
      have_blocks.emplace(added.hashes[i], added.start_block_height + i);
  }
}

void block_queue::erase_block(block_map::iterator j)
{
  MDEBUG("Erasing span starting from block " << j->start_block_height);
  CHECK_AND_ASSERT_THROW_MES(j != blocks.end(), "Invalid iterator");
  // Overlapping spans can share a hash. Erase one occurrence for this span.
  for (std::size_t i = 0; i < j->hashes.size(); ++i)
  {
    const auto requested = requested_hashes.find(j->hashes[i]);
    if (requested != requested_hashes.end())
      requested_hashes.erase(requested);
    if (!j->blocks.empty())
    {
      const auto entries = have_blocks.equal_range(j->hashes[i]);
      const auto height = j->start_block_height + i;
      const auto found = std::find_if(entries.first, entries.second, [height](const auto &entry) { return entry.second == height; });
      if (found != entries.second)
        have_blocks.erase(found);
    }
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

bool block_queue::remove_span(uint64_t start_block_height)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  const auto range = blocks.equal_range(start_block_height);
  if (range.first == range.second || std::next(range.first) != range.second)
    return false;
  erase_block(range.first);
  return true;
}

void block_queue::remove_span(uint64_t height, const boost::uuids::uuid &connection_id, const crypto::hash &last_hash)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  const auto range = blocks.equal_range(height);
  for (auto i = range.first; i != range.second; ++i)
  {
    if (i->connection_id == connection_id && !i->blocks.empty() && i->hashes.back() == last_hash)
    {
      erase_block(i);
      return;
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
  {
    MDEBUG("Span queue is empty");
    return blockchain_height;
  }

  uint64_t covered_until = blockchain_height;

  for (const auto &span: blocks)
  {
    // Ignore spans entirely below current chain height
    const uint64_t span_end = span.start_block_height + span.nblocks - 1;
    if (span_end < blockchain_height)
      continue;

    // If this span starts after what we already have/scheduled, we found the first gap
    if (span.start_block_height > covered_until)
    {
      MDEBUG("Found gap in the span queue from " << covered_until << " to " << span.start_block_height);
      return covered_until;
    }

    // This span overlaps or is adjacent; extend coverage regardless of filled/scheduled
    if (span.start_block_height <= covered_until)
      covered_until = std::max(covered_until, span.start_block_height + span.nblocks);
  }

  return covered_until;
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

bool block_queue::requested(const crypto::hash &hash, uint64_t height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (const auto &span: blocks)
  {
    if (span.start_block_height > height)
      break;
    const uint64_t offset = height - span.start_block_height;
    if (offset < span.hashes.size() && span.hashes[offset] == hash)
      return true;
  }
  return false;
}

bool block_queue::have(const crypto::hash &hash) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  return have_blocks.find(hash) != have_blocks.end();
}

std::uint64_t block_queue::have_height(const crypto::hash &hash) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  const auto elem = have_blocks.find(hash);
  if (elem == have_blocks.end())
    return std::numeric_limits<std::uint64_t>::max();
  return elem->second;
}


std::pair<uint64_t, uint64_t> block_queue::reserve_span(uint64_t first_block_height, uint64_t last_block_height, uint64_t max_blocks, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr, bool sync_pruned_blocks, uint32_t local_pruning_seed, uint32_t pruning_seed, uint64_t blockchain_height, const std::vector<std::pair<crypto::hash, uint64_t>> &block_hashes, boost::posix_time::ptime time, boost::optional<download_limit> limit, boost::optional<crypto::hash> first_parent)
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

  if (limit && span_start_height != limit->chain_height)
  {
    // A full queue can still accept one earlier span that connects to a known parent.
    if (span_start_height > limit->chain_height || span_start_height != first_block_height ||
        !limit->first_block_connects || (!blocks.empty() && blocks.begin()->start_block_height < limit->chain_height))
      return std::make_pair(0, 0);
  }

  uint64_t span_length = 0;
  std::vector<crypto::hash> hashes;
  bool first_is_pruned = sync_pruned_blocks && !tools::has_unpruned_block(span_start_height + span_length, blockchain_height, local_pruning_seed);
  while (i != block_hashes.end() && span_length < max_blocks && !requested_internal((*i).first) &&
      (sync_pruned_blocks || tools::has_unpruned_block(span_start_height + span_length, blockchain_height, pruning_seed)))
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
  span scheduled(span_start_height, span_length, connection_id, addr, time);
  const uint64_t offset = span_start_height - block_hashes_start_height;
  scheduled.parent = offset ? block_hashes[offset - 1].first : first_parent.value_or(crypto::null_hash);
  scheduled.hashes = std::move(hashes);
  if (blocks.find(span_start_height) != blocks.end())
  {
    MDEBUG("Span already starts at height " << span_start_height << ", cannot reserve");
    return std::make_pair(0, 0);
  }
  insert_span(std::move(scheduled));
  MDEBUG("Reserving span " << span_start_height << " - " << (span_start_height + span_length - 1) << " for " << connection_id);
  return std::make_pair(span_start_height, span_length);
}

block_queue::block_map::const_iterator block_queue::find_span(uint64_t height) const
{
  auto span = blocks.end();
  auto first_filled = blocks.end();
  for (auto i = blocks.begin(); i != blocks.end() && i->start_block_height <= height; ++i)
  {
    if (first_filled == blocks.end() && !i->blocks.empty())
      first_filled = i;
    if (height - i->start_block_height < i->nblocks)
    {
      if (!i->blocks.empty())
      {
        span = i;
        break;
      }
      if (span == blocks.end() || i->start_block_height != span->start_block_height || i->nblocks > span->nblocks)
        span = i;
    }
  }
  // The adder processes the earliest filled span first. Follow its fork back
  // through any reservations that must finish before that span can be added.
  if (first_filled == blocks.end())
    return span;
  auto child = first_filled;
  for (auto i = first_filled; i != blocks.begin() && child->parent != crypto::null_hash && requested_internal(child->parent); )
  {
    --i;
    if (i->start_block_height >= child->start_block_height)
      continue;
    const uint64_t offset = child->start_block_height - i->start_block_height - 1;
    if (offset < i->hashes.size() && i->hashes[offset] == child->parent)
    {
      child = i;
      span = i;
    }
  }
  return span;
}

boost::optional<block_queue::scheduled_span> block_queue::get_next_span_if_scheduled(uint64_t height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  auto i = find_span(height);
  if (i == blocks.end())
    i = blocks.begin();
  if (i == blocks.end() || !i->blocks.empty())
    return boost::none;
  return scheduled_span(*i);
}

boost::optional<block_queue::scheduled_span> block_queue::get_scheduled_span_at(uint64_t height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  const auto range = blocks.equal_range(height);
  auto widest = range.second;
  for (auto i = range.first; i != range.second; ++i)
  {
    if (!i->blocks.empty())
      return boost::none;
    if (widest == range.second || i->nblocks > widest->nblocks)
      widest = i;
  }
  if (widest == range.second)
    return boost::none;
  return scheduled_span(*widest);
}

bool block_queue::claim_span(const scheduled_span &scheduled, uint64_t height, const std::vector<crypto::hash> &hashes, const crypto::hash &parent, const boost::uuids::uuid &connection_id, const epee::net_utils::network_address &addr)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (hashes.empty() || height < scheduled.start_block_height)
    return false;
  const uint64_t offset = height - scheduled.start_block_height;
  if (offset >= scheduled.nblocks || hashes.size() > scheduled.nblocks - offset)
    return false;
  const auto range = blocks.equal_range(scheduled.start_block_height);
  const auto observed = std::find_if(range.first, range.second, [&](const span &s) {
    return s.blocks.empty() && s.connection_id == scheduled.connection_id && s.nblocks == scheduled.nblocks && s.time == scheduled.time;
  });
  if (observed == range.second)
    return false;
  const auto target = blocks.equal_range(height);
  if (offset && target.first != target.second)
    return false;
  for (auto i = target.first; i != target.second; ++i)
    if (!i->blocks.empty() || i->connection_id == connection_id)
      return false;

  const auto now = boost::posix_time::microsec_clock::universal_time();
  if (now <= observed->time)
    return false;
  span retry(height, hashes.size(), connection_id, addr, now);
  retry.parent = parent;
  retry.hashes = hashes;
  for (auto i = range.first; i != range.second; ++i)
    if (i->blocks.empty())
      i->time = now;
  insert_span(std::move(retry));
  return true;
}

boost::optional<block_queue::span> block_queue::get_next_span(const std::unordered_set<crypto::hash> &waiting_parents) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  auto i = std::find_if(blocks.begin(), blocks.end(), [](const span &s) { return !s.blocks.empty(); });
  if (i == blocks.end())
    return boost::none;
  const auto end = blocks.upper_bound(i->start_block_height);
  for (; i != end; ++i)
    if (!i->blocks.empty() && !waiting_parents.count(i->parent))
      return *i;
  return boost::none;
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
  const auto i = find_span(height);
  if (i == blocks.end())
    return false;
  filled = !i->blocks.empty();
  time = i->time;
  connection_id = i->connection_id;
  return true;
}

bool block_queue::has_filled_span(uint64_t blockchain_height) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (const auto &span: blocks)
  {
    if (span.start_block_height > blockchain_height)
      break;
    if (!span.blocks.empty())
      return true;
  }
  return false;
}

size_t block_queue::get_data_size() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  size_t size = 0;
  for (const auto &span: blocks)
    size += span.size;
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

uint64_t block_queue::get_num_filled_blocks() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  uint64_t size = 0;
  for (const auto &span: blocks)
  {
    if (!span.blocks.empty())
      size += span.nblocks;
  }
  return size;
}

uint64_t block_queue::get_max_block_size_average() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  uint64_t max_average = 0;
  for (const auto &span: blocks)
  {
    if (span.blocks.empty())
      continue;

    max_average = std::max(max_average, span.sync_size / span.nblocks);
  }
  return max_average;
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
    const auto i = speeds.find(span.connection_id);
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
