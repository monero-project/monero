// Copyright (c) 2019, The Monero Project
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

#include <random>
#include "gtest/gtest.h"
#include "misc_language.h"
#include "rolling_median.h"
#include "crypto/crypto.h"

TEST(rolling_median, one)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(1);
  m.insert(42);
  ASSERT_EQ(m.median(), 42);
  m.insert(18);
  ASSERT_EQ(m.median(), 18);
  m.insert(7483);
  ASSERT_EQ(m.median(), 7483);
}

TEST(rolling_median, two)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(2);
  m.insert(42);
  ASSERT_EQ(m.median(), 42);
  m.insert(45);
  ASSERT_EQ(m.median(), 43);
  m.insert(49);
  ASSERT_EQ(m.median(), 47);
  m.insert(41);
  ASSERT_EQ(m.median(), 45);
  m.insert(43);
  ASSERT_EQ(m.median(), 42);
  m.insert(40);
  ASSERT_EQ(m.median(), 41);
  m.insert(41);
  ASSERT_EQ(m.median(), 40);
}

TEST(rolling_median, series)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(100);
  std::vector<uint64_t> v;
  v.reserve(100);
  for (int i = 0; i < 10000; ++i)
  {
    uint64_t r = rand();
    v.push_back(r);
    if (v.size() > 100)
      v.erase(v.begin());
    m.insert(r);
    std::vector<uint64_t> vcopy = v;
    ASSERT_EQ(m.median(), epee::misc_utils::median(vcopy));
  }
}

TEST(rolling_median, clear_whole)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(100);
  std::vector<uint64_t> random, median;
  random.reserve(10000);
  median.reserve(10000);
  for (int i = 0; i < 10000; ++i)
  {
    random.push_back(rand());
    m.insert(random.back());
    median.push_back(m.median());
  }
  m.clear();
  for (int i = 0; i < 10000; ++i)
  {
    m.insert(random[i]);
    ASSERT_EQ(median[i], m.median());
  }
}

TEST(rolling_median, clear_partway)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(100);
  std::vector<uint64_t> random, median;
  random.reserve(10000);
  median.reserve(10000);
  for (int i = 0; i < 10000; ++i)
  {
    random.push_back(rand());
    m.insert(random.back());
    median.push_back(m.median());
  }
  m.clear();
  for (int i = 10000 - 100; i < 10000; ++i)
  {
    m.insert(random[i]);
  }
  ASSERT_EQ(median[10000-1], m.median());
}

TEST(rolling_median, order)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(1000);
  std::vector<uint64_t> random;
  random.reserve(1000);
  for (int i = 0; i < 1000; ++i)
  {
    random.push_back(rand());
    m.insert(random.back());
  }
  const uint64_t med = m.median();

  std::sort(random.begin(), random.end(), [](uint64_t a, uint64_t b) { return a < b; });
  m.clear();
  for (int i = 0; i < 1000; ++i)
    m.insert(random[i]);
  ASSERT_EQ(med, m.median());

  std::sort(random.begin(), random.end(), [](uint64_t a, uint64_t b) { return a > b; });
  m.clear();
  for (int i = 0; i < 1000; ++i)
    m.insert(random[i]);
  ASSERT_EQ(med, m.median());

  std::shuffle(random.begin(), random.end(), std::default_random_engine(crypto::rand<unsigned>()));
  m.clear();
  for (int i = 0; i < 1000; ++i)
    m.insert(random[i]);
  ASSERT_EQ(med, m.median());
}

TEST(rolling_median, history_blind)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(10);

  uint64_t median = 0;
  for (int i = 0; i < 1000; ++i)
  {
    m.clear();
    int history_length = 743723 % (i+1);
    while (history_length--)
      m.insert(743284 % (i+1));
    for (int j = 0; j < 10; ++j)
      m.insert(8924829384 % (j+1));
    if (i == 0)
      median = m.median();
    else
      ASSERT_EQ(median, m.median());
  }
}

TEST(rolling_median, size)
{
  epee::misc_utils::rolling_median_t<uint64_t> m(10);

  ASSERT_EQ(m.size(), 0);
  m.insert(1);
  ASSERT_EQ(m.size(), 1);
  m.insert(2);
  ASSERT_EQ(m.size(), 2);
  m.clear();
  ASSERT_EQ(m.size(), 0);
  for (int i = 0; i < 10; ++i)
  {
    m.insert(80 % (i + 1));
    ASSERT_EQ(m.size(), i + 1);
  }
  m.insert(1);
  ASSERT_EQ(m.size(), 10);
  m.insert(2);
  ASSERT_EQ(m.size(), 10);
  m.clear();
  ASSERT_EQ(m.size(), 0);
  m.insert(4);
  ASSERT_EQ(m.size(), 1);
  for (int i = 0; i < 1000; ++i)
  {
    m.insert(80 % (i + 1));
    ASSERT_EQ(m.size(), std::min<int>(10, i + 2));
  }
}
