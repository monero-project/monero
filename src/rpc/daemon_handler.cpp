// Copyright (c) 2017-2023, The Monero Project
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

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <boost/uuid/nil_generator.hpp>
#include <boost/utility/string_ref.hpp>
// likely included by daemon_handler.h's includes,
// but including here for clarity
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/blobdatatype.h"
#include "ringct/rctSigs.h"
#include "version.h"

namespace cryptonote
{

namespace rpc
{
  namespace
  {
    using handler_function = epee::byte_slice(DaemonHandler& handler, const rapidjson::Value& id, const rapidjson::Value& msg);
    struct handler_map
    {
      const char* method_name;
      handler_function* call;
    };

    bool operator<(const handler_map& lhs, const handler_map& rhs) noexcept
    {
      return std::strcmp(lhs.method_name, rhs.method_name) < 0;
    }

    bool operator<(const handler_map& lhs, const std::string& rhs) noexcept
    {
      return std::strcmp(lhs.method_name, rhs.c_str()) < 0;
    }

    template<typename Message>
    epee::byte_slice handle_message(DaemonHandler& handler, const rapidjson::Value& id, const rapidjson::Value& parameters)
    {
      typename Message::Request request{};
      request.fromJson(parameters);

      typename Message::Response response{};
      handler.handle(request, response);
      return FullMessage::getResponse(response, id);
    }

    constexpr const handler_map handlers[] =
    {
      {u8"get_block_hash", handle_message<GetBlockHash>},
      {u8"get_block_header_by_hash", handle_message<GetBlockHeaderByHash>},
      {u8"get_block_header_by_height", handle_message<GetBlockHeaderByHeight>},
      {u8"get_block_headers_by_height", handle_message<GetBlockHeadersByHeight>},
      {u8"get_blocks_fast", handle_message<GetBlocksFast>},
      {u8"get_dynamic_fee_estimate", handle_message<GetFeeEstimate>},
      {u8"get_hashes_fast", handle_message<GetHashesFast>},
      {u8"get_height", handle_message<GetHeight>},
      {u8"get_info", handle_message<GetInfo>},
      {u8"get_last_block_header", handle_message<GetLastBlockHeader>},
      {u8"get_output_distribution", handle_message<GetOutputDistribution>},
      {u8"get_output_histogram", handle_message<GetOutputHistogram>},
      {u8"get_output_keys", handle_message<GetOutputKeys>},
      {u8"get_peer_list", handle_message<GetPeerList>},
      {u8"get_rpc_version", handle_message<GetRPCVersion>},
      {u8"get_transaction_pool", handle_message<GetTransactionPool>},
      {u8"get_transactions", handle_message<GetTransactions>},
      {u8"get_tx_global_output_indices", handle_message<GetTxGlobalOutputIndices>},
      {u8"hard_fork_info", handle_message<HardForkInfo>},
      {u8"key_images_spent", handle_message<KeyImagesSpent>},
      {u8"mining_status", handle_message<MiningStatus>},
      {u8"save_bc", handle_message<SaveBC>},
      {u8"send_raw_tx", handle_message<SendRawTx>},
      {u8"send_raw_tx_hex", handle_message<SendRawTxHex>},
      {u8"set_log_level", handle_message<SetLogLevel>},
      {u8"start_mining", handle_message<StartMining>},
      {u8"stop_mining", handle_message<StopMining>}
    };
  } // anonymous

  DaemonHandler::DaemonHandler(cryptonote::core& c, t_p2p& p2p)
    : m_core(c), m_p2p(p2p)
  {
    const auto last_sorted = std::is_sorted_until(std::begin(handlers), std::end(handlers));
    if (last_sorted != std::end(handlers))
      throw std::logic_error{std::string{"ZMQ JSON-RPC handlers map is not properly sorted, see "} + last_sorted->method_name};
  }

  void DaemonHandler::handle(const GetHeight::Request& req, GetHeight::Response& res)
  {
    res.height = m_core.get_current_blockchain_height();

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlocksFast::Request& req, GetBlocksFast::Response& res)
  {
    std::vector<std::pair<std::pair<blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, blobdata> > > > blocks;

    if(!m_core.find_blockchain_supplement(req.start_height, req.block_ids, blocks, res.current_height, res.start_height, req.prune, true, COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT, COMMAND_RPC_GET_BLOCKS_FAST_MAX_TX_COUNT))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "core::find_blockchain_supplement() returned false";
      return;
    }

    res.blocks.resize(blocks.size());
    res.output_indices.resize(blocks.size());

    auto it = blocks.begin();

    uint64_t block_count = 0;
    while (it != blocks.end())
    {
      cryptonote::rpc::block_with_transactions& bwt = res.blocks[block_count];

      if (!parse_and_validate_block_from_blob(it->first.first, bwt.block))
      {
        res.blocks.clear();
        res.output_indices.clear();
        res.status = Message::STATUS_FAILED;
        res.error_details = "failed retrieving a requested block";
        return;
      }

      if (it->second.size() != bwt.block.tx_hashes.size())
      {
          res.blocks.clear();
          res.output_indices.clear();
          res.status = Message::STATUS_FAILED;
          res.error_details = "incorrect number of transactions retrieved for block";
          return;
      }

      cryptonote::rpc::block_output_indices& indices = res.output_indices[block_count];

      // miner tx output indices
      {
        cryptonote::rpc::tx_output_indices tx_indices;
        if (!m_core.get_tx_outputs_gindexs(get_transaction_hash(bwt.block.miner_tx), tx_indices))
        {
          res.status = Message::STATUS_FAILED;
          res.error_details = "core::get_tx_outputs_gindexs() returned false";
          return;
        }
        indices.push_back(std::move(tx_indices));
      }

      auto hash_it = bwt.block.tx_hashes.begin();
      bwt.transactions.reserve(it->second.size());
      for (const auto& blob : it->second)
      {
        bwt.transactions.emplace_back();
        bwt.transactions.back().pruned = req.prune;

        const bool parsed = req.prune ?
          parse_and_validate_tx_base_from_blob(blob.second, bwt.transactions.back()) :
          parse_and_validate_tx_from_blob(blob.second, bwt.transactions.back());
        if (!parsed)
        {
          res.blocks.clear();
          res.output_indices.clear();
          res.status = Message::STATUS_FAILED;
          res.error_details = "failed retrieving a requested transaction";
          return;
        }

        cryptonote::rpc::tx_output_indices tx_indices;
        if (!m_core.get_tx_outputs_gindexs(*hash_it, tx_indices))
        {
          res.status = Message::STATUS_FAILED;
          res.error_details = "core::get_tx_outputs_gindexs() returned false";
          return;
        }

        indices.push_back(std::move(tx_indices));
        ++hash_it;
      }

      it++;
      block_count++;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetHashesFast::Request& req, GetHashesFast::Response& res)
  {
    res.start_height = req.start_height;

    auto& chain = m_core.get_blockchain_storage();

    if (!chain.find_blockchain_supplement(req.known_hashes, res.hashes, NULL, res.start_height, res.current_height, false))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Blockchain::find_blockchain_supplement() returned false";
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetTransactions::Request& req, GetTransactions::Response& res)
  {
    std::vector<cryptonote::transaction> found_txs_vec;
    std::vector<crypto::hash> missed_vec;

    bool r = m_core.get_transactions(req.tx_hashes, found_txs_vec, missed_vec);

    // TODO: consider fixing core::get_transactions to not hide exceptions
    if (!r)
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "core::get_transactions() returned false (exception caught there)";
      return;
    }

    size_t num_found = found_txs_vec.size();

    std::vector<uint64_t> heights(num_found);
    std::vector<bool> in_pool(num_found, false);
    std::vector<crypto::hash> found_hashes(num_found);

    for (size_t i=0; i < num_found; i++)
    {
      found_hashes[i] = get_transaction_hash(found_txs_vec[i]);
      heights[i] = m_core.get_blockchain_storage().get_db().get_tx_block_height(found_hashes[i]);
    }

    // if any missing from blockchain, check in tx pool
    if (!missed_vec.empty())
    {
      std::vector<cryptonote::transaction> pool_txs;

      m_core.get_pool_transactions(pool_txs);

      for (const auto& tx : pool_txs)
      {
        crypto::hash h = get_transaction_hash(tx);

        auto itr = std::find(missed_vec.begin(), missed_vec.end(), h);

        if (itr != missed_vec.end())
        {
          found_hashes.push_back(h);
          found_txs_vec.push_back(tx);
          heights.push_back(std::numeric_limits<uint64_t>::max());
          in_pool.push_back(true);
          missed_vec.erase(itr);
        }
      }
    }

    for (size_t i=0; i < found_hashes.size(); i++)
    {
      cryptonote::rpc::transaction_info info;
      info.height = heights[i];
      info.in_pool = in_pool[i];
      info.transaction = std::move(found_txs_vec[i]);

      res.txs.emplace(found_hashes[i], std::move(info));
    }
                                      
    res.missed_hashes = std::move(missed_vec);
    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const KeyImagesSpent::Request& req, KeyImagesSpent::Response& res)
  {
    res.spent_status.resize(req.key_images.size(), KeyImagesSpent::STATUS::UNSPENT);

    std::vector<bool> chain_spent_status;
    std::vector<bool> pool_spent_status;

    m_core.are_key_images_spent(req.key_images, chain_spent_status);
    m_core.are_key_images_spent_in_pool(req.key_images, pool_spent_status);

    if ((chain_spent_status.size() != req.key_images.size()) || (pool_spent_status.size() != req.key_images.size()))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "tx_pool::have_key_images_as_spent() gave vectors of wrong size(s).";
      return;
    }

    for(size_t i=0; i < req.key_images.size(); i++)
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

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetTxGlobalOutputIndices::Request& req, GetTxGlobalOutputIndices::Response& res)
  {
    if (!m_core.get_tx_outputs_gindexs(req.tx_hash, res.output_indices))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "core::get_tx_outputs_gindexs() returned false";
      return;
    }

    res.status = Message::STATUS_OK;

  }

  void DaemonHandler::handle(const SendRawTx::Request& req, SendRawTx::Response& res)
  {
    handleTxBlob(cryptonote::tx_to_blob(req.tx), req.relay, res);
  }

  void DaemonHandler::handle(const SendRawTxHex::Request& req, SendRawTxHex::Response& res)
  {
    std::string tx_blob;
    if(!epee::string_tools::parse_hexstr_to_binbuff(req.tx_as_hex, tx_blob))
    {
      MERROR("[SendRawTxHex]: Failed to parse tx from hexbuff: " << req.tx_as_hex);
      res.status = Message::STATUS_FAILED;
      res.error_details = "Invalid hex";
      return;
    }
    handleTxBlob(std::move(tx_blob), req.relay, res);
  }

  void DaemonHandler::handleTxBlob(std::string&& tx_blob, bool relay, SendRawTx::Response& res)
  {
    if (!m_p2p.get_payload_object().is_synchronized())
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Not ready to accept transactions; try again later";
      return;
    }

    tx_verification_context tvc = AUTO_VAL_INIT(tvc);

    if(!m_core.handle_incoming_tx(tx_blob, tvc, (relay ? relay_method::local : relay_method::none), false) || tvc.m_verifivation_failed)
    {
      if (tvc.m_verifivation_failed)
      {
        MERROR("[SendRawTx]: tx verification failed");
      }
      else
      {
        MERROR("[SendRawTx]: Failed to process tx");
      }
      res.status = Message::STATUS_FAILED;
      res.error_details = "";

      if (tvc.m_low_mixin)
      {
        res.error_details = "mixin too low";
      }
      if (tvc.m_double_spend)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "double spend";
      }
      if (tvc.m_invalid_input)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "invalid input";
      }
      if (tvc.m_invalid_output)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "invalid output";
      }
      if (tvc.m_too_big)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "too big";
      }
      if (tvc.m_overspend)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "overspend";
      }
      if (tvc.m_fee_too_low)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "fee too low";
      }
      if (tvc.m_too_few_outputs)
      {
        if (!res.error_details.empty()) res.error_details += " and ";
        res.error_details += "too few outputs";
      }
      if (res.error_details.empty())
      {
        res.error_details = "an unknown issue was found with the transaction";
      }

      return;
    }

    if(tvc.m_relay == relay_method::none || !relay)
    {
      MERROR("[SendRawTx]: tx accepted, but not relayed");
      res.error_details = "Not relayed";
      res.relayed = false;
      res.status = Message::STATUS_OK;

      return;
    }

    NOTIFY_NEW_TRANSACTIONS::request r;
    r.txs.push_back(std::move(tx_blob));
    m_core.get_protocol()->relay_transactions(r, boost::uuids::nil_uuid(), epee::net_utils::zone::invalid, relay_method::local);

    //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
    res.status = Message::STATUS_OK;
    res.relayed = true;

    return;
  }

  void DaemonHandler::handle(const StartMining::Request& req, StartMining::Response& res)
  {
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_core.get_nettype(), req.miner_address))
    {
      res.error_details = "Failed, wrong address";
      LOG_PRINT_L0(res.error_details);
      res.status = Message::STATUS_FAILED;
      return;
    }
    if (info.is_subaddress)
    {
      res.error_details = "Failed, mining to subaddress isn't supported yet";
      LOG_PRINT_L0(res.error_details);
      res.status = Message::STATUS_FAILED;
      return;
    }

    unsigned int concurrency_count = boost::thread::hardware_concurrency() * 4;

    // if we couldn't detect threads, set it to a ridiculously high number
    if(concurrency_count == 0)
    {
      concurrency_count = 257;
    }

    // if there are more threads requested than the hardware supports
    // then we fail and log that.
    if(req.threads_count > concurrency_count)
    {
      res.error_details = "Failed, too many threads relative to CPU cores.";
      LOG_PRINT_L0(res.error_details);
      res.status = Message::STATUS_FAILED;
      return;
    }

    if(!m_core.get_miner().start(info.address, static_cast<size_t>(req.threads_count), req.do_background_mining, req.ignore_battery))
    {
      res.error_details = "Failed, mining not started";
      LOG_PRINT_L0(res.error_details);
      res.status = Message::STATUS_FAILED;
      return;
    }
    res.status = Message::STATUS_OK;
    res.error_details = "";

  }

  void DaemonHandler::handle(const GetInfo::Request& req, GetInfo::Response& res)
  {
    res.info.height = m_core.get_current_blockchain_height();

    res.info.target_height = m_core.get_target_blockchain_height();

    if (res.info.height > res.info.target_height)
    {
      res.info.target_height = res.info.height;
    }

    auto& chain = m_core.get_blockchain_storage();

    res.info.wide_difficulty = chain.get_difficulty_for_next_block();
    res.info.difficulty = (res.info.wide_difficulty & 0xffffffffffffffff).convert_to<uint64_t>();

    res.info.target = chain.get_difficulty_target();

    res.info.tx_count = chain.get_total_transactions() - res.info.height; //without coinbase

    res.info.tx_pool_size = m_core.get_pool_transactions_count();

    res.info.alt_blocks_count = chain.get_alternative_blocks_count();

    uint64_t total_conn = m_p2p.get_public_connections_count();
    res.info.outgoing_connections_count = m_p2p.get_public_outgoing_connections_count();
    res.info.incoming_connections_count = total_conn - res.info.outgoing_connections_count;

    res.info.white_peerlist_size = m_p2p.get_public_white_peers_count();

    res.info.grey_peerlist_size = m_p2p.get_public_gray_peers_count();

    res.info.mainnet = m_core.get_nettype() == MAINNET;
    res.info.testnet = m_core.get_nettype() == TESTNET;
    res.info.stagenet = m_core.get_nettype() == STAGENET;
    res.info.wide_cumulative_difficulty = m_core.get_blockchain_storage().get_db().get_block_cumulative_difficulty(res.info.height - 1);
    res.info.cumulative_difficulty = (res.info.wide_cumulative_difficulty & 0xffffffffffffffff).convert_to<uint64_t>();
    res.info.block_size_limit = res.info.block_weight_limit = m_core.get_blockchain_storage().get_current_cumulative_block_weight_limit();
    res.info.block_size_median = res.info.block_weight_median = m_core.get_blockchain_storage().get_current_cumulative_block_weight_median();
    res.info.adjusted_time = m_core.get_blockchain_storage().get_adjusted_time(res.info.height);
    res.info.start_time = (uint64_t)m_core.get_start_time();
    res.info.version = MONERO_VERSION;

    res.status = Message::STATUS_OK;
    res.error_details = "";
  }

  void DaemonHandler::handle(const StopMining::Request& req, StopMining::Response& res)
  {
    if(!m_core.get_miner().stop())
    {
      res.error_details = "Failed, mining not stopped";
      LOG_PRINT_L0(res.error_details);
      res.status = Message::STATUS_FAILED;
      return;
    }

    res.status = Message::STATUS_OK;
    res.error_details = "";
  }

  void DaemonHandler::handle(const MiningStatus::Request& req, MiningStatus::Response& res)
  {
    const cryptonote::miner& lMiner = m_core.get_miner();
    res.active = lMiner.is_mining();
    res.is_background_mining_enabled = lMiner.get_is_background_mining_enabled();
    
    if ( lMiner.is_mining() ) {
      res.speed = lMiner.get_speed();
      res.threads_count = lMiner.get_threads_count();
      const account_public_address& lMiningAdr = lMiner.get_mining_address();
      res.address = get_account_address_as_str(m_core.get_nettype(), false, lMiningAdr);
    }

    res.status = Message::STATUS_OK;
    res.error_details = "";
  }

  void DaemonHandler::handle(const SaveBC::Request& req, SaveBC::Response& res)
  {
    if (!m_core.get_blockchain_storage().store_blockchain())
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Error storing the blockchain";
    }
    else
    {
      res.status = Message::STATUS_OK;
    }
  }

  void DaemonHandler::handle(const GetBlockHash::Request& req, GetBlockHash::Response& res)
  {
    if (m_core.get_current_blockchain_height() <= req.height)
    {
      res.hash = crypto::null_hash;
      res.status = Message::STATUS_FAILED;
      res.error_details = "height given is higher than current chain height";
      return;
    }

    res.hash = m_core.get_block_id_by_height(req.height);

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlockTemplate::Request& req, GetBlockTemplate::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const SubmitBlock::Request& req, SubmitBlock::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const GetLastBlockHeader::Request& req, GetLastBlockHeader::Response& res)
  {
    const crypto::hash block_hash = m_core.get_tail_id();

    if (!getBlockHeaderByHash(block_hash, res.header))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Requested block does not exist";
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlockHeaderByHash::Request& req, GetBlockHeaderByHash::Response& res)
  {
    if (!getBlockHeaderByHash(req.hash, res.header))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Requested block does not exist";
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlockHeaderByHeight::Request& req, GetBlockHeaderByHeight::Response& res)
  {
    const crypto::hash block_hash = m_core.get_block_id_by_height(req.height);

    if (!getBlockHeaderByHash(block_hash, res.header))
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Requested block does not exist";
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlockHeadersByHeight::Request& req, GetBlockHeadersByHeight::Response& res)
  {
    res.headers.resize(req.heights.size());

    for (size_t i=0; i < req.heights.size(); i++)
    {
      const crypto::hash block_hash = m_core.get_block_id_by_height(req.heights[i]);

      if (!getBlockHeaderByHash(block_hash, res.headers[i]))
      {
        res.status = Message::STATUS_FAILED;
        res.error_details = "A requested block does not exist";
        return;
      }
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBlock::Request& req, GetBlock::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const GetPeerList::Request& req, GetPeerList::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const SetLogHashRate::Request& req, SetLogHashRate::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const SetLogLevel::Request& req, SetLogLevel::Response& res)
  {
    if (req.level < 0 || req.level > 4)
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = "Error: log level not valid";
    }
    else
    {
      res.status = Message::STATUS_OK;
      mlog_set_log_level(req.level);
    }
  }

  void DaemonHandler::handle(const GetTransactionPool::Request& req, GetTransactionPool::Response& res)
  {
    bool r = m_core.get_pool_for_rpc(res.transactions, res.key_images);

    if (!r) res.status = Message::STATUS_FAILED;
    else res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetConnections::Request& req, GetConnections::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const GetBlockHeadersRange::Request& req, GetBlockHeadersRange::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const StopDaemon::Request& req, StopDaemon::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const StartSaveGraph::Request& req, StartSaveGraph::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const StopSaveGraph::Request& req, StopSaveGraph::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const HardForkInfo::Request& req, HardForkInfo::Response& res)
  {
    const Blockchain &blockchain = m_core.get_blockchain_storage();
    uint8_t version = req.version > 0 ? req.version : blockchain.get_ideal_hard_fork_version();
    res.info.version = blockchain.get_current_hard_fork_version();
    res.info.enabled = blockchain.get_hard_fork_voting_info(version, res.info.window, res.info.votes, res.info.threshold, res.info.earliest_height, res.info.voting);
    res.info.state = blockchain.get_hard_fork_state();
    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetBans::Request& req, GetBans::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const SetBans::Request& req, SetBans::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const FlushTransactionPool::Request& req, FlushTransactionPool::Response& res)
  {
    res.status = Message::STATUS_FAILED;
    res.error_details = "RPC method not yet implemented.";
  }

  void DaemonHandler::handle(const GetOutputHistogram::Request& req, GetOutputHistogram::Response& res)
  {
    std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t> > histogram;
    try
    {
      histogram = m_core.get_blockchain_storage().get_output_histogram(req.amounts, req.unlocked, req.recent_cutoff);
    }
    catch (const std::exception &e)
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = e.what();
      return;
    }

    res.histogram.clear();
    res.histogram.reserve(histogram.size());
    for (const auto &i: histogram)
    {
      if (std::get<0>(i.second) >= req.min_count && (std::get<0>(i.second) <= req.max_count || req.max_count == 0))
        res.histogram.emplace_back(output_amount_count{i.first, std::get<0>(i.second), std::get<1>(i.second), std::get<2>(i.second)});
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetOutputKeys::Request& req, GetOutputKeys::Response& res)
  {
    try
    {
      for (const auto& i : req.outputs)
      {
        crypto::public_key key;
        rct::key mask;
        bool unlocked;
        m_core.get_blockchain_storage().get_output_key_mask_unlocked(i.amount, i.index, key, mask, unlocked);
        res.keys.emplace_back(output_key_mask_unlocked{key, mask, unlocked});
      }
    }
    catch (const std::exception& e)
    {
      res.status = Message::STATUS_FAILED;
      res.error_details = e.what();
      return;
    }

    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetRPCVersion::Request& req, GetRPCVersion::Response& res)
  {
    res.version = DAEMON_RPC_VERSION_ZMQ;
    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetFeeEstimate::Request& req, GetFeeEstimate::Response& res)
  {
    res.hard_fork_version = m_core.get_blockchain_storage().get_current_hard_fork_version();
    res.estimated_base_fee = m_core.get_blockchain_storage().get_dynamic_base_fee_estimate(req.num_grace_blocks);

    if (res.hard_fork_version < HF_VERSION_PER_BYTE_FEE)
    {
       res.size_scale = 1024; // per KiB fee
       res.fee_mask = 1;
    }
    else
    {
      res.size_scale = 1; // per byte fee
      res.fee_mask = Blockchain::get_fee_quantization_mask();
    }
    res.status = Message::STATUS_OK;
  }

  void DaemonHandler::handle(const GetOutputDistribution::Request& req, GetOutputDistribution::Response& res)
  {
    try
    {
      res.distributions.reserve(req.amounts.size());

      const uint64_t req_to_height = req.to_height ? req.to_height : (m_core.get_current_blockchain_height() - 1);
      for (std::uint64_t amount : req.amounts)
      {
        auto data = rpc::RpcHandler::get_output_distribution([this](uint64_t amount, uint64_t from, uint64_t to, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) { return m_core.get_output_distribution(amount, from, to, start_height, distribution, base); }, amount, req.from_height, req_to_height, [this](uint64_t height) { return m_core.get_blockchain_storage().get_db().get_block_hash_from_height(height); }, req.cumulative, m_core.get_current_blockchain_height());
        if (!data)
        {
          res.distributions.clear();
          res.status = Message::STATUS_FAILED;
          res.error_details = "Failed to get output distribution";
          return;
        }
        res.distributions.push_back(output_distribution{std::move(*data), amount, req.cumulative});
      }
      res.status = Message::STATUS_OK;
    }
    catch (const std::exception& e)
    {
      res.distributions.clear();
      res.status = Message::STATUS_FAILED;
      res.error_details = e.what();
    }
  }

  bool DaemonHandler::getBlockHeaderByHash(const crypto::hash& hash_in, cryptonote::rpc::BlockHeaderResponse& header)
  {
    block b;

    if (!m_core.get_block_by_hash(hash_in, b))
    {
      return false;
    }

    header.hash = hash_in;
    if (b.miner_tx.vin.size() != 1 || b.miner_tx.vin.front().type() != typeid(txin_gen))
    {
      return false;
    }
    header.height = boost::get<txin_gen>(b.miner_tx.vin.front()).height;

    header.major_version = b.major_version;
    header.minor_version = b.minor_version;
    header.timestamp = b.timestamp;
    header.nonce = b.nonce;
    header.prev_id = b.prev_id;

    header.depth = m_core.get_current_blockchain_height() - header.height - 1;

    header.reward = 0;
    for (const auto& out : b.miner_tx.vout)
    {
      header.reward += out.amount;
    }

    header.wide_difficulty = m_core.get_blockchain_storage().block_difficulty(header.height);
    header.difficulty = (header.wide_difficulty & 0xffffffffffffffff).convert_to<uint64_t>();

    return true;
  }

  epee::byte_slice DaemonHandler::handle(std::string&& request)
  {
    MDEBUG("Handling RPC request: " << request);

    try
    {
      FullMessage req_full(std::move(request), true);

      const std::string request_type = req_full.getRequestType();
      const auto matched_handler = std::lower_bound(std::begin(handlers), std::end(handlers), request_type);
      if (matched_handler == std::end(handlers) || matched_handler->method_name != request_type)
        return BAD_REQUEST(request_type, req_full.getID());

      epee::byte_slice response = matched_handler->call(*this, req_full.getID(), req_full.getMessage());

      const boost::string_ref response_view{reinterpret_cast<const char*>(response.data()), response.size()};
      MDEBUG("Returning RPC response: " << response_view);

      return response;
    }
    catch (const std::exception& e)
    {
      return BAD_JSON(e.what());
    }
  }

}  // namespace rpc

}  // namespace cryptonote
