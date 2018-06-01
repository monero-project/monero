// Copyright (c) 2014-2018, The Monero Project
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

#include "include_base_utils.h"
#include "string_tools.h"
using namespace epee;

#include "core_rpc_server.h"
#include "common/command_line.h"
#include "common/updates.h"
#include "common/download.h"
#include "common/util.h"
#include "common/perf_timer.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "misc_language.h"
#include "storages/http_abstract_invoke.h"
#include "crypto/hash.h"
#include "rpc/rpc_args.h"
#include "core_rpc_server_error_codes.h"
#include "p2p/net_node.h"
#include "get_output_distribution_cache.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc"

#define MAX_RESTRICTED_FAKE_OUTS_COUNT 40
#define MAX_RESTRICTED_GLOBAL_FAKE_OUTS_COUNT 5000

namespace
{
  void add_reason(std::string &reasons, const char *reason)
  {
    if (!reasons.empty())
      reasons += ", ";
    reasons += reason;
  }
}

namespace cryptonote
{

  //-----------------------------------------------------------------------------------
  void core_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_rpc_restricted_bind_port);
    command_line::add_arg(desc, arg_restricted_rpc);
    command_line::add_arg(desc, arg_bootstrap_daemon_address);
    command_line::add_arg(desc, arg_bootstrap_daemon_login);
    cryptonote::rpc_args::init_options(desc);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  core_rpc_server::core_rpc_server(
      core& cr
    , nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& p2p
    )
    : m_core(cr)
    , m_p2p(p2p)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::init(
      const boost::program_options::variables_map& vm
      , const bool restricted
      , const network_type nettype
      , const std::string& port
    )
  {
    m_restricted = restricted;
    m_nettype = nettype;
    m_net_server.set_threads_prefix("RPC");

    auto rpc_config = cryptonote::rpc_args::process(vm);
    if (!rpc_config)
      return false;

    m_bootstrap_daemon_address = command_line::get_arg(vm, arg_bootstrap_daemon_address);
    if (!m_bootstrap_daemon_address.empty())
    {
      const std::string &bootstrap_daemon_login = command_line::get_arg(vm, arg_bootstrap_daemon_login);
      const auto loc = bootstrap_daemon_login.find(':');
      if (!bootstrap_daemon_login.empty() && loc != std::string::npos)
      {
        epee::net_utils::http::login login;
        login.username = bootstrap_daemon_login.substr(0, loc);
        login.password = bootstrap_daemon_login.substr(loc + 1);
        m_http_client.set_server(m_bootstrap_daemon_address, login, false);
      }
      else
      {
        m_http_client.set_server(m_bootstrap_daemon_address, boost::none, false);
      }
      m_should_use_bootstrap_daemon = true;
    }
    else
    {
      m_should_use_bootstrap_daemon = false;
    }
    m_was_bootstrap_ever_used = false;

    boost::optional<epee::net_utils::http::login> http_login{};

    if (rpc_config->login)
      http_login.emplace(std::move(rpc_config->login->username), std::move(rpc_config->login->password).password());

    auto rng = [](size_t len, uint8_t *ptr){ return crypto::rand(len, ptr); };
    return epee::http_server_impl_base<core_rpc_server, connection_context>::init(
      rng, std::move(port), std::move(rpc_config->bind_ip), std::move(rpc_config->access_control_origins), std::move(http_login)
    );
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::check_core_ready()
  {
    if(!m_p2p.get_payload_object().is_synchronized())
    {
      return false;
    }
    return true;
  }
#define CHECK_CORE_READY() do { if(!check_core_ready()){res.status =  CORE_RPC_STATUS_BUSY;return true;} } while(0)

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res)
  {
    PERF_TIMER(on_get_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_HEIGHT>(invoke_http_mode::JON, "/getheight", req, res, r))
      return r;

    res.height = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res)
  {
    PERF_TIMER(on_get_info);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_INFO>(invoke_http_mode::JON, "/getinfo", req, res, r))
    {
      res.bootstrap_daemon_address = m_bootstrap_daemon_address;
      crypto::hash top_hash;
      m_core.get_blockchain_top(res.height_without_bootstrap, top_hash);
      ++res.height_without_bootstrap; // turn top block height into blockchain height
      res.was_bootstrap_ever_used = true;
      return r;
    }

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height; // turn top block height into blockchain height
    res.top_block_hash = string_tools::pod_to_hex(top_hash);
    res.target_height = m_core.get_target_blockchain_height();
    res.difficulty = m_core.get_blockchain_storage().get_difficulty_for_next_block();
    res.target = m_core.get_blockchain_storage().get_difficulty_target();
    res.tx_count = m_core.get_blockchain_storage().get_total_transactions() - res.height; //without coinbase
    res.tx_pool_size = m_core.get_pool_transactions_count();
    res.alt_blocks_count = m_core.get_blockchain_storage().get_alternative_blocks_count();
    uint64_t total_conn = m_p2p.get_connections_count();
    res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
    res.incoming_connections_count = total_conn - res.outgoing_connections_count;
    res.rpc_connections_count = get_connections_count();
    res.white_peerlist_size = m_p2p.get_peerlist_manager().get_white_peers_count();
    res.grey_peerlist_size = m_p2p.get_peerlist_manager().get_gray_peers_count();
    res.mainnet = m_nettype == MAINNET;
    res.testnet = m_nettype == TESTNET;
    res.stagenet = m_nettype == STAGENET;
    res.cumulative_difficulty = m_core.get_blockchain_storage().get_db().get_block_cumulative_difficulty(res.height - 1);
    res.block_size_limit = m_core.get_blockchain_storage().get_current_cumulative_blocksize_limit();
    res.block_size_median = m_core.get_blockchain_storage().get_current_cumulative_blocksize_median();
    res.status = CORE_RPC_STATUS_OK;
    res.start_time = (uint64_t)m_core.get_start_time();
    res.free_space = m_restricted ? std::numeric_limits<uint64_t>::max() : m_core.get_free_space();
    res.offline = m_core.offline();
    res.bootstrap_daemon_address = m_bootstrap_daemon_address;
    res.height_without_bootstrap = res.height;
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      res.was_bootstrap_ever_used = m_was_bootstrap_ever_used;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  static cryptonote::blobdata get_pruned_tx_blob(cryptonote::transaction &tx)
  {
    std::stringstream ss;
    binary_archive<true> ba(ss);
    bool r = tx.serialize_base(ba);
    CHECK_AND_ASSERT_MES(r, cryptonote::blobdata(), "Failed to serialize rct signatures base");
    return ss.str();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  static cryptonote::blobdata get_pruned_tx_blob(const cryptonote::blobdata &blobdata)
  {
    cryptonote::transaction tx;

    if (!cryptonote::parse_and_validate_tx_from_blob(blobdata, tx))
    {
      MERROR("Failed to parse and validate tx from blob");
      return cryptonote::blobdata();
    }
    return get_pruned_tx_blob(tx);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res)
  {
    PERF_TIMER(on_get_blocks);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCKS_FAST>(invoke_http_mode::BIN, "/getblocks.bin", req, res, r))
      return r;

    std::list<std::pair<cryptonote::blobdata, std::list<cryptonote::blobdata> > > bs;

    if(!m_core.find_blockchain_supplement(req.start_height, req.block_ids, bs, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      res.status = "Failed";
      return false;
    }

    size_t pruned_size = 0, unpruned_size = 0, ntxes = 0;
    for(auto& bd: bs)
    {
      res.blocks.resize(res.blocks.size()+1);
      res.blocks.back().block = bd.first;
      pruned_size += bd.first.size();
      unpruned_size += bd.first.size();
      res.output_indices.push_back(COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices());
      res.output_indices.back().indices.push_back(COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices());
      block b;
      if (!parse_and_validate_block_from_blob(bd.first, b))
      {
        res.status = "Invalid block";
        return false;
      }
      bool r = m_core.get_tx_outputs_gindexs(get_transaction_hash(b.miner_tx), res.output_indices.back().indices.back().indices);
      if (!r)
      {
        res.status = "Failed";
        return false;
      }
      size_t txidx = 0;
      ntxes += bd.second.size();
      for (std::list<cryptonote::blobdata>::iterator i = bd.second.begin(); i != bd.second.end(); ++i)
      {
        unpruned_size += i->size();
        if (req.prune)
          res.blocks.back().txs.push_back(get_pruned_tx_blob(std::move(*i)));
        else
          res.blocks.back().txs.push_back(std::move(*i));
        i->clear();
        i->shrink_to_fit();
        pruned_size += res.blocks.back().txs.back().size();

        res.output_indices.back().indices.push_back(COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices());
        bool r = m_core.get_tx_outputs_gindexs(b.tx_hashes[txidx++], res.output_indices.back().indices.back().indices);
        if (!r)
        {
          res.status = "Failed";
          return false;
        }
      }
    }

    MDEBUG("on_get_blocks: " << bs.size() << " blocks, " << ntxes << " txes, pruned size " << pruned_size << ", unpruned size " << unpruned_size);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
    bool core_rpc_server::on_get_alt_blocks_hashes(const COMMAND_RPC_GET_ALT_BLOCKS_HASHES::request& req, COMMAND_RPC_GET_ALT_BLOCKS_HASHES::response& res)
    {
      PERF_TIMER(on_get_alt_blocks_hashes);
      bool r;
      if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_ALT_BLOCKS_HASHES>(invoke_http_mode::JON, "/get_alt_blocks_hashes", req, res, r))
        return r;

      std::list<block> blks;

      if(!m_core.get_alternative_blocks(blks))
      {
          res.status = "Failed";
          return false;
      }

      res.blks_hashes.reserve(blks.size());

      for (auto const& blk: blks)
      {
          res.blks_hashes.push_back(epee::string_tools::pod_to_hex(get_block_hash(blk)));
      }

      MDEBUG("on_get_alt_blocks_hashes: " << blks.size() << " blocks " );
      res.status = CORE_RPC_STATUS_OK;
      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks_by_height(const COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response& res)
  {
    PERF_TIMER(on_get_blocks_by_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCKS_BY_HEIGHT>(invoke_http_mode::BIN, "/getblocks_by_height.bin", req, res, r))
      return r;

    res.status = "Failed";
    res.blocks.clear();
    res.blocks.reserve(req.heights.size());
    for (uint64_t height : req.heights)
    {
      block blk;
      try
      {
        blk = m_core.get_blockchain_storage().get_db().get_block_from_height(height);
      }
      catch (...)
      {
        res.status = "Error retrieving block at height " + std::to_string(height);
        return true;
      }
      std::list<transaction> txs;
      std::list<crypto::hash> missed_txs;
      m_core.get_transactions(blk.tx_hashes, txs, missed_txs);
      res.blocks.resize(res.blocks.size() + 1);
      res.blocks.back().block = block_to_blob(blk);
      for (auto& tx : txs)
        res.blocks.back().txs.push_back(tx_to_blob(tx));
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_hashes(const COMMAND_RPC_GET_HASHES_FAST::request& req, COMMAND_RPC_GET_HASHES_FAST::response& res)
  {
    PERF_TIMER(on_get_hashes);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_HASHES_FAST>(invoke_http_mode::BIN, "/gethashes.bin", req, res, r))
      return r;

    NOTIFY_RESPONSE_CHAIN_ENTRY::request resp;

    resp.start_height = req.start_height;
    if(!m_core.find_blockchain_supplement(req.block_ids, resp))
    {
      res.status = "Failed";
      return false;
    }
    res.current_height = resp.total_height;
    res.start_height = resp.start_height;
    res.m_block_ids = std::move(resp.m_block_ids);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
  {
    PERF_TIMER(on_get_random_outs);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS>(invoke_http_mode::BIN, "/getrandom_outs.bin", req, res, r))
      return r;

    res.status = "Failed";

    if (m_restricted)
    {
      if (req.amounts.size() > 100 || req.outs_count > MAX_RESTRICTED_FAKE_OUTS_COUNT)
      {
        res.status = "Too many outs requested";
        return true;
      }
    }

    if(!m_core.get_random_outs_for_amounts(req, res))
    {
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    std::stringstream ss;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)
    {
      ss << "[" << ofa.amount << "]:";
      CHECK_AND_ASSERT_MES(ofa.outs.size(), ;, "internal error: ofa.outs.size() is empty for amount " << ofa.amount);
      std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
          {
            ss << oe.global_amount_index << " ";
          });
      ss << ENDL;
    });
    std::string s = ss.str();
    LOG_PRINT_L2("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_outs_bin(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res)
  {
    PERF_TIMER(on_get_outs_bin);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUTS_BIN>(invoke_http_mode::BIN, "/get_outs.bin", req, res, r))
      return r;

    res.status = "Failed";

    if (m_restricted)
    {
      if (req.outputs.size() > MAX_RESTRICTED_GLOBAL_FAKE_OUTS_COUNT)
      {
        res.status = "Too many outs requested";
        return true;
      }
    }

    if(!m_core.get_outs(req, res))
    {
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_outs(const COMMAND_RPC_GET_OUTPUTS::request& req, COMMAND_RPC_GET_OUTPUTS::response& res)
  {
    PERF_TIMER(on_get_outs);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUTS>(invoke_http_mode::JON, "/get_outs", req, res, r))
      return r;

    res.status = "Failed";

    if (m_restricted)
    {
      if (req.outputs.size() > MAX_RESTRICTED_GLOBAL_FAKE_OUTS_COUNT)
      {
        res.status = "Too many outs requested";
        return true;
      }
    }

    cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::request req_bin;
    req_bin.outputs = req.outputs;
    cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::response res_bin;
    if(!m_core.get_outs(req_bin, res_bin))
    {
      return true;
    }

    // convert to text
    for (const auto &i: res_bin.outs)
    {
      res.outs.push_back(cryptonote::COMMAND_RPC_GET_OUTPUTS::outkey());
      cryptonote::COMMAND_RPC_GET_OUTPUTS::outkey &outkey = res.outs.back();
      outkey.key = epee::string_tools::pod_to_hex(i.key);
      outkey.mask = epee::string_tools::pod_to_hex(i.mask);
      outkey.unlocked = i.unlocked;
      outkey.height = i.height;
      outkey.txid = epee::string_tools::pod_to_hex(i.txid);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_rct_outs(const COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::request& req, COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::response& res)
  {
    PERF_TIMER(on_get_random_rct_outs);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS>(invoke_http_mode::BIN, "/getrandom_rctouts.bin", req, res, r))
      return r;

    res.status = "Failed";
    if(!m_core.get_random_rct_outs(req, res))
    {
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    std::stringstream ss;
    typedef COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::out_entry out_entry;
    CHECK_AND_ASSERT_MES(res.outs.size(), true, "internal error: res.outs.size() is empty");
    std::for_each(res.outs.begin(), res.outs.end(), [&](out_entry& oe)
      {
        ss << oe.global_amount_index << " ";
      });
    ss << ENDL;
    std::string s = ss.str();
    LOG_PRINT_L2("COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS: " << ENDL << s);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res)
  {
    PERF_TIMER(on_get_indexes);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES>(invoke_http_mode::BIN, "/get_o_indexes.bin", req, res, ok))
      return ok;

    bool r = m_core.get_tx_outputs_gindexs(req.txid, res.o_indexes);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    LOG_PRINT_L2("COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES: [" << res.o_indexes.size() << "]");
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res)
  {
    PERF_TIMER(on_get_transactions);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTIONS>(invoke_http_mode::JON, "/gettransactions", req, res, ok))
      return ok;

    std::vector<crypto::hash> vh;
    for(const auto& tx_hex_str: req.txs_hashes)
    {
      blobdata b;
      if(!string_tools::parse_hexstr_to_binbuff(tx_hex_str, b))
      {
        res.status = "Failed to parse hex representation of transaction hash";
        return true;
      }
      if(b.size() != sizeof(crypto::hash))
      {
        res.status = "Failed, size of data mismatch";
        return true;
      }
      vh.push_back(*reinterpret_cast<const crypto::hash*>(b.data()));
    }
    std::list<crypto::hash> missed_txs;
    std::list<transaction> txs;
    bool r = m_core.get_transactions(vh, txs, missed_txs);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    LOG_PRINT_L2("Found " << txs.size() << "/" << vh.size() << " transactions on the blockchain");

    // try the pool for any missing txes
    size_t found_in_pool = 0;
    std::unordered_set<crypto::hash> pool_tx_hashes;
    std::unordered_map<crypto::hash, bool> double_spend_seen;
    if (!missed_txs.empty())
    {
      std::vector<tx_info> pool_tx_info;
      std::vector<spent_key_image_info> pool_key_image_info;
      bool r = m_core.get_pool_transactions_and_spent_keys_info(pool_tx_info, pool_key_image_info);
      if(r)
      {
        // sort to match original request
        std::list<transaction> sorted_txs;
        std::vector<tx_info>::const_iterator i;
        for (const crypto::hash &h: vh)
        {
          if (std::find(missed_txs.begin(), missed_txs.end(), h) == missed_txs.end())
          {
            if (txs.empty())
            {
              res.status = "Failed: internal error - txs is empty";
              return true;
            }
            // core returns the ones it finds in the right order
            if (get_transaction_hash(txs.front()) != h)
            {
              res.status = "Failed: tx hash mismatch";
              return true;
            }
            sorted_txs.push_back(std::move(txs.front()));
            txs.pop_front();
          }
          else if ((i = std::find_if(pool_tx_info.begin(), pool_tx_info.end(), [h](const tx_info &txi) { return epee::string_tools::pod_to_hex(h) == txi.id_hash; })) != pool_tx_info.end())
          {
            cryptonote::transaction tx;
            if (!cryptonote::parse_and_validate_tx_from_blob(i->tx_blob, tx))
            {
              res.status = "Failed to parse and validate tx from blob";
              return true;
            }
            sorted_txs.push_back(tx);
            missed_txs.remove(h);
            pool_tx_hashes.insert(h);
            const std::string hash_string = epee::string_tools::pod_to_hex(h);
            for (const auto &ti: pool_tx_info)
            {
              if (ti.id_hash == hash_string)
              {
                double_spend_seen.insert(std::make_pair(h, ti.double_spend_seen));
                break;
              }
            }
            ++found_in_pool;
          }
        }
        txs = sorted_txs;
      }
      LOG_PRINT_L2("Found " << found_in_pool << "/" << vh.size() << " transactions in the pool");
    }

    std::list<std::string>::const_iterator txhi = req.txs_hashes.begin();
    std::vector<crypto::hash>::const_iterator vhi = vh.begin();
    for(auto& tx: txs)
    {
      res.txs.push_back(COMMAND_RPC_GET_TRANSACTIONS::entry());
      COMMAND_RPC_GET_TRANSACTIONS::entry &e = res.txs.back();

      crypto::hash tx_hash = *vhi++;
      e.tx_hash = *txhi++;
      blobdata blob = req.prune ? get_pruned_tx_blob(tx) : t_serializable_object_to_blob(tx);
      e.as_hex = string_tools::buff_to_hex_nodelimer(blob);
      if (req.decode_as_json)
        e.as_json = obj_to_json_str(tx);
      e.in_pool = pool_tx_hashes.find(tx_hash) != pool_tx_hashes.end();
      if (e.in_pool)
      {
        e.block_height = e.block_timestamp = std::numeric_limits<uint64_t>::max();
        if (double_spend_seen.find(tx_hash) != double_spend_seen.end())
        {
          e.double_spend_seen = double_spend_seen[tx_hash];
        }
        else
        {
          MERROR("Failed to determine double spend status for " << tx_hash);
          e.double_spend_seen = false;
        }
      }
      else
      {
        e.block_height = m_core.get_blockchain_storage().get_db().get_tx_block_height(tx_hash);
        e.block_timestamp = m_core.get_blockchain_storage().get_db().get_block_timestamp(e.block_height);
        e.double_spend_seen = false;
      }

      // fill up old style responses too, in case an old wallet asks
      res.txs_as_hex.push_back(e.as_hex);
      if (req.decode_as_json)
        res.txs_as_json.push_back(e.as_json);

      // output indices too if not in pool
      if (pool_tx_hashes.find(tx_hash) == pool_tx_hashes.end())
      {
        bool r = m_core.get_tx_outputs_gindexs(tx_hash, e.output_indices);
        if (!r)
        {
          res.status = "Failed";
          return false;
        }
      }
    }

    for(const auto& miss_tx: missed_txs)
    {
      res.missed_tx.push_back(string_tools::pod_to_hex(miss_tx));
    }

    LOG_PRINT_L2(res.txs.size() << " transactions found, " << res.missed_tx.size() << " not found");
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_is_key_image_spent(const COMMAND_RPC_IS_KEY_IMAGE_SPENT::request& req, COMMAND_RPC_IS_KEY_IMAGE_SPENT::response& res, bool request_has_rpc_origin)
  {
    PERF_TIMER(on_is_key_image_spent);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_IS_KEY_IMAGE_SPENT>(invoke_http_mode::JON, "/is_key_image_spent", req, res, ok))
      return ok;

    std::vector<crypto::key_image> key_images;
    for(const auto& ki_hex_str: req.key_images)
    {
      blobdata b;
      if(!string_tools::parse_hexstr_to_binbuff(ki_hex_str, b))
      {
        res.status = "Failed to parse hex representation of key image";
        return true;
      }
      if(b.size() != sizeof(crypto::key_image))
      {
        res.status = "Failed, size of data mismatch";
      }
      key_images.push_back(*reinterpret_cast<const crypto::key_image*>(b.data()));
    }
    std::vector<bool> spent_status;
    bool r = m_core.are_key_images_spent(key_images, spent_status);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    res.spent_status.clear();
    for (size_t n = 0; n < spent_status.size(); ++n)
      res.spent_status.push_back(spent_status[n] ? COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_BLOCKCHAIN : COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT);

    // check the pool too
    std::vector<cryptonote::tx_info> txs;
    std::vector<cryptonote::spent_key_image_info> ki;
    r = m_core.get_pool_transactions_and_spent_keys_info(txs, ki, !request_has_rpc_origin || !m_restricted);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    for (std::vector<cryptonote::spent_key_image_info>::const_iterator i = ki.begin(); i != ki.end(); ++i)
    {
      crypto::hash hash;
      crypto::key_image spent_key_image;
      if (parse_hash256(i->id_hash, hash))
      {
        memcpy(&spent_key_image, &hash, sizeof(hash)); // a bit dodgy, should be other parse functions somewhere
        for (size_t n = 0; n < res.spent_status.size(); ++n)
        {
          if (res.spent_status[n] == COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT)
          {
            if (key_images[n] == spent_key_image)
            {
              res.spent_status[n] = COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_POOL;
              break;
            }
          }
        }
      }
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res)
  {
    PERF_TIMER(on_send_raw_tx);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_SEND_RAW_TX>(invoke_http_mode::JON, "/sendrawtransaction", req, res, ok))
      return ok;

    CHECK_CORE_READY();

    std::string tx_blob;
    if(!string_tools::parse_hexstr_to_binbuff(req.tx_as_hex, tx_blob))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex);
      res.status = "Failed";
      return true;
    }

    cryptonote_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    if(!m_core.handle_incoming_tx(tx_blob, tvc, false, false, req.do_not_relay) || tvc.m_verifivation_failed)
    {
      res.status = "Failed";
      res.reason = "";
      if ((res.low_mixin = tvc.m_low_mixin))
        add_reason(res.reason, "ring size too small");
      if ((res.double_spend = tvc.m_double_spend))
        add_reason(res.reason, "double spend");
      if ((res.invalid_input = tvc.m_invalid_input))
        add_reason(res.reason, "invalid input");
      if ((res.invalid_output = tvc.m_invalid_output))
        add_reason(res.reason, "invalid output");
      if ((res.too_big = tvc.m_too_big))
        add_reason(res.reason, "too big");
      if ((res.overspend = tvc.m_overspend))
        add_reason(res.reason, "overspend");
      if ((res.fee_too_low = tvc.m_fee_too_low))
        add_reason(res.reason, "fee too low");
      if ((res.not_rct = tvc.m_not_rct))
        add_reason(res.reason, "tx is not ringct");
      const std::string punctuation = res.reason.empty() ? "" : ": ";
      if (tvc.m_verifivation_failed)
      {
        LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed" << punctuation << res.reason);
      }
      else
      {
        LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx" << punctuation << res.reason);
      }
      return true;
    }

    if(!tvc.m_should_be_relayed)
    {
      LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
      res.reason = "Not relayed";
      res.not_relayed = true;
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }

    NOTIFY_NEW_TRANSACTIONS::request r;
    r.txs.push_back(tx_blob);
    m_core.get_protocol()->relay_transactions(r, fake_context);
    //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res)
  {
    PERF_TIMER(on_start_mining);
    CHECK_CORE_READY();
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_nettype, req.miner_address))
    {
      res.status = "Failed, wrong address";
      LOG_PRINT_L0(res.status);
      return true;
    }
    if (info.is_subaddress)
    {
      res.status = "Mining to subaddress isn't supported yet";
      LOG_PRINT_L0(res.status);
      return true;
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
      res.status = "Failed, too many threads relative to CPU cores.";
      LOG_PRINT_L0(res.status);
      return true;
    }

    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);

    if(!m_core.get_miner().start(info.address, static_cast<size_t>(req.threads_count), attrs, req.do_background_mining, req.ignore_battery))
    {
      res.status = "Failed, mining not started";
      LOG_PRINT_L0(res.status);
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res)
  {
    PERF_TIMER(on_stop_mining);
    if(!m_core.get_miner().stop())
    {
      res.status = "Failed, mining not stopped";
      LOG_PRINT_L0(res.status);
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_mining_status(const COMMAND_RPC_MINING_STATUS::request& req, COMMAND_RPC_MINING_STATUS::response& res)
  {
    PERF_TIMER(on_mining_status);

    const miner& lMiner = m_core.get_miner();
    res.active = lMiner.is_mining();
    res.is_background_mining_enabled = lMiner.get_is_background_mining_enabled();
    
    if ( lMiner.is_mining() ) {
      res.speed = lMiner.get_speed();
      res.threads_count = lMiner.get_threads_count();
      const account_public_address& lMiningAdr = lMiner.get_mining_address();
      res.address = get_account_address_as_str(m_nettype, false, lMiningAdr);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_save_bc(const COMMAND_RPC_SAVE_BC::request& req, COMMAND_RPC_SAVE_BC::response& res)
  {
    PERF_TIMER(on_save_bc);
    if( !m_core.get_blockchain_storage().store_blockchain() )
    {
      res.status = "Error while storing blockchain";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_peer_list(const COMMAND_RPC_GET_PEER_LIST::request& req, COMMAND_RPC_GET_PEER_LIST::response& res)
  {
    PERF_TIMER(on_get_peer_list);
    std::list<nodetool::peerlist_entry> white_list;
    std::list<nodetool::peerlist_entry> gray_list;
    m_p2p.get_peerlist_manager().get_peerlist_full(gray_list, white_list);


    for (auto & entry : white_list)
    {
      if (entry.adr.get_type_id() == epee::net_utils::ipv4_network_address::ID)
        res.white_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv4_network_address>().ip(),
            entry.adr.as<epee::net_utils::ipv4_network_address>().port(), entry.last_seen);
      else
        res.white_list.emplace_back(entry.id, entry.adr.str(), entry.last_seen);
    }

    for (auto & entry : gray_list)
    {
      if (entry.adr.get_type_id() == epee::net_utils::ipv4_network_address::ID)
        res.gray_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv4_network_address>().ip(),
            entry.adr.as<epee::net_utils::ipv4_network_address>().port(), entry.last_seen);
      else
        res.gray_list.emplace_back(entry.id, entry.adr.str(), entry.last_seen);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_log_hash_rate(const COMMAND_RPC_SET_LOG_HASH_RATE::request& req, COMMAND_RPC_SET_LOG_HASH_RATE::response& res)
  {
    PERF_TIMER(on_set_log_hash_rate);
    if(m_core.get_miner().is_mining())
    {
      m_core.get_miner().do_print_hashrate(req.visible);
      res.status = CORE_RPC_STATUS_OK;
    }
    else
    {
      res.status = CORE_RPC_STATUS_NOT_MINING;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_log_level(const COMMAND_RPC_SET_LOG_LEVEL::request& req, COMMAND_RPC_SET_LOG_LEVEL::response& res)
  {
    PERF_TIMER(on_set_log_level);
    if (req.level < 0 || req.level > 4)
    {
      res.status = "Error: log level not valid";
      return true;
    }
    mlog_set_log_level(req.level);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_log_categories(const COMMAND_RPC_SET_LOG_CATEGORIES::request& req, COMMAND_RPC_SET_LOG_CATEGORIES::response& res)
  {
    PERF_TIMER(on_set_log_categories);
    mlog_set_log(req.categories.c_str());
    res.categories = mlog_get_categories();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool(const COMMAND_RPC_GET_TRANSACTION_POOL::request& req, COMMAND_RPC_GET_TRANSACTION_POOL::response& res, bool request_has_rpc_origin)
  {
    PERF_TIMER(on_get_transaction_pool);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL>(invoke_http_mode::JON, "/get_transaction_pool", req, res, r))
      return r;

    m_core.get_pool_transactions_and_spent_keys_info(res.transactions, res.spent_key_images, !request_has_rpc_origin || !m_restricted);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool_hashes(const COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response& res, bool request_has_rpc_origin)
  {
    PERF_TIMER(on_get_transaction_pool_hashes);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_HASHES>(invoke_http_mode::JON, "/get_transaction_pool_hashes.bin", req, res, r))
      return r;

    m_core.get_pool_transaction_hashes(res.tx_hashes, !request_has_rpc_origin || !m_restricted);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool_stats(const COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response& res, bool request_has_rpc_origin)
  {
    PERF_TIMER(on_get_transaction_pool_stats);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_STATS>(invoke_http_mode::JON, "/get_transaction_pool_stats", req, res, r))
      return r;

    m_core.get_pool_transaction_stats(res.pool_stats, !request_has_rpc_origin || !m_restricted);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res)
  {
    PERF_TIMER(on_stop_daemon);
    // FIXME: replace back to original m_p2p.send_stop_signal() after
    // investigating why that isn't working quite right.
    m_p2p.send_stop_signal();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res)
  {
    PERF_TIMER(on_getblockcount);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        res.status = "This command is unsupported for bootstrap daemon";
        return false;
      }
    }
    res.count = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_getblockhash);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        res = "This command is unsupported for bootstrap daemon";
        return false;
      }
    }
    if(req.size() != 1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong parameters, expected height";
      return false;
    }
    uint64_t h = req[0];
    if(m_core.get_current_blockchain_height() <= h)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("Too big height: ") + std::to_string(h) + ", current blockchain height = " +  std::to_string(m_core.get_current_blockchain_height());
    }
    res = string_tools::pod_to_hex(m_core.get_block_id_by_height(h));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  // equivalent of strstr, but with arbitrary bytes (ie, NULs)
  // This does not differentiate between "not found" and "found at offset 0"
  uint64_t slow_memmem(const void* start_buff, size_t buflen,const void* pat,size_t patlen)
  {
    const void* buf = start_buff;
    const void* end=(const char*)buf+buflen;
    if (patlen > buflen || patlen == 0) return 0;
    while(buflen>0 && (buf=memchr(buf,((const char*)pat)[0],buflen-patlen+1)))
    {
      if(memcmp(buf,pat,patlen)==0)
        return (const char*)buf - (const char*)start_buff;
      buf=(const char*)buf+1;
      buflen = (const char*)end - (const char*)buf;
    }
    return 0;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_getblocktemplate);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GETBLOCKTEMPLATE>(invoke_http_mode::JON_RPC, "getblocktemplate", req, res, r))
      return r;

    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }

    if(req.reserve_size > 255)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE;
      error_resp.message = "Too big reserved size, maximum 255";
      return false;
    }

    cryptonote::address_parse_info info;

    if(!req.wallet_address.size() || !cryptonote::get_account_address_from_str(info, m_nettype, req.wallet_address))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS;
      error_resp.message = "Failed to parse wallet address";
      return false;
    }
    if (info.is_subaddress)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_MINING_TO_SUBADDRESS;
      error_resp.message = "Mining to subaddress is not supported yet";
      return false;
    }

    block b = AUTO_VAL_INIT(b);
    cryptonote::blobdata blob_reserve;
    blob_reserve.resize(req.reserve_size, 0);
    if(!m_core.get_block_template(b, info.address, res.difficulty, res.height, res.expected_reward, blob_reserve))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to create block template");
      return false;
    }
    blobdata block_blob = t_serializable_object_to_blob(b);
    crypto::public_key tx_pub_key = cryptonote::get_tx_pub_key_from_extra(b.miner_tx);
    if(tx_pub_key == crypto::null_pkey)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to  tx pub key in coinbase extra");
      return false;
    }
    res.reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
    if(!res.reserved_offset)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to find tx pub key in blockblob");
      return false;
    }
    res.reserved_offset += sizeof(tx_pub_key) + 2; //2 bytes: tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
    if(res.reserved_offset + req.reserve_size > block_blob.size())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to calculate offset for ");
      return false;
    }
    blobdata hashing_blob = get_block_hashing_blob(b);
    res.prev_hash = string_tools::pod_to_hex(b.prev_id);
    res.blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
    res.blockhashing_blob =  string_tools::buff_to_hex_nodelimer(hashing_blob);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_submitblock);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        res.status = "This command is unsupported for bootstrap daemon";
        return false;
      }
    }
    CHECK_CORE_READY();
    if(req.size()!=1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong param";
      return false;
    }
    blobdata blockblob;
    if(!string_tools::parse_hexstr_to_binbuff(req[0], blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }
    
    // Fixing of high orphan issue for most pools
    // Thanks Boolberry!
    block b = AUTO_VAL_INIT(b);
    if(!parse_and_validate_block_from_blob(blockblob, b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }

    // Fix from Boolberry neglects to check block
    // size, do that with the function below
    if(!m_core.check_incoming_block_size(blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB_SIZE;
      error_resp.message = "Block bloc size is too big, rejecting block";
      return false;
    }

    if(!m_core.handle_block_found(b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED;
      error_resp.message = "Block not accepted";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  uint64_t core_rpc_server::get_block_reward(const block& blk)
  {
    uint64_t reward = 0;
    for(const tx_out& out: blk.miner_tx.vout)
    {
      reward += out.amount;
    }
    return reward;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::fill_block_header_response(const block& blk, bool orphan_status, uint64_t height, const crypto::hash& hash, block_header_response& response)
  {
    PERF_TIMER(fill_block_header_response);
    response.major_version = blk.major_version;
    response.minor_version = blk.minor_version;
    response.timestamp = blk.timestamp;
    response.prev_hash = string_tools::pod_to_hex(blk.prev_id);
    response.nonce = blk.nonce;
    response.orphan_status = orphan_status;
    response.height = height;
    response.depth = m_core.get_current_blockchain_height() - height - 1;
    response.hash = string_tools::pod_to_hex(hash);
    response.difficulty = m_core.get_blockchain_storage().block_difficulty(height);
    response.reward = get_block_reward(blk);
    response.block_size = m_core.get_blockchain_storage().get_db().get_block_size(height);
    response.num_txes = blk.tx_hashes.size();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template <typename COMMAND_TYPE>
  bool core_rpc_server::use_bootstrap_daemon_if_necessary(const invoke_http_mode &mode, const std::string &command_name, const typename COMMAND_TYPE::request& req, typename COMMAND_TYPE::response& res, bool &r)
  {
    res.untrusted = false;
    if (m_bootstrap_daemon_address.empty())
      return false;

    boost::unique_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
    if (!m_should_use_bootstrap_daemon)
    {
      MINFO("The local daemon is fully synced. Not switching back to the bootstrap daemon");
      return false;
    }

    auto current_time = std::chrono::system_clock::now();
    if (current_time - m_bootstrap_height_check_time > std::chrono::seconds(30))  // update every 30s
    {
      m_bootstrap_height_check_time = current_time;

      uint64_t top_height;
      crypto::hash top_hash;
      m_core.get_blockchain_top(top_height, top_hash);
      ++top_height; // turn top block height into blockchain height

      // query bootstrap daemon's height
      cryptonote::COMMAND_RPC_GET_HEIGHT::request getheight_req;
      cryptonote::COMMAND_RPC_GET_HEIGHT::response getheight_res;
      bool ok = epee::net_utils::invoke_http_json("/getheight", getheight_req, getheight_res, m_http_client);
      ok = ok && getheight_res.status == CORE_RPC_STATUS_OK;

      m_should_use_bootstrap_daemon = ok && top_height + 10 < getheight_res.height;
      MINFO((m_should_use_bootstrap_daemon ? "Using" : "Not using") << " the bootstrap daemon (our height: " << top_height << ", bootstrap daemon's height: " << getheight_res.height << ")");
    }
    if (!m_should_use_bootstrap_daemon)
      return false;

    if (mode == invoke_http_mode::JON)
    {
      r = epee::net_utils::invoke_http_json(command_name, req, res, m_http_client);
    }
    else if (mode == invoke_http_mode::BIN)
    {
      r = epee::net_utils::invoke_http_bin(command_name, req, res, m_http_client);
    }
    else if (mode == invoke_http_mode::JON_RPC)
    {
      epee::json_rpc::request<typename COMMAND_TYPE::request> json_req = AUTO_VAL_INIT(json_req);
      epee::json_rpc::response<typename COMMAND_TYPE::response, std::string> json_resp = AUTO_VAL_INIT(json_resp);
      json_req.jsonrpc = "2.0";
      json_req.id = epee::serialization::storage_entry(0);
      json_req.method = command_name;
      json_req.params = req;
      r = net_utils::invoke_http_json("/json_rpc", json_req, json_resp, m_http_client);
      if (r)
        res = json_resp.result;
    }
    else
    {
      MERROR("Unknown invoke_http_mode: " << mode);
      return false;
    }
    m_was_bootstrap_ever_used = true;
    r = r && res.status == CORE_RPC_STATUS_OK;
    res.untrusted = true;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_last_block_header);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_LAST_BLOCK_HEADER>(invoke_http_mode::JON_RPC, "getlastblockheader", req, res, r))
      return r;

    CHECK_CORE_READY();
    uint64_t last_block_height;
    crypto::hash last_block_hash;
    m_core.get_blockchain_top(last_block_height, last_block_hash);
    block last_block;
    bool have_last_block = m_core.get_block_by_hash(last_block_hash, last_block);
    if (!have_last_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get last block.";
      return false;
    }
    bool response_filled = fill_block_header_response(last_block, false, last_block_height, last_block_hash, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp){
    PERF_TIMER(on_get_block_header_by_hash);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH>(invoke_http_mode::JON_RPC, "getblockheaderbyhash", req, res, r))
      return r;

    crypto::hash block_hash;
    bool hash_parsed = parse_hash256(req.hash, block_hash);
    if(!hash_parsed)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Failed to parse hex representation of block hash. Hex = " + req.hash + '.';
      return false;
    }
    block blk;
    bool orphan = false;
    bool have_block = m_core.get_block_by_hash(block_hash, blk, &orphan);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by hash. Hash = " + req.hash + '.';
      return false;
    }
    if (blk.miner_tx.vin.size() != 1 || blk.miner_tx.vin.front().type() != typeid(txin_gen))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
      return false;
    }
    uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
    bool response_filled = fill_block_header_response(blk, orphan, block_height, block_hash, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_headers_range(const COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request& req, COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response& res, epee::json_rpc::error& error_resp){
    PERF_TIMER(on_get_block_headers_range);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK_HEADERS_RANGE>(invoke_http_mode::JON_RPC, "getblockheadersrange", req, res, r))
      return r;

    const uint64_t bc_height = m_core.get_current_blockchain_height();
    if (req.start_height >= bc_height || req.end_height >= bc_height || req.start_height > req.end_height)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = "Invalid start/end heights.";
      return false;
    }
    for (uint64_t h = req.start_height; h <= req.end_height; ++h)
    {
      crypto::hash block_hash = m_core.get_block_id_by_height(h);
      block blk;
      bool have_block = m_core.get_block_by_hash(block_hash, blk);
      if (!have_block)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: can't get block by height. Height = " + boost::lexical_cast<std::string>(h) + ". Hash = " + epee::string_tools::pod_to_hex(block_hash) + '.';
        return false;
      }
      if (blk.miner_tx.vin.size() != 1 || blk.miner_tx.vin.front().type() != typeid(txin_gen))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
        return false;
      }
      uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
      if (block_height != h)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: coinbase transaction in the block has the wrong height";
        return false;
      }
      res.headers.push_back(block_header_response());
      bool response_filled = fill_block_header_response(blk, false, block_height, block_hash, res.headers.back());
      if (!response_filled)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: can't produce valid response.";
        return false;
      }
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp){
    PERF_TIMER(on_get_block_header_by_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT>(invoke_http_mode::JON_RPC, "getblockheaderbyheight", req, res, r))
      return r;

    if(m_core.get_current_blockchain_height() <= req.height)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("Too big height: ") + std::to_string(req.height) + ", current blockchain height = " +  std::to_string(m_core.get_current_blockchain_height());
      return false;
    }
    crypto::hash block_hash = m_core.get_block_id_by_height(req.height);
    block blk;
    bool have_block = m_core.get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by height. Height = " + std::to_string(req.height) + '.';
      return false;
    }
    bool response_filled = fill_block_header_response(blk, false, req.height, block_hash, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block(const COMMAND_RPC_GET_BLOCK::request& req, COMMAND_RPC_GET_BLOCK::response& res, epee::json_rpc::error& error_resp){
    PERF_TIMER(on_get_block);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK>(invoke_http_mode::JON_RPC, "getblock", req, res, r))
      return r;

    crypto::hash block_hash;
    if (!req.hash.empty())
    {
      bool hash_parsed = parse_hash256(req.hash, block_hash);
      if(!hash_parsed)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Failed to parse hex representation of block hash. Hex = " + req.hash + '.';
        return false;
      }
    }
    else
    {
      if(m_core.get_current_blockchain_height() <= req.height)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
        error_resp.message = std::string("Too big height: ") + std::to_string(req.height) + ", current blockchain height = " +  std::to_string(m_core.get_current_blockchain_height());
        return false;
      }
      block_hash = m_core.get_block_id_by_height(req.height);
    }
    block blk;
    bool orphan = false;
    bool have_block = m_core.get_block_by_hash(block_hash, blk, &orphan);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by hash. Hash = " + req.hash + '.';
      return false;
    }
    if (blk.miner_tx.vin.size() != 1 || blk.miner_tx.vin.front().type() != typeid(txin_gen))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
      return false;
    }
    uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
    bool response_filled = fill_block_header_response(blk, orphan, block_height, block_hash, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.miner_tx_hash = epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(blk.miner_tx));
    for (size_t n = 0; n < blk.tx_hashes.size(); ++n)
    {
      res.tx_hashes.push_back(epee::string_tools::pod_to_hex(blk.tx_hashes[n]));
    }
    res.blob = string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(blk));
    res.json = obj_to_json_str(blk);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_connections(const COMMAND_RPC_GET_CONNECTIONS::request& req, COMMAND_RPC_GET_CONNECTIONS::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_connections);

    res.connections = m_p2p.get_payload_object().get_connections();

    res.status = CORE_RPC_STATUS_OK;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info_json(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_info_json);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_INFO>(invoke_http_mode::JON_RPC, "get_info", req, res, r))
    {
      res.bootstrap_daemon_address = m_bootstrap_daemon_address;
      crypto::hash top_hash;
      m_core.get_blockchain_top(res.height_without_bootstrap, top_hash);
      ++res.height_without_bootstrap; // turn top block height into blockchain height
      res.was_bootstrap_ever_used = true;
      return r;
    }

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height; // turn top block height into blockchain height
    res.top_block_hash = string_tools::pod_to_hex(top_hash);
    res.target_height = m_core.get_target_blockchain_height();
    res.difficulty = m_core.get_blockchain_storage().get_difficulty_for_next_block();
    res.target = m_core.get_blockchain_storage().get_current_hard_fork_version() < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2;
    res.tx_count = m_core.get_blockchain_storage().get_total_transactions() - res.height; //without coinbase
    res.tx_pool_size = m_core.get_pool_transactions_count();
    res.alt_blocks_count = m_core.get_blockchain_storage().get_alternative_blocks_count();
    uint64_t total_conn = m_p2p.get_connections_count();
    res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
    res.incoming_connections_count = total_conn - res.outgoing_connections_count;
    res.rpc_connections_count = get_connections_count();
    res.white_peerlist_size = m_p2p.get_peerlist_manager().get_white_peers_count();
    res.grey_peerlist_size = m_p2p.get_peerlist_manager().get_gray_peers_count();
    res.mainnet = m_nettype == MAINNET;
    res.testnet = m_nettype == TESTNET;
    res.stagenet = m_nettype == STAGENET;
    res.cumulative_difficulty = m_core.get_blockchain_storage().get_db().get_block_cumulative_difficulty(res.height - 1);
    res.block_size_limit = m_core.get_blockchain_storage().get_current_cumulative_blocksize_limit();
    res.block_size_median = m_core.get_blockchain_storage().get_current_cumulative_blocksize_median();
    res.status = CORE_RPC_STATUS_OK;
    res.start_time = (uint64_t)m_core.get_start_time();
    res.free_space = m_restricted ? std::numeric_limits<uint64_t>::max() : m_core.get_free_space();
    res.offline = m_core.offline();
    res.bootstrap_daemon_address = m_bootstrap_daemon_address;
    res.height_without_bootstrap = res.height;
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      res.was_bootstrap_ever_used = m_was_bootstrap_ever_used;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_hard_fork_info(const COMMAND_RPC_HARD_FORK_INFO::request& req, COMMAND_RPC_HARD_FORK_INFO::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_hard_fork_info);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_HARD_FORK_INFO>(invoke_http_mode::JON_RPC, "hard_fork_info", req, res, r))
      return r;

    const Blockchain &blockchain = m_core.get_blockchain_storage();
    uint8_t version = req.version > 0 ? req.version : blockchain.get_next_hard_fork_version();
    res.version = blockchain.get_current_hard_fork_version();
    res.enabled = blockchain.get_hard_fork_voting_info(version, res.window, res.votes, res.threshold, res.earliest_height, res.voting);
    res.state = blockchain.get_hard_fork_state();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_bans(const COMMAND_RPC_GETBANS::request& req, COMMAND_RPC_GETBANS::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_bans);

    auto now = time(nullptr);
    std::map<std::string, time_t> blocked_hosts = m_p2p.get_blocked_hosts();
    for (std::map<std::string, time_t>::const_iterator i = blocked_hosts.begin(); i != blocked_hosts.end(); ++i)
    {
      if (i->second > now) {
        COMMAND_RPC_GETBANS::ban b;
        b.host = i->first;
        b.ip = 0;
        uint32_t ip;
        if (epee::string_tools::get_ip_int32_from_string(ip, i->first))
          b.ip = ip;
        b.seconds = i->second - now;
        res.bans.push_back(b);
      }
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_bans(const COMMAND_RPC_SETBANS::request& req, COMMAND_RPC_SETBANS::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_set_bans);

    for (auto i = req.bans.begin(); i != req.bans.end(); ++i)
    {
      epee::net_utils::network_address na;
      if (!i->host.empty())
      {
        if (!epee::net_utils::create_network_address(na, i->host))
        {
          error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
          error_resp.message = "Unsupported host type";
          return false;
        }
      }
      else
      {
        na = epee::net_utils::ipv4_network_address{i->ip, 0};
      }
      if (i->ban)
        m_p2p.block_host(na, i->seconds);
      else
        m_p2p.unblock_host(na);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_flush_txpool(const COMMAND_RPC_FLUSH_TRANSACTION_POOL::request& req, COMMAND_RPC_FLUSH_TRANSACTION_POOL::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_flush_txpool);

    bool failed = false;
    std::list<crypto::hash> txids;
    if (req.txids.empty())
    {
      std::list<transaction> pool_txs;
      bool r = m_core.get_pool_transactions(pool_txs);
      if (!r)
      {
        res.status = "Failed to get txpool contents";
        return true;
      }
      for (const auto &tx: pool_txs)
      {
        txids.push_back(cryptonote::get_transaction_hash(tx));
      }
    }
    else
    {
      for (const auto &str: req.txids)
      {
        cryptonote::blobdata txid_data;
        if(!epee::string_tools::parse_hexstr_to_binbuff(str, txid_data))
        {
          failed = true;
        }
        else
        {
          crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());
          txids.push_back(txid);
        }
      }
    }
    if (!m_core.get_blockchain_storage().flush_txes_from_pool(txids))
    {
      res.status = "Failed to remove one or more tx(es)";
      return false;
    }

    if (failed)
    {
      if (txids.empty())
        res.status = "Failed to parse txid";
      else
        res.status = "Failed to parse some of the txids";
      return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_output_histogram(const COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request& req, COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_output_histogram);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUT_HISTOGRAM>(invoke_http_mode::JON_RPC, "get_output_histogram", req, res, r))
      return r;

    std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> histogram;
    try
    {
      histogram = m_core.get_blockchain_storage().get_output_histogram(req.amounts, req.unlocked, req.recent_cutoff, req.min_count);
    }
    catch (const std::exception &e)
    {
      res.status = "Failed to get output histogram";
      return true;
    }

    res.histogram.clear();
    res.histogram.reserve(histogram.size());
    for (const auto &i: histogram)
    {
      if (std::get<0>(i.second) >= req.min_count && (std::get<0>(i.second) <= req.max_count || req.max_count == 0))
        res.histogram.push_back(COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry(i.first, std::get<0>(i.second), std::get<1>(i.second), std::get<2>(i.second)));
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_version(const COMMAND_RPC_GET_VERSION::request& req, COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_version);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_VERSION>(invoke_http_mode::JON_RPC, "get_version", req, res, r))
      return r;

    res.version = CORE_RPC_VERSION;
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_coinbase_tx_sum(const COMMAND_RPC_GET_COINBASE_TX_SUM::request& req, COMMAND_RPC_GET_COINBASE_TX_SUM::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_coinbase_tx_sum);
    std::pair<uint64_t, uint64_t> amounts = m_core.get_coinbase_tx_sum(req.height, req.count);
    res.emission_amount = amounts.first;
    res.fee_amount = amounts.second;
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_per_kb_fee_estimate(const COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::request& req, COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_per_kb_fee_estimate);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE>(invoke_http_mode::JON_RPC, "get_fee_estimate", req, res, r))
      return r;

    res.fee = m_core.get_blockchain_storage().get_dynamic_per_kb_fee_estimate(req.grace_blocks);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alternate_chains(const COMMAND_RPC_GET_ALTERNATE_CHAINS::request& req, COMMAND_RPC_GET_ALTERNATE_CHAINS::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_alternate_chains);
    try
    {
      std::list<std::pair<Blockchain::block_extended_info, uint64_t>> chains = m_core.get_blockchain_storage().get_alternative_chains();
      for (const auto &i: chains)
      {
        res.chains.push_back(COMMAND_RPC_GET_ALTERNATE_CHAINS::chain_info{epee::string_tools::pod_to_hex(get_block_hash(i.first.bl)), i.first.height, i.second, i.first.cumulative_difficulty});
      }
      res.status = CORE_RPC_STATUS_OK;
    }
    catch (...)
    {
      res.status = "Error retrieving alternate chains";
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_limit(const COMMAND_RPC_GET_LIMIT::request& req, COMMAND_RPC_GET_LIMIT::response& res)
  {
    PERF_TIMER(on_get_limit);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_LIMIT>(invoke_http_mode::JON, "/get_limit", req, res, r))
      return r;

    res.limit_down = epee::net_utils::connection_basic::get_rate_down_limit();
    res.limit_up = epee::net_utils::connection_basic::get_rate_up_limit();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_limit(const COMMAND_RPC_SET_LIMIT::request& req, COMMAND_RPC_SET_LIMIT::response& res)
  {
    PERF_TIMER(on_set_limit);
    // -1 = reset to default
    //  0 = do not modify

    if (req.limit_down > 0)
    {
      epee::net_utils::connection_basic::set_rate_down_limit(req.limit_down);
    }
    else if (req.limit_down < 0)
    {
      if (req.limit_down != -1)
      {
        res.status = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        return false;
      }
      epee::net_utils::connection_basic::set_rate_down_limit(nodetool::default_limit_down);
    }

    if (req.limit_up > 0)
    {
      epee::net_utils::connection_basic::set_rate_up_limit(req.limit_up);
    }
    else if (req.limit_up < 0)
    {
      if (req.limit_up != -1)
      {
        res.status = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        return false;
      }
      epee::net_utils::connection_basic::set_rate_up_limit(nodetool::default_limit_up);
    }

    res.limit_down = epee::net_utils::connection_basic::get_rate_down_limit();
    res.limit_up = epee::net_utils::connection_basic::get_rate_up_limit();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_out_peers(const COMMAND_RPC_OUT_PEERS::request& req, COMMAND_RPC_OUT_PEERS::response& res)
  {
    PERF_TIMER(on_out_peers);
    size_t n_connections = m_p2p.get_outgoing_connections_count();
    size_t n_delete = (n_connections > req.out_peers) ? n_connections - req.out_peers : 0;
    m_p2p.m_config.m_net_config.max_out_connection_count = req.out_peers;
    if (n_delete)
      m_p2p.delete_out_connections(n_delete);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_in_peers(const COMMAND_RPC_IN_PEERS::request& req, COMMAND_RPC_IN_PEERS::response& res)
  {
    PERF_TIMER(on_in_peers);
    size_t n_connections = m_p2p.get_incoming_connections_count();
    size_t n_delete = (n_connections > req.in_peers) ? n_connections - req.in_peers : 0;
    m_p2p.m_config.m_net_config.max_in_connection_count = req.in_peers;
    if (n_delete)
      m_p2p.delete_in_connections(n_delete);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_start_save_graph(const COMMAND_RPC_START_SAVE_GRAPH::request& req, COMMAND_RPC_START_SAVE_GRAPH::response& res)
  {
	  PERF_TIMER(on_start_save_graph);
	  m_p2p.set_save_graph(true);
	  res.status = CORE_RPC_STATUS_OK;
	  return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_save_graph(const COMMAND_RPC_STOP_SAVE_GRAPH::request& req, COMMAND_RPC_STOP_SAVE_GRAPH::response& res)
  {
	  PERF_TIMER(on_stop_save_graph);
	  m_p2p.set_save_graph(false);
	  res.status = CORE_RPC_STATUS_OK;
	  return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_update(const COMMAND_RPC_UPDATE::request& req, COMMAND_RPC_UPDATE::response& res)
  {
    PERF_TIMER(on_update);
    static const char software[] = "monero";
#ifdef BUILD_TAG
    static const char buildtag[] = BOOST_PP_STRINGIZE(BUILD_TAG);
    static const char subdir[] = "cli";
#else
    static const char buildtag[] = "source";
    static const char subdir[] = "source";
#endif

    if (req.command != "check" && req.command != "download" && req.command != "update")
    {
      res.status = std::string("unknown command: '") + req.command + "'";
      return true;
    }

    std::string version, hash;
    if (!tools::check_updates(software, buildtag, version, hash))
    {
      res.status = "Error checking for updates";
      return true;
    }
    if (tools::vercmp(version.c_str(), MONERO_VERSION) <= 0)
    {
      res.update = false;
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }
    res.update = true;
    res.version = version;
    res.user_uri = tools::get_update_url(software, subdir, buildtag, version, true);
    res.auto_uri = tools::get_update_url(software, subdir, buildtag, version, false);
    res.hash = hash;
    if (req.command == "check")
    {
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }

    boost::filesystem::path path;
    if (req.path.empty())
    {
      std::string filename;
      const char *slash = strrchr(res.auto_uri.c_str(), '/');
      if (slash)
        filename = slash + 1;
      else
        filename = std::string(software) + "-update-" + version;
      path = epee::string_tools::get_current_module_folder();
      path /= filename;
    }
    else
    {
      path = req.path;
    }

    crypto::hash file_hash;
    if (!tools::sha256sum(path.string(), file_hash) || (hash != epee::string_tools::pod_to_hex(file_hash)))
    {
      MDEBUG("We don't have that file already, downloading");
      if (!tools::download(path.string(), res.auto_uri))
      {
        MERROR("Failed to download " << res.auto_uri);
        return false;
      }
      if (!tools::sha256sum(path.string(), file_hash))
      {
        MERROR("Failed to hash " << path);
        return false;
      }
      if (hash != epee::string_tools::pod_to_hex(file_hash))
      {
        MERROR("Download from " << res.auto_uri << " does not match the expected hash");
        return false;
      }
      MINFO("New version downloaded to " << path);
    }
    else
    {
      MDEBUG("We already have " << path << " with expected hash");
    }
    res.path = path.string();

    if (req.command == "download")
    {
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }

    res.status = "'update' not implemented yet";
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_relay_tx(const COMMAND_RPC_RELAY_TX::request& req, COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_relay_tx);

    bool failed = false;
    res.status = "";
    for (const auto &str: req.txids)
    {
      cryptonote::blobdata txid_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(str, txid_data))
      {
        if (!res.status.empty()) res.status += ", ";
        res.status += std::string("invalid transaction id: ") + str;
        failed = true;
        continue;
      }
      crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

      cryptonote::blobdata txblob;
      bool r = m_core.get_pool_transaction(txid, txblob);
      if (r)
      {
        cryptonote_connection_context fake_context = AUTO_VAL_INIT(fake_context);
        NOTIFY_NEW_TRANSACTIONS::request r;
        r.txs.push_back(txblob);
        m_core.get_protocol()->relay_transactions(r, fake_context);
        //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
      }
      else
      {
        if (!res.status.empty()) res.status += ", ";
        res.status += std::string("transaction not found in pool: ") + str;
        failed = true;
        continue;
      }
    }

    if (failed)
    {
      return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_sync_info(const COMMAND_RPC_SYNC_INFO::request& req, COMMAND_RPC_SYNC_INFO::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_sync_info);

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height; // turn top block height into blockchain height
    res.target_height = m_core.get_target_blockchain_height();

    for (const auto &c: m_p2p.get_payload_object().get_connections())
      res.peers.push_back({c});
    const cryptonote::block_queue &block_queue = m_p2p.get_payload_object().get_block_queue();
    block_queue.foreach([&](const cryptonote::block_queue::span &span) {
      const std::string span_connection_id = epee::string_tools::pod_to_hex(span.connection_id);
      uint32_t speed = (uint32_t)(100.0f * block_queue.get_speed(span.connection_id) + 0.5f);
      std::string address = "";
      for (const auto &c: m_p2p.get_payload_object().get_connections())
        if (c.connection_id == span_connection_id)
          address = c.address;
      res.spans.push_back({span.start_block_height, span.nblocks, span_connection_id, (uint32_t)(span.rate + 0.5f), speed, span.size, address});
      return true;
    });

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_txpool_backlog(const COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_txpool_backlog);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG>(invoke_http_mode::JON_RPC, "get_txpool_backlog", req, res, r))
      return r;

    if (!m_core.get_txpool_backlog(res.backlog))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Failed to get txpool backlog";
      return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_output_distribution(const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request& req, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response& res, epee::json_rpc::error& error_resp)
  {
    PERF_TIMER(on_get_output_distribution);
    try
    {
      for (uint64_t amount: req.amounts)
      {
        static struct D
        {
          boost::mutex mutex;
          std::vector<uint64_t> cached_distribution;
          uint64_t cached_from, cached_to, cached_start_height, cached_base;
          bool cached;
          D(): cached_from(0), cached_to(0), cached_start_height(0), cached_base(0), cached(false) {}
        } d;
        boost::unique_lock<boost::mutex> lock(d.mutex);

        if (d.cached && amount == 0 && d.cached_from == req.from_height && d.cached_to == req.to_height)
        {
          res.distributions.push_back({amount, d.cached_start_height, d.cached_distribution, d.cached_base});
          if (req.cumulative)
          {
            auto &distribution = res.distributions.back().distribution;
            distribution[0] += d.cached_base;
            for (size_t n = 1; n < distribution.size(); ++n)
              distribution[n] += distribution[n-1];
          }
          continue;
        }

        // this is a slow operation, so we have precomputed caches of common cases
        bool found = false;
        for (const auto &slot: get_output_distribution_cache)
        {
          if (slot.amount == amount && slot.from_height == req.from_height && slot.to_height == req.to_height)
          {
            res.distributions.push_back({amount, slot.start_height, slot.distribution, slot.base});
            found = true;
            if (req.cumulative)
            {
              auto &distribution = res.distributions.back().distribution;
              distribution[0] += slot.base;
              for (size_t n = 1; n < distribution.size(); ++n)
                distribution[n] += distribution[n-1];
            }
            break;
          }
        }
        if (found)
          continue;

        std::vector<uint64_t> distribution;
        uint64_t start_height, base;
        if (!m_core.get_output_distribution(amount, req.from_height, req.to_height, start_height, distribution, base))
        {
          error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
          error_resp.message = "Failed to get rct distribution";
          return false;
        }
        if (req.to_height > 0 && req.to_height >= req.from_height)
        {
          uint64_t offset = std::max(req.from_height, start_height);
          if (offset <= req.to_height && req.to_height - offset + 1 < distribution.size())
            distribution.resize(req.to_height - offset + 1);
        }

        if (amount == 0)
        {
          d.cached_from = req.from_height;
          d.cached_to = req.to_height;
          d.cached_distribution = distribution;
          d.cached_start_height = start_height;
          d.cached_base = base;
          d.cached = true;
        }

        if (req.cumulative)
        {
          distribution[0] += base;
          for (size_t n = 1; n < distribution.size(); ++n)
            distribution[n] += distribution[n-1];
        }

        res.distributions.push_back({amount, start_height, std::move(distribution), base});
      }
    }
    catch (const std::exception &e)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Failed to get output distribution";
      return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------


  const command_line::arg_descriptor<std::string, false, true, 2> core_rpc_server::arg_rpc_bind_port = {
      "rpc-bind-port"
    , "Port for RPC server"
    , std::to_string(config::RPC_DEFAULT_PORT)
    , {{ &cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on }}
    , [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
        if (testnet_stagenet[0] && defaulted)
          return std::to_string(config::testnet::RPC_DEFAULT_PORT);
        else if (testnet_stagenet[1] && defaulted)
          return std::to_string(config::stagenet::RPC_DEFAULT_PORT);
        return val;
      }
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_rpc_restricted_bind_port = {
      "rpc-restricted-bind-port"
    , "Port for restricted RPC server"
    , ""
    };

  const command_line::arg_descriptor<bool> core_rpc_server::arg_restricted_rpc = {
      "restricted-rpc"
    , "Restrict RPC to view only commands and do not return privacy sensitive data in RPC calls"
    , false
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_bootstrap_daemon_address = {
      "bootstrap-daemon-address"
    , "URL of a 'bootstrap' remote daemon that the connected wallets can use while this daemon is still not fully synced"
    , ""
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_bootstrap_daemon_login = {
      "bootstrap-daemon-login"
    , "Specify username:password for the bootstrap daemon login"
    , ""
    };
}  // namespace cryptonote
