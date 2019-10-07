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

#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_basic_impl.h"
#include "string_tools.h"
#include "serialization/binary_utils.h"
#include "serialization/container.h"
#include "cryptonote_format_utils.h"
#include "cryptonote_config.h"
#include "misc_language.h"
#include "common/base58.h"
#include "crypto/hash.h"
#include "int-util.h"
#include "common/dns_utils.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

namespace cryptonote {

  struct integrated_address {
    account_public_address adr;
    crypto::hash8 payment_id;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(adr)
      FIELD(payment_id)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(payment_id)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /* Cryptonote helper functions                                          */
  /************************************************************************/
  //-----------------------------------------------------------------------------------------------
  size_t get_min_block_weight(uint8_t version)
  {
    if (version < 2)
      return CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
    if (version < 5)
      return CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2;
    return CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5;
  }
  //-----------------------------------------------------------------------------------------------
  size_t get_max_tx_size()
  {
    return CRYPTONOTE_MAX_TX_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  bool get_block_reward(size_t median_weight, size_t current_block_weight, uint64_t already_generated_coins, uint64_t &reward, uint8_t version) {
    static_assert(DIFFICULTY_TARGET_V2%60==0&&DIFFICULTY_TARGET_V1%60==0,"difficulty targets must be a multiple of 60");
    const int target = version < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2;
    const int target_minutes = target / 60;
    const int emission_speed_factor = EMISSION_SPEED_FACTOR_PER_MINUTE - (target_minutes-1);

    uint64_t base_reward = (MONEY_SUPPLY - already_generated_coins) >> emission_speed_factor;
    if (base_reward < FINAL_SUBSIDY_PER_MINUTE*target_minutes)
    {
      base_reward = FINAL_SUBSIDY_PER_MINUTE*target_minutes;
    }

    uint64_t full_reward_zone = get_min_block_weight(version);

    //make it soft
    if (median_weight < full_reward_zone) {
      median_weight = full_reward_zone;
    }

    if (current_block_weight <= median_weight) {
      reward = base_reward;
      return true;
    }

    if(current_block_weight > 2 * median_weight) {
      MERROR("Block cumulative weight is too big: " << current_block_weight << ", expected less than " << 2 * median_weight);
      return false;
    }

    uint64_t product_hi;
    // BUGFIX: 32-bit saturation bug (e.g. ARM7), the result was being
    // treated as 32-bit by default.
    uint64_t multiplicand = 2 * median_weight - current_block_weight;
    multiplicand *= current_block_weight;
    uint64_t product_lo = mul128(base_reward, multiplicand, &product_hi);

    uint64_t reward_hi;
    uint64_t reward_lo;
    div128_64(product_hi, product_lo, median_weight, &reward_hi, &reward_lo, NULL, NULL);
    div128_64(reward_hi, reward_lo, median_weight, &reward_hi, &reward_lo, NULL, NULL);
    assert(0 == reward_hi);
    assert(reward_lo < base_reward);

    reward = reward_lo;
    return true;
  }
  //------------------------------------------------------------------------------------
  uint8_t get_account_address_checksum(const public_address_outer_blob& bl)
  {
    const unsigned char* pbuf = reinterpret_cast<const unsigned char*>(&bl);
    uint8_t summ = 0;
    for(size_t i = 0; i!= sizeof(public_address_outer_blob)-1; i++)
      summ += pbuf[i];

    return summ;
  }
  //------------------------------------------------------------------------------------
  uint8_t get_account_integrated_address_checksum(const public_integrated_address_outer_blob& bl)
  {
    const unsigned char* pbuf = reinterpret_cast<const unsigned char*>(&bl);
    uint8_t summ = 0;
    for(size_t i = 0; i!= sizeof(public_integrated_address_outer_blob)-1; i++)
      summ += pbuf[i];

    return summ;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_as_str(
      network_type nettype
    , bool subaddress
    , account_public_address const & adr
    )
  {
    uint64_t address_prefix = subaddress ? get_config(nettype).CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX : get_config(nettype).CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX;

    return tools::base58::encode_addr(address_prefix, t_serializable_object_to_blob(adr));
  }
  //-----------------------------------------------------------------------
  std::string get_account_integrated_address_as_str(
      network_type nettype
    , account_public_address const & adr
    , crypto::hash8 const & payment_id
    )
  {
    uint64_t integrated_address_prefix = get_config(nettype).CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX;

    integrated_address iadr = {
      adr, payment_id
    };
    return tools::base58::encode_addr(integrated_address_prefix, t_serializable_object_to_blob(iadr));
  }
  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx)
  {
    if(tx.vin.size() != 1)
      return false;

    if(tx.vin[0].type() != typeid(txin_gen))
      return false;

    return true;
  }
  //-----------------------------------------------------------------------
  bool get_account_address_from_str(
      address_parse_info& info
    , network_type nettype
    , std::string const & str
    )
  {
    uint64_t address_prefix = get_config(nettype).CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX;
    uint64_t integrated_address_prefix = get_config(nettype).CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX;
    uint64_t subaddress_prefix = get_config(nettype).CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX;

    if (2 * sizeof(public_address_outer_blob) != str.size())
    {
      blobdata data;
      uint64_t prefix;
      if (!tools::base58::decode_addr(str, prefix, data))
      {
        LOG_PRINT_L2("Invalid address format");
        return false;
      }

      if (integrated_address_prefix == prefix)
      {
        info.is_subaddress = false;
        info.has_payment_id = true;
      }
      else if (address_prefix == prefix)
      {
        info.is_subaddress = false;
        info.has_payment_id = false;
      }
      else if (subaddress_prefix == prefix)
      {
        info.is_subaddress = true;
        info.has_payment_id = false;
      }
      else {
        LOG_PRINT_L1("Wrong address prefix: " << prefix << ", expected " << address_prefix 
          << " or " << integrated_address_prefix
          << " or " << subaddress_prefix);
        return false;
      }

      if (info.has_payment_id)
      {
        integrated_address iadr;
        if (!::serialization::parse_binary(data, iadr))
        {
          LOG_PRINT_L1("Account public address keys can't be parsed");
          return false;
        }
        info.address = iadr.adr;
        info.payment_id = iadr.payment_id;
      }
      else
      {
        if (!::serialization::parse_binary(data, info.address))
        {
          LOG_PRINT_L1("Account public address keys can't be parsed");
          return false;
        }
      }

      if (!crypto::check_key(info.address.m_spend_public_key) || !crypto::check_key(info.address.m_view_public_key))
      {
        LOG_PRINT_L1("Failed to validate address keys");
        return false;
      }
    }
    else
    {
      // Old address format
      std::string buff;
      if(!string_tools::parse_hexstr_to_binbuff(str, buff))
        return false;

      if(buff.size()!=sizeof(public_address_outer_blob))
      {
        LOG_PRINT_L1("Wrong public address size: " << buff.size() << ", expected size: " << sizeof(public_address_outer_blob));
        return false;
      }

      public_address_outer_blob blob = *reinterpret_cast<const public_address_outer_blob*>(buff.data());


      if(blob.m_ver > CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER)
      {
        LOG_PRINT_L1("Unknown version of public address: " << blob.m_ver << ", expected " << CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER);
        return false;
      }

      if(blob.check_sum != get_account_address_checksum(blob))
      {
        LOG_PRINT_L1("Wrong public address checksum");
        return false;
      }

      //we success
      info.address = blob.m_address;
      info.is_subaddress = false;
      info.has_payment_id = false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool get_account_address_from_str_or_url(
      address_parse_info& info
    , network_type nettype
    , const std::string& str_or_url
    , std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm
    )
  {
    if (get_account_address_from_str(info, nettype, str_or_url))
      return true;
    bool dnssec_valid;
    std::string address_str = tools::dns_utils::get_account_address_as_str_from_url(str_or_url, dnssec_valid, dns_confirm);
    return !address_str.empty() &&
      get_account_address_from_str(info, nettype, address_str);
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const cryptonote::transaction& a, const cryptonote::transaction& b) {
    return cryptonote::get_transaction_hash(a) == cryptonote::get_transaction_hash(b);
  }

  bool operator ==(const cryptonote::block& a, const cryptonote::block& b) {
    return cryptonote::get_block_hash(a) == cryptonote::get_block_hash(b);
  }
}

//--------------------------------------------------------------------------------
bool parse_hash256(const std::string &str_hash, crypto::hash& hash)
{
  std::string buf;
  bool res = epee::string_tools::parse_hexstr_to_binbuff(str_hash, buf);
  if (!res || buf.size() != sizeof(crypto::hash))
  {
    MERROR("invalid hash format: " << str_hash);
    return false;
  }
  else
  {
    buf.copy(reinterpret_cast<char *>(&hash), sizeof(crypto::hash));
    return true;
  }
}
