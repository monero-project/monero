// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "cryptonote_basic.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"


namespace cryptonote {
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_array>
  struct array_hasher: std::unary_function<t_array&, std::size_t>
  {
    std::size_t operator()(const t_array& val) const
    {
      return boost::hash_range(&val.data[0], &val.data[sizeof(val.data)]);
    }
  };


#pragma pack(push, 1)
  struct public_address_outer_blob
  {
    uint8_t m_ver;
    account_public_address m_address;
    uint8_t check_sum;
  };
#pragma pack (pop)


  /************************************************************************/
  /* Cryptonote helper functions                                          */
  /************************************************************************/
  size_t get_max_block_size();
  size_t get_max_tx_size();
  bool get_block_reward(size_t median_size, size_t current_block_size, uint64_t already_generated_coins, uint64_t &reward);
  uint8_t get_account_address_checksum(const public_address_outer_blob& bl);
  std::string get_account_address_as_str(const account_public_address& adr);
  bool get_account_address_from_str(account_public_address& adr, const std::string& str);
  bool is_coinbase(const transaction& tx);

  bool operator ==(const cryptonote::transaction& a, const cryptonote::transaction& b);
  bool operator ==(const cryptonote::block& a, const cryptonote::block& b);
}

template <class T>
std::ostream &print256(std::ostream &o, const T &v) {
  return o << "<" << epee::string_tools::pod_to_hex(v) << ">";
}

bool parse_hash256(const std::string str_hash, crypto::hash& hash);

namespace crypto {
  inline std::ostream &operator <<(std::ostream &o, const crypto::public_key &v) { return print256(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::secret_key &v) { return print256(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_derivation &v) { return print256(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_image &v) { return print256(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::signature &v) { return print256(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::hash &v) { return print256(o, v); }
}
