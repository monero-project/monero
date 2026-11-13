// Copyright (c) 2023-2024, The Monero Project
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
#include <chrono>
#include <csignal>
#include <thread>

#include "gtest/gtest.h"

#include "common/password.h"
#include "common/util.h"

TEST(LocalAddress, localhost) { ASSERT_TRUE(tools::is_local_address("localhost")); }
TEST(LocalAddress, localhost_port) { ASSERT_TRUE(tools::is_local_address("localhost:18081")); }
TEST(LocalAddress, localhost_suffix) { ASSERT_TRUE(tools::is_local_address("test.localhost")); }
TEST(LocalAddress, loopback) { ASSERT_TRUE(tools::is_local_address("127.0.0.1")); }
TEST(LocalAddress, loopback_port) { ASSERT_TRUE(tools::is_local_address("127.0.0.1:18081")); }
TEST(LocalAddress, loopback_protocol) { ASSERT_TRUE(tools::is_local_address("http://127.0.0.1")); }
TEST(LocalAddress, loopback_hi) { ASSERT_TRUE(tools::is_local_address("127.255.255.255")); }
TEST(LocalAddress, loopback_lo) { ASSERT_TRUE(tools::is_local_address("127.0.0.0")); }
TEST(LocalAddress, loopback_ipv6) { ASSERT_TRUE(tools::is_local_address("[0:0:0:0:0:0:0:1]")); }

TEST(LocalAddress, onion) { ASSERT_FALSE(tools::is_local_address("vww6ybal4bd7szmgncyruucpgfkqahzddi37ktceo3ah7ngmcopnpyyd.onion")); }
TEST(LocalAddress, i2p) { ASSERT_FALSE(tools::is_local_address("xmrto2bturnore26xmrto2bturnore26xmrto2bturnore26xmr2.b32.i2p")); }
TEST(LocalAddress, valid_ip) { ASSERT_FALSE(tools::is_local_address("1.2.3.4")); }
TEST(LocalAddress, valid_ipv6) { ASSERT_FALSE(tools::is_local_address("[0:0:0:0:0:0:0:2]")); }
TEST(LocalAddress, valid_domain) { ASSERT_FALSE(tools::is_local_address("getmonero.org")); }
TEST(LocalAddress, local_prefix) { ASSERT_FALSE(tools::is_local_address("localhost.com")); }
TEST(LocalAddress, invalid) { ASSERT_FALSE(tools::is_local_address("test")); }
TEST(LocalAddress, empty) { ASSERT_FALSE(tools::is_local_address("")); }

#ifndef _WIN32
namespace
{
  template<typename F>
  bool wait_for(F f)
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (!f())
    {
      if (std::chrono::steady_clock::now() > deadline)
        return false;
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return true;
  }
}

// install() leaves the dispositions set; restore them so later tests keep default ^C/SIGTERM behavior
class signal_handler : public ::testing::Test
{
protected:
  void SetUp() override
  {
    sigaction(SIGINT, nullptr, &m_int);
    sigaction(SIGTERM, nullptr, &m_term);
    sigaction(SIGPIPE, nullptr, &m_pipe);
  }

  void TearDown() override
  {
    sigaction(SIGINT, &m_int, nullptr);
    sigaction(SIGTERM, &m_term, nullptr);
    sigaction(SIGPIPE, &m_pipe, nullptr);
  }

private:
  struct sigaction m_int, m_term, m_pipe;
};

TEST_F(signal_handler, dispatches_off_signal_thread)
{
  std::atomic<int> received{0};
  std::atomic<bool> other_thread{false};
  const std::thread::id self = std::this_thread::get_id();
  const auto registration = tools::signal_handler::install([&](int type) {
    other_thread = std::this_thread::get_id() != self;
    received = type;
  });
  ASSERT_EQ(0, std::raise(SIGTERM));
  ASSERT_TRUE(wait_for([&] { return received != 0; }));
  EXPECT_EQ(SIGTERM, received);
  EXPECT_TRUE(other_thread);
}

TEST_F(signal_handler, only_live_registration_called)
{
  std::atomic<int> replaced{0}, reset{0}, current{0};
  {
    auto registration = tools::signal_handler::install([&](int) { ++replaced; });
    const auto other = tools::signal_handler::install([&](int) { ++reset; });
    registration = tools::signal_handler::install([&](int) { ++reset; });
  }
  ASSERT_EQ(0, std::raise(SIGINT));
  // the SIGINT may reach this handler; waiting for SIGTERM ensures both are consumed
  const auto registration = tools::signal_handler::install([&](int type) { current = type; });
  ASSERT_EQ(0, std::raise(SIGTERM));
  ASSERT_TRUE(wait_for([&] { return current == SIGTERM; }));
  EXPECT_EQ(0, replaced);
  EXPECT_EQ(0, reset);
}

TEST_F(signal_handler, reset_waits_for_running_handler)
{
  std::atomic<bool> entered{false}, finished{false};
  auto registration = tools::signal_handler::install([&](int) {
    entered = true;
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    finished = true;
  });
  ASSERT_EQ(0, std::raise(SIGTERM));
  ASSERT_TRUE(wait_for([&] { return entered.load(); }));
  registration.reset();
  EXPECT_TRUE(finished);
}

TEST_F(signal_handler, dropped_while_prompting)
{
  std::atomic<int> received{0};
  const auto registration = tools::signal_handler::install([&](int) { ++received; });
  tools::password_container::is_prompting = true;
  ASSERT_EQ(0, std::raise(SIGTERM));
  tools::password_container::is_prompting = false;
  ASSERT_EQ(0, std::raise(SIGTERM));
  ASSERT_TRUE(wait_for([&] { return received != 0; }));
  std::this_thread::sleep_for(std::chrono::milliseconds{50});
  EXPECT_EQ(1, received);
}
#endif
