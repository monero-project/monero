// Copyright (c) 2021-2022, The Monero Project
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

#include "multisig_kex_msg.h"
#include "multisig_kex_msg_serialization.h"

#include "common/base58.h"
#include "crypto/crypto.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "include_base_utils.h"
#include "ringct/rctOps.h"
#include "serialization/binary_archive.h"
#include "serialization/serialization.h"

#include <boost/utility/string_ref.hpp> 

#include <sstream>
#include <utility>
#include <vector>


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multisig"

const boost::string_ref MULTISIG_KEX_V1_MAGIC{"MultisigV1"};
const boost::string_ref MULTISIG_KEX_MSG_V1_MAGIC{"MultisigxV1"};
const boost::string_ref MULTISIG_KEX_MSG_V2_MAGIC_1{"MultisigxV2R1"};  //round 1
const boost::string_ref MULTISIG_KEX_MSG_V2_MAGIC_N{"MultisigxV2Rn"};  //round n > 1

namespace multisig
{
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_kex_msg: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  multisig_kex_msg::multisig_kex_msg(const std::uint32_t round,
    const crypto::secret_key &signing_privkey,
    std::vector<crypto::public_key> msg_pubkeys,
    const crypto::secret_key &msg_privkey) :
      m_kex_round{round}
  {
    CHECK_AND_ASSERT_THROW_MES(round > 0, "Kex round must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(sc_check((const unsigned char*)&signing_privkey) == 0 &&
      signing_privkey != crypto::null_skey, "Invalid msg signing key.");

    if (round == 1)
    {
      CHECK_AND_ASSERT_THROW_MES(sc_check((const unsigned char*)&msg_privkey) == 0 &&
        msg_privkey != crypto::null_skey, "Invalid msg privkey.");

      m_msg_privkey = msg_privkey;
    }
    else
    {
      for (const auto &pubkey : msg_pubkeys)
      {
        CHECK_AND_ASSERT_THROW_MES(pubkey != crypto::null_pkey && pubkey != rct::rct2pk(rct::identity()),
          "Pubkey for message was invalid.");
        CHECK_AND_ASSERT_THROW_MES((rct::scalarmultKey(rct::pk2rct(pubkey), rct::curveOrder()) == rct::identity()),
          "Pubkey for message was not in prime subgroup.");
      }

      m_msg_pubkeys = std::move(msg_pubkeys);
    }
    CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(signing_privkey, m_signing_pubkey),
      "Failed to derive public key");

    // sets message and signing pub key
    construct_msg(signing_privkey);
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_kex_msg: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  multisig_kex_msg::multisig_kex_msg(std::string msg) : m_msg{std::move(msg)}
  {
    parse_and_validate_msg();
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_kex_msg: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  crypto::hash multisig_kex_msg::get_msg_to_sign() const
  {
    ////
    // msg_content = kex_round | signing_pubkey | expand(msg_pubkeys) | OPTIONAL msg_privkey
    // sign_msg = versioning-domain-sep | msg_content
    ///

    std::string data;
    CHECK_AND_ASSERT_THROW_MES(MULTISIG_KEX_MSG_V2_MAGIC_1.size() == MULTISIG_KEX_MSG_V2_MAGIC_N.size(),
      "Multisig kex msg magic inconsistency.");
    data.reserve(MULTISIG_KEX_MSG_V2_MAGIC_1.size() + 4 + 32*(1 + (m_kex_round == 1 ? 1 : 0) + m_msg_pubkeys.size()));

    // versioning domain-sep
    if (m_kex_round == 1)
      data.append(MULTISIG_KEX_MSG_V2_MAGIC_1.data(), MULTISIG_KEX_MSG_V2_MAGIC_1.size());
    else
      data.append(MULTISIG_KEX_MSG_V2_MAGIC_N.data(), MULTISIG_KEX_MSG_V2_MAGIC_N.size());

    // kex_round as little-endian bytes
    for (std::size_t i{0}; i < 4; ++i)
    {
      data += static_cast<char>(m_kex_round >> i*8);
    }

    // signing pubkey
    data.append((const char *)&m_signing_pubkey, sizeof(crypto::public_key));

    // add msg privkey if kex_round == 1
    if (m_kex_round == 1)
      data.append((const char *)&m_msg_privkey, sizeof(crypto::secret_key));
    else
    {
      // only add pubkeys if not round 1

      // msg pubkeys
      for (const auto &key : m_msg_pubkeys)
        data.append((const char *)&key, sizeof(crypto::public_key));
    }

    // message to sign
    crypto::hash hash;
    crypto::cn_fast_hash(data.data(), data.size(), hash);

    return hash;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_kex_msg: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_kex_msg::construct_msg(const crypto::secret_key &signing_privkey)
  {
    ////
    // msg_content = kex_round | signing_pubkey | expand(msg_pubkeys) | OPTIONAL msg_privkey
    // sign_msg = versioning-domain-sep | msg_content
    // msg = versioning-domain-sep | serialize(msg_content | crypto_sig[signing_privkey](sign_msg))
    ///

    // sign the message
    crypto::signature msg_signature;
    crypto::hash msg_to_sign{get_msg_to_sign()};
    crypto::generate_signature(msg_to_sign, m_signing_pubkey, signing_privkey, msg_signature);

    // assemble the message
    m_msg.clear();

    std::stringstream serialized_msg_ss;
    binary_archive<true> b_archive(serialized_msg_ss);

    if (m_kex_round == 1)
    {
      m_msg.append(MULTISIG_KEX_MSG_V2_MAGIC_1.data(), MULTISIG_KEX_MSG_V2_MAGIC_1.size());

      multisig_kex_msg_serializable_round1 msg_serializable;
      msg_serializable.msg_privkey    = m_msg_privkey;
      msg_serializable.signing_pubkey = m_signing_pubkey;
      msg_serializable.signature      = msg_signature;

      CHECK_AND_ASSERT_THROW_MES(::serialization::serialize(b_archive, msg_serializable),
        "Failed to serialize multisig kex msg");
    }
    else
    {
      m_msg.append(MULTISIG_KEX_MSG_V2_MAGIC_N.data(), MULTISIG_KEX_MSG_V2_MAGIC_N.size());

      multisig_kex_msg_serializable_general msg_serializable;
      msg_serializable.kex_round      = m_kex_round;
      msg_serializable.msg_pubkeys    = m_msg_pubkeys;
      msg_serializable.signing_pubkey = m_signing_pubkey;
      msg_serializable.signature      = msg_signature;

      CHECK_AND_ASSERT_THROW_MES(::serialization::serialize(b_archive, msg_serializable),
        "Failed to serialize multisig kex msg");
    }

    m_msg.append(tools::base58::encode(serialized_msg_ss.str()));
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_kex_msg: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_kex_msg::parse_and_validate_msg()
  {
    // check message type
    CHECK_AND_ASSERT_THROW_MES(m_msg.size() > 0, "Kex message unexpectedly empty.");
    CHECK_AND_ASSERT_THROW_MES(m_msg.substr(0, MULTISIG_KEX_V1_MAGIC.size()) != MULTISIG_KEX_V1_MAGIC,
      "V1 multisig kex messages are deprecated (unsafe).");
    CHECK_AND_ASSERT_THROW_MES(m_msg.substr(0, MULTISIG_KEX_MSG_V1_MAGIC.size()) != MULTISIG_KEX_MSG_V1_MAGIC,
      "V1 multisig kex messages are deprecated (unsafe).");

    // deserialize the message
    std::string msg_no_magic;
    CHECK_AND_ASSERT_THROW_MES(MULTISIG_KEX_MSG_V2_MAGIC_1.size() == MULTISIG_KEX_MSG_V2_MAGIC_N.size(),
      "Multisig kex msg magic inconsistency.");
    CHECK_AND_ASSERT_THROW_MES(tools::base58::decode(m_msg.substr(MULTISIG_KEX_MSG_V2_MAGIC_1.size()), msg_no_magic),
      "Multisig kex msg decoding error.");
    binary_archive<false> b_archive{epee::strspan<std::uint8_t>(msg_no_magic)};
    crypto::signature msg_signature;

    if (m_msg.substr(0, MULTISIG_KEX_MSG_V2_MAGIC_1.size()) == MULTISIG_KEX_MSG_V2_MAGIC_1)
    {
      // try round 1 message
      multisig_kex_msg_serializable_round1 kex_msg_rnd1;

      if (::serialization::serialize(b_archive, kex_msg_rnd1))
      {
        // in round 1 the message stores a private ancillary key component for the multisig account
        // that will be shared by all participants (e.g. a shared private view key)
        m_kex_round      = 1;
        m_msg_privkey    = kex_msg_rnd1.msg_privkey;
        m_signing_pubkey = kex_msg_rnd1.signing_pubkey;
        msg_signature    = kex_msg_rnd1.signature;
      }
      else
      {
        CHECK_AND_ASSERT_THROW_MES(false, "Deserializing kex msg failed.");
      }
    }
    else if (m_msg.substr(0, MULTISIG_KEX_MSG_V2_MAGIC_N.size()) == MULTISIG_KEX_MSG_V2_MAGIC_N)
    {
      // try general message
      multisig_kex_msg_serializable_general kex_msg_general;

      if (::serialization::serialize(b_archive, kex_msg_general))
      {
        m_kex_round      = kex_msg_general.kex_round;
        m_msg_privkey    = crypto::null_skey;
        m_msg_pubkeys    = std::move(kex_msg_general.msg_pubkeys);
        m_signing_pubkey = kex_msg_general.signing_pubkey;
        msg_signature    = kex_msg_general.signature;

        CHECK_AND_ASSERT_THROW_MES(m_kex_round > 1, "Invalid kex message round (must be > 1 for the general msg type).");
      }
      else
      {
        CHECK_AND_ASSERT_THROW_MES(false, "Deserializing kex msg failed.");
      }
    }
    else
    {
      // unknown message type
      CHECK_AND_ASSERT_THROW_MES(false, "Only v2 multisig kex messages are supported.");
    }

    // checks
    for (const auto &pubkey: m_msg_pubkeys)
    {
      CHECK_AND_ASSERT_THROW_MES(pubkey != crypto::null_pkey && pubkey != rct::rct2pk(rct::identity()),
        "Pubkey from message was invalid.");
      CHECK_AND_ASSERT_THROW_MES(rct::isInMainSubgroup(rct::pk2rct(pubkey)),
        "Pubkey from message was not in prime subgroup.");
    }

    CHECK_AND_ASSERT_THROW_MES(m_signing_pubkey != crypto::null_pkey && m_signing_pubkey != rct::rct2pk(rct::identity()),
      "Message signing key was invalid.");
    CHECK_AND_ASSERT_THROW_MES(rct::isInMainSubgroup(rct::pk2rct(m_signing_pubkey)),
      "Message signing key was not in prime subgroup.");

    // validate signature
    crypto::hash signed_msg{get_msg_to_sign()};
    CHECK_AND_ASSERT_THROW_MES(crypto::check_signature(signed_msg, m_signing_pubkey, msg_signature),
      "Multisig kex msg signature invalid.");
  }
  //----------------------------------------------------------------------------------------------------------------------
} //namespace multisig
