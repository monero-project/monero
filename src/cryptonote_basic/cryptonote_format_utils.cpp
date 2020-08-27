// Copyright (c) 2014-2020, The Monero Project
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
#include "serialization/string.h"
#include "cryptonote_format_utils.h"
#include "cryptonote_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "ringct/rctSigs.h"

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

// #define ENABLE_HASH_CASH_INTEGRITY_CHECK

using namespace crypto;

static const uint64_t valid_decomposed_outputs[] = {
  (uint64_t)1, (uint64_t)2, (uint64_t)3, (uint64_t)4, (uint64_t)5, (uint64_t)6, (uint64_t)7, (uint64_t)8, (uint64_t)9, // 1 piconero
  (uint64_t)10, (uint64_t)20, (uint64_t)30, (uint64_t)40, (uint64_t)50, (uint64_t)60, (uint64_t)70, (uint64_t)80, (uint64_t)90,
  (uint64_t)100, (uint64_t)200, (uint64_t)300, (uint64_t)400, (uint64_t)500, (uint64_t)600, (uint64_t)700, (uint64_t)800, (uint64_t)900,
  (uint64_t)1000, (uint64_t)2000, (uint64_t)3000, (uint64_t)4000, (uint64_t)5000, (uint64_t)6000, (uint64_t)7000, (uint64_t)8000, (uint64_t)9000,
  (uint64_t)10000, (uint64_t)20000, (uint64_t)30000, (uint64_t)40000, (uint64_t)50000, (uint64_t)60000, (uint64_t)70000, (uint64_t)80000, (uint64_t)90000,
  (uint64_t)100000, (uint64_t)200000, (uint64_t)300000, (uint64_t)400000, (uint64_t)500000, (uint64_t)600000, (uint64_t)700000, (uint64_t)800000, (uint64_t)900000,
  (uint64_t)1000000, (uint64_t)2000000, (uint64_t)3000000, (uint64_t)4000000, (uint64_t)5000000, (uint64_t)6000000, (uint64_t)7000000, (uint64_t)8000000, (uint64_t)9000000, // 1 micronero
  (uint64_t)10000000, (uint64_t)20000000, (uint64_t)30000000, (uint64_t)40000000, (uint64_t)50000000, (uint64_t)60000000, (uint64_t)70000000, (uint64_t)80000000, (uint64_t)90000000,
  (uint64_t)100000000, (uint64_t)200000000, (uint64_t)300000000, (uint64_t)400000000, (uint64_t)500000000, (uint64_t)600000000, (uint64_t)700000000, (uint64_t)800000000, (uint64_t)900000000,
  (uint64_t)1000000000, (uint64_t)2000000000, (uint64_t)3000000000, (uint64_t)4000000000, (uint64_t)5000000000, (uint64_t)6000000000, (uint64_t)7000000000, (uint64_t)8000000000, (uint64_t)9000000000,
  (uint64_t)10000000000, (uint64_t)20000000000, (uint64_t)30000000000, (uint64_t)40000000000, (uint64_t)50000000000, (uint64_t)60000000000, (uint64_t)70000000000, (uint64_t)80000000000, (uint64_t)90000000000,
  (uint64_t)100000000000, (uint64_t)200000000000, (uint64_t)300000000000, (uint64_t)400000000000, (uint64_t)500000000000, (uint64_t)600000000000, (uint64_t)700000000000, (uint64_t)800000000000, (uint64_t)900000000000,
  (uint64_t)1000000000000, (uint64_t)2000000000000, (uint64_t)3000000000000, (uint64_t)4000000000000, (uint64_t)5000000000000, (uint64_t)6000000000000, (uint64_t)7000000000000, (uint64_t)8000000000000, (uint64_t)9000000000000, // 1 monero
  (uint64_t)10000000000000, (uint64_t)20000000000000, (uint64_t)30000000000000, (uint64_t)40000000000000, (uint64_t)50000000000000, (uint64_t)60000000000000, (uint64_t)70000000000000, (uint64_t)80000000000000, (uint64_t)90000000000000,
  (uint64_t)100000000000000, (uint64_t)200000000000000, (uint64_t)300000000000000, (uint64_t)400000000000000, (uint64_t)500000000000000, (uint64_t)600000000000000, (uint64_t)700000000000000, (uint64_t)800000000000000, (uint64_t)900000000000000,
  (uint64_t)1000000000000000, (uint64_t)2000000000000000, (uint64_t)3000000000000000, (uint64_t)4000000000000000, (uint64_t)5000000000000000, (uint64_t)6000000000000000, (uint64_t)7000000000000000, (uint64_t)8000000000000000, (uint64_t)9000000000000000,
  (uint64_t)10000000000000000, (uint64_t)20000000000000000, (uint64_t)30000000000000000, (uint64_t)40000000000000000, (uint64_t)50000000000000000, (uint64_t)60000000000000000, (uint64_t)70000000000000000, (uint64_t)80000000000000000, (uint64_t)90000000000000000,
  (uint64_t)100000000000000000, (uint64_t)200000000000000000, (uint64_t)300000000000000000, (uint64_t)400000000000000000, (uint64_t)500000000000000000, (uint64_t)600000000000000000, (uint64_t)700000000000000000, (uint64_t)800000000000000000, (uint64_t)900000000000000000,
  (uint64_t)1000000000000000000, (uint64_t)2000000000000000000, (uint64_t)3000000000000000000, (uint64_t)4000000000000000000, (uint64_t)5000000000000000000, (uint64_t)6000000000000000000, (uint64_t)7000000000000000000, (uint64_t)8000000000000000000, (uint64_t)9000000000000000000, // 1 meganero
  (uint64_t)10000000000000000000ull
};

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

  uint64_t get_transaction_weight_clawback(const transaction &tx, size_t n_padded_outputs)
  {
    const rct::rctSig &rv = tx.rct_signatures;
    const uint64_t bp_base = 368;
    const size_t n_outputs = tx.vout.size();
    if (n_padded_outputs <= 2)
      return 0;
    size_t nlr = 0;
    while ((1u << nlr) < n_padded_outputs)
      ++nlr;
    nlr += 6;
    const size_t bp_size = 32 * (9 + 2 * nlr);
    CHECK_AND_ASSERT_THROW_MES_L1(n_outputs <= BULLETPROOF_MAX_OUTPUTS, "maximum number of outputs is " + std::to_string(BULLETPROOF_MAX_OUTPUTS) + " per transaction");
    CHECK_AND_ASSERT_THROW_MES_L1(bp_base * n_padded_outputs >= bp_size, "Invalid bulletproof clawback: bp_base " + std::to_string(bp_base) + ", n_padded_outputs "
        + std::to_string(n_padded_outputs) + ", bp_size " + std::to_string(bp_size));
    const uint64_t bp_clawback = (bp_base * n_padded_outputs - bp_size) * 4 / 5;
    return bp_clawback;
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
  
  //---------------------------------------------------------------  
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h)
  {
    std::ostringstream s;
    binary_archive<true> a(s);
    ::serialization::serialize(a, const_cast<transaction_prefix&>(tx));
    crypto::cn_fast_hash(s.str().data(), s.str().size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx)
  {
    crypto::hash h = null_hash;
    get_transaction_prefix_hash(tx, h);
    return h;
  }
  //---------------------------------------------------------------
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
        if (tx.vout[n].target.type() != typeid(txout_to_key))
        {
          LOG_PRINT_L1("Unsupported output type in tx " << get_transaction_hash(tx));
          return false;
        }
        rv.outPk[n].dest = rct::pk2rct(boost::get<txout_to_key>(tx.vout[n].target).key);
      }

      if (!base_only)
      {
        const bool bulletproof = rct::is_rct_bulletproof(rv.type);
        if (bulletproof)
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
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
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
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = tx.serialize_base(ba);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    CHECK_AND_ASSERT_MES(expand_transaction_1(tx, true), false, "Failed to expand transaction data");
    tx.invalidate_hashes();
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_prefix_from_blob(const blobdata_ref& tx_blob, transaction_prefix& tx)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize_noeof(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction prefix from blob");
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx, crypto::hash& tx_hash)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    CHECK_AND_ASSERT_MES(expand_transaction_1(tx, false), false, "Failed to expand transaction data");
    tx.invalidate_hashes();
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
      MWARNING("key image helper: failed to generate_key_derivation(" << tx_public_key << ", " << ack.m_view_secret_key << ")");
      memcpy(&recv_derivation, rct::identity().bytes, sizeof(recv_derivation));
    }

    std::vector<crypto::key_derivation> additional_recv_derivations;
    for (size_t i = 0; i < additional_tx_public_keys.size(); ++i)
    {
      crypto::key_derivation additional_recv_derivation = AUTO_VAL_INIT(additional_recv_derivation);
      r = hwdev.generate_key_derivation(additional_tx_public_keys[i], ack.m_view_secret_key, additional_recv_derivation);
      if (!r)
      {
        MWARNING("key image helper: failed to generate_key_derivation(" << additional_tx_public_keys[i] << ", " << ack.m_view_secret_key << ")");
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
      hwdev.derive_secret_key(recv_derivation, real_output_index, ack.m_spend_secret_key, scalar_step1); // computes Hs(a*R || idx) + b

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
  uint64_t get_transaction_weight(const transaction &tx, size_t blob_size)
  {
    CHECK_AND_ASSERT_MES(!tx.pruned, std::numeric_limits<uint64_t>::max(), "get_transaction_weight does not support pruned txes");
    if (tx.version < 2)
      return blob_size;
    const rct::rctSig &rv = tx.rct_signatures;
    if (!rct::is_rct_bulletproof(rv.type))
      return blob_size;
    const size_t n_padded_outputs = rct::n_bulletproof_max_amounts(rv.p.bulletproofs);
    uint64_t bp_clawback = get_transaction_weight_clawback(tx, n_padded_outputs);
    CHECK_AND_ASSERT_THROW_MES_L1(bp_clawback <= std::numeric_limits<uint64_t>::max() - blob_size, "Weight overflow");
    return blob_size + bp_clawback;
  }
  //---------------------------------------------------------------
  uint64_t get_pruned_transaction_weight(const transaction &tx)
  {
    CHECK_AND_ASSERT_MES(tx.pruned, std::numeric_limits<uint64_t>::max(), "get_pruned_transaction_weight does not support non pruned txes");
    CHECK_AND_ASSERT_MES(tx.version >= 2, std::numeric_limits<uint64_t>::max(), "get_pruned_transaction_weight does not support v1 txes");
    CHECK_AND_ASSERT_MES(tx.rct_signatures.type >= rct::RCTTypeBulletproof2 || tx.rct_signatures.type == rct::RCTTypeCLSAG,
        std::numeric_limits<uint64_t>::max(), "get_pruned_transaction_weight does not support older range proof types");
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
    extra = 32 * (9 + 2 * nrl) + 2;
    weight += extra;

    // calculate deterministic CLSAG/MLSAG data size
    const size_t ring_size = boost::get<cryptonote::txin_to_key>(tx.vin[0]).key_offsets.size();
    if (tx.rct_signatures.type == rct::RCTTypeCLSAG)
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

    std::string extra_str(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size());
    std::istringstream iss(extra_str);
    binary_archive<false> ar(iss);

    bool eof = false;
    while (!eof)
    {
      tx_extra_field field;
      bool r = ::do_serialize(ar, field);
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
      tx_extra_fields.push_back(field);

      std::ios_base::iostate state = iss.rdstate();
      eof = (EOF == iss.peek());
      iss.clear(state);
    }
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

    std::string extra_str(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size());
    std::istringstream iss(extra_str);
    binary_archive<false> ar(iss);

    bool eof = false;
    size_t processed = 0;
    while (!eof)
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
      processed = iss.tellg();

      std::ios_base::iostate state = iss.rdstate();
      eof = (EOF == iss.peek());
      iss.clear(state);
    }
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
  bool remove_field_from_tx_extra(std::vector<uint8_t>& tx_extra, const std::type_info &type)
  {
    if (tx_extra.empty())
      return true;
    std::string extra_str(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size());
    std::istringstream iss(extra_str);
    binary_archive<false> ar(iss);
    std::ostringstream oss;
    binary_archive<true> newar(oss);

    bool eof = false;
    while (!eof)
    {
      tx_extra_field field;
      bool r = ::do_serialize(ar, field);
      CHECK_AND_NO_ASSERT_MES_L1(r, false, "failed to deserialize extra field. extra = " << string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(tx_extra.data()), tx_extra.size())));
      if (field.type() != type)
        ::do_serialize(newar, field);

      std::ios_base::iostate state = iss.rdstate();
      eof = (EOF == iss.peek());
      iss.clear(state);
    }
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
      CHECK_AND_ASSERT_MES(out.target.type() == typeid(txout_to_key), false, "wrong variant type: "
        << out.target.type().name() << ", expected " << typeid(txout_to_key).name()
        << ", in transaction id=" << get_transaction_hash(tx));

      if (tx.version == 1)
      {
        CHECK_AND_NO_ASSERT_MES(0 < out.amount, false, "zero amount output in transaction id=" << get_transaction_hash(tx));
      }

      if(!check_key(boost::get<txout_to_key>(out.target).key))
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
  std::string short_hash_str(const crypto::hash& h)
  {
    std::string res = string_tools::pod_to_hex(h);
    CHECK_AND_ASSERT_MES(res.size() == 64, res, "wrong hash256 with string_tools::pod_to_hex conversion");
    auto erased_pos = res.erase(8, 48);
    res.insert(8, "....");
    return res;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_keys& acc, const txout_to_key& out_key, const crypto::public_key& tx_pub_key, const std::vector<crypto::public_key>& additional_tx_pub_keys, size_t output_index)
  {
    crypto::key_derivation derivation;
    bool r = acc.get_device().generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
    crypto::public_key pk;
    r = acc.get_device().derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
    CHECK_AND_ASSERT_MES(r, false, "Failed to derive public key");
    if (pk == out_key.key)
      return true;
    // try additional tx pubkeys if available
    if (!additional_tx_pub_keys.empty())
    {
      CHECK_AND_ASSERT_MES(output_index < additional_tx_pub_keys.size(), false, "wrong number of additional tx pubkeys");
      r = acc.get_device().generate_key_derivation(additional_tx_pub_keys[output_index], acc.m_view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      r = acc.get_device().derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
      CHECK_AND_ASSERT_MES(r, false, "Failed to derive public key");
      return pk == out_key.key;
    }
    return false;
  }
  //---------------------------------------------------------------
  boost::optional<subaddress_receive_info> is_out_to_acc_precomp(const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, const crypto::public_key& out_key, const crypto::key_derivation& derivation, const std::vector<crypto::key_derivation>& additional_derivations, size_t output_index, hw::device &hwdev)
  {
    // try the shared tx pubkey
    crypto::public_key subaddress_spendkey;
    hwdev.derive_subaddress_public_key(out_key, derivation, output_index, subaddress_spendkey);
    auto found = subaddresses.find(subaddress_spendkey);
    if (found != subaddresses.end())
      return subaddress_receive_info{ found->second, derivation };
    // try additional tx pubkeys if available
    if (!additional_derivations.empty())
    {
      CHECK_AND_ASSERT_MES(output_index < additional_derivations.size(), boost::none, "wrong number of additional derivations");
      hwdev.derive_subaddress_public_key(out_key, additional_derivations[output_index], output_index, subaddress_spendkey);
      found = subaddresses.find(subaddress_spendkey);
      if (found != subaddresses.end())
        return subaddress_receive_info{ found->second, additional_derivations[output_index] };
    }
    return boost::none;
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
      CHECK_AND_ASSERT_MES(o.target.type() ==  typeid(txout_to_key), false, "wrong type id in transaction out" );
      if(is_out_to_acc(acc, boost::get<txout_to_key>(o.target), tx_pub_key, additional_tx_pub_keys, i))
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
      const size_t mixin = t.vin.empty() ? 0 : t.vin[0].type() == typeid(txin_to_key) ? boost::get<txin_to_key>(t.vin[0]).key_offsets.size() - 1 : 0;
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
    crypto::hash tree_root_hash = get_tx_tree_hash(b);
    blob.append(reinterpret_cast<const char*>(&tree_root_hash), sizeof(tree_root_hash));
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
    std::stringstream ss;
    ss << b_blob;
    binary_archive<false> ba(ss);
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
  void get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h)
  {
    tree_hash(tx_hashes.data(), tx_hashes.size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes)
  {
    crypto::hash h = null_hash;
    get_tx_tree_hash(tx_hashes, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const block& b)
  {
    std::vector<crypto::hash> txs_ids;
    txs_ids.reserve(1 + b.tx_hashes.size());
    crypto::hash h = null_hash;
    size_t bl_sz = 0;
    CHECK_AND_ASSERT_THROW_MES(get_transaction_hash(b.miner_tx, h, bl_sz), "Failed to calculate transaction hash");
    txs_ids.push_back(h);
    for(auto& th: b.tx_hashes)
      txs_ids.push_back(th);
    return get_tx_tree_hash(txs_ids);
  }
  //---------------------------------------------------------------
  bool is_valid_decomposed_amount(uint64_t amount)
  {
    const uint64_t *begin = valid_decomposed_outputs;
    const uint64_t *end = valid_decomposed_outputs + sizeof(valid_decomposed_outputs) / sizeof(valid_decomposed_outputs[0]);
    return std::binary_search(begin, end, amount);
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
}
