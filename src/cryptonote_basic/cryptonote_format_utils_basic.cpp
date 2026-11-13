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

#include "cryptonote_format_utils.h"

namespace cryptonote
{
  void calculate_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h)
  {
    std::ostringstream s;
    binary_archive<true> a(s);
    ::serialization::serialize(a, const_cast<transaction_prefix&>(tx));
    const std::string blob = s.str();
    crypto::cn_fast_hash(blob.data(), blob.size(), h);
  }

  crypto::hash calculate_transaction_prefix_hash(const transaction_prefix& tx)
  {
    crypto::hash h = crypto::null_hash;
    calculate_transaction_prefix_hash(tx, h);
    return h;
  }

  void get_transaction_prefix_hash(const transaction& tx, crypto::hash& h)
  {
    // the prefix hash is only cached alongside a valid tx hash, so code which modifies a tx must
    // already call invalidate_hashes(), and a stale prefix hash can't outlive a stale tx hash
    if (tx.is_hash_valid() && tx.is_prefix_hash_valid())
    {
#ifdef ENABLE_HASH_CASH_INTEGRITY_CHECK
      calculate_transaction_prefix_hash(tx, h);
      CHECK_AND_ASSERT_THROW_MES(tx.prefix_hash == h, "tx prefix hash cash integrity failure");
#endif
      h = tx.prefix_hash;
      return;
    }

    calculate_transaction_prefix_hash(tx, h);
    if (tx.is_hash_valid())
      tx.set_prefix_hash(h);
  }

  crypto::hash get_transaction_prefix_hash(const transaction& tx)
  {
    crypto::hash h = crypto::null_hash;
    get_transaction_prefix_hash(tx, h);
    return h;
  }
}
