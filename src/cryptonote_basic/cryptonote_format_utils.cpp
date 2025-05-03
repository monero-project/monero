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

#include <atomic>
#include <boost/algorithm/string.hpp>
#include "wipeable_string.h"
#include "string_tools.h"
#include "string_tools_lexical.h"
#include "serialization/string.h"
#include "cryptonote_format_utils.h"
#include "cryptonote_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "ringct/rctOps.h"

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

// #define ENABLE_HASH_CASH_INTEGRITY_CHECK

using namespace crypto;

static std::atomic<unsigned int> default_decimal_point(CRYPTONOTE_DISPLAY_DECIMAL_POINT);

static std::atomic<uint64_t> tx_hashes_calculated_count(0);
static std::atomic<uint64_t> tx_hashes_cached_count(0);
static std::atomic<uint64_t> block_hashes_calculated_count(0);
static std::atomic<uint64_t> block_hashes_cached_count(0);

#define CHECK_AND_ASSERT_THROW_MES_L1(expr, message) {if(!(expr)) {MWARNING(message); throw std::runtime_error(message);}}

namespace cryptonote
{
  static inline unsigned char *operator &(ec_point &point) {
    return &reinterpret_cast<unsigned char &>(point);
  }
  static inline const unsigned char *operator &(const ec_point &point) {
    return &reinterpret_cast<const unsigned char &>(point);
  }

  // a copy of rct::addKeys, since we can't link to libringct to avoid circular dependencies
  static void add_public_key(crypto::public_key &AB, const crypto::public_key &A, const crypto::public_key &B) {
      ge_p3 B2, A2;
      CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&B2, &B) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
      CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A2, &A) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
      ge_cached tmp2;
      ge_p3_to_cached(&tmp2, &B2);
      ge_p1p1 tmp3;
      ge_add(&tmp3, &A2, &tmp2);
      ge_p1p1_to_p3(&A2, &tmp3);
      ge_p3_tobytes(&AB, &A2);
  }
  //---------------------------------------------------------------
  static uint64_t get_transaction_weight_clawback(const bool plus, const size_t n_outputs, const size_t n_padded_outputs)
  {
    const uint64_t bp_base = (32 * ((plus ? 6 : 9) + 7 * 2)) / 2; // notional size of a 2 output proof, normalized to 1 proof (ie, divided by 2)
    if (n_padded_outputs <= 2)
      return 0;
    size_t nlr = 0;
    while ((1u << nlr) < n_padded_outputs)
      ++nlr;
    nlr += 6;
    const size_t bp_size = 32 * ((plus ? 6 : 9) + 2 * nlr);
    CHECK_AND_ASSERT_THROW_MES_L1(n_outputs <= BULLETPROOF_MAX_OUTPUTS, "maximum number of outputs is " + std::to_string(BULLETPROOF_MAX_OUTPUTS) + " per transaction");
    CHECK_AND_ASSERT_THROW_MES_L1(bp_base * n_padded_outputs >= bp_size, "Invalid bulletproof clawback: bp_base " + std::to_string(bp_base) + ", n_padded_outputs "
        + std::to_string(n_padded_outputs) + ", bp_size " + std::to_string(bp_size));
    const uint64_t bp_clawback = (bp_base * n_padded_outputs - bp_size) * 4 / 5;
    return bp_clawback;
  }

  static void get_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h)
  {
    tree_hash(tx_hashes.data(), tx_hashes.size(), h);
  }

  static crypto::hash get_tree_hash(const std::vector<crypto::hash>& tx_hashes)
  {
    crypto::hash h = null_hash;
    get_tree_hash(tx_hashes, h);
    return h;
  }
  //---------------------------------------------------------------
  static uint64_t get_transaction_weight_clawback(const transaction &tx, const size_t n_padded_outputs)
  {
    const rct::rctSig &rv = tx.rct_signatures;
    const bool plus = rv.type == rct::RCTTypeBulletproofPlus || rv.type == rct::RCTTypeFcmpPlusPlus;
    const size_t n_outputs = tx.vout.size();
    return get_transaction_weight_clawback(plus, n_outputs, n_padded_outputs);
  }
  //---------------------------------------------------------------
  // Helper function to group outputs by last locked block idx
  static uint64_t set_tx_outs_by_last_locked_block(const cryptonote::transaction &tx,
    const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments,
    const uint64_t &first_output_id,
    const uint64_t block_idx,
    fcmp_pp::curve_trees::OutsByLastLockedBlock &outs_by_last_locked_block_inout,
    std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/> &timelocked_outputs_inout)
  {
    const uint64_t last_locked_block = cryptonote::get_last_locked_block_index(tx.unlock_time, block_idx);
    const bool has_custom_timelock = cryptonote::is_custom_timelocked(cryptonote::is_coinbase(tx),
      last_locked_block,
      block_idx);

    for (std::size_t i = 0; i < tx.vout.size(); ++i)
    {
      const uint64_t output_id = first_output_id + i;
      const auto &out = tx.vout[i];

      crypto::public_key output_public_key;
      CHECK_AND_ASSERT_THROW_MES(cryptonote::get_output_public_key(out, output_public_key),
          "failed to get out pubkey");

      rct::key commitment;
      CHECK_AND_ASSERT_THROW_MES(cryptonote::get_commitment(tx, i, transparent_amount_commitments, commitment),
          "failed to get tx commitment");

      const fcmp_pp::curve_trees::OutputContext output_context{
              .output_id       = output_id,
              .torsion_checked = cryptonote::tx_outs_checked_for_torsion(tx),
              .output_pair     = fcmp_pp::curve_trees::OutputPair{output_public_key, commitment}
          };

      if (has_custom_timelock)
      {
          timelocked_outputs_inout[output_id] = last_locked_block;
      }

      outs_by_last_locked_block_inout[last_locked_block].emplace_back(output_context);
    }

    return tx.vout.size();
  }
  //---------------------------------------------------------------
}

namespace cryptonote
{
  //---------------------------------------------------------------
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h, hw::device &hwdev)
  {
    hwdev.get_transaction_prefix_hash(tx,h);    
  }

  //---------------------------------------------------------------  
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx, hw::device &hwdev)
  {
    crypto::hash h = null_hash;
    get_transaction_prefix_hash(tx, h, hwdev);
    return h;
  }
  
  bool expand_transaction_1(transaction &tx, bool base_only)
  {
    if (tx.version >= 2 && !is_coinbase(tx))
    {
      rct::rctSig &rv = tx.rct_signatures;
      if (rv.type == rct::RCTTypeNull)
        return true;
      if (rv.outPk.size() != tx.vout.size())
      {
        LOG_PRINT_L1("Failed to parse transaction from blob, bad outPk size in tx " << get_transaction_hash(tx));
        return false;
      }
      for (size_t n = 0; n < tx.rct_signatures.outPk.size(); ++n)
      {
        crypto::public_key output_public_key;
        if (!get_output_public_key(tx.vout[n], output_public_key))
        {
          LOG_PRINT_L1("Failed to get output public key for output " << n << " in tx " << get_transaction_hash(tx));
          return false;
        }
        rv.outPk[n].dest = rct::pk2rct(output_public_key);
      }

      if (!base_only)
      {
        const bool bulletproof = rct::is_rct_bulletproof(rv.type);
        const bool bulletproof_plus = rct::is_rct_bulletproof_plus(rv.type);
        if (bulletproof_plus)
        {
          if (rv.p.bulletproofs_plus.size() != 1)
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs_plus size in tx " << get_transaction_hash(tx));
            return false;
          }
          if (rv.p.bulletproofs_plus[0].L.size() < 6)
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs_plus L size in tx " << get_transaction_hash(tx));
            return false;
          }
          const size_t max_outputs = rct::n_bulletproof_plus_max_amounts(rv.p.bulletproofs_plus[0]);
          if (max_outputs < tx.vout.size())
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs_plus max outputs in tx " << get_transaction_hash(tx));
            return false;
          }
          const size_t n_amounts = tx.vout.size();
          CHECK_AND_ASSERT_MES(n_amounts == rv.outPk.size(), false, "Internal error filling out V");
          rv.p.bulletproofs_plus[0].V.resize(n_amounts);
          for (size_t i = 0; i < n_amounts; ++i)
            rv.p.bulletproofs_plus[0].V[i] = rct::scalarmultKey(rv.outPk[i].mask, rct::INV_EIGHT);
        }
        else if (bulletproof)
        {
          if (rv.p.bulletproofs.size() != 1)
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs size in tx " << get_transaction_hash(tx));
            return false;
          }
          if (rv.p.bulletproofs[0].L.size() < 6)
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs L size in tx " << get_transaction_hash(tx));
            return false;
          }
          const size_t max_outputs = 1 << (rv.p.bulletproofs[0].L.size() - 6);
          if (max_outputs < tx.vout.size())
          {
            LOG_PRINT_L1("Failed to parse transaction from blob, bad bulletproofs max outputs in tx " << get_transaction_hash(tx));
            return false;
          }
          const size_t n_amounts = tx.vout.size();
          CHECK_AND_ASSERT_MES(n_amounts == rv.outPk.size(), false, "Internal error filling out V");
          rv.p.bulletproofs[0].V.resize(n_amounts);
          for (size_t i = 0; i < n_amounts; ++i)
            rv.p.bulletproofs[0].V[i] = rct::scalarmultKey(rv.outPk[i].mask, rct::INV_EIGHT);
        }
      }
    }
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(tx_blob)};
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    CHECK_AND_ASSERT_MES(expand_transaction_1(tx, false), false, "Failed to expand transaction data");
    tx.invalidate_hashes();
    tx.set_blob_size(tx_blob.size());
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_base_from_blob(const blobdata_ref& tx_blob, transaction& tx)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(tx_blob)};
    bool r = tx.serialize_base(ba);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    CHECK_AND_ASSERT_MES(expand_transaction_1(tx, true), false, "Failed to expand transaction data");
    tx.invalidate_hashes();
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_prefix_from_blob(const blobdata_ref& tx_blob, transaction_prefix& tx)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(tx_blob)};
    bool r = ::serialization::serialize_noeof(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction prefix from blob");
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx, crypto::hash& tx_hash)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(tx_blob)};
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    CHECK_AND_ASSERT_MES(expand_transaction_1(tx, false), false, "Failed to expand transaction data");
    tx.invalidate_hashes();
    tx.set_blob_size(tx_blob.size());
    //TODO: validate tx

    return get_transaction_hash(tx, tx_hash);
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash)
  {
    if (!parse_and_validate_tx_from_blob(tx_blob, tx, tx_hash))
      return false;
    get_transaction_prefix_hash(tx, tx_prefix_hash);
    return true;
  }
  //---------------------------------------------------------------
  bool is_v1_tx(const blobdata_ref& tx_blob)
  {
    uint64_t version;
    const char* begin = static_cast<const char*>(tx_blob.data());
    const char* end = begin + tx_blob.size();
    int read = tools::read_varint(begin, end, version);
    if (read <= 0)
      throw std::runtime_error("Internal error getting transaction version");
    return version <= 1;
  }
  //---------------------------------------------------------------
  bool is_v1_tx(const blobdata& tx_blob)
  {
    return is_v1_tx(blobdata_ref{tx_blob.data(), tx_blob.size()});
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper(const account_keys& ack, const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, const crypto::public_key& out_key, const crypto::public_key& tx_public_key, const std::vector<crypto::public_key>& additional_tx_public_keys, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki, hw::device &hwdev)
  {
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    bool r = hwdev.generate_key_derivation(tx_public_key, ack.m_view_secret_key, recv_derivation);
    if (!r)
    {
      MWARNING("key image helper: failed to generate_key_derivation(" << tx_public_key << ", <viewkey>)");
      memcpy(&recv_derivation, rct::identity().bytes, sizeof(recv_derivation));
    }

    std::vector<crypto::key_derivation> additional_recv_derivations;
    for (size_t i = 0; i < additional_tx_public_keys.size(); ++i)
    {
      crypto::key_derivation additional_recv_derivation = AUTO_VAL_INIT(additional_recv_derivation);
      r = hwdev.generate_key_derivation(additional_tx_public_keys[i], ack.m_view_secret_key, additional_recv_derivation);
      if (!r)
      {
        MWARNING("key image helper: failed to generate_key_derivation(" << additional_tx_public_keys[i] << ", <viewkey>)");
      }
      else
      {
        additional_recv_derivations.push_back(additional_recv_derivation);
      }
    }

    boost::optional<subaddress_receive_info> subaddr_recv_info = is_out_to_acc_precomp(subaddresses, out_key, recv_derivation, additional_recv_derivations, real_output_index,hwdev);
    CHECK_AND_ASSERT_MES(subaddr_recv_info, false, "key image helper: given output pubkey doesn't seem to belong to this address");

    return generate_key_image_helper_precomp(ack, out_key, subaddr_recv_info->derivation, real_output_index, subaddr_recv_info->index, in_ephemeral, ki, hwdev);
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper_precomp(const account_keys& ack, const crypto::public_key& out_key, const crypto::key_derivation& recv_derivation, size_t real_output_index, const subaddress_index& received_index, keypair& in_ephemeral, crypto::key_image& ki, hw::device &hwdev)
  {
    if (hwdev.compute_key_image(ack, out_key, recv_derivation, real_output_index, received_index, in_ephemeral, ki))
    {
      return true;
    }

    if (ack.m_spend_secret_key == crypto::null_skey)
    {
      // for watch-only wallet, simply copy the known output pubkey
      in_ephemeral.pub = out_key;
      in_ephemeral.sec = crypto::null_skey;
    }
    else
    {
      // derive secret key with subaddress - step 1: original CN derivation
      crypto::secret_key scalar_step1;
      crypto::secret_key spend_skey = crypto::null_skey;

      if (ack.m_multisig_keys.empty())
      {
        // if not multisig, use normal spend skey
        spend_skey = ack.m_spend_secret_key;
      }
      else
      {
        // if multisig, use sum of multisig privkeys (local account's share of aggregate spend key)
        for (const auto &multisig_key : ack.m_multisig_keys)
        {
          sc_add((unsigned char*)spend_skey.data,
            (const unsigned char*)multisig_key.data,
            (const unsigned char*)spend_skey.data);
        }
      }

      // computes Hs(a*R || idx) + b
      hwdev.derive_secret_key(recv_derivation, real_output_index, spend_skey, scalar_step1);

      // step 2: add Hs(a || index_major || index_minor)
      crypto::secret_key subaddr_sk;
      crypto::secret_key scalar_step2;
      if (received_index.is_zero())
      {
        scalar_step2 = scalar_step1;    // treat index=(0,0) as a special case representing the main address
      }
      else
      {
        subaddr_sk = hwdev.get_subaddress_secret_key(ack.m_view_secret_key, received_index);
        hwdev.sc_secret_add(scalar_step2, scalar_step1,subaddr_sk);
      }

      in_ephemeral.sec = scalar_step2;

      if (ack.m_multisig_keys.empty())
      {
        // when not in multisig, we know the full spend secret key, so the output pubkey can be obtained by scalarmultBase
        CHECK_AND_ASSERT_MES(hwdev.secret_key_to_public_key(in_ephemeral.sec, in_ephemeral.pub), false, "Failed to derive public key");
      }
      else
      {
        // when in multisig, we only know the partial spend secret key. but we do know the full spend public key, so the output pubkey can be obtained by using the standard CN key derivation
        CHECK_AND_ASSERT_MES(hwdev.derive_public_key(recv_derivation, real_output_index, ack.m_account_address.m_spend_public_key, in_ephemeral.pub), false, "Failed to derive public key");
        // and don't forget to add the contribution from the subaddress part
        if (!received_index.is_zero())
        {
          crypto::public_key subaddr_pk;
          CHECK_AND_ASSERT_MES(hwdev.secret_key_to_public_key(subaddr_sk, subaddr_pk), false, "Failed to derive public key");
          add_public_key(in_ephemeral.pub, in_ephemeral.pub, subaddr_pk);
        }
      }

      CHECK_AND_ASSERT_MES(in_ephemeral.pub == out_key,
           false, "key image helper precomp: given output pubkey doesn't match the derived one");
    }

    hwdev.generate_key_image(in_ephemeral.pub, in_ephemeral.sec, ki);
    return true;
  }
  //---------------------------------------------------------------
  uint64_t power_integral(uint64_t a, uint64_t b)
  {
    if(b == 0)
      return 1;
    uint64_t total = a;
    for(uint64_t i = 1; i != b; i++)
      total *= a;
    return total;
  }
  //---------------------------------------------------------------
  bool parse_amount(uint64_t& amount, const std::string& str_amount_)
  {
    std::string str_amount = str_amount_;
    boost::algorithm::trim(str_amount);

    size_t point_index = str_amount.find_first_of('.');
    size_t fraction_size;
    if (std::string::npos != point_index)
    {
      fraction_size = str_amount.size() - point_index - 1;
      while (default_decimal_point < fraction_size && '0' == str_amount.back())
      {
        str_amount.erase(str_amount.size() - 1, 1);
        --fraction_size;
      }
      if (default_decimal_point < fraction_size)
        return false;
      str_amount.erase(point_index, 1);
    }
    else
    {
      fraction_size = 0;
    }

    if (str_amount.empty())
      return false;

    if (fraction_size < default_decimal_point)
    {
      str_amount.append(default_decimal_point - fraction_size, '0');
    }

    return string_tools::get_xtype_from_string(amount, str_amount);
  }
  //---------------------------------------------------------------
  uint64_t get_fcmp_pp_prefix_weight_v1(const size_t n_inputs, const size_t n_outputs, const size_t extra_len)
  {
    MTRACE(__func__ << "(n_inputs=" << n_inputs << ", n_outputs=" << n_outputs << ", extra_len=" << extra_len);

    CHECK_AND_ASSERT_MES(n_inputs && n_inputs <= FCMP_PLUS_PLUS_MAX_INPUTS,
      std::numeric_limits<uint64_t>::max(),
      "get_fcmp_pp_transaction_weight_v1: invalid n_inputs");
    CHECK_AND_ASSERT_MES(n_outputs >= 2 && n_outputs <= FCMP_PLUS_PLUS_MAX_OUTPUTS,
      std::numeric_limits<uint64_t>::max(),
      "get_fcmp_pp_transaction_weight_v1: invalid n_outputs");
    CHECK_AND_ASSERT_MES(extra_len <= MAX_TX_EXTRA_SIZE,
      std::numeric_limits<uint64_t>::max(),
      "get_fcmp_pp_transaction_weight_v1: invalid extra_len");

    static constexpr uint64_t txin_to_key_weight = 1 /*amount=0*/ + 1 /*key_offsets.size()=0*/ + 32 /*k_image*/;
    static constexpr uint64_t txout_to_carrot_weight = 32 /*key*/ + 3 /*view_tag*/ + 16 /*encrypted_janus_anchor*/;
    static constexpr uint64_t tx_out_weight = 1 /*amount=0*/ + txout_to_carrot_weight + 1 /*txout_target_v tag*/;

    return
      1 /*version=2*/
      + 1 /*unlock_time=0*/
      + 1 /*vin.size()<=FCMP_PLUS_PLUS_MAX_INPUTS*/
      + n_inputs * (txin_to_key_weight /*txin_to_key*/ + 1 /*txin_v tag*/)
      + 1 /*vout.size()<=FCMP_PLUS_PLUS_MAX_OUTPUTS*/
      + (n_outputs * tx_out_weight /*tx_out*/)
      + (extra_len >= 128 ? 2 : 1) /*extra.size()*/
      + extra_len;
  }
  //---------------------------------------------------------------
  uint64_t get_fcmp_pp_unprunable_weight_v1(const size_t n_inputs, const size_t n_outputs, const size_t extra_len)
  {
    MTRACE(__func__ << "(n_inputs=" << n_inputs << ", n_outputs=" << n_outputs << ", extra_len=" << extra_len);

    const uint64_t prefix_weight = get_fcmp_pp_prefix_weight_v1(n_inputs, n_outputs, extra_len);
    if (prefix_weight == std::numeric_limits<uint64_t>::max())
      return prefix_weight;

    static constexpr uint64_t max_u64_varint_len = 10;        // size of varint storing 2**64-1
    static constexpr uint64_t rct_sig_base_per_out_weight = 8 /*ecdhInfo.at(i).amount*/ + 32 /*outPk.at(i).mask*/;

    return prefix_weight
      + 1 /*type*/
      + max_u64_varint_len /*txnFee*/
      + (n_outputs * rct_sig_base_per_out_weight);
  }
  //---------------------------------------------------------------
  uint64_t get_fcmp_pp_transaction_weight_v1(const size_t n_inputs, const size_t n_outputs, const size_t extra_len)
  {
    MTRACE(__func__ << "(n_inputs=" << n_inputs << ", n_outputs=" << n_outputs << ", extra_len=" << extra_len);

    const uint64_t unprunable_weight = get_fcmp_pp_unprunable_weight_v1(n_inputs, n_outputs, extra_len);
    if (unprunable_weight == std::numeric_limits<uint64_t>::max())
      return unprunable_weight;

    static constexpr uint64_t max_block_index_varint_len = 5; // size of varint storing CRYPTONOTE_MAX_BLOCK_NUMBER

    static constexpr uint64_t rerandomized_output_weight = FCMP_PP_INPUT_TUPLE_SIZE_V1 + 32 /*C~ AKA pseudoOut*/;

    const uint64_t total_sal_weight = n_inputs * (rerandomized_output_weight + FCMP_PP_SAL_PROOF_SIZE_V1);
    const uint64_t misc_fcmp_pp_weight = max_block_index_varint_len /*reference_block*/ + 1 /*n_tree_layers*/;

    // Calculate deterministic bulletproofs size (assumes canonical BP format)
    size_t nrl = 0, n_padded_outputs;
    while ((n_padded_outputs = (1u << nrl)) < n_outputs)
      ++nrl;
    nrl += 6;
    uint64_t bp_weight = 32 * (6 + 2 * nrl) + 2;
    bp_weight += 1 /*nbp*/;

    // BP+ clawback to price in linear verification times
    const uint64_t bp_clawback = get_transaction_weight_clawback(/*plus=*/true, n_outputs, n_padded_outputs);
    MDEBUG("bulletproof+ clawback: " << bp_clawback);
    CHECK_AND_ASSERT_MES(std::numeric_limits<uint64_t>::max() - bp_clawback > bp_weight,
      std::numeric_limits<uint64_t>::max(),
      "get_fcmp_pp_transaction_weight_v1: overflow with bulletproof clawback");
    bp_weight += bp_clawback;

    // Much like bulletproofs, the verification time of a FCMP is linear in the number of inputs,
    // rounded up to the nearest power of 2, so round n_inputs up to power of 2 to price this in
    size_t n_padded_inputs = 1;
    while (n_padded_inputs < n_inputs)
      n_padded_inputs *= 2;

    // There's a few reasons why we treat n_tree_layers as a fixed value for weight calculation:
    //     a. If we took n_tree_layers into account when calculating weight, then fee calculation
    //        would be a function of the number of layers in the FCMP tree. This has a couple
    //        implications:
    //            i.  To determine the "correct" fee in multi-signer/cold-signer contexts, signers
    //                would have to transmit and agree upon what the current n_tree_layers value is,
    //                which complicates these protocols, and is inherently difficult to validate
    //                for offline signers. It also just complicates the process for normal wallets.
    //            ii. If signers need guarantees that a signature for a transaction proposal with a
    //                certain fee isn't reused for similar transaction but with a different
    //                n_tree_layers, and thus weight, then n_tree_layers would have to be included
    //                in rctSigBase and hashed into the signable_tx_hash, which means an extra byte
    //                per pruned transaction when wallets are refreshing. Also, more subjectively,
    //                putting n_tree_layers into rctSigBase feels misplaced.
    //     b. Dropping the weight for low values of n_tree_layers directly incentivizes spenders of
    //        old enotes to use as small a value of n_tree_layers as possible, which hurts their
    //        anonymity.
    //
    // We chose 7 specifically because at the time of writing (9 April 2025), the current layer size
    // of the Monero mainnet would be 6. 7 is approaching relatively quickly, and would be the value
    // for many decades at the current tx volume.
    static constexpr size_t fake_n_tree_layers = 7;

    const uint64_t fcmp_weight_base = fcmp_pp::membership_proof_len(/*n_inputs=*/1, fake_n_tree_layers);
    const uint64_t fcmp_weight = fcmp_weight_base * n_padded_inputs;

    const uint64_t rct_sig_prunable_weight = bp_weight + total_sal_weight + misc_fcmp_pp_weight + fcmp_weight;

    return unprunable_weight + rct_sig_prunable_weight;
  }
  //---------------------------------------------------------------
  uint64_t get_fcmp_pp_transaction_weight_v1(const transaction_prefix &tx_prefix)
  {
    return get_fcmp_pp_transaction_weight_v1(tx_prefix.vin.size(), tx_prefix.vout.size(), tx_prefix.extra.size());
  }
  //---------------------------------------------------------------
  uint64_t get_transaction_weight(const transaction &tx, size_t blob_size)
  {
    CHECK_AND_ASSERT_MES(!tx.pruned, std::numeric_limits<uint64_t>::max(), "get_transaction_weight does not support pruned txes");
    CHECK_AND_ASSERT_MES(tx.rct_signatures.type <= rct::RCTTypeFcmpPlusPlus,
      std::numeric_limits<uint64_t>::max(),
      "get_transaction_weight does not support transactions newer than FCMP++ v1");
    if (tx.version < 2)
      return blob_size;
    else if (tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus)
      return get_fcmp_pp_transaction_weight_v1(tx);
    const rct::rctSig &rv = tx.rct_signatures;
    const bool bulletproof = rct::is_rct_bulletproof(rv.type);
    const bool bulletproof_plus = rct::is_rct_bulletproof_plus(rv.type);
    if (!bulletproof && !bulletproof_plus)
      return blob_size;
    const size_t n_padded_outputs = bulletproof_plus ? rct::n_bulletproof_plus_max_amounts(rv.p.bulletproofs_plus) : rct::n_bulletproof_max_amounts(rv.p.bulletproofs);
    uint64_t bp_clawback = get_transaction_weight_clawback(tx, n_padded_outputs);
    CHECK_AND_ASSERT_THROW_MES_L1(bp_clawback <= std::numeric_limits<uint64_t>::max() - blob_size, "Weight overflow");
    return blob_size + bp_clawback;
  }
  //---------------------------------------------------------------
  uint64_t get_pruned_transaction_weight(const transaction &tx)
  {
    CHECK_AND_ASSERT_MES(tx.pruned, std::numeric_limits<uint64_t>::max(), "get_pruned_transaction_weight does not support non pruned txes");
    CHECK_AND_ASSERT_MES(tx.version >= 2, std::numeric_limits<uint64_t>::max(), "get_pruned_transaction_weight does not support v1 txes");

    if (tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus)
      return get_fcmp_pp_transaction_weight_v1(tx);

    CHECK_AND_ASSERT_MES(tx.rct_signatures.type == rct::RCTTypeBulletproof2 || tx.rct_signatures.type == rct::RCTTypeCLSAG || tx.rct_signatures.type == rct::RCTTypeBulletproofPlus,
        std::numeric_limits<uint64_t>::max(), "Unsupported rct_signatures type in get_pruned_transaction_weight");
    CHECK_AND_ASSERT_MES(!tx.vin.empty(), std::numeric_limits<uint64_t>::max(), "empty vin");
    CHECK_AND_ASSERT_MES(tx.vin[0].type() == typeid(cryptonote::txin_to_key), std::numeric_limits<uint64_t>::max(), "empty vin");

    // get pruned data size
    std::ostringstream s;
    binary_archive<true> a(s);
    ::serialization::serialize(a, const_cast<transaction&>(tx));
    uint64_t weight = s.str().size(), extra;

    // nbps (technically varint)
    weight += 1;

    // calculate deterministic bulletproofs size (assumes canonical BP format)
    size_t nrl = 0, n_padded_outputs;
    while ((n_padded_outputs = (1u << nrl)) < tx.vout.size())
      ++nrl;
    nrl += 6;
    extra = 32 * ((rct::is_rct_bulletproof_plus(tx.rct_signatures.type) ? 6 : 9) + 2 * nrl) + 2;
    weight += extra;

    // calculate deterministic CLSAG/MLSAG data size
    // TODO: update for fcmp_pp
    const size_t ring_size = boost::get<cryptonote::txin_to_key>(tx.vin[0]).key_offsets.size();
    if (rct::is_rct_clsag(tx.rct_signatures.type))
      extra = tx.vin.size() * (ring_size + 2) * 32;
    else
      extra = tx.vin.size() * (ring_size * (1 + 1) * 32 + 32 /* cc */);
    weight += extra;

    // calculate deterministic pseudoOuts size
    extra =  32 * (tx.vin.size());
    weight += extra;

    // clawback
    uint64_t bp_clawback = get_transaction_weight_clawback(tx, n_padded_outputs);
    CHECK_AND_ASSERT_THROW_MES_L1(bp_clawback <= std::numeric_limits<uint64_t>::max() - weight, "Weight overflow");
    weight += bp_clawback;

    return weight;
  }
  //---------------------------------------------------------------
  uint64_t get_transaction_weight(const transaction &tx)
  {
    size_t blob_size;
    if (tx.is_blob_size_valid())
    {
      blob_size = tx.blob_size;
    }
    else
    {
      std::ostringstream s;
      binary_archive<true> a(s);
      ::serialization::serialize(a, const_cast<transaction&>(tx));
      blob_size = s.str().size();
    }
    return get_transaction_weight(tx, blob_size);
  }
  //---------------------------------------------------------------
  uint64_t get_transaction_blob_size(const transaction& tx)
  {
    if (!tx.is_blob_size_valid())
    {
      const cryptonote::blobdata tx_blob = tx_to_blob(tx);
      tx.set_blob_size(tx_blob.size());
    }

    CHECK_AND_ASSERT_THROW_MES(tx.is_blob_size_valid(), "BUG: blob size valid not set");

    return tx.blob_size;
  }
  //---------------------------------------------------------------
  bool get_tx_fee(const transaction& tx, uint64_t & fee)
  {
    if (tx.version > 1)
    {
      fee = tx.rct_signatures.txnFee;
      return true;
    }
    uint64_t amount_in = 0;
    uint64_t amount_out = 0;
    for(auto& in: tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key), 0, "unexpected type id in transaction");
      amount_in += boost::get<txin_to_key>(in).amount;
    }
    for(auto& o: tx.vout)
      amount_out += o.amount;

    CHECK_AND_ASSERT_MES(amount_in >= amount_out, false, "transaction spend (" <<amount_in << ") more than it has (" << amount_out << ")");
    fee = amount_in - amount_out;
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_fee(const transaction& tx)
  {
    uint64_t r = 0;
    if(!get_tx_fee(tx, r))
      return 0;
    return r;
  }
  //---------------------------------------------------------------
  bool parse_tx_extra(const std::vector<uint8_t>& tx_extra, std::vector<tx_extra_field>& tx_extra_fields)
  {
    tx_extra_fields.clear();

    if(tx_extra.empty())
      return true;

    binary_archive<false> ar{epee::to_span(tx_extra)};

    do
    {
      tx_extra_field field;
      bool r = ::do_serialize(ar, field);
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
      tx_extra_fields.push_back(field);
    } while (!ar.eof());
    CHECK_AND_NO_ASSERT_MES_L1(::serialization::check_stream_state(ar), false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));

    return true;
  }
  //---------------------------------------------------------------
  template<typename T>
  static bool pick(binary_archive<true> &ar, std::vector<tx_extra_field> &fields, uint8_t tag)
  {
    std::vector<tx_extra_field>::iterator it;
    while ((it = std::find_if(fields.begin(), fields.end(), [](const tx_extra_field &f) { return f.type() == typeid(T); })) != fields.end())
    {
      bool r = ::do_serialize(ar, tag);
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to serialize tx extra field");
      r = ::do_serialize(ar, boost::get<T>(*it));
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to serialize tx extra field");
      fields.erase(it);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool sort_tx_extra(const std::vector<uint8_t>& tx_extra, std::vector<uint8_t> &sorted_tx_extra, bool allow_partial)
  {
    std::vector<tx_extra_field> tx_extra_fields;

    if(tx_extra.empty())
    {
      sorted_tx_extra.clear();
      return true;
    }

    binary_archive<false> ar{epee::to_span(tx_extra)};

    size_t processed = 0;
    do
    {
      tx_extra_field field;
      bool r = ::do_serialize(ar, field);
      if (!r)
      {
        MWARNING("failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
        if (!allow_partial)
          return false;
        break;
      }
      tx_extra_fields.push_back(field);
      processed = ar.getpos();
    } while (!ar.eof());
    if (!::serialization::check_stream_state(ar))
    {
      MWARNING("failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
      if (!allow_partial)
        return false;
    }
    MTRACE("Sorted " << processed << "/" << tx_extra.size());

    std::ostringstream oss;
    binary_archive<true> nar(oss);

    // sort by:
    if (!pick<tx_extra_pub_key>(nar, tx_extra_fields, TX_EXTRA_TAG_PUBKEY)) return false;
    if (!pick<tx_extra_additional_pub_keys>(nar, tx_extra_fields, TX_EXTRA_TAG_ADDITIONAL_PUBKEYS)) return false;
    if (!pick<tx_extra_nonce>(nar, tx_extra_fields, TX_EXTRA_NONCE)) return false;
    if (!pick<tx_extra_merge_mining_tag>(nar, tx_extra_fields, TX_EXTRA_MERGE_MINING_TAG)) return false;
    if (!pick<tx_extra_mysterious_minergate>(nar, tx_extra_fields, TX_EXTRA_MYSTERIOUS_MINERGATE_TAG)) return false;
    if (!pick<tx_extra_padding>(nar, tx_extra_fields, TX_EXTRA_TAG_PADDING)) return false;

    // if not empty, someone added a new type and did not add a case above
    if (!tx_extra_fields.empty())
    {
      MERROR("tx_extra_fields not empty after sorting, someone forgot to add a case above");
      return false;
    }

    std::string oss_str = oss.str();
    if (allow_partial && processed < tx_extra.size())
    {
      MDEBUG("Appending unparsed data");
      oss_str += std::string((const char*)tx_extra.data() + processed, tx_extra.size() - processed);
    }
    sorted_tx_extra = std::vector<uint8_t>(oss_str.begin(), oss_str.end());
    return true;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const std::vector<uint8_t>& tx_extra, size_t pk_index)
  {
    std::vector<tx_extra_field> tx_extra_fields;
    parse_tx_extra(tx_extra, tx_extra_fields);

    tx_extra_pub_key pub_key_field;
    if(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, pk_index))
      return null_pkey;

    return pub_key_field.pub_key;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction_prefix& tx_prefix, size_t pk_index)
  {
    return get_tx_pub_key_from_extra(tx_prefix.extra, pk_index);
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx, size_t pk_index)
  {
    return get_tx_pub_key_from_extra(tx.extra, pk_index);
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    return add_tx_pub_key_to_extra(tx.extra, tx_pub_key);
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction_prefix& tx, const crypto::public_key& tx_pub_key)
  {
    return add_tx_pub_key_to_extra(tx.extra, tx_pub_key);
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(std::vector<uint8_t>& tx_extra, const crypto::public_key& tx_pub_key)
  {
    tx_extra.resize(tx_extra.size() + 1 + sizeof(crypto::public_key));
    tx_extra[tx_extra.size() - 1 - sizeof(crypto::public_key)] = TX_EXTRA_TAG_PUBKEY;
    *reinterpret_cast<crypto::public_key*>(&tx_extra[tx_extra.size() - sizeof(crypto::public_key)]) = tx_pub_key;
    return true;
  }
  //---------------------------------------------------------------
  std::vector<crypto::public_key> get_additional_tx_pub_keys_from_extra(const std::vector<uint8_t>& tx_extra)
  {
    // parse
    std::vector<tx_extra_field> tx_extra_fields;
    parse_tx_extra(tx_extra, tx_extra_fields);
    // find corresponding field
    tx_extra_additional_pub_keys additional_pub_keys;
    if(!find_tx_extra_field_by_type(tx_extra_fields, additional_pub_keys))
      return {};
    return additional_pub_keys.data;
  }
  //---------------------------------------------------------------
  std::vector<crypto::public_key> get_additional_tx_pub_keys_from_extra(const transaction_prefix& tx)
  {
    return get_additional_tx_pub_keys_from_extra(tx.extra);
  }
  //---------------------------------------------------------------
  bool add_additional_tx_pub_keys_to_extra(std::vector<uint8_t>& tx_extra, const std::vector<crypto::public_key>& additional_pub_keys)
  {
    // convert to variant
    tx_extra_field field = tx_extra_additional_pub_keys{ additional_pub_keys };
    // serialize
    std::ostringstream oss;
    binary_archive<true> ar(oss);
    bool r = ::do_serialize(ar, field);
    CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to serialize tx extra additional tx pub keys");
    // append
    std::string tx_extra_str = oss.str();
    size_t pos = tx_extra.size();
    tx_extra.resize(tx_extra.size() + tx_extra_str.size());
    memcpy(&tx_extra[pos], tx_extra_str.data(), tx_extra_str.size());
    return true;
  }
  //---------------------------------------------------------------
  bool add_extra_nonce_to_tx_extra(std::vector<uint8_t>& tx_extra, const blobdata& extra_nonce)
  {
    CHECK_AND_ASSERT_MES(extra_nonce.size() <= TX_EXTRA_NONCE_MAX_COUNT, false, "extra nonce could be 255 bytes max");
    size_t start_pos = tx_extra.size();
    tx_extra.resize(tx_extra.size() + 2 + extra_nonce.size());
    //write tag
    tx_extra[start_pos] = TX_EXTRA_NONCE;
    //write len
    ++start_pos;
    tx_extra[start_pos] = static_cast<uint8_t>(extra_nonce.size());
    //write data
    ++start_pos;
    memcpy(&tx_extra[start_pos], extra_nonce.data(), extra_nonce.size());
    return true;
  }
  //---------------------------------------------------------------
  bool add_mm_merkle_root_to_tx_extra(std::vector<uint8_t>& tx_extra, const crypto::hash& mm_merkle_root, uint64_t mm_merkle_tree_depth)
  {
    size_t start_pos = tx_extra.size();
    static const size_t max_varint_size = 16;
    tx_extra.resize(tx_extra.size() + 2 + 32 + max_varint_size);
    //write tag
    tx_extra[start_pos] = TX_EXTRA_MERGE_MINING_TAG;
    //write data size
    ++start_pos;
    const off_t len_bytes = start_pos;
    // one byte placeholder for length since we'll only know the size later after writing a varint
    tx_extra[start_pos] = 0;
    //write depth varint
    ++start_pos;
    uint8_t *ptr = &tx_extra[start_pos], *start = ptr;
    tools::write_varint(ptr, mm_merkle_tree_depth);
    //write data
    const size_t varint_size = ptr - start;
    start_pos += varint_size;
    memcpy(&tx_extra[start_pos], &mm_merkle_root, 32);
    tx_extra.resize(tx_extra.size() - (max_varint_size - varint_size));
    tx_extra[len_bytes] = 32 + varint_size;
    return true;
  }
  //---------------------------------------------------------------
  bool remove_field_from_tx_extra(std::vector<uint8_t>& tx_extra, const std::type_info &type)
  {
    if (tx_extra.empty())
      return true;
    std::string extra_str(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size());
    binary_archive<false> ar{epee::strspan<std::uint8_t>(extra_str)};
    std::ostringstream oss;
    binary_archive<true> newar(oss);

    do
    {
      tx_extra_field field;
      bool r = ::do_serialize(ar, field);
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
      if (field.type() != type)
        ::do_serialize(newar, field);
    } while (!ar.eof());
    CHECK_AND_NO_ASSERT_MES_L1(::serialization::check_stream_state(ar), false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
    tx_extra.clear();
    std::string s = oss.str();
    tx_extra.reserve(s.size());
    std::copy(s.begin(), s.end(), std::back_inserter(tx_extra));
    return true;
  }
  //---------------------------------------------------------------
  void set_payment_id_to_tx_extra_nonce(blobdata& extra_nonce, const crypto::hash& payment_id)
  {
    extra_nonce.clear();
    extra_nonce.push_back(TX_EXTRA_NONCE_PAYMENT_ID);
    const uint8_t* payment_id_ptr = reinterpret_cast<const uint8_t*>(&payment_id);
    std::copy(payment_id_ptr, payment_id_ptr + sizeof(payment_id), std::back_inserter(extra_nonce));
  }
  //---------------------------------------------------------------
  void set_encrypted_payment_id_to_tx_extra_nonce(blobdata& extra_nonce, const crypto::hash8& payment_id)
  {
    extra_nonce.clear();
    extra_nonce.push_back(TX_EXTRA_NONCE_ENCRYPTED_PAYMENT_ID);
    const uint8_t* payment_id_ptr = reinterpret_cast<const uint8_t*>(&payment_id);
    std::copy(payment_id_ptr, payment_id_ptr + sizeof(payment_id), std::back_inserter(extra_nonce));
  }
  //---------------------------------------------------------------
  bool get_payment_id_from_tx_extra_nonce(const blobdata& extra_nonce, crypto::hash& payment_id)
  {
    if(sizeof(crypto::hash) + 1 != extra_nonce.size())
      return false;
    if(TX_EXTRA_NONCE_PAYMENT_ID != extra_nonce[0])
      return false;
    payment_id = *reinterpret_cast<const crypto::hash*>(extra_nonce.data() + 1);
    return true;
  }
  //---------------------------------------------------------------
  bool get_encrypted_payment_id_from_tx_extra_nonce(const blobdata& extra_nonce, crypto::hash8& payment_id)
  {
    if(sizeof(crypto::hash8) + 1 != extra_nonce.size())
      return false;
    if (TX_EXTRA_NONCE_ENCRYPTED_PAYMENT_ID != extra_nonce[0])
      return false;
    payment_id = *reinterpret_cast<const crypto::hash8*>(extra_nonce.data() + 1);
    return true;
  }
  //---------------------------------------------------------------
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money)
  {
    money = 0;
    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      money += tokey_in.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const block& b)
  {
    CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 1, 0, "wrong miner tx in block: " << get_block_hash(b) << ", b.miner_tx.vin.size() != 1");
    CHECKED_GET_SPECIFIC_VARIANT(b.miner_tx.vin[0], const txin_gen, coinbase_in, 0);
    return coinbase_in.height;
  }
  //---------------------------------------------------------------
  bool check_inputs_types_supported(const transaction& tx)
  {
    for(const auto& in: tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key), false, "wrong variant type: "
        << in.type().name() << ", expected " << typeid(txin_to_key).name()
        << ", in transaction id=" << get_transaction_hash(tx));

    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_outs_valid(const transaction& tx)
  {
    for(const tx_out& out: tx.vout)
    {
      crypto::public_key output_public_key;
      CHECK_AND_ASSERT_MES(get_output_public_key(out, output_public_key), false, "Failed to get output public key (output type: "
        << out.target.type().name() << "), in transaction id=" << get_transaction_hash(tx));

      if (tx.version == 1)
      {
        CHECK_AND_NO_ASSERT_MES(0 < out.amount, false, "zero amount output in transaction id=" << get_transaction_hash(tx));
      }

      if(!check_key(output_public_key))
        return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_money_overflow(const transaction& tx)
  {
    return check_inputs_overflow(tx) && check_outs_overflow(tx);
  }
  //---------------------------------------------------------------
  bool check_inputs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      if(money > tokey_in.amount + money)
        return false;
      money += tokey_in.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool check_outs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    for(const auto& o: tx.vout)
    {
      if(money > o.amount + money)
        return false;
      money += o.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_outs_money_amount(const transaction& tx)
  {
    uint64_t outputs_amount = 0;
    for(const auto& o: tx.vout)
      outputs_amount += o.amount;
    return outputs_amount;
  }
  //---------------------------------------------------------------
  bool get_output_public_key(const cryptonote::tx_out& out, crypto::public_key& output_public_key)
  {
    // before HF_VERSION_VIEW_TAGS, outputs with public keys are of type txout_to_key
    // after HF_VERSION_VIEW_TAGS, outputs with public keys are of type txout_to_tagged_key
    // after HF_VERSION_FCMP_PLUS_PLUS, outputs with public keys are of type txout_to_carrot_v1
    if (out.target.type() == typeid(txout_to_key))
      output_public_key = boost::get< txout_to_key >(out.target).key;
    else if (out.target.type() == typeid(txout_to_tagged_key))
      output_public_key = boost::get< txout_to_tagged_key >(out.target).key;
    else if (out.target.type() == typeid(txout_to_carrot_v1))
      output_public_key = boost::get< txout_to_carrot_v1 >(out.target).key;
    else
    {
      LOG_ERROR("Unexpected output target type found: " << out.target.type().name());
      return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  boost::optional<crypto::view_tag> get_output_view_tag(const cryptonote::tx_out& out)
  {
    return out.target.type() == typeid(txout_to_tagged_key)
      ? boost::optional<crypto::view_tag>(boost::get< txout_to_tagged_key >(out.target).view_tag)
      : boost::optional<crypto::view_tag>();
  }
  //---------------------------------------------------------------
  bool get_commitment(const transaction& tx, std::size_t o_idx, const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments, rct::key &c_out)
  {
    static_assert(CURRENT_TRANSACTION_VERSION == 2, "This section of code was written with 2 tx versions in mind. "
      "Revisit this section and update for the new tx version.");
    CHECK_AND_ASSERT_THROW_MES(tx.version == 1 || tx.version == 2, "encountered unexpected tx version");

    if (tx.version >= 2 && !cryptonote::is_coinbase(tx))
    {
      CHECK_AND_ASSERT_MES(tx.rct_signatures.outPk.size() > o_idx, false, "get_commitment: o_idx must be < tx.rct_signatures.outPk.size()");
      c_out = tx.rct_signatures.outPk.at(o_idx).mask;
      return true;
    }

    // tx version 1 OR miner tx
    // return the pre-calculated transparent amount commitment
    CHECK_AND_ASSERT_MES(tx.vout.size() > o_idx, false, "get_commitment: o_idx must be < tx.vout.size()");
    const auto it = transparent_amount_commitments.find(tx.vout.at(o_idx).amount);
    CHECK_AND_ASSERT_MES(it != transparent_amount_commitments.end(), false, "get_commitment: transparent amount commitment missing");
    c_out = it->second;
    return true;
  }
  //---------------------------------------------------------------
  std::string short_hash_str(const crypto::hash& h)
  {
    std::string res = string_tools::pod_to_hex(h);
    CHECK_AND_ASSERT_MES(res.size() == 64, res, "wrong hash256 with string_tools::pod_to_hex conversion");
    auto erased_pos = res.erase(8, 48);
    res.insert(8, "....");
    return res;
  }
  //---------------------------------------------------------------
  void set_tx_out(const uint64_t amount, const crypto::public_key& output_public_key, const bool use_view_tags, const crypto::view_tag& view_tag, tx_out& out)
  {
    out.amount = amount;
    if (use_view_tags)
    {
      txout_to_tagged_key ttk;
      ttk.key = output_public_key;
      ttk.view_tag = view_tag;
      out.target = ttk;
    }
    else
    {
      txout_to_key tk;
      tk.key = output_public_key;
      out.target = tk;
    }
  }
  //---------------------------------------------------------------
  bool check_output_types(const transaction& tx, const uint8_t hf_version)
  {
    for (const auto &o: tx.vout)
    {
      if (hf_version > HF_VERSION_CARROT)
      {
        // from v18, require outputs be carrot outputs
        CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_carrot_v1), false, "wrong variant type: "
          << o.target.type().name() << ", expected txout_to_carrot_v1 in transaction id=" << get_transaction_hash(tx));
      }
      else if (hf_version == HF_VERSION_CARROT)
      {
        // during v17, require outputs be of type txout_to_tagged_key OR txout_to_carrot_v1
        // to allow grace period before requiring all to be txout_to_carrot_v1
        const std::type_info &o_type = o.target.type();
        CHECK_AND_ASSERT_MES(
          o_type == typeid(txout_to_carrot_v1) || o_type == typeid(txout_to_tagged_key),
          false, "wrong variant type: " << o_type.name()
          << ", expected txout_to_key or txout_to_tagged_key in transaction id=" << get_transaction_hash(tx));

        // require all outputs in a tx be of the same type
        const std::type_info &first_type = tx.vout.at(0).target.type();
        CHECK_AND_ASSERT_MES(o_type == first_type, false, "non-matching variant types: "
          << o_type.name() << " and " << first_type.name() << ", "
          << "expected matching variant types in transaction id=" << get_transaction_hash(tx));
      }
      else if (hf_version > HF_VERSION_VIEW_TAGS)
      {
        // from v16, require outputs have view tags
        CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_tagged_key), false, "wrong variant type: "
          << o.target.type().name() << ", expected txout_to_tagged_key in transaction id=" << get_transaction_hash(tx));
      }
      else if (hf_version < HF_VERSION_VIEW_TAGS)
      {
        // require outputs to be of type txout_to_key
        CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_key), false, "wrong variant type: "
          << o.target.type().name() << ", expected txout_to_key in transaction id=" << get_transaction_hash(tx));
      }
      else  //(hf_version == HF_VERSION_VIEW_TAGS)
      {
        // require outputs be of type txout_to_key OR txout_to_tagged_key
        // to allow grace period before requiring all to be txout_to_tagged_key
        CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_key) || o.target.type() == typeid(txout_to_tagged_key), false, "wrong variant type: "
          << o.target.type().name() << ", expected txout_to_key or txout_to_tagged_key in transaction id=" << get_transaction_hash(tx));

        // require all outputs in a tx be of the same type
        CHECK_AND_ASSERT_MES(o.target.type() == tx.vout[0].target.type(), false, "non-matching variant types: "
          << o.target.type().name() << " and " << tx.vout[0].target.type().name() << ", "
          << "expected matching variant types in transaction id=" << get_transaction_hash(tx));
      }
    }

    // during v17, require non-coinbase carrot txs use FCMP++ and legacy use BP+
    if (hf_version == HF_VERSION_CARROT && !cryptonote::is_coinbase(tx) && tx.vout.size())
    {
      const auto &o = tx.vout.front();
      CHECK_AND_ASSERT_MES(
        (o.target.type() == typeid(txout_to_carrot_v1) && tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus) ||
        (o.target.type() == typeid(txout_to_tagged_key) && tx.rct_signatures.type == rct::RCTTypeBulletproofPlus),
        false, "mismatched output type to tx proof type in transaction id=" << get_transaction_hash(tx));
    }

    return true;
  }
  //---------------------------------------------------------------
  bool tx_outs_checked_for_torsion(const transaction& tx)
  {
    for (const auto &o: tx.vout)
    {
      // This function only knows about these output types. If there is a new type, we want to check it for torsion too.
      const bool is_known_output_type = o.target.type() == typeid(txout_to_carrot_v1) || o.target.type() == typeid(txout_to_tagged_key) || o.target.type() == typeid(txout_to_key);
      CHECK_AND_ASSERT_THROW_MES(is_known_output_type, "unknown variant type: " << o.target.type().name() << "in transaction id=" << get_transaction_hash(tx));

      // We start checking for torsion at consensus with carrot outs
      if (o.target.type() != typeid(txout_to_carrot_v1))
        return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool out_can_be_to_acc(const boost::optional<crypto::view_tag>& view_tag_opt, const crypto::key_derivation& derivation, const size_t output_index, hw::device* hwdev)
  {
    // If there is no view tag to check, the output can possibly belong to the account.
    // Will need to derive the output pub key to be certain whether or not the output belongs to the account.
    if (!view_tag_opt)
      return true;

    crypto::view_tag view_tag = *view_tag_opt;

    // If the output's view tag does *not* match the derived view tag, the output should not belong to the account.
    // Therefore can fail out early to avoid expensive crypto ops needlessly deriving output public key to
    // determine if output belongs to the account.
    crypto::view_tag derived_view_tag;
    if (hwdev != nullptr)
    {
      bool r = hwdev->derive_view_tag(derivation, output_index, derived_view_tag);
      CHECK_AND_ASSERT_MES(r, false, "Failed to derive view tag");
    }
    else
    {
      crypto::derive_view_tag(derivation, output_index, derived_view_tag);
    }
    return view_tag == derived_view_tag;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_keys& acc, const crypto::public_key& output_public_key, const crypto::public_key& tx_pub_key, const std::vector<crypto::public_key>& additional_tx_pub_keys, size_t output_index, const boost::optional<crypto::view_tag>& view_tag_opt)
  {
    crypto::key_derivation derivation;
    bool r = acc.get_device().generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
    crypto::public_key pk;
    if (out_can_be_to_acc(view_tag_opt, derivation, output_index, &acc.get_device()))
    {
      r = acc.get_device().derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
      CHECK_AND_ASSERT_MES(r, false, "Failed to derive public key");
      if (pk == output_public_key)
        return true;
    }

    // try additional tx pubkeys if available
    if (!additional_tx_pub_keys.empty())
    {
      CHECK_AND_ASSERT_MES(output_index < additional_tx_pub_keys.size(), false, "wrong number of additional tx pubkeys");
      r = acc.get_device().generate_key_derivation(additional_tx_pub_keys[output_index], acc.m_view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      if (out_can_be_to_acc(view_tag_opt, derivation, output_index, &acc.get_device()))
      {
        r = acc.get_device().derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
        CHECK_AND_ASSERT_MES(r, false, "Failed to derive public key");
        return pk == output_public_key;
      }
    }
    return false;
  }
  //---------------------------------------------------------------
  boost::optional<subaddress_receive_info> is_out_to_acc_precomp(
    const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses,
    const crypto::public_key& out_key,
    const crypto::key_derivation& derivation,
    const epee::span<const crypto::key_derivation> additional_derivations,
    size_t output_index,
    hw::device &hwdev,
    const boost::optional<crypto::view_tag>& view_tag_opt)
  {
    // try the shared tx pubkey
    crypto::public_key subaddress_spendkey;
    if (out_can_be_to_acc(view_tag_opt, derivation, output_index, &hwdev))
    {
      CHECK_AND_ASSERT_MES(hwdev.derive_subaddress_public_key(out_key, derivation, output_index, subaddress_spendkey), boost::none, "Failed to derive subaddress public key");
      auto found = subaddresses.find(subaddress_spendkey);
      if (found != subaddresses.end())
        return subaddress_receive_info{ found->second, derivation };
    }

    // try additional tx pubkeys if available
    if (!additional_derivations.empty())
    {
      CHECK_AND_ASSERT_MES(output_index < additional_derivations.size(), boost::none, "wrong number of additional derivations");
      if (out_can_be_to_acc(view_tag_opt, additional_derivations[output_index], output_index, &hwdev))
      {
        CHECK_AND_ASSERT_MES(hwdev.derive_subaddress_public_key(out_key, additional_derivations[output_index], output_index, subaddress_spendkey), boost::none, "Failed to derive subaddress public key");
        auto found = subaddresses.find(subaddress_spendkey);
        if (found != subaddresses.end())
          return subaddress_receive_info{ found->second, additional_derivations[output_index] };
      }
    }
    return boost::none;
  }
  //---------------------------------------------------------------
  boost::optional<subaddress_receive_info> is_out_to_acc_precomp(
    const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses,
    const crypto::public_key& out_key,
    const crypto::key_derivation& derivation,
    const std::vector<crypto::key_derivation>& additional_derivations,
    size_t output_index,
    hw::device &hwdev,
    const boost::optional<crypto::view_tag>& view_tag_opt)
  {
    return is_out_to_acc_precomp(subaddresses,
      out_key,
      derivation,
      epee::to_span(additional_derivations),
      output_index,
      hwdev,
      view_tag_opt);
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
    if(null_pkey == tx_pub_key)
      return false;
    std::vector<crypto::public_key> additional_tx_pub_keys = get_additional_tx_pub_keys_from_extra(tx);
    return lookup_acc_outs(acc, tx, tx_pub_key, additional_tx_pub_keys, outs, money_transfered);
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, const std::vector<crypto::public_key>& additional_tx_pub_keys, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    CHECK_AND_ASSERT_MES(additional_tx_pub_keys.empty() || additional_tx_pub_keys.size() == tx.vout.size(), false, "wrong number of additional pubkeys" );
    money_transfered = 0;
    size_t i = 0;
    for(const tx_out& o:  tx.vout)
    {
      crypto::public_key output_public_key;
      CHECK_AND_ASSERT_MES(get_output_public_key(o, output_public_key), false, "unable to get output public key from transaction out" );
      if(is_out_to_acc(acc, output_public_key, tx_pub_key, additional_tx_pub_keys, i, get_output_view_tag(o)))
      {
        outs.push_back(i);
        money_transfered += o.amount;
      }
      i++;
    }
    return true;
  }
  //---------------------------------------------------------------
  void get_blob_hash(const blobdata_ref& blob, crypto::hash& res)
  {
    cn_fast_hash(blob.data(), blob.size(), res);
  }
  //---------------------------------------------------------------
  void get_blob_hash(const blobdata& blob, crypto::hash& res)
  {
    cn_fast_hash(blob.data(), blob.size(), res);
  }
  //---------------------------------------------------------------
  void set_default_decimal_point(unsigned int decimal_point)
  {
    switch (decimal_point)
    {
      case 12:
      case 9:
      case 6:
      case 3:
      case 0:
        default_decimal_point = decimal_point;
        break;
      default:
        ASSERT_MES_AND_THROW("Invalid decimal point specification: " << decimal_point);
    }
  }
  //---------------------------------------------------------------
  unsigned int get_default_decimal_point()
  {
    return default_decimal_point;
  }
  //---------------------------------------------------------------
  std::string get_unit(unsigned int decimal_point)
  {
    if (decimal_point == (unsigned int)-1)
      decimal_point = default_decimal_point;
    switch (decimal_point)
    {
      case 12:
        return "monero";
      case 9:
        return "millinero";
      case 6:
        return "micronero";
      case 3:
        return "nanonero";
      case 0:
        return "piconero";
      default:
        ASSERT_MES_AND_THROW("Invalid decimal point specification: " << decimal_point);
    }
  }
  //---------------------------------------------------------------
  static void insert_money_decimal_point(std::string &s, unsigned int decimal_point)
  {
    if (decimal_point == (unsigned int)-1)
      decimal_point = default_decimal_point;
    if(s.size() < decimal_point+1)
    {
      s.insert(0, decimal_point+1 - s.size(), '0');
    }
    if (decimal_point > 0)
      s.insert(s.size() - decimal_point, ".");
  }
  //---------------------------------------------------------------
  std::string print_money(uint64_t amount, unsigned int decimal_point)
  {
    std::string s = std::to_string(amount);
    insert_money_decimal_point(s, decimal_point);
    return s;
  }
  //---------------------------------------------------------------
  std::string print_money(const boost::multiprecision::uint128_t &amount, unsigned int decimal_point)
  {
    std::stringstream ss;
    ss << amount;
    std::string s = ss.str();
    insert_money_decimal_point(s, decimal_point);
    return s;
  }
  //---------------------------------------------------------------
  uint64_t round_money_up(uint64_t amount, unsigned significant_digits)
  {
    // round monetary amount up with the requested amount of significant digits

    CHECK_AND_ASSERT_THROW_MES(significant_digits > 0, "significant_digits must not be 0");
    static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "Unexpected unsigned long long size");

    // we don't need speed, so we do it via text, as it's easier to get right
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)amount);
    const size_t len = strlen(buf);
    if (len > significant_digits)
    {
      bool bump = false;
      char *ptr;
      for (ptr = buf + significant_digits; *ptr; ++ptr)
      {
        // bump digits by one if the following digits past significant digits were to be 5 or more
        if (*ptr != '0')
        {
          bump = true;
          *ptr = '0';
        }
      }
      ptr = buf + significant_digits;
      while (bump && ptr > buf)
      {
        --ptr;
        // bumping a nine overflows
        if (*ptr == '9')
          *ptr = '0';
        else
        {
          // bumping another digit means we can stop bumping (no carry)
          ++*ptr;
          bump = false;
        }
      }
      if (bump)
      {
        // carry reached the highest digit, we need to add a 1 in front
        size_t offset = strlen(buf);
        for (size_t i = offset + 1; i > 0; --i)
          buf[i] = buf[i - 1];
        buf[0] = '1';
      }
    }
    char *end = NULL;
    errno = 0;
    const unsigned long long ull = strtoull(buf, &end, 10);
    CHECK_AND_ASSERT_THROW_MES(ull != ULLONG_MAX || errno == 0, "Failed to parse rounded amount: " << buf);
    CHECK_AND_ASSERT_THROW_MES(ull != 0 || amount == 0, "Overflow in rounding");
    return ull;
  }
  //---------------------------------------------------------------
  std::string round_money_up(const std::string &s, unsigned significant_digits)
  {
    uint64_t amount;
    CHECK_AND_ASSERT_THROW_MES(parse_amount(amount, s), "Failed to parse amount: " << s);
    amount = round_money_up(amount, significant_digits);
    return print_money(amount);
  }
  //---------------------------------------------------------------
  crypto::hash get_blob_hash(const blobdata& blob)
  {
    crypto::hash h = null_hash;
    get_blob_hash(blob, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_blob_hash(const blobdata_ref& blob)
  {
    crypto::hash h = null_hash;
    get_blob_hash(blob, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_hash(const transaction& t)
  {
    crypto::hash h = null_hash;
    get_transaction_hash(t, h, NULL);
    CHECK_AND_ASSERT_THROW_MES(get_transaction_hash(t, h, NULL), "Failed to calculate transaction hash");
    return h;
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res)
  {
    return get_transaction_hash(t, res, NULL);
  }
  //---------------------------------------------------------------
  bool calculate_transaction_prunable_hash(const transaction& t, const cryptonote::blobdata_ref *blob, crypto::hash& res)
  {
    if (t.version == 1)
      return false;
    const unsigned int unprunable_size = t.unprunable_size;
    if (blob && unprunable_size)
    {
      CHECK_AND_ASSERT_MES(unprunable_size <= blob->size(), false, "Inconsistent transaction unprunable and blob sizes");
      cryptonote::get_blob_hash(blobdata_ref(blob->data() + unprunable_size, blob->size() - unprunable_size), res);
    }
    else
    {
      transaction &tt = const_cast<transaction&>(t);
      std::stringstream ss;
      binary_archive<true> ba(ss);
      const size_t inputs = t.vin.size();
      const size_t outputs = t.vout.size();
      const size_t mixin = (t.vin.empty() || t.rct_signatures.type == rct::RCTTypeFcmpPlusPlus || t.vin[0].type() != typeid(txin_to_key))
        ? 0 : boost::get<txin_to_key>(t.vin[0]).key_offsets.size() - 1;
      bool r = tt.rct_signatures.p.serialize_rctsig_prunable(ba, t.rct_signatures.type, inputs, outputs, mixin);
      CHECK_AND_ASSERT_MES(r, false, "Failed to serialize rct signatures prunable");
      cryptonote::get_blob_hash(ss.str(), res);
    }
    return true;
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_prunable_hash(const transaction& t, const cryptonote::blobdata_ref *blobdata)
  {
    crypto::hash res;
    if (t.is_prunable_hash_valid())
    {
#ifdef ENABLE_HASH_CASH_INTEGRITY_CHECK
      CHECK_AND_ASSERT_THROW_MES(!calculate_transaction_prunable_hash(t, blobdata, res) || t.hash == res, "tx hash cash integrity failure");
#endif
      res = t.prunable_hash;
      ++tx_hashes_cached_count;
      return res;
    }

    ++tx_hashes_calculated_count;
    CHECK_AND_ASSERT_THROW_MES(calculate_transaction_prunable_hash(t, blobdata, res), "Failed to calculate tx prunable hash");
    t.set_prunable_hash(res);
    return res;
  }
  //---------------------------------------------------------------
  crypto::hash get_pruned_transaction_hash(const transaction& t, const crypto::hash &pruned_data_hash)
  {
    // v1 transactions hash the entire blob
    CHECK_AND_ASSERT_THROW_MES(t.version > 1, "Hash for pruned v1 tx cannot be calculated");

    // v2 transactions hash different parts together, than hash the set of those hashes
    crypto::hash hashes[3];

    // prefix
    get_transaction_prefix_hash(t, hashes[0]);

    transaction &tt = const_cast<transaction&>(t);

    // base rct
    {
      std::stringstream ss;
      binary_archive<true> ba(ss);
      const size_t inputs = t.vin.size();
      const size_t outputs = t.vout.size();
      bool r = tt.rct_signatures.serialize_rctsig_base(ba, inputs, outputs);
      CHECK_AND_ASSERT_THROW_MES(r, "Failed to serialize rct signatures base");
      cryptonote::get_blob_hash(ss.str(), hashes[1]);
    }

    // prunable rct
    if (t.rct_signatures.type == rct::RCTTypeNull)
      hashes[2] = crypto::null_hash;
    else
      hashes[2] = pruned_data_hash;

    // the tx hash is the hash of the 3 hashes
    crypto::hash res = cn_fast_hash(hashes, sizeof(hashes));
    t.set_hash(res);
    return res;
  }
  //---------------------------------------------------------------
  bool calculate_transaction_hash(const transaction& t, crypto::hash& res, size_t* blob_size)
  {
    CHECK_AND_ASSERT_MES(!t.pruned, false, "Cannot calculate the hash of a pruned transaction");

    // v1 transactions hash the entire blob
    if (t.version == 1)
    {
      size_t ignored_blob_size, &blob_size_ref = blob_size ? *blob_size : ignored_blob_size;
      return get_object_hash(t, res, blob_size_ref);
    }

    // v2 transactions hash different parts together, than hash the set of those hashes
    crypto::hash hashes[3];

    // prefix
    get_transaction_prefix_hash(t, hashes[0]);

    const blobdata blob = tx_to_blob(t);
    const unsigned int unprunable_size = t.unprunable_size;
    const unsigned int prefix_size = t.prefix_size;

    // base rct
    CHECK_AND_ASSERT_MES(prefix_size <= unprunable_size && unprunable_size <= blob.size(), false, "Inconsistent transaction prefix, unprunable and blob sizes");
    cryptonote::get_blob_hash(blobdata_ref(blob.data() + prefix_size, unprunable_size - prefix_size), hashes[1]);

    // prunable rct
    if (t.rct_signatures.type == rct::RCTTypeNull)
    {
      hashes[2] = crypto::null_hash;
    }
    else
    {
      cryptonote::blobdata_ref blobref(blob);
      CHECK_AND_ASSERT_MES(calculate_transaction_prunable_hash(t, &blobref, hashes[2]), false, "Failed to get tx prunable hash");
    }

    // the tx hash is the hash of the 3 hashes
    res = cn_fast_hash(hashes, sizeof(hashes));

    // we still need the size
    if (blob_size)
    {
      if (!t.is_blob_size_valid())
      {
        t.set_blob_size(blob.size());
      }
      *blob_size = t.blob_size;
    }

    return true;
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t* blob_size)
  {
    if (t.is_hash_valid())
    {
#ifdef ENABLE_HASH_CASH_INTEGRITY_CHECK
      CHECK_AND_ASSERT_THROW_MES(!calculate_transaction_hash(t, res, blob_size) || t.hash == res, "tx hash cash integrity failure");
#endif
      res = t.hash;
      if (blob_size)
      {
        if (!t.is_blob_size_valid())
        {
          t.set_blob_size(get_object_blobsize(t));
        }
        *blob_size = t.blob_size;
      }
      ++tx_hashes_cached_count;
      return true;
    }
    ++tx_hashes_calculated_count;
    bool ret = calculate_transaction_hash(t, res, blob_size);
    if (!ret)
      return false;
    t.set_hash(res);
    if (blob_size)
    {
      t.set_blob_size(*blob_size);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size)
  {
    return get_transaction_hash(t, res, &blob_size);
  }
  //---------------------------------------------------------------
  blobdata get_block_hashing_blob(const block& b)
  {
    blobdata blob = t_serializable_object_to_blob(static_cast<block_header>(b));
    crypto::hash block_content_hash = get_block_content_hash(b);
    blob.append(reinterpret_cast<const char*>(&block_content_hash), sizeof(block_content_hash));
    blob.append(tools::get_varint_data(b.tx_hashes.size()+1));
    return blob;
  }
  //---------------------------------------------------------------
  bool calculate_block_hash(const block& b, crypto::hash& res, const blobdata_ref *blob)
  {
    blobdata bd;
    blobdata_ref bdref;
    if (!blob)
    {
      bd = block_to_blob(b);
      bdref = bd;
      blob = &bdref;
    }

    bool hash_result = get_object_hash(get_block_hashing_blob(b), res);
    if (!hash_result)
      return false;

    if (b.miner_tx.vin.size() == 1 && b.miner_tx.vin[0].type() == typeid(cryptonote::txin_gen))
    {
      const cryptonote::txin_gen &txin_gen = boost::get<cryptonote::txin_gen>(b.miner_tx.vin[0]);
      if (txin_gen.height != 202612)
        return true;
    }

    // EXCEPTION FOR BLOCK 202612
    const std::string correct_blob_hash_202612 = "3a8a2b3a29b50fc86ff73dd087ea43c6f0d6b8f936c849194d5c84c737903966";
    const std::string existing_block_id_202612 = "bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698";
    crypto::hash block_blob_hash = get_blob_hash(*blob);

    if (string_tools::pod_to_hex(block_blob_hash) == correct_blob_hash_202612)
    {
      string_tools::hex_to_pod(existing_block_id_202612, res);
      return true;
    }

    {
      // make sure that we aren't looking at a block with the 202612 block id but not the correct blobdata
      if (string_tools::pod_to_hex(res) == existing_block_id_202612)
      {
        LOG_ERROR("Block with block id for 202612 but incorrect block blob hash found!");
        res = null_hash;
        return false;
      }
    }
    return hash_result;
  }
  //---------------------------------------------------------------
  bool get_block_hash(const block& b, crypto::hash& res)
  {
    if (b.is_hash_valid())
    {
#ifdef ENABLE_HASH_CASH_INTEGRITY_CHECK
      CHECK_AND_ASSERT_THROW_MES(!calculate_block_hash(b, res) || b.hash == res, "block hash cash integrity failure");
#endif
      res = b.hash;
      ++block_hashes_cached_count;
      return true;
    }
    ++block_hashes_calculated_count;
    bool ret = calculate_block_hash(b, res);
    if (!ret)
      return false;
    b.set_hash(res);
    return true;
  }
  //---------------------------------------------------------------
  crypto::hash get_block_hash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_hash(b, p);
    return p;
  }
  //---------------------------------------------------------------
  std::vector<uint64_t> relative_output_offsets_to_absolute(const std::vector<uint64_t>& off)
  {
    std::vector<uint64_t> res = off;
    for(size_t i = 1; i < res.size(); i++)
      res[i] += res[i-1];
    return res;
  }
  //---------------------------------------------------------------
  std::vector<uint64_t> absolute_output_offsets_to_relative(const std::vector<uint64_t>& off)
  {
    std::vector<uint64_t> res = off;
    if(!off.size())
      return res;
    std::sort(res.begin(), res.end());//just to be sure, actually it is already should be sorted
    for(size_t i = res.size()-1; i != 0; i--)
      res[i] -= res[i-1];

    return res;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b, crypto::hash *block_hash)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(b_blob)};
    bool r = ::serialization::serialize(ba, b);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse block from blob");
    b.invalidate_hashes();
    b.miner_tx.invalidate_hashes();
    if (block_hash)
    {
      calculate_block_hash(b, *block_hash, &b_blob);
      ++block_hashes_calculated_count;
      b.set_hash(*block_hash);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b)
  {
    return parse_and_validate_block_from_blob(b_blob, b, NULL);
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b, crypto::hash &block_hash)
  {
    return parse_and_validate_block_from_blob(b_blob, b, &block_hash);
  }
  //---------------------------------------------------------------
  blobdata block_to_blob(const block& b)
  {
    return t_serializable_object_to_blob(b);
  }
  //---------------------------------------------------------------
  bool block_to_blob(const block& b, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(b, b_blob);
  }
  //---------------------------------------------------------------
  blobdata tx_to_blob(const transaction& tx)
  {
    return t_serializable_object_to_blob(tx);
  }
  //---------------------------------------------------------------
  bool tx_to_blob(const transaction& tx, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(tx, b_blob);
  }
  //---------------------------------------------------------------
  crypto::hash get_block_content_hash(const block& b)
  {
    std::vector<crypto::hash> hashes;
    hashes.reserve(1 + 1 + 1 + b.tx_hashes.size());
    // 1. n tree layers in FCMP++ tree
    static_assert(sizeof(crypto::hash) >= sizeof(uint8_t), "crypto::hash is too small");
    if (b.major_version >= HF_VERSION_FCMP_PLUS_PLUS)
      hashes.push_back(crypto::hash{static_cast<char>(b.fcmp_pp_n_tree_layers)});
    // 2. FCMP++ tree root
    static_assert(sizeof(crypto::hash) == sizeof(crypto::ec_point), "expected sizeof hash == sizeof ec_point");
    if (b.major_version >= HF_VERSION_FCMP_PLUS_PLUS)
      hashes.push_back((crypto::hash&)b.fcmp_pp_tree_root);
    // 3. Miner tx
    crypto::hash h = null_hash;
    size_t bl_sz = 0;
    CHECK_AND_ASSERT_THROW_MES(get_transaction_hash(b.miner_tx, h, bl_sz), "Failed to calculate transaction hash");
    hashes.push_back(h);
    // 4. All other txs
    for(auto& th: b.tx_hashes)
      hashes.push_back(th);
    return get_tree_hash(hashes);
  }
  //---------------------------------------------------------------
  bool is_valid_decomposed_amount(uint64_t amount)
  {
    if (0 == amount)
      return false;

    // divide out all trailing zeros (in base 10)
    while (amount % 10 == 0)
      amount /= 10;

    return amount < 10; // are we left with 1 leading digit?
  }
  //---------------------------------------------------------------
  void get_hash_stats(uint64_t &tx_hashes_calculated, uint64_t &tx_hashes_cached, uint64_t &block_hashes_calculated, uint64_t & block_hashes_cached)
  {
    tx_hashes_calculated = tx_hashes_calculated_count;
    tx_hashes_cached = tx_hashes_cached_count;
    block_hashes_calculated = block_hashes_calculated_count;
    block_hashes_cached = block_hashes_cached_count;
  }
  //---------------------------------------------------------------
  crypto::secret_key encrypt_key(crypto::secret_key key, const epee::wipeable_string &passphrase)
  {
    crypto::hash hash;
    crypto::cn_slow_hash(passphrase.data(), passphrase.size(), hash);
    sc_add((unsigned char*)key.data, (const unsigned char*)key.data, (const unsigned char*)hash.data);
    return key;
  }
  //---------------------------------------------------------------
  crypto::secret_key decrypt_key(crypto::secret_key key, const epee::wipeable_string &passphrase)
  {
    crypto::hash hash;
    crypto::cn_slow_hash(passphrase.data(), passphrase.size(), hash);
    sc_sub((unsigned char*)key.data, (const unsigned char*)key.data, (const unsigned char*)hash.data);
    return key;
  }
  //---------------------------------------------------------------
  uint64_t get_default_last_locked_block_index(const uint64_t block_included_in_chain)
  {
    static_assert(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > 0, "unexpected default spendable age");
    return block_included_in_chain + (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1);
  }
  //---------------------------------------------------------------
  // TODO: write tests for this func that match with current daemon logic
  uint64_t get_last_locked_block_index(uint64_t unlock_time, uint64_t block_included_in_chain)
  {
    uint64_t last_locked_block_index = 0;

    const uint64_t default_block_index = get_default_last_locked_block_index(block_included_in_chain);

    if (unlock_time == 0)
    {
      last_locked_block_index = default_block_index;
    }
    else if (unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
    {
      // The unlock_time in this case is supposed to be the chain height at which the output unlocks
      // The chain height is 1 higher than the highest block index, so we subtract 1 for this delta
      last_locked_block_index = unlock_time > 0 ? (unlock_time - 1) : 0;
    }
    else
    {
      // Interpret the unlock_time as time
      // TODO: hardcode correct times for each network and take in nettype
      const auto hf_v15_time = 1656629118;
      const auto hf_v15_height = 2689608;

      // Use the last hard fork's time and block combo to convert the time-based timelock into an last locked block
      // TODO: consider taking into account 60s block times when that was consensus
      if (hf_v15_time > unlock_time)
      {
        const auto seconds_since_unlock = hf_v15_time - unlock_time;
        const auto blocks_since_unlock = seconds_since_unlock / DIFFICULTY_TARGET_V2;

        last_locked_block_index = hf_v15_height > blocks_since_unlock
          ? (hf_v15_height - blocks_since_unlock)
          : default_block_index;
      }
      else
      {
        const auto seconds_until_unlock = unlock_time - hf_v15_time;
        const auto blocks_until_unlock = seconds_until_unlock / DIFFICULTY_TARGET_V2;
        last_locked_block_index = hf_v15_height + blocks_until_unlock;
      }

      /* Note: since this function was introduced for the hf that included fcmp's, it's possible for an output to be
          spent before it reaches the last_locked_block_index going by the old rules; this is ok. It can't be spent again b/c
          it'll have a duplicate key image. It's also possible for an output to unlock by old rules, and then re-lock
          again at the fork. This is also ok, we just need to be sure that the new hf rules use this last_locked_block_index
          starting at the fork for fcmp's.
      */

      // TODO: double check the accuracy of this calculation
      MDEBUG("unlock time: " << unlock_time << " , last_locked_block_index: " << last_locked_block_index);
    }

    // Can't unlock earlier than the default last locked block
    return std::max(last_locked_block_index, default_block_index);
  }
  //---------------------------------------------------------------
  bool is_custom_timelocked(bool is_coinbase, uint64_t last_locked_block_idx, uint64_t block_included_in_chain)
  {
    if (is_coinbase)
      return false;

    return last_locked_block_idx > cryptonote::get_default_last_locked_block_index(block_included_in_chain);
  }
  //---------------------------------------------------------------
  OutsByLastLockedBlockMeta get_outs_by_last_locked_block(
    const std::vector<std::reference_wrapper<const cryptonote::transaction>> &txs,
    const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments,
    const uint64_t first_output_id,
    const uint64_t block_idx)
  {
    OutsByLastLockedBlockMeta outs_by_last_locked_block_meta_out;
    outs_by_last_locked_block_meta_out.next_output_id = first_output_id;

    for (const auto &tx : txs)
    {
      outs_by_last_locked_block_meta_out.next_output_id += set_tx_outs_by_last_locked_block(
        tx.get(),
        transparent_amount_commitments,
        outs_by_last_locked_block_meta_out.next_output_id,
        block_idx,
        outs_by_last_locked_block_meta_out.outs_by_last_locked_block,
        outs_by_last_locked_block_meta_out.timelocked_outputs);
    }

    return outs_by_last_locked_block_meta_out;
  }
}
