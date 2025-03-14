// Copyright (c) 2014-2024, The Monero Project
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

#pragma  once 

#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "bootstrap_daemon.h"
#include "net/http_server_impl_base.h"
#include "net/http_client.h"
#include "core_rpc_server_commands_defs.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "rpc_payment.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc"

namespace cryptonote
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class core_rpc_server: public epee::http_server_impl_base<core_rpc_server>
  {
  public:

    static const command_line::arg_descriptor<std::string, false, true, 2> arg_rpc_bind_port;
    static const command_line::arg_descriptor<std::string> arg_rpc_restricted_bind_port;
    static const command_line::arg_descriptor<bool> arg_restricted_rpc;
    static const command_line::arg_descriptor<std::string> arg_rpc_ssl;
    static const command_line::arg_descriptor<std::string> arg_rpc_ssl_private_key;
    static const command_line::arg_descriptor<std::string> arg_rpc_ssl_certificate;
    static const command_line::arg_descriptor<std::string> arg_rpc_ssl_ca_certificates;
    static const command_line::arg_descriptor<std::vector<std::string>> arg_rpc_ssl_allowed_fingerprints;
    static const command_line::arg_descriptor<bool> arg_rpc_ssl_allow_any_cert;
    static const command_line::arg_descriptor<std::string> arg_bootstrap_daemon_address;
    static const command_line::arg_descriptor<std::string> arg_bootstrap_daemon_login;
    static const command_line::arg_descriptor<std::string> arg_bootstrap_daemon_proxy;
    static const command_line::arg_descriptor<std::string> arg_rpc_payment_address;
    static const command_line::arg_descriptor<uint64_t> arg_rpc_payment_difficulty;
    static const command_line::arg_descriptor<uint64_t> arg_rpc_payment_credits;
    static const command_line::arg_descriptor<bool> arg_rpc_payment_allow_free_loopback;
    static const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections_per_public_ip;
    static const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections_per_private_ip;
    static const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections;
    static const command_line::arg_descriptor<std::size_t> arg_rpc_response_soft_limit;

    typedef epee::net_utils::connection_context_base connection_context;

    core_rpc_server(
        core& cr
      , nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& p2p
      );
    ~core_rpc_server();

    static void init_options(boost::program_options::options_description& desc);
    bool init(
        const boost::program_options::variables_map& vm,
        const bool restricted,
        const std::string& port,
        bool allow_rpc_payment,
        const std::string& proxy = {}
      );
    network_type nettype() const { return m_core.get_nettype(); }

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      MAP_URI_AUTO_JON2("/get_height", on_get_height, COMMAND_RPC_GET_HEIGHT)
      MAP_URI_AUTO_JON2("/getheight", on_get_height, COMMAND_RPC_GET_HEIGHT)
      MAP_URI_AUTO_BIN2("/get_blocks.bin", on_get_blocks, COMMAND_RPC_GET_BLOCKS_FAST)
      MAP_URI_AUTO_BIN2("/getblocks.bin", on_get_blocks, COMMAND_RPC_GET_BLOCKS_FAST)
      MAP_URI_AUTO_BIN2("/get_blocks_by_height.bin", on_get_blocks_by_height, COMMAND_RPC_GET_BLOCKS_BY_HEIGHT)
      MAP_URI_AUTO_BIN2("/getblocks_by_height.bin", on_get_blocks_by_height, COMMAND_RPC_GET_BLOCKS_BY_HEIGHT)
      MAP_URI_AUTO_BIN2("/get_hashes.bin", on_get_hashes, COMMAND_RPC_GET_HASHES_FAST)
      MAP_URI_AUTO_BIN2("/gethashes.bin", on_get_hashes, COMMAND_RPC_GET_HASHES_FAST)
      MAP_URI_AUTO_BIN2("/get_o_indexes.bin", on_get_indexes, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES)      
      MAP_URI_AUTO_BIN2("/get_outs.bin", on_get_outs_bin, COMMAND_RPC_GET_OUTPUTS_BIN)
      MAP_URI_AUTO_BIN2("/get_path_by_amount_output_id.bin", on_get_path_by_amount_output_id_bin, COMMAND_RPC_GET_PATH_BY_AMOUNT_OUTPUT_ID_BIN)
      MAP_URI_AUTO_JON2("/get_transactions", on_get_transactions, COMMAND_RPC_GET_TRANSACTIONS)
      MAP_URI_AUTO_JON2("/gettransactions", on_get_transactions, COMMAND_RPC_GET_TRANSACTIONS)
      MAP_URI_AUTO_JON2("/get_alt_blocks_hashes", on_get_alt_blocks_hashes, COMMAND_RPC_GET_ALT_BLOCKS_HASHES)
      MAP_URI_AUTO_JON2("/is_key_image_spent", on_is_key_image_spent, COMMAND_RPC_IS_KEY_IMAGE_SPENT)
      MAP_URI_AUTO_JON2("/send_raw_transaction", on_send_raw_tx, COMMAND_RPC_SEND_RAW_TX)
      MAP_URI_AUTO_JON2("/sendrawtransaction", on_send_raw_tx, COMMAND_RPC_SEND_RAW_TX)
      MAP_URI_AUTO_JON2_IF("/start_mining", on_start_mining, COMMAND_RPC_START_MINING, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/stop_mining", on_stop_mining, COMMAND_RPC_STOP_MINING, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/mining_status", on_mining_status, COMMAND_RPC_MINING_STATUS, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/save_bc", on_save_bc, COMMAND_RPC_SAVE_BC, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/get_peer_list", on_get_peer_list, COMMAND_RPC_GET_PEER_LIST, !m_restricted)
      MAP_URI_AUTO_JON2("/get_public_nodes", on_get_public_nodes, COMMAND_RPC_GET_PUBLIC_NODES)
      MAP_URI_AUTO_JON2_IF("/set_log_hash_rate", on_set_log_hash_rate, COMMAND_RPC_SET_LOG_HASH_RATE, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/set_log_level", on_set_log_level, COMMAND_RPC_SET_LOG_LEVEL, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/set_log_categories", on_set_log_categories, COMMAND_RPC_SET_LOG_CATEGORIES, !m_restricted)
      MAP_URI_AUTO_JON2("/get_transaction_pool", on_get_transaction_pool, COMMAND_RPC_GET_TRANSACTION_POOL)
      MAP_URI_AUTO_JON2("/get_transaction_pool_hashes.bin", on_get_transaction_pool_hashes_bin, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN)
      MAP_URI_AUTO_JON2("/get_transaction_pool_hashes", on_get_transaction_pool_hashes, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES)
      MAP_URI_AUTO_JON2("/get_transaction_pool_stats", on_get_transaction_pool_stats, COMMAND_RPC_GET_TRANSACTION_POOL_STATS)
      MAP_URI_AUTO_JON2_IF("/set_bootstrap_daemon", on_set_bootstrap_daemon, COMMAND_RPC_SET_BOOTSTRAP_DAEMON, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/stop_daemon", on_stop_daemon, COMMAND_RPC_STOP_DAEMON, !m_restricted)
      MAP_URI_AUTO_JON2("/get_info", on_get_info, COMMAND_RPC_GET_INFO)
      MAP_URI_AUTO_JON2("/getinfo", on_get_info, COMMAND_RPC_GET_INFO)
      MAP_URI_AUTO_JON2_IF("/get_net_stats", on_get_net_stats, COMMAND_RPC_GET_NET_STATS, !m_restricted)
      MAP_URI_AUTO_JON2("/get_limit", on_get_limit, COMMAND_RPC_GET_LIMIT)
      MAP_URI_AUTO_JON2_IF("/set_limit", on_set_limit, COMMAND_RPC_SET_LIMIT, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/out_peers", on_out_peers, COMMAND_RPC_OUT_PEERS, !m_restricted)
      MAP_URI_AUTO_JON2_IF("/in_peers", on_in_peers, COMMAND_RPC_IN_PEERS, !m_restricted)
      MAP_URI_AUTO_JON2("/get_outs", on_get_outs, COMMAND_RPC_GET_OUTPUTS)      
      MAP_URI_AUTO_JON2_IF("/update", on_update, COMMAND_RPC_UPDATE, !m_restricted)
      MAP_URI_AUTO_BIN2("/get_output_distribution.bin", on_get_output_distribution_bin, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION)
      MAP_URI_AUTO_JON2_IF("/pop_blocks", on_pop_blocks, COMMAND_RPC_POP_BLOCKS, !m_restricted)
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC("get_block_count",           on_getblockcount,              COMMAND_RPC_GETBLOCKCOUNT)
        MAP_JON_RPC("getblockcount",             on_getblockcount,              COMMAND_RPC_GETBLOCKCOUNT)
        MAP_JON_RPC_WE("on_get_block_hash",      on_getblockhash,               COMMAND_RPC_GETBLOCKHASH)
        MAP_JON_RPC_WE("on_getblockhash",        on_getblockhash,               COMMAND_RPC_GETBLOCKHASH)
        MAP_JON_RPC_WE("get_block_template",     on_getblocktemplate,           COMMAND_RPC_GETBLOCKTEMPLATE)
        MAP_JON_RPC_WE("getblocktemplate",       on_getblocktemplate,           COMMAND_RPC_GETBLOCKTEMPLATE)
        MAP_JON_RPC_WE("get_miner_data",         on_getminerdata,               COMMAND_RPC_GETMINERDATA)
        MAP_JON_RPC_WE_IF("calc_pow",            on_calcpow,                    COMMAND_RPC_CALCPOW, !m_restricted)
        MAP_JON_RPC_WE("add_aux_pow",            on_add_aux_pow,                COMMAND_RPC_ADD_AUX_POW)
        MAP_JON_RPC_WE("submit_block",           on_submitblock,                COMMAND_RPC_SUBMITBLOCK)
        MAP_JON_RPC_WE("submitblock",            on_submitblock,                COMMAND_RPC_SUBMITBLOCK)
        MAP_JON_RPC_WE_IF("generateblocks",         on_generateblocks,             COMMAND_RPC_GENERATEBLOCKS, !m_restricted)
        MAP_JON_RPC_WE("get_last_block_header",  on_get_last_block_header,      COMMAND_RPC_GET_LAST_BLOCK_HEADER)
        MAP_JON_RPC_WE("getlastblockheader",     on_get_last_block_header,      COMMAND_RPC_GET_LAST_BLOCK_HEADER)
        MAP_JON_RPC_WE("get_block_header_by_hash", on_get_block_header_by_hash,   COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH)
        MAP_JON_RPC_WE("getblockheaderbyhash",   on_get_block_header_by_hash,   COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH)
        MAP_JON_RPC_WE("get_block_header_by_height", on_get_block_header_by_height, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT)
        MAP_JON_RPC_WE("getblockheaderbyheight", on_get_block_header_by_height, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT)
        MAP_JON_RPC_WE("get_block_headers_range", on_get_block_headers_range,    COMMAND_RPC_GET_BLOCK_HEADERS_RANGE)
        MAP_JON_RPC_WE("getblockheadersrange",   on_get_block_headers_range,    COMMAND_RPC_GET_BLOCK_HEADERS_RANGE)
        MAP_JON_RPC_WE("get_block",              on_get_block,                 COMMAND_RPC_GET_BLOCK)
        MAP_JON_RPC_WE("getblock",                on_get_block,                 COMMAND_RPC_GET_BLOCK)
        MAP_JON_RPC_WE_IF("get_connections",     on_get_connections,            COMMAND_RPC_GET_CONNECTIONS, !m_restricted)
        MAP_JON_RPC_WE("get_info",               on_get_info_json,              COMMAND_RPC_GET_INFO)
        MAP_JON_RPC_WE("hard_fork_info",         on_hard_fork_info,             COMMAND_RPC_HARD_FORK_INFO)
        MAP_JON_RPC_WE_IF("set_bans",            on_set_bans,                   COMMAND_RPC_SETBANS, !m_restricted)
        MAP_JON_RPC_WE_IF("get_bans",            on_get_bans,                   COMMAND_RPC_GETBANS, !m_restricted)
        MAP_JON_RPC_WE_IF("banned",              on_banned,                     COMMAND_RPC_BANNED, !m_restricted)
        MAP_JON_RPC_WE_IF("flush_txpool",        on_flush_txpool,               COMMAND_RPC_FLUSH_TRANSACTION_POOL, !m_restricted)
        MAP_JON_RPC_WE("get_output_histogram",   on_get_output_histogram,       COMMAND_RPC_GET_OUTPUT_HISTOGRAM)
        MAP_JON_RPC_WE("get_version",            on_get_version,                COMMAND_RPC_GET_VERSION)
        MAP_JON_RPC_WE_IF("get_coinbase_tx_sum", on_get_coinbase_tx_sum,        COMMAND_RPC_GET_COINBASE_TX_SUM, !m_restricted)
        MAP_JON_RPC_WE("get_fee_estimate",       on_get_base_fee_estimate,      COMMAND_RPC_GET_BASE_FEE_ESTIMATE)
        MAP_JON_RPC_WE_IF("get_alternate_chains",on_get_alternate_chains,       COMMAND_RPC_GET_ALTERNATE_CHAINS, !m_restricted)
        MAP_JON_RPC_WE_IF("relay_tx",            on_relay_tx,                   COMMAND_RPC_RELAY_TX, !m_restricted)
        MAP_JON_RPC_WE_IF("sync_info",           on_sync_info,                  COMMAND_RPC_SYNC_INFO, !m_restricted)
        MAP_JON_RPC_WE("get_txpool_backlog",     on_get_txpool_backlog,         COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG)
        MAP_JON_RPC_WE("get_output_distribution", on_get_output_distribution, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION)
        MAP_JON_RPC_WE_IF("prune_blockchain",    on_prune_blockchain,           COMMAND_RPC_PRUNE_BLOCKCHAIN, !m_restricted)
        MAP_JON_RPC_WE_IF("flush_cache",         on_flush_cache,                COMMAND_RPC_FLUSH_CACHE, !m_restricted)
        MAP_JON_RPC_WE("get_txids_loose",        on_get_txids_loose,            COMMAND_RPC_GET_TXIDS_LOOSE)
        MAP_JON_RPC_WE("rpc_access_info",        on_rpc_access_info,            COMMAND_RPC_ACCESS_INFO)
        MAP_JON_RPC_WE("rpc_access_submit_nonce",on_rpc_access_submit_nonce,    COMMAND_RPC_ACCESS_SUBMIT_NONCE)
        MAP_JON_RPC_WE("rpc_access_pay",         on_rpc_access_pay,             COMMAND_RPC_ACCESS_PAY)
        MAP_JON_RPC_WE_IF("rpc_access_tracking", on_rpc_access_tracking,        COMMAND_RPC_ACCESS_TRACKING, !m_restricted)
        MAP_JON_RPC_WE_IF("rpc_access_data",     on_rpc_access_data,            COMMAND_RPC_ACCESS_DATA, !m_restricted)
        MAP_JON_RPC_WE_IF("rpc_access_account",  on_rpc_access_account,         COMMAND_RPC_ACCESS_ACCOUNT, !m_restricted)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

    bool on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, const connection_context *ctx = NULL);
    bool on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, const connection_context *ctx = NULL);
    bool on_get_alt_blocks_hashes(const COMMAND_RPC_GET_ALT_BLOCKS_HASHES::request& req, COMMAND_RPC_GET_ALT_BLOCKS_HASHES::response& res, const connection_context *ctx = NULL);
    bool on_get_blocks_by_height(const COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response& res, const connection_context *ctx = NULL);
    bool on_get_hashes(const COMMAND_RPC_GET_HASHES_FAST::request& req, COMMAND_RPC_GET_HASHES_FAST::response& res, const connection_context *ctx = NULL);
    bool on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, const connection_context *ctx = NULL);
    bool on_is_key_image_spent(const COMMAND_RPC_IS_KEY_IMAGE_SPENT::request& req, COMMAND_RPC_IS_KEY_IMAGE_SPENT::response& res, const connection_context *ctx = NULL);
    bool on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, const connection_context *ctx = NULL);
    bool on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, const connection_context *ctx = NULL);
    bool on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, const connection_context *ctx = NULL);
    bool on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, const connection_context *ctx = NULL);
    bool on_mining_status(const COMMAND_RPC_MINING_STATUS::request& req, COMMAND_RPC_MINING_STATUS::response& res, const connection_context *ctx = NULL);
    bool on_get_outs_bin(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res, const connection_context *ctx = NULL);
    bool on_get_outs(const COMMAND_RPC_GET_OUTPUTS::request& req, COMMAND_RPC_GET_OUTPUTS::response& res, const connection_context *ctx = NULL);
    bool on_get_path_by_amount_output_id_bin(const COMMAND_RPC_GET_PATH_BY_AMOUNT_OUTPUT_ID_BIN::request& req, COMMAND_RPC_GET_PATH_BY_AMOUNT_OUTPUT_ID_BIN::response& res, const connection_context *ctx = NULL);
    bool on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, const connection_context *ctx = NULL);
    bool on_get_net_stats(const COMMAND_RPC_GET_NET_STATS::request& req, COMMAND_RPC_GET_NET_STATS::response& res, const connection_context *ctx = NULL);
    bool on_save_bc(const COMMAND_RPC_SAVE_BC::request& req, COMMAND_RPC_SAVE_BC::response& res, const connection_context *ctx = NULL);
    bool on_get_peer_list(const COMMAND_RPC_GET_PEER_LIST::request& req, COMMAND_RPC_GET_PEER_LIST::response& res, const connection_context *ctx = NULL);
    bool on_get_public_nodes(const COMMAND_RPC_GET_PUBLIC_NODES::request& req, COMMAND_RPC_GET_PUBLIC_NODES::response& res, const connection_context *ctx = NULL);
    bool on_set_log_hash_rate(const COMMAND_RPC_SET_LOG_HASH_RATE::request& req, COMMAND_RPC_SET_LOG_HASH_RATE::response& res, const connection_context *ctx = NULL);
    bool on_set_log_level(const COMMAND_RPC_SET_LOG_LEVEL::request& req, COMMAND_RPC_SET_LOG_LEVEL::response& res, const connection_context *ctx = NULL);
    bool on_set_log_categories(const COMMAND_RPC_SET_LOG_CATEGORIES::request& req, COMMAND_RPC_SET_LOG_CATEGORIES::response& res, const connection_context *ctx = NULL);
    bool on_get_transaction_pool(const COMMAND_RPC_GET_TRANSACTION_POOL::request& req, COMMAND_RPC_GET_TRANSACTION_POOL::response& res, const connection_context *ctx = NULL);
    bool on_get_transaction_pool_hashes_bin(const COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::response& res, const connection_context *ctx = NULL);
    bool on_get_transaction_pool_hashes(const COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response& res, const connection_context *ctx = NULL);
    bool on_get_transaction_pool_stats(const COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response& res, const connection_context *ctx = NULL);
    bool on_set_bootstrap_daemon(const COMMAND_RPC_SET_BOOTSTRAP_DAEMON::request& req, COMMAND_RPC_SET_BOOTSTRAP_DAEMON::response& res, const connection_context *ctx = NULL);
    bool on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res, const connection_context *ctx = NULL);
    bool on_get_limit(const COMMAND_RPC_GET_LIMIT::request& req, COMMAND_RPC_GET_LIMIT::response& res, const connection_context *ctx = NULL);
    bool on_set_limit(const COMMAND_RPC_SET_LIMIT::request& req, COMMAND_RPC_SET_LIMIT::response& res, const connection_context *ctx = NULL);
    bool on_out_peers(const COMMAND_RPC_OUT_PEERS::request& req, COMMAND_RPC_OUT_PEERS::response& res, const connection_context *ctx = NULL);
    bool on_in_peers(const COMMAND_RPC_IN_PEERS::request& req, COMMAND_RPC_IN_PEERS::response& res, const connection_context *ctx = NULL);
    bool on_update(const COMMAND_RPC_UPDATE::request& req, COMMAND_RPC_UPDATE::response& res, const connection_context *ctx = NULL);
    bool on_get_output_distribution_bin(const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request& req, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response& res, const connection_context *ctx = NULL);
    bool on_pop_blocks(const COMMAND_RPC_POP_BLOCKS::request& req, COMMAND_RPC_POP_BLOCKS::response& res, const connection_context *ctx = NULL);
    
    //json_rpc
    bool on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, const connection_context *ctx = NULL);
    bool on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_getminerdata(const COMMAND_RPC_GETMINERDATA::request& req, COMMAND_RPC_GETMINERDATA::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_calcpow(const COMMAND_RPC_CALCPOW::request& req, COMMAND_RPC_CALCPOW::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_add_aux_pow(const COMMAND_RPC_ADD_AUX_POW::request& req, COMMAND_RPC_ADD_AUX_POW::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_generateblocks(const COMMAND_RPC_GENERATEBLOCKS::request& req, COMMAND_RPC_GENERATEBLOCKS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_block_headers_range(const COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request& req, COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_block(const COMMAND_RPC_GET_BLOCK::request& req, COMMAND_RPC_GET_BLOCK::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_connections(const COMMAND_RPC_GET_CONNECTIONS::request& req, COMMAND_RPC_GET_CONNECTIONS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_info_json(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_hard_fork_info(const COMMAND_RPC_HARD_FORK_INFO::request& req, COMMAND_RPC_HARD_FORK_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_set_bans(const COMMAND_RPC_SETBANS::request& req, COMMAND_RPC_SETBANS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_bans(const COMMAND_RPC_GETBANS::request& req, COMMAND_RPC_GETBANS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_banned(const COMMAND_RPC_BANNED::request& req, COMMAND_RPC_BANNED::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_flush_txpool(const COMMAND_RPC_FLUSH_TRANSACTION_POOL::request& req, COMMAND_RPC_FLUSH_TRANSACTION_POOL::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_output_histogram(const COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request& req, COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_version(const COMMAND_RPC_GET_VERSION::request& req, COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_coinbase_tx_sum(const COMMAND_RPC_GET_COINBASE_TX_SUM::request& req, COMMAND_RPC_GET_COINBASE_TX_SUM::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_base_fee_estimate(const COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request& req, COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_alternate_chains(const COMMAND_RPC_GET_ALTERNATE_CHAINS::request& req, COMMAND_RPC_GET_ALTERNATE_CHAINS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_relay_tx(const COMMAND_RPC_RELAY_TX::request& req, COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_sync_info(const COMMAND_RPC_SYNC_INFO::request& req, COMMAND_RPC_SYNC_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_txpool_backlog(const COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_output_distribution(const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request& req, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_prune_blockchain(const COMMAND_RPC_PRUNE_BLOCKCHAIN::request& req, COMMAND_RPC_PRUNE_BLOCKCHAIN::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_flush_cache(const COMMAND_RPC_FLUSH_CACHE::request& req, COMMAND_RPC_FLUSH_CACHE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_get_txids_loose(const COMMAND_RPC_GET_TXIDS_LOOSE::request& req, COMMAND_RPC_GET_TXIDS_LOOSE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_info(const COMMAND_RPC_ACCESS_INFO::request& req, COMMAND_RPC_ACCESS_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_submit_nonce(const COMMAND_RPC_ACCESS_SUBMIT_NONCE::request& req, COMMAND_RPC_ACCESS_SUBMIT_NONCE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_pay(const COMMAND_RPC_ACCESS_PAY::request& req, COMMAND_RPC_ACCESS_PAY::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_tracking(const COMMAND_RPC_ACCESS_TRACKING::request& req, COMMAND_RPC_ACCESS_TRACKING::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_data(const COMMAND_RPC_ACCESS_DATA::request& req, COMMAND_RPC_ACCESS_DATA::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    bool on_rpc_access_account(const COMMAND_RPC_ACCESS_ACCOUNT::request& req, COMMAND_RPC_ACCESS_ACCOUNT::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx = NULL);
    //-----------------------

private:
    bool check_core_busy();
    bool check_core_ready();
    bool add_host_fail(const connection_context *ctx, unsigned int score = 1);
    
    //utils
    uint64_t get_block_reward(const block& blk);
    bool fill_block_header_response(const block& blk, bool orphan_status, uint64_t height, const crypto::hash& hash, block_header_response& response, bool fill_pow_hash);
    std::map<std::string, bool> get_public_nodes(uint32_t credits_per_hash_threshold = 0);
    bool set_bootstrap_daemon(
      const std::string &address,
      const std::string &username_password,
      const std::string &proxy);
    bool set_bootstrap_daemon(
      const std::string &address,
      const boost::optional<epee::net_utils::http::login> &credentials,
      const std::string &proxy);
    enum invoke_http_mode { JON, BIN, JON_RPC };
    template <typename COMMAND_TYPE>
    bool use_bootstrap_daemon_if_necessary(const invoke_http_mode &mode, const std::string &command_name, const typename COMMAND_TYPE::request& req, typename COMMAND_TYPE::response& res, bool &r);
    bool get_block_template(const account_public_address &address, const crypto::hash *prev_block, const cryptonote::blobdata &extra_nonce, size_t &reserved_offset, cryptonote::difficulty_type &difficulty, uint64_t &height, uint64_t &expected_reward, uint64_t& cumulative_weight, block &b, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, epee::json_rpc::error &error_resp);
    bool check_payment(const std::string &client, uint64_t payment, const std::string &rpc, bool same_ts, std::string &message, uint64_t &credits, std::string &top_hash);
    
    core& m_core;
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& m_p2p;
    boost::shared_mutex m_bootstrap_daemon_mutex;
    std::unique_ptr<bootstrap_daemon> m_bootstrap_daemon;
    std::string m_bootstrap_daemon_proxy;
    bool m_should_use_bootstrap_daemon;
    std::chrono::system_clock::time_point m_bootstrap_height_check_time;
    bool m_was_bootstrap_ever_used;
    bool m_restricted;
    epee::critical_section m_host_fails_score_lock;
    std::map<std::string, uint64_t> m_host_fails_score;
    std::unique_ptr<rpc_payment> m_rpc_payment;
    bool disable_rpc_ban;
    bool m_rpc_payment_allow_free_loopback;
  };
}

BOOST_CLASS_VERSION(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >, 1);
