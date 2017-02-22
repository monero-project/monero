// Copyright (c) 2014-2017, The Monero Project
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
#include "common/task_region.h"
#include "common/thread_group.h"

TEST(ThreadGroup, NoThreads)
{
  tools::task_region(tools::thread_group(0), [] (tools::task_region_handle& region) {
    std::atomic<bool> completed{false};
    region.run([&] { completed = true; });
    EXPECT_TRUE(completed);
  });
  {
    tools::thread_group group(0);
    std::atomic<bool> completed{false};
    group.dispatch([&] { completed = true; });
    EXPECT_TRUE(completed);
  }
}

TEST(ThreadGroup, OneThread)
{
  tools::thread_group group(1);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<bool> completed{false};
    tools::task_region(group, [&] (tools::task_region_handle& region) {
      region.run([&] { completed = true; });
    });
    EXPECT_TRUE(completed);
  }
}


TEST(ThreadGroup, UseActiveThreadOnSync)
{
  tools::thread_group group(1);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<bool> completed{false};
    tools::task_region(group, [&] (tools::task_region_handle& region) {
      region.run([&] { while (!completed); });
      region.run([&] { completed = true; });
    });
    EXPECT_TRUE(completed);
  }
}

TEST(ThreadGroup, InOrder)
{
  tools::thread_group group(1);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<unsigned> count{0};
    std::atomic<bool> completed{false};
    tools::task_region(group, [&] (tools::task_region_handle& region) {
      region.run([&] { while (!completed); });
      region.run([&] { if (count == 0) completed = true; });
      region.run([&] { ++count; });
    });
    EXPECT_TRUE(completed);
    EXPECT_EQ(1u, count);
  }
}

TEST(ThreadGroup, TwoThreads)
{
  tools::thread_group group(2);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<bool> completed{false};
    tools::task_region(group, [&] (tools::task_region_handle& region) {
      region.run([&] { while (!completed); });
      region.run([&] { while (!completed); });
      region.run([&] { completed = true; });
    });
    EXPECT_TRUE(completed);
  }
}

TEST(ThreadGroup, Nested) {
  struct fib {
    unsigned operator()(tools::thread_group& group, unsigned value) const {
      if (value == 0 || value == 1) {
        return value;
      }
      unsigned left = 0;
      unsigned right = 0;
      tools::task_region(group, [&, value] (tools::task_region_handle& region) {
        region.run([&, value] { left = fib{}(group, value - 1); });
        region.run([&, value] { right = fib{}(group, value - 2); } );
      });
      return left + right;
    }

    unsigned operator()(tools::thread_group&& group, unsigned value) const {
      return (*this)(group, value);
    }
  };
  // be careful of depth on asynchronous version
  EXPECT_EQ(6765, fib{}(tools::thread_group(0), 20));
  EXPECT_EQ(377, fib{}(tools::thread_group(1), 14));
}

TEST(ThreadGroup, Many)
{
  tools::thread_group group(1);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<unsigned> count{0};
    tools::task_region(group, [&] (tools::task_region_handle& region) {
      for (unsigned tasks = 0; tasks < 1000; ++tasks) {
        region.run([&] { ++count; });
      }
    });
    EXPECT_EQ(1000u, count);
  }
}

TEST(ThreadGroup, ThrowInTaskRegion)
{
  class test_exception final : std::exception {
  public:
    explicit test_exception() : std::exception() {}

    virtual const char* what() const noexcept override {
      return "test_exception";
    }
  };

  tools::thread_group group(1);

  for (unsigned i = 0; i < 3; ++i) {
    std::atomic<unsigned> count{0};
    EXPECT_THROW(
      [&] {
        tools::task_region(group, [&] (tools::task_region_handle& region) {
          for (unsigned tasks = 0; tasks < 1000; ++tasks) {
            region.run([&] { ++count; });
          }
          throw test_exception();
        });
      }(),
      test_exception
    );
    EXPECT_GE(1000u, count);
  }
}
