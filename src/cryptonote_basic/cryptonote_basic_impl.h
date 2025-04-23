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

#include "cryptonote_basic.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"


namespace cryptonote {
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

#pragma pack(push, 1)
  struct public_address_outer_blob
  {
    uint8_t m_ver;
    account_public_address m_address;
    uint8_t check_sum;
  };
  struct public_integrated_address_outer_blob
  {
    uint8_t m_ver;
    account_public_address m_address;
    crypto::hash8 payment_id;
    uint8_t check_sum;
  };
#pragma pack (pop)

  namespace
  {
    inline std::string return_first_address(const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)
    {
      if (addresses.empty())
        return {};
      return addresses[0];
    }
  }

  struct address_parse_info
  {
    account_public_address address;
    bool is_subaddress;
    bool has_payment_id;
    crypto::hash8 payment_id;
  };

  /************************************************************************/
  /* Cryptonote helper functions                                          */
  /************************************************************************/
  size_t get_min_block_weight(uint8_t version);
  size_t get_max_tx_size();
  bool get_block_reward(size_t median_weight, size_t current_block_weight, uint64_t already_generated_coins, uint64_t &reward, uint8_t version);
  uint8_t get_account_address_checksum(const public_address_outer_blob& bl);
  uint8_t get_account_integrated_address_checksum(const public_integrated_address_outer_blob& bl);

  std::string get_account_address_as_str(
      network_type nettype
    , bool subaddress
    , const account_public_address& adr
    );

  std::string get_account_integrated_address_as_str(
      network_type nettype
    , const account_public_address& adr
    , const crypto::hash8& payment_id
    );

  bool get_account_address_from_str(
      address_parse_info& info
    , network_type nettype
    , const std::string& str
    );

  bool get_account_address_from_str_or_url(
      address_parse_info& info
    , network_type nettype
    , const std::string& str_or_url
    , std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm = return_first_address
    );

  bool is_coinbase(const transaction_prefix& tx);

  bool operator ==(const cryptonote::transaction& a, const cryptonote::transaction& b);
  bool operator ==(const cryptonote::block& a, const cryptonote::block& b);

  /************************************************************************/
  /* K-anonymity helper functions                                         */
  /************************************************************************/

  /**
   * @brief Compares two hashes up to `nbits` bits in reverse byte order ("LMDB key order")
   *
   * The comparison essentially goes from the 31th, 30th, 29th, ..., 0th byte and compares the MSBs
   * to the LSBs in each byte, up to `nbits` bits. If we use up `nbits` bits before finding a
   * difference in the bits between the two hashes, we return 0. If we encounter a zero bit in `ha`
   * where `hb` has a one in that bit place, then we reutrn -1. If the converse scenario happens,
   * we return a 1. When `nbits` == 256 (there are 256 bits in `crypto::hash`), calling this is
   * functionally identical to `BlockchainLMDB::compare_hash32`.
   *
   * @param ha left hash
   * @param hb right hash
   * @param nbits the number of bits to consider, a higher value means a finer comparison
   * @return int 0 if ha == hb, -1 if ha < hb, 1 if ha > hb
   */
  int compare_hash32_reversed_nbits(const crypto::hash& ha, const crypto::hash& hb, unsigned int nbits);

  /**
   * @brief Make a template which matches `h` in LMDB order up to `nbits` bits, safe for k-anonymous fetching
   *
   * To be more technical, this function creates a hash which satifies the following property:
   *     For all `H_prime` s.t. `0 == compare_hash32_reversed_nbits(real_hash, H_prime, nbits)`,
   *     `1 > compare_hash32_reversed_nbits(real_hash, H_prime, 256)`.
   * In other words, we return the "least" hash nbit-equal to `real_hash`.
   *
   * @param nbits The number of "MSB" bits to include in the template
   * @param real_hash The original hash which contains more information than we want to disclose
   * @return crypto::hash hash template that contains `nbits` bits matching real_hash and no more
   */
  crypto::hash make_hash32_loose_template(unsigned int nbits, const crypto::hash& real_hash);
}

bool parse_hash256(const std::string &str_hash, crypto::hash& hash);

