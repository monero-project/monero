// Copyright (c) 2014-2023, The Monero Project
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

#include "gtest/gtest.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "p2p/net_node.inl"
#include "cryptonote_core/i_core_events.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include <condition_variable>

#define MAKE_IPV4_ADDRESS(a,b,c,d) epee::net_utils::ipv4_network_address{MAKE_IP(a,b,c,d),0}
#define MAKE_IPV4_ADDRESS_PORT(a,b,c,d,e) epee::net_utils::ipv4_network_address{MAKE_IP(a,b,c,d),e}
#define MAKE_IPV4_SUBNET(a,b,c,d,e) epee::net_utils::ipv4_network_subnet{MAKE_IP(a,b,c,d),e}

namespace cryptonote {
  class blockchain_storage;
}

class test_core : public cryptonote::i_core_events
{
public:
  virtual bool is_synchronized() const final { return true; }
  void on_synchronized(){}
  void safesyncmode(const bool){}
  virtual uint64_t get_current_blockchain_height() const final {return 1;}
  void set_target_blockchain_height(uint64_t) {}
  bool init(const boost::program_options::variables_map& vm) {return true ;}
  bool deinit(){return true;}
  bool get_short_chain_history(std::list<crypto::hash>& ids) const { return true; }
  bool have_block(const crypto::hash& id, int *where = NULL) const {return false;}
  bool have_block_unlocked(const crypto::hash& id, int *where = NULL) const {return false;}
  void get_blockchain_top(uint64_t& height, crypto::hash& top_id)const{height=0;top_id=crypto::null_hash;}
  bool handle_incoming_tx(const cryptonote::blobdata& tx_blob, cryptonote::tx_verification_context& tvc, cryptonote::relay_method tx_relay, bool relayed) { return true; }
  bool handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block, cryptonote::block_verification_context& bvc, bool update_miner_blocktemplate = true) { return true; }
  bool handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block, cryptonote::block_verification_context& bvc, cryptonote::pool_supplement& extra_block_txs, bool update_miner_blocktemplate = true) { return true; }
  void pause_mine(){}
  void resume_mine(){}
  bool on_idle(){return true;}
  bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, bool clip_pruned, cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp){return true;}
  bool handle_get_objects(cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote::cryptonote_connection_context& context){return true;}
  cryptonote::blockchain_storage &get_blockchain_storage() { throw std::runtime_error("Called invalid member function: please never call get_blockchain_storage on the TESTING class test_core."); }
  bool get_test_drop_download() const {return true;}
  bool get_test_drop_download_height() const {return true;}
  bool prepare_handle_incoming_blocks(const std::vector<cryptonote::block_complete_entry>  &blocks_entry, std::vector<cryptonote::block> &blocks) { return true; }
  bool cleanup_handle_incoming_blocks(bool force_sync = false) { return true; }
  bool update_checkpoints(const bool skip_dns = false) { return true; }
  uint64_t get_target_blockchain_height() const { return 1; }
  size_t get_block_sync_size(uint64_t height) const { return BLOCKS_SYNCHRONIZING_DEFAULT_COUNT; }
  virtual void on_transactions_relayed(epee::span<const cryptonote::blobdata> tx_blobs, cryptonote::relay_method tx_relay) {}
  cryptonote::network_type get_nettype() const { return cryptonote::MAINNET; }
  bool get_pool_transaction(const crypto::hash& id, cryptonote::blobdata& tx_blob, cryptonote::relay_category tx_category) const { return false; }
  bool pool_has_tx(const crypto::hash &txid) const { return false; }
  bool get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata, cryptonote::block>>& blocks, std::vector<cryptonote::blobdata>& txs) const { return false; }
  bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::vector<cryptonote::blobdata>& txs, std::vector<crypto::hash>& missed_txs, bool pruned = false) const { return false; }
  bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::vector<cryptonote::transaction>& txs, std::vector<crypto::hash>& missed_txs) const { return false; }
  bool get_block_by_hash(const crypto::hash &h, cryptonote::block &blk, bool *orphan = NULL) const { return false; }
  uint8_t get_ideal_hard_fork_version() const { return 0; }
  uint8_t get_ideal_hard_fork_version(uint64_t height) const { return 0; }
  uint8_t get_hard_fork_version(uint64_t height) const { return 0; }
  uint64_t get_earliest_ideal_height_for_version(uint8_t version) const { return 0; }
  cryptonote::difficulty_type get_block_cumulative_difficulty(uint64_t height) const { return 0; }
  uint64_t prevalidate_block_hashes(uint64_t height, const std::vector<crypto::hash> &hashes, const std::vector<uint64_t> &weights) { return 0; }
  bool pad_transactions() { return false; }
  uint32_t get_blockchain_pruning_seed() const { return 0; }
  bool prune_blockchain(uint32_t pruning_seed = 0) { return true; }
  bool is_within_compiled_block_hash_area(uint64_t height) const { return false; }
  bool has_block_weights(uint64_t height, uint64_t nblocks) const { return false; }
  bool get_txpool_complement(const std::vector<crypto::hash> &hashes, std::vector<cryptonote::blobdata> &txes) { return false; }
  bool get_pool_transaction_hashes(std::vector<crypto::hash>& txs, bool include_unrelayed_txes = true) const { return false; }
  crypto::hash get_block_id_by_height(uint64_t height) const { return crypto::null_hash; }
  void stop() {}
};

typedef nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<test_core>> Server;

static bool is_blocked(Server &server, const epee::net_utils::network_address &address, time_t *t = NULL)
{
  std::map<std::string, time_t> hosts = server.get_blocked_hosts();
  for (auto rec: hosts)
  {
    if (rec.first == address.host_str())
    {
      if (t)
        *t = rec.second;
      return true;
    }
  }
  return false;
}

TEST(ban, add)
{
  test_core pr_core;
  cryptonote::t_cryptonote_protocol_handler<test_core> cprotocol(pr_core, NULL);
  Server server(cprotocol);
  cprotocol.set_p2p_endpoint(&server);

  // starts empty
  ASSERT_TRUE(server.get_blocked_hosts().empty());
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // add an IP
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // add the same, should not change
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // remove an unblocked IP, should not change
  ASSERT_FALSE(server.unblock_host(MAKE_IPV4_ADDRESS(1,2,3,5)));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // remove the IP, ends up empty
  ASSERT_TRUE(server.unblock_host(MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 0);
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // remove the IP from an empty list, still empty
  ASSERT_FALSE(server.unblock_host(MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 0);
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // add two for known amounts of time, they're both blocked
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), 1));
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,5), 3));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 2);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));
  ASSERT_TRUE(server.unblock_host(MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.unblock_host(MAKE_IPV4_ADDRESS(1,2,3,5)));

  // these tests would need to call is_remote_ip_allowed, which is private
#if 0
  // after two seconds, the first IP is unblocked, but not the second yet
  sleep(2);
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));

  // after two more seconds, the second IP is also unblocked
  sleep(2);
  ASSERT_TRUE(server.get_blocked_hosts().size() == 0);
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));
#endif

  // add an IP again, then re-ban for longer, then shorter
  time_t t;
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), 2));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4), &t));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));
  ASSERT_TRUE(t >= 1);
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), 9));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4), &t));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));
  ASSERT_TRUE(t >= 8);
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), 5));
  ASSERT_TRUE(server.get_blocked_hosts().size() == 1);
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4), &t));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,5)));
  ASSERT_TRUE(t >= 4);
}

TEST(ban, limit)
{
  test_core pr_core;
  cryptonote::t_cryptonote_protocol_handler<test_core> cprotocol(pr_core, NULL);
  Server server(cprotocol);
  cprotocol.set_p2p_endpoint(&server);

  // starts empty
  ASSERT_TRUE(server.get_blocked_hosts().empty());
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), std::numeric_limits<time_t>::max() - 1));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS(1,2,3,4), 1));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS(1,2,3,4)));
}

TEST(ban, subnet)
{
  time_t seconds;
  test_core pr_core;
  cryptonote::t_cryptonote_protocol_handler<test_core> cprotocol(pr_core, NULL);
  Server server(cprotocol);
  cprotocol.set_p2p_endpoint(&server);

  ASSERT_TRUE(server.block_subnet(MAKE_IPV4_SUBNET(1,2,3,4,24), 10));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 1);
  ASSERT_TRUE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,3,4), &seconds));
  ASSERT_TRUE(seconds >= 9);
  ASSERT_TRUE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,3,255), &seconds));
  ASSERT_TRUE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,3,0), &seconds));
  ASSERT_FALSE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,4,0), &seconds));
  ASSERT_FALSE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,2,0), &seconds));
  ASSERT_TRUE(server.unblock_subnet(MAKE_IPV4_SUBNET(1,2,3,8,24)));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 0);
  ASSERT_FALSE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,3,255), &seconds));
  ASSERT_FALSE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,2,3,0), &seconds));
  ASSERT_TRUE(server.block_subnet(MAKE_IPV4_SUBNET(1,2,3,4,8), 10));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 1);
  ASSERT_TRUE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,255,3,255), &seconds));
  ASSERT_TRUE(server.is_host_blocked(MAKE_IPV4_ADDRESS(1,0,3,255), &seconds));
  ASSERT_FALSE(server.unblock_subnet(MAKE_IPV4_SUBNET(1,2,3,8,24)));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 1);
  ASSERT_TRUE(server.block_subnet(MAKE_IPV4_SUBNET(1,2,3,4,8), 10));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 1);
  ASSERT_TRUE(server.unblock_subnet(MAKE_IPV4_SUBNET(1,255,0,0,8)));
  ASSERT_TRUE(server.get_blocked_subnets().size() == 0);
}

TEST(ban, ignores_port)
{
  test_core pr_core;
  cryptonote::t_cryptonote_protocol_handler<test_core> cprotocol(pr_core, NULL);
  Server server(cprotocol);
  cprotocol.set_p2p_endpoint(&server);

  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS_PORT(1,2,3,4,5)));
  ASSERT_TRUE(server.block_host(MAKE_IPV4_ADDRESS_PORT(1,2,3,4,5), std::numeric_limits<time_t>::max() - 1));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS_PORT(1,2,3,4,5)));
  ASSERT_TRUE(is_blocked(server,MAKE_IPV4_ADDRESS_PORT(1,2,3,4,6)));
  ASSERT_TRUE(server.unblock_host(MAKE_IPV4_ADDRESS_PORT(1,2,3,4,5)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS_PORT(1,2,3,4,5)));
  ASSERT_FALSE(is_blocked(server,MAKE_IPV4_ADDRESS_PORT(1,2,3,4,6)));
}

TEST(node_server, bind_same_p2p_port)
{
  struct test_data_t
  {
    test_core pr_core;
    cryptonote::t_cryptonote_protocol_handler<test_core> cprotocol;
    std::unique_ptr<Server> server;

    test_data_t(): cprotocol(pr_core, NULL)
    {
      server.reset(new Server(cprotocol));
      cprotocol.set_p2p_endpoint(server.get());
    }
  };

  const auto new_node = []() -> std::unique_ptr<test_data_t> {
    test_data_t *d = new test_data_t;
    return std::unique_ptr<test_data_t>(d);
  };

  const auto init = [](const std::unique_ptr<test_data_t>& server, const char* port) -> bool {
    boost::program_options::options_description desc_options("Command line options");
    cryptonote::core::init_options(desc_options);
    Server::init_options(desc_options);

    const char *argv[2] = {nullptr, nullptr};
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(1, argv, desc_options), vm);

    /*
    Reason for choosing '127.0.0.2' as the IP:

    A TCP local socket address that has been bound is unavailable for some time after closing, unless the SO_REUSEADDR flag has been set.
    That's why connections with automatically assigned source port 48080/58080 from previous test blocks the next to bind acceptor
    so solution is to either set reuse_addr option for each socket in all tests
    or use ip different from localhost for acceptors in order to not interfere with automatically assigned source endpoints

    Relevant part about REUSEADDR from man:
    https://www.man7.org/linux/man-pages/man7/ip.7.html

    For Mac OSX and OpenBSD, set the following alias (by running the command as root), before running the test, or else it will fail:
    ifconfig lo0 alias 127.0.0.2
    */
    vm.find(nodetool::arg_p2p_bind_ip.name)->second   = boost::program_options::variable_value(std::string("127.0.0.2"), false);
    vm.find(nodetool::arg_p2p_bind_port.name)->second = boost::program_options::variable_value(std::string(port), false);

    boost::program_options::notify(vm);

    return server->server->init(vm);
  };

  constexpr char port[] = "48080";
  constexpr char port_another[] = "58080";

  const auto node = new_node();
  EXPECT_TRUE(init(node, port));

  EXPECT_FALSE(init(new_node(), port));
  EXPECT_TRUE(init(new_node(), port_another));
}

TEST(cryptonote_protocol_handler, race_condition)
{
  struct contexts {
    using basic = epee::net_utils::connection_context_base;
    using cryptonote = cryptonote::cryptonote_connection_context;
    using p2p = nodetool::p2p_connection_context_t<cryptonote>;
  };
  using context_t = contexts::p2p;
  using handler_t = epee::levin::async_protocol_handler<context_t>;
  using connection_t = epee::net_utils::connection<handler_t>;
  using connection_ptr = boost::shared_ptr<connection_t>;
  using connections_t = std::vector<connection_ptr>;
  using shared_state_t = typename connection_t::shared_state;
  using shared_state_ptr = std::shared_ptr<shared_state_t>;
  using io_context_t = boost::asio::io_service;
  using event_t = epee::simple_event;
  using ec_t = boost::system::error_code;
  auto create_conn_pair = [](connection_ptr in, connection_ptr out) {
    using endpoint_t = boost::asio::ip::tcp::endpoint;
    using acceptor_t = boost::asio::ip::tcp::acceptor;
    io_context_t io_context;
    endpoint_t endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5262);
    acceptor_t acceptor(io_context);
    ec_t ec;
    acceptor.open(endpoint.protocol(), ec);
    EXPECT_EQ(ec.value(), 0);
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint, ec);
    EXPECT_EQ(ec.value(), 0);
    acceptor.listen(boost::asio::socket_base::max_connections, ec);
    EXPECT_EQ(ec.value(), 0);
    out->socket().open(endpoint.protocol(), ec);
    EXPECT_EQ(ec.value(), 0);
    acceptor.async_accept(in->socket(), [](const ec_t &ec){});
    out->socket().async_connect(endpoint, [](const ec_t &ec){});
    io_context.run();
    acceptor.close(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(in->start(true, true));
    EXPECT_TRUE(out->start(false, true));
    return std::make_pair<>(std::move(in), std::move(out));
  };
  auto get_conn_tag = [](connection_t &conn){
    context_t context;
    conn.get_context(context);
    return context.m_connection_id;
  };
  using work_t = boost::asio::io_service::work;
  using work_ptr = std::shared_ptr<work_t>;
  using workers_t = std::vector<std::thread>;
  using commands_handler_t = epee::levin::levin_commands_handler<context_t>;
  using p2p_endpoint_t = nodetool::i_p2p_endpoint<contexts::cryptonote>;
  using core_t = cryptonote::core;
  using core_ptr = std::unique_ptr<core_t>;
  using core_protocol_t = cryptonote::t_cryptonote_protocol_handler<core_t>;
  using core_protocol_ptr = std::shared_ptr<core_protocol_t>;
  using block_t = cryptonote::block;
  using diff_t = cryptonote::difficulty_type;
  using reward_t = uint64_t;
  using height_t = uint64_t;
  struct span {
    using blocks = epee::span<const block_t>;
  };
  auto get_block_template = [](
    core_t &core,
    block_t &block,
    diff_t &diff,
    reward_t &reward
  ){
    auto &storage = core.get_blockchain_storage();
    const auto height = storage.get_current_blockchain_height();
    const auto hardfork = storage.get_current_hard_fork_version();
    block.major_version = hardfork;
    block.minor_version = storage.get_ideal_hard_fork_version();
    block.prev_id = storage.get_tail_id();
    auto &db = storage.get_db();
    block.timestamp = db.get_top_block_timestamp();
    block.nonce = 0xACAB;
    block.miner_tx.vin.clear();
    block.miner_tx.vout.clear();
    block.miner_tx.extra.clear();
    block.miner_tx.version = hardfork >= 4 ? 2 : 1;
    block.miner_tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
    block.miner_tx.vin.push_back(cryptonote::txin_gen{height});
    cryptonote::add_tx_pub_key_to_extra(block.miner_tx, {});
    cryptonote::get_block_reward(
      db.get_block_weight(height - 1),
      {},
      db.get_block_already_generated_coins(height - 1),
      reward,
      hardfork
    );
    block.miner_tx.vout.push_back(cryptonote::tx_out{reward, cryptonote::txout_to_key{}});
    diff = storage.get_difficulty_for_next_block();
  };
  struct stat {
    struct chain {
      diff_t diff;
      reward_t reward;
    };
  };
  auto add_block = [](
    core_t &core,
    const block_t &block,
    const stat::chain &stat
  ){
    core.get_blockchain_storage().get_db().batch_start({}, {});
    core.get_blockchain_storage().get_db().add_block(
      {block, cryptonote::block_to_blob(block)},
      cryptonote::get_transaction_weight(block.miner_tx),
      core.get_blockchain_storage().get_next_long_term_block_weight(
        cryptonote::get_transaction_weight(block.miner_tx)
      ),
      stat.diff,
      stat.reward,
      {}
    );
    core.get_blockchain_storage().get_db().batch_stop();
  };
  struct messages {
    struct core {
      using sync = cryptonote::CORE_SYNC_DATA;
    };
    using handshake = nodetool::COMMAND_HANDSHAKE_T<core::sync>;
  };
  struct net_node_t: commands_handler_t, p2p_endpoint_t {
    using span_t = epee::span<const uint8_t>;
    using zone_t = epee::net_utils::zone;
    using uuid_t = boost::uuids::uuid;
    using relay_t = cryptonote::relay_method;
    using blobs_t = std::vector<cryptonote::blobdata>;
    using id_t = nodetool::peerid_type;
    using callback_t = std::function<bool(contexts::cryptonote &, id_t, uint32_t)>;
    using address_t = epee::net_utils::network_address;
    using connections_t = std::vector<std::pair<zone_t, uuid_t>>;
    struct bans {
      using subnets = std::map<epee::net_utils::ipv4_network_subnet, time_t>;
      using hosts = std::map<std::string, time_t>;
    };
    shared_state_ptr shared_state;
    core_protocol_ptr core_protocol;
    virtual int invoke(int command, const span_t in, epee::byte_stream &out, context_t &context) override {
      if (core_protocol) {
        if (command == messages::handshake::ID) {
          return epee::net_utils::buff_to_t_adapter<void, typename messages::handshake::request, typename messages::handshake::response>(
            command,
            in,
            out,
            [this](int command, typename messages::handshake::request &in, typename messages::handshake::response &out, context_t &context){
              core_protocol->process_payload_sync_data(in.payload_data, context, true);
              core_protocol->get_payload_sync_data(out.payload_data);
              return 1;
            },
            context
          );
        }
        bool handled;
        return core_protocol->handle_invoke_map(false, command, in, out, context, handled);
      }
      else
        return {};
    }
    virtual int notify(int command, const span_t in, context_t &context) override {
      if (core_protocol) {
        bool handled;
        epee::byte_stream out;
        return core_protocol->handle_invoke_map(true, command, in, out, context, handled);
      }
      else
        return {};
    }
    virtual void callback(context_t &context) override {
      if (core_protocol)
        core_protocol->on_callback(context);
    }
    virtual void on_connection_new(context_t&) override {}
    virtual void on_connection_close(context_t &context) override {
      if (core_protocol)
        core_protocol->on_connection_close(context);
    }
    virtual ~net_node_t() override {}
    virtual bool add_host_fail(const address_t&, unsigned int = {}) override {
      return {};
    }
    virtual bool block_host(address_t address, time_t = {}, bool = {}) override {
      return {};
    }
    virtual bool drop_connection(const contexts::basic& context) override {
      if (shared_state)
        return shared_state->close(context.m_connection_id);
      else
        return {};
    }
    virtual bool for_connection(const uuid_t& uuid, callback_t f) override {
      if (shared_state)
        return shared_state->for_connection(uuid,[&f](context_t &context){
          return f(context, context.peer_id, context.support_flags);
        });
      else
        return {};
    }
    virtual bool invoke_notify_to_peer(int command, epee::levin::message_writer in, const contexts::basic& context) override {
      if (shared_state)
        return shared_state->send(in.finalize_notify(command), context.m_connection_id);
      else
        return {};
    }
    virtual bool relay_notify_to_list(int command, epee::levin::message_writer in, connections_t connections) override {
      if (shared_state) {
        for (auto &e: connections)
          shared_state->send(in.finalize_notify(command), e.second);
      }
      return {};
    }
    virtual bool unblock_host(const address_t&) override {
      return {};
    }
    virtual zone_t send_txs(blobs_t, const zone_t, const uuid_t&, relay_t) override {
      return {};
    }
    virtual bans::subnets get_blocked_subnets() override {
      return {};
    }
    virtual bans::hosts get_blocked_hosts() override {
      return {};
    }
    virtual uint64_t get_public_connections_count() override {
      if (shared_state)
        return shared_state->get_connections_count();
      else
        return {};
    }
    virtual void add_used_stripe_peer(const contexts::cryptonote&) override {}
    virtual void clear_used_stripe_peers() override {}
    virtual void remove_used_stripe_peer(const contexts::cryptonote&) override {}
    virtual void for_each_connection(callback_t f) override {
      if (shared_state)
        shared_state->foreach_connection([&f](context_t &context){
          return f(context, context.peer_id, context.support_flags);
        });
    }
    virtual void request_callback(const contexts::basic &context) override {
      if (shared_state)
        shared_state->request_callback(context.m_connection_id);
    }
  };
  auto conduct_handshake = [get_conn_tag](net_node_t &net_node, connection_ptr conn){
    event_t handshaked;
    net_node.shared_state->for_connection(
      get_conn_tag(*conn),
      [&handshaked, &net_node](context_t &context){
        typename messages::handshake::request msg;
        net_node.core_protocol->get_payload_sync_data(msg.payload_data);
        epee::net_utils::async_invoke_remote_command2<typename messages::handshake::response>(
          context,
          messages::handshake::ID,
          msg,
          *net_node.shared_state,
          [&handshaked, &net_node](int code, const typename messages::handshake::response &msg, context_t &context){
            EXPECT_TRUE(code >= 0);
            net_node.core_protocol->process_payload_sync_data(msg.payload_data, context, true);
            handshaked.raise();
          },
          P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT
        );
        return true;
      }
    );
    handshaked.wait();
  };
  using path_t = boost::filesystem::path;
  auto create_dir = []{
    ec_t ec;
    path_t path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("daemon-%%%%%%%%%%%%%%%%", ec);
    if (ec)
      return path_t{};
    auto success = boost::filesystem::create_directory(path, ec);
    if (not ec && success)
      return path;
    return path_t{};
  };
  auto remove_tree = [](const path_t &path){
    ec_t ec;
    boost::filesystem::remove_all(path, ec);
  };
  using options_t = boost::program_options::variables_map;
  struct daemon_t {
    options_t options;
    core_ptr core;
    core_protocol_ptr core_protocol;
    net_node_t net_node;
    shared_state_ptr shared_state;
    connections_t conn;
  };
  struct daemons_t {
    daemon_t main;
    daemon_t alt;
  };
  using options_description_t = boost::program_options::options_description;

  const auto dir = create_dir();
  ASSERT_TRUE(not dir.empty());

  daemons_t daemon{
    {
      [&dir]{
        options_t options;
        boost::program_options::store(
          boost::program_options::command_line_parser({
            "--data-dir",
            (dir / "main").string(),
            "--disable-dns-checkpoints",
            "--check-updates=disabled",
            "--fixed-difficulty=1",
            "--block-sync-size=1",
            "--db-sync-mode=fastest:async:50000",
          }).options([]{
            options_description_t options_description{};
            cryptonote::core::init_options(options_description);
            return options_description;
          }()).run(),
          options
        );
        return options;
      }(),
      {},
      {},
      {},
      {},
      {},
    },
    {
      [&dir]{
        options_t options;
        boost::program_options::store(
          boost::program_options::command_line_parser({
            "--data-dir",
            (dir / "alt").string(),
            "--disable-dns-checkpoints",
            "--check-updates=disabled",
            "--fixed-difficulty=1",
            "--block-sync-size=1",
            "--db-sync-mode=fastest:async:50000",
          }).options([]{
            options_description_t options_description{};
            cryptonote::core::init_options(options_description);
            return options_description;
          }()).run(),
          options
        );
        return options;
      }(),
      {},
      {},
      {},
      {},
      {},
    },
  };

  io_context_t io_context;
  work_ptr work = std::make_shared<work_t>(io_context);
  workers_t workers;
  while (workers.size() < 4) {
    workers.emplace_back([&io_context]{
      io_context.run();
    });
  }

  connection_t::set_rate_up_limit(std::numeric_limits<int64_t>::max());
  connection_t::set_rate_down_limit(std::numeric_limits<int64_t>::max());

  {
    daemon.main.core = core_ptr(new core_t(nullptr));
    daemon.main.core->init(daemon.main.options, nullptr, nullptr);
    daemon.main.net_node.core_protocol = daemon.main.core_protocol = core_protocol_ptr(new core_protocol_t(
      *daemon.main.core, &daemon.main.net_node, {}
    ));
    daemon.main.core->set_cryptonote_protocol(daemon.main.core_protocol.get());
    daemon.main.core_protocol->init(daemon.main.options);
    daemon.main.net_node.shared_state = daemon.main.shared_state = std::make_shared<shared_state_t>();
    daemon.main.shared_state->set_handler(&daemon.main.net_node);
    daemon.alt.shared_state = std::make_shared<shared_state_t>();
    daemon.alt.shared_state->set_handler(&daemon.alt.net_node);

    struct {
      event_t prepare;
      event_t check;
      event_t finish;
    } events;
    auto connections = create_conn_pair(
      connection_ptr(new connection_t(io_context, daemon.main.shared_state, {}, {})),
      connection_ptr(new connection_t(io_context, daemon.alt.shared_state, {}, {}))
    );
    {
      auto conn = connections.first;
      auto shared_state = daemon.main.shared_state;
      const auto tag = get_conn_tag(*conn);
      conn->strand_.post([tag, conn, shared_state, &events]{
        shared_state->for_connection(tag, [](context_t &context){
          context.m_expect_height = -1;
          context.m_expect_response = -1;
          context.m_last_request_time = boost::date_time::min_date_time;
          context.m_score = 0;
          context.m_state = contexts::cryptonote::state_synchronizing;
          return true;
        });
        events.prepare.raise();
        events.check.wait();
        shared_state->for_connection(tag, [](context_t &context){
          EXPECT_TRUE(context.m_expect_height == -1);
          EXPECT_TRUE(context.m_expect_response == -1);
          EXPECT_TRUE(context.m_last_request_time == boost::date_time::min_date_time);
          EXPECT_TRUE(context.m_score == 0);
          EXPECT_TRUE(context.m_state == contexts::cryptonote::state_synchronizing);
          return true;
        });
        events.finish.raise();
      });
    }
    events.prepare.wait();
    daemon.main.core_protocol->on_idle();
    events.check.raise();
    events.finish.wait();

    connections.first->strand_.post([connections]{
      connections.first->cancel();
    });
    connections.second->strand_.post([connections]{
      connections.second->cancel();
    });
    connections.first.reset();
    connections.second.reset();
    while (daemon.main.shared_state->sock_count);
    while (daemon.alt.shared_state->sock_count);
    daemon.main.core_protocol->deinit();
    daemon.main.core->stop();
    daemon.main.core->deinit();
    daemon.main.net_node.shared_state.reset();
    daemon.main.shared_state.reset();
    daemon.main.core_protocol.reset();
    daemon.main.core.reset();
    daemon.alt.shared_state.reset();
  }

  {
    daemon.main.core = core_ptr(new core_t(nullptr));
    daemon.main.core->init(daemon.main.options, nullptr, nullptr);
    daemon.main.net_node.core_protocol = daemon.main.core_protocol = core_protocol_ptr(new core_protocol_t(
      *daemon.main.core, &daemon.main.net_node, {}
    ));
    daemon.main.core->set_cryptonote_protocol(daemon.main.core_protocol.get());
    daemon.main.core->set_checkpoints({});
    daemon.main.core_protocol->init(daemon.main.options);
    daemon.main.net_node.shared_state = daemon.main.shared_state = std::make_shared<shared_state_t>();
    daemon.main.shared_state->set_handler(&daemon.main.net_node);
    daemon.alt.core = core_ptr(new core_t(nullptr));
    daemon.alt.core->init(daemon.alt.options, nullptr, nullptr);
    daemon.alt.net_node.core_protocol = daemon.alt.core_protocol = core_protocol_ptr(new core_protocol_t(
      *daemon.alt.core, &daemon.alt.net_node, {}
    ));
    daemon.alt.core->set_cryptonote_protocol(daemon.alt.core_protocol.get());
    daemon.alt.core->set_checkpoints({});
    daemon.alt.core_protocol->init(daemon.alt.options);
    daemon.alt.net_node.shared_state = daemon.alt.shared_state = std::make_shared<shared_state_t>();
    daemon.alt.shared_state->set_handler(&daemon.alt.net_node);

    struct {
      io_context_t io_context;
      work_ptr work;
      workers_t workers;
    } check;
    check.work = std::make_shared<work_t>(check.io_context);
    while (check.workers.size() < 2) {
      check.workers.emplace_back([&check]{
        check.io_context.run();
      });
    }
    while (daemon.main.conn.size() < 1) {
      daemon.main.conn.emplace_back(new connection_t(check.io_context, daemon.main.shared_state, {}, {}));
      daemon.alt.conn.emplace_back(new connection_t(io_context, daemon.alt.shared_state, {}, {}));
      create_conn_pair(daemon.main.conn.back(), daemon.alt.conn.back());
      conduct_handshake(daemon.alt.net_node, daemon.alt.conn.back());
    }
    struct {
      event_t prepare;
      event_t sync;
      event_t finish;
    } events;
    {
      auto conn = daemon.main.conn.back();
      auto shared_state = daemon.main.shared_state;
      const auto tag = get_conn_tag(*conn);
      conn->strand_.post([tag, conn, shared_state, &events]{
        shared_state->for_connection(tag, [](context_t &context){
          EXPECT_TRUE(context.m_state == contexts::cryptonote::state_normal);
          return true;
        });
        events.prepare.raise();
        events.sync.wait();
        shared_state->for_connection(tag, [](context_t &context){
          EXPECT_TRUE(context.m_state == contexts::cryptonote::state_normal);
          return true;
        });
        events.finish.raise();
      });
    }
    events.prepare.wait();
    daemon.main.core->get_blockchain_storage().add_block_notify(
      [&events](height_t height, span::blocks blocks){
        if (height >= CRYPTONOTE_PRUNING_STRIPE_SIZE)
          events.sync.raise();
      }
    );
    {
      stat::chain stat{
        daemon.alt.core->get_blockchain_storage().get_db().get_block_cumulative_difficulty(
          daemon.alt.core->get_current_blockchain_height() - 1
        ),
        daemon.alt.core->get_blockchain_storage().get_db().get_block_already_generated_coins(
          daemon.alt.core->get_current_blockchain_height() - 1
        ),
      };
      while (daemon.alt.core->get_current_blockchain_height() < CRYPTONOTE_PRUNING_STRIPE_SIZE + CRYPTONOTE_PRUNING_TIP_BLOCKS) {
        block_t block;
        diff_t diff;
        reward_t reward;
        get_block_template(*daemon.alt.core, block, diff, reward);
        stat.diff += diff;
        stat.reward = stat.reward < (MONEY_SUPPLY - stat.reward) ? stat.reward + reward : MONEY_SUPPLY;
        add_block(*daemon.alt.core, block, stat);
        if (daemon.main.core->get_current_blockchain_height() + 1 < CRYPTONOTE_PRUNING_STRIPE_SIZE)
          add_block(*daemon.main.core, block, stat);
      }
    }
    while (daemon.main.conn.size() < 2) {
      daemon.main.conn.emplace_back(new connection_t(check.io_context, daemon.main.shared_state, {}, {}));
      daemon.alt.conn.emplace_back(new connection_t(io_context, daemon.alt.shared_state, {}, {}));
      create_conn_pair(daemon.main.conn.back(), daemon.alt.conn.back());
      conduct_handshake(daemon.alt.net_node, daemon.alt.conn.back());
    }
    events.finish.wait();

    for (;daemon.main.conn.size(); daemon.main.conn.pop_back()) {
      auto conn = daemon.main.conn.back();
      conn->strand_.post([conn]{
        conn->cancel();
      });
    }
    for (;daemon.alt.conn.size(); daemon.alt.conn.pop_back()) {
      auto conn = daemon.alt.conn.back();
      conn->strand_.post([conn]{
        conn->cancel();
      });
    }
    while (daemon.main.shared_state->sock_count);
    while (daemon.alt.shared_state->sock_count);
    daemon.main.core_protocol->deinit();
    daemon.main.core->stop();
    daemon.main.core->deinit();
    daemon.main.net_node.shared_state.reset();
    daemon.main.shared_state.reset();
    daemon.main.core_protocol.reset();
    daemon.main.core.reset();
    daemon.alt.core_protocol->deinit();
    daemon.alt.core->stop();
    daemon.alt.core->deinit();
    daemon.alt.net_node.shared_state.reset();
    daemon.alt.shared_state.reset();
    daemon.alt.core_protocol.reset();
    daemon.alt.core.reset();
    check.work.reset();
    for (auto& w: check.workers) {
      w.join();
    }
  }

  work.reset();
  for (auto& w: workers) {
    w.join();
  }
  remove_tree(dir);
}

TEST(node_server, race_condition)
{
  struct contexts {
    using cryptonote = cryptonote::cryptonote_connection_context;
    using p2p = nodetool::p2p_connection_context_t<cryptonote>;
  };
  using context_t = contexts::cryptonote;
  using options_t = boost::program_options::variables_map;
  using options_description_t = boost::program_options::options_description;
  using worker_t = std::thread;
  struct protocol_t {
  private:
    using p2p_endpoint_t = nodetool::i_p2p_endpoint<context_t>;
    using lock_t = std::mutex;
    using condition_t = std::condition_variable_any;
    using unique_lock_t = std::unique_lock<lock_t>;
    p2p_endpoint_t *p2p_endpoint;
    lock_t lock;
    condition_t condition;
    bool started{};
    size_t counter{};
  public:
    using payload_t = cryptonote::CORE_SYNC_DATA;
    using blob_t = cryptonote::blobdata;
    using connection_context = context_t;
    using payload_type = payload_t;
    using relay_t = cryptonote::relay_method;
    using string_t = std::string;
    using span_t = epee::span<const uint8_t>;
    using blobs_t = epee::span<const cryptonote::blobdata>;
    using connections_t = std::list<cryptonote::connection_info>;
    using block_queue_t = cryptonote::block_queue;
    using stripes_t = std::pair<uint32_t, uint32_t>;
    using byte_stream_t = epee::byte_stream;
    struct core_events_t: cryptonote::i_core_events {
      uint64_t get_current_blockchain_height() const override { return {}; }
      bool is_synchronized() const override { return {}; }
      void on_transactions_relayed(blobs_t blobs, relay_t relay) override {}
    };
    int handle_invoke_map(bool is_notify, int command, const span_t in, byte_stream_t &out, context_t &context, bool &handled) {
      return {};
    }
    bool on_idle() {
      if (not p2p_endpoint)
        return {};
      {
        unique_lock_t guard(lock);
        if (not started)
          started = true;
        else
          return {};
      }
      std::vector<blob_t> txs(128 / 64 * 1024 * 1024, blob_t(1, 'x'));
      worker_t worker([this]{
        p2p_endpoint->for_each_connection(
          [this](context_t &, uint64_t, uint32_t){
            {
              unique_lock_t guard(lock);
              ++counter;
              condition.notify_all();
              condition.wait(guard, [this]{ return counter >= 3; });
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
            return false;
          }
        );
      });
      {
        unique_lock_t guard(lock);
        ++counter;
        condition.notify_all();
        condition.wait(guard, [this]{ return counter >= 3; });
        ++counter;
        condition.notify_all();
        condition.wait(guard, [this]{ return counter >= 5; });
      }
      p2p_endpoint->send_txs(
        std::move(txs),
        epee::net_utils::zone::public_,
        {},
        relay_t::fluff
      );
      worker.join();
      return {};
    }
    bool init(const options_t &options) { return {}; }
    bool deinit() { return {}; }
    void set_p2p_endpoint(p2p_endpoint_t *p2p_endpoint) {
      this->p2p_endpoint = p2p_endpoint;
    }
    bool process_payload_sync_data(const payload_t &payload, contexts::p2p &context, bool is_inital) {
      context.m_state = context_t::state_normal;
      context.m_needed_objects.resize(512 * 1024);
      {
        unique_lock_t guard(lock);
        ++counter;
        condition.notify_all();
        condition.wait(guard, [this]{ return counter >= 3; });
        ++counter;
        condition.notify_all();
        condition.wait(guard, [this]{ return counter >= 5; });
      }
      return true;
    }
    bool get_payload_sync_data(blob_t &blob) { return {}; }
    bool get_payload_sync_data(payload_t &payload) { return {}; }
    bool on_callback(context_t &context) { return {}; }
    core_events_t &get_core(){ static core_events_t core_events; return core_events;}
    void log_connections() {}
    connections_t get_connections() { return {}; }
    const block_queue_t &get_block_queue() const {
      static block_queue_t block_queue;
      return block_queue;
    }
    void stop() {}
    void on_connection_close(context_t &context) {}
    void set_max_out_peers(epee::net_utils::zone zone, unsigned int max) {}
    bool no_sync() const { return {}; }
    void set_no_sync(bool value) {}
    string_t get_peers_overview() const { return {}; }
    stripes_t get_next_needed_pruning_stripe() const { return {}; }
    bool needs_new_sync_connections(epee::net_utils::zone zone) const { return {}; }
    bool is_busy_syncing() { return {}; }
  };
  using node_server_t = nodetool::node_server<protocol_t>;
  auto conduct_test = [](protocol_t &protocol){
    struct messages {
      struct core {
        using sync = cryptonote::CORE_SYNC_DATA;
      };
      using handshake = nodetool::COMMAND_HANDSHAKE_T<core::sync>;
    };
    using handler_t = epee::levin::async_protocol_handler<context_t>;
    using connection_t = epee::net_utils::connection<handler_t>;
    using connection_ptr = boost::shared_ptr<connection_t>;
    using shared_state_t = typename connection_t::shared_state;
    using shared_state_ptr = std::shared_ptr<shared_state_t>;
    using io_context_t = boost::asio::io_service;
    using work_t = boost::asio::io_service::work;
    using work_ptr = std::shared_ptr<work_t>;
    using workers_t = std::vector<std::thread>;
    using endpoint_t = boost::asio::ip::tcp::endpoint;
    using event_t = epee::simple_event;
    struct command_handler_t: epee::levin::levin_commands_handler<context_t> {
      using span_t = epee::span<const uint8_t>;
      using byte_stream_t = epee::byte_stream;
      int invoke(int, const span_t, byte_stream_t &, context_t &) override { return {}; }
      int notify(int, const span_t, context_t &) override { return {}; }
      void callback(context_t &) override {}
      void on_connection_new(context_t &) override {}
      void on_connection_close(context_t &) override {}
      ~command_handler_t() override {}
      static void destroy(epee::levin::levin_commands_handler<context_t>* ptr) { delete ptr; }
    };
    io_context_t io_context;
    work_ptr work = std::make_shared<work_t>(io_context);
    workers_t workers;
    while (workers.size() < 4) {
      workers.emplace_back([&io_context]{
        io_context.run();
      });
    }
    io_context.post([&]{
      protocol.on_idle();
    });
    io_context.post([&]{
      protocol.on_idle();
    });
    shared_state_ptr shared_state = std::make_shared<shared_state_t>();
    shared_state->set_handler(new command_handler_t, &command_handler_t::destroy);
    connection_ptr conn{new connection_t(io_context, shared_state, {}, {})};
    endpoint_t endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 48080);
    conn->socket().connect(endpoint);
    conn->socket().set_option(boost::asio::ip::tcp::socket::reuse_address(true));
    conn->start({}, {});
    context_t context;
    conn->get_context(context);
    event_t handshaked;
    typename messages::handshake::request_t msg{{
      ::config::NETWORK_ID,
      58080,
    }};
    epee::net_utils::async_invoke_remote_command2<typename messages::handshake::response>(
      context,
      messages::handshake::ID,
      msg,
      *shared_state,
      [conn, &handshaked](int code, const typename messages::handshake::response &msg, context_t &context){
        EXPECT_TRUE(code >= 0);
        handshaked.raise();
      },
      P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT
    );
    handshaked.wait();
    conn->strand_.post([conn]{
      conn->cancel();
    });
    conn.reset();
    work.reset();
    for (auto& w: workers) {
      w.join();
    }
  };
  using path_t = boost::filesystem::path;
  using ec_t = boost::system::error_code;
  auto create_dir = []{
    ec_t ec;
    path_t path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("daemon-%%%%%%%%%%%%%%%%", ec);
    if (ec)
      return path_t{};
    auto success = boost::filesystem::create_directory(path, ec);
    if (not ec && success)
      return path;
    return path_t{};
  };
  auto remove_tree = [](const path_t &path){
    ec_t ec;
    boost::filesystem::remove_all(path, ec);
  };
  const auto dir = create_dir();
  ASSERT_TRUE(not dir.empty());
  protocol_t protocol{};
  node_server_t node_server(protocol);
  protocol.set_p2p_endpoint(&node_server);
  node_server.init(
    [&dir]{
      options_t options;
      boost::program_options::store(
        boost::program_options::command_line_parser({
          "--p2p-bind-ip=127.0.0.1",
          "--p2p-bind-port=48080",
          "--out-peers=0",
          "--data-dir",
          dir.string(),
          "--no-igd",
          "--add-exclusive-node=127.0.0.1:48080",
          "--check-updates=disabled",
          "--disable-dns-checkpoints",
        }).options([]{
          options_description_t options_description{};
          cryptonote::core::init_options(options_description);
          node_server_t::init_options(options_description);
          return options_description;
        }()).run(),
        options
      );
      return options;
    }()
  );
  worker_t worker([&]{
    node_server.run();
  });
  conduct_test(protocol);
  node_server.send_stop_signal();
  worker.join();
  node_server.deinit();
  remove_tree(dir);
}

namespace nodetool { template class node_server<cryptonote::t_cryptonote_protocol_handler<test_core>>; }
namespace cryptonote { template class t_cryptonote_protocol_handler<test_core>; }
