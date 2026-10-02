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

// Fuzzes wallet2::process_new_transaction() with daemon-supplied transactions.
// The harness computes output keys/view tags/RingCT amounts the wallet can recognise;
// every structural choice and value comes from the fuzz input.

#include "include_base_utils.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctOps.h"
#include "wallet/wallet2.h"
#include "wallet/wallet2_basic/wallet2_serialization.h"
#include "serialization/binary_utils.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

class wallet_accessor_test
{
public:
  static void process_new_transaction(tools::wallet2 &w, const crypto::hash &txid, const cryptonote::transaction &tx,
      const std::vector<uint64_t> &o_indices, uint64_t height, uint8_t block_version, uint64_t ts,
      bool miner_tx, bool pool, bool double_spend_seen, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache)
  {
    w.process_new_transaction(txid, tx, o_indices, height, block_version, ts, miner_tx, pool, double_spend_seen,
        tools::wallet2::tx_cache_data{}, output_tracker_cache);
  }
};

static constexpr size_t kMaxTxs = 4;
static constexpr size_t kMaxOuts = 6;
static constexpr size_t kMaxIns = 3;
// Subaddress lookahead is (2, 3), so indices up to (2, 4) include unknown ones.
static constexpr uint32_t kMaxMajor = 2;
static constexpr uint32_t kMaxMinor = 4;

static tools::wallet2 *wallet = NULL;
static cryptonote::account_public_address foreign_address;
static std::string snapshot;

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
  tx.version = fdp.ConsumeIntegralInRange<size_t>(1, 2);
  tx.unlock_time = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint64_t>();

  const bool explicit_amounts = miner_tx || tx.version == 1;
  const uint8_t rct_type = explicit_amounts ? rct::RCTTypeNull : fdp.ConsumeIntegralInRange<uint8_t>(0, rct::RCTTypeBulletproofPlus + 1);
  const bool ecdh_v2 = rct_type == rct::RCTTypeBulletproof2 || rct_type == rct::RCTTypeCLSAG || rct_type == rct::RCTTypeBulletproofPlus;
  tx.rct_signatures.type = rct_type;
  if (!explicit_amounts)
    tx.rct_signatures.txnFee = fdp.ConsumeIntegral<uint64_t>();

  // Inputs, possibly spending outputs the wallet received earlier in this run.
  if (miner_tx)
  {
    cryptonote::txin_gen gen;
    gen.height = height;
    tx.vin.push_back(gen);
  }
  else
  {
    const size_t n_ins = fdp.ConsumeIntegralInRange<size_t>(0, kMaxIns);
    for (size_t i = 0; i < n_ins; ++i)
    {
      cryptonote::txin_to_key in;
      in.amount = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint64_t>();
      const size_t n_offsets = fdp.ConsumeIntegralInRange<size_t>(1, 3);
      for (size_t j = 0; j < n_offsets; ++j)
        in.key_offsets.push_back(fdp.ConsumeIntegralInRange<uint64_t>(0, 16));
      const size_t n_transfers = wallet->get_num_transfer_details();
      if (n_transfers > 0 && fdp.ConsumeBool())
        in.k_image = wallet->get_transfer_details(fdp.ConsumeIntegralInRange<size_t>(0, n_transfers - 1)).m_key_image;
      else
        in.k_image = rct::rct2ki(rct::scalarmultBase(rct::sk2rct(consume_scalar(fdp))));
      tx.vin.push_back(in);
    }
  }

  // Outputs.
  const crypto::secret_key r = consume_scalar(fdp);
  const recipient primary_to = consume_recipient(fdp);
  const crypto::public_key tx_pub_key = tx_pub_key_for(r, primary_to);
  const bool use_additional = fdp.ConsumeBool();
  std::vector<crypto::public_key> additional_pub_keys;

  const size_t n_outs = fdp.ConsumeIntegralInRange<size_t>(0, kMaxOuts);
  for (size_t i = 0; i < n_outs; ++i)
  {
    const recipient to = consume_recipient(fdp);
    crypto::secret_key r_out = r;
    if (use_additional)
    {
      r_out = consume_scalar(fdp);
      additional_pub_keys.push_back(tx_pub_key_for(r_out, to));
    }

    crypto::key_derivation derivation;
    crypto::public_key out_key;
    if (!crypto::generate_key_derivation(to.addr.m_view_public_key, r_out, derivation) ||
        !crypto::derive_public_key(derivation, i, to.addr.m_spend_public_key, out_key))
      throw std::runtime_error("failed to derive output key");

    cryptonote::tx_out out;
    if (fdp.ConsumeBool())
    {
      crypto::view_tag view_tag;
      crypto::derive_view_tag(derivation, i, view_tag);
      if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
        view_tag.data ^= 1;
      out.target = cryptonote::txout_to_tagged_key(out_key, view_tag);
    }
    else
    {
      out.target = cryptonote::txout_to_key(out_key);
    }

    const uint64_t amount = fdp.ConsumeIntegral<uint64_t>();
    if (explicit_amounts)
    {
      out.amount = amount;
    }
    else
    {
      out.amount = fdp.ConsumeIntegralInRange<int>(0, 15) == 0 ? amount : 0;
      rct::key shared_secret;
      crypto::derivation_to_scalar(derivation, i, reinterpret_cast<crypto::ec_scalar&>(shared_secret));
      rct::ecdhTuple ecdh;
      ecdh.amount = rct::d2h(amount);
      ecdh.mask = ecdh_v2 ? rct::genCommitmentMask(shared_secret) : rct::sk2rct(consume_scalar(fdp));
      const rct::key commitment = fdp.ConsumeIntegralInRange<int>(0, 7) == 0
          ? rct::scalarmultBase(rct::sk2rct(consume_scalar(fdp)))
          : rct::commit(amount, ecdh.mask);
      rct::ecdhEncode(ecdh, shared_secret, ecdh_v2);
      tx.rct_signatures.ecdhInfo.push_back(ecdh);
      rct::ctkey out_pk;
      out_pk.dest = rct::pk2rct(out_key);
      out_pk.mask = commitment;
      tx.rct_signatures.outPk.push_back(out_pk);
    }
    tx.vout.push_back(out);
  }
  if (!tx.rct_signatures.ecdhInfo.empty() && fdp.ConsumeIntegralInRange<int>(0, 15) == 0)
    tx.rct_signatures.ecdhInfo.pop_back();

  // Extra: tx pubkeys, additional pubkeys, payment id nonce, trailing garbage.
  switch (fdp.ConsumeIntegralInRange<int>(0, 3))
  {
    case 0:
      break;
    case 1:
      cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key);
      break;
    case 2:
      cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key);
      cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key_for(consume_scalar(fdp), consume_recipient(fdp)));
      break;
    default:
    {
      crypto::public_key raw;
      const std::string bytes = fdp.ConsumeBytesAsString(sizeof(raw));
      memset(&raw, 0, sizeof(raw));
      memcpy(&raw, bytes.data(), bytes.size());
      cryptonote::add_tx_pub_key_to_extra(tx.extra, raw);
      cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key);
      break;
    }
  }
  if (!additional_pub_keys.empty() && fdp.ConsumeIntegralInRange<int>(0, 7) != 0)
  {
    if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
      additional_pub_keys.pop_back();
    cryptonote::add_additional_tx_pub_keys_to_extra(tx.extra, additional_pub_keys);
  }
  cryptonote::blobdata nonce;
  switch (fdp.ConsumeIntegralInRange<int>(0, 3))
  {
    case 0:
      break;
    case 1:
    {
      crypto::hash8 payment_id8;
      const std::string bytes = fdp.ConsumeBytesAsString(sizeof(payment_id8));
      memset(&payment_id8, 0, sizeof(payment_id8));
      memcpy(&payment_id8, bytes.data(), bytes.size());
      cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(nonce, payment_id8);
      break;
    }
    case 2:
    {
      crypto::hash payment_id = crypto::null_hash;
      const std::string bytes = fdp.ConsumeBytesAsString(sizeof(payment_id));
      memcpy(&payment_id, bytes.data(), bytes.size());
      cryptonote::set_payment_id_to_tx_extra_nonce(nonce, payment_id);
      break;
    }
    default:
      nonce = fdp.ConsumeRandomLengthString(TX_EXTRA_NONCE_MAX_COUNT);
      break;
  }
  if (!nonce.empty())
    cryptonote::add_extra_nonce_to_tx_extra(tx.extra, nonce);
  if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
  {
    const std::string garbage = fdp.ConsumeRandomLengthString(32);
    tx.extra.insert(tx.extra.end(), garbage.begin(), garbage.end());
  }
  return tx;
}

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  wallet = &local_wallet;

  static const char * const spendkey_hex = "f285d4ac9e66271256fc7cde0d3d6b36f66efff6ccd766706c408e86f4997a0d";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);

  wallet->init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  wallet->set_subaddress_lookahead(2, 3);
  wallet->generate("", "", spendkey, true, false);

  static const char * const foreign_spendkey_hex = "0b4f47697ec99c3de6579304e5f25c68b07afbe55b71d99620bf6cbf4e45a80f";
  crypto::secret_key foreign_spendkey;
  epee::string_tools::hex_to_pod(foreign_spendkey_hex, foreign_spendkey);
  cryptonote::account_base foreign;
  foreign.generate(foreign_spendkey, true, false);
  foreign_address = foreign.get_keys().m_account_address;

  if (!::serialization::dump_binary(*wallet, snapshot))
  {
    std::cerr << "failed to snapshot wallet state" << std::endl;
    return 1;
  }
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  binary_archive<false> restore{epee::strspan<std::uint8_t>(snapshot)};
  if (!::serialization::serialize(restore, *wallet))
    abort();

  FuzzedDataProvider fdp(buf, len);
  static const tools::wallet2::RefreshType refresh_types[] = {tools::wallet2::RefreshFull, tools::wallet2::RefreshOptimizeCoinbase, tools::wallet2::RefreshNoCoinbase};
  wallet->set_refresh_type(fdp.PickValueInArray(refresh_types));
  wallet->track_uses(fdp.ConsumeBool());
  wallet->store_tx_info(fdp.ConsumeBool());
  std::map<std::pair<uint64_t, uint64_t>, size_t> output_tracker_cache;
  const bool use_tracker = fdp.ConsumeBool();

  const size_t n_txs = fdp.ConsumeIntegralInRange<size_t>(1, kMaxTxs);
  for (size_t t = 0; t < n_txs && fdp.remaining_bytes() > 0; ++t)
  {
    const bool miner_tx = fdp.ConsumeIntegralInRange<int>(0, 3) == 0;
    const bool pool = !miner_tx && fdp.ConsumeBool();
    const bool double_spend_seen = pool && fdp.ConsumeBool();
    const uint64_t height = fdp.ConsumeIntegralInRange<uint64_t>(0, 4000000);
    const uint8_t block_version = fdp.ConsumeIntegralInRange<uint8_t>(1, 16);
    const uint64_t ts = fdp.ConsumeIntegral<uint64_t>();
    // Few distinct txids so the same tx can be seen in the pool and then confirmed.
    crypto::hash txid = crypto::null_hash;
    txid.data[0] = fdp.ConsumeIntegralInRange<uint8_t>(0, 3);

    const cryptonote::transaction tx = build_tx(fdp, miner_tx, height);

    std::vector<uint64_t> o_indices;
    for (size_t i = 0; i < tx.vout.size(); ++i)
      o_indices.push_back(fdp.ConsumeIntegralInRange<uint64_t>(0, 16));
    if (fdp.ConsumeIntegralInRange<int>(0, 15) == 0)
      o_indices.push_back(0);

    wallet_accessor_test::process_new_transaction(*wallet, txid, tx, o_indices, height, block_version, ts,
        miner_tx, pool, double_spend_seen, use_tracker ? &output_tracker_cache : NULL);
  }
  wallet->balance_all(false);
END_SIMPLE_FUZZER()
