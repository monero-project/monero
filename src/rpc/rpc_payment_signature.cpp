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

#include <cinttypes>
#include <stdlib.h>
#include <chrono>
#include "include_base_utils.h"
#include "string_tools.h"
#include "rpc_payment_signature.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc.payment"

#define TIMESTAMP_LEEWAY (60 * 1000000) /* 60 seconds, in microseconds */

namespace cryptonote
{
  std::string make_rpc_payment_signature(const crypto::secret_key &skey)
  {
    std::string s;
    crypto::public_key pkey;
    crypto::secret_key_to_public_key(skey, pkey);
    crypto::signature sig;
    const uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    char ts[17];
    int ret = snprintf(ts, sizeof(ts), "%16.16" PRIx64, now);
    CHECK_AND_ASSERT_MES(ret == 16, "", "snprintf failed");
    ts[16] = 0;
    CHECK_AND_ASSERT_MES(strlen(ts) == 16, "", "Invalid time conversion");
    crypto::hash hash;
    crypto::cn_fast_hash(ts, 16, hash);
    crypto::generate_signature(hash, pkey, skey, sig);
    s = epee::string_tools::pod_to_hex(pkey) + ts + epee::string_tools::pod_to_hex(sig);
    return s;
  }

  bool verify_rpc_payment_signature(const std::string &message, crypto::public_key &pkey, uint64_t &ts)
  {
    if (message.size() != 2 * sizeof(crypto::public_key) + 16 + 2 * sizeof(crypto::signature))
    {
      MDEBUG("Bad message size: " << message.size());
      return false;
    }
    const std::string pkey_string = message.substr(0, 2 * sizeof(crypto::public_key));
    const std::string ts_string = message.substr(2 * sizeof(crypto::public_key), 16);
    const std::string signature_string = message.substr(2 * sizeof(crypto::public_key) + 16);
    if (!epee::string_tools::hex_to_pod(pkey_string, pkey))
    {
      MDEBUG("Bad client id");
      return false;
    }
    crypto::signature signature;
    if (!epee::string_tools::hex_to_pod(signature_string, signature))
    {
      MDEBUG("Bad signature");
      return false;
    }
    crypto::hash hash;
    crypto::cn_fast_hash(ts_string.data(), 16, hash);
    if (!crypto::check_signature(hash, pkey, signature))
    {
      MDEBUG("signature does not verify");
      return false;
    }
    char *endptr = NULL;
    errno = 0;
    unsigned long long ull = strtoull(ts_string.c_str(), &endptr, 16);
    if (ull == ULLONG_MAX && errno == ERANGE)
    {
      MDEBUG("bad timestamp");
      return false;
    }
    ts = ull;
    const uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (ts > now + TIMESTAMP_LEEWAY)
    {
      MDEBUG("Timestamp is in the future");
      return false;
    }
    if (ts < now - TIMESTAMP_LEEWAY)
    {
      MDEBUG("Timestamp is too old");
      return false;
    }
    return true;
  }
}
