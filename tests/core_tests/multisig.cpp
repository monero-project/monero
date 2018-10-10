// Copyright (c) 2017-2018, The Monero Project
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

#include "ringct/rctSigs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "multisig/multisig.h"
#include "common/apply_permutation.h"
#include "chaingen.h"
#include "multisig.h"
#include "device/device.hpp"
using namespace epee;
using namespace crypto;
using namespace cryptonote;

//#define NO_MULTISIG

void make_multisig_accounts(std::vector<cryptonote::account_base>& account, uint32_t threshold)
{
  std::vector<crypto::secret_key> all_view_keys;
  std::vector<std::vector<crypto::public_key>> derivations(account.size());
  //storage for all set of multisig derivations and spend public key (in first round)
  std::unordered_set<crypto::public_key> exchanging_keys;

  for (size_t msidx = 0; msidx < account.size(); ++msidx)
  {
    crypto::secret_key vkh = cryptonote::get_multisig_blinded_secret_key(account[msidx].get_keys().m_view_secret_key);
    all_view_keys.push_back(vkh);

    crypto::secret_key skh = cryptonote::get_multisig_blinded_secret_key(account[msidx].get_keys().m_spend_secret_key);
    crypto::public_key pskh;
    crypto::secret_key_to_public_key(skh, pskh);

    derivations[msidx].push_back(pskh);
    exchanging_keys.insert(pskh);
  }

  uint32_t roundsTotal = 1;
  if (threshold < account.size())
    roundsTotal = account.size() - threshold;

  //secret multisig keys of every account
  std::vector<std::vector<crypto::secret_key>> multisig_keys(account.size());
  std::vector<crypto::secret_key> spend_skey(account.size());
  std::vector<crypto::public_key> spend_pkey(account.size());
  for (uint32_t round = 0; round < roundsTotal; ++round)
  {
    std::unordered_set<crypto::public_key> roundKeys;
    for (size_t msidx = 0; msidx < account.size(); ++msidx)
    {
      // subtracting one's keys from set of all unique keys is the same as key exchange
      auto myKeys = exchanging_keys;
      for (const auto& d: derivations[msidx])
          myKeys.erase(d);

      if (threshold == account.size())
      {
        cryptonote::generate_multisig_N_N(account[msidx].get_keys(), std::vector<crypto::public_key>(myKeys.begin(), myKeys.end()), multisig_keys[msidx], (rct::key&)spend_skey[msidx], (rct::key&)spend_pkey[msidx]);
      }
      else
      {
        derivations[msidx] = cryptonote::generate_multisig_derivations(account[msidx].get_keys(), std::vector<crypto::public_key>(myKeys.begin(), myKeys.end()));
        roundKeys.insert(derivations[msidx].begin(), derivations[msidx].end());
      }
    }

    exchanging_keys = roundKeys;
    roundKeys.clear();
  }

  std::unordered_set<crypto::public_key> all_multisig_keys;
  for (size_t msidx = 0; msidx < account.size(); ++msidx)
  {
    std::unordered_set<crypto::secret_key> view_keys(all_view_keys.begin(), all_view_keys.end());
    view_keys.erase(all_view_keys[msidx]);

    crypto::secret_key view_skey = cryptonote::generate_multisig_view_secret_key(account[msidx].get_keys().m_view_secret_key, std::vector<secret_key>(view_keys.begin(), view_keys.end()));
    if (threshold < account.size())
    {
      multisig_keys[msidx] = cryptonote::calculate_multisig_keys(derivations[msidx]);
      spend_skey[msidx] = cryptonote::calculate_multisig_signer_key(multisig_keys[msidx]);
    }
    account[msidx].make_multisig(view_skey, spend_skey[msidx], spend_pkey[msidx], multisig_keys[msidx]);
    for (const auto &k: multisig_keys[msidx]) {
      all_multisig_keys.insert(rct::rct2pk(rct::scalarmultBase(rct::sk2rct(k))));
    }
  }

  if (threshold < account.size())
  {
    std::vector<crypto::public_key> public_keys(std::vector<crypto::public_key>(all_multisig_keys.begin(), all_multisig_keys.end()));
    crypto::public_key spend_pkey = cryptonote::generate_multisig_M_N_spend_public_key(public_keys);

    for (size_t msidx = 0; msidx < account.size(); ++msidx)
      account[msidx].finalize_multisig(spend_pkey);
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_multisig_tx_validation_base::generate_with(std::vector<test_event_entry>& events,
    size_t inputs, size_t mixin, uint64_t amount_paid, bool valid,
    size_t threshold, size_t total, size_t creator, std::vector<size_t> signers,
    const std::function<void(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations)> &pre_tx,
    const std::function<void(transaction &tx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;
  bool r;

  CHECK_AND_ASSERT_MES(total >= 2, false, "Bad scheme");
  CHECK_AND_ASSERT_MES(threshold <= total, false, "Bad scheme");
#ifdef NO_MULTISIG
  CHECK_AND_ASSERT_MES(total <= 5, false, "Unsupported scheme");
#endif
  CHECK_AND_ASSERT_MES(inputs >= 1 && inputs <= 8, false, "Inputs should between 1 and 8");

  // given as 1 based for clarity
  --creator;
  for (size_t &signer: signers)
    --signer;

  CHECK_AND_ASSERT_MES(creator < total, false, "invalid creator");
  for (size_t signer: signers)
    CHECK_AND_ASSERT_MES(signer < total, false, "invalid signer");

#ifdef NO_MULTISIG
  GENERATE_ACCOUNT(acc0);
  GENERATE_ACCOUNT(acc1);
  GENERATE_ACCOUNT(acc2);
  GENERATE_ACCOUNT(acc3);
  GENERATE_ACCOUNT(acc4);
  account_base miner_account[5] = {acc0, acc1, acc2, acc3, acc4};
#else
  GENERATE_MULTISIG_ACCOUNT(miner_account, threshold, total);
#endif

  MAKE_GENESIS_BLOCK(events, blk_0, miner_account[creator], ts_start);

  constexpr int block_major = 7;
  constexpr int block_minor = 7;

  // create 8 miner accounts, and have them mine the next 8 blocks
  // they will have a coinbase with a single out that's pseudo rct
  constexpr size_t n_coinbases = 48;
  cryptonote::account_base miner_accounts[n_coinbases];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[n_coinbases];
  for (size_t n = 0; n < n_coinbases; ++n) {
    // the first block goes to the multisig account
    miner_accounts[n].generate();
    account_base &account = n < inputs ? miner_account[creator] : miner_accounts[n];
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        block_major, block_minor, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
    LOG_PRINT_L0("Initial miner tx " << n << ": " << obj_to_json_str(blocks[n].miner_tx));
    LOG_PRINT_L0("in block: " << obj_to_json_str(blocks[n]));
  }

  // rewind
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[n_coinbases - 1];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_accounts[0],
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          block_major, block_minor, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
          false, "Failed to generate block");
      events.push_back(blk);
      blk_last = blk;
    }
    blk_r = blk_last;
  }

  cryptonote::keypair in_ephemeral;
  crypto::public_key tx_pub_key[n_coinbases];
  crypto::public_key output_pub_key[n_coinbases];
  for (size_t n = 0; n < n_coinbases; ++n)
  {
    tx_pub_key[n] = get_tx_pub_key_from_extra(blocks[n].miner_tx);
    MDEBUG("tx_pub_key: " << tx_pub_key);
    output_pub_key[n] = boost::get<txout_to_key>(blocks[n].miner_tx.vout[0].target).key;
    MDEBUG("output_pub_key: " << output_pub_key);
  }

  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  subaddresses[miner_account[0].get_keys().m_account_address.m_spend_public_key] = {0,0};

#ifndef NO_MULTISIG
  // create k/L/R/ki for that output we're going to spend
  std::vector<std::vector<std::vector<crypto::secret_key>>> account_k(total);
  std::vector<std::vector<std::vector<crypto::public_key>>> account_L(total);
  std::vector<std::vector<std::vector<crypto::public_key>>> account_R(total);
  std::vector<std::vector<std::vector<crypto::key_image>>> account_ki(total);
  std::vector<crypto::public_key> additional_tx_keys;
  for (size_t msidx = 0; msidx < total; ++msidx)
  {
    CHECK_AND_ASSERT_MES(miner_account[msidx].get_keys().m_account_address.m_spend_public_key == miner_account[0].get_keys().m_account_address.m_spend_public_key,
        false, "Mismatched spend public keys");

    size_t nlr = threshold < total ? threshold - 1 : 1;
    account_k[msidx].resize(inputs);
    account_L[msidx].resize(inputs);
    account_R[msidx].resize(inputs);
    account_ki[msidx].resize(inputs);
    for (size_t tdidx = 0; tdidx < inputs; ++tdidx)
    {
      account_L[msidx][tdidx].resize(nlr);
      account_R[msidx][tdidx].resize(nlr);
      for (size_t n = 0; n < nlr; ++n)
      {
        account_k[msidx][tdidx].push_back(rct::rct2sk(rct::skGen()));
        cryptonote::generate_multisig_LR(output_pub_key[tdidx], account_k[msidx][tdidx][n], account_L[msidx][tdidx][n], account_R[msidx][tdidx][n]);
      }
      size_t numki = miner_account[msidx].get_multisig_keys().size();
      account_ki[msidx][tdidx].resize(numki);
      for (size_t kiidx = 0; kiidx < numki; ++kiidx)
      {
        r = cryptonote::generate_multisig_key_image(miner_account[msidx].get_keys(), kiidx, output_pub_key[tdidx], account_ki[msidx][tdidx][kiidx]);
        CHECK_AND_ASSERT_MES(r, false, "Failed to generate multisig export key image");
      }
      MDEBUG("Party " << msidx << ":");
      MDEBUG("spend: sec " << miner_account[msidx].get_keys().m_spend_secret_key << ", pub " << miner_account[msidx].get_keys().m_account_address.m_spend_public_key);
      MDEBUG("view: sec " << miner_account[msidx].get_keys().m_view_secret_key << ", pub " << miner_account[msidx].get_keys().m_account_address.m_view_public_key);
      for (const auto &k: miner_account[msidx].get_multisig_keys())
        MDEBUG("msk: " << k);
      for (size_t n = 0; n < account_k[msidx][tdidx].size(); ++n)
      {
        MDEBUG("k: " << account_k[msidx][tdidx][n]);
        MDEBUG("L: " << account_L[msidx][tdidx][n]);
        MDEBUG("R: " << account_R[msidx][tdidx][n]);
      }
      for (const auto &ki: account_ki[msidx][tdidx])
        MDEBUG("ki: " << ki);
    }
  }
#endif

  // create kLRki
  std::vector<rct::multisig_kLRki> kLRkis;
  std::unordered_set<crypto::public_key> used_L;
  for (size_t tdidx = 0; tdidx < inputs; ++tdidx)
  {
    kLRkis.push_back(rct::multisig_kLRki());
    rct::multisig_kLRki &kLRki = kLRkis.back();
#ifdef NO_MULTISIG
    kLRki = {rct::zero(), rct::zero(), rct::zero(), rct::zero()};
#else
    kLRki.k = rct::sk2rct(account_k[creator][tdidx][0]);
    kLRki.L = rct::pk2rct(account_L[creator][tdidx][0]);
    kLRki.R = rct::pk2rct(account_R[creator][tdidx][0]);
    MDEBUG("Starting with k " << kLRki.k);
    MDEBUG("Starting with L " << kLRki.L);
    MDEBUG("Starting with R " << kLRki.R);
    for (size_t msidx = 0; msidx < total; ++msidx)
    {
      if (msidx == creator)
        continue;
      if (std::find(signers.begin(), signers.end(), msidx) == signers.end())
        continue;
      for (size_t lr = 0; lr < account_L[msidx][tdidx].size(); ++lr)
      {
        if (used_L.find(account_L[msidx][tdidx][lr]) == used_L.end())
        {
          used_L.insert(account_L[msidx][tdidx][lr]);
          MDEBUG("Adding L " << account_L[msidx][tdidx][lr] << " (for k " << account_k[msidx][tdidx][lr] << ")");
          MDEBUG("Adding R " << account_R[msidx][tdidx][lr]);
          rct::addKeys((rct::key&)kLRki.L, kLRki.L, rct::pk2rct(account_L[msidx][tdidx][lr]));
          rct::addKeys((rct::key&)kLRki.R, kLRki.R, rct::pk2rct(account_R[msidx][tdidx][lr]));
          break;
        }
      }
    }
    std::vector<crypto::key_image> pkis;
    for (size_t msidx = 0; msidx < total; ++msidx)
      for (size_t n = 0; n < account_ki[msidx][tdidx].size(); ++n)
        pkis.push_back(account_ki[msidx][tdidx][n]);
    r = cryptonote::generate_multisig_composite_key_image(miner_account[0].get_keys(), subaddresses, output_pub_key[tdidx], tx_pub_key[tdidx], additional_tx_keys, 0, pkis, (crypto::key_image&)kLRki.ki);
    CHECK_AND_ASSERT_MES(r, false, "Failed to generate composite key image");
    MDEBUG("composite ki: " << kLRki.ki);
    MDEBUG("L: " << kLRki.L);
    MDEBUG("R: " << kLRki.R);
    for (size_t n = 1; n < total; ++n)
    {
      rct::key ki;
      r = cryptonote::generate_multisig_composite_key_image(miner_account[n].get_keys(), subaddresses, output_pub_key[tdidx], tx_pub_key[tdidx], additional_tx_keys, 0, pkis, (crypto::key_image&)ki);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate composite key image");
      CHECK_AND_ASSERT_MES(kLRki.ki == ki, false, "Composite key images do not match");
    }
  }
#endif

  // create a tx: we have 8 outputs, all from coinbase, so "fake" rct - use 2
  std::vector<tx_source_entry> sources;
  for (size_t n = 0; n < inputs; ++n)
  {
    sources.resize(sources.size() + 1);
    tx_source_entry& src = sources.back();

    src.real_output = n;
    src.amount = blocks[n].miner_tx.vout[0].amount;
    src.real_out_tx_key = tx_pub_key[n];
    src.real_output_in_tx_index = 0;
    src.mask = rct::identity();
    src.rct = true;
    src.multisig_kLRki = kLRkis[n];

    for (size_t m = 0; m <= mixin; ++m)
    {
      rct::ctkey ctkey;
      ctkey.dest = rct::pk2rct(boost::get<txout_to_key>(blocks[m].miner_tx.vout[0].target).key);
      MDEBUG("using " << (m == n ? "real" : "fake") << " input " << ctkey.dest);
      ctkey.mask = rct::commit(blocks[m].miner_tx.vout[0].amount, rct::identity()); // since those are coinbases, the masks are known
      src.outputs.push_back(std::make_pair(m, ctkey));
    }
  }

  //fill outputs entry
  tx_destination_entry td;
  td.addr = miner_account[creator].get_keys().m_account_address;
  td.amount = amount_paid;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(td);

  if (pre_tx)
    pre_tx(sources, destinations);

  transaction tx;
  crypto::secret_key tx_key;
#ifdef NO_MULTISIG
  rct::multisig_out *msoutp = NULL;
#else
  rct::multisig_out msout;
  rct::multisig_out *msoutp = &msout;
#endif
  std::vector<crypto::secret_key> additional_tx_secret_keys;
  auto sources_copy = sources;
  r = construct_tx_and_get_tx_key(miner_account[creator].get_keys(), subaddresses, sources, destinations, boost::none, std::vector<uint8_t>(), tx, 0, tx_key, additional_tx_secret_keys, true, rct::RangeProofBorromean, msoutp);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");

#ifndef NO_MULTISIG
  // work out the permutation done on sources
  std::vector<size_t> ins_order;
  for (size_t n = 0; n < sources.size(); ++n)
  {
    for (size_t idx = 0; idx < sources_copy.size(); ++idx)
    {
      CHECK_AND_ASSERT_MES((size_t)sources_copy[idx].real_output < sources_copy[idx].outputs.size(),
          false, "Invalid real_output");
      if (sources_copy[idx].outputs[sources_copy[idx].real_output].second.dest == sources[n].outputs[sources[n].real_output].second.dest)
        ins_order.push_back(idx);
    }
  }
  CHECK_AND_ASSERT_MES(ins_order.size() == sources.size(), false, "Failed to work out sources permutation");
#endif

#ifndef NO_MULTISIG
  // sign
  std::unordered_set<crypto::secret_key> used_keys;
  const std::vector<crypto::secret_key> &msk0 = miner_account[creator].get_multisig_keys();
  for (const auto &sk: msk0)
    used_keys.insert(sk);
  for (size_t signer: signers)
  {
    rct::key skey = rct::zero();
    const std::vector<crypto::secret_key> &msk1 = miner_account[signer].get_multisig_keys();
    for (size_t n = 0; n < msk1.size(); ++n)
    {
      const crypto::secret_key &sk1 = msk1[n];
      if (used_keys.find(sk1) == used_keys.end())
      {
        used_keys.insert(sk1);
        sc_add(skey.bytes, skey.bytes, rct::sk2rct(sk1).bytes);
      }
    }
    CHECK_AND_ASSERT_MES(!(skey == rct::zero()), false, "failed to find secret multisig key to sign transaction");
    std::vector<unsigned int> indices;
    for (const auto &src: sources_copy)
      indices.push_back(src.real_output);
    rct::keyV k;
    for (size_t tdidx = 0; tdidx < inputs; ++tdidx)
    {
      k.push_back(rct::zero());
      for (size_t n = 0; n < account_k[signer][tdidx].size(); ++n)
      {
        crypto::public_key L;
        rct::scalarmultBase((rct::key&)L, rct::sk2rct(account_k[signer][tdidx][n]));
        if (used_L.find(L) != used_L.end())
        {
          sc_add(k.back().bytes, k.back().bytes, rct::sk2rct(account_k[signer][tdidx][n]).bytes);
        }
      }
      CHECK_AND_ASSERT_MES(!(k.back() == rct::zero()), false, "failed to find k to sign transaction");
    }
    tools::apply_permutation(ins_order, indices);
    tools::apply_permutation(ins_order, k);

    MDEBUG("signing with k size " << k.size());
    MDEBUG("signing with k " << k.back());
    MDEBUG("signing with sk " << skey);
    for (const auto &sk: used_keys)
      MDEBUG("  created with sk " << sk);
    MDEBUG("signing with c size " << msout.c.size());
    MDEBUG("signing with c " << msout.c.back());
    r = rct::signMultisig(tx.rct_signatures, indices, k, msout, skey);
    CHECK_AND_ASSERT_MES(r, false, "failed to sign transaction");
  }
#endif

  // verify this tx is really to the expected address
  const crypto::public_key tx_pub_key2 = get_tx_pub_key_from_extra(tx, 0);
  crypto::key_derivation derivation;
  r = crypto::generate_key_derivation(tx_pub_key2, miner_account[creator].get_keys().m_view_secret_key, derivation);
  CHECK_AND_ASSERT_MES(r, false, "Failed to generate derivation");
  uint64_t n_outs = 0, amount = 0;
  std::vector<crypto::key_derivation> additional_derivations;
  for (size_t n = 0; n < tx.vout.size(); ++n)
  {
    CHECK_AND_ASSERT_MES(typeid(txout_to_key) == tx.vout[n].target.type(), false, "Unexpected tx out type");
    if (is_out_to_acc_precomp(subaddresses, boost::get<txout_to_key>(tx.vout[n].target).key, derivation, additional_derivations, n, hw::get_device(("default"))))
    {
      ++n_outs;
      CHECK_AND_ASSERT_MES(tx.vout[n].amount == 0, false, "Destination amount is not zero");
      rct::key Ctmp;
      crypto::secret_key scalar1;
      crypto::derivation_to_scalar(derivation, n, scalar1);
      rct::ecdhTuple ecdh_info = tx.rct_signatures.ecdhInfo[n];
      rct::ecdhDecode(ecdh_info, rct::sk2rct(scalar1));
      rct::key C = tx.rct_signatures.outPk[n].mask;
      rct::addKeys2(Ctmp, ecdh_info.mask, ecdh_info.amount, rct::H);
      CHECK_AND_ASSERT_MES(rct::equalKeys(C, Ctmp), false, "Failed to decode amount");
      amount += rct::h2d(ecdh_info.amount);
    }
  }
  CHECK_AND_ASSERT_MES(n_outs == 1, false, "Not exactly 1 output was received");
  CHECK_AND_ASSERT_MES(amount == amount_paid, false, "Amount paid was not the expected amount");

  if (post_tx)
    post_tx(tx);

  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  LOG_PRINT_L0("Test tx: " << obj_to_json_str(tx));

  return true;
}

bool gen_multisig_tx_valid_22_1_2::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 2, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_22_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 4, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 2, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_22_2_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 2, 2, {1}, NULL, NULL);
}

bool gen_multisig_tx_valid_33_1_23::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 3, 3, 1, {2, 3}, NULL, NULL);
}

bool gen_multisig_tx_valid_33_3_21::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 3, 3, 3, {2, 1}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_1_2::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 3, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_1_3::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 3, 1, {3}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_2_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 3, 2, {1}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_2_3::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 3, 2, {3}, NULL, NULL);
}

bool gen_multisig_tx_valid_45_1_234::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 4, 5, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_valid_45_4_135_many_inputs::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 4, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 4, 5, 4, {1, 3, 5}, NULL, NULL);
}

bool gen_multisig_tx_valid_89_3_1245789::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 8, 9, 3, {1, 2, 4, 5, 7, 8, 9}, NULL, NULL);
}

bool gen_multisig_tx_valid_24_1_2::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 4, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_24_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 4, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_25_1_2::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 5, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_25_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 2, 5, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_234::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 4, 8, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_234_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, true, 4, 8, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_invalid_22_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 2, 2, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 3, 3, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1_2_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 3, 3, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1_3_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 3, 3, 1, {3}, NULL, NULL);
}

bool gen_multisig_tx_invalid_23_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 2, 3, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_45_5_23_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 4, 5, 5, {2, 3}, NULL, NULL);
}

bool gen_multisig_tx_valid_24_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 2, 4, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_valid_25_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 2, 5, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 4, 8, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_23_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, CRYPTONOTE_TX_DEFAULT_MIX, amount_paid, false, 4, 8, 1, {2, 3}, NULL, NULL);
}
