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

// Fuzzes wallet2::refresh against a fake daemon. The wallet talks to an in-process HTTP
// client that serves /getblocks.bin, /gethashes.bin, /get_transaction_pool_hashes.bin and
// /gettransactions from a small chain and pool built from the fuzz input (outputs paying the
// wallet, spends of its outputs, reorgs, pool txs being mined or dropped). Responses are
// mostly honest; sometimes the daemon lies. After a round where every chain response was
// honest and refresh succeeded, the wallet must end on the daemon's top block.

#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "net/abstract_http_client.h"
#include "net/http_base.h"
#include "ringct/rctOps.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "serialization/binary_utils.h"
#include "storages/portable_storage_template_helper.h"
#include "string_tools.h"
#include "wallet/wallet2.h"
#include "wallet/wallet2_basic/wallet2_serialization.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

class wallet_accessor_test
{
public:
  // State that lives outside the wallet cache, so the snapshot does not reset it.
  static void reset_session(tools::wallet2 &w)
  {
    w.m_first_refresh_done = false;
    w.m_pool_info_query_time = 0;
    w.m_node_rpc_proxy.invalidate();
  }

  static crypto::hash block_hash(const tools::wallet2 &w, uint64_t height)
  {
    return w.m_blockchain[height];
  }
};

static constexpr size_t kMaxRounds = 3;
static constexpr size_t kMaxNewBlocks = 4;
static constexpr size_t kMaxTxsPerBlock = 2;
static constexpr size_t kMaxPoolAdd = 2;
static constexpr size_t kMaxOuts = 4;
static constexpr size_t kMaxIns = 2;
static constexpr size_t kCallsPerRound = 24;
// Subaddress lookahead is (2, 3), so indices up to (2, 4) include unknown ones.
static constexpr uint32_t kMaxMajor = 2;
static constexpr uint32_t kMaxMinor = 4;

using get_blocks_t = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST;

static tools::wallet2 *wallet = NULL;
static cryptonote::account_public_address foreign_address;
static std::string snapshot;

//----------------------------------------------------------------------------------------------------------------------
// Fake daemon state
//----------------------------------------------------------------------------------------------------------------------
struct daemon_tx
{
  cryptonote::blobdata base_blob;
  crypto::hash prunable_hash;
  crypto::hash txid;
  size_t n_outs;
  bool double_spend_seen;
};

struct daemon_block
{
  cryptonote::block_complete_entry entry;
  get_blocks_t::block_output_indices indices;
  crypto::hash id;
};

struct fake_node
{
  std::mutex lock;
  FuzzedDataProvider *fdp = NULL;
  size_t calls_left = 0;
  // Cleared when a chain response (getblocks/gethashes) deviates from what an honest node sends.
  bool honest = true;
  std::vector<daemon_block> chain;
  std::vector<daemon_tx> pool;
  std::vector<crypto::hash> removed_pool;
  uint64_t next_global_index = 0;
  epee::net_utils::http::http_response_info response;
};

static fake_node node;
static daemon_block genesis;

//----------------------------------------------------------------------------------------------------------------------
// Transaction and block building
//----------------------------------------------------------------------------------------------------------------------
struct recipient
{
  cryptonote::account_public_address addr;
  bool is_subaddress;
};

// Small seed space on purpose: tx keys repeat across txs, producing duplicate output keys.
static crypto::secret_key consume_scalar(FuzzedDataProvider &fdp)
{
  const std::string seed = fdp.ConsumeBytesAsString(2);
  crypto::secret_key k;
  crypto::hash_to_scalar(seed.data(), seed.size(), k);
  return k;
}

static crypto::hash consume_hash(FuzzedDataProvider &fdp)
{
  crypto::hash h = crypto::null_hash;
  const std::string bytes = fdp.ConsumeBytesAsString(fdp.ConsumeBool() ? 1 : sizeof(h));
  memcpy(&h, bytes.data(), bytes.size());
  return h;
}

static recipient consume_recipient(FuzzedDataProvider &fdp)
{
  switch (fdp.ConsumeIntegralInRange<int>(0, 2))
  {
    case 0:
      return {wallet->get_account().get_keys().m_account_address, false};
    case 1:
    {
      const cryptonote::subaddress_index idx{fdp.ConsumeIntegralInRange<uint32_t>(0, kMaxMajor), fdp.ConsumeIntegralInRange<uint32_t>(0, kMaxMinor)};
      return {wallet->get_subaddress(idx), !idx.is_zero()};
    }
    default:
      return {foreign_address, false};
  }
}

static crypto::public_key tx_pub_key_for(const crypto::secret_key &r, const recipient &to)
{
  if (to.is_subaddress)
    return rct::rct2pk(rct::scalarmultKey(rct::pk2rct(to.addr.m_spend_public_key), rct::sk2rct(r)));
  return rct::rct2pk(rct::scalarmultBase(rct::sk2rct(r)));
}

static cryptonote::transaction build_tx(FuzzedDataProvider &fdp, bool miner_tx, uint64_t height)
{
  cryptonote::transaction tx;
  tx.version = fdp.ConsumeIntegralInRange<int>(0, 7) == 0 ? 1 : 2;
  tx.unlock_time = miner_tx ? height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW : 0;
  if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
    tx.unlock_time = fdp.ConsumeIntegral<uint64_t>();

  static const uint8_t rct_types[] = {rct::RCTTypeNull, rct::RCTTypeBulletproof2, rct::RCTTypeCLSAG, rct::RCTTypeBulletproofPlus};
  const uint8_t rct_type = miner_tx || tx.version == 1 ? (uint8_t)rct::RCTTypeNull : fdp.PickValueInArray(rct_types);
  const bool explicit_amounts = rct_type == rct::RCTTypeNull;
  tx.rct_signatures.type = rct_type;
  if (!explicit_amounts)
    tx.rct_signatures.txnFee = fdp.ConsumeIntegralInRange<uint64_t>(0, 1000000000);

  if (miner_tx)
  {
    cryptonote::txin_gen gen;
    gen.height = height;
    tx.vin.push_back(gen);
  }
  else
  {
    const size_t n_ins = fdp.ConsumeIntegralInRange<size_t>(1, kMaxIns);
    for (size_t i = 0; i < n_ins; ++i)
    {
      cryptonote::txin_to_key in;
      in.amount = 0;
      const size_t n_offsets = fdp.ConsumeIntegralInRange<size_t>(1, 3);
      for (size_t j = 0; j < n_offsets; ++j)
        in.key_offsets.push_back(fdp.ConsumeIntegralInRange<uint64_t>(0, 16));
      // Spend outputs the wallet received in an earlier round of this input.
      const size_t n_transfers = wallet->get_num_transfer_details();
      if (n_transfers > 0 && fdp.ConsumeBool())
        in.k_image = wallet->get_transfer_details(fdp.ConsumeIntegralInRange<size_t>(0, n_transfers - 1)).m_key_image;
      else
        in.k_image = rct::rct2ki(rct::scalarmultBase(rct::sk2rct(consume_scalar(fdp))));
      tx.vin.push_back(in);
    }
  }

  const crypto::secret_key r = consume_scalar(fdp);
  const recipient primary_to = consume_recipient(fdp);
  const bool use_additional = fdp.ConsumeBool();
  std::vector<crypto::public_key> additional_pub_keys;

  const size_t n_outs = fdp.ConsumeIntegralInRange<size_t>(1, kMaxOuts);
  for (size_t i = 0; i < n_outs; ++i)
  {
    const recipient to = i == 0 ? primary_to : consume_recipient(fdp);
    crypto::secret_key r_out = r;
    if (use_additional)
    {
      r_out = consume_scalar(fdp);
      additional_pub_keys.push_back(tx_pub_key_for(r_out, to));
    }

    crypto::key_derivation derivation;
    crypto::public_key out_key;
    FUZZ_CHECK(crypto::generate_key_derivation(to.addr.m_view_public_key, r_out, derivation));
    FUZZ_CHECK(crypto::derive_public_key(derivation, i, to.addr.m_spend_public_key, out_key));

    cryptonote::tx_out out;
    if (fdp.ConsumeBool())
    {
      crypto::view_tag view_tag;
      crypto::derive_view_tag(derivation, i, view_tag);
      if (fdp.ConsumeIntegralInRange<int>(0, 15) == 0)
        view_tag.data ^= 1;
      out.target = cryptonote::txout_to_tagged_key(out_key, view_tag);
    }
    else
    {
      out.target = cryptonote::txout_to_key(out_key);
    }

    const uint64_t amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 1000000000000000);
    if (explicit_amounts)
    {
      out.amount = amount;
    }
    else
    {
      out.amount = 0;
      rct::key shared_secret;
      crypto::derivation_to_scalar(derivation, i, reinterpret_cast<crypto::ec_scalar&>(shared_secret));
      rct::ecdhTuple ecdh;
      ecdh.amount = rct::d2h(amount);
      ecdh.mask = rct::genCommitmentMask(shared_secret);
      const rct::key commitment = fdp.ConsumeIntegralInRange<int>(0, 15) == 0
          ? rct::scalarmultBase(rct::sk2rct(consume_scalar(fdp)))
          : rct::commit(amount, ecdh.mask);
      rct::ecdhEncode(ecdh, shared_secret, true);
      tx.rct_signatures.ecdhInfo.push_back(ecdh);
      rct::ctkey out_pk;
      out_pk.dest = rct::pk2rct(out_key);
      out_pk.mask = commitment;
      tx.rct_signatures.outPk.push_back(out_pk);
    }
    tx.vout.push_back(out);
  }

  if (fdp.ConsumeIntegralInRange<int>(0, 15) != 0)
    cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key_for(r, primary_to));
  if (!additional_pub_keys.empty() && fdp.ConsumeIntegralInRange<int>(0, 7) != 0)
    cryptonote::add_additional_tx_pub_keys_to_extra(tx.extra, additional_pub_keys);
  if (fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
  {
    crypto::hash8 payment_id8;
    const std::string bytes = fdp.ConsumeBytesAsString(sizeof(payment_id8));
    memset(&payment_id8, 0, sizeof(payment_id8));
    memcpy(&payment_id8, bytes.data(), bytes.size());
    cryptonote::blobdata nonce;
    cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(nonce, payment_id8);
    cryptonote::add_extra_nonce_to_tx_extra(tx.extra, nonce);
  }
  return tx;
}

// What a pruning daemon sends: the tx base, the hash of the prunable part, and the txid.
static bool make_daemon_tx(FuzzedDataProvider &fdp, cryptonote::transaction tx, daemon_tx &out)
{
  std::ostringstream ss;
  binary_archive<true> ar(ss);
  if (!tx.serialize_base(ar))
    return false;
  out.base_blob = ss.str();
  out.prunable_hash = consume_hash(fdp);
  if (tx.version > 1)
  {
    if (!cryptonote::get_pruned_transaction_hash(tx, out.prunable_hash, out.txid))
      return false;
  }
  else
  {
    out.txid = cryptonote::get_transaction_prefix_hash(tx);
  }
  out.n_outs = tx.vout.size();
  out.double_spend_seen = fdp.ConsumeIntegralInRange<int>(0, 7) == 0;
  return true;
}

static get_blocks_t::tx_output_indices next_output_indices(size_t n_outs)
{
  get_blocks_t::tx_output_indices indices;
  for (size_t i = 0; i < n_outs; ++i)
    indices.indices.push_back(node.next_global_index++);
  return indices;
}

static void append_block(FuzzedDataProvider &fdp)
{
  static const uint8_t versions[] = {1, 1, 2, HF_VERSION_VIEW_TAGS, HF_VERSION_VIEW_TAGS + 1};
  const uint64_t height = node.chain.size();

  cryptonote::block b;
  b.major_version = fdp.ConsumeIntegralInRange<int>(0, 15) == 0 ? fdp.ConsumeIntegralInRange<uint8_t>(1, 20) : fdp.PickValueInArray(versions);
  b.minor_version = b.major_version;
  b.timestamp = fdp.ConsumeIntegral<uint32_t>();
  b.prev_id = node.chain.back().id;
  b.nonce = fdp.ConsumeIntegral<uint32_t>();
  b.miner_tx = build_tx(fdp, true, height);

  daemon_block db;
  db.indices.indices.push_back(next_output_indices(b.miner_tx.vout.size()));

  const size_t n_txs = fdp.ConsumeIntegralInRange<size_t>(0, kMaxTxsPerBlock);
  for (size_t t = 0; t < n_txs; ++t)
  {
    daemon_tx tx;
    if (!node.pool.empty() && fdp.ConsumeBool())
    {
      // Mine a pool tx: it leaves the pool and shows up in the block.
      const size_t idx = fdp.ConsumeIntegralInRange<size_t>(0, node.pool.size() - 1);
      tx = node.pool[idx];
      node.removed_pool.push_back(tx.txid);
      node.pool.erase(node.pool.begin() + idx);
    }
    else if (!make_daemon_tx(fdp, build_tx(fdp, false, height), tx))
    {
      continue;
    }
    b.tx_hashes.push_back(tx.txid);
    db.entry.txs.emplace_back(tx.base_blob, tx.prunable_hash);
    db.indices.indices.push_back(next_output_indices(tx.n_outs));
  }

  db.entry.pruned = true;
  db.entry.block = cryptonote::block_to_blob(b);
  db.entry.block_weight = db.entry.block.size();
  db.id = cryptonote::get_block_hash(b);
  node.chain.push_back(std::move(db));
}

// Between refresh rounds the node changes: pool txs arrive or are dropped, blocks are mined,
// and sometimes the top of the chain is replaced.
static void advance_node(FuzzedDataProvider &fdp)
{
  const size_t n_pool = fdp.ConsumeIntegralInRange<size_t>(0, kMaxPoolAdd);
  for (size_t i = 0; i < n_pool; ++i)
  {
    daemon_tx tx;
    if (make_daemon_tx(fdp, build_tx(fdp, false, node.chain.size()), tx))
      node.pool.push_back(tx);
  }
  if (!node.pool.empty() && fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
  {
    const size_t idx = fdp.ConsumeIntegralInRange<size_t>(0, node.pool.size() - 1);
    node.removed_pool.push_back(node.pool[idx].txid);
    node.pool.erase(node.pool.begin() + idx);
  }

  size_t popped = 0;
  if (node.chain.size() > 1 && fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
  {
    popped = fdp.ConsumeIntegralInRange<size_t>(1, node.chain.size() - 1);
    node.chain.resize(node.chain.size() - popped);
  }
  size_t n_new = fdp.ConsumeIntegralInRange<size_t>(0, kMaxNewBlocks);
  if (n_new < popped)
  {
    // An honest chain never gets shorter, so the wallet would keep its stale tail.
    if (fdp.ConsumeBool())
      n_new = popped;
    else
      node.honest = false;
  }
  for (size_t i = 0; i < n_new; ++i)
    append_block(fdp);
}

//----------------------------------------------------------------------------------------------------------------------
// Fake daemon RPC
//----------------------------------------------------------------------------------------------------------------------
static epee::span<const uint8_t> as_span(const boost::string_ref body)
{
  return epee::span<const uint8_t>(reinterpret_cast<const uint8_t*>(body.data()), body.size());
}

// Highest block the wallet and node agree on; block_ids are ordered from the wallet's top.
static size_t common_ancestor(const std::list<crypto::hash> &block_ids)
{
  for (const crypto::hash &id : block_ids)
    for (size_t h = node.chain.size(); h-- > 0;)
      if (node.chain[h].id == id)
        return h;
  return 0;
}

static void mutate_bytes(FuzzedDataProvider &fdp, std::string &blob)
{
  if (blob.empty() || fdp.ConsumeBool())
  {
    blob.resize(fdp.ConsumeIntegralInRange<size_t>(0, blob.size()));
    return;
  }
  const size_t n = fdp.ConsumeIntegralInRange<size_t>(1, 4);
  for (size_t i = 0; i < n; ++i)
    blob[fdp.ConsumeIntegralInRange<size_t>(0, blob.size() - 1)] ^= fdp.ConsumeIntegralInRange<uint8_t>(1, 255);
}

static void add_pool_info(FuzzedDataProvider &fdp, get_blocks_t::response &res)
{
  res.pool_info_extent = fdp.ConsumeIntegralInRange<uint8_t>(get_blocks_t::NONE, get_blocks_t::FULL);
  if (res.pool_info_extent == get_blocks_t::NONE)
    return;
  for (const daemon_tx &tx : node.pool)
  {
    // Restricted daemons only send some txs inline and leave the rest to /gettransactions.
    if (fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
    {
      res.remaining_added_pool_txids.push_back(tx.txid);
      continue;
    }
    get_blocks_t::pool_tx_info info;
    info.tx_hash = tx.txid;
    info.tx_blob = tx.base_blob;
    info.double_spend_seen = tx.double_spend_seen;
    res.added_pool_txs.push_back(std::move(info));
  }
  if (res.pool_info_extent == get_blocks_t::INCREMENTAL)
    res.removed_pool_txids = node.removed_pool;
  node.removed_pool.clear();
}

static bool serve_get_blocks(FuzzedDataProvider &fdp, const boost::string_ref body, std::string &out)
{
  get_blocks_t::request req;
  FUZZ_CHECK(epee::serialization::load_t_from_binary(req, as_span(body)));

  get_blocks_t::response res;
  res.status = CORE_RPC_STATUS_OK;
  const int lie = fdp.ConsumeIntegralInRange<int>(0, 31);
  if (lie >= 1 && lie <= 10)
    node.honest = false;

  size_t start = common_ancestor(req.block_ids);
  if (lie == 1)
    start = fdp.ConsumeIntegralInRange<size_t>(0, node.chain.size() + 1);
  const size_t available = start < node.chain.size() ? node.chain.size() - start : 0;
  // Honest nodes may also page the reply.
  const size_t count = available == 0 ? 0 : fdp.ConsumeIntegralInRange<size_t>(1, available);
  for (size_t h = start; h < start + count; ++h)
  {
    res.blocks.push_back(node.chain[h].entry);
    res.output_indices.push_back(node.chain[h].indices);
  }
  res.start_height = start;
  res.current_height = node.chain.size();
  res.top_block_hash = node.chain.back().id;
  res.daemon_time = fdp.ConsumeIntegral<uint32_t>();
  if (req.requested_info != get_blocks_t::BLOCKS_ONLY || lie == 2)
    add_pool_info(fdp, res);

  switch (lie)
  {
    case 3:
      res.current_height = fdp.ConsumeIntegralInRange<uint64_t>(0, node.chain.size() + 4);
      break;
    case 4:
      if (!res.output_indices.empty() && fdp.ConsumeBool())
        res.output_indices.pop_back();
      else
        res.output_indices.emplace_back();
      break;
    case 5:
      if (!res.output_indices.empty())
      {
        auto &indices = res.output_indices[fdp.ConsumeIntegralInRange<size_t>(0, res.output_indices.size() - 1)].indices;
        if (!indices.empty() && fdp.ConsumeBool())
        {
          auto &tx_indices = indices[fdp.ConsumeIntegralInRange<size_t>(0, indices.size() - 1)].indices;
          if (!tx_indices.empty() && fdp.ConsumeBool())
            tx_indices[0] = fdp.ConsumeIntegralInRange<uint64_t>(0, node.next_global_index);
          else
            tx_indices.push_back(fdp.ConsumeIntegralInRange<uint64_t>(0, node.next_global_index));
        }
        else if (!indices.empty())
        {
          indices.pop_back();
        }
      }
      break;
    case 6:
      if (!res.blocks.empty())
        mutate_bytes(fdp, res.blocks[fdp.ConsumeIntegralInRange<size_t>(0, res.blocks.size() - 1)].block);
      break;
    case 7:
      if (!res.blocks.empty())
      {
        auto &txs = res.blocks[fdp.ConsumeIntegralInRange<size_t>(0, res.blocks.size() - 1)].txs;
        if (!txs.empty() && fdp.ConsumeBool())
          mutate_bytes(fdp, txs[fdp.ConsumeIntegralInRange<size_t>(0, txs.size() - 1)].blob);
        else if (!txs.empty())
          txs.pop_back();
      }
      break;
    case 8:
      if (res.blocks.size() >= 2)
      {
        std::swap(res.blocks[0], res.blocks[1]);
        std::swap(res.output_indices[0], res.output_indices[1]);
      }
      break;
    case 9:
    {
      static const char *const statuses[] = {CORE_RPC_STATUS_BUSY, "Failed", ""};
      res.status = fdp.PickValueInArray(statuses);
      break;
    }
    case 10:
      return false;
    default:
      break;
  }

  epee::byte_slice slice;
  FUZZ_CHECK(epee::serialization::store_t_to_binary(res, slice));
  out.assign(reinterpret_cast<const char*>(slice.data()), slice.size());
  if (fdp.ConsumeIntegralInRange<int>(0, 63) == 0)
  {
    node.honest = false;
    mutate_bytes(fdp, out);
  }
  return true;
}

static bool serve_get_hashes(FuzzedDataProvider &fdp, const boost::string_ref body, std::string &out)
{
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::request req;
  FUZZ_CHECK(epee::serialization::load_t_from_binary(req, as_span(body)));

  cryptonote::COMMAND_RPC_GET_HASHES_FAST::response res;
  res.status = CORE_RPC_STATUS_OK;
  res.start_height = common_ancestor(req.block_ids);
  for (size_t h = res.start_height; h < node.chain.size(); ++h)
    res.m_block_ids.push_back(node.chain[h].id);
  res.current_height = node.chain.size();
  if (fdp.ConsumeIntegralInRange<int>(0, 15) == 0)
  {
    node.honest = false;
    if (fdp.ConsumeBool())
      res.start_height = fdp.ConsumeIntegralInRange<uint64_t>(0, node.chain.size() + 1);
    else
      res.m_block_ids.push_back(consume_hash(fdp));
  }

  epee::byte_slice slice;
  FUZZ_CHECK(epee::serialization::store_t_to_binary(res, slice));
  out.assign(reinterpret_cast<const char*>(slice.data()), slice.size());
  return true;
}

static bool serve_pool_hashes(FuzzedDataProvider &fdp, std::string &out)
{
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::response res;
  res.status = CORE_RPC_STATUS_OK;
  for (const daemon_tx &tx : node.pool)
    res.tx_hashes.push_back(tx.txid);
  if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
    res.tx_hashes.push_back(consume_hash(fdp));
  return epee::serialization::store_t_to_json(res, out);
}

static bool serve_get_transactions(FuzzedDataProvider &fdp, const boost::string_ref body, std::string &out)
{
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  FUZZ_CHECK(epee::serialization::load_t_from_json(req, std::string(body.data(), body.size())));

  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;
  res.status = CORE_RPC_STATUS_OK;
  for (const std::string &hash : req.txs_hashes)
  {
    const daemon_tx *found = NULL;
    for (const daemon_tx &tx : node.pool)
      if (epee::string_tools::pod_to_hex(tx.txid) == hash)
        found = &tx;
    if (!found)
    {
      res.missed_tx.push_back(hash);
      continue;
    }
    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::entry e;
    e.tx_hash = hash;
    e.pruned_as_hex = epee::string_tools::buff_to_hex_nodelimer(found->base_blob);
    e.prunable_hash = epee::string_tools::pod_to_hex(found->prunable_hash);
    e.in_pool = true;
    e.double_spend_seen = found->double_spend_seen;
    e.relayed = true;
    e.received_timestamp = fdp.ConsumeIntegral<uint32_t>();
    e.block_height = 0;
    e.confirmations = 0;
    e.block_timestamp = 0;
    switch (fdp.ConsumeIntegralInRange<int>(0, 15))
    {
      case 0:
        e.prunable_hash = epee::string_tools::pod_to_hex(consume_hash(fdp));
        break;
      case 1:
        e.in_pool = false;
        break;
      case 2:
        e.pruned_as_hex.resize(fdp.ConsumeIntegralInRange<size_t>(0, e.pruned_as_hex.size()));
        break;
      case 3:
        continue;
      default:
        break;
    }
    res.txs.push_back(std::move(e));
  }
  return epee::serialization::store_t_to_json(res, out);
}

static bool serve(const boost::string_ref uri, const boost::string_ref body, const epee::net_utils::http::http_response_info **response_info)
{
  std::lock_guard<std::mutex> lock(node.lock);
  const bool chain_call = uri == "/getblocks.bin" || uri == "/gethashes.bin";
  if (!node.fdp || node.calls_left == 0)
  {
    if (chain_call)
      node.honest = false;
    return false;
  }
  --node.calls_left;
  FuzzedDataProvider &fdp = *node.fdp;

  std::string out;
  bool ok = false;
  if (uri == "/getblocks.bin")
    ok = serve_get_blocks(fdp, body, out);
  else if (uri == "/gethashes.bin")
    ok = serve_get_hashes(fdp, body, out);
  else if (uri == "/get_transaction_pool_hashes.bin")
    ok = serve_pool_hashes(fdp, out);
  else if (uri == "/gettransactions")
    ok = serve_get_transactions(fdp, body, out);
  if (!ok)
  {
    if (chain_call)
      node.honest = false;
    return false;
  }

  node.response.clear();
  node.response.m_response_code = 200;
  if (fdp.ConsumeIntegralInRange<int>(0, 63) == 0)
  {
    node.response.m_response_code = 500;
    if (chain_call)
      node.honest = false;
  }
  node.response.m_body = std::move(out);
  if (response_info)
    *response_info = &node.response;
  return true;
}

class fake_http_client : public epee::net_utils::http::abstract_http_client
{
public:
  void set_server(std::string, std::string, boost::optional<epee::net_utils::http::login>, epee::net_utils::ssl_options_t) override {}
  void set_auto_connect(bool) override {}
  bool connect(std::chrono::milliseconds) override { return true; }
  bool disconnect() override { return true; }
  bool is_connected(bool *ssl) override
  {
    if (ssl)
      *ssl = false;
    return true;
  }
  bool invoke(const boost::string_ref uri, const boost::string_ref, const boost::string_ref body, std::chrono::milliseconds,
      const epee::net_utils::http::http_response_info **ppresponse_info, const epee::net_utils::http::fields_list &) override
  {
    return serve(uri, body, ppresponse_info);
  }
  bool invoke_get(const boost::string_ref, std::chrono::milliseconds, const std::string &,
      const epee::net_utils::http::http_response_info **, const epee::net_utils::http::fields_list &) override
  {
    return false;
  }
  uint64_t get_bytes_sent() const override { return 0; }
  uint64_t get_bytes_received() const override { return 0; }
};

class fake_http_client_factory : public epee::net_utils::http::http_client_factory
{
public:
  std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
  {
    return std::unique_ptr<epee::net_utils::http::abstract_http_client>(new fake_http_client());
  }
};

// Detaches the fuzz input from the fake node however the input ends.
struct node_input_guard
{
  explicit node_input_guard(FuzzedDataProvider &fdp)
  {
    std::lock_guard<std::mutex> lock(node.lock);
    node.fdp = &fdp;
  }
  ~node_input_guard()
  {
    std::lock_guard<std::mutex> lock(node.lock);
    node.fdp = NULL;
  }
};

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true,
      std::unique_ptr<epee::net_utils::http::http_client_factory>(new fake_http_client_factory()));
  wallet = &local_wallet;

  static const char * const spendkey_hex = "f285d4ac9e66271256fc7cde0d3d6b36f66efff6ccd766706c408e86f4997a0d";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);

  wallet->init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  wallet->set_subaddress_lookahead(2, 3);
  wallet->generate("", "", spendkey, true, false);
  // Scan every block regardless of its timestamp.
  wallet->get_account().set_createtime(0);
  wallet->set_refresh_from_block_height(0);

  static const char * const foreign_spendkey_hex = "0b4f47697ec99c3de6579304e5f25c68b07afbe55b71d99620bf6cbf4e45a80f";
  crypto::secret_key foreign_spendkey;
  epee::string_tools::hex_to_pod(foreign_spendkey_hex, foreign_spendkey);
  cryptonote::account_base foreign;
  foreign.generate(foreign_spendkey, true, false);
  foreign_address = foreign.get_keys().m_account_address;

  cryptonote::block genesis_block;
  FUZZ_CHECK(cryptonote::generate_genesis_block(genesis_block, config::testnet::GENESIS_TX, config::testnet::GENESIS_NONCE));
  genesis.entry.pruned = true;
  genesis.entry.block = cryptonote::block_to_blob(genesis_block);
  genesis.entry.block_weight = genesis.entry.block.size();
  genesis.id = cryptonote::get_block_hash(genesis_block);
  get_blocks_t::tx_output_indices genesis_indices;
  for (size_t i = 0; i < genesis_block.miner_tx.vout.size(); ++i)
    genesis_indices.indices.push_back(i);
  genesis.indices.indices.push_back(genesis_indices);
  FUZZ_CHECK(wallet->get_blockchain_current_height() == 1);
  FUZZ_CHECK(wallet_accessor_test::block_hash(*wallet, 0) == genesis.id);

  if (!::serialization::dump_binary(*wallet, snapshot))
  {
    std::cerr << "failed to snapshot wallet state" << std::endl;
    return 1;
  }
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  // Refresh runs on the threadpool; create the logging storage before any worker logs.
  el::base::Storage::getELPP();

  binary_archive<false> restore{epee::strspan<std::uint8_t>(snapshot)};
  if (!::serialization::serialize(restore, *wallet))
    abort();
  wallet_accessor_test::reset_session(*wallet);

  FuzzedDataProvider fdp(buf, len);
  static const tools::wallet2::RefreshType refresh_types[] = {tools::wallet2::RefreshFull, tools::wallet2::RefreshOptimizeCoinbase, tools::wallet2::RefreshNoCoinbase};
  wallet->set_refresh_type(fdp.PickValueInArray(refresh_types));
  wallet->track_uses(fdp.ConsumeBool());
  wallet->allow_mismatched_daemon_version(fdp.ConsumeIntegralInRange<int>(0, 7) != 0);
  wallet->max_reorg_depth(fdp.ConsumeIntegralInRange<int>(0, 7) == 0 ? fdp.ConsumeIntegralInRange<uint64_t>(0, 3) : ORPHANED_BLOCKS_MAX_COUNT);
  wallet->set_refresh_from_block_height(fdp.ConsumeIntegralInRange<int>(0, 3) == 0 ? fdp.ConsumeIntegralInRange<uint64_t>(0, 8) : 0);

  node.chain.assign(1, genesis);
  node.pool.clear();
  node.removed_pool.clear();
  node.next_global_index = genesis.indices.indices[0].indices.size();
  node_input_guard guard(fdp);

  const size_t rounds = fdp.ConsumeIntegralInRange<size_t>(1, kMaxRounds);
  for (size_t round = 0; round < rounds && fdp.remaining_bytes() > 0; ++round)
  {
    {
      std::lock_guard<std::mutex> lock(node.lock);
      node.honest = true;
      node.calls_left = kCallsPerRound;
    }
    advance_node(fdp);

    const bool trusted_daemon = fdp.ConsumeBool();
    const bool check_pool = fdp.ConsumeBool();
    const bool try_incremental = fdp.ConsumeBool();
    uint64_t blocks_fetched = 0;
    bool received_money = false;
    bool refreshed = true;
    try
    {
      wallet->refresh(trusted_daemon, 0, blocks_fetched, received_money, check_pool, try_incremental);
    }
    catch (const std::exception &)
    {
      refreshed = false;
    }

    std::lock_guard<std::mutex> lock(node.lock);
    if (refreshed && node.honest)
    {
      FUZZ_CHECK(wallet->get_blockchain_current_height() == node.chain.size());
      FUZZ_CHECK(wallet_accessor_test::block_hash(*wallet, node.chain.size() - 1) == node.chain.back().id);
    }
  }
  wallet->balance_all(false);
END_SIMPLE_FUZZER()
