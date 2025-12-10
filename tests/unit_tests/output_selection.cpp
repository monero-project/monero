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

void pick_outputs(size_t npicks, std::vector<uint64_t> offsets, size_t n_outs, tools::gamma_picker picker, double &median)
{
  std::vector<double> ages(npicks);
  double age_scale = 120. * (offsets.size() / (double)n_outs);
  for (size_t i = 0; i < ages.size(); )
  {
    uint64_t o = picker.pick();
    bool acceptable_bad_pick = o == std::numeric_limits<uint64_t>::max();
    if (acceptable_bad_pick)
      continue;
    ages[i] = (n_outs - 1 - o) * age_scale;
    ASSERT_GE(ages[i], CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * 120);
    ASSERT_LE(ages[i], offsets.size() * 120);
    ++i;
  }
  median = epee::misc_utils::median(ages);
}

// TODO: deterministic tests would be nice
TEST(select_outputs, gamma)
{
  std::vector<uint64_t> offsets;

  MKOFFSETS(300000, 1);
  tools::gamma_picker picker(offsets);

  size_t NPICKS = 100000;
  double median = 0;
  pick_outputs(NPICKS, offsets, n_outs, picker, median);

  // expected median is ~115,100s, or 1.33 * 86400. Calculated by running the algorithm
  // under the same conditions as the paper (outputs <10 blocks old can be selected, and
  // are selected from chain tip)
  double MEDIAN_LOWER_BOUND = 1.28 * 86400;
  double MEDIAN_UPPER_BOUND = 1.38 * 86400;

  // should be rare, but if needed, 10x NPICKS and try again to get a more accurate median
  if (median < MEDIAN_LOWER_BOUND || median > MEDIAN_UPPER_BOUND)
    pick_outputs(NPICKS * 10, offsets, n_outs, picker, median);

  ASSERT_GE(median, MEDIAN_LOWER_BOUND);
  ASSERT_LE(median, MEDIAN_UPPER_BOUND);
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
    if (o == std::numeric_limits<uint64_t>::max())
      continue;
    auto it = std::lower_bound(offsets.begin(), offsets.end(), o);
    auto idx = std::distance(offsets.begin(), it);
    ASSERT_LT(idx, offsets.size() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
    ++picks[idx];
    ++i;
  }

  std::vector<float> selected_ratios;
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
    ASSERT_GT(selected_ratio, 0.0f);
    ASSERT_GT(chain_ratio, 0.0f);

    float MAX_DEVIATION = 2.25f;
    ASSERT_GT(MAX_DEVIATION * selected_ratio, chain_ratio);
    ASSERT_GT(MAX_DEVIATION * chain_ratio, selected_ratio);

    selected_ratios.push_back(selected_ratio);
  }
  ASSERT_LT(selected_ratios[0], selected_ratios[selected_ratios.size() - 1]);
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
    if (o == std::numeric_limits<uint64_t>::max())
      continue;
    auto it = std::lower_bound(offsets.begin(), offsets.end(), o);
    auto idx = std::distance(offsets.begin(), it);
    ASSERT_LT(idx, offsets.size() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
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
  ASSERT_LT(avg_dev, 0.025);
}

TEST(select_outputs, exact_unlock_block)
{
  std::vector<uint64_t> offsets;
  MKOFFSETS(300000, 1 + (crypto::rand<size_t>() & 0x1f));
  tools::gamma_picker picker(offsets);

  // Calculate output offset ranges for the very first block that is spendable.
  // Since gamma_picker is passed data for EXISTING blocks. The last block it can select outputs
  // from *inclusive* that is allowed by consensus is the
  // -(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1)th block passed to the gamma picker.
  // In the case that there is not unlock time limit enforced by the protocol, this pointer would
  // point to rct_offsets.end() [the address of the element after the last existing element]
  const uint64_t* const first_block_too_young = offsets.data() + offsets.size() - (std::max(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, 1) - 1);
  const uint64_t exact_block_offsets_start_inclusive = *(first_block_too_young - 2);
  const uint64_t exact_block_offsets_stop_exclusive = *(first_block_too_young - 1);

  // if too low we may fail by not picking exact block
  // if too high test is not as sensitive as it could be
  constexpr size_t NUM_PICK_TESTS = 1 << 20;

  bool picked_exact_unlock_block = false;
  for (size_t i = 0; i < NUM_PICK_TESTS; ++i)
  {
    const uint64_t picked_i = picker.pick();
    if (picked_i >= n_outs) // routine bad pick, handle by looping
      continue;

    ASSERT_LT(picked_i, exact_block_offsets_stop_exclusive); // This pick is too young

    if (picked_i >= exact_block_offsets_start_inclusive)
    {
      // this pick is for the youngest valid spendable block
      picked_exact_unlock_block = true;
      break;
    }
  }

  EXPECT_TRUE(picked_exact_unlock_block);
}

TEST(select_outputs, exact_unlock_block_tiny)
{
  // Create chain of length CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE where there is one output in block 0
  std::vector<uint64_t> offsets(std::max(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, 1), 0);
  offsets[0] = 1;
  tools::gamma_picker picker(offsets);

  constexpr size_t MAX_PICK_TRIES = 10;
  bool found_the_one_output = false;
  for (size_t i = 0; i < MAX_PICK_TRIES; ++i)
    if (picker.pick() == 0)
      found_the_one_output = true;

  EXPECT_TRUE(found_the_one_output);
}
