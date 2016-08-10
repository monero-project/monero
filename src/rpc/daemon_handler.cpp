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

#include "daemon_handler.h"

namespace cryptonote
{

namespace rpc
{

  void DaemonHandler::handle(GetHeight::Request& req, GetHeight::Response& res)
  {
    res.height = m_core.get_current_blockchain_height();
  }

  void DaemonHandler::handle(GetBlocksFast::Request& req, GetBlocksFast::Response& res)
  {
    std::list<std::pair<block, std::list<transaction> > > blocks;

    if(!m_core.find_blockchain_supplement(req.start_height, req.block_ids, blocks, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "core::find_blockchain_supplement() returned false";
      return;
    }

    res.blocks.resize(blocks.size());

    //TODO: really need to switch uses of std::list to std::vector unless
    //      it's a huge performance concern

    auto it = blocks.begin();

    uint64_t block_count = 0;
    while (it != blocks.end())
    {
      cryptonote::rpc::block_with_transactions& bwt = res.blocks[block_count];

      block& blk = it->first;
      bwt.block = blk;

      std::list<transaction>& txs = it->second;

      // assume each block returned is returned with all its transactions
      // in the correct order.
      auto tx_it = txs.begin();
      for (crypto::hash& h : blk.tx_hashes)
      {
        bwt.transactions.emplace(h, *tx_it);
        tx_it++;
      }

      it++;
      block_count++;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(GetHashesFast::Request& req, GetHashesFast::Response& res)
  {
    res.start_height = req.start_height;

    auto& chain = m_core.get_blockchain_storage();

    if (!chain.find_blockchain_supplement(req.known_hashes, res.hashes, res.start_height, res.current_height))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Blockchain::find_blockchain_supplement() returned false";
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(GetTransactions::Request& req, GetTransactions::Response& res)
  {
    std::list<cryptonote::transaction> found_txs;
    std::list<crypto::hash> missed_hashes;

    bool r = m_core.get_transactions(req.tx_hashes, found_txs, missed_hashes);

    // TODO: handle return false (std::exception caught above)
    if (!r)
    {
    }

    uint64_t num_found = found_txs.size();

    // std::list is annoying
    std::vector<cryptonote::transaction> found_txs_vec
    {
      std::make_move_iterator(std::begin(found_txs)),
      std::make_move_iterator(std::end(found_txs))
    };

    std::vector<crypto::hash> missed_vec
    {
      std::make_move_iterator(std::begin(missed_hashes)),
      std::make_move_iterator(std::end(missed_hashes))
    };

    std::vector<uint64_t> heights(num_found);
    std::vector<bool> in_pool(num_found, false);
    std::vector<crypto::hash> found_hashes(num_found);

    for (uint32_t i=0; i < num_found; i++)
    {
      found_hashes[i] = get_transaction_hash(found_txs_vec[i]);
      heights[i] = m_core.get_blockchain_storage().get_db().get_tx_block_height(found_hashes[i]);
    }

    // if any missing from blockchain, check in tx pool
    if (!missed_vec.empty())
    {
      std::list<cryptonote::transaction> pool_txs;

      m_core.get_pool_transactions(pool_txs);

      for (auto& tx : pool_txs)
      {
        crypto::hash h = get_transaction_hash(tx);

        auto itr = std::find(missed_vec.begin(), missed_vec.end(), h);

        if (itr != missed_vec.begin())
        {
          found_hashes.push_back(h);
          found_txs_vec.push_back(tx);
          heights.push_back(std::numeric_limits<uint64_t>::max());
          in_pool.push_back(true);
          missed_vec.erase(itr);
        }
      }
    }

    for (uint32_t i=0; i < found_hashes.size(); i++)
    {
      cryptonote::rpc::transaction_info info;
      info.height = heights[i];
      info.in_pool = in_pool[i];
      info.transaction = std::move(found_txs_vec[i]);

      res.txs.emplace(found_hashes[i], std::move(info));
    }
                                      
    res.missed_hashes = std::move(missed_vec);
  }

  void DaemonHandler::handle(KeyImagesSpent::Request& req, KeyImagesSpent::Response& res)
  {
    res.spent_status.resize(req.key_images.size(), KeyImagesSpent::STATUS::UNSPENT);

    std::vector<bool> chain_spent_status;
    std::vector<bool> pool_spent_status;

    m_core.are_key_images_spent(req.key_images, chain_spent_status);
    m_core.get_pool().have_key_images_as_spent(req.key_images, pool_spent_status);

    for(uint64_t i=0; i < req.key_images.size(); i++)
    {
      if ( chain_spent_status[i] )
      {
        res.spent_status[i] = KeyImagesSpent::STATUS::SPENT_IN_BLOCKCHAIN;
      }
      else if ( pool_spent_status[i] )
      {
        res.spent_status[i] = KeyImagesSpent::STATUS::SPENT_IN_POOL;
      }
    }
  }

  void DaemonHandler::handle(GetTxGlobalOutputIndexes::Request& req, GetTxGlobalOutputIndexes::Response& res)
  {
  }

  void DaemonHandler::handle(GetRandomOutputsForAmounts::Request& req, GetRandomOutputsForAmounts::Response& res)
  {
  }

  void DaemonHandler::handle(SendRawTx::Request& req, SendRawTx::Response& res)
  {
  }

  void DaemonHandler::handle(StartMining::Request& req, StartMining::Response& res)
  {
  }

  void DaemonHandler::handle(GetInfo::Request& req, GetInfo::Response& res)
  {
    res.height = m_core.get_current_blockchain_height();

    res.target_height = m_core.get_target_blockchain_height();

    auto& chain = m_core.get_blockchain_storage();

    res.difficulty = chain.get_difficulty_for_next_block();

    res.target = chain.get_difficulty_target();

    res.tx_count = chain.get_total_transactions() - res.height; //without coinbase

    res.tx_pool_size = m_core.get_pool_transactions_count();

    res.alt_blocks_count = chain.get_alternative_blocks_count();

    uint64_t total_conn = m_p2p.get_connections_count();
    res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
    res.incoming_connections_count = total_conn - res.outgoing_connections_count;

    res.white_peerlist_size = m_p2p.get_peerlist_manager().get_white_peers_count();

    res.grey_peerlist_size = m_p2p.get_peerlist_manager().get_gray_peers_count();

    res.testnet = m_core.is_testnet();
  }

  void DaemonHandler::handle(StopMining::Request& req, StopMining::Response& res)
  {
  }

  void DaemonHandler::handle(MiningStatus::Request& req, MiningStatus::Response& res)
  {
  }

  void DaemonHandler::handle(SaveBC::Request& req, SaveBC::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlockHash::Request& req, GetBlockHash::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlockTemplate::Request& req, GetBlockTemplate::Response& res)
  {
  }

  void DaemonHandler::handle(SubmitBlock::Request& req, SubmitBlock::Response& res)
  {
  }

  void DaemonHandler::handle(GetLastBlockHeader::Request& req, GetLastBlockHeader::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlockHeaderByHash::Request& req, GetBlockHeaderByHash::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlockHeaderByHeight::Request& req, GetBlockHeaderByHeight::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlock::Request& req, GetBlock::Response& res)
  {
  }

  void DaemonHandler::handle(GetPeerList::Request& req, GetPeerList::Response& res)
  {
  }

  void DaemonHandler::handle(SetLogHashRate::Request& req, SetLogHashRate::Response& res)
  {
  }

  void DaemonHandler::handle(SetLogLevel::Request& req, SetLogLevel::Response& res)
  {
  }

  void DaemonHandler::handle(GetTransactionPool::Request& req, GetTransactionPool::Response& res)
  {
  }

  void DaemonHandler::handle(GetConnections::Request& req, GetConnections::Response& res)
  {
  }

  void DaemonHandler::handle(GetBlockHeadersRange::Request& req, GetBlockHeadersRange::Response& res)
  {
  }

  void DaemonHandler::handle(StopDaemon::Request& req, StopDaemon::Response& res)
  {
  }

  void DaemonHandler::handle(FastExit::Request& req, FastExit::Response& res)
  {
  }

  void DaemonHandler::handle(OutPeers::Request& req, OutPeers::Response& res)
  {
  }

  void DaemonHandler::handle(StartSaveGraph::Request& req, StartSaveGraph::Response& res)
  {
  }

  void DaemonHandler::handle(StopSaveGraph::Request& req, StopSaveGraph::Response& res)
  {
  }

  void DaemonHandler::handle(HardForkInfo::Request& req, HardForkInfo::Response& res)
  {
  }

  void DaemonHandler::handle(GetBans::Request& req, GetBans::Response& res)
  {
  }

  void DaemonHandler::handle(SetBans::Request& req, SetBans::Response& res)
  {
  }

  void DaemonHandler::handle(FlushTransactionPool::Request& req, FlushTransactionPool::Response& res)
  {
  }

  void DaemonHandler::handle(GetOutputHistogram::Request& req, GetOutputHistogram::Response& res)
  {
  }

  std::string DaemonHandler::handle(std::string& request)
  {
    FullMessage req_full(request);

    rapidjson::Value& req_json = req_full.getMessage();

    std::string request_type = req_full.getRequestType();

    Message* resp_message = NULL;

    // create correct Message subclass and call handle() on it
    REQ_RESP_TYPES_MACRO(request_type, GetHeight, req_json, resp_message, handle);
    REQ_RESP_TYPES_MACRO(request_type, GetBlocksFast, req_json, resp_message, handle);
    REQ_RESP_TYPES_MACRO(request_type, GetHashesFast, req_json, resp_message, handle);
    REQ_RESP_TYPES_MACRO(request_type, GetTransactions, req_json, resp_message, handle);
    REQ_RESP_TYPES_MACRO(request_type, KeyImagesSpent, req_json, resp_message, handle);
    REQ_RESP_TYPES_MACRO(request_type, GetInfo, req_json, resp_message, handle);

    FullMessage resp_full(req_full.getVersion(), request_type, resp_message);

    std::string response = resp_full.getJson();
    delete resp_message;

    std::cout << "DaemonHandler::handle() response: " << response << std::endl;

    return response;
  }

}  // namespace rpc

}  // namespace cryptonote
