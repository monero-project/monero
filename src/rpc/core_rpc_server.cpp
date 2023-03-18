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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/preprocessor/stringize.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/filesystem.hpp>
#include "include_base_utils.h"
#include "string_tools.h"
using namespace epee;

#include "core_rpc_server.h"
#include "common/command_line.h"
#include "common/updates.h"
#include "common/download.h"
#include "common/util.h"
#include "common/perf_timer.h"
#include "int-util.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/merge_mining.h"
#include "cryptonote_core/tx_sanity_check.h"
#include "misc_language.h"
#include "net/local_ip.h"
#include "net/parse.h"
#include "storages/http_abstract_invoke.h"
#include "crypto/hash.h"
#include "rpc/rpc_args.h"
#include "rpc/rpc_handler.h"
#include "rpc/rpc_payment_costs.h"
#include "rpc/rpc_payment_signature.h"
#include "core_rpc_server_error_codes.h"
#include "p2p/net_node.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc"

#define MAX_RESTRICTED_FAKE_OUTS_COUNT 40
#define MAX_RESTRICTED_GLOBAL_FAKE_OUTS_COUNT 5000

#define OUTPUT_HISTOGRAM_RECENT_CUTOFF_RESTRICTION (3 * 86400) // 3 days max, the wallet requests 1.8 days

#define DEFAULT_PAYMENT_DIFFICULTY 1000
#define DEFAULT_PAYMENT_CREDITS_PER_HASH 100

#define RESTRICTED_BLOCK_HEADER_RANGE 1000
#define RESTRICTED_TRANSACTIONS_COUNT 100
#define RESTRICTED_SPENT_KEY_IMAGES_COUNT 5000
#define RESTRICTED_BLOCK_COUNT 1000

#define RPC_TRACKER(rpc) \
  PERF_TIMER(rpc); \
  RPCTracker tracker(#rpc, PERF_TIMER_NAME(rpc))

namespace
{
  class RPCTracker
  {
  public:
    struct entry_t
    {
      uint64_t count;
      uint64_t time;
      uint64_t credits;
    };

    RPCTracker(const char *rpc, tools::LoggingPerformanceTimer &timer): rpc(rpc), timer(timer) {
    }
    ~RPCTracker() {
      try
      {
        boost::unique_lock<boost::mutex> lock(mutex);
        auto &e = tracker[rpc];
        ++e.count;
        e.time += timer.value();
      }
      catch (...) { /* ignore */ }
    }
    void pay(uint64_t amount) {
      boost::unique_lock<boost::mutex> lock(mutex);
      auto &e = tracker[rpc];
      e.credits += amount;
    }
    const std::string &rpc_name() const { return rpc; }
    static void clear() { boost::unique_lock<boost::mutex> lock(mutex); tracker.clear(); }
    static std::unordered_map<std::string, entry_t> data() { boost::unique_lock<boost::mutex> lock(mutex); return tracker; }
  private:
    std::string rpc;
    tools::LoggingPerformanceTimer &timer;
    static boost::mutex mutex;
    static std::unordered_map<std::string, entry_t> tracker;
  };
  boost::mutex RPCTracker::mutex;
  std::unordered_map<std::string, RPCTracker::entry_t> RPCTracker::tracker;

  void add_reason(std::string &reasons, const char *reason)
  {
    if (!reasons.empty())
      reasons += ", ";
    reasons += reason;
  }

  uint64_t round_up(uint64_t value, uint64_t quantum)
  {
    return (value + quantum - 1) / quantum * quantum;
  }

  void store_128(boost::multiprecision::uint128_t value, uint64_t &slow64, std::string &swide, uint64_t &stop64)
  {
    slow64 = (value & 0xffffffffffffffff).convert_to<uint64_t>();
    swide = cryptonote::hex(value);
    stop64 = ((value >> 64) & 0xffffffffffffffff).convert_to<uint64_t>();
  }

  void store_difficulty(cryptonote::difficulty_type difficulty, uint64_t &sdiff, std::string &swdiff, uint64_t &stop64)
  {
    store_128(difficulty, sdiff, swdiff, stop64);
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
    command_line::add_arg(desc, arg_bootstrap_daemon_proxy);
    cryptonote::rpc_args::init_options(desc, true);
    command_line::add_arg(desc, arg_rpc_payment_address);
    command_line::add_arg(desc, arg_rpc_payment_difficulty);
    command_line::add_arg(desc, arg_rpc_payment_credits);
    command_line::add_arg(desc, arg_rpc_payment_allow_free_loopback);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  core_rpc_server::core_rpc_server(
      core& cr
    , nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& p2p
    )
    : m_core(cr)
    , m_p2p(p2p)
    , m_was_bootstrap_ever_used(false)
    , disable_rpc_ban(false)
    , m_rpc_payment_allow_free_loopback(false)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::set_bootstrap_daemon(
    const std::string &address,
    const std::string &username_password,
    const std::string &proxy)
  {
    boost::optional<epee::net_utils::http::login> credentials;
    const auto loc = username_password.find(':');
    if (loc != std::string::npos)
    {
      credentials = epee::net_utils::http::login(username_password.substr(0, loc), username_password.substr(loc + 1));
    }
    return set_bootstrap_daemon(address, credentials, proxy);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  std::map<std::string, bool> core_rpc_server::get_public_nodes(uint32_t credits_per_hash_threshold/* = 0*/)
  {
    COMMAND_RPC_GET_PUBLIC_NODES::request request;
    COMMAND_RPC_GET_PUBLIC_NODES::response response;

    request.gray = true;
    request.white = true;
    request.include_blocked = false;
    if (!on_get_public_nodes(request, response) || response.status != CORE_RPC_STATUS_OK)
    {
      return {};
    }

    std::map<std::string, bool> result;

    const auto append = [&result, &credits_per_hash_threshold](const std::vector<public_node> &nodes, bool white) {
      for (const auto &node : nodes)
      {
        const bool rpc_payment_enabled = credits_per_hash_threshold > 0;
        const bool node_rpc_payment_enabled = node.rpc_credits_per_hash > 0;
        if (!node_rpc_payment_enabled ||
            (rpc_payment_enabled && node.rpc_credits_per_hash >= credits_per_hash_threshold))
        {
          result.insert(std::make_pair(node.host + ":" + std::to_string(node.rpc_port), white));
        }
      }
    };

    append(response.white, true);
    append(response.gray, false);

    return result;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::set_bootstrap_daemon(
    const std::string &address,
    const boost::optional<epee::net_utils::http::login> &credentials,
    const std::string &proxy)
  {
    boost::unique_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);

    constexpr const uint32_t credits_per_hash_threshold = 0;
    constexpr const bool rpc_payment_enabled = credits_per_hash_threshold != 0;

    if (address.empty())
    {
      m_bootstrap_daemon.reset(nullptr);
    }
    else if (address == "auto")
    {
      auto get_nodes = [this]() {
        return get_public_nodes(credits_per_hash_threshold);
      };
      m_bootstrap_daemon.reset(new bootstrap_daemon(std::move(get_nodes), rpc_payment_enabled, m_bootstrap_daemon_proxy.empty() ? proxy : m_bootstrap_daemon_proxy));
    }
    else
    {
      m_bootstrap_daemon.reset(new bootstrap_daemon(address, credentials, rpc_payment_enabled, m_bootstrap_daemon_proxy.empty() ? proxy : m_bootstrap_daemon_proxy));
    }

    m_should_use_bootstrap_daemon = m_bootstrap_daemon.get() != nullptr;

    return true;
  }
  core_rpc_server::~core_rpc_server()
  {
    if (m_rpc_payment)
      m_rpc_payment->store();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::init(
      const boost::program_options::variables_map& vm
      , const bool restricted
      , const std::string& port
      , bool allow_rpc_payment
      , const std::string& proxy
    )
  {
    m_bootstrap_daemon_proxy = proxy;
    m_restricted = restricted;
    m_net_server.set_threads_prefix("RPC");
    m_net_server.set_connection_filter(&m_p2p);

    auto rpc_config = cryptonote::rpc_args::process(vm, true);
    if (!rpc_config)
      return false;

    std::string bind_ip_str = rpc_config->bind_ip;
    std::string bind_ipv6_str = rpc_config->bind_ipv6_address;
    if (restricted)
    {
      const auto restricted_rpc_port_arg = cryptonote::core_rpc_server::arg_rpc_restricted_bind_port;
      const bool has_restricted_rpc_port_arg = !command_line::is_arg_defaulted(vm, restricted_rpc_port_arg);
      if (has_restricted_rpc_port_arg && port == command_line::get_arg(vm, restricted_rpc_port_arg))
      {
        bind_ip_str = rpc_config->restricted_bind_ip;
        bind_ipv6_str = rpc_config->restricted_bind_ipv6_address;
      }
    }
    disable_rpc_ban = rpc_config->disable_rpc_ban;
    const std::string data_dir{command_line::get_arg(vm, cryptonote::arg_data_dir)};
    std::string address = command_line::get_arg(vm, arg_rpc_payment_address);
    if (!address.empty() && allow_rpc_payment)
    {
      if (!m_restricted && nettype() != FAKECHAIN)
      {
        MFATAL("RPC payment enabled, but server is not restricted, anyone can adjust their balance to bypass payment");
        return false;
      }
      cryptonote::address_parse_info info;
      if (!get_account_address_from_str(info, nettype(), address))
      {
        MFATAL("Invalid payment address: " << address);
        return false;
      }
      if (info.is_subaddress)
      {
        MFATAL("Payment address may not be a subaddress: " << address);
        return false;
      }
      uint64_t diff = command_line::get_arg(vm, arg_rpc_payment_difficulty);
      uint64_t credits = command_line::get_arg(vm, arg_rpc_payment_credits);
      if (diff == 0 || credits == 0)
      {
        MFATAL("Payments difficulty and/or payments credits are 0, but a payment address was given");
        return false;
      }
      m_rpc_payment_allow_free_loopback = command_line::get_arg(vm, arg_rpc_payment_allow_free_loopback);
      m_rpc_payment.reset(new rpc_payment(info.address, diff, credits));
      m_rpc_payment->load(data_dir);
      m_p2p.set_rpc_credits_per_hash(RPC_CREDITS_PER_HASH_SCALE * (credits / (float)diff));
    }

    if (!m_rpc_payment)
    {
      uint32_t bind_ip;
      bool ok = epee::string_tools::get_ip_int32_from_string(bind_ip, bind_ip_str);
      if (ok & !epee::net_utils::is_ip_loopback(bind_ip))
        MWARNING("The RPC server is accessible from the outside, but no RPC payment was setup. RPC access will be free for all.");
    }

    if (!set_bootstrap_daemon(
          command_line::get_arg(vm, arg_bootstrap_daemon_address),
          command_line::get_arg(vm, arg_bootstrap_daemon_login),
          command_line::get_arg(vm, arg_bootstrap_daemon_proxy)))
    {
      MFATAL("Failed to parse bootstrap daemon address");
      return false;
    }

    boost::optional<epee::net_utils::http::login> http_login{};

    if (rpc_config->login)
      http_login.emplace(std::move(rpc_config->login->username), std::move(rpc_config->login->password).password());

    if (m_rpc_payment)
      m_net_server.add_idle_handler([this](){ return m_rpc_payment->on_idle(); }, 60 * 1000);

    bool store_ssl_key = !restricted && rpc_config->ssl_options && rpc_config->ssl_options.auth.certificate_path.empty();
    const auto ssl_base_path = (boost::filesystem::path{data_dir} / "rpc_ssl").string();
    const bool ssl_cert_file_exists = boost::filesystem::exists(ssl_base_path + ".crt");
    const bool ssl_pkey_file_exists = boost::filesystem::exists(ssl_base_path + ".key");
    if (store_ssl_key)
    {
      // .key files are often given different read permissions as their corresponding .crt files.
      // Consequently, sometimes the .key file wont't get copied, while the .crt file will.
      if (ssl_cert_file_exists != ssl_pkey_file_exists)
      {
        MFATAL("Certificate (.crt) and private key (.key) files must both exist or both not exist at path: " << ssl_base_path);
        return false;
      }
      else if (ssl_cert_file_exists) { // and ssl_pkey_file_exists
        // load key from previous run, password prompted by OpenSSL
        store_ssl_key = false;
        rpc_config->ssl_options.auth =
          epee::net_utils::ssl_authentication_t{ssl_base_path + ".key", ssl_base_path + ".crt"};
      }
    }

    auto rng = [](size_t len, uint8_t *ptr){ return crypto::rand(len, ptr); };
    const bool inited = epee::http_server_impl_base<core_rpc_server, connection_context>::init(
      rng, std::move(port), std::move(bind_ip_str),
      std::move(bind_ipv6_str), std::move(rpc_config->use_ipv6), std::move(rpc_config->require_ipv4),
      std::move(rpc_config->access_control_origins), std::move(http_login), std::move(rpc_config->ssl_options)
    );

    m_net_server.get_config_object().m_max_content_length = MAX_RPC_CONTENT_LENGTH;

    if (store_ssl_key && inited)
    {
      // new keys were generated, store for next run
      const auto error = epee::net_utils::store_ssl_keys(m_net_server.get_ssl_context(), ssl_base_path);
      if (error)
        MFATAL("Failed to store HTTP SSL cert/key for " << (restricted ? "restricted " : "") << "RPC server: " << error.message());
      return !bool(error);
    }
    return inited;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::check_payment(const std::string &client_message, uint64_t payment, const std::string &rpc, bool same_ts, std::string &message, uint64_t &credits, std::string &top_hash)
  {
    if (m_rpc_payment == NULL)
    {
      credits = 0;
      return true;
    }
    uint64_t height;
    crypto::hash hash;
    m_core.get_blockchain_top(height, hash);
    top_hash = epee::string_tools::pod_to_hex(hash);
    crypto::public_key client;
    uint64_t ts;
#ifndef NDEBUG
    if (nettype() == TESTNET && client_message == "debug")
    {
      credits = 0;
      return true;
    }
#endif
    if (!cryptonote::verify_rpc_payment_signature(client_message, client, ts))
    {
      credits = 0;
      message = "Client signature does not verify for " + rpc;
      return false;
    }
    if (!m_rpc_payment->pay(client, ts, payment, rpc, same_ts, credits))
    {
      message = CORE_RPC_STATUS_PAYMENT_REQUIRED;
      return false;
    }
    return true;
  }
#define CHECK_PAYMENT_BASE(req, res, payment, same_ts) do { if (!ctx) break; uint64_t P = (uint64_t)payment; if (P > 0 && !check_payment(req.client, P, tracker.rpc_name(), same_ts, res.status, res.credits, res.top_hash)){return true;} tracker.pay(P); } while(0)
#define CHECK_PAYMENT(req, res, payment) CHECK_PAYMENT_BASE(req, res, payment, false)
#define CHECK_PAYMENT_SAME_TS(req, res, payment) CHECK_PAYMENT_BASE(req, res, payment, true)
#define CHECK_PAYMENT_MIN1(req, res, payment, same_ts) do { if (!ctx || (m_rpc_payment_allow_free_loopback && ctx->m_remote_address.is_loopback())) break; uint64_t P = (uint64_t)payment; if (P == 0) P = 1; if(!check_payment(req.client, P, tracker.rpc_name(), same_ts, res.status, res.credits, res.top_hash)){return true;} tracker.pay(P); } while(0)
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::check_core_ready()
  {
    if(!m_p2p.get_payload_object().is_synchronized())
    {
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::add_host_fail(const connection_context *ctx, unsigned int score)
  {
    if(!ctx || !ctx->m_remote_address.is_blockable() || disable_rpc_ban)
      return false;

    CRITICAL_REGION_LOCAL(m_host_fails_score_lock);
    uint64_t fails = m_host_fails_score[ctx->m_remote_address.host_str()] += score;
    MDEBUG("Host " << ctx->m_remote_address.host_str() << " fail score=" << fails);
    if(fails > RPC_IP_FAILS_BEFORE_BLOCK)
    {
      auto it = m_host_fails_score.find(ctx->m_remote_address.host_str());
      CHECK_AND_ASSERT_MES(it != m_host_fails_score.end(), false, "internal error");
      it->second = RPC_IP_FAILS_BEFORE_BLOCK/2;
      m_p2p.block_host(ctx->m_remote_address);
    }
    return true;
  }
#define CHECK_CORE_READY() do { if(!check_core_ready()){res.status =  CORE_RPC_STATUS_BUSY;return true;} } while(0)

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_HEIGHT>(invoke_http_mode::JON, "/getheight", req, res, r))
      return r;

    crypto::hash hash;
    m_core.get_blockchain_top(res.height, hash);
    ++res.height; // block height to chain height
    res.hash = string_tools::pod_to_hex(hash);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_info);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_INFO>(invoke_http_mode::JON, "/getinfo", req, res, r))
    {
      {
        boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
        if (m_bootstrap_daemon.get() != nullptr)
        {
          res.bootstrap_daemon_address = m_bootstrap_daemon->address();
        }
      }
      crypto::hash top_hash;
      m_core.get_blockchain_top(res.height_without_bootstrap, top_hash);
      ++res.height_without_bootstrap; // turn top block height into blockchain height
      res.was_bootstrap_ever_used = true;
      return r;
    }

    CHECK_PAYMENT_MIN1(req, res, COST_PER_GET_INFO, false);

    const bool restricted = m_restricted && ctx;

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height; // turn top block height into blockchain height
    res.top_block_hash = string_tools::pod_to_hex(top_hash);
    res.target_height = m_p2p.get_payload_object().is_synchronized() ? 0 : m_core.get_target_blockchain_height();
    store_difficulty(m_core.get_blockchain_storage().get_difficulty_for_next_block(), res.difficulty, res.wide_difficulty, res.difficulty_top64);
    res.target = m_core.get_blockchain_storage().get_difficulty_target();
    res.tx_count = m_core.get_blockchain_storage().get_total_transactions() - res.height; //without coinbase
    res.tx_pool_size = m_core.get_pool_transactions_count(!restricted);
    res.alt_blocks_count = restricted ? 0 : m_core.get_blockchain_storage().get_alternative_blocks_count();
    uint64_t total_conn = restricted ? 0 : m_p2p.get_public_connections_count();
    res.outgoing_connections_count = restricted ? 0 : m_p2p.get_public_outgoing_connections_count();
    res.incoming_connections_count = restricted ? 0 : (total_conn - res.outgoing_connections_count);
    res.rpc_connections_count = restricted ? 0 : get_connections_count();
    res.white_peerlist_size = restricted ? 0 : m_p2p.get_public_white_peers_count();
    res.grey_peerlist_size = restricted ? 0 : m_p2p.get_public_gray_peers_count();

    cryptonote::network_type net_type = nettype();
    res.mainnet = net_type == MAINNET;
    res.testnet = net_type == TESTNET;
    res.stagenet = net_type == STAGENET;
    res.nettype = net_type == MAINNET ? "mainnet" : net_type == TESTNET ? "testnet" : net_type == STAGENET ? "stagenet" : "fakechain";
    store_difficulty(m_core.get_blockchain_storage().get_db().get_block_cumulative_difficulty(res.height - 1),
        res.cumulative_difficulty, res.wide_cumulative_difficulty, res.cumulative_difficulty_top64);
    res.block_size_limit = res.block_weight_limit = m_core.get_blockchain_storage().get_current_cumulative_block_weight_limit();
    res.block_size_median = res.block_weight_median = m_core.get_blockchain_storage().get_current_cumulative_block_weight_median();
    res.adjusted_time = m_core.get_blockchain_storage().get_adjusted_time(res.height);

    res.start_time = restricted ? 0 : (uint64_t)m_core.get_start_time();
    res.free_space = restricted ? std::numeric_limits<uint64_t>::max() : m_core.get_free_space();
    res.offline = m_core.offline();
    res.height_without_bootstrap = restricted ? 0 : res.height;
    if (restricted)
    {
      res.bootstrap_daemon_address = "";
      res.was_bootstrap_ever_used = false;
    }
    else
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_bootstrap_daemon.get() != nullptr)
      {
        res.bootstrap_daemon_address = m_bootstrap_daemon->address();
      }
      res.was_bootstrap_ever_used = m_was_bootstrap_ever_used;
    }
    res.database_size = m_core.get_blockchain_storage().get_db().get_database_size();
    if (restricted)
      res.database_size = round_up(res.database_size, 5ull* 1024 * 1024 * 1024);
    res.update_available = restricted ? false : m_core.is_update_available();
    res.version = restricted ? "" : MONERO_VERSION_FULL;
    res.synchronized = check_core_ready();
    res.busy_syncing = m_p2p.get_payload_object().is_busy_syncing();
    res.restricted = restricted;

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_net_stats(const COMMAND_RPC_GET_NET_STATS::request& req, COMMAND_RPC_GET_NET_STATS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_net_stats);
    // No bootstrap daemon check: Only ever get stats about local server
    res.start_time = (uint64_t)m_core.get_start_time();
    {
      CRITICAL_REGION_LOCAL(epee::net_utils::network_throttle_manager::m_lock_get_global_throttle_in);
      epee::net_utils::network_throttle_manager::get_global_throttle_in().get_stats(res.total_packets_in, res.total_bytes_in);
    }
    {
      CRITICAL_REGION_LOCAL(epee::net_utils::network_throttle_manager::m_lock_get_global_throttle_out);
      epee::net_utils::network_throttle_manager::get_global_throttle_out().get_stats(res.total_packets_out, res.total_bytes_out);
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  class pruned_transaction {
    transaction& tx;
  public:
    pruned_transaction(transaction& tx) : tx(tx) {}
    BEGIN_SERIALIZE_OBJECT()
      bool r = tx.serialize_base(ar);
      if (!r) return false;
    END_SERIALIZE()
  };
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_blocks);

    bool use_bootstrap_daemon;
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      use_bootstrap_daemon = m_should_use_bootstrap_daemon;
    }
    if (use_bootstrap_daemon)
    {
      bool r;
      return use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCKS_FAST>(invoke_http_mode::BIN, "/getblocks.bin", req, res, r);
    }

    CHECK_PAYMENT(req, res, 1);

    // quick check for noop
    if (!req.block_ids.empty())
    {
      uint64_t last_block_height;
      crypto::hash last_block_hash;
      m_core.get_blockchain_top(last_block_height, last_block_hash);
      if (last_block_hash == req.block_ids.front())
      {
        res.start_height = 0;
        res.current_height = m_core.get_current_blockchain_height();
        res.status = CORE_RPC_STATUS_OK;
        return true;
      }
    }

    size_t max_blocks = COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT;
    if (m_rpc_payment)
    {
      max_blocks = res.credits / COST_PER_BLOCK;
      if (max_blocks > COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT)
        max_blocks = COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT;
      if (max_blocks == 0)
      {
        res.status = CORE_RPC_STATUS_PAYMENT_REQUIRED;
        return true;
      }
    }

    std::vector<std::pair<std::pair<cryptonote::blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, cryptonote::blobdata> > > > bs;
    if(!m_core.find_blockchain_supplement(req.start_height, req.block_ids, bs, res.current_height, res.start_height, req.prune, !req.no_miner_tx, max_blocks, COMMAND_RPC_GET_BLOCKS_FAST_MAX_TX_COUNT))
    {
      res.status = "Failed";
      add_host_fail(ctx);
      return true;
    }

    CHECK_PAYMENT_SAME_TS(req, res, bs.size() * COST_PER_BLOCK);

    size_t size = 0, ntxes = 0;
    res.blocks.reserve(bs.size());
    res.output_indices.reserve(bs.size());
    for(auto& bd: bs)
    {
      res.blocks.resize(res.blocks.size()+1);
      res.blocks.back().pruned = req.prune;
      res.blocks.back().block = bd.first.first;
      size += bd.first.first.size();
      res.output_indices.push_back(COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices());
      ntxes += bd.second.size();
      res.output_indices.back().indices.reserve(1 + bd.second.size());
      if (req.no_miner_tx)
        res.output_indices.back().indices.push_back(COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices());
      res.blocks.back().txs.reserve(bd.second.size());
      for (std::vector<std::pair<crypto::hash, cryptonote::blobdata>>::iterator i = bd.second.begin(); i != bd.second.end(); ++i)
      {
        res.blocks.back().txs.push_back({std::move(i->second), crypto::null_hash});
        i->second.clear();
        i->second.shrink_to_fit();
        size += res.blocks.back().txs.back().blob.size();
      }

      const size_t n_txes_to_lookup = bd.second.size() + (req.no_miner_tx ? 0 : 1);
      if (n_txes_to_lookup > 0)
      {
        std::vector<std::vector<uint64_t>> indices;
        bool r = m_core.get_tx_outputs_gindexs(req.no_miner_tx ? bd.second.front().first : bd.first.second, n_txes_to_lookup, indices);
        if (!r)
        {
          res.status = "Failed";
          return true;
        }
        if (indices.size() != n_txes_to_lookup || res.output_indices.back().indices.size() != (req.no_miner_tx ? 1 : 0))
        {
          res.status = "Failed";
          return true;
        }
        for (size_t i = 0; i < indices.size(); ++i)
          res.output_indices.back().indices.push_back({std::move(indices[i])});
      }
    }

    MDEBUG("on_get_blocks: " << bs.size() << " blocks, " << ntxes << " txes, size " << size);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
    bool core_rpc_server::on_get_alt_blocks_hashes(const COMMAND_RPC_GET_ALT_BLOCKS_HASHES::request& req, COMMAND_RPC_GET_ALT_BLOCKS_HASHES::response& res, const connection_context *ctx)
    {
      RPC_TRACKER(get_alt_blocks_hashes);
      bool r;
      if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_ALT_BLOCKS_HASHES>(invoke_http_mode::JON, "/get_alt_blocks_hashes", req, res, r))
        return r;

      std::vector<block> blks;

      if(!m_core.get_alternative_blocks(blks))
      {
          res.status = "Failed";
          return true;
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
  bool core_rpc_server::on_get_blocks_by_height(const COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_blocks_by_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCKS_BY_HEIGHT>(invoke_http_mode::BIN, "/getblocks_by_height.bin", req, res, r))
      return r;

    const bool restricted = m_restricted && ctx;
    if (restricted && req.heights.size() > RESTRICTED_BLOCK_COUNT)
    {
      res.status = "Too many blocks requested in restricted mode";
      return true;
    }

    res.status = "Failed";
    res.blocks.clear();
    res.blocks.reserve(req.heights.size());
    CHECK_PAYMENT_MIN1(req, res, req.heights.size() * COST_PER_BLOCK, false);
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
      std::vector<transaction> txs;
      std::vector<crypto::hash> missed_txs;
      m_core.get_transactions(blk.tx_hashes, txs, missed_txs);
      res.blocks.resize(res.blocks.size() + 1);
      res.blocks.back().block = block_to_blob(blk);
      for (auto& tx : txs)
        res.blocks.back().txs.push_back({tx_to_blob(tx), crypto::null_hash});
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_hashes(const COMMAND_RPC_GET_HASHES_FAST::request& req, COMMAND_RPC_GET_HASHES_FAST::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_hashes);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_HASHES_FAST>(invoke_http_mode::BIN, "/gethashes.bin", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, 1);

    res.start_height = req.start_height;
    if(!m_core.get_blockchain_storage().find_blockchain_supplement(req.block_ids, res.m_block_ids, NULL, res.start_height, res.current_height, false))
    {
      res.status = "Failed";
      add_host_fail(ctx);
      return true;
    }

    CHECK_PAYMENT_SAME_TS(req, res, res.m_block_ids.size() * COST_PER_BLOCK_HASH);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_outs_bin(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_outs_bin);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUTS_BIN>(invoke_http_mode::BIN, "/get_outs.bin", req, res, r))
      return r;

    CHECK_PAYMENT_MIN1(req, res, req.outputs.size() * COST_PER_OUT, false);

    res.status = "Failed";

    const bool restricted = m_restricted && ctx;
    if (restricted)
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
  bool core_rpc_server::on_get_outs(const COMMAND_RPC_GET_OUTPUTS::request& req, COMMAND_RPC_GET_OUTPUTS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_outs);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUTS>(invoke_http_mode::JON, "/get_outs", req, res, r))
      return r;

    CHECK_PAYMENT_MIN1(req, res, req.outputs.size() * COST_PER_OUT, false);

    res.status = "Failed";

    const bool restricted = m_restricted && ctx;
    if (restricted)
    {
      if (req.outputs.size() > MAX_RESTRICTED_GLOBAL_FAKE_OUTS_COUNT)
      {
        res.status = "Too many outs requested";
        return true;
      }
    }

    cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::request req_bin;
    req_bin.outputs = req.outputs;
    req_bin.get_txid = req.get_txid;
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
      if (req.get_txid)
        outkey.txid = epee::string_tools::pod_to_hex(i.txid);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_indexes);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES>(invoke_http_mode::BIN, "/get_o_indexes.bin", req, res, ok))
      return ok;

    CHECK_PAYMENT_MIN1(req, res, COST_PER_OUTPUT_INDEXES, false);

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
  bool core_rpc_server::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_transactions);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTIONS>(invoke_http_mode::JON, "/gettransactions", req, res, ok))
      return ok;

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;

    if (restricted && req.txs_hashes.size() > RESTRICTED_TRANSACTIONS_COUNT)
    {
      res.status = "Too many transactions requested in restricted mode";
      return true;
    }

    CHECK_PAYMENT_MIN1(req, res, req.txs_hashes.size() * COST_PER_TX, false);

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
    std::vector<crypto::hash> missed_txs;
    std::vector<std::tuple<crypto::hash, cryptonote::blobdata, crypto::hash, cryptonote::blobdata>> txs;
    bool r = m_core.get_split_transactions_blobs(vh, txs, missed_txs);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    LOG_PRINT_L2("Found " << txs.size() << "/" << vh.size() << " transactions on the blockchain");

    // try the pool for any missing txes
    size_t found_in_pool = 0;
    std::unordered_set<crypto::hash> pool_tx_hashes;
    std::unordered_map<crypto::hash, tx_info> per_tx_pool_tx_info;
    if (!missed_txs.empty())
    {
      std::vector<tx_info> pool_tx_info;
      std::vector<spent_key_image_info> pool_key_image_info;
      bool r = m_core.get_pool_transactions_and_spent_keys_info(pool_tx_info, pool_key_image_info, !request_has_rpc_origin || !restricted);
      if(r)
      {
        // sort to match original request
        std::vector<std::tuple<crypto::hash, cryptonote::blobdata, crypto::hash, cryptonote::blobdata>> sorted_txs;
        std::vector<tx_info>::const_iterator i;
        unsigned txs_processed = 0;
        for (const crypto::hash &h: vh)
        {
          if (std::find(missed_txs.begin(), missed_txs.end(), h) == missed_txs.end())
          {
            if (txs.size() == txs_processed)
            {
              res.status = "Failed: internal error - txs is empty";
              return true;
            }
            // core returns the ones it finds in the right order
            if (std::get<0>(txs[txs_processed]) != h)
            {
              res.status = "Failed: tx hash mismatch";
              return true;
            }
            sorted_txs.push_back(std::move(txs[txs_processed]));
            ++txs_processed;
          }
          else if ((i = std::find_if(pool_tx_info.begin(), pool_tx_info.end(), [h](const tx_info &txi) { return epee::string_tools::pod_to_hex(h) == txi.id_hash; })) != pool_tx_info.end())
          {
            cryptonote::transaction tx;
            if (!cryptonote::parse_and_validate_tx_from_blob(i->tx_blob, tx))
            {
              res.status = "Failed to parse and validate tx from blob";
              return true;
            }
            std::stringstream ss;
            binary_archive<true> ba(ss);
            bool r = const_cast<cryptonote::transaction&>(tx).serialize_base(ba);
            if (!r)
            {
              res.status = "Failed to serialize transaction base";
              return true;
            }
            const cryptonote::blobdata pruned = ss.str();
            const crypto::hash prunable_hash = tx.version == 1 ? crypto::null_hash : get_transaction_prunable_hash(tx);
            sorted_txs.push_back(std::make_tuple(h, pruned, prunable_hash, std::string(i->tx_blob, pruned.size())));
            missed_txs.erase(std::find(missed_txs.begin(), missed_txs.end(), h));
            pool_tx_hashes.insert(h);
            const std::string hash_string = epee::string_tools::pod_to_hex(h);
            for (const auto &ti: pool_tx_info)
            {
              if (ti.id_hash == hash_string)
              {
                per_tx_pool_tx_info.insert(std::make_pair(h, ti));
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

    CHECK_AND_ASSERT_MES(txs.size() + missed_txs.size() == vh.size(), false, "mismatched number of txs");

    auto txhi = req.txs_hashes.cbegin();
    auto vhi = vh.cbegin();
    auto missedi = missed_txs.cbegin();

    for(auto& tx: txs)
    {
      res.txs.push_back(COMMAND_RPC_GET_TRANSACTIONS::entry());
      COMMAND_RPC_GET_TRANSACTIONS::entry &e = res.txs.back();

      while (missedi != missed_txs.end() && *missedi == *vhi)
      {
          ++vhi;
          ++txhi;
          ++missedi;
      }

      crypto::hash tx_hash = *vhi++;
      CHECK_AND_ASSERT_MES(tx_hash == std::get<0>(tx), false, "mismatched tx hash");
      e.tx_hash = *txhi++;
      e.prunable_hash = epee::string_tools::pod_to_hex(std::get<2>(tx));
      if (req.split || req.prune || std::get<3>(tx).empty())
      {
        // use splitted form with pruned and prunable (filled only when prune=false and the daemon has it), leaving as_hex as empty
        e.pruned_as_hex = string_tools::buff_to_hex_nodelimer(std::get<1>(tx));
        if (!req.prune)
          e.prunable_as_hex = string_tools::buff_to_hex_nodelimer(std::get<3>(tx));
        if (req.decode_as_json)
        {
          cryptonote::blobdata tx_data;
          cryptonote::transaction t;
          if (req.prune || std::get<3>(tx).empty())
          {
            // decode pruned tx to JSON
            tx_data = std::get<1>(tx);
            if (cryptonote::parse_and_validate_tx_base_from_blob(tx_data, t))
            {
              pruned_transaction pruned_tx{t};
              e.as_json = obj_to_json_str(pruned_tx);
            }
            else
            {
              res.status = "Failed to parse and validate pruned tx from blob";
              return true;
            }
          }
          else
          {
            // decode full tx to JSON
            tx_data = std::get<1>(tx) + std::get<3>(tx);
            if (cryptonote::parse_and_validate_tx_from_blob(tx_data, t))
            {
              e.as_json = obj_to_json_str(t);
            }
            else
            {
              res.status = "Failed to parse and validate tx from blob";
              return true;
            }
          }
        }
      }
      else
      {
        // use non-splitted form, leaving pruned_as_hex and prunable_as_hex as empty
        cryptonote::blobdata tx_data = std::get<1>(tx) + std::get<3>(tx);
        e.as_hex = string_tools::buff_to_hex_nodelimer(tx_data);
        if (req.decode_as_json)
        {
          cryptonote::transaction t;
          if (cryptonote::parse_and_validate_tx_from_blob(tx_data, t))
          {
            e.as_json = obj_to_json_str(t);
          }
          else
          {
            res.status = "Failed to parse and validate tx from blob";
            return true;
          }
        }
      }
      e.in_pool = pool_tx_hashes.find(tx_hash) != pool_tx_hashes.end();
      if (e.in_pool)
      {
        e.block_height = e.block_timestamp = std::numeric_limits<uint64_t>::max();
        e.confirmations = 0;
        auto it = per_tx_pool_tx_info.find(tx_hash);
        if (it != per_tx_pool_tx_info.end())
        {
          e.double_spend_seen = it->second.double_spend_seen;
          e.relayed = it->second.relayed;
          e.received_timestamp = it->second.receive_time;
        }
        else
        {
          MERROR("Failed to determine pool info for " << tx_hash);
          e.double_spend_seen = false;
          e.relayed = false;
          e.received_timestamp = 0;
        }
      }
      else
      {
        e.block_height = m_core.get_blockchain_storage().get_db().get_tx_block_height(tx_hash);
        e.confirmations = m_core.get_current_blockchain_height() - e.block_height;
        e.block_timestamp = m_core.get_blockchain_storage().get_db().get_block_timestamp(e.block_height);
        e.received_timestamp = 0;
        e.double_spend_seen = false;
        e.relayed = false;
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
          return true;
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
  bool core_rpc_server::on_is_key_image_spent(const COMMAND_RPC_IS_KEY_IMAGE_SPENT::request& req, COMMAND_RPC_IS_KEY_IMAGE_SPENT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(is_key_image_spent);
    bool ok;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_IS_KEY_IMAGE_SPENT>(invoke_http_mode::JON, "/is_key_image_spent", req, res, ok))
      return ok;

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;

    if (restricted && req.key_images.size() > RESTRICTED_SPENT_KEY_IMAGES_COUNT)
    {
      res.status = "Too many key images queried in restricted mode";
      return true;
    }

    CHECK_PAYMENT_MIN1(req, res, req.key_images.size() * COST_PER_KEY_IMAGE, false);

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
    r = m_core.get_pool_transactions_and_spent_keys_info(txs, ki, !request_has_rpc_origin || !restricted);
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
  bool core_rpc_server::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(send_raw_tx);

    {
      bool ok;
      use_bootstrap_daemon_if_necessary<COMMAND_RPC_SEND_RAW_TX>(invoke_http_mode::JON, "/sendrawtransaction", req, res, ok);
    }

    const bool restricted = m_restricted && ctx;

    bool skip_validation = false;
    if (!restricted)
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        skip_validation = !check_core_ready();
      }
      else
      {
        CHECK_CORE_READY();
      }
    }
    else
    {
      CHECK_CORE_READY();
    }

    CHECK_PAYMENT_MIN1(req, res, COST_PER_TX_RELAY, false);

    std::string tx_blob;
    if(!string_tools::parse_hexstr_to_binbuff(req.tx_as_hex, tx_blob))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex);
      res.status = "Failed";
      return true;
    }

    if (req.do_sanity_checks && !cryptonote::tx_sanity_check(tx_blob, m_core.get_blockchain_storage().get_num_mature_outputs(0)))
    {
      res.status = "Failed";
      res.reason = "Sanity check failed";
      res.sanity_check_failed = true;
      return true;
    }
    res.sanity_check_failed = false;

    if (!skip_validation)
    {
      tx_verification_context tvc{};
      if(!m_core.handle_incoming_tx({tx_blob, crypto::null_hash}, tvc, (req.do_not_relay ? relay_method::none : relay_method::local), false) || tvc.m_verifivation_failed)
      {
        res.status = "Failed";
        std::string reason = "";
        if ((res.low_mixin = tvc.m_low_mixin))
          add_reason(reason, "bad ring size");
        if ((res.double_spend = tvc.m_double_spend))
          add_reason(reason, "double spend");
        if ((res.invalid_input = tvc.m_invalid_input))
          add_reason(reason, "invalid input");
        if ((res.invalid_output = tvc.m_invalid_output))
          add_reason(reason, "invalid output");
        if ((res.too_big = tvc.m_too_big))
          add_reason(reason, "too big");
        if ((res.overspend = tvc.m_overspend))
          add_reason(reason, "overspend");
        if ((res.fee_too_low = tvc.m_fee_too_low))
          add_reason(reason, "fee too low");
        if ((res.too_few_outputs = tvc.m_too_few_outputs))
          add_reason(reason, "too few outputs");
        if ((res.tx_extra_too_big = tvc.m_tx_extra_too_big))
          add_reason(reason, "tx-extra too big");
        const std::string punctuation = reason.empty() ? "" : ": ";
        if (tvc.m_verifivation_failed)
        {
          LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed" << punctuation << reason);
        }
        else
        {
          LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx" << punctuation << reason);
        }
        return true;
      }

      if(tvc.m_relay == relay_method::none)
      {
        LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
        res.reason = "Not relayed";
        res.not_relayed = true;
        res.status = CORE_RPC_STATUS_OK;
        return true;
      }
    }

    NOTIFY_NEW_TRANSACTIONS::request r;
    r.txs.push_back(std::move(tx_blob));
    m_core.get_protocol()->relay_transactions(r, boost::uuids::nil_uuid(), epee::net_utils::zone::invalid, relay_method::local);
    //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(start_mining);
    CHECK_CORE_READY();
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, nettype(), req.miner_address))
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

    cryptonote::miner &miner= m_core.get_miner();
    if (miner.is_mining())
    {
      res.status = "Already mining";
      return true;
    }
    if(!miner.start(info.address, static_cast<size_t>(req.threads_count), req.do_background_mining, req.ignore_battery))
    {
      res.status = "Failed, mining not started";
      LOG_PRINT_L0(res.status);
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(stop_mining);
    cryptonote::miner &miner= m_core.get_miner();
    if(!miner.is_mining())
    {
      res.status = "Mining never started";
      LOG_PRINT_L0(res.status);
      return true;
    }
    if(!miner.stop())
    {
      res.status = "Failed, mining not stopped";
      LOG_PRINT_L0(res.status);
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_mining_status(const COMMAND_RPC_MINING_STATUS::request& req, COMMAND_RPC_MINING_STATUS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(mining_status);

    const miner& lMiner = m_core.get_miner();
    res.active = lMiner.is_mining();
    res.is_background_mining_enabled = lMiner.get_is_background_mining_enabled();
    store_difficulty(m_core.get_blockchain_storage().get_difficulty_for_next_block(), res.difficulty, res.wide_difficulty, res.difficulty_top64);
    
    res.block_target = m_core.get_blockchain_storage().get_current_hard_fork_version() < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2;
    if ( lMiner.is_mining() ) {
      res.speed = lMiner.get_speed();
      res.threads_count = lMiner.get_threads_count();
      res.block_reward = lMiner.get_block_reward();
    }
    const account_public_address& lMiningAdr = lMiner.get_mining_address();
    if (lMiner.is_mining() || lMiner.get_is_background_mining_enabled())
      res.address = get_account_address_as_str(nettype(), false, lMiningAdr);
    const uint8_t major_version = m_core.get_blockchain_storage().get_current_hard_fork_version();
    const unsigned variant = major_version >= 7 ? major_version - 6 : 0;
    switch (variant)
    {
      case 0: res.pow_algorithm = "Cryptonight"; break;
      case 1: res.pow_algorithm = "CNv1 (Cryptonight variant 1)"; break;
      case 2: case 3: res.pow_algorithm = "CNv2 (Cryptonight variant 2)"; break;
      case 4: case 5: res.pow_algorithm = "CNv4 (Cryptonight variant 4)"; break;
      case 6: case 7: case 8: case 9: res.pow_algorithm = "RandomX"; break;
      default: res.pow_algorithm = "RandomX"; break; // assumed
    }
    if (res.is_background_mining_enabled)
    {
      res.bg_idle_threshold = lMiner.get_idle_threshold();
      res.bg_min_idle_seconds = lMiner.get_min_idle_seconds();
      res.bg_ignore_battery = lMiner.get_ignore_battery();
      res.bg_target = lMiner.get_mining_target();
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_save_bc(const COMMAND_RPC_SAVE_BC::request& req, COMMAND_RPC_SAVE_BC::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(save_bc);
    if( !m_core.get_blockchain_storage().store_blockchain() )
    {
      res.status = "Error while storing blockchain";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_peer_list(const COMMAND_RPC_GET_PEER_LIST::request& req, COMMAND_RPC_GET_PEER_LIST::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_peer_list);
    std::vector<nodetool::peerlist_entry> white_list;
    std::vector<nodetool::peerlist_entry> gray_list;

    if (req.public_only)
    {
      m_p2p.get_public_peerlist(gray_list, white_list);
    }
    else
    {
      m_p2p.get_peerlist(gray_list, white_list);
    }

    for (auto & entry : white_list)
    {
      if (!req.include_blocked && m_p2p.is_host_blocked(entry.adr, NULL))
        continue;
      if (entry.adr.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
        res.white_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv4_network_address>().ip(),
            entry.adr.as<epee::net_utils::ipv4_network_address>().port(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
      else if (entry.adr.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
        res.white_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv6_network_address>().host_str(),
            entry.adr.as<epee::net_utils::ipv6_network_address>().port(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
      else
        res.white_list.emplace_back(entry.id, entry.adr.str(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
    }

    for (auto & entry : gray_list)
    {
      if (!req.include_blocked && m_p2p.is_host_blocked(entry.adr, NULL))
        continue;
      if (entry.adr.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
        res.gray_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv4_network_address>().ip(),
            entry.adr.as<epee::net_utils::ipv4_network_address>().port(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
      else if (entry.adr.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
        res.gray_list.emplace_back(entry.id, entry.adr.as<epee::net_utils::ipv6_network_address>().host_str(),
            entry.adr.as<epee::net_utils::ipv6_network_address>().port(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
      else
        res.gray_list.emplace_back(entry.id, entry.adr.str(), entry.last_seen, entry.pruning_seed, entry.rpc_port, entry.rpc_credits_per_hash);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_public_nodes(const COMMAND_RPC_GET_PUBLIC_NODES::request& req, COMMAND_RPC_GET_PUBLIC_NODES::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_public_nodes);

    COMMAND_RPC_GET_PEER_LIST::request peer_list_req;
    COMMAND_RPC_GET_PEER_LIST::response peer_list_res;
    peer_list_req.include_blocked = req.include_blocked;
    const bool success = on_get_peer_list(peer_list_req, peer_list_res, ctx);
    res.status = peer_list_res.status;
    if (!success)
    {      
      res.status = "Failed to get peer list";
      return true;
    }
    if (res.status != CORE_RPC_STATUS_OK)
    {
      return true;
    }

    const auto collect = [](const std::vector<peer> &peer_list, std::vector<public_node> &public_nodes)
    {
      for (const auto &entry : peer_list)
      {
        if (entry.rpc_port != 0)
        {
          public_nodes.emplace_back(entry);
        }
      }
    };

    if (req.white)
    {
      collect(peer_list_res.white_list, res.white);
    }
    if (req.gray)
    {
      collect(peer_list_res.gray_list, res.gray);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_log_hash_rate(const COMMAND_RPC_SET_LOG_HASH_RATE::request& req, COMMAND_RPC_SET_LOG_HASH_RATE::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(set_log_hash_rate);
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
  bool core_rpc_server::on_set_log_level(const COMMAND_RPC_SET_LOG_LEVEL::request& req, COMMAND_RPC_SET_LOG_LEVEL::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(set_log_level);
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
  bool core_rpc_server::on_set_log_categories(const COMMAND_RPC_SET_LOG_CATEGORIES::request& req, COMMAND_RPC_SET_LOG_CATEGORIES::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(set_log_categories);
    mlog_set_log(req.categories.c_str());
    res.categories = mlog_get_categories();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool(const COMMAND_RPC_GET_TRANSACTION_POOL::request& req, COMMAND_RPC_GET_TRANSACTION_POOL::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_transaction_pool);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL>(invoke_http_mode::JON, "/get_transaction_pool", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, 1);

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;
    const bool allow_sensitive = !request_has_rpc_origin || !restricted;

    size_t n_txes = m_core.get_pool_transactions_count(allow_sensitive);
    if (n_txes > 0)
    {
      CHECK_PAYMENT_SAME_TS(req, res, n_txes * COST_PER_TX);
      m_core.get_pool_transactions_and_spent_keys_info(res.transactions, res.spent_key_images, allow_sensitive);
      for (tx_info& txi : res.transactions)
        txi.tx_blob = epee::string_tools::buff_to_hex_nodelimer(txi.tx_blob);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool_hashes_bin(const COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_transaction_pool_hashes);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN>(invoke_http_mode::JON, "/get_transaction_pool_hashes.bin", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, 1);

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;
    const bool allow_sensitive = !request_has_rpc_origin || !restricted;

    size_t n_txes = m_core.get_pool_transactions_count(allow_sensitive);
    if (n_txes > 0)
    {
      CHECK_PAYMENT_SAME_TS(req, res, n_txes * COST_PER_POOL_HASH);
      m_core.get_pool_transaction_hashes(res.tx_hashes, allow_sensitive);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool_hashes(const COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_transaction_pool_hashes);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_HASHES>(invoke_http_mode::JON, "/get_transaction_pool_hashes", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, 1);

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;
    const bool allow_sensitive = !request_has_rpc_origin || !restricted;

    size_t n_txes = m_core.get_pool_transactions_count(allow_sensitive);
    if (n_txes > 0)
    {
      CHECK_PAYMENT_SAME_TS(req, res, n_txes * COST_PER_POOL_HASH);
      std::vector<crypto::hash> tx_hashes;
      m_core.get_pool_transaction_hashes(tx_hashes, allow_sensitive);
      res.tx_hashes.reserve(tx_hashes.size());
      for (const crypto::hash &tx_hash: tx_hashes)
        res.tx_hashes.push_back(epee::string_tools::pod_to_hex(tx_hash));
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transaction_pool_stats(const COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_transaction_pool_stats);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_STATS>(invoke_http_mode::JON, "/get_transaction_pool_stats", req, res, r))
      return r;

    CHECK_PAYMENT_MIN1(req, res, COST_PER_TX_POOL_STATS, false);

    const bool restricted = m_restricted && ctx;
    const bool request_has_rpc_origin = ctx != NULL;
    m_core.get_pool_transaction_stats(res.pool_stats, !request_has_rpc_origin || !restricted);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_bootstrap_daemon(const COMMAND_RPC_SET_BOOTSTRAP_DAEMON::request& req, COMMAND_RPC_SET_BOOTSTRAP_DAEMON::response& res, const connection_context *ctx)
  {
    PERF_TIMER(on_set_bootstrap_daemon);

    boost::optional<epee::net_utils::http::login> credentials;
    if (!req.username.empty() || !req.password.empty())
    {
      credentials = epee::net_utils::http::login(req.username, req.password);
    }

    if (set_bootstrap_daemon(req.address, credentials, req.proxy))
    {
      res.status = CORE_RPC_STATUS_OK;
    }
    else
    {
      res.status = "Failed to set bootstrap daemon";
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(stop_daemon);
    // FIXME: replace back to original m_p2p.send_stop_signal() after
    // investigating why that isn't working quite right.
    m_p2p.send_stop_signal();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(getblockcount);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        res.status = "This command is unsupported for bootstrap daemon";
        return true;
      }
    }
    res.count = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(getblockhash);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        res = "This command is unsupported for bootstrap daemon";
        return true;
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
      error_resp.message = std::string("Requested block height: ") + std::to_string(h) + " greater than current top block height: " +  std::to_string(m_core.get_current_blockchain_height() - 1);
    }
    res = string_tools::pod_to_hex(m_core.get_block_id_by_height(h));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  // equivalent of strstr, but with arbitrary bytes (ie, NULs)
  // This does not differentiate between "not found" and "found at offset 0"
  size_t slow_memmem(const void* start_buff, size_t buflen,const void* pat,size_t patlen)
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
  bool core_rpc_server::get_block_template(const account_public_address &address, const crypto::hash *prev_block, const cryptonote::blobdata &extra_nonce, size_t &reserved_offset, cryptonote::difficulty_type  &difficulty, uint64_t &height, uint64_t &expected_reward, block &b, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, epee::json_rpc::error &error_resp)
  {
    b = boost::value_initialized<cryptonote::block>();
    if(!m_core.get_block_template(b, prev_block, address, difficulty, height, expected_reward, extra_nonce, seed_height, seed_hash))
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
      LOG_ERROR("Failed to get tx pub key in coinbase extra");
      return false;
    }

    uint64_t next_height;
    crypto::rx_seedheights(height, &seed_height, &next_height);
    if (next_height != seed_height)
      next_seed_hash = m_core.get_block_id_by_height(next_height);
    else
      next_seed_hash = seed_hash;

    if (extra_nonce.empty())
    {
      reserved_offset = 0;
      return true;
    }

    reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
    if(!reserved_offset)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to find tx pub key in blockblob");
      return false;
    }
    reserved_offset += sizeof(tx_pub_key) + 2; //2 bytes: tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
    if(reserved_offset + extra_nonce.size() > block_blob.size())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to calculate offset for ");
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(getblocktemplate);
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

    if(req.reserve_size && !req.extra_nonce.empty())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Cannot specify both a reserve_size and an extra_nonce";
      return false;
    }

    if(req.extra_nonce.size() > 510)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE;
      error_resp.message = "Too big extra_nonce size, maximum 510 hex chars";
      return false;
    }

    cryptonote::address_parse_info info;

    if(!req.wallet_address.size() || !cryptonote::get_account_address_from_str(info, nettype(), req.wallet_address))
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

    block b;
    cryptonote::blobdata blob_reserve;
    size_t reserved_offset;
    if(!req.extra_nonce.empty())
    {
      if(!string_tools::parse_hexstr_to_binbuff(req.extra_nonce, blob_reserve))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Parameter extra_nonce should be a hex string";
        return false;
      }
    }
    else
      blob_reserve.resize(req.reserve_size, 0);
    cryptonote::difficulty_type wdiff;
    crypto::hash prev_block;
    if (!req.prev_block.empty())
    {
      if (!epee::string_tools::hex_to_pod(req.prev_block, prev_block))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Invalid prev_block";
        return false;
      }
    }
    crypto::hash seed_hash, next_seed_hash;
    if (!get_block_template(info.address, req.prev_block.empty() ? NULL : &prev_block, blob_reserve, reserved_offset, wdiff, res.height, res.expected_reward, b, res.seed_height, seed_hash, next_seed_hash, error_resp))
      return false;
    if (b.major_version >= RX_BLOCK_VERSION)
    {
      res.seed_hash = string_tools::pod_to_hex(seed_hash);
      if (seed_hash != next_seed_hash)
        res.next_seed_hash = string_tools::pod_to_hex(next_seed_hash);
    }

    res.reserved_offset = reserved_offset;
    store_difficulty(wdiff, res.difficulty, res.wide_difficulty, res.difficulty_top64);
    blobdata block_blob = t_serializable_object_to_blob(b);
    blobdata hashing_blob = get_block_hashing_blob(b);
    res.prev_hash = string_tools::pod_to_hex(b.prev_id);
    res.blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
    res.blockhashing_blob =  string_tools::buff_to_hex_nodelimer(hashing_blob);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getminerdata(const COMMAND_RPC_GETMINERDATA::request& req, COMMAND_RPC_GETMINERDATA::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }

    crypto::hash prev_id, seed_hash;
    difficulty_type difficulty;

    std::vector<tx_block_template_backlog_entry> tx_backlog;
    if (!m_core.get_miner_data(res.major_version, res.height, prev_id, seed_hash, difficulty, res.median_weight, res.already_generated_coins, tx_backlog))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to get miner data";
      LOG_ERROR("Failed to get miner data");
      return false;
    }

    res.tx_backlog.clear();
    res.tx_backlog.reserve(tx_backlog.size());

    for (const auto& entry : tx_backlog)
    {
      res.tx_backlog.emplace_back(COMMAND_RPC_GETMINERDATA::response::tx_backlog_entry{string_tools::pod_to_hex(entry.id), entry.weight, entry.fee});
    }

    res.prev_id = string_tools::pod_to_hex(prev_id);
    res.seed_hash = string_tools::pod_to_hex(seed_hash);
    res.difficulty = cryptonote::hex(difficulty);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_calcpow(const COMMAND_RPC_CALCPOW::request& req, COMMAND_RPC_CALCPOW::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(calcpow);

    blobdata blockblob;
    if(!string_tools::parse_hexstr_to_binbuff(req.block_blob, blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }
    if(!m_core.check_incoming_block_size(blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB_SIZE;
      error_resp.message = "Block blob size is too big, rejecting block";
      return false;
    }
    crypto::hash seed_hash, pow_hash;
    std::string buf;
    if(req.seed_hash.size())
    {
      if (!string_tools::parse_hexstr_to_binbuff(req.seed_hash, buf) ||
        buf.size() != sizeof(crypto::hash))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Wrong seed hash";
        return false;
      }
      buf.copy(reinterpret_cast<char *>(&seed_hash), sizeof(crypto::hash));
    }

    cryptonote::get_block_longhash(&(m_core.get_blockchain_storage()), blockblob, pow_hash, req.height,
      req.major_version, req.seed_hash.size() ? &seed_hash : NULL, 0);
    res = string_tools::pod_to_hex(pow_hash);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_add_aux_pow(const COMMAND_RPC_ADD_AUX_POW::request& req, COMMAND_RPC_ADD_AUX_POW::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(add_aux_pow);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ADD_AUX_POW>(invoke_http_mode::JON_RPC, "add_aux_pow", req, res, r))
      return r;

    if (req.aux_pow.empty())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Empty aux pow hash vector";
      return false;
    }

    crypto::hash merkle_root;
    size_t merkle_tree_depth = 0;
    std::vector<std::pair<crypto::hash, crypto::hash>> aux_pow;
    std::vector<crypto::hash> aux_pow_raw;
    aux_pow.reserve(req.aux_pow.size());
    aux_pow_raw.reserve(req.aux_pow.size());
    for (const auto &s: req.aux_pow)
    {
      aux_pow.push_back({});
      if (!epee::string_tools::hex_to_pod(s.id, aux_pow.back().first))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Invalid aux pow id";
        return false;
      }
      if (!epee::string_tools::hex_to_pod(s.hash, aux_pow.back().second))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Invalid aux pow hash";
        return false;
      }
      aux_pow_raw.push_back(aux_pow.back().second);
    }

    size_t path_domain = 1;
    while ((1u << path_domain) < aux_pow.size())
      ++path_domain;
    uint32_t nonce;
    const uint32_t max_nonce = 65535;
    bool collision = true;
    for (nonce = 0; nonce <= max_nonce; ++nonce)
    {
      std::vector<bool> slots(aux_pow.size(), false);
      collision = false;
      for (size_t idx = 0; idx < aux_pow.size(); ++idx)
      {
        const uint32_t slot = cryptonote::get_aux_slot(aux_pow[idx].first, nonce, aux_pow.size());
        if (slot >= aux_pow.size())
        {
          error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
          error_resp.message = "Computed slot is out of range";
          return false;
        }
        if (slots[slot])
        {
          collision = true;
          break;
        }
        slots[slot] = true;
      }
      if (!collision)
        break;
    }
    if (collision)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Failed to find a suitable nonce";
      return false;
    }

    crypto::tree_hash((const char(*)[crypto::HASH_SIZE])aux_pow_raw.data(), aux_pow_raw.size(), merkle_root.data);
    res.merkle_root = epee::string_tools::pod_to_hex(merkle_root);
    res.merkle_tree_depth = cryptonote::encode_mm_depth(aux_pow.size(), nonce);

    blobdata blocktemplate_blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.blocktemplate_blob, blocktemplate_blob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Invalid blocktemplate_blob";
      return false;
    }

    block b;
    if (!parse_and_validate_block_from_blob(blocktemplate_blob, b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong blocktemplate_blob";
      return false;
    }

    if (!remove_field_from_tx_extra(b.miner_tx.extra, typeid(cryptonote::tx_extra_merge_mining_tag)))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Error removing existing merkle root";
      return false;
    }
    if (!add_mm_merkle_root_to_tx_extra(b.miner_tx.extra, merkle_root, merkle_tree_depth))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Error adding merkle root";
      return false;
    }
    b.invalidate_hashes();
    b.miner_tx.invalidate_hashes();

    const blobdata block_blob = t_serializable_object_to_blob(b);
    const blobdata hashing_blob = get_block_hashing_blob(b);

    res.blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
    res.blockhashing_blob = string_tools::buff_to_hex_nodelimer(hashing_blob);
    res.aux_pow = req.aux_pow;
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(submitblock);
    {
      boost::shared_lock<boost::shared_mutex> lock(m_bootstrap_daemon_mutex);
      if (m_should_use_bootstrap_daemon)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_UNSUPPORTED_BOOTSTRAP;
        error_resp.message = "This command is unsupported for bootstrap daemon";
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
    block b;
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

    block_verification_context bvc;
    if(!m_core.handle_block_found(b, bvc))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED;
      error_resp.message = "Block not accepted";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_generateblocks(const COMMAND_RPC_GENERATEBLOCKS::request& req, COMMAND_RPC_GENERATEBLOCKS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(generateblocks);

    CHECK_CORE_READY();
    
    res.status = CORE_RPC_STATUS_OK;

    if(m_core.get_nettype() != FAKECHAIN)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_REGTEST_REQUIRED;
      error_resp.message = "Regtest required when generating blocks";      
      return false;
    }

    COMMAND_RPC_GETBLOCKTEMPLATE::request template_req;
    COMMAND_RPC_GETBLOCKTEMPLATE::response template_res;
    COMMAND_RPC_SUBMITBLOCK::request submit_req;
    COMMAND_RPC_SUBMITBLOCK::response submit_res;

    template_req.reserve_size = 1;
    template_req.wallet_address = req.wallet_address;
    template_req.prev_block = req.prev_block;
    submit_req.push_back(std::string{});
    res.height = m_core.get_blockchain_storage().get_current_blockchain_height();

    for(size_t i = 0; i < req.amount_of_blocks; i++)
    {
      bool r = on_getblocktemplate(template_req, template_res, error_resp, ctx);
      res.status = template_res.status;
      template_req.prev_block.clear();
      
      if (!r) return false;

      blobdata blockblob;
      if(!string_tools::parse_hexstr_to_binbuff(template_res.blocktemplate_blob, blockblob))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
        error_resp.message = "Wrong block blob";
        return false;
      }
      block b;
      if(!parse_and_validate_block_from_blob(blockblob, b))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
        error_resp.message = "Wrong block blob";
        return false;
      }
      b.nonce = req.starting_nonce;
      crypto::hash seed_hash = crypto::null_hash;
      if (b.major_version >= RX_BLOCK_VERSION && !epee::string_tools::hex_to_pod(template_res.seed_hash, seed_hash))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Error converting seed hash";
        return false;
      }
      miner::find_nonce_for_given_block([this](const cryptonote::block &b, uint64_t height, const crypto::hash *seed_hash, unsigned int threads, crypto::hash &hash) {
        return cryptonote::get_block_longhash(&(m_core.get_blockchain_storage()), b, hash, height, seed_hash, threads);
      }, b, template_res.difficulty, template_res.height, &seed_hash);

      submit_req.front() = string_tools::buff_to_hex_nodelimer(block_to_blob(b));
      r = on_submitblock(submit_req, submit_res, error_resp, ctx);
      res.status = submit_res.status;

      if (!r) return false;

      res.blocks.push_back(epee::string_tools::pod_to_hex(get_block_hash(b)));
      template_req.prev_block = res.blocks.back();
      res.height = template_res.height;
    }

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
  bool core_rpc_server::fill_block_header_response(const block& blk, bool orphan_status, uint64_t height, const crypto::hash& hash, block_header_response& response, bool fill_pow_hash)
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
    store_difficulty(m_core.get_blockchain_storage().block_difficulty(height),
        response.difficulty, response.wide_difficulty, response.difficulty_top64);
    store_difficulty(m_core.get_blockchain_storage().get_db().get_block_cumulative_difficulty(height),
        response.cumulative_difficulty, response.wide_cumulative_difficulty, response.cumulative_difficulty_top64);
    response.reward = get_block_reward(blk);
    response.block_size = response.block_weight = m_core.get_blockchain_storage().get_db().get_block_weight(height);
    response.num_txes = blk.tx_hashes.size();
    response.pow_hash = fill_pow_hash ? string_tools::pod_to_hex(get_block_longhash(&(m_core.get_blockchain_storage()), blk, height, 0)) : "";
    response.long_term_weight = m_core.get_blockchain_storage().get_db().get_block_long_term_weight(height);
    response.miner_tx_hash = string_tools::pod_to_hex(cryptonote::get_transaction_hash(blk.miner_tx));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template <typename COMMAND_TYPE>
  bool core_rpc_server::use_bootstrap_daemon_if_necessary(const invoke_http_mode &mode, const std::string &command_name, const typename COMMAND_TYPE::request& req, typename COMMAND_TYPE::response& res, bool &r)
  {
    res.untrusted = false;

    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_bootstrap_daemon_mutex);

    if (m_bootstrap_daemon.get() == nullptr)
    {
      return false;
    }

    if (!m_should_use_bootstrap_daemon)
    {
      MINFO("The local daemon is fully synced. Not switching back to the bootstrap daemon");
      return false;
    }

    auto current_time = std::chrono::system_clock::now();
    if (current_time - m_bootstrap_height_check_time > std::chrono::seconds(30))  // update every 30s
    {
      {
        boost::upgrade_to_unique_lock<boost::shared_mutex> lock(upgrade_lock);
        m_bootstrap_height_check_time = current_time;
      }

      boost::optional<std::pair<uint64_t, uint64_t>> bootstrap_daemon_height_info = m_bootstrap_daemon->get_height();
      if (!bootstrap_daemon_height_info)
      {
        MERROR("Failed to fetch bootstrap daemon height");
        return false;
      }

      const uint64_t bootstrap_daemon_height = bootstrap_daemon_height_info->first;
      const uint64_t bootstrap_daemon_target_height = bootstrap_daemon_height_info->second;
      if (bootstrap_daemon_height < bootstrap_daemon_target_height)
      {
        MINFO("Bootstrap daemon is out of sync");
        return m_bootstrap_daemon->handle_result(false, {});
      }

      if (bootstrap_daemon_height < m_core.get_checkpoints().get_max_height())
      {
        MINFO("Bootstrap daemon height is lower than the latest checkpoint");
        return m_bootstrap_daemon->handle_result(false, {});
      }

      if (!m_p2p.get_payload_object().no_sync())
      {
        uint64_t top_height = m_core.get_current_blockchain_height();
        m_should_use_bootstrap_daemon = top_height + 10 < bootstrap_daemon_height;
        MINFO((m_should_use_bootstrap_daemon ? "Using" : "Not using") << " the bootstrap daemon (our height: " << top_height << ", bootstrap daemon's height: " << bootstrap_daemon_height << ")");

        if (!m_should_use_bootstrap_daemon)
          return false;
      }
    }

    if (mode == invoke_http_mode::JON)
    {
      r = m_bootstrap_daemon->invoke_http_json(command_name, req, res);
    }
    else if (mode == invoke_http_mode::BIN)
    {
      r = m_bootstrap_daemon->invoke_http_bin(command_name, req, res);
    }
    else if (mode == invoke_http_mode::JON_RPC)
    {
      r = m_bootstrap_daemon->invoke_http_json_rpc(command_name, req, res);
    }
    else
    {
      MERROR("Unknown invoke_http_mode: " << mode);
      return false;
    }

    {
      boost::upgrade_to_unique_lock<boost::shared_mutex> lock(upgrade_lock);
      m_was_bootstrap_ever_used = true;
    }

    if (r && res.status != CORE_RPC_STATUS_PAYMENT_REQUIRED && res.status != CORE_RPC_STATUS_OK)
    {
      MINFO("Failing RPC " << command_name << " due to peer return status " << res.status);
      r = false;
    }
    res.untrusted = true;
    return r;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_last_block_header);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_LAST_BLOCK_HEADER>(invoke_http_mode::JON_RPC, "getlastblockheader", req, res, r))
      return r;

    CHECK_CORE_READY();
    CHECK_PAYMENT_MIN1(req, res, COST_PER_BLOCK_HEADER, false);
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
    const bool restricted = m_restricted && ctx;
    bool response_filled = fill_block_header_response(last_block, false, last_block_height, last_block_hash, res.block_header, req.fill_pow_hash && !restricted);
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
  bool core_rpc_server::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_block_header_by_hash);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH>(invoke_http_mode::JON_RPC, "getblockheaderbyhash", req, res, r))
      return r;

    CHECK_PAYMENT_MIN1(req, res, COST_PER_BLOCK_HEADER, false);

    const bool restricted = m_restricted && ctx;
    if (restricted && req.hashes.size() > RESTRICTED_BLOCK_COUNT)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_RESTRICTED;
      error_resp.message = "Too many block headers requested in restricted mode";
      return false;
    }

    auto get = [this](const std::string &hash, bool fill_pow_hash, block_header_response &block_header, bool restricted, epee::json_rpc::error& error_resp) -> bool {
      crypto::hash block_hash;
      bool hash_parsed = parse_hash256(hash, block_hash);
      if(!hash_parsed)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Failed to parse hex representation of block hash. Hex = " + hash + '.';
        return false;
      }
      block blk;
      bool orphan = false;
      bool have_block = m_core.get_block_by_hash(block_hash, blk, &orphan);
      if (!have_block)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: can't get block by hash. Hash = " + hash + '.';
        return false;
      }
      if (blk.miner_tx.vin.size() != 1 || blk.miner_tx.vin.front().type() != typeid(txin_gen))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
        return false;
      }
      uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
      bool response_filled = fill_block_header_response(blk, orphan, block_height, block_hash, block_header, fill_pow_hash && !restricted);
      if (!response_filled)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: can't produce valid response.";
        return false;
      }
      return true;
    };

    if (!req.hash.empty())
    {
      if (!get(req.hash, req.fill_pow_hash, res.block_header, restricted, error_resp))
        return false;
    }
    res.block_headers.reserve(req.hashes.size());
    for (const std::string &hash: req.hashes)
    {
      res.block_headers.push_back({});
      if (!get(hash, req.fill_pow_hash, res.block_headers.back(), restricted, error_resp))
        return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_headers_range(const COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request& req, COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_block_headers_range);
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
    const bool restricted = m_restricted && ctx;
    if (restricted && req.end_height - req.start_height > RESTRICTED_BLOCK_HEADER_RANGE)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_RESTRICTED;
      error_resp.message = "Too many block headers requested.";
      return false;
    }

    CHECK_PAYMENT_MIN1(req, res, (req.end_height - req.start_height + 1) * COST_PER_BLOCK_HEADER, false);
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
      bool response_filled = fill_block_header_response(blk, false, block_height, block_hash, res.headers.back(), req.fill_pow_hash && !restricted);
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
  bool core_rpc_server::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_block_header_by_height);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT>(invoke_http_mode::JON_RPC, "getblockheaderbyheight", req, res, r))
      return r;

    if(m_core.get_current_blockchain_height() <= req.height)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("Requested block height: ") + std::to_string(req.height) + " greater than current top block height: " +  std::to_string(m_core.get_current_blockchain_height() - 1);
      return false;
    }
    CHECK_PAYMENT_MIN1(req, res, COST_PER_BLOCK_HEADER, false);
    crypto::hash block_hash = m_core.get_block_id_by_height(req.height);
    block blk;
    bool have_block = m_core.get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by height. Height = " + std::to_string(req.height) + '.';
      return false;
    }
    const bool restricted = m_restricted && ctx;
    bool response_filled = fill_block_header_response(blk, false, req.height, block_hash, res.block_header, req.fill_pow_hash && !restricted);
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
  bool core_rpc_server::on_get_block(const COMMAND_RPC_GET_BLOCK::request& req, COMMAND_RPC_GET_BLOCK::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_block);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BLOCK>(invoke_http_mode::JON_RPC, "getblock", req, res, r))
      return r;

    CHECK_PAYMENT_MIN1(req, res, COST_PER_BLOCK, false);

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
        error_resp.message = std::string("Requested block height: ") + std::to_string(req.height) + " greater than current top block height: " +  std::to_string(m_core.get_current_blockchain_height() - 1);
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
    const bool restricted = m_restricted && ctx;
    bool response_filled = fill_block_header_response(blk, orphan, block_height, block_hash, res.block_header, req.fill_pow_hash && !restricted);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.miner_tx_hash = res.block_header.miner_tx_hash;
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
  bool core_rpc_server::on_get_connections(const COMMAND_RPC_GET_CONNECTIONS::request& req, COMMAND_RPC_GET_CONNECTIONS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_connections);

    res.connections = m_p2p.get_payload_object().get_connections();

    res.status = CORE_RPC_STATUS_OK;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info_json(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    if (!on_get_info(req, res, ctx) || res.status != CORE_RPC_STATUS_OK)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = res.status;
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_hard_fork_info(const COMMAND_RPC_HARD_FORK_INFO::request& req, COMMAND_RPC_HARD_FORK_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(hard_fork_info);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_HARD_FORK_INFO>(invoke_http_mode::JON_RPC, "hard_fork_info", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, COST_PER_HARD_FORK_INFO);
    const Blockchain &blockchain = m_core.get_blockchain_storage();
    uint8_t version = req.version > 0 ? req.version : blockchain.get_next_hard_fork_version();
    res.version = blockchain.get_current_hard_fork_version();
    res.enabled = blockchain.get_hard_fork_voting_info(version, res.window, res.votes, res.threshold, res.earliest_height, res.voting);
    res.state = blockchain.get_hard_fork_state();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_bans(const COMMAND_RPC_GETBANS::request& req, COMMAND_RPC_GETBANS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_bans);

    auto now = time(nullptr);
    std::map<std::string, time_t> blocked_hosts = m_p2p.get_blocked_hosts();
    for (std::map<std::string, time_t>::const_iterator i = blocked_hosts.begin(); i != blocked_hosts.end(); ++i)
    {
      if (i->second > now) {
        COMMAND_RPC_GETBANS::ban b;
        b.host = i->first;
        b.ip = 0;
        uint32_t ip;
        if (epee::string_tools::get_ip_int32_from_string(ip, b.host))
          b.ip = ip;
        b.seconds = i->second - now;
        res.bans.push_back(b);
      }
    }
    std::map<epee::net_utils::ipv4_network_subnet, time_t> blocked_subnets = m_p2p.get_blocked_subnets();
    for (std::map<epee::net_utils::ipv4_network_subnet, time_t>::const_iterator i = blocked_subnets.begin(); i != blocked_subnets.end(); ++i)
    {
      if (i->second > now) {
        COMMAND_RPC_GETBANS::ban b;
        b.host = i->first.host_str();
        b.ip = 0;
        b.seconds = i->second - now;
        res.bans.push_back(b);
      }
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_banned(const COMMAND_RPC_BANNED::request& req, COMMAND_RPC_BANNED::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    PERF_TIMER(on_banned);

    auto na_parsed = net::get_network_address(req.address, 0);
    if (!na_parsed)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Unsupported host type";
      return false;
    }
    epee::net_utils::network_address na = std::move(*na_parsed);

    time_t seconds;
    if (m_p2p.is_host_blocked(na, &seconds))
    {
      res.banned = true;
      res.seconds = seconds;
    }
    else
    {
      res.banned = false;
      res.seconds = 0;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_bans(const COMMAND_RPC_SETBANS::request& req, COMMAND_RPC_SETBANS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(set_bans);

    for (auto i = req.bans.begin(); i != req.bans.end(); ++i)
    {
      epee::net_utils::network_address na;

      // try subnet first
      if (!i->host.empty())
      {
        auto ns_parsed = net::get_ipv4_subnet_address(i->host);
        if (ns_parsed)
        {
          if (i->ban)
            m_p2p.block_subnet(*ns_parsed, i->seconds);
          else
            m_p2p.unblock_subnet(*ns_parsed);
          continue;
        }
      }

      // then host
      if (!i->host.empty())
      {
        auto na_parsed = net::get_network_address(i->host, 0);
        if (!na_parsed)
        {
          error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
          error_resp.message = "Unsupported host/subnet type";
          return false;
        }
        na = std::move(*na_parsed);
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
  bool core_rpc_server::on_flush_txpool(const COMMAND_RPC_FLUSH_TRANSACTION_POOL::request& req, COMMAND_RPC_FLUSH_TRANSACTION_POOL::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(flush_txpool);

    bool failed = false;
    std::vector<crypto::hash> txids;
    if (req.txids.empty())
    {
      std::vector<transaction> pool_txs;
      bool r = m_core.get_pool_transactions(pool_txs, true);
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
        crypto::hash txid;
        if(!epee::string_tools::hex_to_pod(str, txid))
        {
          failed = true;
        }
        else
        {
          txids.push_back(txid);
        }
      }
    }
    if (!m_core.get_blockchain_storage().flush_txes_from_pool(txids))
    {
      res.status = "Failed to remove one or more tx(es)";
      return true;
    }

    if (failed)
    {
      if (txids.empty())
        res.status = "Failed to parse txid";
      else
        res.status = "Failed to parse some of the txids";
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_output_histogram(const COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request& req, COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_output_histogram);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUT_HISTOGRAM>(invoke_http_mode::JON_RPC, "get_output_histogram", req, res, r))
      return r;

    const bool restricted = m_restricted && ctx;
    size_t amounts = req.amounts.size();
    if (restricted && amounts == 0)
    {
      res.status = "Restricted RPC will not serve histograms on the whole blockchain. Use your own node.";
      return true;
    }

    uint64_t cost = req.amounts.empty() ? COST_PER_FULL_OUTPUT_HISTOGRAM : (COST_PER_OUTPUT_HISTOGRAM * amounts);
    CHECK_PAYMENT_MIN1(req, res, cost, false);

    if (restricted && req.recent_cutoff > 0 && req.recent_cutoff < (uint64_t)time(NULL) - OUTPUT_HISTOGRAM_RECENT_CUTOFF_RESTRICTION)
    {
      res.status = "Recent cutoff is too old";
      return true;
    }

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
  bool core_rpc_server::on_get_version(const COMMAND_RPC_GET_VERSION::request& req, COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_version);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_VERSION>(invoke_http_mode::JON_RPC, "get_version", req, res, r))
      return r;

    res.version = CORE_RPC_VERSION;
    res.release = MONERO_VERSION_IS_RELEASE;
    res.current_height = m_core.get_current_blockchain_height();
    res.target_height = m_p2p.get_payload_object().is_synchronized() ? 0 : m_core.get_target_blockchain_height();
    for (const auto &hf : m_core.get_blockchain_storage().get_hardforks())
       res.hard_forks.push_back({hf.version, hf.height});
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_coinbase_tx_sum(const COMMAND_RPC_GET_COINBASE_TX_SUM::request& req, COMMAND_RPC_GET_COINBASE_TX_SUM::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_coinbase_tx_sum);
    const uint64_t bc_height = m_core.get_current_blockchain_height();
    if (req.height >= bc_height || req.count > bc_height)
    {
      res.status = "height or count is too large";
      return true;
    }
    CHECK_PAYMENT_MIN1(req, res, COST_PER_COINBASE_TX_SUM_BLOCK * req.count, false);
    std::pair<boost::multiprecision::uint128_t, boost::multiprecision::uint128_t> amounts = m_core.get_coinbase_tx_sum(req.height, req.count);
    store_128(amounts.first, res.emission_amount, res.wide_emission_amount, res.emission_amount_top64);
    store_128(amounts.second, res.fee_amount, res.wide_fee_amount, res.fee_amount_top64);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_base_fee_estimate(const COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request& req, COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_base_fee_estimate);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_BASE_FEE_ESTIMATE>(invoke_http_mode::JON_RPC, "get_fee_estimate", req, res, r))
      return r;

    CHECK_PAYMENT(req, res, COST_PER_FEE_ESTIMATE);

    const uint8_t version = m_core.get_blockchain_storage().get_current_hard_fork_version();
    if (version >= HF_VERSION_2021_SCALING)
    {
      m_core.get_blockchain_storage().get_dynamic_base_fee_estimate_2021_scaling(req.grace_blocks, res.fees);
      res.fee = res.fees[0];
    }
    else
    {
      res.fee = m_core.get_blockchain_storage().get_dynamic_base_fee_estimate(req.grace_blocks);
    }
    res.quantization_mask = Blockchain::get_fee_quantization_mask();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alternate_chains(const COMMAND_RPC_GET_ALTERNATE_CHAINS::request& req, COMMAND_RPC_GET_ALTERNATE_CHAINS::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_alternate_chains);
    try
    {
      std::vector<std::pair<Blockchain::block_extended_info, std::vector<crypto::hash>>> chains = m_core.get_blockchain_storage().get_alternative_chains();
      for (const auto &i: chains)
      {
        difficulty_type wdiff = i.first.cumulative_difficulty;
        res.chains.push_back(COMMAND_RPC_GET_ALTERNATE_CHAINS::chain_info{epee::string_tools::pod_to_hex(get_block_hash(i.first.bl)), i.first.height, i.second.size(), 0, "", 0, {}, std::string()});
        store_difficulty(wdiff, res.chains.back().difficulty, res.chains.back().wide_difficulty, res.chains.back().difficulty_top64);
        res.chains.back().block_hashes.reserve(i.second.size());
        for (const crypto::hash &block_id: i.second)
          res.chains.back().block_hashes.push_back(epee::string_tools::pod_to_hex(block_id));
        if (i.first.height < i.second.size())
        {
          res.status = "Error finding alternate chain attachment point";
          return true;
        }
        cryptonote::block main_chain_parent_block;
        try { main_chain_parent_block = m_core.get_blockchain_storage().get_db().get_block_from_height(i.first.height - i.second.size()); }
        catch (const std::exception &e) { res.status = "Error finding alternate chain attachment point"; return true; }
        res.chains.back().main_chain_parent_block = epee::string_tools::pod_to_hex(get_block_hash(main_chain_parent_block));
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
  bool core_rpc_server::on_get_limit(const COMMAND_RPC_GET_LIMIT::request& req, COMMAND_RPC_GET_LIMIT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_limit);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_LIMIT>(invoke_http_mode::JON, "/get_limit", req, res, r))
      return r;

    res.limit_down = epee::net_utils::connection_basic::get_rate_down_limit();
    res.limit_up = epee::net_utils::connection_basic::get_rate_up_limit();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_limit(const COMMAND_RPC_SET_LIMIT::request& req, COMMAND_RPC_SET_LIMIT::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(set_limit);
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
        res.status = "Invalid parameter";
        return true;
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
        res.status = "Invalid parameter";
        return true;
      }
      epee::net_utils::connection_basic::set_rate_up_limit(nodetool::default_limit_up);
    }

    res.limit_down = epee::net_utils::connection_basic::get_rate_down_limit();
    res.limit_up = epee::net_utils::connection_basic::get_rate_up_limit();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_out_peers(const COMMAND_RPC_OUT_PEERS::request& req, COMMAND_RPC_OUT_PEERS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(out_peers);
    if (req.set)
      m_p2p.change_max_out_public_peers(req.out_peers);
    res.out_peers = m_p2p.get_max_out_public_peers();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_in_peers(const COMMAND_RPC_IN_PEERS::request& req, COMMAND_RPC_IN_PEERS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(in_peers);
    if (req.set)
      m_p2p.change_max_in_public_peers(req.in_peers);
    res.in_peers = m_p2p.get_max_in_public_peers();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_update(const COMMAND_RPC_UPDATE::request& req, COMMAND_RPC_UPDATE::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(update);

    res.update = false;
    if (m_core.offline())
    {
      res.status = "Daemon is running offline";
      return true;
    }

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
        return true;
      }
      if (!tools::sha256sum(path.string(), file_hash))
      {
        MERROR("Failed to hash " << path);
        return true;
      }
      if (hash != epee::string_tools::pod_to_hex(file_hash))
      {
        MERROR("Download from " << res.auto_uri << " does not match the expected hash");
        return true;
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
  bool core_rpc_server::on_pop_blocks(const COMMAND_RPC_POP_BLOCKS::request& req, COMMAND_RPC_POP_BLOCKS::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(pop_blocks);

    m_core.get_blockchain_storage().pop_blocks(req.nblocks);

    res.height = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_relay_tx(const COMMAND_RPC_RELAY_TX::request& req, COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(relay_tx);
    CHECK_PAYMENT_MIN1(req, res, req.txids.size() * COST_PER_TX_RELAY, false);

    const bool restricted = m_restricted && ctx;

    bool failed = false;
    res.status = "";
    for (const auto &str: req.txids)
    {
      crypto::hash txid;
      if(!epee::string_tools::hex_to_pod(str, txid))
      {
        if (!res.status.empty()) res.status += ", ";
        res.status += std::string("invalid transaction id: ") + str;
        failed = true;
        continue;
      }

      //TODO: The get_pool_transaction could have an optional meta parameter
      bool broadcasted = false;
      cryptonote::blobdata txblob;
      if ((broadcasted = m_core.get_pool_transaction(txid, txblob, relay_category::broadcasted)) || (!restricted && m_core.get_pool_transaction(txid, txblob, relay_category::all)))
      {
        // The settings below always choose i2p/tor if enabled. Otherwise, do fluff iff previously relayed else dandelion++ stem.
        NOTIFY_NEW_TRANSACTIONS::request r;
        r.txs.push_back(std::move(txblob));
        const auto tx_relay = broadcasted ? relay_method::fluff : relay_method::local;
        m_core.get_protocol()->relay_transactions(r, boost::uuids::nil_uuid(), epee::net_utils::zone::invalid, tx_relay);
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
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = res.status;
      return false;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_sync_info(const COMMAND_RPC_SYNC_INFO::request& req, COMMAND_RPC_SYNC_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(sync_info);
    CHECK_PAYMENT(req, res, COST_PER_SYNC_INFO);

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height; // turn top block height into blockchain height
    res.target_height = m_p2p.get_payload_object().is_synchronized() ? 0 : m_core.get_target_blockchain_height();
    res.next_needed_pruning_seed = m_p2p.get_payload_object().get_next_needed_pruning_stripe().second;

    for (const auto &c: m_p2p.get_payload_object().get_connections())
      res.peers.push_back({c});
    const cryptonote::block_queue &block_queue = m_p2p.get_payload_object().get_block_queue();
    block_queue.foreach([&](const cryptonote::block_queue::span &span) {
      const std::string span_connection_id = epee::string_tools::pod_to_hex(span.connection_id);
      uint32_t speed = (uint32_t)(100.0f * block_queue.get_speed(span.connection_id) + 0.5f);
      res.spans.push_back({span.start_block_height, span.nblocks, span_connection_id, (uint32_t)(span.rate + 0.5f), speed, span.size, span.origin.str()});
      return true;
    });
    res.overview = block_queue.get_overview(res.height);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_txpool_backlog(const COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::request& req, COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_txpool_backlog);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG>(invoke_http_mode::JON_RPC, "get_txpool_backlog", req, res, r))
      return r;
    size_t n_txes = m_core.get_pool_transactions_count();
    CHECK_PAYMENT_MIN1(req, res, COST_PER_TX_POOL_STATS * n_txes, false);

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
  bool core_rpc_server::on_get_output_distribution(const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request& req, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(get_output_distribution);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUT_DISTRIBUTION>(invoke_http_mode::JON_RPC, "get_output_distribution", req, res, r))
      return r;

    const bool restricted = m_restricted && ctx;
    if (restricted && req.amounts != std::vector<uint64_t>(1, 0))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_RESTRICTED;
      error_resp.message = "Restricted RPC can only get output distribution for rct outputs. Use your own node.";
      return false;
    }

    size_t n_0 = 0, n_non0 = 0;
    for (uint64_t amount: req.amounts)
      if (amount) ++n_non0; else ++n_0;
    CHECK_PAYMENT_MIN1(req, res, n_0 * COST_PER_OUTPUT_DISTRIBUTION_0 + n_non0 * COST_PER_OUTPUT_DISTRIBUTION, false);

    try
    {
      // 0 is placeholder for the whole chain
      const uint64_t req_to_height = req.to_height ? req.to_height : (m_core.get_current_blockchain_height() - 1);
      for (uint64_t amount: req.amounts)
      {
        auto data = rpc::RpcHandler::get_output_distribution([this](uint64_t amount, uint64_t from, uint64_t to, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) { return m_core.get_output_distribution(amount, from, to, start_height, distribution, base); }, amount, req.from_height, req_to_height, [this](uint64_t height) { return m_core.get_blockchain_storage().get_db().get_block_hash_from_height(height); }, req.cumulative, m_core.get_current_blockchain_height());
        if (!data)
        {
          error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
          error_resp.message = "Failed to get output distribution";
          return false;
        }

        res.distributions.push_back({std::move(*data), amount, "", req.binary, req.compress});
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
  bool core_rpc_server::on_get_output_distribution_bin(const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request& req, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response& res, const connection_context *ctx)
  {
    RPC_TRACKER(get_output_distribution_bin);

    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_GET_OUTPUT_DISTRIBUTION>(invoke_http_mode::BIN, "/get_output_distribution.bin", req, res, r))
      return r;

    const bool restricted = m_restricted && ctx;
    if (restricted && req.amounts != std::vector<uint64_t>(1, 0))
    {
      res.status = "Restricted RPC can only get output distribution for rct outputs. Use your own node.";
      return false;
    }

    size_t n_0 = 0, n_non0 = 0;
    for (uint64_t amount: req.amounts)
      if (amount) ++n_non0; else ++n_0;
    CHECK_PAYMENT_MIN1(req, res, n_0 * COST_PER_OUTPUT_DISTRIBUTION_0 + n_non0 * COST_PER_OUTPUT_DISTRIBUTION, false);

    res.status = "Failed";

    if (!req.binary)
    {
      res.status = "Binary only call";
      return true;
    }
    try
    {
      // 0 is placeholder for the whole chain
      const uint64_t req_to_height = req.to_height ? req.to_height : (m_core.get_current_blockchain_height() - 1);
      for (uint64_t amount: req.amounts)
      {
        auto data = rpc::RpcHandler::get_output_distribution([this](uint64_t amount, uint64_t from, uint64_t to, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) { return m_core.get_output_distribution(amount, from, to, start_height, distribution, base); }, amount, req.from_height, req_to_height, [this](uint64_t height) { return m_core.get_blockchain_storage().get_db().get_block_hash_from_height(height); }, req.cumulative, m_core.get_current_blockchain_height());
        if (!data)
        {
          res.status = "Failed to get output distribution";
          return true;
        }

        res.distributions.push_back({std::move(*data), amount, "", req.binary, req.compress});
      }
    }
    catch (const std::exception &e)
    {
      res.status = "Failed to get output distribution";
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_prune_blockchain(const COMMAND_RPC_PRUNE_BLOCKCHAIN::request& req, COMMAND_RPC_PRUNE_BLOCKCHAIN::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(prune_blockchain);

    try
    {
      if (!(req.check ? m_core.check_blockchain_pruning() : m_core.prune_blockchain()))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = req.check ? "Failed to check blockchain pruning" : "Failed to prune blockchain";
        return false;
      }
      res.pruning_seed = m_core.get_blockchain_pruning_seed();
      res.pruned = res.pruning_seed != 0;
    }
    catch (const std::exception &e)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Failed to prune blockchain";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_info(const COMMAND_RPC_ACCESS_INFO::request& req, COMMAND_RPC_ACCESS_INFO::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_info);

    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ACCESS_INFO>(invoke_http_mode::JON_RPC, "rpc_access_info", req, res, r))
      return r;

    // if RPC payment is not enabled
    if (m_rpc_payment == NULL)
    {
      res.diff = 0;
      res.credits_per_hash_found = 0;
      res.credits = 0;
      res.height = 0;
      res.seed_height = 0;
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }

    crypto::public_key client;
    uint64_t ts;
    if (!cryptonote::verify_rpc_payment_signature(req.client, client, ts))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INVALID_CLIENT;
      error_resp.message = "Invalid client ID";
      return false;
    }

    crypto::hash top_hash;
    m_core.get_blockchain_top(res.height, top_hash);
    ++res.height;
    cryptonote::blobdata hashing_blob;
    crypto::hash seed_hash, next_seed_hash;
    if (!m_rpc_payment->get_info(client, [&](const cryptonote::blobdata &extra_nonce, cryptonote::block &b, uint64_t &seed_height, crypto::hash &seed_hash)->bool{
      cryptonote::difficulty_type difficulty;
      uint64_t height, expected_reward;
      size_t reserved_offset;
      if (!get_block_template(m_rpc_payment->get_payment_address(), NULL, extra_nonce, reserved_offset, difficulty, height, expected_reward, b, seed_height, seed_hash, next_seed_hash, error_resp))
        return false;
      return true;
    }, hashing_blob, res.seed_height, seed_hash, top_hash, res.diff, res.credits_per_hash_found, res.credits, res.cookie))
    {
      return false;
    }
    if (hashing_blob.empty())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Invalid hashing blob";
      return false;
    }
    res.hashing_blob = epee::string_tools::buff_to_hex_nodelimer(hashing_blob);
    res.top_hash = epee::string_tools::pod_to_hex(top_hash);
    if (hashing_blob[0] >= RX_BLOCK_VERSION)
    {
      res.seed_hash = string_tools::pod_to_hex(seed_hash);
      if (seed_hash != next_seed_hash)
        res.next_seed_hash = string_tools::pod_to_hex(next_seed_hash);
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_flush_cache(const COMMAND_RPC_FLUSH_CACHE::request& req, COMMAND_RPC_FLUSH_CACHE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(flush_cache);
    if (req.bad_txs)
      m_core.flush_bad_txs_cache();
    if (req.bad_blocks)
      m_core.flush_invalid_blocks();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_submit_nonce(const COMMAND_RPC_ACCESS_SUBMIT_NONCE::request& req, COMMAND_RPC_ACCESS_SUBMIT_NONCE::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_submit_nonce);
    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ACCESS_SUBMIT_NONCE>(invoke_http_mode::JON_RPC, "rpc_access_submit_nonce", req, res, r))
      return r;

    // if RPC payment is not enabled
    if (m_rpc_payment == NULL)
    {
      res.status = "Payment not necessary";
      return true;
    }

    crypto::public_key client;
    uint64_t ts;
    if (!cryptonote::verify_rpc_payment_signature(req.client, client, ts))
    {
      res.credits = 0;
      error_resp.code = CORE_RPC_ERROR_CODE_INVALID_CLIENT;
      error_resp.message = "Invalid client ID";
      return false;
    }

    crypto::hash hash;
    cryptonote::block block;
    crypto::hash top_hash;
    uint64_t height;
    bool stale;
    m_core.get_blockchain_top(height, top_hash);
    if (!m_rpc_payment->submit_nonce(client, req.nonce, top_hash, error_resp.code, error_resp.message, res.credits, hash, block, req.cookie, stale))
    {
      return false;
    }

    if (!stale)
    {
      // it might be a valid block!
      const difficulty_type current_difficulty = m_core.get_blockchain_storage().get_difficulty_for_next_block();
      if (check_hash(hash, current_difficulty))
      {
        MINFO("This payment meets the current network difficulty");
        block_verification_context bvc;
        if(m_core.handle_block_found(block, bvc))
          MGINFO_GREEN("Block found by RPC user at height " << get_block_height(block) << ": " <<
              print_money(cryptonote::get_outs_money_amount(block.miner_tx)));
        else
          MERROR("Seemingly valid block was not accepted");
      }
    }

    m_core.get_blockchain_top(height, top_hash);
    res.top_hash = epee::string_tools::pod_to_hex(top_hash);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_pay(const COMMAND_RPC_ACCESS_PAY::request& req, COMMAND_RPC_ACCESS_PAY::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_pay);

    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ACCESS_PAY>(invoke_http_mode::JON_RPC, "rpc_access_pay", req, res, r))
      return r;

    // if RPC payment is not enabled
    if (m_rpc_payment == NULL)
    {
      res.status = "Payment not necessary";
      return true;
    }

    crypto::public_key client;
    uint64_t ts;
    if (!cryptonote::verify_rpc_payment_signature(req.client, client, ts))
    {
      res.credits = 0;
      error_resp.code = CORE_RPC_ERROR_CODE_INVALID_CLIENT;
      error_resp.message = "Invalid client ID";
      return false;
    }

    RPCTracker ext_tracker(("external:" + req.paying_for).c_str(), PERF_TIMER_NAME(rpc_access_pay));
    if (!check_payment(req.client, req.payment, req.paying_for, false, res.status, res.credits, res.top_hash))
      return true;
    ext_tracker.pay(req.payment);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_tracking(const COMMAND_RPC_ACCESS_TRACKING::request& req, COMMAND_RPC_ACCESS_TRACKING::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_tracking);

    if (req.clear)
    {
      RPCTracker::clear();
      res.status = CORE_RPC_STATUS_OK;
      return true;
    }

    auto data = RPCTracker::data();
    for (const auto &d: data)
    {
      res.data.resize(res.data.size() + 1);
      res.data.back().rpc = d.first;
      res.data.back().count = d.second.count;
      res.data.back().time = d.second.time;
      res.data.back().credits = d.second.credits;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_data(const COMMAND_RPC_ACCESS_DATA::request& req, COMMAND_RPC_ACCESS_DATA::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_data);

    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ACCESS_DATA>(invoke_http_mode::JON_RPC, "rpc_access_data", req, res, r))
      return r;

    if (!m_rpc_payment)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_PAYMENTS_NOT_ENABLED;
      error_resp.message = "Payments not enabled";
      return false;
    }

    m_rpc_payment->foreach([&](const crypto::public_key &client, const rpc_payment::client_info &info){
      res.entries.push_back({
        epee::string_tools::pod_to_hex(client), info.credits, std::max(info.last_request_timestamp / 1000000, info.update_time),
        info.credits_total, info.credits_used, info.nonces_good, info.nonces_stale, info.nonces_bad, info.nonces_dupe
      });
      return true;
    });

    res.hashrate = m_rpc_payment->get_hashes(600) / 600;

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_access_account(const COMMAND_RPC_ACCESS_ACCOUNT::request& req, COMMAND_RPC_ACCESS_ACCOUNT::response& res, epee::json_rpc::error& error_resp, const connection_context *ctx)
  {
    RPC_TRACKER(rpc_access_account);

    bool r;
    if (use_bootstrap_daemon_if_necessary<COMMAND_RPC_ACCESS_ACCOUNT>(invoke_http_mode::JON_RPC, "rpc_access_account", req, res, r))
      return r;

    if (!m_rpc_payment)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_PAYMENTS_NOT_ENABLED;
      error_resp.message = "Payments not enabled";
      return false;
    }

    crypto::public_key client;
    if (!epee::string_tools::hex_to_pod(req.client.substr(0, 2 * sizeof(client)), client))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INVALID_CLIENT;
      error_resp.message = "Invalid client ID";
      return false;
    }

    res.credits = m_rpc_payment->balance(client, req.delta_balance);

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
    , "URL of a 'bootstrap' remote daemon that the connected wallets can use while this daemon is still not fully synced.\n"
      "Use 'auto' to enable automatic public nodes discovering and bootstrap daemon switching"
    , ""
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_bootstrap_daemon_login = {
      "bootstrap-daemon-login"
    , "Specify username:password for the bootstrap daemon login"
    , ""
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_bootstrap_daemon_proxy = {
      "bootstrap-daemon-proxy"
    , "<ip>:<port> socks proxy to use for bootstrap daemon connections"
    , ""
    };

  const command_line::arg_descriptor<std::string> core_rpc_server::arg_rpc_payment_address = {
      "rpc-payment-address"
    , "Restrict RPC to clients sending micropayment to this address"
    , ""
    };

  const command_line::arg_descriptor<uint64_t> core_rpc_server::arg_rpc_payment_difficulty = {
      "rpc-payment-difficulty"
    , "Restrict RPC to clients sending micropayment at this difficulty"
    , DEFAULT_PAYMENT_DIFFICULTY
    };

  const command_line::arg_descriptor<uint64_t> core_rpc_server::arg_rpc_payment_credits = {
      "rpc-payment-credits"
    , "Restrict RPC to clients sending micropayment, yields that many credits per payment"
    , DEFAULT_PAYMENT_CREDITS_PER_HASH
    };

  const command_line::arg_descriptor<bool> core_rpc_server::arg_rpc_payment_allow_free_loopback = {
      "rpc-payment-allow-free-loopback"
    , "Allow free access from the loopback address (ie, the local host)"
    , false
    };
}  // namespace cryptonote
