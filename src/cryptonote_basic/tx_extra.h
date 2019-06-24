// Copyright (c) 2014-2019, The Monero Project
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

#include "serialization/serialization.h"
#include "serialization/binary_archive.h"
#include "serialization/variant.h"
#include "crypto/crypto.h"
#include <boost/variant.hpp>


#define TX_EXTRA_PADDING_MAX_COUNT              255
#define TX_EXTRA_NONCE_MAX_COUNT                255

#define TX_EXTRA_TAG_PADDING                    0x00
#define TX_EXTRA_TAG_PUBKEY                     0x01
#define TX_EXTRA_NONCE                          0x02
#define TX_EXTRA_MERGE_MINING_TAG               0x03
#define TX_EXTRA_TAG_ADDITIONAL_PUBKEYS         0x04
#define TX_EXTRA_TAG_SERVICE_NODE_REGISTER      0x70
#define TX_EXTRA_TAG_SERVICE_NODE_DEREG_OLD     0x71
#define TX_EXTRA_TAG_SERVICE_NODE_WINNER        0x72
#define TX_EXTRA_TAG_SERVICE_NODE_CONTRIBUTOR   0x73
#define TX_EXTRA_TAG_SERVICE_NODE_PUBKEY        0x74
#define TX_EXTRA_TAG_TX_SECRET_KEY              0x75
#define TX_EXTRA_TAG_TX_KEY_IMAGE_PROOFS        0x76
#define TX_EXTRA_TAG_TX_KEY_IMAGE_UNLOCK        0x77
#define TX_EXTRA_TAG_SERVICE_NODE_STATE_CHANGE  0x78

#define TX_EXTRA_MYSTERIOUS_MINERGATE_TAG       0xDE

#define TX_EXTRA_NONCE_PAYMENT_ID               0x00
#define TX_EXTRA_NONCE_ENCRYPTED_PAYMENT_ID     0x01

namespace service_nodes {
  enum class new_state : uint16_t
  {
    deregister,
    decommission,
    recommission,
    _count
  };
};

namespace cryptonote
{
  struct tx_extra_padding
  {
    size_t size;

    // load
    template <template <bool> class Archive>
    bool do_serialize(Archive<false>& ar)
    {
      // size - 1 - because of variant tag
      for (size = 1; size <= TX_EXTRA_PADDING_MAX_COUNT; ++size)
      {
        std::ios_base::iostate state = ar.stream().rdstate();
        bool eof = EOF == ar.stream().peek();
        ar.stream().clear(state);

        if (eof)
          break;

        uint8_t zero;
        if (!::do_serialize(ar, zero))
          return false;

        if (0 != zero)
          return false;
      }

      return size <= TX_EXTRA_PADDING_MAX_COUNT;
    }

    // store
    template <template <bool> class Archive>
    bool do_serialize(Archive<true>& ar)
    {
      if(TX_EXTRA_PADDING_MAX_COUNT < size)
        return false;

      // i = 1 - because of variant tag
      for (size_t i = 1; i < size; ++i)
      {
        uint8_t zero = 0;
        if (!::do_serialize(ar, zero))
          return false;
      }
      return true;
    }
  };

  struct tx_extra_pub_key
  {
    crypto::public_key pub_key;

    BEGIN_SERIALIZE()
      FIELD(pub_key)
    END_SERIALIZE()
  };

  struct tx_extra_nonce
  {
    std::string nonce;

    BEGIN_SERIALIZE()
      FIELD(nonce)
      if(TX_EXTRA_NONCE_MAX_COUNT < nonce.size()) return false;
    END_SERIALIZE()
  };

  struct tx_extra_merge_mining_tag
  {
    struct serialize_helper
    {
      tx_extra_merge_mining_tag& mm_tag;

      serialize_helper(tx_extra_merge_mining_tag& mm_tag_) : mm_tag(mm_tag_)
      {
      }

      BEGIN_SERIALIZE()
        VARINT_FIELD_N("depth", mm_tag.depth)
        FIELD_N("merkle_root", mm_tag.merkle_root)
      END_SERIALIZE()
    };

    size_t depth;
    crypto::hash merkle_root;

    // load
    template <template <bool> class Archive>
    bool do_serialize(Archive<false>& ar)
    {
      std::string field;
      if(!::do_serialize(ar, field))
        return false;

      std::istringstream iss(field);
      binary_archive<false> iar(iss);
      serialize_helper helper(*this);
      return ::serialization::serialize(iar, helper);
    }

    // store
    template <template <bool> class Archive>
    bool do_serialize(Archive<true>& ar)
    {
      std::ostringstream oss;
      binary_archive<true> oar(oss);
      serialize_helper helper(*this);
      if(!::do_serialize(oar, helper))
        return false;

      std::string field = oss.str();
      return ::serialization::serialize(ar, field);
    }
  };

  // per-output additional tx pubkey for multi-destination transfers involving at least one subaddress
  struct tx_extra_additional_pub_keys
  {
    std::vector<crypto::public_key> data;

    BEGIN_SERIALIZE()
      FIELD(data)
    END_SERIALIZE()
  };

  struct tx_extra_mysterious_minergate
  {
    std::string data;

    BEGIN_SERIALIZE()
      FIELD(data)
    END_SERIALIZE()
  };

  struct tx_extra_service_node_winner
  {
    crypto::public_key m_service_node_key;

    BEGIN_SERIALIZE()
      FIELD(m_service_node_key)
    END_SERIALIZE()
  };

  struct tx_extra_service_node_pubkey
  {
    crypto::public_key m_service_node_key;

    BEGIN_SERIALIZE()
      FIELD(m_service_node_key)
    END_SERIALIZE()
  };


  struct tx_extra_service_node_register
  {
    std::vector<crypto::public_key> m_public_spend_keys;
    std::vector<crypto::public_key> m_public_view_keys;
    uint64_t m_portions_for_operator;
    std::vector<uint64_t> m_portions;
    uint64_t m_expiration_timestamp;
    crypto::signature m_service_node_signature;

    BEGIN_SERIALIZE()
      FIELD(m_public_spend_keys)
      FIELD(m_public_view_keys)
      FIELD(m_portions_for_operator)
      FIELD(m_portions)
      FIELD(m_expiration_timestamp)
      FIELD(m_service_node_signature)
    END_SERIALIZE()
  };

  struct tx_extra_service_node_contributor
  {
    crypto::public_key m_spend_public_key;
    crypto::public_key m_view_public_key;

    BEGIN_SERIALIZE()
      FIELD(m_spend_public_key)
      FIELD(m_view_public_key)
    END_SERIALIZE()
  };

  struct tx_extra_service_node_state_change
  {
    struct vote
    {
      vote() = default;
      vote(crypto::signature const &signature, uint32_t validator_index): signature(signature), validator_index(validator_index) { }
      crypto::signature signature;
      uint32_t          validator_index;

      BEGIN_SERIALIZE()
        VARINT_FIELD(validator_index);
        FIELD(signature);
      END_SERIALIZE()
    };

    service_nodes::new_state state;
    uint64_t                 block_height;
    uint32_t                 service_node_index;
    std::vector<vote>        votes;

    tx_extra_service_node_state_change() = default;

    template <typename... VotesArgs>
    tx_extra_service_node_state_change(service_nodes::new_state state, uint64_t block_height, uint32_t service_node_index, VotesArgs &&...votes)
        : state{state}, block_height{block_height}, service_node_index{service_node_index}, votes{std::forward<VotesArgs>(votes)...} {}

    // Compares equal if this represents a state change of the same SN (does *not* require equality of stored votes)
    bool operator==(const tx_extra_service_node_state_change &sc) const {
      return state == sc.state && block_height == sc.block_height && service_node_index == sc.service_node_index;
    }

    BEGIN_SERIALIZE()
      ENUM_FIELD(state, state < service_nodes::new_state::_count);
      VARINT_FIELD(block_height);
      VARINT_FIELD(service_node_index);
      FIELD(votes);
    END_SERIALIZE()
  };

  // Pre-Heimdall service node deregistration data; it doesn't carry the state change (it is only
  // used for deregistrations), and is stored slightly less efficiently in the tx extra data.
  struct tx_extra_service_node_deregister_old
  {
#pragma pack(push, 4)
    struct vote { // Not simply using state_change::vote because this gets blob serialized for v11 backwards compat
      vote() = default;
      vote(const tx_extra_service_node_state_change::vote &v) : signature{v.signature}, validator_index{v.validator_index} {}
      crypto::signature signature;
      uint32_t          validator_index;

      operator tx_extra_service_node_state_change::vote() const { return {signature, validator_index}; }
    };
#pragma pack(pop)
    static_assert(sizeof(vote) == sizeof(crypto::signature) + sizeof(uint32_t), "deregister_old tx extra vote size is not packed");

    uint64_t          block_height;
    uint32_t          service_node_index;
    std::vector<vote> votes;

    tx_extra_service_node_deregister_old() = default;
    tx_extra_service_node_deregister_old(const tx_extra_service_node_state_change &state_change)
      : block_height{state_change.block_height},
        service_node_index{state_change.service_node_index},
        votes{state_change.votes.begin(), state_change.votes.end()}
    {
      assert(state_change.state == service_nodes::new_state::deregister);
    }

    BEGIN_SERIALIZE()
      FIELD(block_height)
      FIELD(service_node_index)
      FIELD(votes)
    END_SERIALIZE()
  };

  struct tx_extra_tx_secret_key
  {
    crypto::secret_key key;

    BEGIN_SERIALIZE()
      FIELD(key)
    END_SERIALIZE()
  };

  struct tx_extra_tx_key_image_proofs
  {
    struct proof
    {
      crypto::key_image key_image;
      crypto::signature signature;
    };
    static_assert(sizeof(proof) == sizeof(crypto::key_image) + sizeof(crypto::signature), "tx_extra key image proof data structure is not packed");

    std::vector<proof> proofs;

    BEGIN_SERIALIZE()
      FIELD(proofs)
    END_SERIALIZE()
  };

  struct tx_extra_tx_key_image_unlock
  {
    crypto::key_image key_image;
    crypto::signature signature;
    uint32_t          nonce;

    // Compares equal if this represents the same key image unlock (but does *not* require equality of signature/nonce)
    bool operator==(const tx_extra_tx_key_image_unlock &other) const { return key_image == other.key_image; }

    BEGIN_SERIALIZE()
      FIELD(key_image)
      FIELD(signature)
      FIELD(nonce)
    END_SERIALIZE()
  };

  // tx_extra_field format, except tx_extra_padding and tx_extra_pub_key:
  //   varint tag;
  //   varint size;
  //   varint data[];
  typedef boost::variant<tx_extra_padding,
                         tx_extra_pub_key,
                         tx_extra_nonce,
                         tx_extra_merge_mining_tag,
                         tx_extra_additional_pub_keys,
                         tx_extra_mysterious_minergate,
                         tx_extra_service_node_pubkey,
                         tx_extra_service_node_register,
                         tx_extra_service_node_contributor,
                         tx_extra_service_node_winner,
                         tx_extra_service_node_state_change,
                         tx_extra_service_node_deregister_old,
                         tx_extra_tx_secret_key,
                         tx_extra_tx_key_image_proofs,
                         tx_extra_tx_key_image_unlock
                        > tx_extra_field;
}

BLOB_SERIALIZER(cryptonote::tx_extra_service_node_deregister_old::vote);
BLOB_SERIALIZER(cryptonote::tx_extra_tx_key_image_proofs::proof);

VARIANT_TAG(binary_archive, cryptonote::tx_extra_padding,                     TX_EXTRA_TAG_PADDING);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_pub_key,                     TX_EXTRA_TAG_PUBKEY);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_nonce,                       TX_EXTRA_NONCE);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_merge_mining_tag,            TX_EXTRA_MERGE_MINING_TAG);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_additional_pub_keys,         TX_EXTRA_TAG_ADDITIONAL_PUBKEYS);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_mysterious_minergate,        TX_EXTRA_MYSTERIOUS_MINERGATE_TAG);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_register,       TX_EXTRA_TAG_SERVICE_NODE_REGISTER);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_state_change,   TX_EXTRA_TAG_SERVICE_NODE_STATE_CHANGE);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_deregister_old, TX_EXTRA_TAG_SERVICE_NODE_DEREG_OLD);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_contributor,    TX_EXTRA_TAG_SERVICE_NODE_CONTRIBUTOR);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_winner,         TX_EXTRA_TAG_SERVICE_NODE_WINNER);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_service_node_pubkey,         TX_EXTRA_TAG_SERVICE_NODE_PUBKEY);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_tx_secret_key,               TX_EXTRA_TAG_TX_SECRET_KEY);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_tx_key_image_proofs,         TX_EXTRA_TAG_TX_KEY_IMAGE_PROOFS);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_tx_key_image_unlock,         TX_EXTRA_TAG_TX_KEY_IMAGE_UNLOCK);
