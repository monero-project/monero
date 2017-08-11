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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <vector>
#include <unordered_map>
#include <boost/uuid/nil_generator.hpp>
#include "cryptonote_protocol_defs.h"
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

void block_queue::add_blocks(uint64_t height, std::list<cryptonote::block_complete_entry> bcel, const boost::uuids::uuid &connection_id, float rate, size_t size)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  remove_span(height);
  blocks.insert(span(height, std::move(bcel), connection_id, rate, size));
}

void block_queue::add_blocks(uint64_t height, uint64_t nblocks, const boost::uuids::uuid &connection_id, boost::posix_time::ptime time)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  blocks.insert(span(height, nblocks, connection_id, time));
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
      blocks.erase(j);
    }
  }
}

void block_queue::remove_span(uint64_t start_block_height)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); ++i)
  {
    if (i->start_block_height == start_block_height)
    {
      blocks.erase(i);
      return;
    }
  }
}

void block_queue::remove_spans(const boost::uuids::uuid &connection_id, uint64_t start_block_height)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); )
  {
    block_map::iterator j = i++;
    if (j->connection_id == connection_id && j->start_block_height <= start_block_height)
    {
      blocks.erase(j);
    }
  }
}

void block_queue::mark_last_block(uint64_t last_block_height)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (!blocks.empty() && is_blockchain_placeholder(*blocks.begin()))
    blocks.erase(*blocks.begin());
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); )
  {
    block_map::iterator j = i++;
    if (j->start_block_height + j->nblocks - 1 <= last_block_height)
    {
      blocks.erase(j);
    }
  }

  // mark the current state of the db (for a fresh db, it's just the genesis block)
  add_blocks(0, last_block_height + 1, boost::uuids::nil_uuid());
  MDEBUG("last blocked marked at " << last_block_height);
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

void block_queue::print() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  MDEBUG("Block queue has " << blocks.size() << " spans");
  for (const auto &span: blocks)
    MDEBUG("  " << span.start_block_height << " - " << (span.start_block_height+span.nblocks-1) << " (" << span.nblocks << ") - " << (is_blockchain_placeholder(span) ? "blockchain" : span.blocks.empty() ? "scheduled" : "filled    ") << "  " << span.connection_id << " (" << ((unsigned)(span.rate*10/1024.f))/10.f << " kB/s)");
}

std::string block_queue::get_overview() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return "[]";
  block_map::const_iterator i = blocks.begin();
  std::string s = std::string("[") + std::to_string(i->start_block_height + i->nblocks - 1) + ":";
  while (++i != blocks.end())
    s += i->blocks.empty() ? "." : "o";
  s += "]";
  return s;
}

std::pair<uint64_t, uint64_t> block_queue::reserve_span(uint64_t first_block_height, uint64_t last_block_height, uint64_t max_blocks, const boost::uuids::uuid &connection_id, boost::posix_time::ptime time)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);

  if (last_block_height < first_block_height || max_blocks == 0)
  {
    MDEBUG("reserve_span: early out: first_block_height " << first_block_height << ", last_block_height " << last_block_height << ", max_blocks " << max_blocks);
    return std::make_pair(0, 0);
  }

  uint64_t max_block_height = get_max_block_height();
  if (last_block_height > max_block_height)
    max_block_height = last_block_height;
  if (max_block_height == 0)
  {
    MDEBUG("reserve_span: max_block_height is 0");
    return std::make_pair(first_block_height, std::min(last_block_height - first_block_height + 1, max_blocks));
  }

  uint64_t base = 0, last_placeholder_block = 0;
  bool has_placeholder = false;
  block_map::const_iterator i = blocks.begin();
  if (i != blocks.end() && is_blockchain_placeholder(*i))
  {
    base = i->start_block_height + i->nblocks;
    last_placeholder_block = base - 1;
    has_placeholder = true;
    ++i;
    for (block_map::const_iterator j = i; j != blocks.end(); ++j)
    {
      if (j->start_block_height < base)
        base = j->start_block_height;
    }
  }
  if (base > first_block_height)
    base = first_block_height;

  CHECK_AND_ASSERT_MES (base <= max_block_height + 1, std::make_pair(0, 0), "Blockchain placeholder larger than max block height");
  std::vector<uint8_t> bitmap(max_block_height + 1 - base, 0);
  MDEBUG("base " << base << ", last_placeholder_block " << (has_placeholder ? std::to_string(last_placeholder_block) : "none") << ", first_block_height " << first_block_height);
  if (has_placeholder && last_placeholder_block >= base)
    memset(bitmap.data(), 1, last_placeholder_block + 1 - base);
  while (i != blocks.end())
  {
    CHECK_AND_ASSERT_MES (i->start_block_height >= base, std::make_pair(0, 0), "Span starts before blochckain placeholder");
    memset(bitmap.data() + i->start_block_height - base, 1, i->nblocks);
    ++i;
  }

  const uint8_t *ptr = (const uint8_t*)memchr(bitmap.data() + first_block_height - base, 0, bitmap.size() - (first_block_height - base));
  if (!ptr)
  {
    MDEBUG("reserve_span: 0 not found in bitmap: " << first_block_height << " " << bitmap.size());
    print();
    return std::make_pair(0, 0);
  }
  uint64_t start_block_height = ptr - bitmap.data() + base;
  if (start_block_height > last_block_height)
  {
    MDEBUG("reserve_span: start_block_height > last_block_height: " << start_block_height << " < " << last_block_height);
    return std::make_pair(0, 0);
  }
  if (start_block_height + max_blocks - 1 < first_block_height)
  {
    MDEBUG("reserve_span: start_block_height + max_blocks - 1 < first_block_height: " << start_block_height << " + " << max_blocks << " - 1 < " << first_block_height);
    return std::make_pair(0, 0);
  }

  uint64_t nblocks = 1;
  while (start_block_height + nblocks <= last_block_height && nblocks < max_blocks && bitmap[start_block_height + nblocks - base] == 0)
    ++nblocks;

  MDEBUG("Reserving span " << start_block_height << " - " << (start_block_height + nblocks - 1) << " for " << connection_id);
  add_blocks(start_block_height, nblocks, connection_id, time);
  return std::make_pair(start_block_height, nblocks);
}

bool block_queue::is_blockchain_placeholder(const span &span) const
{
  static const boost::uuids::uuid uuid0 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  return span.connection_id == uuid0;
}

std::pair<uint64_t, uint64_t> block_queue::get_start_gap_span() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return std::make_pair(0, 0);
  block_map::const_iterator i = blocks.begin();
  if (!is_blockchain_placeholder(*i))
    return std::make_pair(0, 0);
  uint64_t current_height = i->start_block_height + i->nblocks - 1;
  ++i;
  if (i == blocks.end())
    return std::make_pair(0, 0);
  uint64_t first_span_height = i->start_block_height;
  if (first_span_height <= current_height + 1)
    return std::make_pair(0, 0);
  MDEBUG("Found gap at start of spans: last blockchain block height " << current_height << ", first span's block height " << first_span_height);
  print();
  return std::make_pair(current_height + 1, first_span_height - current_height - 1);
}

std::pair<uint64_t, uint64_t> block_queue::get_next_span_if_scheduled(std::list<crypto::hash> &hashes, boost::uuids::uuid &connection_id, boost::posix_time::ptime &time) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return std::make_pair(0, 0);
  block_map::const_iterator i = blocks.begin();
  if (is_blockchain_placeholder(*i))
    ++i;
  if (i == blocks.end())
    return std::make_pair(0, 0);
  if (!i->blocks.empty())
    return std::make_pair(0, 0);
  hashes = i->hashes;
  connection_id = i->connection_id;
  time = i->time;
  return std::make_pair(i->start_block_height, i->nblocks);
}

void block_queue::set_span_hashes(uint64_t start_height, const boost::uuids::uuid &connection_id, std::list<crypto::hash> hashes)
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  for (block_map::iterator i = blocks.begin(); i != blocks.end(); ++i)
  {
    if (i->start_block_height == start_height && i->connection_id == connection_id)
    {
      span s = *i;
      blocks.erase(i);
      s.hashes = std::move(hashes);
      blocks.insert(s);
      return;
    }
  }
}

bool block_queue::get_next_span(uint64_t &height, std::list<cryptonote::block_complete_entry> &bcel, boost::uuids::uuid &connection_id, bool filled) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  if (blocks.empty())
    return false;
  block_map::const_iterator i = blocks.begin();
  if (is_blockchain_placeholder(*i))
    ++i;
  for (; i != blocks.end(); ++i)
  {
    if (!filled || !i->blocks.empty())
    {
      height = i->start_block_height;
      bcel = i->blocks;
      connection_id = i->connection_id;
      return true;
    }
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

size_t block_queue::get_num_filled_spans_prefix() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);

  if (blocks.empty())
    return 0;
  block_map::const_iterator i = blocks.begin();
  if (is_blockchain_placeholder(*i))
    ++i;
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

crypto::hash block_queue::get_last_known_hash() const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  crypto::hash hash = cryptonote::null_hash;
  uint64_t highest_height = 0;
  for (const auto &span: blocks)
  {
    uint64_t h = span.start_block_height + span.nblocks - 1;
    if (h > highest_height && span.hashes.size() == span.nblocks)
    {
      hash = span.hashes.back();
      highest_height = h;
    }
  }
  return hash;
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
  for (auto i: speeds)
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

bool block_queue::foreach(std::function<bool(const span&)> f, bool include_blockchain_placeholder) const
{
  boost::unique_lock<boost::recursive_mutex> lock(mutex);
  block_map::const_iterator i = blocks.begin();
  if (!include_blockchain_placeholder && i != blocks.end() && is_blockchain_placeholder(*i))
    ++i;
  while (i != blocks.end())
    if (!f(*i++))
      return false;
  return true;
}

}
