// Copyright (c) 2014-2016, The Monero Project
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

#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_format_utils.h"
#include <boost/foreach.hpp>
#include "cryptonote_config.h"
#include "miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "ringct/rctSigs.h"

#define ENCRYPTED_PAYMENT_ID_TAIL 0x8d

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

namespace cryptonote
{
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
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    //TODO: validate tx

    get_transaction_hash(tx, tx_hash);
    get_transaction_prefix_hash(tx, tx_prefix_hash);
    return true;
  }
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, size_t current_block_size, uint64_t fee, const account_public_address &miner_address, transaction& tx, const blobdata& extra_nonce, size_t max_outs, uint8_t hard_fork_version) {
    tx.vin.clear();
    tx.vout.clear();
    tx.extra.clear();

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if(!extra_nonce.empty())
      if(!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
        return false;

    txin_gen in;
    in.height = height;

    uint64_t block_reward;
    if(!get_block_reward(median_size, current_block_size, already_generated_coins, block_reward, hard_fork_version))
    {
      LOG_PRINT_L0("Block is too big");
      return false;
    }

#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
    LOG_PRINT_L1("Creating block template: reward " << block_reward <<
      ", fee " << fee);
#endif
    block_reward += fee;

    // from hard fork 2, we cut out the low significant digits. This makes the tx smaller, and
    // keeps the paid amount almost the same. The unpaid remainder gets pushed back to the
    // emission schedule
    // from hard fork 4, we use a single "dusty" output. This makes the tx even smaller,
    // and avoids the quantization. These outputs will be added as rct outputs with identity
    // masks, to they can be used as rct inputs.
    if (hard_fork_version >= 2 && hard_fork_version < 4) {
      block_reward = block_reward - block_reward % ::config::BASE_REWARD_CLAMP_THRESHOLD;
    }

    std::vector<uint64_t> out_amounts;
    decompose_amount_into_digits(block_reward, hard_fork_version >= 2 ? 0 : ::config::DEFAULT_DUST_THRESHOLD,
      [&out_amounts](uint64_t a_chunk) { out_amounts.push_back(a_chunk); },
      [&out_amounts](uint64_t a_dust) { out_amounts.push_back(a_dust); });

    CHECK_AND_ASSERT_MES(1 <= max_outs, false, "max_out must be non-zero");
    if (height == 0 || hard_fork_version >= 4)
    {
      // the genesis block was not decomposed, for unknown reasons
      while (max_outs < out_amounts.size())
      {
        //out_amounts[out_amounts.size() - 2] += out_amounts.back();
        //out_amounts.resize(out_amounts.size() - 1);
        out_amounts[1] += out_amounts[0];
        for (size_t n = 1; n < out_amounts.size(); ++n)
          out_amounts[n - 1] = out_amounts[n];
        out_amounts.resize(out_amounts.size() - 1);
      }
    }
    else
    {
      CHECK_AND_ASSERT_MES(max_outs >= out_amounts.size(), false, "max_out exceeded");
    }

    uint64_t summary_amounts = 0;
    for (size_t no = 0; no < out_amounts.size(); no++)
    {
      crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);;
      crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
      bool r = crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
      CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << miner_address.m_view_public_key << ", " << txkey.sec << ")");

      r = crypto::derive_public_key(derivation, no, miner_address.m_spend_public_key, out_eph_public_key);
      CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << no << ", "<< miner_address.m_spend_public_key << ")");

      txout_to_key tk;
      tk.key = out_eph_public_key;

      tx_out out;
      summary_amounts += out.amount = out_amounts[no];
      out.target = tk;
      tx.vout.push_back(out);
    }

    CHECK_AND_ASSERT_MES(summary_amounts == block_reward, false, "Failed to construct miner tx, summary_amounts = " << summary_amounts << " not equal block_reward = " << block_reward);

    if (hard_fork_version >= 4)
      tx.version = 2;
    else
      tx.version = 1;

    //lock
    tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
    tx.vin.push_back(in);
    //LOG_PRINT("MINER_TX generated ok, block_reward=" << print_money(block_reward) << "("  << print_money(block_reward - fee) << "+" << print_money(fee)
    //  << "), current_block_size=" << current_block_size << ", already_generated_coins=" << already_generated_coins << ", tx_id=" << get_transaction_hash(tx), LOG_LEVEL_2);
    return true;
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki)
  {
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    bool r = crypto::generate_key_derivation(tx_public_key, ack.m_view_secret_key, recv_derivation);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to generate_key_derivation(" << tx_public_key << ", " << ack.m_view_secret_key << ")");

    r = crypto::derive_public_key(recv_derivation, real_output_index, ack.m_account_address.m_spend_public_key, in_ephemeral.pub);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to derive_public_key(" << recv_derivation << ", " << real_output_index <<  ", " << ack.m_account_address.m_spend_public_key << ")");

    crypto::derive_secret_key(recv_derivation, real_output_index, ack.m_spend_secret_key, in_ephemeral.sec);

    crypto::generate_key_image(in_ephemeral.pub, in_ephemeral.sec, ki);
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
      while (CRYPTONOTE_DISPLAY_DECIMAL_POINT < fraction_size && '0' == str_amount.back())
      {
        str_amount.erase(str_amount.size() - 1, 1);
        --fraction_size;
      }
      if (CRYPTONOTE_DISPLAY_DECIMAL_POINT < fraction_size)
        return false;
      str_amount.erase(point_index, 1);
    }
    else
    {
      fraction_size = 0;
    }

    if (str_amount.empty())
      return false;

    if (fraction_size < CRYPTONOTE_DISPLAY_DECIMAL_POINT)
    {
      str_amount.append(CRYPTONOTE_DISPLAY_DECIMAL_POINT - fraction_size, '0');
    }

    return string_tools::get_xtype_from_string(amount, str_amount);
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
    BOOST_FOREACH(auto& in, tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key), 0, "unexpected type id in transaction");
      amount_in += boost::get<txin_to_key>(in).amount;
    }
    BOOST_FOREACH(auto& o, tx.vout)
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
  crypto::public_key get_tx_pub_key_from_extra(const std::vector<uint8_t>& tx_extra)
  {
    std::vector<tx_extra_field> tx_extra_fields;
    parse_tx_extra(tx_extra, tx_extra_fields);

    tx_extra_pub_key pub_key_field;
    if(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field))
      return null_pkey;

    return pub_key_field.pub_key;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction_prefix& tx_prefix)
  {
    return get_tx_pub_key_from_extra(tx_prefix.extra);
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx)
  {
    return get_tx_pub_key_from_extra(tx.extra);
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    tx.extra.resize(tx.extra.size() + 1 + sizeof(crypto::public_key));
    tx.extra[tx.extra.size() - 1 - sizeof(crypto::public_key)] = TX_EXTRA_TAG_PUBKEY;
    *reinterpret_cast<crypto::public_key*>(&tx.extra[tx.extra.size() - sizeof(crypto::public_key)]) = tx_pub_key;
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
  crypto::public_key get_destination_view_key_pub(const std::vector<tx_destination_entry> &destinations, const account_keys &sender_keys)
  {
    if (destinations.empty())
      return null_pkey;
    for (size_t n = 1; n < destinations.size(); ++n)
    {
      if (!memcmp(&destinations[n].addr, &sender_keys.m_account_address, sizeof(destinations[0].addr)))
        continue;
      if (memcmp(&destinations[n].addr, &destinations[0].addr, sizeof(destinations[0].addr)))
        return null_pkey;
    }
    return destinations[0].addr.m_view_public_key;
  }
  //---------------------------------------------------------------
  bool encrypt_payment_id(crypto::hash8 &payment_id, const crypto::public_key &public_key, const crypto::secret_key &secret_key)
  {
    crypto::key_derivation derivation;
    crypto::hash hash;
    char data[33]; /* A hash, and an extra byte */

    if (!generate_key_derivation(public_key, secret_key, derivation))
      return false;

    memcpy(data, &derivation, 32);
    data[32] = ENCRYPTED_PAYMENT_ID_TAIL;
    cn_fast_hash(data, 33, hash);

    for (size_t b = 0; b < 8; ++b)
      payment_id.data[b] ^= hash.data[b];

    return true;
  }
  bool decrypt_payment_id(crypto::hash8 &payment_id, const crypto::public_key &public_key, const crypto::secret_key &secret_key)
  {
    // Encryption and decryption are the same operation (xor with a key)
    return encrypt_payment_id(payment_id, public_key, secret_key);
  }
  //---------------------------------------------------------------
  bool construct_tx_and_get_tx_key(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, const std::vector<tx_destination_entry>& destinations, std::vector<uint8_t> extra, transaction& tx, uint64_t unlock_time, crypto::secret_key &tx_key, bool rct)
  {
    std::vector<rct::key> amount_keys;
    tx.set_null();
    amount_keys.clear();

    tx.version = rct ? 2 : 1;
    tx.unlock_time = unlock_time;

    tx.extra = extra;
    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    tx_key = txkey.sec;

    // if we have a stealth payment id, find it and encrypt it with the tx key now
    std::vector<tx_extra_field> tx_extra_fields;
    if (parse_tx_extra(tx.extra, tx_extra_fields))
    {
      tx_extra_nonce extra_nonce;
      if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
      {
        crypto::hash8 payment_id = null_hash8;
        if (get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
          LOG_PRINT_L2("Encrypting payment id " << payment_id);
          crypto::public_key view_key_pub = get_destination_view_key_pub(destinations, sender_account_keys);
          if (view_key_pub == null_pkey)
          {
            LOG_ERROR("Destinations have to have exactly one output to support encrypted payment ids");
            return false;
          }

          if (!encrypt_payment_id(payment_id, view_key_pub, txkey.sec))
          {
            LOG_ERROR("Failed to encrypt payment id");
            return false;
          }

          std::string extra_nonce;
          set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
          remove_field_from_tx_extra(tx.extra, typeid(tx_extra_fields));
          if (!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
          {
            LOG_ERROR("Failed to add encrypted payment id to tx extra");
            return false;
          }
          LOG_PRINT_L1("Encrypted payment ID: " << payment_id);
        }
      }
    }
    else
    {
      LOG_ERROR("Failed to parse tx extra");
      return false;
    }

    struct input_generation_context_data
    {
      keypair in_ephemeral;
    };
    std::vector<input_generation_context_data> in_contexts;

    uint64_t summary_inputs_money = 0;
    //fill inputs
    BOOST_FOREACH(const tx_source_entry& src_entr,  sources)
    {
      if(src_entr.real_output >= src_entr.outputs.size())
      {
        LOG_ERROR("real_output index (" << src_entr.real_output << ")bigger than output_keys.size()=" << src_entr.outputs.size());
        return false;
      }
      summary_inputs_money += src_entr.amount;

      //key_derivation recv_derivation;
      in_contexts.push_back(input_generation_context_data());
      keypair& in_ephemeral = in_contexts.back().in_ephemeral;
      crypto::key_image img;
      if(!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img))
        return false;

      //check that derivated key is equal with real output key
      if( !(in_ephemeral.pub == src_entr.outputs[src_entr.real_output].second.dest) )
      {
        LOG_ERROR("derived public key missmatch with output public key! "<< ENDL << "derived_key:"
          << string_tools::pod_to_hex(in_ephemeral.pub) << ENDL << "real output_public_key:"
          << string_tools::pod_to_hex(src_entr.outputs[src_entr.real_output].second) );
        return false;
      }

      //put key image into tx input
      txin_to_key input_to_key;
      input_to_key.amount = src_entr.amount;
      input_to_key.k_image = img;

      //fill outputs array and use relative offsets
      BOOST_FOREACH(const tx_source_entry::output_entry& out_entry, src_entr.outputs)
        input_to_key.key_offsets.push_back(out_entry.first);

      input_to_key.key_offsets = absolute_output_offsets_to_relative(input_to_key.key_offsets);
      tx.vin.push_back(input_to_key);
    }

    // "Shuffle" outs
    std::vector<tx_destination_entry> shuffled_dsts(destinations);
    std::sort(shuffled_dsts.begin(), shuffled_dsts.end(), [](const tx_destination_entry& de1, const tx_destination_entry& de2) { return de1.amount < de2.amount; } );

    uint64_t summary_outs_money = 0;
    //fill outputs
    size_t output_index = 0;
    BOOST_FOREACH(const tx_destination_entry& dst_entr,  shuffled_dsts)
    {
      CHECK_AND_ASSERT_MES(dst_entr.amount > 0 || tx.version > 1, false, "Destination with wrong amount: " << dst_entr.amount);
      crypto::key_derivation derivation;
      crypto::public_key out_eph_public_key;
      bool r = crypto::generate_key_derivation(dst_entr.addr.m_view_public_key, txkey.sec, derivation);
      CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << dst_entr.addr.m_view_public_key << ", " << txkey.sec << ")");

      if (tx.version > 1)
      {
        crypto::secret_key scalar1;
        crypto::derivation_to_scalar(derivation, output_index, scalar1);
        amount_keys.push_back(rct::sk2rct(scalar1));
      }
      r = crypto::derive_public_key(derivation, output_index, dst_entr.addr.m_spend_public_key, out_eph_public_key);
      CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to derive_public_key(" << derivation << ", " << output_index << ", "<< dst_entr.addr.m_spend_public_key << ")");

      tx_out out;
      out.amount = dst_entr.amount;
      txout_to_key tk;
      tk.key = out_eph_public_key;
      out.target = tk;
      tx.vout.push_back(out);
      output_index++;
      summary_outs_money += dst_entr.amount;
    }

    //check money
    if(summary_outs_money > summary_inputs_money )
    {
      LOG_ERROR("Transaction inputs money ("<< summary_inputs_money << ") less than outputs money (" << summary_outs_money << ")");
      return false;
    }

    // check for watch only wallet
    bool zero_secret_key = true;
    for (size_t i = 0; i < sizeof(sender_account_keys.m_spend_secret_key); ++i)
      zero_secret_key &= (sender_account_keys.m_spend_secret_key.data[i] == 0);
    if (zero_secret_key)
    {
      LOG_PRINT_L1("Null secret key, skipping signatures");
    }

    if (tx.version == 1)
    {
      //generate ring signatures
      crypto::hash tx_prefix_hash;
      get_transaction_prefix_hash(tx, tx_prefix_hash);

      std::stringstream ss_ring_s;
      size_t i = 0;
      BOOST_FOREACH(const tx_source_entry& src_entr,  sources)
      {
        ss_ring_s << "pub_keys:" << ENDL;
        std::vector<const crypto::public_key*> keys_ptrs;
        std::vector<crypto::public_key> keys(src_entr.outputs.size());
        size_t ii = 0;
        BOOST_FOREACH(const tx_source_entry::output_entry& o, src_entr.outputs)
        {
          keys[ii] = rct2pk(o.second.dest);
          keys_ptrs.push_back(&keys[ii]);
          ss_ring_s << o.second.dest << ENDL;
          ++ii;
        }

        tx.signatures.push_back(std::vector<crypto::signature>());
        std::vector<crypto::signature>& sigs = tx.signatures.back();
        sigs.resize(src_entr.outputs.size());
        if (!zero_secret_key)
          crypto::generate_ring_signature(tx_prefix_hash, boost::get<txin_to_key>(tx.vin[i]).k_image, keys_ptrs, in_contexts[i].in_ephemeral.sec, src_entr.real_output, sigs.data());
        ss_ring_s << "signatures:" << ENDL;
        std::for_each(sigs.begin(), sigs.end(), [&](const crypto::signature& s){ss_ring_s << s << ENDL;});
        ss_ring_s << "prefix_hash:" << tx_prefix_hash << ENDL << "in_ephemeral_key: " << in_contexts[i].in_ephemeral.sec << ENDL << "real_output: " << src_entr.real_output;
        i++;
      }

      LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL << ss_ring_s.str() , LOG_LEVEL_3);
    }
    else
    {
      size_t n_total_outs = sources[0].outputs.size(); // only for non-simple rct

      // the non-simple version is slightly smaller, but assumes all real inputs
      // are on the same index, so can only be used if there just one ring.
      bool use_simple_rct = sources.size() > 1;

      if (!use_simple_rct)
      {
        // non simple ringct requires all real inputs to be at the same index for all inputs
        BOOST_FOREACH(const tx_source_entry& src_entr,  sources)
        {
          if(src_entr.real_output != sources.begin()->real_output)
          {
            LOG_ERROR("All inputs must have the same index for non-simple ringct");
            return false;
          }
        }

        // enforce same mixin for all outputs
        for (size_t i = 1; i < sources.size(); ++i) {
          if (n_total_outs != sources[i].outputs.size()) {
            LOG_ERROR("Non-simple ringct transaction has varying mixin");
            return false;
          }
        }
      }

      uint64_t amount_in = 0, amount_out = 0;
      rct::ctkeyV inSk;
      // mixRing indexing is done the other way round for simple
      rct::ctkeyM mixRing(use_simple_rct ? sources.size() : n_total_outs);
      rct::keyV destinations;
      std::vector<uint64_t> inamounts, outamounts;
      std::vector<unsigned int> index;
      for (size_t i = 0; i < sources.size(); ++i)
      {
        rct::ctkey ctkey;
        amount_in += sources[i].amount;
        inamounts.push_back(sources[i].amount);
        index.push_back(sources[i].real_output);
        // inSk: (secret key, mask)
        ctkey.dest = rct::sk2rct(in_contexts[i].in_ephemeral.sec);
        ctkey.mask = sources[i].mask;
        inSk.push_back(ctkey);
        // inPk: (public key, commitment)
        // will be done when filling in mixRing
      }
      for (size_t i = 0; i < tx.vout.size(); ++i)
      {
        destinations.push_back(rct::pk2rct(boost::get<txout_to_key>(tx.vout[i].target).key));
        outamounts.push_back(tx.vout[i].amount);
        amount_out += tx.vout[i].amount;
      }

      if (use_simple_rct)
      {
        // mixRing indexing is done the other way round for simple
        for (size_t i = 0; i < sources.size(); ++i)
        {
          mixRing[i].resize(sources[i].outputs.size());
          for (size_t n = 0; n < sources[i].outputs.size(); ++n)
          {
            mixRing[i][n] = sources[i].outputs[n].second;
          }
        }
      }
      else
      {
        for (size_t i = 0; i < n_total_outs; ++i) // same index assumption
        {
          mixRing[i].resize(sources.size());
          for (size_t n = 0; n < sources.size(); ++n)
          {
            mixRing[i][n] = sources[n].outputs[i].second;
          }
        }
      }

      // fee
      if (!use_simple_rct && amount_in > amount_out)
        outamounts.push_back(amount_in - amount_out);

      // zero out all amounts to mask rct outputs, real amounts are now encrypted
      for (size_t i = 0; i < tx.vin.size(); ++i)
      {
        if (sources[i].rct)
          boost::get<txin_to_key>(tx.vin[i]).amount = 0;
      }
      for (size_t i = 0; i < tx.vout.size(); ++i)
        tx.vout[i].amount = 0;

      crypto::hash tx_prefix_hash;
      get_transaction_prefix_hash(tx, tx_prefix_hash);
      rct::ctkeyV outSk;
      if (use_simple_rct)
        tx.rct_signatures = rct::genRctSimple(rct::hash2rct(tx_prefix_hash), inSk, destinations, inamounts, outamounts, amount_in - amount_out, mixRing, amount_keys, index, outSk);
      else
        tx.rct_signatures = rct::genRct(rct::hash2rct(tx_prefix_hash), inSk, destinations, outamounts, mixRing, amount_keys, sources[0].real_output, outSk); // same index assumption

      CHECK_AND_ASSERT_MES(tx.vout.size() == outSk.size(), false, "outSk size does not match vout");

      LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL, LOG_LEVEL_3);
    }

    return true;
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, const std::vector<tx_destination_entry>& destinations, std::vector<uint8_t> extra, transaction& tx, uint64_t unlock_time)
  {
     crypto::secret_key tx_key;
     return construct_tx_and_get_tx_key(sender_account_keys, sources, destinations, extra, tx, unlock_time, tx_key);
  }
  //---------------------------------------------------------------
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money)
  {
    money = 0;
    BOOST_FOREACH(const auto& in, tx.vin)
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
    BOOST_FOREACH(const auto& in, tx.vin)
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
    BOOST_FOREACH(const tx_out& out, tx.vout)
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
    BOOST_FOREACH(const auto& in, tx.vin)
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
    BOOST_FOREACH(const auto& o, tx.vout)
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
    BOOST_FOREACH(const auto& o, tx.vout)
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
  bool is_out_to_acc(const account_keys& acc, const txout_to_key& out_key, const crypto::public_key& tx_pub_key, size_t output_index)
  {
    crypto::key_derivation derivation;
    generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);
    crypto::public_key pk;
    derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
    return pk == out_key.key;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
    if(null_pkey == tx_pub_key)
      return false;
    return lookup_acc_outs(acc, tx, tx_pub_key, outs, money_transfered);
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    money_transfered = 0;
    size_t i = 0;
    BOOST_FOREACH(const tx_out& o,  tx.vout)
    {
      CHECK_AND_ASSERT_MES(o.target.type() ==  typeid(txout_to_key), false, "wrong type id in transaction out" );
      if(is_out_to_acc(acc, boost::get<txout_to_key>(o.target), tx_pub_key, i))
      {
        outs.push_back(i);
        money_transfered += o.amount;
      }
      i++;
    }
    return true;
  }
  //---------------------------------------------------------------
  void get_blob_hash(const blobdata& blob, crypto::hash& res)
  {
    cn_fast_hash(blob.data(), blob.size(), res);
  }
  //---------------------------------------------------------------
  std::string print_money(uint64_t amount)
  {
    std::string s = std::to_string(amount);
    if(s.size() < CRYPTONOTE_DISPLAY_DECIMAL_POINT+1)
    {
      s.insert(0, CRYPTONOTE_DISPLAY_DECIMAL_POINT+1 - s.size(), '0');
    }
    s.insert(s.size() - CRYPTONOTE_DISPLAY_DECIMAL_POINT, ".");
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
  crypto::hash get_transaction_hash(const transaction& t)
  {
    crypto::hash h = null_hash;
    get_transaction_hash(t, h, NULL);
    return h;
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res)
  {
    return get_transaction_hash(t, res, NULL);
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t* blob_size)
  {
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

    transaction &tt = const_cast<transaction&>(t);

    // base rct
    {
      std::stringstream ss;
      binary_archive<true> ba(ss);
      const size_t inputs = t.vin.size();
      const size_t outputs = t.vout.size();
      bool r = tt.rct_signatures.serialize_rctsig_base(ba, inputs, outputs);
      CHECK_AND_ASSERT_MES(r, false, "Failed to serialize rct signatures base");
      cryptonote::get_blob_hash(ss.str(), hashes[1]);
    }

    // prunable rct
    if (t.rct_signatures.type == rct::RCTTypeNull)
    {
      hashes[2] = cryptonote::null_hash;
    }
    else
    {
      std::stringstream ss;
      binary_archive<true> ba(ss);
      const size_t inputs = t.vin.size();
      const size_t outputs = t.vout.size();
      const size_t mixin = t.vin.empty() ? 0 : t.vin[0].type() == typeid(txin_to_key) ? boost::get<txin_to_key>(t.vin[0]).key_offsets.size() - 1 : 0;
      bool r = tt.rct_signatures.p.serialize_rctsig_prunable(ba, t.rct_signatures.type, inputs, outputs, mixin);
      CHECK_AND_ASSERT_MES(r, false, "Failed to serialize rct signatures prunable");
      cryptonote::get_blob_hash(ss.str(), hashes[2]);
    }

    // the tx hash is the hash of the 3 hashes
    res = cn_fast_hash(hashes, sizeof(hashes));

    // we still need the size
    if (blob_size)
      *blob_size = get_object_blobsize(t);

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
  bool get_block_hash(const block& b, crypto::hash& res)
  {
    // EXCEPTION FOR BLOCK 202612
    const std::string correct_blob_hash_202612 = "3a8a2b3a29b50fc86ff73dd087ea43c6f0d6b8f936c849194d5c84c737903966";
    const std::string existing_block_id_202612 = "bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698";
    crypto::hash block_blob_hash = get_blob_hash(block_to_blob(b));

    if (string_tools::pod_to_hex(block_blob_hash) == correct_blob_hash_202612)
    {
      string_tools::hex_to_pod(existing_block_id_202612, res);
      return true;
    }
    bool hash_result = get_object_hash(get_block_hashing_blob(b), res);

    if (hash_result)
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
  crypto::hash get_block_hash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_hash(b, p);
    return p;
  }
  //---------------------------------------------------------------
  bool generate_genesis_block(
      block& bl
    , std::string const & genesis_tx
    , uint32_t nonce
    )
  {
    //genesis block
    bl = boost::value_initialized<block>();


    account_public_address ac = boost::value_initialized<account_public_address>();
    std::vector<size_t> sz;
    construct_miner_tx(0, 0, 0, 0, 0, ac, bl.miner_tx); // zero fee in genesis
    blobdata txb = tx_to_blob(bl.miner_tx);
    std::string hex_tx_represent = string_tools::buff_to_hex_nodelimer(txb);

    std::string genesis_coinbase_tx_hex = config::GENESIS_TX;

    blobdata tx_bl;
    string_tools::parse_hexstr_to_binbuff(genesis_coinbase_tx_hex, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = CURRENT_BLOCK_MAJOR_VERSION;
    bl.minor_version = CURRENT_BLOCK_MINOR_VERSION;
    bl.timestamp = 0;
    bl.nonce = nonce;
    miner::find_nonce_for_given_block(bl, 1, 0);
    return true;
  }
  //---------------------------------------------------------------
  bool get_block_longhash(const block& b, crypto::hash& res, uint64_t height)
  {
    // block 202612 bug workaround
    const std::string longhash_202612 = "84f64766475d51837ac9efbef1926486e58563c95a19fef4aec3254f03000000";
    if (height == 202612)
    {
      string_tools::hex_to_pod(longhash_202612, res);
      return true;
    }
    block b_local = b; //workaround to avoid const errors with do_serialize
    blobdata bd = get_block_hashing_blob(b);
    crypto::cn_slow_hash(bd.data(), bd.size(), res);
    return true;
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
  crypto::hash get_block_longhash(const block& b, uint64_t height)
  {
    crypto::hash p = null_hash;
    get_block_longhash(b, p, height);
    return p;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata& b_blob, block& b)
  {
    std::stringstream ss;
    ss << b_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, b);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse block from blob");
    return true;
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
    crypto::hash h = null_hash;
    size_t bl_sz = 0;
    get_transaction_hash(b.miner_tx, h, bl_sz);
    txs_ids.push_back(h);
    BOOST_FOREACH(auto& th, b.tx_hashes)
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
}
