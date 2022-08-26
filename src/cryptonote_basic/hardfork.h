// Copyright (c) 2014-2022, The Monero Project
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

#include "syncobj.h"
#include "hardforks/hardforks.h"
#include "cryptonote_basic/cryptonote_basic.h"

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

    static const uint64_t DEFAULT_ORIGINAL_VERSION_TILL_HEIGHT = 0; // <= actual height
    static const time_t DEFAULT_FORKED_TIME = 31557600; // a year in seconds
    static const time_t DEFAULT_UPDATE_TIME = 31557600 / 2;
    static const uint64_t DEFAULT_WINDOW_SIZE = 10080; // supermajority window check length - a week
    static const uint8_t DEFAULT_THRESHOLD_PERCENT = 80;

    /**
     * @brief creates a new HardFork object
     *
     * @param original_version the block version for blocks 0 through to the first fork
     * @param forked_time the time in seconds before thinking we're forked
     * @param update_time the time in seconds before thinking we need to update
     * @param window_size the size of the window in blocks to consider for version voting
     * @param default_threshold_percent the size of the majority in percents
     */
    HardFork(cryptonote::BlockchainDB &db, uint8_t original_version = 1, uint64_t original_version_till_height = DEFAULT_ORIGINAL_VERSION_TILL_HEIGHT, time_t forked_time = DEFAULT_FORKED_TIME, time_t update_time = DEFAULT_UPDATE_TIME, uint64_t window_size = DEFAULT_WINDOW_SIZE, uint8_t default_threshold_percent = DEFAULT_THRESHOLD_PERCENT);

    /**
     * @brief add a new hardfork height
     *
     * returns true if no error, false otherwise
     *
     * @param version the major block version for the fork
     * @param height The height the hardfork takes effect
     * @param threshold The threshold of votes needed for this fork (0-100)
     * @param time Approximate time of the hardfork (seconds since epoch)
     */
    bool add_fork(uint8_t version, uint64_t height, uint8_t threshold, time_t time);

    /**
     * @brief add a new hardfork height
     *
     * returns true if no error, false otherwise
     *
     * @param version the major block version for the fork
     * @param height The height the hardfork takes effect
     * @param time Approximate time of the hardfork (seconds since epoch)
     */
    bool add_fork(uint8_t version, uint64_t height, time_t time);

    /**
     * @brief initialize the object
     *
     * Must be done after adding all the required hardforks via add above
     */
    void init();

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
     * hardfork object to reorganize.
     */
    bool check(const cryptonote::block &block) const;

    /**
     * @brief same as check, but for a particular height, rather than the top
     *
     * NOTE: this does not play well with voting, and relies on voting to be
     * disabled (that is, forks happen on the scheduled date, whether or not
     * enough blocks have voted for the fork).
     *
     * returns true if no error, false otherwise
     *
     * @param block the new block
     * @param height which height to check for
     */
    bool check_for_height(const cryptonote::block &block, uint64_t height) const;

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
    bool reorganize_from_block_height(uint64_t height);
    bool reorganize_from_chain_height(uint64_t height);

    /**
     * @brief called when one or more blocks are popped from the blockchain
     *
     * The current fork will be updated by looking up the db,
     * which is much cheaper than recomputing everything
     *
     * @param new_chain_height the height of the chain after popping
     */
    void on_block_popped(uint64_t new_chain_height);

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
     * @brief returns the latest "ideal" version
     *
     * This is the latest version that's been scheduled
     */
    uint8_t get_ideal_version() const;

    /**
     * @brief returns the "ideal" version for a given height
     *
     * @param height height of the block to check
     */
    uint8_t get_ideal_version(uint64_t height) const;

    /**
     * @brief returns the next version
     *
     * This is the version which will we fork to next
     */
    uint8_t get_next_version() const;

    /**
     * @brief returns the current version
     *
     * This is the latest version that's past its trigger date and had enough votes
     * at one point in the past.
     */
    uint8_t get_current_version() const;

    /**
     * @brief returns the earliest block a given version may activate
     */
    uint64_t get_earliest_ideal_height_for_version(uint8_t version) const;

    /**
     * @brief returns information about current voting state
     *
     * returns true if the given version is enabled (ie, the current version
     * is at least the passed version), false otherwise
     *
     * @param version the version to check voting for
     * @param window the number of blocks considered in voting
     * @param votes number of votes for next version
     * @param threshold number of votes needed to switch to next version
     * @param earliest_height earliest height at which the version can take effect
     */
    bool get_voting_info(uint8_t version, uint32_t &window, uint32_t &votes, uint32_t &threshold, uint64_t &earliest_height, uint8_t &voting) const;

    /**
     * @brief returns the size of the voting window in blocks
     */
    uint64_t get_window_size() const { return window_size; }

    /**
     * @brief returns info for all known hard forks
     */
    const std::vector<hardfork_t>& get_hardforks() const { return heights; }

  private:

    uint8_t get_block_version(uint64_t height) const;
    bool do_check(uint8_t block_version, uint8_t voting_version) const;
    bool do_check_for_height(uint8_t block_version, uint8_t voting_version, uint64_t height) const;
    int get_voted_fork_index(uint64_t height) const;
    uint8_t get_effective_version(uint8_t voting_version) const;
    bool add(uint8_t block_version, uint8_t voting_version, uint64_t height);

    bool rescan_from_block_height(uint64_t height);
    bool rescan_from_chain_height(uint64_t height);

  private:

    BlockchainDB &db;

    time_t forked_time;
    time_t update_time;
    uint64_t window_size;
    uint8_t default_threshold_percent;

    uint8_t original_version;
    uint64_t original_version_till_height;

    std::vector<hardfork_t> heights;

    std::deque<uint8_t> versions; /* rolling window of the last N blocks' versions */
    unsigned int last_versions[256]; /* count of the block versions in the last N blocks */
    uint32_t current_fork_index;

    mutable epee::critical_section lock;
  };

}  // namespace cryptonote

