// Copyright (c) 2026, The Monero Project
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

#include "gtest/gtest.h"

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/tx_sanity_check.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace
{
  cryptonote::blobdata make_tx_blob(const std::vector<uint64_t>& key_offsets)
  {
    cryptonote::transaction tx;

    cryptonote::txin_to_key input{};
    input.amount = 0;
    input.key_offsets = key_offsets;
    tx.vin.push_back(input);

    tx.signatures.resize(1);
    tx.signatures[0].resize(key_offsets.size());

    cryptonote::tx_out output{};
    output.target = cryptonote::txout_to_key{};
    tx.vout.push_back(output);

    cryptonote::blobdata blob;
    EXPECT_TRUE(cryptonote::t_serializable_object_to_blob(tx, blob));
    return blob;
  }
}

TEST(tx_sanity_check, accepts_canonical_key_offsets)
{
  EXPECT_TRUE(cryptonote::tx_sanity_check(make_tx_blob({1, 2, 3}), 100000));
}

TEST(tx_sanity_check, rejects_non_canonical_key_offsets)
{
  const uint64_t max_offset = std::numeric_limits<uint64_t>::max();
  EXPECT_FALSE(cryptonote::tx_sanity_check(make_tx_blob({max_offset, 1}), 100000));
}
