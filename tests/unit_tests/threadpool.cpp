// Copyright (c) 2018-2024, The Monero Project

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

#include <atomic>
#include <thread>

#include "misc_language.h"
#include "common/threadpool.h"
#include "common/util.h"

TEST(threadpool, wait_nothing)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter(*tpool);;
  waiter.wait();
}

TEST(threadpool, wait_waits)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter(*tpool);
  std::atomic<bool> b(false);
  tpool->submit(&waiter, [&b](){ epee::misc_utils::sleep_no_w(1000); b = true; });
  ASSERT_FALSE(b);
  waiter.wait();
  ASSERT_TRUE(b);
}

TEST(threadpool, one_thread)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(1));
  tools::threadpool::waiter waiter(*tpool);

  std::atomic<unsigned int> counter(0);
  for (size_t n = 0; n < 4096; ++n)
  {
    tpool->submit(&waiter, [&counter](){++counter;});
  }
  waiter.wait();
  ASSERT_EQ(counter, 4096);
}

TEST(threadpool, many_threads)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(256));
  tools::threadpool::waiter waiter(*tpool);

  std::atomic<unsigned int> counter(0);
  for (size_t n = 0; n < 4096; ++n)
  {
    tpool->submit(&waiter, [&counter](){++counter;});
  }
  waiter.wait();
  ASSERT_EQ(counter, 4096);
}

static uint64_t fibonacci(std::shared_ptr<tools::threadpool> tpool, uint64_t n)
{
  if (n <= 1)
    return n;
  uint64_t f1, f2;
  tools::threadpool::waiter waiter(*tpool);
  tpool->submit(&waiter, [&tpool, &f1, n](){ f1 = fibonacci(tpool, n-1); });
  tpool->submit(&waiter, [&tpool, &f2, n](){ f2 = fibonacci(tpool, n-2); });
  waiter.wait();
  return f1 + f2;
}

TEST(threadpool, reentrency)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter(*tpool);

  uint64_t f = fibonacci(tpool, 13);
  waiter.wait();
  ASSERT_EQ(f, 233);
}

TEST(threadpool, reentrancy)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter(*tpool);

  uint64_t f = fibonacci(tpool, 13);
  waiter.wait();
  ASSERT_EQ(f, 233);
}

TEST(threadpool, leaf_throws)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests());
  tools::threadpool::waiter waiter(*tpool);

  bool thrown = false, executed = false;
  tpool->submit(&waiter, [&](){
    try { tpool->submit(&waiter, [&](){ executed = true; }); }
    catch(const std::exception &e) { thrown = true; }
  }, true);
  waiter.wait();
  ASSERT_TRUE(thrown);
  ASSERT_FALSE(executed);
}

TEST(threadpool, leaf_reentrancy)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(4));
  tools::threadpool::waiter waiter(*tpool);

  std::atomic<int> counter(0);
  for (int i = 0; i < 1000; ++i)
  {
    tpool->submit(&waiter, [&](){
      tools::threadpool::waiter waiter(*tpool);
      for (int j = 0; j < 500; ++j)
      {
        tpool->submit(&waiter, [&](){ ++counter; }, true);
      }
      waiter.wait();
    });
  }
  waiter.wait();
  ASSERT_EQ(counter, 500000);
}

TEST(threadpool, light_and_heavy_starvation_non_leaf)
{
  std::shared_ptr<tools::threadpool> tpool(tools::threadpool::getNewForUnitTests(2 * tools::get_max_concurrency() + 1));
  std::atomic<bool> is_light_done_waiting(false);
  std::atomic<bool> queued_heavy_job(false);
  std::chrono::microseconds light_waiting_period(3 * 1000000); // 3 seconds

  // First, make a "light" thread with one trivial job. Then, spawn a "heavy"
  // thread with indefinite hard jobs. Then, wait a little bit in the light
  // thread for jobs to enqueue. Then, test that the "light" thread's waiter
  // completes within a reasonable timeframe.

  std::thread light_thread([&tpool, &is_light_done_waiting, &queued_heavy_job, &light_waiting_period](){
    tools::threadpool::waiter light_waiter(*tpool);
    tpool->submit(&light_waiter, [](){ /* light work, no reaction */ });
    while (!queued_heavy_job.load()); // spin lock until at least one heavy job is queued
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait a little bit longer for good measure
    const auto pre_wait_time = std::chrono::steady_clock::now();
    light_waiter.wait();
    const auto post_wait_time = std::chrono::steady_clock::now();
    light_waiting_period = std::chrono::duration_cast<std::chrono::microseconds>(post_wait_time - pre_wait_time);
    is_light_done_waiting = true;
  });

  std::thread heavy_thread([&tpool, &is_light_done_waiting, &queued_heavy_job](){
    tools::threadpool::waiter heavy_waiter(*tpool);
    while (!is_light_done_waiting) // so this test doesn't take forever
    {
      tpool->submit(&heavy_waiter, [&is_light_done_waiting](){
        if (is_light_done_waiting) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //slow work
      });
      queued_heavy_job = true;
    }
    heavy_waiter.wait();
  });

  light_thread.join();
  heavy_thread.join();

  // Assert that the light thread waited less than 2 seconds. Each heavy job is 1 second, and
  // the light thread waiter might have picked up one heavy job, but it should be less than 2
  ASSERT_LT(light_waiting_period.count(), 2 * 1000000);
}
