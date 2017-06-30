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

#pragma once

#include <atomic>

#include <boost/optional/optional.hpp>
#include <boost/thread/locks.hpp>

#include "rpc/daemon_rpc_client.h"

#include "rpc/message_data_structs.h"
#include "net/http_client.h"

namespace cryptonote
{

namespace rpc
{


/**
 * @brief Daemon RPC Client interface
 *
 * This class serves to bridge the old RPC with the new ZMQ RPC such that
 * one can command a wallet to use either to communicate with a daemon.
 */
class DaemonRPCClientOld : public DaemonRPCClient
{
  public:

    DaemonRPCClientOld() = delete;
    ~DaemonRPCClientOld() { }

    DaemonRPCClientOld(const std::string& address, boost::optional<epee::net_utils::http::login> loginInfo);

    boost::optional<std::string> checkConnection(
        uint32_t timeout,
        uint32_t& version);

    boost::optional<std::string> getHeight(
        uint64_t& height);

    boost::optional<std::string> getTargetHeight(
        uint64_t& target_height);

    boost::optional<std::string> getNetworkDifficulty(
        uint64_t& difficulty);

    boost::optional<std::string> getDifficultyTarget(
        uint64_t& target);

    boost::optional<std::string> getBlocksFast(
        const std::list<crypto::hash>& short_chain_history,
        const uint64_t start_height_in,
        const bool prune,
        std::vector<cryptonote::rpc::block_with_transactions>& blocks,
        uint64_t& start_height_out,
        uint64_t& current_height,
        std::vector<cryptonote::rpc::block_output_indices>& output_indices);

    boost::optional<std::string> getHashesFast(
        const std::list<crypto::hash>& short_chain_history,
        const uint64_t start_height_in,
        std::list<crypto::hash>& hashes,
        uint64_t& start_height_out,
        uint64_t& current_height);

    boost::optional<std::string> getTransactions(
        const std::vector<crypto::hash>& tx_hashes,
        std::unordered_map<crypto::hash, cryptonote::rpc::transaction_info> txs,
        std::vector<crypto::hash> missed_hashes);

    boost::optional<std::string> getBlockHeadersByHeight(
        const std::vector<uint64_t>& heights,
        std::vector<cryptonote::rpc::BlockHeaderResponse> headers);

    boost::optional<std::string> keyImagesSpent(
        const std::vector<crypto::key_image>& images,
        std::vector<bool>& spent,
        std::vector<bool>& spent_in_chain,
        std::vector<bool>& spent_in_pool);

    boost::optional<std::string> getTransactionPool(
        std::unordered_map<crypto::hash, cryptonote::rpc::tx_in_pool>& transactions,
        std::unordered_map<crypto::key_image, std::vector<crypto::hash> >& key_images);

    boost::optional<std::string> getRandomOutputsForAmounts(
        const std::vector<uint64_t>& amounts,
        const uint64_t count,
        std::vector<amount_with_random_outputs>& amounts_with_outputs);

    boost::optional<std::string> sendRawTx(
        const cryptonote::transaction& tx,
        bool& relayed,
        bool relay = true);

    boost::optional<std::string> hardForkInfo(
        const uint8_t version,
        hard_fork_info& info);

    boost::optional<std::string> getHardForkEarliestHeight(
        const uint8_t version,
        uint64_t& earliest_height);

    boost::optional<std::string> getOutputHistogram(
        const std::vector<uint64_t>& amounts,
        uint64_t min_count,
        uint64_t max_count,
        bool unlocked,
        uint64_t recent_cutoff,
        std::vector<output_amount_count>& histogram);

    boost::optional<std::string> getOutputKeys(
        const std::vector<output_amount_and_index>& outputs,
        std::vector<output_key_mask_unlocked>& keys);

    boost::optional<std::string> getRPCVersion(
        uint32_t& version);

    boost::optional<std::string> getPerKBFeeEstimate(
        const uint64_t num_grace_blocks,
        uint64_t& estimated_per_kb_fee);

    boost::optional<std::string> getMiningHashRate(
        uint64_t& speed);

    boost::optional<std::string> isMining(
        bool& status);

    boost::optional<std::string> startMining(
        const std::string& miner_address,
        const uint64_t threads_count,
        const bool do_background_mining,
        const bool ignore_battery);

    boost::optional<std::string> stopMining();

    uint32_t getOurRPCVersion();

  private:

    boost::optional<std::string> connect();
    boost::optional<std::string> disconnect();

    std::string daemonAddress;
    boost::optional<epee::net_utils::http::login> loginInfo;

    void resetCachedValues();

    epee::net_utils::http::http_simple_client httpClient;
    
    boost::mutex inUseMutex;

    time_t lastHeightCheckTime;
    uint64_t cachedHeight;

    uint32_t cachedRPCVersion;

    uint64_t cachedHardForkEarliestHeights[256];

    uint64_t cachedPerKBFeeEstimate;
    uint64_t cachedPerKBFeeEstimateHeight;
    uint64_t cachedPerKBFeeEstimateGraceBlocks;
};


} // namespace rpc
} // namespace cryptonote

