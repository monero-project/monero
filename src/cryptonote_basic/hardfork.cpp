// Copyright (c) 2014-2026, The Monero Project
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

#include <ctime>

#include "cryptonote_basic/cryptonote_basic.h"
#include "hardfork.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "hardfork"

using namespace cryptonote;

static uint8_t get_block_vote(const cryptonote::block &b)
{
  // Pre-hardfork blocks have a minor version hardcoded to 0.
  // For the purposes of voting, we consider 0 to refer to
  // version number 1, which is what all blocks from the genesis
  // block are. It makes things simpler.
  if (b.minor_version == 0)
    return 1;
  return b.minor_version;
}

static uint8_t get_block_version(const cryptonote::block &b)
{
  return b.major_version;
}

HardFork::HardFork(uint8_t original_version, uint64_t original_version_till_height, time_t forked_time, time_t update_time):
  forked_time(forked_time),
  update_time(update_time),
  original_version(original_version),
  original_version_till_height(original_version_till_height)
{
}

bool HardFork::add_fork(uint8_t version, uint64_t height, uint8_t threshold, time_t time)
{
  CRITICAL_REGION_LOCAL(lock);

  // add in order
  if (version == 0)
    return false;
  if (!heights.empty()) {
    if (version <= heights.back().version)
      return false;
    if (height <= heights.back().height)
      return false;
    if (time <= heights.back().time)
      return false;
  }
  if (threshold > 100)
    return false;
  heights.push_back(hardfork_t(version, height, threshold, time));
  return true;
}

bool HardFork::add_fork(uint8_t version, uint64_t height, time_t time)
{
  return add_fork(version, height, /*threshold=*/0, time);
}

bool HardFork::do_check_for_height(uint8_t block_version, uint8_t voting_version, uint64_t height) const
{
  const uint8_t checking_version = get_ideal_version(height);
  return block_version == checking_version && voting_version >= checking_version;
}

bool HardFork::check_for_height(const cryptonote::block &block, uint64_t height) const
{
  CRITICAL_REGION_LOCAL(lock);
  return do_check_for_height(::get_block_version(block), ::get_block_vote(block), height);
}

void HardFork::init()
{
  CRITICAL_REGION_LOCAL(lock);

  // add a placeholder for the default version, to avoid special cases
  if (heights.empty())
    heights.push_back(hardfork_t(original_version, 0, 0, 0));

  MDEBUG("init done");
}

HardFork::State HardFork::get_state(time_t t) const
{
  CRITICAL_REGION_LOCAL(lock);

  // no hard forks setup yet
  if (heights.size() <= 1)
    return Ready;

  time_t t_last_fork = heights.back().time;
  if (t >= t_last_fork + forked_time)
    return LikelyForked;
  if (t >= t_last_fork + update_time)
    return UpdateNeeded;
  return Ready;
}

HardFork::State HardFork::get_state() const
{
  return get_state(time(NULL));
}

uint8_t HardFork::get_ideal_version() const
{
  CRITICAL_REGION_LOCAL(lock);
  return heights.back().version;
}

uint8_t HardFork::get_ideal_version(uint64_t height) const
{
  CRITICAL_REGION_LOCAL(lock);
  for (unsigned int n = heights.size() - 1; n > 0; --n) {
    if (height >= heights[n].height) {
      return heights[n].version;
    }
  }
  return original_version;
}

uint64_t HardFork::get_earliest_ideal_height_for_version(uint8_t version) const
{
  uint64_t height = std::numeric_limits<uint64_t>::max();
  for (auto i = heights.rbegin(); i != heights.rend(); ++i) {
    if (i->version >= version) {
      height = i->height;
    } else {
      break;
    }
  }
  return height;
}
