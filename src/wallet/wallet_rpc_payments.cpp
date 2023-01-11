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

#include <boost/optional/optional.hpp>
#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
#include "cryptonote_config.h"
#include "wallet_rpc_helpers.h"
#include "wallet2.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "rpc/rpc_payment_signature.h"
#include "misc_language.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "int-util.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/blobdatatype.h"
#include "common/i18n.h"
#include "common/util.h"
#include "common/threadpool.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.wallet2.rpc_payments"

#define RPC_PAYMENT_POLL_PERIOD 10 /* seconds*/

namespace tools
{
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_client_signature() const
{
  return cryptonote::make_rpc_payment_signature(m_rpc_client_secret_key);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_rpc_payment_info(bool mining, bool &payment_required, uint64_t &credits, uint64_t &diff, uint64_t &credits_per_hash_found, cryptonote::blobdata &hashing_blob, uint64_t &height, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, uint32_t &cookie)
{
  boost::optional<std::string> result = m_node_rpc_proxy.get_rpc_payment_info(mining, payment_required, credits, diff, credits_per_hash_found, hashing_blob, height, seed_height, seed_hash, next_seed_hash, cookie);
  credits = m_rpc_payment_state.credits;
  if (result && *result != CORE_RPC_STATUS_OK)
    return false;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::daemon_requires_payment()
{
  bool payment_required = false;
  uint64_t credits, diff, credits_per_hash_found, height, seed_height;
  uint32_t cookie;
  cryptonote::blobdata blob;
  crypto::hash seed_hash, next_seed_hash;
  return get_rpc_payment_info(false, payment_required, credits, diff, credits_per_hash_found, blob, height, seed_height, seed_hash, next_seed_hash, cookie) && payment_required;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::make_rpc_payment(uint32_t nonce, uint32_t cookie, uint64_t &credits, uint64_t &balance)
{
  cryptonote::COMMAND_RPC_ACCESS_SUBMIT_NONCE::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_ACCESS_SUBMIT_NONCE::response res = AUTO_VAL_INIT(res);
  req.nonce = nonce;
  req.cookie = cookie;
  m_daemon_rpc_mutex.lock();
  uint64_t pre_call_credits = m_rpc_payment_state.credits;
  req.client = get_client_signature();
  epee::json_rpc::error error;
  bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", "rpc_access_submit_nonce", req, res, error, *m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_ON_RPC_RESPONSE_ERROR_GENERIC(r, error, res, "rpc_access_submit_nonce");
  THROW_WALLET_EXCEPTION_IF(res.credits < pre_call_credits, error::wallet_internal_error, "RPC payment did not increase balance");
  if (m_rpc_payment_state.top_hash != res.top_hash)
  {
    m_rpc_payment_state.top_hash = res.top_hash;
    m_rpc_payment_state.stale = true;
  }

  m_rpc_payment_state.credits = res.credits;
  balance = res.credits;
  credits = balance - pre_call_credits;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::search_for_rpc_payment(uint64_t credits_target, uint32_t n_threads, const std::function<bool(uint64_t, uint64_t)> &startfunc, const std::function<bool(unsigned)> &contfunc, const std::function<bool(uint64_t)> &foundfunc, const std::function<void(const std::string&)> &errorfunc)
{
  bool need_payment = false;
  bool payment_required;
  uint64_t credits, diff, credits_per_hash_found, height, seed_height;
  uint32_t cookie;
  unsigned int n_hashes = 0;
  cryptonote::blobdata hashing_blob;
  crypto::hash seed_hash, next_seed_hash;
  try
  {
    need_payment = get_rpc_payment_info(false, payment_required, credits, diff, credits_per_hash_found, hashing_blob, height, seed_height, seed_hash, next_seed_hash, cookie) && payment_required && credits < credits_target;
    if (!need_payment)
      return true;
    if (!startfunc(diff, credits_per_hash_found))
      return true;
  }
  catch (const std::exception &e) { return false; }

  static std::atomic<uint32_t> nonce(0);
  while (contfunc(n_hashes))
  {
    try
    {
      need_payment = get_rpc_payment_info(true, payment_required, credits, diff, credits_per_hash_found, hashing_blob, height, seed_height, seed_hash, next_seed_hash, cookie) && payment_required && credits < credits_target;
      if (!need_payment)
        return true;
    }
    catch (const std::exception &e) { return false; }
    if (hashing_blob.empty())
    {
      MERROR("Bad hashing blob from daemon");
      if (errorfunc)
        errorfunc("Bad hashing blob from daemon, trying again");
      epee::misc_utils::sleep_no_w(1000);
      continue;
    }

    if(n_threads == 0)
      n_threads = boost::thread::hardware_concurrency();

    std::vector<crypto::hash> hash(n_threads);
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    const uint32_t local_nonce = nonce += n_threads; // wrapping's OK
    for (size_t i = 0; i < n_threads; i++)
    {
      tpool.submit(&waiter, [&, i] {
        *(uint32_t*)(hashing_blob.data() + 39) = SWAP32LE(local_nonce-i);
        const uint8_t major_version = hashing_blob[0];
        if (major_version >= RX_BLOCK_VERSION)
        {
          crypto::rx_slow_hash(seed_hash.data, hashing_blob.data(), hashing_blob.size(), hash[i].data);
        }
        else
        {
          int cn_variant = hashing_blob[0] >= 7 ? hashing_blob[0] - 6 : 0;
          crypto::cn_slow_hash(hashing_blob.data(), hashing_blob.size(), hash[i], cn_variant, height);
        }
      });
    }
    waiter.wait();
    n_hashes += n_threads;

    for(size_t i=0; i < n_threads; i++)
    {
      if (cryptonote::check_hash(hash[i], diff))
      {
        uint64_t credits, balance;
        try
        {
          make_rpc_payment(local_nonce-i, cookie, credits, balance);
          if (credits != credits_per_hash_found)
          {
            MERROR("Found nonce, but daemon did not credit us with the expected amount");
            if (errorfunc)
              errorfunc("Found nonce, but daemon did not credit us with the expected amount");
            return false;
          }
          MDEBUG("Found nonce " << local_nonce-i << " at diff " << diff << ", gets us " << credits_per_hash_found << ", now " << balance << " credits");
          if (!foundfunc(credits))
            break;
        }
        catch (const tools::error::wallet_coded_rpc_error &e)
        {
          MWARNING("Found a local_nonce at diff " << diff << ", but failed to send it to the daemon");
          if (errorfunc)
            errorfunc("Found nonce, but daemon errored out with error " + std::to_string(e.code()) + ": " + e.status() + ", continuing");
        }
        catch (const std::exception &e)
        {
          MWARNING("Found a local_nonce at diff " << diff << ", but failed to send it to the daemon");
          if (errorfunc)
            errorfunc("Found nonce, but daemon errored out with: '" + std::string(e.what()) + "', continuing");
        }
      }
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_rpc_cost(const char *call, uint64_t post_call_credits, uint64_t pre_call_credits, double expected_cost)
{
  return tools::check_rpc_cost(m_rpc_payment_state, call, post_call_credits, pre_call_credits, expected_cost);
}
//----------------------------------------------------------------------------------------------------
}
