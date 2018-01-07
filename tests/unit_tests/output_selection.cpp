// Copyright (c) 2014-2018, The Monero Project
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

// FIXME: move this into a full wallet2 unit test suite, if possible

#include "gtest/gtest.h"

#include "wallet/wallet2.h"
#include <string>

static tools::wallet2::transfer_container make_transfers_container(size_t N)
{
  tools::wallet2::transfer_container transfers;
  for (size_t n = 0; n < N; ++n)
  {
    transfers.push_back(AUTO_VAL_INIT(tools::wallet2::transfer_details()));
    tools::wallet2::transfer_details &td = transfers.back();
    td.m_block_height = 1000;
    td.m_spent = false;
    td.m_txid = crypto::null_hash;
    td.m_txid.data[0] = n & 0xff;
    td.m_txid.data[1] = (n >> 8) & 0xff;
    td.m_txid.data[2] = (n >> 16) & 0xff;
    td.m_txid.data[3] = (n >> 24) & 0xff;
  }
  return transfers;
}

#define SELECT(idx) \
  do { \
    auto i = std::find(unused_indices.begin(), unused_indices.end(), idx); \
    ASSERT_TRUE(i != unused_indices.end()); \
    unused_indices.erase(i); \
    selected.push_back(idx); \
  } while(0)

#define PICK(expected) \
  do { \
    size_t idx = w.pop_best_value_from(transfers, unused_indices, selected); \
    ASSERT_EQ(expected, idx); \
    selected.push_back(idx); \
  } while(0)

TEST(select_outputs, one_out_of_N)
{
  tools::wallet2 w;

  // check that if there are N-1 outputs of the same height, one of them
  // already selected, the next one selected is the one that's from a
  // different height
  tools::wallet2::transfer_container transfers = make_transfers_container(10);
  transfers[6].m_block_height = 700;
  std::vector<size_t> unused_indices({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  std::vector<size_t> selected;
  SELECT(2);
  PICK(6);
}

TEST(select_outputs, order)
{
  tools::wallet2 w;

  // check that most unrelated heights are picked in order
  tools::wallet2::transfer_container transfers = make_transfers_container(5);
  transfers[0].m_block_height = 700;
  transfers[1].m_block_height = 700;
  transfers[2].m_block_height = 704;
  transfers[3].m_block_height = 716;
  transfers[4].m_block_height = 701;
  std::vector<size_t> unused_indices({0, 1, 2, 3, 4});
  std::vector<size_t> selected;
  SELECT(0);
  PICK(3); // first the one that's far away
  PICK(2); // then the one that's close
  PICK(4); // then the one that's adjacent
  PICK(1); // then the one that's on the same height
}

