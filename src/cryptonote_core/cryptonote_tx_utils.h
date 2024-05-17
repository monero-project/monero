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

#pragma once
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "fcmp_pp/curve_trees.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include "ringct/rctOps.h"

namespace cryptonote
{
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_weight, uint64_t already_generated_coins, size_t current_block_weight, uint64_t fee, const account_public_address &miner_address, transaction& tx, const blobdata& extra_nonce = blobdata(), size_t max_outs = 999, uint8_t hard_fork_version = 1);

  struct tx_source_entry
  {
    typedef std::pair<uint64_t, rct::ctkey> output_entry;

    std::vector<output_entry> outputs;  //index + key + optional ringct commitment
    uint64_t real_output;               //index in outputs vector of real output_entry
    crypto::public_key real_out_tx_key; //incoming real tx public key
    std::vector<crypto::public_key> real_out_additional_tx_keys; //incoming real tx additional public keys
    uint64_t real_output_in_tx_index;   //index in transaction outputs vector
    uint64_t amount;                    //money
    bool rct;                           //true if the output is rct
    rct::key mask;                      //ringct amount mask
    rct::multisig_kLRki multisig_kLRki; //multisig info

    FcmpRerandomizedOutputCompressed rerandomized_output;

    void push_output(uint64_t idx, const crypto::public_key &k, uint64_t amount) { outputs.push_back(std::make_pair(idx, rct::ctkey({rct::pk2rct(k), rct::zeroCommitVartime(amount)}))); }

    BEGIN_SERIALIZE_OBJECT()
      FIELD(outputs)
      FIELD(real_output)
      FIELD(real_out_tx_key)
      FIELD(real_out_additional_tx_keys)
      FIELD(real_output_in_tx_index)
      FIELD(amount)
      FIELD(rct)
      FIELD(mask)
      FIELD(multisig_kLRki)

      if (real_output >= outputs.size())
        return false;
    END_SERIALIZE()
  };

  struct tx_destination_entry
  {
    std::string original;
    uint64_t amount;                    //money
    account_public_address addr;        //destination address
    bool is_subaddress;
    bool is_integrated;

    tx_destination_entry() : amount(0), addr(AUTO_VAL_INIT(addr)), is_subaddress(false), is_integrated(false) { }
    tx_destination_entry(uint64_t a, const account_public_address &ad, bool is_subaddress) : amount(a), addr(ad), is_subaddress(is_subaddress), is_integrated(false) { }
    tx_destination_entry(const std::string &o, uint64_t a, const account_public_address &ad, bool is_subaddress) : original(o), amount(a), addr(ad), is_subaddress(is_subaddress), is_integrated(false) { }

    std::string address(network_type nettype, const crypto::hash &payment_id) const
    {
      if (!original.empty())
      {
        return original;
      }

      if (is_integrated)
      {
        return get_account_integrated_address_as_str(nettype, addr, reinterpret_cast<const crypto::hash8 &>(payment_id));
      }

      return get_account_address_as_str(nettype, is_subaddress, addr);
    }

    BEGIN_SERIALIZE_OBJECT()
      FIELD(original)
      VARINT_FIELD(amount)
      FIELD(addr)
      FIELD(is_subaddress)
      FIELD(is_integrated)
    END_SERIALIZE()
  };

  //---------------------------------------------------------------

  struct tx_block_template_backlog_entry
  {
    crypto::hash id;
    uint64_t weight;
    uint64_t fee;
  };

  //---------------------------------------------------------------
  crypto::public_key get_destination_view_key_pub(const std::vector<tx_destination_entry> &destinations, const boost::optional<cryptonote::account_public_address>& change_addr);
  bool construct_tx(const account_keys& sender_account_keys, std::vector<tx_source_entry> &sources, const std::vector<tx_destination_entry>& destinations, const boost::optional<cryptonote::account_public_address>& change_addr, const std::vector<uint8_t> &extra, transaction& tx);
  bool construct_tx_with_tx_key(const account_keys& sender_account_keys, const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, std::vector<tx_source_entry>& sources, std::vector<tx_destination_entry>& destinations, const boost::optional<cryptonote::account_public_address>& change_addr, const std::vector<uint8_t> &extra, transaction& tx, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, fcmp_pp::ProofParams &fcmp_pp_params, bool rct = false, const rct::RCTConfig &rct_config = { rct::RangeProofBorromean, 0 }, bool shuffle_outs = true, bool use_view_tags = false);
  bool construct_tx_and_get_tx_key(const account_keys& sender_account_keys, const std::unordered_map<crypto::public_key, subaddress_index>& subaddresses, std::vector<tx_source_entry>& sources, std::vector<tx_destination_entry>& destinations, const boost::optional<cryptonote::account_public_address>& change_addr, const std::vector<uint8_t> &extra, transaction& tx, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys, fcmp_pp::ProofParams &fcmp_pp_params, bool rct = false, const rct::RCTConfig &rct_config = { rct::RangeProofBorromean, 0 }, bool use_view_tags = false);
  bool generate_output_ephemeral_keys(const size_t tx_version, const cryptonote::account_keys &sender_account_keys, const crypto::public_key &txkey_pub,  const crypto::secret_key &tx_key,
                                      const cryptonote::tx_destination_entry &dst_entr, const boost::optional<cryptonote::account_public_address> &change_addr, const size_t output_index,
                                      const bool &need_additional_txkeys, const std::vector<crypto::secret_key> &additional_tx_keys,
                                      std::vector<crypto::public_key> &additional_tx_public_keys,
                                      std::vector<rct::key> &amount_keys,
                                      crypto::public_key &out_eph_public_key,
                                      const bool use_view_tags, crypto::view_tag &view_tag) ;

  bool generate_output_ephemeral_keys(const size_t tx_version, const cryptonote::account_keys &sender_account_keys, const crypto::public_key &txkey_pub,  const crypto::secret_key &tx_key,
                                      const cryptonote::tx_destination_entry &dst_entr, const boost::optional<cryptonote::account_public_address> &change_addr, const size_t output_index,
                                      const bool &need_additional_txkeys, const std::vector<crypto::secret_key> &additional_tx_keys,
                                      std::vector<crypto::public_key> &additional_tx_public_keys,
                                      std::vector<rct::key> &amount_keys,
                                      crypto::public_key &out_eph_public_key,
                                      const bool use_view_tags, crypto::view_tag &view_tag) ;

  bool generate_genesis_block(
      block& bl
    , std::string const & genesis_tx
    , uint32_t nonce
    );

  class Blockchain;
  bool get_block_longhash(const Blockchain *pb, const blobdata& bd, crypto::hash& res, const uint64_t height, const int major_version, const crypto::hash *seed_hash, const int miners = 0);
  bool get_block_longhash(const Blockchain *pb, const block& b, crypto::hash& res, const uint64_t height, const crypto::hash *seed_hash = nullptr, const int miners = 0);
  crypto::hash get_block_longhash(const Blockchain *pb, const block& b, const uint64_t height, const crypto::hash *seed_hash = nullptr, const int miners = 0);
  void get_altblock_longhash(const block& b, crypto::hash& res, const crypto::hash& seed_hash);

}

BOOST_CLASS_VERSION(cryptonote::tx_source_entry, 1)
BOOST_CLASS_VERSION(cryptonote::tx_destination_entry, 2)

namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, cryptonote::tx_source_entry &x, const boost::serialization::version_type ver)
    {
      a & x.outputs;
      a & x.real_output;
      a & x.real_out_tx_key;
      a & x.real_output_in_tx_index;
      a & x.amount;
      a & x.rct;
      a & x.mask;
      if (ver < 1)
        return;
      a & x.multisig_kLRki;
      a & x.real_out_additional_tx_keys;
    }

    template <class Archive>
    inline void serialize(Archive& a, cryptonote::tx_destination_entry& x, const boost::serialization::version_type ver)
    {
      a & x.amount;
      a & x.addr;
      if (ver < 1)
        return;
      a & x.is_subaddress;
      if (ver < 2)
      {
        x.is_integrated = false;
        return;
      }
      a & x.original;
      a & x.is_integrated;
    }
  }
}
