// Copyright (c) 2026, The Monero Project
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

// Feeds P2P and block-sync protocol messages from a remote peer into node_server's levin
// command handler, the same entry point the TCP server uses. The node runs offline with
// ping-back disabled, so no sockets are bound or connected.

#include <cstdio>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "include_base_utils.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "p2p/net_node.inl"
#include "cryptonote_core/i_core_events.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include "storages/portable_storage_template_helper.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

namespace cryptonote {
  class blockchain_storage;
}

// Core stub, same as tests/unit_tests/node_server.cpp.
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
  bool get_short_chain_history(std::list<crypto::hash>& ids, uint64_t& current_height) const { return true; }
  bool have_block(const crypto::hash& id, int *where = NULL) const {return false;}
  bool have_block_unlocked(const crypto::hash& id, int *where = NULL) const {return false;}
  void get_blockchain_top(uint64_t& height, crypto::hash& top_id)const{height=0;top_id=crypto::null_hash;}
  bool handle_incoming_tx(const cryptonote::blobdata& tx_blob, cryptonote::tx_verification_context& tvc, cryptonote::relay_method tx_relay, bool relayed) { return true; }
  bool handle_single_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *b, cryptonote::block_verification_context& bvc, cryptonote::pool_supplement& extra_block_txs, bool update_miner_blocktemplate = true) { return true; }
  bool handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block, cryptonote::block_verification_context& bvc, bool update_miner_blocktemplate = true) { return true; }
  bool handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block, cryptonote::block_verification_context& bvc, cryptonote::pool_supplement& extra_block_txs, bool update_miner_blocktemplate = true) { return true; }
  void pause_mine(){}
  void resume_mine(){}
  bool on_idle(){return true;}
  bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, bool clip_pruned, cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp){return true;}
  bool handle_get_objects(cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote::cryptonote_connection_context& context){return true;}
  cryptonote::blockchain_storage &get_blockchain_storage() { throw std::runtime_error("Called invalid member function: please never call get_blockchain_storage on the TESTING class test_core."); }
  bool prepare_handle_incoming_blocks(const std::vector<cryptonote::block_complete_entry>  &blocks_entry, std::vector<cryptonote::block> &blocks) { return true; }
  bool cleanup_handle_incoming_blocks(bool force_sync = false) { return true; }
  bool check_incoming_block_size(const cryptonote::blobdata& block_blob) const { return true; }
  bool update_checkpoints(const bool skip_dns = false) { return true; }
  uint64_t get_target_blockchain_height() const { return 1; }
  size_t get_block_sync_size(uint64_t height, const uint64_t max_average_of_blocksize_in_queue = 0) const { return BLOCKS_SYNCHRONIZING_DEFAULT_COUNT; }
  bool is_block_sync_size_adaptive() const { return false; }
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
  bool check_block_weights(uint64_t height, const std::vector<cryptonote::block_complete_entry> &blocks) const { return false; }
  bool get_txpool_complement(const std::vector<crypto::hash> &hashes, std::vector<cryptonote::blobdata> &txes) { return false; }
  bool get_pool_transaction_hashes(std::vector<crypto::hash>& txs, bool include_unrelayed_txes = true) const { return false; }
  crypto::hash get_block_id_by_height(uint64_t height) const { return crypto::null_hash; }
  void stop() {}
};

using protocol_t = cryptonote::t_cryptonote_protocol_handler<test_core>;
using server_t = nodetool::node_server<protocol_t>;
using p2p_context = nodetool::p2p_connection_context_t<cryptonote::cryptonote_connection_context>;
using handshake_t = nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>;
using timed_sync_t = nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>;

static constexpr size_t kMaxMessages = 8;
static constexpr size_t kMaxItems = 12;

static boost::program_options::variables_map vm;

//----------------------------------------------------------------------------------------------------------------------
// Message payload builders
//----------------------------------------------------------------------------------------------------------------------
// Small pool so the same ids recur across messages.
static crypto::hash consume_hash(FuzzedDataProvider &fdp)
{
  crypto::hash h = crypto::null_hash;
  if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
  {
    const std::string bytes = fdp.ConsumeBytesAsString(sizeof(h));
    memcpy(&h, bytes.data(), bytes.size());
  }
  else
  {
    h.data[0] = fdp.ConsumeIntegralInRange<uint8_t>(0, 7);
  }
  return h;
}

static std::vector<crypto::hash> consume_hashes(FuzzedDataProvider &fdp)
{
  std::vector<crypto::hash> hashes(fdp.ConsumeIntegralInRange<size_t>(0, kMaxItems));
  for (auto &h : hashes)
    h = consume_hash(fdp);
  return hashes;
}

static cryptonote::CORE_SYNC_DATA consume_sync_data(FuzzedDataProvider &fdp)
{
  cryptonote::CORE_SYNC_DATA sync{};
  sync.current_height = fdp.ConsumeBool() ? fdp.ConsumeIntegralInRange<uint64_t>(0, 4) : fdp.ConsumeIntegral<uint64_t>();
  sync.cumulative_difficulty = fdp.ConsumeIntegral<uint64_t>();
  sync.cumulative_difficulty_top64 = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint64_t>();
  sync.top_id = consume_hash(fdp);
  sync.top_version = fdp.ConsumeIntegral<uint8_t>();
  sync.pruning_seed = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint32_t>();
  return sync;
}

static cryptonote::blobdata consume_tx_blob(FuzzedDataProvider &fdp)
{
  if (fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
    return fdp.ConsumeRandomLengthString(256);
  cryptonote::transaction tx;
  tx.version = fdp.ConsumeIntegralInRange<size_t>(1, 2);
  tx.unlock_time = fdp.ConsumeIntegral<uint8_t>();
  cryptonote::txin_to_key in;
  in.amount = fdp.ConsumeIntegral<uint32_t>();
  in.key_offsets.push_back(fdp.ConsumeIntegral<uint8_t>());
  const crypto::hash key_image = consume_hash(fdp);
  in.k_image = reinterpret_cast<const crypto::key_image&>(key_image);
  tx.vin.push_back(in);
  cryptonote::tx_out out;
  out.amount = fdp.ConsumeIntegral<uint32_t>();
  const crypto::hash out_key = consume_hash(fdp);
  out.target = cryptonote::txout_to_key(reinterpret_cast<const crypto::public_key&>(out_key));
  tx.vout.push_back(out);
  if (tx.version == 1)
    tx.signatures.resize(1, std::vector<crypto::signature>(1));
  return cryptonote::tx_to_blob(tx);
}

static cryptonote::blobdata consume_block_blob(FuzzedDataProvider &fdp)
{
  if (fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
    return fdp.ConsumeRandomLengthString(256);
  cryptonote::block b;
  b.major_version = fdp.ConsumeIntegralInRange<uint8_t>(1, 17);
  b.minor_version = fdp.ConsumeIntegral<uint8_t>();
  b.timestamp = fdp.ConsumeIntegral<uint32_t>();
  b.prev_id = consume_hash(fdp);
  b.nonce = fdp.ConsumeIntegral<uint32_t>();
  b.miner_tx.version = 2;
  cryptonote::txin_gen gen;
  gen.height = fdp.ConsumeIntegralInRange<uint64_t>(0, 4);
  b.miner_tx.vin.push_back(gen);
  cryptonote::tx_out out;
  out.amount = fdp.ConsumeIntegral<uint32_t>();
  const crypto::hash out_key = consume_hash(fdp);
  out.target = cryptonote::txout_to_key(reinterpret_cast<const crypto::public_key&>(out_key));
  b.miner_tx.vout.push_back(out);
  const size_t n_txs = fdp.ConsumeIntegralInRange<size_t>(0, 3);
  for (size_t i = 0; i < n_txs; ++i)
    b.tx_hashes.push_back(consume_hash(fdp));
  return cryptonote::block_to_blob(b);
}

static cryptonote::block_complete_entry consume_block_entry(FuzzedDataProvider &fdp)
{
  cryptonote::block_complete_entry entry{};
  entry.pruned = fdp.ConsumeBool();
  entry.block = consume_block_blob(fdp);
  entry.block_weight = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint32_t>();
  const size_t n_txs = fdp.ConsumeIntegralInRange<size_t>(0, 3);
  for (size_t i = 0; i < n_txs; ++i)
  {
    const cryptonote::blobdata tx_blob = consume_tx_blob(fdp);
    entry.txs.push_back(cryptonote::tx_blob_entry{tx_blob, consume_hash(fdp)});
  }
  return entry;
}

template<typename T>
static std::string to_blob(T &msg)
{
  epee::byte_slice slice;
  FUZZ_CHECK(epee::serialization::store_t_to_binary(msg, slice));
  return std::string(reinterpret_cast<const char *>(slice.data()), slice.size());
}

struct message
{
  int command;
  bool is_notify;
  std::string blob;
};

static message build_message(FuzzedDataProvider &fdp)
{
  switch (fdp.ConsumeIntegralInRange<int>(0, 13))
  {
    case 0:
    {
      handshake_t::request req{};
      if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
      {
        const std::string bytes = fdp.ConsumeBytesAsString(sizeof(req.node_data.network_id));
        memcpy(&req.node_data.network_id, bytes.data(), bytes.size());
      }
      else
      {
        memcpy(&req.node_data.network_id, &::config::NETWORK_ID, sizeof(req.node_data.network_id));
      }
      req.node_data.my_port = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint16_t>();
      req.node_data.rpc_port = fdp.ConsumeIntegral<uint16_t>();
      req.node_data.rpc_credits_per_hash = fdp.ConsumeIntegral<uint32_t>();
      req.node_data.peer_id = fdp.ConsumeIntegral<uint64_t>();
      req.node_data.support_flags = fdp.ConsumeIntegral<uint32_t>();
      req.payload_data = consume_sync_data(fdp);
      return {handshake_t::ID, false, to_blob(req)};
    }
    case 1:
    {
      timed_sync_t::request req{};
      req.payload_data = consume_sync_data(fdp);
      return {timed_sync_t::ID, false, to_blob(req)};
    }
    case 2:
    {
      nodetool::COMMAND_PING::request req{};
      return {nodetool::COMMAND_PING::ID, false, to_blob(req)};
    }
    case 3:
    {
      nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::request req{};
      return {nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::ID, false, to_blob(req)};
    }
    case 4:
    {
      cryptonote::NOTIFY_NEW_TRANSACTIONS::request req{};
      const size_t n = fdp.ConsumeIntegralInRange<size_t>(0, 4);
      for (size_t i = 0; i < n; ++i)
        req.txs.push_back(consume_tx_blob(fdp));
      req._ = fdp.ConsumeRandomLengthString(32);
      req.dandelionpp_fluff = fdp.ConsumeBool();
      return {cryptonote::NOTIFY_NEW_TRANSACTIONS::ID, true, to_blob(req)};
    }
    case 5:
    {
      cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request req{};
      req.blocks = consume_hashes(fdp);
      req.prune = fdp.ConsumeBool();
      return {cryptonote::NOTIFY_REQUEST_GET_OBJECTS::ID, true, to_blob(req)};
    }
    case 6:
    {
      cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request req{};
      const size_t n = fdp.ConsumeIntegralInRange<size_t>(0, 3);
      for (size_t i = 0; i < n; ++i)
        req.blocks.push_back(consume_block_entry(fdp));
      req.missed_ids = consume_hashes(fdp);
      req.current_blockchain_height = fdp.ConsumeIntegral<uint64_t>();
      return {cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::ID, true, to_blob(req)};
    }
    case 7:
    {
      cryptonote::NOTIFY_REQUEST_CHAIN::request req{};
      for (const auto &h : consume_hashes(fdp))
        req.block_ids.push_back(h);
      req.prune = fdp.ConsumeBool();
      return {cryptonote::NOTIFY_REQUEST_CHAIN::ID, true, to_blob(req)};
    }
    case 8:
    {
      cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request req{};
      req.start_height = fdp.ConsumeBool() ? fdp.ConsumeIntegralInRange<uint64_t>(0, 4) : fdp.ConsumeIntegral<uint64_t>();
      req.total_height = fdp.ConsumeBool() ? fdp.ConsumeIntegralInRange<uint64_t>(0, 16) : fdp.ConsumeIntegral<uint64_t>();
      req.cumulative_difficulty = fdp.ConsumeIntegral<uint64_t>();
      req.cumulative_difficulty_top64 = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint64_t>();
      req.m_block_ids = consume_hashes(fdp);
      req.m_block_weights.resize(fdp.ConsumeIntegralInRange<size_t>(0, kMaxItems));
      for (auto &w : req.m_block_weights)
        w = fdp.ConsumeIntegral<uint32_t>();
      req.first_block = consume_block_blob(fdp);
      return {cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::ID, true, to_blob(req)};
    }
    case 9:
    {
      cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request req{};
      req.b = consume_block_entry(fdp);
      req.current_blockchain_height = fdp.ConsumeIntegral<uint64_t>();
      return {cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::ID, true, to_blob(req)};
    }
    case 10:
    {
      cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::request req{};
      req.block_hash = consume_hash(fdp);
      req.current_blockchain_height = fdp.ConsumeIntegral<uint64_t>();
      req.missing_tx_indices.resize(fdp.ConsumeIntegralInRange<size_t>(0, kMaxItems));
      for (auto &i : req.missing_tx_indices)
        i = fdp.ConsumeIntegralInRange<uint64_t>(0, 8);
      return {cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::ID, true, to_blob(req)};
    }
    case 11:
    {
      cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::request req{};
      req.hashes = consume_hashes(fdp);
      return {cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::ID, true, to_blob(req)};
    }
    case 12:
    {
      cryptonote::NOTIFY_NEW_BLOCK::request req{};
      req.b = consume_block_entry(fdp);
      req.current_blockchain_height = fdp.ConsumeIntegral<uint64_t>();
      return {cryptonote::NOTIFY_NEW_BLOCK::ID, true, to_blob(req)};
    }
    default:
      return {fdp.ConsumeIntegral<int>(), fdp.ConsumeBool(), fdp.ConsumeRandomLengthString(512)};
  }
}

static epee::net_utils::network_address consume_remote_address(FuzzedDataProvider &fdp)
{
  const uint16_t port = fdp.ConsumeIntegral<uint16_t>();
  switch (fdp.ConsumeIntegralInRange<int>(0, 3))
  {
    case 0:
      return epee::net_utils::ipv6_network_address{boost::asio::ip::make_address_v6(fdp.ConsumeBool() ? "2001:db8::1" : "::ffff:1.2.3.4"), port};
    case 1:
      // Zones other than public_ are not configured; handlers must cope.
      return net::tor_address::unknown();
    default:
      return epee::net_utils::ipv4_network_address{fdp.ConsumeIntegral<uint32_t>(), port};
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Response checks
//----------------------------------------------------------------------------------------------------------------------
static void check_peerlist(const std::vector<nodetool::peerlist_entry> &peers)
{
  FUZZ_CHECK(peers.size() <= P2P_DEFAULT_PEERS_IN_HANDSHAKE);
  for (const auto &pe : peers)
    FUZZ_CHECK(!nodetool::is_forbidden_ipv6_address(pe.adr));
}

static void check_response(int command, const epee::byte_stream &out)
{
  const epee::span<const uint8_t> span{out.data(), out.size()};
  if (command == handshake_t::ID)
  {
    handshake_t::response rsp{};
    FUZZ_CHECK(epee::serialization::load_t_from_binary(rsp, span));
    check_peerlist(rsp.local_peerlist_new);
  }
  else if (command == timed_sync_t::ID)
  {
    timed_sync_t::response rsp{};
    FUZZ_CHECK(epee::serialization::load_t_from_binary(rsp, span));
    check_peerlist(rsp.local_peerlist_new);
  }
  else if (command == nodetool::COMMAND_PING::ID)
  {
    nodetool::COMMAND_PING::response rsp{};
    FUZZ_CHECK(epee::serialization::load_t_from_binary(rsp, span));
    FUZZ_CHECK(rsp.status == PING_OK_RESPONSE_STATUS_TEXT);
  }
  else if (command == nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::ID)
  {
    nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::response rsp{};
    FUZZ_CHECK(epee::serialization::load_t_from_binary(rsp, span));
  }
}

BEGIN_INIT_SIMPLE_FUZZER()
  boost::program_options::options_description desc;
  cryptonote::core::init_options(desc);
  server_t::init_options(desc);

  // The data dir is never created: init only tries to read p2pstate.bin from it.
  const std::string data_dir = (boost::filesystem::temp_directory_path() / "monero-p2p-commands-fuzz").string();
  const std::vector<std::string> args{
    "--regtest",
    "--offline",
    "--out-peers=0",
    "--in-peers=0",
    "--data-dir", data_dir,
    "--check-updates=disabled",
    "--disable-dns-checkpoints",
    "--add-peer", "1.2.3.4:18080",
    "--add-peer", "5.6.7.8:18081",
  };
  boost::program_options::store(boost::program_options::command_line_parser(args).options(desc).run(), vm);
  boost::program_options::notify(vm);
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  test_core core;
  protocol_t protocol(core, NULL);
  server_t server(protocol);
  protocol.set_p2p_endpoint(&server);
  // The proxy is never contacted; setting one turns off handshake ping-back (an outbound connection).
  FUZZ_CHECK(server.init(vm, "127.0.0.1:9050", false));

  p2p_context context;
  boost::uuids::uuid connection_id{};
  const epee::net_utils::network_address remote = consume_remote_address(fdp);
  const bool is_income = fdp.ConsumeBool();
  static_cast<epee::net_utils::connection_context_base &>(context) =
      epee::net_utils::connection_context_base(connection_id, remote, is_income, false);
  context.m_state = static_cast<cryptonote::cryptonote_connection_context::state>(fdp.ConsumeIntegralInRange<int>(0, 3));

  epee::levin::levin_commands_handler<p2p_context> &handler = server;
  const size_t n_messages = fdp.ConsumeIntegralInRange<size_t>(1, kMaxMessages);
  for (size_t i = 0; i < n_messages && fdp.remaining_bytes() > 0; ++i)
  {
    message msg = build_message(fdp);
    const size_t n_mutations = fdp.ConsumeIntegralInRange<size_t>(0, 3);
    for (size_t m = 0; m < n_mutations && !msg.blob.empty(); ++m)
      msg.blob[fdp.ConsumeIntegralInRange<size_t>(0, msg.blob.size() - 1)] = fdp.ConsumeIntegral<char>();
    if (fdp.ConsumeIntegralInRange<int>(0, 15) == 0)
      msg.is_notify = !msg.is_notify;

    const epee::span<const uint8_t> in{reinterpret_cast<const uint8_t *>(msg.blob.data()), msg.blob.size()};
    if (msg.is_notify)
    {
      handler.notify(msg.command, in, context);
    }
    else
    {
      epee::byte_stream out;
      if (handler.invoke(msg.command, in, out, context) == 1)
        check_response(msg.command, out);
    }
  }
END_SIMPLE_FUZZER()
