// Copyright (c) 2014-2019, The Monero Project
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

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <numeric>
#include <string>

#include "wallet/output_selection.h"
#include "wallet/wallet2.h"


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

TEST(select_outputs, move)
{
  tools::gamma_picker picker{};

  EXPECT_FALSE(bool(picker));
  EXPECT_FALSE(picker.is_valid());
  EXPECT_TRUE(picker.offsets().empty());

  picker = tools::gamma_picker{std::vector<std::uint64_t>(100)};

  EXPECT_TRUE(bool(picker));
  EXPECT_TRUE(picker.is_valid());
  EXPECT_EQ(100u, picker.offsets().size());
}

TEST(select_outputs, clone)
{
  tools::gamma_picker picker{std::vector<std::uint64_t>(100)};

  EXPECT_TRUE(bool(picker));
  EXPECT_TRUE(picker.is_valid());
  EXPECT_EQ(100u, picker.offsets().size());

  tools::gamma_picker cloned{picker.clone()};

  EXPECT_TRUE(bool(picker));
  EXPECT_TRUE(bool(cloned));
  EXPECT_TRUE(picker.is_valid());
  EXPECT_TRUE(cloned.is_valid());
  EXPECT_EQ(100u, picker.offsets().size());
  EXPECT_EQ(100u, cloned.offsets().size());

  EXPECT_FALSE(std::is_copy_constructible<tools::gamma_picker>());
}

TEST(select_outputs, small_chain)
{
  tools::gamma_picker picker{};
  {
    std::vector<std::uint64_t> offsets(50);
    std::iota(offsets.begin(), offsets.end(), 0);
    picker = tools::gamma_picker{std::move(offsets)};
  }

  ASSERT_TRUE(bool(picker));
  EXPECT_TRUE(picker.is_valid());
  EXPECT_GE(40, picker());
}

#define MKOFFSETS(n) \
  size_t n_outs = 0; \
  for (auto &offset: offsets) \
  { \
    offset = n_outs += (n); \
  }

TEST(select_outputs, gamma)
{
  using namespace boost::accumulators;
  using gamma_stats = accumulator_set<double, features<tag::mean, tag::skewness, tag::variance>>;

  static constexpr const crypto::random_device engine{};
  static constexpr const uint32_t trials = 100000;
  static constexpr const uint32_t blocks = 300000;
  static constexpr const unsigned seconds_per_block = 120;
  static constexpr const double shape = 19.28;
  static constexpr const double rate = 1.61;
  static constexpr const double ideal_mean = shape / rate;
  static constexpr const double ideal_variance = shape / (rate * rate);
  const double ideal_skewness = 2 / std::sqrt(shape);
  const double min_time = std::log(double(10 * seconds_per_block));
  const double max_time = std::log(double(blocks * seconds_per_block));

  gamma_stats empirical{};
  gamma_stats expected{};

  std::gamma_distribution<double> gamma{shape, 1 / rate};
  for (uint32_t i = 0; i < trials; ++i)
  {
    const double x = gamma(engine);
    empirical(x);
    if (x < max_time && min_time < x)
      expected(x);
  }

  ASSERT_NEAR(ideal_mean, mean(empirical), 0.05);
  ASSERT_NEAR(ideal_skewness, skewness(empirical), 0.05);
  ASSERT_NEAR(ideal_variance, variance(empirical), 0.1);

  std::vector<uint64_t> offsets(blocks);
  for (unsigned i = 1; i <= 4; ++i)
  {
    MKOFFSETS(i);

    tools::gamma_picker picker{std::move(offsets)};

    EXPECT_TRUE(bool(picker));
    EXPECT_TRUE(picker.is_valid());
    ASSERT_EQ(blocks, picker.offsets().size());
    EXPECT_EQ((blocks * i) - (10 * i), picker.spendable_upper_bound());

    gamma_stats actual{};
    for (uint32_t j = 0; j < trials; ++j)
    {
      const uint64_t o = picker();
      ASSERT_GT(picker.offsets().back(), o);

      // convert back into seconds, and then inverse the scaling
      actual(std::log((double(picker.offsets().back() - o) / i) * seconds_per_block));
    }
    offsets = picker.take_offsets();
    EXPECT_FALSE(bool(picker));
    EXPECT_FALSE(picker.is_valid());
    EXPECT_THROW(picker(), std::logic_error);

    EXPECT_NEAR(mean(expected), mean(actual), 0.05);
    EXPECT_NEAR(skewness(expected), skewness(actual), 0.05);
    EXPECT_NEAR(variance(expected), variance(actual), 0.08);
  }
}

TEST(select_outputs, density)
{
  static constexpr const size_t NPICKS = 1000000;

  tools::gamma_picker picker;
  {
    std::vector<uint64_t> offsets(300000);
    MKOFFSETS(1 + (crypto::rand<std::size_t>() & 0x1f));
    picker = tools::gamma_picker(std::move(offsets));
  }

  std::vector<int> picks(/*n_outs*/picker.offsets().size(), 0);
  for (int i = 0; i < NPICKS; )
  {
    const uint64_t o = picker();
    ASSERT_GE(picker.offsets().back(), o);
    auto it = std::lower_bound(picker.offsets().begin(), picker.offsets().end(), o);
    auto idx = std::distance(picker.offsets().begin(), it);
    ASSERT_LT(idx, picks.size());
    ++picks[idx];
    ++i;
  }

  for (int d = 1; d < 0x20; ++d)
  {
    // count the number of times an output in a block of d outputs was selected
    // count how many outputs are in a block of d outputs
    size_t count_selected = 0, count_chain = 0;
    for (size_t i = 0; i < picker.offsets().size(); ++i)
    {
      size_t n_outputs = picker.offsets()[i] - (i == 0 ? 0 : picker.offsets()[i - 1]);
      if (n_outputs == d)
      {
        count_selected += picks[i];
        count_chain += d;
      }
    }
    float selected_ratio = count_selected / (float)NPICKS;
    float chain_ratio = count_chain / (float)picker.offsets().back();
    MDEBUG(count_selected << "/" << NPICKS << " outputs selected in blocks of density " << d << ", " << 100.0f * selected_ratio << "%");
    MDEBUG(count_chain << "/" << picker.offsets().size() << " outputs in blocks of density " << d << ", " << 100.0f * chain_ratio << "%");
    ASSERT_LT(fabsf(selected_ratio - chain_ratio), 0.025f);
  }
}

TEST(select_outputs, same_distribution)
{
  static constexpr const size_t NPICKS = 1000000;

  tools::gamma_picker picker;
  {
    std::vector<uint64_t> offsets(300000);
    MKOFFSETS(1 + (crypto::rand<std::size_t>() & 0x1f));
    picker = tools::gamma_picker(std::move(offsets));
  }

  std::vector<int> chain_picks(picker.offsets().size(), 0);
  std::vector<int> output_picks(picker.offsets().back(), 0);
  for (int i = 0; i < NPICKS; )
  {
    const uint64_t o = picker();
    ASSERT_GE(picker.offsets().back(), o);
    auto it = std::lower_bound(picker.offsets().begin(), picker.offsets().end(), o);
    auto idx = std::distance(picker.offsets().begin(), it);
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

  double max_dev = 0.0, avg_dev = 0.0;
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
