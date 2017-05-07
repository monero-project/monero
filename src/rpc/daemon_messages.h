// Copyright (c) 2016-2017, The Monero Project
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

#include "message.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/message_data_structs.h"
#include "rpc/daemon_rpc_version.h"
#include "cryptonote_basic/cryptonote_basic.h"

namespace cryptonote
{

namespace rpc
{

class GetHeight
{
  public:
    static const char* const name;


    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t height;
    };
};

class GetBlocksFast
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::list<crypto::hash> block_ids;
        uint64_t start_height;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<cryptonote::rpc::block_with_transactions> blocks;
        uint64_t start_height;
        uint64_t current_height;
        std::vector<cryptonote::rpc::block_output_indices> output_indices;
    };
};

class GetHashesFast
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::list<crypto::hash> known_hashes;
        uint64_t start_height;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::list<crypto::hash> hashes;
        uint64_t start_height;
        uint64_t current_height;

    };
};

class GetTransactions
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<crypto::hash> tx_hashes;

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        std::unordered_map<crypto::hash, cryptonote::rpc::transaction_info> txs;

        std::vector<crypto::hash> missed_hashes;

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class KeyImagesSpent
{
  public:
    static const char* const name;

    enum STATUS {
      UNSPENT = 0,
      SPENT_IN_BLOCKCHAIN = 1,
      SPENT_IN_POOL = 2,
    };

    class Request : public Message
    {
      public:

        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<crypto::key_image> key_images;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<uint64_t> spent_status;
    };
};

class GetTxGlobalOutputIndices
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        crypto::hash tx_hash;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<uint64_t> output_indices;
    };
};

class GetRandomOutputsForAmounts
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<uint64_t> amounts;
        uint64_t count;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<amount_with_random_outputs> amounts_with_outputs;
    };
};

class SendRawTx
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        cryptonote::transaction tx;
        bool relay;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        bool relayed;
    };
};

class StartMining
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::string miner_address;
        uint64_t thread_count;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        bool success;
    };
};

class GetInfo
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t height;
        uint64_t target_height;
        uint64_t difficulty;
        uint64_t target;
        uint64_t tx_count;
        uint64_t tx_pool_size;
        uint64_t alt_blocks_count;
        uint64_t outgoing_connections_count;
        uint64_t incoming_connections_count;
        uint64_t white_peerlist_size;
        uint64_t grey_peerlist_size;
        bool testnet;
        crypto::hash top_block_hash;
    };
};

class StopMining
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        bool success;
    };
};

class MiningStatus
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        bool active;
        uint64_t speed;
        uint64_t thread_count;
        std::string address;
    };
};

class SaveBC
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class GetBlockHash
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t height;

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        crypto::hash hash;
    };
};

class GetBlockTemplate
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class SubmitBlock
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class GetLastBlockHeader
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        cryptonote::rpc::BlockHeaderResponse header;
    };
};

class GetBlockHeaderByHash
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        crypto::hash hash;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        cryptonote::rpc::BlockHeaderResponse header;
    };
};

class GetBlockHeaderByHeight
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t height;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        cryptonote::rpc::BlockHeaderResponse header;
    };
};

class GetBlock
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class GetPeerList
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<peer> white_list;
        std::vector<peer> gray_list;
    };
};

class SetLogHashRate
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class SetLogLevel
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        int8_t level;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);
    };
};

class GetTransactionPool
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::unordered_map<crypto::hash, cryptonote::rpc::tx_in_pool> transactions;
        std::unordered_map<crypto::key_image, std::vector<crypto::hash> > key_images;
    };
};

class GetConnections
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class GetBlockHeadersRange
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class StopDaemon
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class FastExit
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class OutPeers
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class StartSaveGraph
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class StopSaveGraph
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class HardForkInfo
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint8_t version;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        hard_fork_info info;
    };
};

class GetBans
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class SetBans
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class FlushTransactionPool
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };
};

class GetOutputHistogram
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<uint64_t> amounts;
        uint64_t min_count;
        uint64_t max_count;
        bool unlocked;
        uint64_t recent_cutoff;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<output_amount_count> histogram;
    };
};

class GetOutputKeys
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<output_amount_and_index> outputs;

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<output_key_mask_unlocked> keys;
    };
};

class GetRPCVersion
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint32_t version;
    };
};

class GetPerKBFeeEstimate
{
  public:
    static const char* const name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t num_grace_blocks;
    };

    class Response : public Message
    {
      public:
        Response() { }
        ~Response() { }

        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        uint64_t estimated_fee_per_kb;
    };
};

}  // namespace rpc

}  // namespace cryptonote
