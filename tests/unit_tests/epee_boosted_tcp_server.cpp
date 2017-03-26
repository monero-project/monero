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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/chrono/chrono.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "string_tools.h"
#include "net/abstract_tcp_server2.h"

namespace
{
  const uint32_t test_server_port = 5626;
  const std::string test_server_host("127.0.0.1");

  struct test_connection_context : public epee::net_utils::connection_context_base
  {
  };

  struct test_protocol_handler_config
  {
  };

  struct test_protocol_handler
  {
    typedef test_connection_context connection_context;
    typedef test_protocol_handler_config config_type;

    test_protocol_handler(epee::net_utils::i_service_endpoint* /*psnd_hndlr*/, config_type& /*config*/, connection_context& /*conn_context*/)
    {
    }

    void after_init_connection()
    {
    }

    void handle_qued_callback()
    {
    }

    bool release_protocol()
    {
      return true;
    }

    bool handle_recv(const void* /*data*/, size_t /*size*/)
    {
      return false;
    }
  };

  typedef epee::net_utils::boosted_tcp_server<test_protocol_handler> test_tcp_server;
}

TEST(boosted_tcp_server, worker_threads_are_exception_resistant)
{
  test_tcp_server srv(epee::net_utils::e_connection_type_RPC); // RPC disables network limit for unit tests
  ASSERT_TRUE(srv.init_server(test_server_port, test_server_host));

  boost::mutex mtx;
  boost::condition_variable cond;
  int counter = 0;

  auto counter_incrementer = [&counter, &cond, &mtx]()
  {
    boost::unique_lock<boost::mutex> lock(mtx);
    ++counter;
    if (4 <= counter)
    {
      cond.notify_one();
    }
  };

  // 2 theads, but 4 exceptions
  ASSERT_TRUE(srv.run_server(2, false));
  ASSERT_TRUE(srv.async_call([&counter_incrementer]() { counter_incrementer(); throw std::runtime_error("test 1"); }));
  ASSERT_TRUE(srv.async_call([&counter_incrementer]() { counter_incrementer(); throw std::string("test 2"); }));
  ASSERT_TRUE(srv.async_call([&counter_incrementer]() { counter_incrementer(); throw "test 3"; }));
  ASSERT_TRUE(srv.async_call([&counter_incrementer]() { counter_incrementer(); throw 4; }));

  {
    boost::unique_lock<boost::mutex> lock(mtx);
    ASSERT_NE(boost::cv_status::timeout, cond.wait_for(lock, boost::chrono::seconds(5)));
    ASSERT_EQ(4, counter);
  }

  // Check if threads are alive
  counter = 0;
  //auto counter_incrementer = [&counter]() { counter.fetch_add(1); epee::misc_utils::sleep_no_w(counter.load() * 10); };
  ASSERT_TRUE(srv.async_call(counter_incrementer));
  ASSERT_TRUE(srv.async_call(counter_incrementer));
  ASSERT_TRUE(srv.async_call(counter_incrementer));
  ASSERT_TRUE(srv.async_call(counter_incrementer));

  {
    boost::unique_lock<boost::mutex> lock(mtx);
    ASSERT_NE(boost::cv_status::timeout, cond.wait_for(lock, boost::chrono::seconds(5)));
    ASSERT_EQ(4, counter);
  }

  srv.send_stop_signal();
  ASSERT_TRUE(srv.timed_wait_server_stop(5 * 1000));
  ASSERT_TRUE(srv.deinit_server());
}
