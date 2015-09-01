// Copyright (c) 2015, The Monero Project
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

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include "syncobj.h"
#include "cryptonote_core/cryptonote_basic.h"

namespace cryptonote
{
  class BlockchainDB;

  class HardFork
  {
  public:
    typedef enum {
      LikelyForked,
      UpdateNeeded,
      Ready,
    } State;

    static const time_t DEFAULT_FORKED_TIME = 31557600; // a year in seconds
    static const time_t DEFAULT_UPDATE_TIME = 31557600 / 2;
    static const uint64_t DEFAULT_MAX_HISTORY = 50; // supermajority window check length
    static const int DEFAULT_THRESHOLD_PERCENT = 80;
    static const uint64_t DEFAULT_CHECKPOINT_PERIOD = 1024; // mark a checkpoint every that many blocks

    /**
     * @brief creates a new HardFork object
     *
     * @param original_version the block version for blocks 0 through to the first fork
     * @param forked_time the time in seconds before thinking we're forked
     * @param update_time the time in seconds before thinking we need to update
     * @param max_history the size of the window in blocks to consider for version voting
     * @param threshold_percent the size of the majority in percents
     */
    HardFork(uint8_t original_version = 1, time_t forked_time = DEFAULT_FORKED_TIME, time_t update_time = DEFAULT_UPDATE_TIME, uint64_t max_history = DEFAULT_MAX_HISTORY, int threshold_percent = DEFAULT_THRESHOLD_PERCENT, uint64_t checkpoint_period = DEFAULT_CHECKPOINT_PERIOD);

    /**
     * @brief add a new hardfork height
     *
     * returns true if no error, false otherwise
     *
     * @param version the major block version for the fork
     * @param height The height the hardfork takes effect
     * @param time Approximate time of the hardfork (seconds since epoch)
     */
    bool add(uint8_t version, uint64_t height, time_t time);

    /**
     * @brief check whether a new block would be accepted
     *
     * returns true if the block is accepted, false otherwise
     *
     * @param block the new block
     *
     * This check is made by add. It is exposed publicly to allow
     * the caller to inexpensively check whether a block would be
     * accepted or rejected by its version number. Indeed, if this
     * check could only be done as part of add, the caller would
     * either have to add the block to the blockchain first, then
     * call add, then have to pop the block from the blockchain if
     * its version did not satisfy the hard fork requirements, or
     * call add first, then, if the hard fork requirements are met,
     * add the block to the blockchain, upon which a failure (the
     * block being invalid, double spending, etc) would cause the
     * hardfork object to rescan the blockchain versions past the
     * last checkpoint, potentially causing a large number of DB
     * operations.
     */
    bool check(const cryptonote::block &block) const;

    /**
     * @brief add a new block
     *
     * returns true if no error, false otherwise
     *
     * @param block the new block
     */
    bool add(const cryptonote::block &block, uint64_t height);

    /**
     * @brief called when the blockchain is reorganized
     *
     * This will rescan the blockchain to determine which hard forks
     * have been triggered
     *
     * returns true if no error, false otherwise
     *
     * @param blockchain the blockchain
     * @param height of the last block kept from the previous blockchain
     */
    bool reorganize_from_block_height(const cryptonote::BlockchainDB *db, uint64_t height);
    bool reorganize_from_chain_height(const cryptonote::BlockchainDB *db, uint64_t height);

    /**
     * @brief returns current state at the given time
     *
     * Based on the approximate time of the last known hard fork,
     * estimate whether we need to update, or if we're way behind
     *
     * @param t the time to consider
     */
    State get_state(time_t t) const;
    State get_state() const;

    /**
     * @brief returns the hard fork version for the given block height
     *
     * @param height height of the block to check
     */
    uint8_t get(uint64_t height) const;

    /**
     * @brief returns the height of the first block on the fork with th given version
     *
     * @param version version of the fork to query the starting block for
     */
    uint64_t get_start_height(uint8_t version) const;

    /**
     * @brief returns the latest "ideal" version
     *
     * This is the latest version that's been scheduled
     */
    uint8_t get_ideal_version() const;

    /**
     * @brief returns the current version
     *
     * This is the latest version that's past its trigger date and had enough votes
     * at one point in the past.
     */
    uint8_t get_current_version() const;

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);

  private:

    void init();
    bool do_check(const cryptonote::block &block) const;
    int get_voted_fork_index(uint64_t height) const;
    uint8_t get_effective_version(const cryptonote::block &block) const;

  private:

    time_t forked_time;
    time_t update_time;
    uint64_t max_history;
    int threshold_percent;

    uint8_t original_version;

    typedef struct {
      uint8_t version;
      uint64_t height;
      time_t time;
    } Params;
    std::vector<Params> heights;

    std::deque<uint8_t> versions; /* rolling window of the last N blocks' versions */
    unsigned int last_versions[256]; /* count of the block versions in the lsat N blocks */
    uint64_t starting[256]; /* block height at which each fork starts */
    unsigned int current_fork_index;
    unsigned int vote_threshold;

    uint64_t checkpoint_period;
    std::vector<std::pair<uint64_t, int>> checkpoints;

    mutable epee::critical_section lock;
  };

}  // namespace cryptonote

BOOST_CLASS_VERSION(cryptonote::HardFork, 1)
