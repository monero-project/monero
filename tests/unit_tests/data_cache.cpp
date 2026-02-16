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

#include "gtest/gtest.h"
#include "common/data_cache.h"

#include <string>
#include <thread>
#include <vector>

TEST(data_cache, empty_cache_has_nothing)
{
  tools::data_cache<uint64_t, 4> cache;
  EXPECT_FALSE(cache.has(0));
  EXPECT_FALSE(cache.has(1));
  EXPECT_FALSE(cache.has(UINT64_MAX));
}

TEST(data_cache, add_and_find)
{
  tools::data_cache<uint64_t, 4> cache;
  cache.add(42);
  EXPECT_TRUE(cache.has(42));
  EXPECT_FALSE(cache.has(43));
}

TEST(data_cache, add_multiple_within_capacity)
{
  tools::data_cache<uint64_t, 4> cache;
  cache.add(1);
  cache.add(2);
  cache.add(3);
  cache.add(4);
  EXPECT_TRUE(cache.has(1));
  EXPECT_TRUE(cache.has(2));
  EXPECT_TRUE(cache.has(3));
  EXPECT_TRUE(cache.has(4));
}

TEST(data_cache, eviction_on_overflow)
{
  tools::data_cache<uint64_t, 4> cache;
  // Fill cache
  cache.add(1);
  cache.add(2);
  cache.add(3);
  cache.add(4);
  // Adding a 5th element should evict the oldest (1)
  cache.add(5);
  EXPECT_FALSE(cache.has(1));
  EXPECT_TRUE(cache.has(2));
  EXPECT_TRUE(cache.has(3));
  EXPECT_TRUE(cache.has(4));
  EXPECT_TRUE(cache.has(5));
}

TEST(data_cache, eviction_wraps_around)
{
  tools::data_cache<uint64_t, 3> cache;
  // Fill: slots [0]=1, [1]=2, [2]=3
  cache.add(1);
  cache.add(2);
  cache.add(3);
  // Overflow: slot [0] overwritten with 4, evicts 1
  cache.add(4);
  EXPECT_FALSE(cache.has(1));
  EXPECT_TRUE(cache.has(4));
  // Overflow again: slot [1] overwritten with 5, evicts 2
  cache.add(5);
  EXPECT_FALSE(cache.has(2));
  EXPECT_TRUE(cache.has(5));
  // Overflow again: slot [2] overwritten with 6, evicts 3
  cache.add(6);
  EXPECT_FALSE(cache.has(3));
  EXPECT_TRUE(cache.has(4));
  EXPECT_TRUE(cache.has(5));
  EXPECT_TRUE(cache.has(6));
}

TEST(data_cache, duplicate_add_is_noop)
{
  tools::data_cache<uint64_t, 4> cache;
  cache.add(1);
  cache.add(2);
  cache.add(3);
  // Re-adding 1 should not advance the counter or evict anything
  cache.add(1);
  cache.add(4);
  // All should still be present since duplicate didn't consume a slot
  EXPECT_TRUE(cache.has(1));
  EXPECT_TRUE(cache.has(2));
  EXPECT_TRUE(cache.has(3));
  EXPECT_TRUE(cache.has(4));
}

TEST(data_cache, string_keys)
{
  tools::data_cache<std::string, 2> cache;
  cache.add("hello");
  cache.add("world");
  EXPECT_TRUE(cache.has("hello"));
  EXPECT_TRUE(cache.has("world"));
  cache.add("foo");
  EXPECT_FALSE(cache.has("hello"));
  EXPECT_TRUE(cache.has("world"));
  EXPECT_TRUE(cache.has("foo"));
}

TEST(data_cache, size_one_cache)
{
  tools::data_cache<int, 1> cache;
  cache.add(10);
  EXPECT_TRUE(cache.has(10));
  cache.add(20);
  EXPECT_FALSE(cache.has(10));
  EXPECT_TRUE(cache.has(20));
}

TEST(data_cache, concurrent_access)
{
  tools::data_cache<uint64_t, 128> cache;
  constexpr int num_threads = 4;
  constexpr int items_per_thread = 100;

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t)
  {
    threads.emplace_back([&cache, t]() {
      for (int i = 0; i < items_per_thread; ++i)
      {
        uint64_t val = t * items_per_thread + i;
        cache.add(val);
        // Just exercise has() concurrently; result may vary due to eviction
        cache.has(val);
      }
    });
  }
  for (auto &th : threads)
    th.join();

  // No crash or deadlock is the primary assertion; also verify cache still works
  cache.add(999999);
  EXPECT_TRUE(cache.has(999999));
}
