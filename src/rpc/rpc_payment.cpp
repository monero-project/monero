// Copyright (c) 2018-2019, The Monero Project
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

#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include "cryptonote_config.h"
#include "include_base_utils.h"
#include "string_tools.h"
#include "file_io_utils.h"
#include "int-util.h"
#include "common/util.h"
#include "serialization/crypto.h"
#include "common/unordered_containers_boost_serialization.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/difficulty.h"
#include "core_rpc_server_error_codes.h"
#include "rpc_payment.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc.payment"

#define STALE_THRESHOLD 15 /* seconds */

#define PENALTY_FOR_STALE 0
#define PENALTY_FOR_BAD_HASH 20
#define PENALTY_FOR_DUPLICATE 20

#define DEFAULT_FLUSH_AGE (3600 * 24 * 180) // half a year
#define DEFAULT_ZERO_FLUSH_AGE (60 * 2) // 2 minutes

#define RPC_PAYMENT_NONCE_TAIL 0x58

namespace cryptonote
{
  rpc_payment::client_info::client_info():
    previous_seed_height(0),
    seed_height(0),
    previous_seed_hash(crypto::null_hash),
    seed_hash(crypto::null_hash),
    cookie(0),
    top(crypto::null_hash),
    previous_top(crypto::null_hash),
    credits(0),
    update_time(time(NULL)),
    last_request_timestamp(0),
    block_template_update_time(0),
    credits_total(0),
    credits_used(0),
    nonces_good(0),
    nonces_stale(0),
    nonces_bad(0),
    nonces_dupe(0)
  {
  }

  rpc_payment::rpc_payment(const cryptonote::account_public_address &address, uint64_t diff, uint64_t credits_per_hash_found):
    m_address(address),
    m_diff(diff),
    m_credits_per_hash_found(credits_per_hash_found),
    m_credits_total(0),
    m_credits_used(0),
    m_nonces_good(0),
    m_nonces_stale(0),
    m_nonces_bad(0),
    m_nonces_dupe(0)
  {
  }

  uint64_t rpc_payment::balance(const crypto::public_key &client, int64_t delta)
  {
    client_info &info = m_client_info[client]; // creates if not found
    uint64_t credits = info.credits;
    if (delta > 0 && credits > std::numeric_limits<uint64_t>::max() - delta)
      credits = std::numeric_limits<uint64_t>::max();
    else if (delta < 0 && credits < (uint64_t)-delta)
      credits = 0;
    else
      credits += delta;
    if (delta)
      MINFO("Client " << client << ": balance change from " << info.credits << " to " << credits);
    return info.credits = credits;
  }

  bool rpc_payment::pay(const crypto::public_key &client, uint64_t ts, uint64_t payment, const std::string &rpc, bool same_ts, uint64_t &credits)
  {
    client_info &info = m_client_info[client]; // creates if not found
    if (ts < info.last_request_timestamp || (ts == info.last_request_timestamp && !same_ts))
    {
      MDEBUG("Invalid ts: " << ts << " <= " << info.last_request_timestamp);
      return false;
    }
    info.last_request_timestamp = ts;
    if (info.credits < payment)
    {
      MDEBUG("Not enough credits: " << info.credits << " < " << payment);
      credits = info.credits;
      return false;
    }
    info.credits -= payment;
    add64clamp(&info.credits_used, payment);
    add64clamp(&m_credits_used, payment);
    MDEBUG("client " << client << " paying " << payment << " for " << rpc << ", " << info.credits << " left");
    credits = info.credits;
    return true;
  }

  bool rpc_payment::get_info(const crypto::public_key &client, const std::function<bool(const cryptonote::blobdata&, cryptonote::block&, uint64_t &seed_height, crypto::hash &seed_hash)> &get_block_template, cryptonote::blobdata &hashing_blob, uint64_t &seed_height, crypto::hash &seed_hash, const crypto::hash &top, uint64_t &diff, uint64_t &credits_per_hash_found, uint64_t &credits, uint32_t &cookie)
  {
    client_info &info = m_client_info[client]; // creates if not found
    const uint64_t now = time(NULL);
    bool need_template = top != info.top || now >= info.block_template_update_time + STALE_THRESHOLD;
    if (need_template)
    {
      cryptonote::block new_block;
      uint64_t new_seed_height;
      crypto::hash new_seed_hash;
      cryptonote::blobdata extra_nonce("\x42\x42\x42\x42", 4);
      if (!get_block_template(extra_nonce, new_block, new_seed_height, new_seed_hash))
        return false;
      if(!remove_field_from_tx_extra(new_block.miner_tx.extra, typeid(cryptonote::tx_extra_nonce)))
        return false;
      char data[33];
      memcpy(data, &client, 32);
      data[32] = RPC_PAYMENT_NONCE_TAIL;
      crypto::hash hash;
      cn_fast_hash(data, sizeof(data), hash);
      extra_nonce = cryptonote::blobdata((const char*)&hash, 4);
      if(!add_extra_nonce_to_tx_extra(new_block.miner_tx.extra, extra_nonce))
        return false;
      info.previous_block = std::move(info.block);
      info.block = std::move(new_block);
      hashing_blob = get_block_hashing_blob(info.block);
      info.previous_hashing_blob = info.hashing_blob;
      info.hashing_blob = hashing_blob;
      info.previous_top = info.top;
      info.previous_seed_height = info.seed_height;
      info.seed_height = new_seed_height;
      info.previous_seed_hash = info.seed_hash;
      info.seed_hash = new_seed_hash;
      std::swap(info.previous_payments, info.payments);
      info.payments.clear();
      ++info.cookie;
      info.block_template_update_time = now;
    }
    info.top = top;
    info.update_time = now;
    hashing_blob = info.hashing_blob;
    diff = m_diff;
    credits_per_hash_found = m_credits_per_hash_found;
    credits = info.credits;
    seed_height = info.seed_height;
    seed_hash = info.seed_hash;
    cookie = info.cookie;
    return true;
  }

  bool rpc_payment::submit_nonce(const crypto::public_key &client, uint32_t nonce, const crypto::hash &top, int64_t &error_code, std::string &error_message, uint64_t &credits, crypto::hash &hash, cryptonote::block &block, uint32_t cookie, bool &stale)
  {
    client_info &info = m_client_info[client]; // creates if not found
    if (cookie != info.cookie && cookie != info.cookie - 1)
    {
      MWARNING("Very stale nonce");
      ++m_nonces_stale;
      ++info.nonces_stale;
      sub64clamp(&info.credits, PENALTY_FOR_STALE * m_credits_per_hash_found);
      error_code = CORE_RPC_ERROR_CODE_STALE_PAYMENT;
      error_message = "Very stale payment";
      return false;
    }
    const bool is_current = cookie == info.cookie;
    MINFO("client " << client << " sends nonce: " << nonce << ", " << (is_current ? "current" : "stale"));
    std::unordered_set<uint64_t> &payments = is_current ? info.payments : info.previous_payments;
    if (!payments.insert(nonce).second)
    {
      MWARNING("Duplicate nonce " << nonce << " from " << (is_current ? "current" : "previous"));
      ++m_nonces_dupe;
      ++info.nonces_dupe;
      sub64clamp(&info.credits, PENALTY_FOR_DUPLICATE * m_credits_per_hash_found);
      error_code = CORE_RPC_ERROR_CODE_DUPLICATE_PAYMENT;
      error_message = "Duplicate payment";
      return false;
    }

    const uint64_t now = time(NULL);
    if (!is_current)
    {
      if (now > info.update_time + STALE_THRESHOLD)
      {
        MWARNING("Nonce is stale (top " << top << ", should be " << info.top << " or within " << STALE_THRESHOLD << " seconds");
        ++m_nonces_stale;
        ++info.nonces_stale;
        sub64clamp(&info.credits, PENALTY_FOR_STALE * m_credits_per_hash_found);
        error_code = CORE_RPC_ERROR_CODE_STALE_PAYMENT;
        error_message = "stale payment";
        return false;
      }
    }

    cryptonote::blobdata hashing_blob = is_current ? info.hashing_blob : info.previous_hashing_blob;
    if (hashing_blob.size() < 43)
    {
      // not initialized ?
      error_code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_message = "not initialized";
      return false;
    }

    block = is_current ? info.block : info.previous_block;
    *(uint32_t*)(hashing_blob.data() + 39) = SWAP32LE(nonce);
    if (block.major_version >= RX_BLOCK_VERSION)
    {
      const uint64_t seed_height = is_current ? info.seed_height : info.previous_seed_height;
      const crypto::hash &seed_hash = is_current ? info.seed_hash : info.previous_seed_hash;
      const uint64_t height = cryptonote::get_block_height(block);
      crypto::rx_slow_hash(height, seed_height, seed_hash.data, hashing_blob.data(), hashing_blob.size(), hash.data, 0, 0);
    }
    else
    {
      const int cn_variant = hashing_blob[0] >= 7 ? hashing_blob[0] - 6 : 0;
      crypto::cn_slow_hash(hashing_blob.data(), hashing_blob.size(), hash, cn_variant, cryptonote::get_block_height(block));
    }
    if (!check_hash(hash, m_diff))
    {
      MWARNING("Payment too low");
      ++m_nonces_bad;
      ++info.nonces_bad;
      error_code = CORE_RPC_ERROR_CODE_PAYMENT_TOO_LOW;
      error_message = "Hash does not meet difficulty (could be wrong PoW hash, or mining at lower difficulty than required, or attempt to defraud)";
      sub64clamp(&info.credits, PENALTY_FOR_BAD_HASH * m_credits_per_hash_found);
      return false;
    }

    add64clamp(&info.credits, m_credits_per_hash_found);
    MINFO("client " << client << " credited for " << m_credits_per_hash_found << ", now " << info.credits << (is_current ? "" : " (close)"));

    m_hashrate[now] += m_diff;
    add64clamp(&m_credits_total, m_credits_per_hash_found);
    add64clamp(&info.credits_total, m_credits_per_hash_found);
    ++m_nonces_good;
    ++info.nonces_good;

    credits = info.credits;
    block = info.block;
    block.nonce = nonce;
    stale = !is_current;
    return true;
  }

  bool rpc_payment::foreach(const std::function<bool(const crypto::public_key &client, const client_info &info)> &f) const
  {
    for (std::unordered_map<crypto::public_key, client_info>::const_iterator i = m_client_info.begin(); i != m_client_info.end(); ++i)
    {
      if (!f(i->first, i->second))
        return false;
    }
    return true;
  }

  bool rpc_payment::load(std::string directory)
  {
    TRY_ENTRY();
    m_directory = std::move(directory);
    std::string state_file_path = directory + "/" + RPC_PAYMENTS_DATA_FILENAME;
    MINFO("loading rpc payments data from " << state_file_path);
    std::ifstream data;
    data.open(state_file_path, std::ios_base::binary | std::ios_base::in);
    if (!data.fail())
    {
      try
      {
        boost::archive::portable_binary_iarchive a(data);
        a >> *this;
      }
      catch (const std::exception &e)
      {
        MERROR("Failed to load RPC payments file: " << e.what());
        m_client_info.clear();
      }
    }
    else
    {
      m_client_info.clear();
    }

    CATCH_ENTRY_L0("rpc_payment::load", false);
    return true;
  }

  bool rpc_payment::store(const std::string &directory_) const
  {
    TRY_ENTRY();
    const std::string &directory = directory_.empty() ? m_directory : directory_;
    MDEBUG("storing rpc payments data to " << directory);
    if (!tools::create_directories_if_necessary(directory))
    {
      MWARNING("Failed to create data directory: " << directory);
      return false;
    }
    const boost::filesystem::path state_file_path = (boost::filesystem::path(directory) / RPC_PAYMENTS_DATA_FILENAME);
    if (boost::filesystem::exists(state_file_path))
    {
      std::string state_file_path_old = state_file_path.string() + ".old";
      boost::system::error_code ec;
      boost::filesystem::remove(state_file_path_old, ec);
      std::error_code e = tools::replace_file(state_file_path.string(), state_file_path_old);
      if (e)
        MWARNING("Failed to rename " << state_file_path << " to " << state_file_path_old << ": " << e);
    }
    std::ofstream data;
    data.open(state_file_path.string(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
    if (data.fail())
    {
      MWARNING("Failed to save RPC payments to file " << state_file_path);
      return false;
    };
    boost::archive::portable_binary_oarchive a(data);
    a << *this;
    return true;
    CATCH_ENTRY_L0("rpc_payment::store", false);
  }

  unsigned int rpc_payment::flush_by_age(time_t seconds)
  {
    unsigned int count = 0;
    const time_t now = time(NULL);
    time_t seconds0 = seconds;
    if (seconds == 0)
    {
      seconds = DEFAULT_FLUSH_AGE;
      seconds0 = DEFAULT_ZERO_FLUSH_AGE;
    }
    const time_t threshold = seconds > now ? 0 : now - seconds;
    const time_t threshold0 = seconds0 > now ? 0 : now - seconds0;
    for (std::unordered_map<crypto::public_key, client_info>::iterator i = m_client_info.begin(); i != m_client_info.end(); )
    {
      std::unordered_map<crypto::public_key, client_info>::iterator j = i++;
      const time_t t = std::max(j->second.last_request_timestamp, j->second.update_time);
      const bool erase = t < ((j->second.credits == 0) ? threshold0 : threshold);
      if (erase)
      {
        MINFO("Erasing " << j->first << " with " << j->second.credits << " credits, inactive for " << (now-t)/86400 << " days");
        m_client_info.erase(j);
        ++count;
      }
    }
    return count;
  }

  uint64_t rpc_payment::get_hashes(unsigned int seconds) const
  {
    const uint64_t now = time(NULL);
    uint64_t hashes = 0;
    for (std::map<uint64_t, uint64_t>::const_reverse_iterator i = m_hashrate.crbegin(); i != m_hashrate.crend(); ++i)
    {
      if (now > i->first + seconds)
        break;
      hashes += i->second;
    }
    return hashes;
  }

  void rpc_payment::prune_hashrate(unsigned int seconds)
  {
    const uint64_t now = time(NULL);
    std::map<uint64_t, uint64_t>::iterator i;
    for (i = m_hashrate.begin(); i != m_hashrate.end(); ++i)
    {
      if (now <= i->first + seconds)
        break;
    }
    m_hashrate.erase(m_hashrate.begin(), i);
  }

  bool rpc_payment::on_idle()
  {
    flush_by_age();
    prune_hashrate(3600);
    return true;
  }
}
