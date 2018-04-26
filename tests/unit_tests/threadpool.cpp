// Copyright (c) 2018, The Monero Project
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

#include <atomic>
#include "gtest/gtest.h"
#include "misc_language.h"
#include "common/threadpool.h"

TEST(threadpool, wait_nothing)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter;
  waiter.wait(tpool.get());
}

TEST(threadpool, wait_waits)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter;
  std::atomic<bool> b(false);
  tpool->submit(&waiter, [&b](){ epee::misc_utils::sleep_no_w(1000); b = true; });
  ASSERT_FALSE(b);
  waiter.wait(tpool.get());
  ASSERT_TRUE(b);
}

TEST(threadpool, one_thread)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(1));
  tools::threadpool::waiter waiter;

  std::atomic<unsigned int> counter(0);
  for (size_t n = 0; n < 4096; ++n)
  {
    tpool->submit(&waiter, [&counter](){++counter;});
  }
  waiter.wait(tpool.get());
  ASSERT_EQ(counter, 4096);
}

TEST(threadpool, many_threads)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(256));
  tools::threadpool::waiter waiter;

  std::atomic<unsigned int> counter(0);
  for (size_t n = 0; n < 4096; ++n)
  {
    tpool->submit(&waiter, [&counter](){++counter;});
  }
  waiter.wait(tpool.get());
  ASSERT_EQ(counter, 4096);
}

static uint64_t fibonacci(std::shared_ptr<tools::threadpool> tpool, uint64_t n)
{
  if (n <= 1)
    return n;
  uint64_t f1, f2;
  tools::threadpool::waiter waiter;
  tpool->submit(&waiter, [&tpool, &f1, n](){ f1 = fibonacci(tpool, n-1); });
  tpool->submit(&waiter, [&tpool, &f2, n](){ f2 = fibonacci(tpool, n-2); });
  waiter.wait(tpool.get());
  return f1 + f2;
}

TEST(threadpool, reentrency)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter;

  uint64_t f = fibonacci(tpool, 13);
  waiter.wait(tpool.get());
  ASSERT_EQ(f, 233);
}

TEST(threadpool, reentrancy)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter;

  uint64_t f = fibonacci(tpool, 13);
  waiter.wait(tpool.get());
  ASSERT_EQ(f, 233);
}

TEST(threadpool, leaf_throws)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter;

  bool thrown = false, executed = false;
  tpool->submit(&waiter, [&](){
    try { tpool->submit(&waiter, [&](){ executed = true; }); }
    catch(const std::exception &e) { thrown = true; }
  }, true);
  waiter.wait(tpool.get());
  ASSERT_TRUE(thrown);
  ASSERT_FALSE(executed);
}

TEST(threadpool, leaf_reentrancy)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter;

  std::atomic<int> counter(0);
  for (int i = 0; i < 1000; ++i)
  {
    tpool->submit(&waiter, [&](){
      tools::threadpool::waiter waiter;
      for (int j = 0; j < 500; ++j)
      {
        tpool->submit(&waiter, [&](){ ++counter; }, true);
      }
      waiter.wait(tpool.get());
    });
  }
  waiter.wait(tpool.get());
  ASSERT_EQ(counter, 500000);
}
