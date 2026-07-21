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

#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_basic_impl.h"
#include "string_tools.h"
#include "serialization/binary_utils.h"
#include "cryptonote_format_utils.h"
#include "cryptonote_config.h"
#include "common/base58.h"
#include "crypto/hash.h"
#include "int-util.h"
#include "misc_log_ex.h"
#include "common/dns_utils.h"
#include "serialization/crypto.h"
#include "serialization/string.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

#define FIELDS_VARINT(v) do { ar.serialize_uvarint(v); if (!ar.good()) return false; } while (0)
#define FIELDS_VARINT_SIGNED(v) do { serialize_small_signed_varint(ar, v); if (!ar.good()) return false; } while (0);

namespace
{
//! @brief Serialize varint and encode sign val as LSB
void serialize_small_signed_varint(binary_archive<true> &ar, std::int64_t v)
{
  const bool is_neg = v < 0;
  const std::uint64_t varint_val = (static_cast<std::uint64_t>(std::abs(v)) << 1) + is_neg;
  ar.serialize_uvarint(varint_val);
}

//! @brief Deserialize varint and decode sign val as LSB
void serialize_small_signed_varint(binary_archive<false> &ar, std::int64_t &v)
{
  std::uint64_t varint_val;
  ar.serialize_uvarint(varint_val);
  if (1 == varint_val) // -0 encoding
    ar.set_fail();
  const char neg = 1 - (static_cast<char>(varint_val & 1) << 1);
  v = static_cast<std::int64_t>(varint_val >> 1) * neg;
}
} //anonymous namespace

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
  bool is_coinbase(const transaction_prefix& tx)
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
  //--------------------------------------------------------------------------------
  bool operator ==(const block_header &a, const block_header &b)
  {
    static_assert(MAX_HF_VERSION == 16, "Double check this impl when bumping HF versions");
    return a.major_version == b.major_version
        && a.minor_version == b.minor_version
        && a.timestamp     == b.timestamp
        && a.prev_id       == b.prev_id
        && a.nonce         == b.nonce
    ;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const cryptonote::block& a, const cryptonote::block& b) {
    return cryptonote::get_block_hash(a) == cryptonote::get_block_hash(b);
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const hashable_block_header_info &a, const hashable_block_header_info &b)
  {
    return a.header == b.header && a.block_content_hash == b.block_content_hash && a.n_txs_in_block == b.n_txs_in_block;
  }
  //--------------------------------------------------------------------------------
  int compare_hash32_reversed_nbits(const crypto::hash& ha, const crypto::hash& hb, unsigned int nbits)
  {
    static_assert(sizeof(uint64_t) * 4 == sizeof(crypto::hash), "hash is wrong size");

    // We have to copy these buffers b/c of the strict aliasing rule
    uint64_t va[4];
    memcpy(va, &ha, sizeof(crypto::hash));
    uint64_t vb[4];
    memcpy(vb, &hb, sizeof(crypto::hash));

    for (int n = 3; n >= 0 && nbits; --n)
    {
      const unsigned int msb_nbits = std::min<unsigned int>(64, nbits);
      const uint64_t lsb_nbits_dropped = static_cast<uint64_t>(64 - msb_nbits);
      const uint64_t van = SWAP64LE(va[n]) >> lsb_nbits_dropped;
      const uint64_t vbn = SWAP64LE(vb[n]) >> lsb_nbits_dropped;
      nbits -= msb_nbits;

      if (van < vbn) return -1; else if (van > vbn) return 1;
    }

    return 0;
  }

  crypto::hash make_hash32_loose_template(unsigned int nbits, const crypto::hash& h)
  {
    static_assert(sizeof(uint64_t) * 4 == sizeof(crypto::hash), "hash is wrong size");

    // We have to copy this buffer b/c of the strict aliasing rule
    uint64_t vh[4];
    memcpy(vh, &h, sizeof(crypto::hash));

    for (int n = 3; n >= 0; --n)
    {
      const unsigned int msb_nbits = std::min<unsigned int>(64, nbits);
      const uint64_t mask = msb_nbits ? (~((std::uint64_t(1) << (64 - msb_nbits)) - 1)) : 0;
      nbits -= msb_nbits;

      vh[n] &= SWAP64LE(mask);
    }

    crypto::hash res;
    memcpy(&res, vh, sizeof(crypto::hash));
    return res;
  }
  //--------------------------------------------------------------------------------
  bool calculate_block_hash_from_header_info(const hashable_block_header_info &header_info, crypto::hash &hash_out)
  {
    //! @TODO: Update header info -> block hashing blob logic when we bump to v17 w/ commitments
    CHECK_AND_ASSERT_MES(header_info.header.major_version <= 16,
      false, "Unrecognized block major version: " << header_info.header.major_version);

    constexpr unsigned char EXISTING_202612_MERKLE_HASH[32] = {
      139, 103, 77, 234, 197, 70, 109, 119,
      223, 58, 49, 182, 41, 210, 227, 3,
      158, 165, 124, 33, 147, 216, 146, 56,
      92, 251, 220, 80, 24, 137, 26, 51
    };

    constexpr unsigned char CORRECT_202612_MERKLE_HASH[32] = {
      243, 83, 201, 109, 231, 76, 83, 248,
      115, 137, 182, 111, 166, 37, 237, 31,
      134, 118, 190, 235, 93, 71, 180, 240,
      25, 59, 209, 107, 88, 73, 51, 190
    };

    CHECK_AND_ASSERT_MES(0 != memcmp(&header_info.block_content_hash, EXISTING_202612_MERKLE_HASH, crypto::HASH_SIZE),
      false, "Block content hash cannot be old, unbinding 202612 merkle root");

    crypto::hash adjusted_block_content_hash = header_info.block_content_hash;
    if (0 == memcmp(&header_info.block_content_hash, CORRECT_202612_MERKLE_HASH, crypto::HASH_SIZE))
      memcpy(&adjusted_block_content_hash, EXISTING_202612_MERKLE_HASH, crypto::HASH_SIZE);

    constexpr std::size_t max_varint_size = 10;
    constexpr std::size_t max_blob_size = 2 + 2 + 2 * max_varint_size + 2 * crypto::HASH_SIZE + 4 + /*object_hash*/1;
    static_assert(max_blob_size < 128, "Object hash varint prefix cannot fit in one byte");

    // This "block hashing blob" is slightly different from the one in cryptonote_format_utils
    // in that it includes the varint prefix added by get_object_hash(). This skips one allocation.
    unsigned char block_hashing_blob[max_blob_size];
    unsigned char *p = block_hashing_blob + 1;
    tools::write_varint(p, header_info.header.major_version);
    tools::write_varint(p, header_info.header.minor_version);
    tools::write_varint(p, header_info.header.timestamp);
    memcpy(p, header_info.header.prev_id.data, crypto::HASH_SIZE);
    p += 32;
    const uint32_t nonce_le = SWAP32LE(header_info.header.nonce);
    memcpy(p, &nonce_le, 4);
    p += 4;
    memcpy(p, adjusted_block_content_hash.data, crypto::HASH_SIZE);
    p += 32;
    tools::write_varint(p, header_info.n_txs_in_block);
    assert(p >= block_hashing_blob + 73);
    assert(p <= block_hashing_blob + max_blob_size);
    block_hashing_blob[0] = p - 1 - block_hashing_blob; // varint length prefix  added by get_object_hash()

    crypto::cn_fast_hash(block_hashing_blob, p - block_hashing_blob, hash_out);
    return true;
  }
  //--------------------------------------------------------------------------------
  static constexpr std::uint8_t compressed_header_version = 0;
  //--------------------------------------------------------------------------------
  bool compress_block_header_chain(epee::span<const hashable_block_header_info> headers, std::string &headers_blob_out)
  {
    //! @TODO: intermediate PoW hash when applicable

    headers_blob_out.clear();
    const std::size_t n_blocks = headers.size();

    // Start writing main prefix:
    //  - Format version
    //  - Number of blocks
    //  - First prev_id
    // Base timestamp is written later, after timestamp diff average is determined for first major span
    std::stringstream ss;
    binary_archive<true> ar(ss);
    FIELDS(compressed_header_version);
    FIELDS_VARINT(n_blocks);
    if (0 == n_blocks)
    {
      headers_blob_out = ss.str();
      return ar.good();
    }
    const block_header first_header = headers[0].header;
    FIELDS(const_cast<crypto::hash&>(first_header.prev_id));

    std::size_t header_begin = 0;
    std::int64_t last_timestamp = 0;
    std::int64_t last_n_txs_in_block = 0;
    while (header_begin < n_blocks)
    {
      // Search for next span of consecutive headers sharing the same major version.
      const std::uint8_t major_version = headers[header_begin].header.major_version;
      std::size_t header_end = header_begin;
      for (; header_end < n_blocks && headers[header_end].header.major_version == major_version; ++header_end) {}
      assert(header_end > header_begin);
      const std::size_t n_headers_of_major_version = header_end - header_begin;

      // Calculate a good timestamp difference to use as a reference, we use the average diff, excluding genesis.
      // You could also do the median, but that's more expensive to calculate. Average tends to be pretty good.
      const bool begin_is_genesis = headers[header_begin].header.prev_id == crypto::null_hash;
      const std::size_t top_timestamp_index = header_end - 1;
      const std::size_t bottom_timestamp_index = header_begin + begin_is_genesis;
      std::uint64_t ts_diff_avg = 0;
      if (top_timestamp_index > bottom_timestamp_index)
      {
        const std::uint64_t top_timestamp = headers[top_timestamp_index].header.timestamp;
        const std::uint64_t bottom_timestamp = headers[bottom_timestamp_index].header.timestamp;
        if (top_timestamp > bottom_timestamp)
          ts_diff_avg = (top_timestamp - bottom_timestamp) / (top_timestamp_index - bottom_timestamp_index);
      }

      // Before the first major span prefix is serialized, put the "base timestamp" into the main prefix
      const bool first_major_span = 0 == header_begin;
      if (first_major_span)
      {
        const std::uint64_t base_timestamp = std::max(first_header.timestamp, ts_diff_avg) - ts_diff_avg;
        FIELDS_VARINT(base_timestamp);
        last_timestamp = base_timestamp;
      }

      // Write major span prefix:
      //   - Number of blocks in major span
      //   - Major version
      //   - Average of differences between timestamps
      FIELDS_VARINT(n_headers_of_major_version);
      FIELDS(major_version);
      FIELDS_VARINT(ts_diff_avg);

      while (header_begin < header_end)
      {
        // Search for next span of consecutive headers sharing the same minor version.
        const std::uint8_t minor_version = headers[header_begin].header.minor_version;
        std::size_t header_minor_end = header_begin;
        while (header_minor_end < header_end && headers[header_minor_end].header.minor_version == minor_version)
        {
          ++header_minor_end;
        }
        assert(header_minor_end > header_begin);
        const std::size_t n_headers_of_minor_version = header_minor_end - header_begin;

        // Write minor span prefix:
        //   - Number of blocks in minor span
        //   - Minor version
        FIELDS_VARINT(n_headers_of_minor_version);
        FIELDS(minor_version);

        for (; header_begin < header_minor_end; ++header_begin)
        {
          assert(header_begin < n_blocks);
          const hashable_block_header_info &header_info = headers[header_begin];
          const block_header &header = header_info.header;

          // skip major, minor version

          // serialize timestamp as deviation from average diff
          CHECK_AND_ASSERT_MES(0 == (header.timestamp >> 63), false, "Header timestamp cannot have MSB set");
          std::int64_t ts_dev = (static_cast<std::int64_t>(header.timestamp) - last_timestamp);
          ts_dev -= static_cast<std::int64_t>(ts_diff_avg);
          FIELDS_VARINT_SIGNED(ts_dev);
          last_timestamp = static_cast<int64_t>(header.timestamp);

          // skip prev_id, only first is serialized, rest can be inferred

          FIELDS(header.nonce);
          FIELDS(const_cast<crypto::hash&>(header_info.block_content_hash));

          // serialize the number of txs in block as diff from previous
          CHECK_AND_ASSERT_MES(0 == (header_info.n_txs_in_block >> 63), false, "Block num txs cannot have MSB set");
          const std::int64_t n_txs_diff = (static_cast<std::int64_t>(header_info.n_txs_in_block) - last_n_txs_in_block);
          FIELDS_VARINT_SIGNED(n_txs_diff);
          last_n_txs_in_block = static_cast<std::int64_t>(header_info.n_txs_in_block);
        }
      }
    }

    assert(header_begin == n_blocks);

    headers_blob_out = ss.str();

    return ar.good();
  }
  //--------------------------------------------------------------------------------
  bool decompress_block_header_chain(epee::span<const unsigned char> headers_blob,
    std::vector<hashable_block_header_info> &headers_out)
  {
    headers_out.clear();

    constexpr std::size_t min_header_size = 1 + 4 + 32 + 1;

    binary_archive<false> ar(headers_blob);

    std::uint8_t version;
    FIELDS(version);
    CHECK_AND_ASSERT_MES(version <= compressed_header_version, false, "Unrecognized header chain version: " << version);

    std::size_t n_blocks;
    FIELDS_VARINT(n_blocks);
    if (0 == n_blocks)
      return ::serialization::check_stream_state(ar);
    CHECK_AND_ASSERT_MES(ar.remaining_bytes() / min_header_size >= n_blocks,
      false, "Too many claimed headers in chain: " << n_blocks);

    headers_out.reserve(n_blocks);

    crypto::hash prev_id;
    FIELDS(prev_id);

    std::uint64_t last_timestamp = 0;
    FIELDS_VARINT(last_timestamp);

    std::size_t n_headers_of_major_version = 0;
    std::size_t n_headers_of_minor_version = 0;
    std::uint8_t major_version;
    std::uint8_t minor_version;
    std::uint64_t ts_diff_avg = 0;
    std::uint64_t last_n_txs_in_block = 0;
    while (headers_out.size() < n_blocks)
    {
      if (!n_headers_of_major_version)
      {
        FIELDS_VARINT(n_headers_of_major_version);
        FIELDS(major_version);
        FIELDS_VARINT(ts_diff_avg);
        CHECK_AND_ASSERT_MES(n_headers_of_major_version, false, "n_headers_of_major_version should be non-zero");
        CHECK_AND_ASSERT_MES(n_headers_of_major_version <= (n_blocks - headers_out.size()),
          false, "n_headers_of_major_version should be less than or equal to remaining block header count");
      }
      if (!n_headers_of_minor_version)
      {
        FIELDS_VARINT(n_headers_of_minor_version);
        FIELDS(minor_version);
        CHECK_AND_ASSERT_MES(n_headers_of_minor_version, false, "n_headers_of_minor_version should be non-zero");
        CHECK_AND_ASSERT_MES(n_headers_of_minor_version <= n_headers_of_major_version,
          false, "n_headers_of_minor_version should be less than or equal to n_headers_of_major_version");
      }
      --n_headers_of_major_version;
      --n_headers_of_minor_version;

      hashable_block_header_info &header_info = headers_out.emplace_back();
      block_header &header = header_info.header;

      header.major_version = major_version;
      header.minor_version = minor_version;

      // serialize timestamp as diff from prev, sign is LSB of varint
      std::int64_t ts_dev;
      FIELDS_VARINT_SIGNED(ts_dev);
      header.timestamp = last_timestamp + ts_diff_avg + ts_dev; //! @TODO: check overflow
      CHECK_AND_ASSERT_MES(0 == (header.timestamp >> 63), false, "Cumulative header timestamp cannot have MSB set");
      last_timestamp = header.timestamp;

      header.prev_id = prev_id;

      FIELDS(header.nonce);
      FIELDS(header_info.block_content_hash);

      std::int64_t n_txs_diff;
      FIELDS_VARINT_SIGNED(n_txs_diff);
      header_info.n_txs_in_block = last_n_txs_in_block + n_txs_diff; //! @TODO: check overflow
      last_n_txs_in_block = header_info.n_txs_in_block;

      // calculate current block ID to fill in next prevID
      if (headers_out.size() != n_blocks)
      {
        if (!calculate_block_hash_from_header_info(header_info, prev_id))
          return false;
      }
    }

    return ::serialization::check_stream_state(ar);
  }
  //--------------------------------------------------------------------------------
  bool decompress_block_header_chain(const std::string &headers_blob,
    std::vector<hashable_block_header_info> &headers_out)
  {
    const epee::span<const unsigned char> byte_span{
      reinterpret_cast<const unsigned char*>(headers_blob.data()), headers_blob.size()};
    return decompress_block_header_chain(byte_span, headers_out);
  }
  //--------------------------------------------------------------------------------
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
