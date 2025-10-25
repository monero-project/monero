// Copyright (c) 2017-2024, The Monero Project
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
#include "multisig/multisig_tx_builder_ringct.h"
#include "common/apply_permutation.h"
#include "chaingen.h"
#include "multisig.h"

#include "common/apply_permutation.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "device/device.hpp"
#include "multisig/multisig.h"
#include "multisig/multisig_account.h"
#include "multisig/multisig_kex_msg.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"

using namespace epee;
using namespace crypto;
using namespace cryptonote;
using namespace multisig;

//#define NO_MULTISIG

static bool make_multisig_accounts(std::vector<cryptonote::account_base> &accounts, const uint32_t threshold)
{
  CHECK_AND_ASSERT_MES(accounts.size() > 0, false, "Invalid multisig scheme");

  std::vector<multisig_account> multisig_accounts;
  std::vector<crypto::public_key> signers;
  std::vector<multisig_kex_msg> round_msgs;
  multisig_accounts.reserve(accounts.size());
  signers.reserve(accounts.size());
  round_msgs.reserve(accounts.size());

  // create multisig accounts
  for (std::size_t account_index{0}; account_index < accounts.size(); ++account_index)
  {
    // create account and collect signer
    multisig_accounts.emplace_back(
        multisig_account{
          get_multisig_blinded_secret_key(accounts[account_index].get_keys().m_spend_secret_key),
          get_multisig_blinded_secret_key(accounts[account_index].get_keys().m_view_secret_key)
        }
      );

    signers.emplace_back(multisig_accounts.back().get_base_pubkey());

    // collect account's first kex msg
    round_msgs.emplace_back(multisig_accounts.back().get_next_kex_round_msg());
  }

  // initialize accounts and collect kex messages for the next round
  std::vector<multisig_kex_msg> temp_round_msgs(multisig_accounts.size());
  for (std::size_t account_index{0}; account_index < accounts.size(); ++account_index)
  {
    multisig_accounts[account_index].initialize_kex(threshold, signers, round_msgs);
    temp_round_msgs[account_index] = multisig_accounts[account_index].get_next_kex_round_msg();
  }

  // perform key exchange rounds
  while (!multisig_accounts[0].multisig_is_ready())
  {
    round_msgs = temp_round_msgs;

    for (std::size_t account_index{0}; account_index < multisig_accounts.size(); ++account_index)
    {
      multisig_accounts[account_index].kex_update(round_msgs);
      temp_round_msgs[account_index] = multisig_accounts[account_index].get_next_kex_round_msg();
    }
  }

  // update accounts post key exchange
  for (std::size_t account_index{0}; account_index < accounts.size(); ++account_index)
  {
    accounts[account_index].make_multisig(multisig_accounts[account_index].get_common_privkey(),
      multisig_accounts[account_index].get_base_privkey(),
      multisig_accounts[account_index].get_multisig_pubkey(),
      multisig_accounts[account_index].get_multisig_privkeys());
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_multisig_tx_validation_base::generate_with(std::vector<test_event_entry>& events,
    size_t inputs, size_t mixin, uint64_t amount_paid, bool valid,
    size_t threshold, size_t total, size_t creator, std::vector<size_t> other_signers,
    const std::function<void(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations)> &pre_tx,
    const std::function<void(transaction &tx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;
  bool r;

  CHECK_AND_ASSERT_MES(total >= 2, false, "Bad scheme");
  CHECK_AND_ASSERT_MES(threshold <= total, false, "Bad scheme");
  CHECK_AND_ASSERT_MES(inputs >= 1 && inputs <= 8, false, "Inputs should between 1 and 8");

  // given as 1 based for clarity
  --creator;
  for (size_t &signer: other_signers)
    --signer;

  CHECK_AND_ASSERT_MES(creator < total, false, "invalid creator");
  for (size_t signer: other_signers)
    CHECK_AND_ASSERT_MES(signer < total, false, "invalid signer");

  GENERATE_MULTISIG_ACCOUNT(miner_account, threshold, total);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_account[creator], ts_start);

  // create 16 miner accounts, and have them mine the next 16 blocks
  // they will have a coinbase with a single out that's pseudo rct
  constexpr size_t n_coinbases = 16;
  cryptonote::account_base miner_accounts[n_coinbases];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[n_coinbases];
  for (size_t n = 0; n < n_coinbases; ++n) {
    // the first block goes to the multisig account
    miner_accounts[n].generate();
    account_base &account = n < inputs ? miner_account[creator] : miner_accounts[n];
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version | test_generator::bf_max_outs,
        HF_VERSION_BULLETPROOF_PLUS, HF_VERSION_BULLETPROOF_PLUS, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 1, 4),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
  }

  // rewind
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[n_coinbases - 1];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_accounts[0],
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version | test_generator::bf_max_outs,
          HF_VERSION_BULLETPROOF_PLUS, HF_VERSION_BULLETPROOF_PLUS, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 1, 4),
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
    cryptonote::get_output_public_key(blocks[n].miner_tx.vout[0], output_pub_key[n]);
    MDEBUG("output_pub_key: " << output_pub_key);
  }

  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  subaddresses[miner_account[0].get_keys().m_account_address.m_spend_public_key] = {0,0};

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
    nlr *= multisig::signing::kAlphaComponents;
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
        multisig::generate_multisig_LR(output_pub_key[tdidx], account_k[msidx][tdidx][n], account_L[msidx][tdidx][n], account_R[msidx][tdidx][n]);
      }
      size_t num_account_partial_ki = miner_account[msidx].get_multisig_keys().size();
      account_ki[msidx][tdidx].resize(num_account_partial_ki);
      for (size_t kiidx = 0; kiidx < num_account_partial_ki; ++kiidx)
      {
        r = multisig::generate_multisig_key_image(miner_account[msidx].get_keys(), kiidx, output_pub_key[tdidx], account_ki[msidx][tdidx][kiidx]);
        CHECK_AND_ASSERT_MES(r, false, "Failed to generate multisig export key image");
      }
      MDEBUG("Party " << msidx << ":");
      MDEBUG("spend: sec " << crypto::secret_key_explicit_print_ref{miner_account[msidx].get_keys().m_spend_secret_key} << ", pub " << miner_account[msidx].get_keys().m_account_address.m_spend_public_key);
      MDEBUG("view: sec " << crypto::secret_key_explicit_print_ref{miner_account[msidx].get_keys().m_view_secret_key} << ", pub " << miner_account[msidx].get_keys().m_account_address.m_view_public_key);
      for (const auto &k: miner_account[msidx].get_multisig_keys())
        MDEBUG("msk: " << crypto::secret_key_explicit_print_ref{k});
      for (size_t n = 0; n < account_k[msidx][tdidx].size(); ++n)
      {
        MDEBUG("k: " << crypto::secret_key_explicit_print_ref{account_k[msidx][tdidx][n]});
        MDEBUG("L: " << account_L[msidx][tdidx][n]);
        MDEBUG("R: " << account_R[msidx][tdidx][n]);
      }
      for (const auto &ki: account_ki[msidx][tdidx])
        MDEBUG("ki: " << ki);
    }
  }

  // create kLRki
  std::vector<rct::multisig_kLRki> kLRkis;
  std::unordered_set<crypto::public_key> used_L;
  for (size_t tdidx = 0; tdidx < inputs; ++tdidx)
  {
    kLRkis.push_back(rct::multisig_kLRki());
    rct::multisig_kLRki &kLRki = kLRkis.back();
    std::vector<crypto::key_image> pkis;
    for (size_t msidx = 0; msidx < total; ++msidx)
      for (size_t n = 0; n < account_ki[msidx][tdidx].size(); ++n)
        pkis.push_back(account_ki[msidx][tdidx][n]);
    r = multisig::generate_multisig_composite_key_image(miner_account[0].get_keys(), subaddresses, output_pub_key[tdidx], tx_pub_key[tdidx], additional_tx_keys, 0, pkis, (crypto::key_image&)kLRki.ki);
    CHECK_AND_ASSERT_MES(r, false, "Failed to generate composite key image");
    MDEBUG("composite ki: " << kLRki.ki);
    for (size_t n = 1; n < total; ++n)
    {
      rct::key ki;
      r = multisig::generate_multisig_composite_key_image(miner_account[n].get_keys(), subaddresses, output_pub_key[tdidx], tx_pub_key[tdidx], additional_tx_keys, 0, pkis, (crypto::key_image&)ki);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate composite key image");
      CHECK_AND_ASSERT_MES(kLRki.ki == ki, false, "Composite key images do not match");
    }
  }

  // prepare a tx: we have 8 outputs, all from coinbase, so "fake" rct - use 2
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
      crypto::public_key output_public_key;
      cryptonote::get_output_public_key(blocks[m].miner_tx.vout[0], output_public_key);
      ctkey.dest = rct::pk2rct(output_public_key);
      MDEBUG("using " << (m == n ? "real" : "fake") << " input " << ctkey.dest);
      ctkey.mask = rct::commit(blocks[m].miner_tx.vout[0].amount, rct::identity()); // since those are coinbases, the masks are known
      src.outputs.push_back(std::make_pair(m, ctkey));
    }
  }

  //fill outputs entry
  tx_destination_entry td;
  td.addr = miner_account[creator].get_keys().m_account_address;
  td.amount = amount_paid;
  std::vector<tx_destination_entry> destinations;  //tx need two outputs since HF_VERSION_MIN_2_OUTPUTS
  destinations.push_back(td);
  td.amount = 0;
  destinations.push_back(td);

  if (pre_tx)
    pre_tx(sources, destinations);

  transaction tx;
  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_secret_keys;
  crypto::secret_key multisig_tx_key_entropy;
  auto sources_copy = sources;
  multisig::signing::tx_builder_ringct_t tx_builder;
  CHECK_AND_ASSERT_MES(tx_builder.init(miner_account[creator].get_keys(), {}, 0, {0}, sources, destinations, {}, {rct::RangeProofPaddedBulletproof, 4}, true, false, tx_key, additional_tx_secret_keys, multisig_tx_key_entropy, tx), false, "error: multisig::signing::tx_builder_ringct_t::init");

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

  struct {
    rct::keyM total_alpha_G;
    rct::keyM total_alpha_H;
    rct::keyV c_0;
    rct::keyV s;
  } sig;
  {
    used_L.clear();
    sig.total_alpha_G.resize(sources.size(), rct::keyV(multisig::signing::kAlphaComponents, rct::identity()));
    sig.total_alpha_H.resize(sources.size(), rct::keyV(multisig::signing::kAlphaComponents, rct::identity()));
    sig.c_0.resize(sources.size());
    sig.s.resize(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
      rct::keyV alpha(multisig::signing::kAlphaComponents);
      for (std::size_t m = 0; m < multisig::signing::kAlphaComponents; ++m) {
        alpha[m] = rct::sk2rct(account_k[creator][ins_order[i]][m]);
        sig.total_alpha_G[i][m] = rct::pk2rct(account_L[creator][ins_order[i]][m]);
        sig.total_alpha_H[i][m] = rct::pk2rct(account_R[creator][ins_order[i]][m]);
        for (size_t j = 0; j < total; ++j) {
          if (j == creator)
            continue;
          if (std::find(other_signers.begin(), other_signers.end(), j) == other_signers.end())
            continue;
          for (std::size_t n = 0; n < account_L[j][ins_order[i]].size(); ++n) {
            if (used_L.find(account_L[j][ins_order[i]][n]) == used_L.end()) {
              used_L.insert(account_L[j][ins_order[i]][n]);
              rct::addKeys(sig.total_alpha_G[i][m], sig.total_alpha_G[i][m], rct::pk2rct(account_L[j][ins_order[i]][n]));
              rct::addKeys(sig.total_alpha_H[i][m], sig.total_alpha_H[i][m], rct::pk2rct(account_R[j][ins_order[i]][n]));
              break;
            }
          }
        }
      }
      CHECK_AND_ASSERT_MES(tx_builder.first_partial_sign(i, sig.total_alpha_G[i], sig.total_alpha_H[i], alpha, sig.c_0[i], sig.s[i]), false, "error: multisig::signing::tx_builder_ringct_t::first_partial_sign");
    }
  }

  // sign
  std::unordered_set<crypto::secret_key> used_keys;
  const std::vector<crypto::secret_key> &msk0 = miner_account[creator].get_multisig_keys();
  for (const auto &sk: msk0)
    used_keys.insert(sk);  //these were used in 'tx_builder.init() -> tx_builder.first_partial_sign()'
  for (size_t signer: other_signers)
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
    rct::keyM k(sources.size(), rct::keyV(multisig::signing::kAlphaComponents));
    for (std::size_t i = 0; i < sources.size(); ++i) {
      for (std::size_t j = 0; j < multisig::signing::kAlphaComponents; ++j) {
        for (std::size_t n = 0; n < account_k[signer][i].size(); ++n) {
          crypto::public_key L;
          rct::scalarmultBase((rct::key&)L, rct::sk2rct(account_k[signer][i][n]));
          if (used_L.find(L) != used_L.end()) {
            k[i][j] = rct::sk2rct(account_k[signer][i][n]);
            account_k[signer][i][n] = rct::rct2sk(rct::zero());  //demo: always clear nonces from long-term storage after use
            break;
          }
        }
        CHECK_AND_ASSERT_MES(!(k[i][j] == rct::zero()), false, "failed to find k to sign transaction");
      }
    }
    tools::apply_permutation(ins_order, k);
    multisig::signing::tx_builder_ringct_t signer_tx_builder;
    CHECK_AND_ASSERT_MES(signer_tx_builder.init(miner_account[signer].get_keys(), {}, 0, {0}, sources, destinations, {}, {rct::RangeProofPaddedBulletproof, 4}, true, true, tx_key, additional_tx_secret_keys, multisig_tx_key_entropy, tx), false, "error: multisig::signing::tx_builder_ringct_t::init");

    MDEBUG("signing with k size " << k.size());
    for (size_t n = 0; n < multisig::signing::kAlphaComponents; ++n)
      MDEBUG("signing with k " << k.back()[n]);
    MDEBUG("signing with sk " << skey);
    for (const auto &sk: used_keys)
      MDEBUG("  created with sk " << crypto::secret_key_explicit_print_ref{sk});
    CHECK_AND_ASSERT_MES(signer_tx_builder.next_partial_sign(sig.total_alpha_G, sig.total_alpha_H, k, skey, sig.c_0, sig.s), false, "error: multisig::signing::tx_builder_ringct_t::next_partial_sign");

    // in round-robin signing, the last signer finalizes the tx
    if (signer == other_signers.back())
      CHECK_AND_ASSERT_MES(signer_tx_builder.finalize_tx(sources, sig.c_0, sig.s, tx), false, "error: multisig::signing::tx_builder_ringct_t::finalize_tx");
  }

  // verify this tx is really to the expected address
  const crypto::public_key tx_pub_key2 = get_tx_pub_key_from_extra(tx, 0);
  crypto::key_derivation derivation;
  r = crypto::generate_key_derivation(tx_pub_key2, miner_account[creator].get_keys().m_view_secret_key, derivation);
  CHECK_AND_ASSERT_MES(r, false, "Failed to generate derivation");
  uint64_t n_outs = 0, amount = 0;
  std::vector<crypto::key_derivation> additional_derivations;
  crypto::public_key output_public_key;
  for (size_t n = 0; n < tx.vout.size(); ++n)
  {
    CHECK_AND_ASSERT_MES(typeid(txout_to_tagged_key) == tx.vout[n].target.type(), false, "Unexpected tx out type");
    cryptonote::get_output_public_key(tx.vout[n], output_public_key);
    if (is_out_to_acc_precomp(subaddresses, output_public_key, derivation, additional_derivations, n, hw::get_device(("default"))))
    {
      ++n_outs;
      CHECK_AND_ASSERT_MES(tx.vout[n].amount == 0, false, "Destination amount is not zero");
      rct::key Ctmp;
      crypto::secret_key scalar1;
      crypto::derivation_to_scalar(derivation, n, scalar1);
      rct::ecdhTuple ecdh_info = tx.rct_signatures.ecdhInfo[n];
      rct::ecdhDecode(ecdh_info, rct::sk2rct(scalar1), rct::is_rct_short_amount(tx.rct_signatures.type));
      rct::key C = tx.rct_signatures.outPk[n].mask;
      rct::addKeys2(Ctmp, ecdh_info.mask, ecdh_info.amount, rct::H);
      CHECK_AND_ASSERT_MES(rct::equalKeys(C, Ctmp), false, "Failed to decode amount");
      amount += rct::h2d(ecdh_info.amount);
    }
  }
  CHECK_AND_ASSERT_MES(n_outs == 2, false, "Not exactly 2 outputs were received");
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
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 2, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_22_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 4, mixin, amount_paid, true, 2, 2, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_22_2_1::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 2, 2, {1}, NULL, NULL);
}

bool gen_multisig_tx_valid_33_1_23::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 3, 3, 1, {2, 3}, NULL, NULL);
}

bool gen_multisig_tx_valid_33_3_21::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 3, 3, 3, {2, 1}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_1_2::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 3, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_1_3::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 3, 1, {3}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_2_1::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 3, 2, {1}, NULL, NULL);
}

bool gen_multisig_tx_valid_23_2_3::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 2, 3, 2, {3}, NULL, NULL);
}

bool gen_multisig_tx_valid_45_1_234::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 4, 5, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_valid_45_4_135_many_inputs::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 4, mixin, amount_paid, true, 4, 5, 4, {1, 3, 5}, NULL, NULL);
}

bool gen_multisig_tx_valid_89_3_1245789::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, true, 8, 9, 3, {1, 2, 4, 5, 7, 8, 9}, NULL, NULL);
}

bool gen_multisig_tx_valid_24_1_2::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, mixin, amount_paid, true, 2, 4, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_24_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, mixin, amount_paid, true, 2, 4, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_25_1_2::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, mixin, amount_paid, true, 2, 5, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_25_1_2_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, mixin, amount_paid, true, 2, 5, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_234::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 2, mixin, amount_paid, true, 4, 8, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_valid_48_1_234_many_inputs::generate(std::vector<test_event_entry>& events) const
{
    const size_t mixin = 10;
    const uint64_t amount_paid = 10000;
    return generate_with(events, 4, mixin, amount_paid, true, 4, 8, 1, {2, 3, 4}, NULL, NULL);
}

bool gen_multisig_tx_invalid_22_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 2, 2, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 3, 3, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1_2_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 3, 3, 1, {2}, NULL, NULL);
}

bool gen_multisig_tx_invalid_33_1_3_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 3, 3, 1, {3}, NULL, NULL);
}

bool gen_multisig_tx_invalid_23_1__no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 2, 3, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_45_5_23_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 4, 5, 5, {2, 3}, NULL, NULL);
}

bool gen_multisig_tx_invalid_24_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 2, 4, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_25_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 2, 5, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_48_1_no_signers::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 4, 8, 1, {}, NULL, NULL);
}

bool gen_multisig_tx_invalid_48_1_23_no_threshold::generate(std::vector<test_event_entry>& events) const
{
  const size_t mixin = 10;
  const uint64_t amount_paid = 10000;
  return generate_with(events, 2, mixin, amount_paid, false, 4, 8, 1, {2, 3}, NULL, NULL);
}
