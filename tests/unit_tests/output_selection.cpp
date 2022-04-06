// Copyright (c) 2014-2022, The Monero Project
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

#define MKOFFSETS(N, n) \
  offsets.resize(N); \
  size_t n_outs = 0; \
  for (auto &offset: offsets) \
  { \
    offset = n_outs += (n); \
  }

TEST(select_outputs, gamma)
{
  std::vector<uint64_t> offsets;

  MKOFFSETS(300000, 1);
  tools::gamma_picker picker(offsets);
  std::vector<double> ages(100000);
  double age_scale = 120. * (offsets.size() / (double)n_outs);
  for (size_t i = 0; i < ages.size(); )
  {
    uint64_t o = picker.pick();
    if (o >= n_outs)
      continue;
    ages[i] = (n_outs - 1 - o) * age_scale;
    ASSERT_GE(ages[i], 0);
    ASSERT_LE(ages[i], offsets.size() * 120);
    ++i;
  }
  double median = epee::misc_utils::median(ages);
  MDEBUG("median age: " << median / 86400. << " days");
  ASSERT_GE(median, 1.3 * 86400);
  ASSERT_LE(median, 1.4 * 86400);
}

TEST(select_outputs, density)
{
  static const size_t NPICKS = 1000000;
  std::vector<uint64_t> offsets;

  MKOFFSETS(300000, 1 + (crypto::rand<size_t>() & 0x1f));
  tools::gamma_picker picker(offsets);

  std::vector<int> picks(/*n_outs*/offsets.size(), 0);
  for (int i = 0; i < NPICKS; )
  {
    uint64_t o = picker.pick();
    if (o >= n_outs)
      continue;
    auto it = std::lower_bound(offsets.begin(), offsets.end(), o);
    auto idx = std::distance(offsets.begin(), it);
    ASSERT_LT(idx, picks.size());
    ++picks[idx];
    ++i;
  }

  for (int d = 1; d < 0x20; ++d)
  {
    // count the number of times an output in a block of d outputs was selected
    // count how many outputs are in a block of d outputs
    size_t count_selected = 0, count_chain = 0;
    for (size_t i = 0; i < offsets.size(); ++i)
    {
      size_t n_outputs = offsets[i] - (i == 0 ? 0 : offsets[i - 1]);
      if (n_outputs == d)
      {
        count_selected += picks[i];
        count_chain += d;
      }
    }
    float selected_ratio = count_selected / (float)NPICKS;
    float chain_ratio = count_chain / (float)n_outs;
    MDEBUG(count_selected << "/" << NPICKS << " outputs selected in blocks of density " << d << ", " << 100.0f * selected_ratio << "%");
    MDEBUG(count_chain << "/" << offsets.size() << " outputs in blocks of density " << d << ", " << 100.0f * chain_ratio << "%");
    ASSERT_LT(fabsf(selected_ratio - chain_ratio), 0.025f);
  }
}

TEST(select_outputs, same_distribution)
{
  static const size_t NPICKS = 1000000;
  std::vector<uint64_t> offsets;

  MKOFFSETS(300000, 1 + (crypto::rand<size_t>() & 0x1f));
  tools::gamma_picker picker(offsets);

  std::vector<int> chain_picks(offsets.size(), 0);
  std::vector<int> output_picks(n_outs, 0);
  for (int i = 0; i < NPICKS; )
  {
    uint64_t o = picker.pick();
    if (o >= n_outs)
      continue;
    auto it = std::lower_bound(offsets.begin(), offsets.end(), o);
    auto idx = std::distance(offsets.begin(), it);
    ASSERT_LT(idx, chain_picks.size());
    ++chain_picks[idx];
    ++output_picks[o];
    ++i;
  }

  // scale them both to 0-100
  std::vector<int> chain_norm(100, 0), output_norm(100, 0);
  for (size_t i = 0; i < output_picks.size(); ++i)
    output_norm[i * 100 / output_picks.size()] += output_picks[i];
  for (size_t i = 0; i < chain_picks.size(); ++i)
    chain_norm[i * 100 / chain_picks.size()] += chain_picks[i];

  double avg_dev = 0.0;
  for (size_t i = 0; i < 100; ++i)
  {
    const double diff = (double)output_norm[i] - (double)chain_norm[i];
    double dev = fabs(2.0 * diff / (output_norm[i] + chain_norm[i]));
    ASSERT_LT(dev, 0.15);
    avg_dev += dev;
  }
  avg_dev /= 100;
  MDEBUG("avg_dev: " << avg_dev);
  ASSERT_LT(avg_dev, 0.02);
}
