// Copyright (c) 2018-2022, The Monero Project
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

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/serialization/version.hpp>
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include "serialization/crypto.h"
#include "serialization/string.h"
#include "serialization/pair.h"
#include "serialization/containers.h"

namespace cryptonote
{
  class rpc_payment
  {
  public:
    struct client_info
    {
      cryptonote::block block;
      cryptonote::block previous_block;
      cryptonote::blobdata hashing_blob;
      cryptonote::blobdata previous_hashing_blob;
      uint64_t previous_seed_height;
      uint64_t seed_height;
      crypto::hash previous_seed_hash;
      crypto::hash seed_hash;
      uint32_t cookie;
      crypto::hash top;
      crypto::hash previous_top;
      uint64_t credits;
      std::unordered_set<uint64_t> payments;
      std::unordered_set<uint64_t> previous_payments;
      uint64_t update_time;
      uint64_t last_request_timestamp;
      uint64_t block_template_update_time;
      uint64_t credits_total;
      uint64_t credits_used;
      uint64_t nonces_good;
      uint64_t nonces_stale;
      uint64_t nonces_bad;
      uint64_t nonces_dupe;

      client_info();

      template <class t_archive>
      inline void serialize(t_archive &a, const unsigned int ver)
      {
        a & block;
        a & previous_block;
        a & hashing_blob;
        a & previous_hashing_blob;
        a & seed_height;
        a & previous_seed_height;
        a & seed_hash;
        a & previous_seed_hash;
        a & cookie;
        a & top;
        a & previous_top;
        a & credits;
        a & payments;
        a & previous_payments;
        a & update_time;
        a & last_request_timestamp;
        a & block_template_update_time;
        a & credits_total;
        a & credits_used;
        a & nonces_good;
        a & nonces_stale;
        a & nonces_bad;
        a & nonces_dupe;
      }

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(block)
        FIELD(previous_block)
        FIELD(hashing_blob)
        FIELD(previous_hashing_blob)
        VARINT_FIELD(seed_height)
        VARINT_FIELD(previous_seed_height)
        FIELD(seed_hash)
        FIELD(previous_seed_hash)
        VARINT_FIELD(cookie)
        FIELD(top)
        FIELD(previous_top)
        VARINT_FIELD(credits)
        FIELD(payments)
        FIELD(previous_payments)
        FIELD(update_time)
        FIELD(last_request_timestamp)
        FIELD(block_template_update_time)
        VARINT_FIELD(credits_total)
        VARINT_FIELD(credits_used)
        VARINT_FIELD(nonces_good)
        VARINT_FIELD(nonces_stale)
        VARINT_FIELD(nonces_bad)
        VARINT_FIELD(nonces_dupe)
      END_SERIALIZE()
    };

  public:
    rpc_payment(const cryptonote::account_public_address &address, uint64_t diff, uint64_t credits_per_hash_found);
    uint64_t balance(const crypto::public_key &client, int64_t delta = 0);
    bool pay(const crypto::public_key &client, uint64_t ts, uint64_t payment, const std::string &rpc, bool same_ts, uint64_t &credits);
    bool get_info(const crypto::public_key &client, const std::function<bool(const cryptonote::blobdata&, cryptonote::block&, uint64_t &seed_height, crypto::hash &seed_hash)> &get_block_template, cryptonote::blobdata &hashing_blob, uint64_t &seed_height, crypto::hash &seed_hash, const crypto::hash &top, uint64_t &diff, uint64_t &credits_per_hash_found, uint64_t &credits, uint32_t &cookie);
    bool submit_nonce(const crypto::public_key &client, uint32_t nonce, const crypto::hash &top, int64_t &error_code, std::string &error_message, uint64_t &credits, crypto::hash &hash, cryptonote::block &block, uint32_t cookie, bool &stale);
    const cryptonote::account_public_address &get_payment_address() const { return m_address; }
    bool foreach(const std::function<bool(const crypto::public_key &client, const client_info &info)> &f) const;
    unsigned int flush_by_age(time_t seconds = 0);
    uint64_t get_hashes(unsigned int seconds) const;
    void prune_hashrate(unsigned int seconds);
    bool on_idle();

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      a & m_client_info.parent();
      a & m_hashrate.parent();
      a & m_credits_total;
      a & m_credits_used;
      a & m_nonces_good;
      a & m_nonces_stale;
      a & m_nonces_bad;
      a & m_nonces_dupe;
    }

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      FIELD(m_client_info)
      FIELD(m_hashrate)
      VARINT_FIELD(m_credits_total)
      VARINT_FIELD(m_credits_used)
      VARINT_FIELD(m_nonces_good)
      VARINT_FIELD(m_nonces_stale)
      VARINT_FIELD(m_nonces_bad)
      VARINT_FIELD(m_nonces_dupe)
    END_SERIALIZE()

    bool load(std::string directory);
    bool store(const std::string &directory = std::string()) const;

  private:
    cryptonote::account_public_address m_address;
    uint64_t m_diff;
    uint64_t m_credits_per_hash_found;
    serializable_unordered_map<crypto::public_key, client_info> m_client_info;
    std::string m_directory;
    serializable_map<uint64_t, uint64_t> m_hashrate;
    uint64_t m_credits_total;
    uint64_t m_credits_used;
    uint64_t m_nonces_good;
    uint64_t m_nonces_stale;
    uint64_t m_nonces_bad;
    uint64_t m_nonces_dupe;
    mutable boost::mutex mutex;
  };
}
