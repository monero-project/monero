// Copyright (c) 2016-2018, The Monero Project
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

#include <unordered_map>
#include <vector>

#include "message.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/message_data_structs.h"
#include "rpc/daemon_rpc_version.h"
#include "cryptonote_basic/cryptonote_basic.h"

#define BEGIN_RPC_MESSAGE_CLASS(classname) \
class classname \
{ \
  public: \
    static const char* const name;

#define BEGIN_RPC_MESSAGE_REQUEST \
    class Request : public Message \
    { \
      public: \
        Request() { *this = {}; } \
        ~Request() { } \
        rapidjson::Value toJson(rapidjson::Document& doc) const; \
        void fromJson(rapidjson::Value& val);

#define BEGIN_RPC_MESSAGE_RESPONSE \
    class Response : public Message \
    { \
      public: \
        Response() { *this = {}; } \
        ~Response() { } \
        rapidjson::Value toJson(rapidjson::Document& doc) const; \
        void fromJson(rapidjson::Value& val);

#define END_RPC_MESSAGE_REQUEST };
#define END_RPC_MESSAGE_RESPONSE };
#define END_RPC_MESSAGE_CLASS };

namespace cryptonote
{

namespace rpc
{

BEGIN_RPC_MESSAGE_CLASS(GetHeight);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    uint64_t height;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(GetBlocksFast);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::list<crypto::hash> block_ids;
    uint64_t start_height;
    bool prune;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<cryptonote::rpc::block_with_transactions> blocks;
    uint64_t start_height;
    uint64_t current_height;
    std::vector<cryptonote::rpc::block_output_indices> output_indices;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(GetHashesFast);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::list<crypto::hash> known_hashes;
    uint64_t start_height;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<crypto::hash> hashes;
    uint64_t start_height;
    uint64_t current_height;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(GetTransactions);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<crypto::hash> tx_hashes;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    using txes_map = std::unordered_map<crypto::hash, transaction_info>;
    txes_map txs;
    std::vector<crypto::hash> missed_hashes;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(KeyImagesSpent);
  enum STATUS {
    UNSPENT = 0,
    SPENT_IN_BLOCKCHAIN = 1,
    SPENT_IN_POOL = 2,
  };
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<crypto::key_image> key_images;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<uint64_t> spent_status;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(GetTxGlobalOutputIndices);
  BEGIN_RPC_MESSAGE_REQUEST;
    crypto::hash tx_hash;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<uint64_t> output_indices;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;


BEGIN_RPC_MESSAGE_CLASS(GetRandomOutputsForAmounts);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<uint64_t> amounts;
    uint64_t count;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<amount_with_random_outputs> amounts_with_outputs;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SendRawTx);
  BEGIN_RPC_MESSAGE_REQUEST;
    cryptonote::transaction tx;
    bool relay;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    bool relayed;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(StartMining);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::string miner_address;
    uint64_t threads_count;
    bool do_background_mining;
    bool ignore_battery;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetInfo);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    DaemonInfo info;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(StopMining);
    BEGIN_RPC_MESSAGE_REQUEST;
    END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(MiningStatus);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    bool active;
    uint64_t speed;
    uint64_t threads_count;
    std::string address;
    bool is_background_mining_enabled;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SaveBC);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockHash);
  BEGIN_RPC_MESSAGE_REQUEST;
    uint64_t height;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    crypto::hash hash;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockTemplate);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SubmitBlock);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetLastBlockHeader);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    cryptonote::rpc::BlockHeaderResponse header;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockHeaderByHash);
  BEGIN_RPC_MESSAGE_REQUEST;
    crypto::hash hash;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    cryptonote::rpc::BlockHeaderResponse header;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockHeaderByHeight);
  BEGIN_RPC_MESSAGE_REQUEST;
    uint64_t height;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    cryptonote::rpc::BlockHeaderResponse header;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockHeadersByHeight);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<uint64_t> heights;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<cryptonote::rpc::BlockHeaderResponse> headers;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlock);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetPeerList);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<peer> white_list;
    std::vector<peer> gray_list;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SetLogHashRate);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SetLogLevel);
  BEGIN_RPC_MESSAGE_REQUEST;
    int8_t level;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetTransactionPool);
    BEGIN_RPC_MESSAGE_REQUEST;
    END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<cryptonote::rpc::tx_in_pool> transactions;
    key_images_with_tx_hashes key_images;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetConnections);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBlockHeadersRange);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(StopDaemon);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(StartSaveGraph);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(StopSaveGraph);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(HardForkInfo);
  BEGIN_RPC_MESSAGE_REQUEST;
    uint8_t version;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    hard_fork_info info;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetBans);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(SetBans);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(FlushTransactionPool);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetOutputHistogram);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<uint64_t> amounts;
    uint64_t min_count;
    uint64_t max_count;
    bool unlocked;
    uint64_t recent_cutoff;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<output_amount_count> histogram;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetOutputKeys);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<output_amount_and_index> outputs;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<output_key_mask_unlocked> keys;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetRPCVersion);
  BEGIN_RPC_MESSAGE_REQUEST;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    uint32_t version;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetFeeEstimate);
  BEGIN_RPC_MESSAGE_REQUEST;
    uint64_t num_grace_blocks;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    uint64_t estimated_base_fee;
    uint64_t fee_mask;
    uint32_t size_scale;
    uint8_t hard_fork_version;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

BEGIN_RPC_MESSAGE_CLASS(GetOutputDistribution);
  BEGIN_RPC_MESSAGE_REQUEST;
    std::vector<uint64_t> amounts;
    uint64_t from_height;
    uint64_t to_height;
    bool cumulative;
  END_RPC_MESSAGE_REQUEST;
  BEGIN_RPC_MESSAGE_RESPONSE;
    std::vector<output_distribution> distributions;
  END_RPC_MESSAGE_RESPONSE;
END_RPC_MESSAGE_CLASS;

}  // namespace rpc

}  // namespace cryptonote
