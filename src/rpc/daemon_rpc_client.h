// Copyright (c) 2016, The Monero Project
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

#include "zmq_client.h"

#include "rpc/daemon_messages.h"

namespace cryptonote
{

namespace rpc
{

class DaemonRPCClient
{
  public:

    DaemonRPCClient();
    ~DaemonRPCClient();

    void connect(const std::string& addr, const std::string& port);

    bool getHeight(uint64_t& height);

    bool getBlocksFast(
        const std::list<crypto::hash>& block_ids,
        const uint64_t start_height_in,
        std::vector<cryptonote::rpc::block_with_transactions>& blocks,
        uint64_t& start_height_out,
        uint64_t& current_height);

    bool getHashesFast(
        const std::list<crypto::hash>& known_hashes,
        const uint64_t start_height_in,
        std::list<crypto::hash>& hashes,
        uint64_t& start_height_out,
        uint64_t& current_height);

    bool keyImagesSpent(
        const std::vector<crypto::key_image>& images,
        std::vector<bool>& spent,
        std::vector<bool>& spent_in_chain,
        std::vector<bool>& spent_in_pool,
        bool where = false);

    bool getTransactionPool(
        std::unordered_map<crypto::hash, cryptonote::rpc::tx_in_pool>& transactions,
        std::unordered_map<crypto::key_image, std::vector<crypto::hash> >& key_images);

    bool getTxGlobalOutputIndices(const crypto::hash& tx_hash, std::vector<uint64_t>& output_indices);

    bool getRandomOutputsForAmounts(
        const std::vector<uint64_t>& amounts,
        const uint64_t count,
        std::vector<amount_with_random_outputs>& amounts_with_outputs);

    bool sendRawTx(
        const cryptonote::transaction& tx,
        bool& relayed,
        std::string& error_details,
        bool relay = true);

    bool hardForkInfo(
        const uint8_t version,
        hard_fork_info& info);

    bool getOutputHistogram(
        const std::vector<uint64_t>& amounts,
        uint64_t min_count,
        uint64_t max_count,
        bool unlocked,
        std::vector<output_amount_count>& histogram);

    bool getOutputKeys(
        std::vector<output_amount_and_index>& outputs,
        std::vector<output_key_mask_unlocked>& keys);

    bool getRPCVersion(uint32_t& version);

  private:

    template <typename ReqType>
    rapidjson::Value doRequest(typename ReqType::Request& request);

    template <typename ReqType>
    typename ReqType::Response parseResponse(rapidjson::Value& resp);

    cryptonote::rpc::error parseError(rapidjson::Value& resp);

    ZmqClient zmq_client;
};

}  // namespace rpc

}  // namespace cryptonote
