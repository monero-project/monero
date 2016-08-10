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

#include "message.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/message_data_structs.h"
#include "cryptonote_core/cryptonote_basic.h"

namespace cryptonote
{

namespace rpc
{

class GetHeight
{
  public:
    static const char* name;


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
    static const char* name;

    class Request : public Message
    {
      public:
        Request() { }
        ~Request() { }


        rapidjson::Value toJson(rapidjson::Document& doc);
        void fromJson(rapidjson::Value& val);

        std::vector<crypto::hash> block_ids;
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
    };
};

class GetHashesFast
{
  public:
    static const char* name;

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

        std::list<crypto::hash> m_block_ids;
        uint64_t start_height;
        uint64_t current_height;

    };
};

class GetTransactions
{
  public:
    static const char* name;

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
    static const char* name;

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

class GetTxGlobalOutputIndexes
{
  public:
    static const char* name;

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
    static const char* name;

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

        std::vector<outputs_for_amount> outputs;
    };
};

class SendRawTx
{
  public:
    static const char* name;

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

class StartMining
{
  public:
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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

// TODO: rename to GetHash?
class GetBlockHash
{
  public:
    static const char* name;

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

class GetBlockTemplate
{
  public:
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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

class GetBlockHeaderByHash
{
  public:
    static const char* name;

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

class GetBlockHeaderByHeight
{
  public:
    static const char* name;

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

class GetBlock
{
  public:
    static const char* name;

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
    static const char* name;

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

class SetLogHashRate
{
  public:
    static const char* name;

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
    static const char* name;

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

class GetTransactionPool
{
  public:
    static const char* name;

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

class GetConnections
{
  public:
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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

class GetBans
{
  public:
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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
    static const char* name;

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

}  // namespace rpc

}  // namespace cryptonote
