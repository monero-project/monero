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

#pragma once
#include "blobdatatype.h"
#include "cryptonote_basic_impl.h"
#include "tx_extra.h"
#include "account.h"
#include "subaddress_index.h"
#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include <unordered_map>
#include <boost/multiprecision/cpp_int.hpp>

namespace epee
{
  class wipeable_string;
}

namespace cryptonote
{
  //---------------------------------------------------------------
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h, hw::device &hwdev);
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx, hw::device &hwdev);
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h);
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx);
  bool parse_and_validate_tx_prefix_from_blob(const blobdata_ref& tx_blob, transaction_prefix& tx);
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash);
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx, crypto::hash& tx_hash);
  bool parse_and_validate_tx_from_blob(const blobdata_ref& tx_blob, transaction& tx);
  bool parse_and_validate_tx_base_from_blob(const blobdata_ref& tx_blob, transaction& tx);
  bool is_v1_tx(const blobdata_ref& tx_blob);
  bool is_v1_tx(const blobdata& tx_blob);

  template<typename T>
  bool find_tx_extra_field_by_type(const std::vector<tx_extra_field>& tx_extra_fields, T& field, size_t index = 0)
  {
    auto it = std::find_if(tx_extra_fields.begin(), tx_extra_fields.end(), [&index](const tx_extra_field& f) { return typeid(T) == f.type() && !index--; });
    if(tx_extra_fields.end() == it)
      return false;

    field = boost::get<T>(*it);
    return true;
  }

  bool parse_tx_extra(const std::vector<uint8_t>& tx_extra, std::vector<tx_extra_field>& tx_extra_fields);
  bool sort_tx_extra(const std::vector<uint8_t>& tx_extra, std::vector<uint8_t> &sorted_tx_extra, bool allow_partial = false);
  crypto::public_key get_tx_pub_key_from_extra(const std::vector<uint8_t>& tx_extra, size_t pk_index = 0);
  crypto::public_key get_tx_pub_key_from_extra(const transaction_prefix& tx, size_t pk_index = 0);
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx, size_t pk_index = 0);
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key);
  bool add_tx_pub_key_to_extra(transaction_prefix& tx, const crypto::public_key& tx_pub_key);
  bool add_tx_pub_key_to_extra(std::vector<uint8_t>& tx_extra, const crypto::public_key& tx_pub_key);
  std::vector<crypto::public_key> get_additional_tx_pub_keys_from_extra(const std::vector<uint8_t>& tx_extra);
  std::vector<crypto::public_key> get_additional_tx_pub_keys_from_extra(const transaction_prefix& tx);
  bool add_additional_tx_pub_keys_to_extra(std::vector<uint8_t>& tx_extra, const std::vector<crypto::public_key>& additional_pub_keys);
  bool add_extra_nonce_to_tx_extra(std::vector<uint8_t>& tx_extra, const blobdata& extra_nonce);
  bool add_mm_merkle_root_to_tx_extra(std::vector<uint8_t>& tx_extra, const crypto::hash& mm_merkle_root, uint64_t mm_merkle_tree_depth);
  bool remove_field_from_tx_extra(std::vector<uint8_t>& tx_extra, const std::type_info &type);
  void set_payment_id_to_tx_extra_nonce(blobdata& extra_nonce, const crypto::hash& payment_id);
  void set_encrypted_payment_id_to_tx_extra_nonce(blobdata& extra_nonce, const crypto::hash8& payment_id);
  bool get_payment_id_from_tx_extra_nonce(const blobdata& extra_nonce, crypto::hash& payment_id);
  bool get_encrypted_payment_id_from_tx_extra_nonce(const blobdata& extra_nonce, crypto::hash8& payment_id);
  void set_tx_out(const uint64_t amount, const crypto::public_key& output_public_key, const bool use_view_tags, const crypto::view_tag& view_tag, tx_out& out);
  bool check_output_types(const transaction& tx, const uint8_t hf_version);
  bool out_can_be_to_acc(const boost::optional<crypto::view_tag>& view_tag_opt, const crypto::key_derivation& derivation, const size_t output_index, hw::device *hwdev = nullptr);
  bool is_out_to_acc(const account_keys& acc, const crypto::public_key& output_public_key, const crypto::public_key& tx_pub_key, const std::vector<crypto::public_key>& additional_tx_public_keys, size_t output_index, const boost::optional<crypto::view_tag>& view_tag_opt = boost::optional<crypto::view_tag>());
  struct subaddress_receive_info
  {
    subaddress_index index;
    crypto::key_derivation derivation;
  };
  boost::optional<subaddress_receive_info> is_out_to_acc_precomp(const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, const crypto::public_key& out_key, const crypto::key_derivation& derivation, const std::vector<crypto::key_derivation>& additional_derivations, size_t output_index, hw::device &hwdev, const boost::optional<crypto::view_tag>& view_tag_opt = boost::optional<crypto::view_tag>());
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, const std::vector<crypto::public_key>& additional_tx_public_keys, std::vector<size_t>& outs, uint64_t& money_transfered);
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered);
  bool get_tx_fee(const transaction& tx, uint64_t & fee);
  uint64_t get_tx_fee(const transaction& tx);
  bool generate_key_image_helper(const account_keys& ack, const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, const crypto::public_key& out_key, const crypto::public_key& tx_public_key, const std::vector<crypto::public_key>& additional_tx_public_keys, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki, hw::device &hwdev);
  bool generate_key_image_helper_precomp(const account_keys& ack, const crypto::public_key& out_key, const crypto::key_derivation& recv_derivation, size_t real_output_index, const subaddress_index& received_index, keypair& in_ephemeral, crypto::key_image& ki, hw::device &hwdev);
  void get_blob_hash(const blobdata& blob, crypto::hash& res);
  void get_blob_hash(const blobdata_ref& blob, crypto::hash& res);
  crypto::hash get_blob_hash(const blobdata& blob);
  crypto::hash get_blob_hash(const blobdata_ref& blob);
  std::string short_hash_str(const crypto::hash& h);

  crypto::hash get_transaction_hash(const transaction& t);
  bool get_transaction_hash(const transaction& t, crypto::hash& res);
  bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size);
  bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t* blob_size);
  bool calculate_transaction_prunable_hash(const transaction& t, const cryptonote::blobdata_ref *blob, crypto::hash& res);
  crypto::hash get_transaction_prunable_hash(const transaction& t, const cryptonote::blobdata_ref *blob = NULL);
  bool calculate_transaction_hash(const transaction& t, crypto::hash& res, size_t* blob_size);
  crypto::hash get_pruned_transaction_hash(const transaction& t, const crypto::hash &pruned_data_hash);

  blobdata get_block_hashing_blob(const block& b);
  bool calculate_block_hash(const block& b, crypto::hash& res, const blobdata_ref *blob = NULL);
  bool get_block_hash(const block& b, crypto::hash& res);
  crypto::hash get_block_hash(const block& b);
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b, crypto::hash *block_hash);
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b);
  bool parse_and_validate_block_from_blob(const blobdata_ref& b_blob, block& b, crypto::hash &block_hash);
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money);
  uint64_t get_outs_money_amount(const transaction& tx);
  bool get_output_public_key(const cryptonote::tx_out& out, crypto::public_key& output_public_key);
  boost::optional<crypto::view_tag> get_output_view_tag(const cryptonote::tx_out& out);
  bool check_inputs_types_supported(const transaction& tx);
  bool check_outs_valid(const transaction& tx);
  bool parse_amount(uint64_t& amount, const std::string& str_amount);
  uint64_t get_transaction_weight(const transaction &tx);
  uint64_t get_transaction_weight(const transaction &tx, size_t blob_size);
  uint64_t get_pruned_transaction_weight(const transaction &tx);
  uint64_t get_transaction_blob_size(const transaction& tx);

  bool check_money_overflow(const transaction& tx);
  bool check_outs_overflow(const transaction& tx);
  bool check_inputs_overflow(const transaction& tx);
  uint64_t get_block_height(const block& b);
  std::vector<uint64_t> relative_output_offsets_to_absolute(const std::vector<uint64_t>& off);
  std::vector<uint64_t> absolute_output_offsets_to_relative(const std::vector<uint64_t>& off);
  void set_default_decimal_point(unsigned int decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT);
  unsigned int get_default_decimal_point();
  std::string get_unit(unsigned int decimal_point = -1);
  std::string print_money(uint64_t amount, unsigned int decimal_point = -1);
  std::string print_money(const boost::multiprecision::uint128_t &amount, unsigned int decimal_point = -1);
  uint64_t round_money_up(uint64_t amount, unsigned significant_digits);
  std::string round_money_up(const std::string &amount, unsigned significant_digits);
  //---------------------------------------------------------------
  template<class t_object>
  bool t_serializable_object_from_blob(t_object& to, const blobdata& b_blob)
  {
    binary_archive<false> ba{epee::strspan<std::uint8_t>(b_blob)};
    bool r = ::serialization::serialize(ba, to);
    return r;
  }
  //---------------------------------------------------------------
  template<class t_object>
  bool t_serializable_object_to_blob(const t_object& to, blobdata& b_blob)
  {
    std::stringstream ss;
    binary_archive<true> ba(ss);
    bool r = ::serialization::serialize(ba, const_cast<t_object&>(to));
    b_blob = ss.str();
    return r;
  }
  //---------------------------------------------------------------
  template<class t_object>
  blobdata t_serializable_object_to_blob(const t_object& to)
  {
    blobdata b;
    t_serializable_object_to_blob(to, b);
    return b;
  }
  //---------------------------------------------------------------
  template<class t_object>
  bool get_object_hash(const t_object& o, crypto::hash& res)
  {
    get_blob_hash(t_serializable_object_to_blob(o), res);
    return true;
  }
  //---------------------------------------------------------------
  template<class t_object>
  size_t get_object_blobsize(const t_object& o)
  {
    blobdata b = t_serializable_object_to_blob(o);
    return b.size();
  }
  //---------------------------------------------------------------
  template<class t_object>
  bool get_object_hash(const t_object& o, crypto::hash& res, size_t& blob_size)
  {
    blobdata bl = t_serializable_object_to_blob(o);
    blob_size = bl.size();
    get_blob_hash(bl, res);
    return true;
  }
  //---------------------------------------------------------------
  template <typename T>
  std::string obj_to_json_str(T& obj)
  {
    std::stringstream ss;
    json_archive<true> ar(ss, true);
    bool r = ::serialization::serialize(ar, obj);
    CHECK_AND_ASSERT_MES(r, "", "obj_to_json_str failed: serialization::serialize returned false");
    return ss.str();
  }
  //---------------------------------------------------------------
  // 62387455827 -> 455827 + 7000000 + 80000000 + 300000000 + 2000000000 + 60000000000, where 455827 <= dust_threshold
  template<typename chunk_handler_t, typename dust_handler_t>
  void decompose_amount_into_digits(uint64_t amount, uint64_t dust_threshold, const chunk_handler_t& chunk_handler, const dust_handler_t& dust_handler)
  {
    if (0 == amount)
    {
      return;
    }

    bool is_dust_handled = false;
    uint64_t dust = 0;
    uint64_t order = 1;
    while (0 != amount)
    {
      uint64_t chunk = (amount % 10) * order;
      amount /= 10;
      order *= 10;

      if (dust + chunk <= dust_threshold)
      {
        dust += chunk;
      }
      else
      {
        if (!is_dust_handled && 0 != dust)
        {
          dust_handler(dust);
          is_dust_handled = true;
        }
        if (0 != chunk)
        {
          chunk_handler(chunk);
        }
      }
    }

    if (!is_dust_handled && 0 != dust)
    {
      dust_handler(dust);
    }
  }
  //---------------------------------------------------------------
  blobdata block_to_blob(const block& b);
  bool block_to_blob(const block& b, blobdata& b_blob);
  blobdata tx_to_blob(const transaction& b);
  bool tx_to_blob(const transaction& b, blobdata& b_blob);
  void get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h);
  crypto::hash get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes);
  crypto::hash get_tx_tree_hash(const block& b);
  bool is_valid_decomposed_amount(uint64_t amount);
  void get_hash_stats(uint64_t &tx_hashes_calculated, uint64_t &tx_hashes_cached, uint64_t &block_hashes_calculated, uint64_t & block_hashes_cached);

  crypto::secret_key encrypt_key(crypto::secret_key key, const epee::wipeable_string &passphrase);
  crypto::secret_key decrypt_key(crypto::secret_key key, const epee::wipeable_string &passphrase);
#define CHECKED_GET_SPECIFIC_VARIANT(variant_var, specific_type, variable_name, fail_return_val) \
  CHECK_AND_ASSERT_MES(variant_var.type() == typeid(specific_type), fail_return_val, "wrong variant type: " << variant_var.type().name() << ", expected " << typeid(specific_type).name()); \
  specific_type& variable_name = boost::get<specific_type>(variant_var);
}
