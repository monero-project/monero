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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <atomic>
#include <chrono>
#include <functional>
#include <numeric>
#include <boost/thread/thread.hpp>
#include <vector>

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "misc_language.h"
#include "misc_log_ex.h"
#include "storages/levin_abstract_invoke2.h"
#include "common/util.h"

#include "net_load_tests.h"

using namespace net_load_tests;

namespace
{
  const size_t CONNECTION_COUNT = 100000;
  const size_t CONNECTION_TIMEOUT = 10000;
  const size_t DEFAULT_OPERATION_TIMEOUT = 30000;
  const size_t RESERVED_CONN_CNT = 1;

  template<typename t_predicate>
  bool busy_wait_for(size_t timeout_ms, const t_predicate& predicate, size_t sleep_ms = 10)
  {
    for (size_t i = 0; i < timeout_ms / sleep_ms; ++i)
    {
      if (predicate())
        return true;
      //std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
      epee::misc_utils::sleep_no_w(static_cast<long>(sleep_ms));
    }
    return false;
  }

  class t_connection_opener_1
  {
  public:
    t_connection_opener_1(test_tcp_server& tcp_server, size_t open_request_target)
      : m_tcp_server(tcp_server)
      , m_open_request_target(open_request_target)
      , m_next_id(0)
      , m_error_count(0)
      , m_connections(open_request_target)
    {
      for (auto& conn_id : m_connections)
        conn_id = boost::uuids::nil_uuid();
    }

    bool open()
    {
      size_t id = m_next_id.fetch_add(1, std::memory_order_relaxed);
      if (m_open_request_target <= id)
        return false;

      bool r = m_tcp_server.connect_async("127.0.0.1", srv_port, CONNECTION_TIMEOUT, [=](const test_connection_context& context, const boost::system::error_code& ec) {
        if (!ec)
        {
          m_connections[id] = context.m_connection_id;
        }
        else
        {
          m_error_count.fetch_add(1, std::memory_order_relaxed);
        }
      });

      if (!r)
      {
        m_error_count.fetch_add(1, std::memory_order_relaxed);
      }

      return true;
    }

    bool close(size_t id)
    {
      if (!m_connections[id].is_nil())
      {
        m_tcp_server.get_config_object().close(m_connections[id]);
        return true;
      }
      else
      {
        return false;
      }
    }

    size_t error_count() const { return m_error_count.load(std::memory_order_relaxed); }

  private:
    test_tcp_server& m_tcp_server;
    size_t m_open_request_target;
    std::atomic<size_t> m_next_id;
    std::atomic<size_t> m_error_count;
    std::vector<boost::uuids::uuid> m_connections;
  };

  class t_connection_opener_2
  {
  public:
    t_connection_opener_2(test_tcp_server& tcp_server, size_t open_request_target, size_t max_opened_connection_count)
      : m_tcp_server(tcp_server)
      , m_open_request_target(open_request_target)
      , m_open_request_count(0)
      , m_error_count(0)
      , m_open_close_test_helper(tcp_server, open_request_target, max_opened_connection_count)
    {
    }

    bool open_and_close()
    {
      size_t req_count = m_open_request_count.fetch_add(1, std::memory_order_relaxed);
      if (m_open_request_target <= req_count)
        return false;

      bool r = m_tcp_server.connect_async("127.0.0.1", srv_port, CONNECTION_TIMEOUT, [=](const test_connection_context& context, const boost::system::error_code& ec) {
        if (!ec)
        {
          m_open_close_test_helper.handle_new_connection(context.m_connection_id);
        }
        else
        {
          m_error_count.fetch_add(1, std::memory_order_relaxed);
        }
      });

      if (!r)
      {
        m_error_count.fetch_add(1, std::memory_order_relaxed);
      }

      return true;
    }

    void close_remaining_connections()
    {
      m_open_close_test_helper.close_remaining_connections();
    }

    size_t opened_connection_count() const { return m_open_close_test_helper.opened_connection_count(); }
    size_t error_count() const { return m_error_count.load(std::memory_order_relaxed); }

  private:
    test_tcp_server& m_tcp_server;
    size_t m_open_request_target;
    std::atomic<size_t> m_open_request_count;
    std::atomic<size_t> m_error_count;
    open_close_test_helper m_open_close_test_helper;
  };

  class net_load_test_clt : public ::testing::Test
  {
  public:
    net_load_test_clt()
    : m_tcp_server(epee::net_utils::e_connection_type_RPC) // RPC disables network limit for unit tests
    {
	}
  protected:
    virtual void SetUp()
    {
      m_thread_count = (std::max)(min_thread_count, boost::thread::hardware_concurrency() / 2);

      m_tcp_server.get_config_object().set_handler(&m_commands_handler);
      m_tcp_server.get_config_object().m_invoke_timeout = CONNECTION_TIMEOUT;

      ASSERT_TRUE(m_tcp_server.init_server(clt_port, "127.0.0.1"));
      ASSERT_TRUE(m_tcp_server.run_server(m_thread_count, false));

      // Connect to server
      std::atomic<int> conn_status(0);
      m_context = {};
      ASSERT_TRUE(m_tcp_server.connect_async("127.0.0.1", srv_port, CONNECTION_TIMEOUT, [&](const test_connection_context& context, const boost::system::error_code& ec) {
        if (!ec)
        {
          m_context = context;
        }
        else
        {
          LOG_ERROR("Connection error: " << ec.message());
        }
        conn_status.store(1, std::memory_order_seq_cst);
      }));

      EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 0 != conn_status.load(std::memory_order_seq_cst); })) << "connect_async timed out";
      ASSERT_EQ(1, conn_status.load(std::memory_order_seq_cst));
      ASSERT_FALSE(m_context.m_connection_id.is_nil());

      conn_status.store(0, std::memory_order_seq_cst);
      CMD_RESET_STATISTICS::request req;
      ASSERT_TRUE(epee::net_utils::async_invoke_remote_command2<CMD_RESET_STATISTICS::response>(m_context, CMD_RESET_STATISTICS::ID, req,
        m_tcp_server.get_config_object(), [&](int code, const CMD_RESET_STATISTICS::response& rsp, const test_connection_context&) {
          conn_status.store(code, std::memory_order_seq_cst);
      }));

      EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 0 != conn_status.load(std::memory_order_seq_cst); })) << "reset statistics timed out";
      ASSERT_LT(0, conn_status.load(std::memory_order_seq_cst));
    }

    virtual void TearDown()
    {
      m_tcp_server.send_stop_signal();
      ASSERT_TRUE(m_tcp_server.timed_wait_server_stop(DEFAULT_OPERATION_TIMEOUT));
    }

    static void TearDownTestCase()
    {
      // Stop server
      test_levin_commands_handler *commands_handler_ptr = new test_levin_commands_handler();
      test_levin_commands_handler &commands_handler = *commands_handler_ptr;
      test_tcp_server tcp_server(epee::net_utils::e_connection_type_RPC);
      tcp_server.get_config_object().set_handler(commands_handler_ptr, [](epee::levin::levin_commands_handler<test_connection_context> *handler)->void { delete handler; });
      tcp_server.get_config_object().m_invoke_timeout = CONNECTION_TIMEOUT;

      if (!tcp_server.init_server(clt_port, "127.0.0.1")) return;
      if (!tcp_server.run_server(2, false)) return;

      // Connect to server and invoke shutdown command
      std::atomic<int> conn_status(0);
      test_connection_context cmd_context;
      tcp_server.connect_async("127.0.0.1", srv_port, CONNECTION_TIMEOUT, [&](const test_connection_context& context, const boost::system::error_code& ec) {
        cmd_context = context;
        conn_status.store(!ec ? 1 : -1, std::memory_order_seq_cst);
      });

      if (!busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 0 != conn_status.load(std::memory_order_seq_cst); })) return;
      if (1 != conn_status.load(std::memory_order_seq_cst)) return;

      epee::net_utils::notify_remote_command2(cmd_context, CMD_SHUTDOWN::ID, CMD_SHUTDOWN::request(), tcp_server.get_config_object());

      busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 0 != commands_handler.close_connection_counter(); });
    }

    template<typename Func>
    static auto call_func(size_t /*thread_index*/, const Func& func, int) -> decltype(func())
    {
      func();
    }

    template<typename Func>
    static auto call_func(size_t thread_index, const Func& func, long) -> decltype(func(thread_index))
    {
      func(thread_index);
    }

    template<typename Func>
    void parallel_exec(const Func& func)
    {
      unit_test::call_counter properly_finished_threads;
      std::vector<boost::thread> threads(m_thread_count);
      for (size_t i = 0; i < threads.size(); ++i)
      {
        threads[i] = boost::thread([&, i] {
          call_func(i, func, 0);
          properly_finished_threads.inc();
        });
      }

      for (auto& th : threads)
        th.join();

      ASSERT_EQ(properly_finished_threads.get(), m_thread_count);
    }

    void get_server_statistics(CMD_GET_STATISTICS::response& statistics)
    {
      std::atomic<int> req_status(0);
      CMD_GET_STATISTICS::request req;
      ASSERT_TRUE(epee::net_utils::async_invoke_remote_command2<CMD_GET_STATISTICS::response>(m_context, CMD_GET_STATISTICS::ID, req,
        m_tcp_server.get_config_object(), [&](int code, const CMD_GET_STATISTICS::response& rsp, const test_connection_context&) {
          if (0 < code)
          {
            statistics = rsp;
          }
          else
          {
            LOG_ERROR("Get server statistics error: " << code);
          }
          req_status.store(0 < code ? 1 : -1, std::memory_order_seq_cst);
      }));

      EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 0 != req_status.load(std::memory_order_seq_cst); })) << "get_server_statistics timed out";
      ASSERT_EQ(1, req_status.load(std::memory_order_seq_cst));
    }

    template <typename t_predicate>
    bool busy_wait_for_server_statistics(CMD_GET_STATISTICS::response& statistics, const t_predicate& predicate)
    {
      for (size_t i = 0; i < 30; ++i)
      {
        get_server_statistics(statistics);
        if (predicate(statistics))
        {
          return true;
        }

        //std::this_thread::sleep_for(std::chrono::seconds(1));
        epee::misc_utils::sleep_no_w(1000);
      }

      return false;
    }

    void ask_for_data_requests(size_t request_size = 0)
    {
      CMD_SEND_DATA_REQUESTS::request req;
      req.request_size = request_size;
      epee::net_utils::notify_remote_command2(m_context, CMD_SEND_DATA_REQUESTS::ID, req, m_tcp_server.get_config_object());
    }

  protected:
    test_tcp_server m_tcp_server;
    test_levin_commands_handler m_commands_handler;
    size_t m_thread_count;
    test_connection_context m_context;
  };
}

TEST_F(net_load_test_clt, a_lot_of_client_connections_and_connections_closed_by_client)
{
  // Open connections
  t_connection_opener_1 connection_opener(m_tcp_server, CONNECTION_COUNT);
  parallel_exec([&] {
    while (connection_opener.open());
  });

  // Wait for all open requests to complete
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return CONNECTION_COUNT + RESERVED_CONN_CNT <= m_commands_handler.new_connection_counter() + connection_opener.error_count(); }));
  LOG_PRINT_L0("number of opened connections / fails (total): " << m_commands_handler.new_connection_counter() <<
    " / " << connection_opener.error_count() << " (" << (m_commands_handler.new_connection_counter() + connection_opener.error_count()) << ")");

  // Check
  ASSERT_GT(m_commands_handler.new_connection_counter(), RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() + connection_opener.error_count(), CONNECTION_COUNT + RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() - m_commands_handler.close_connection_counter(), m_tcp_server.get_config_object().get_connections_count());

  // Close connections
  parallel_exec([&](size_t thread_idx) {
    for (size_t i = thread_idx; i < CONNECTION_COUNT; i += m_thread_count)
    {
      connection_opener.close(i);
    }
  });

  // Wait for all opened connections to close
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT <= m_commands_handler.close_connection_counter(); }));
  LOG_PRINT_L0("number of opened / closed connections: " << m_tcp_server.get_config_object().get_connections_count() <<
    " / " << m_commands_handler.close_connection_counter());

  // Check all connections are closed
  ASSERT_EQ(m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT, m_commands_handler.close_connection_counter());
  ASSERT_EQ(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());

  // Wait for server to handle all open and close requests
  CMD_GET_STATISTICS::response srv_stat;
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status
  // It's OK, if server didn't close all opened connections, because of it could receive not all FIN packets
  ASSERT_LE(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_LE(RESERVED_CONN_CNT, srv_stat.opened_connections_count);

  // Request data from server, it causes to close rest connections
  ask_for_data_requests();

  // Wait for server to close rest connections
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status. All connections should be closed
  ASSERT_EQ(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, srv_stat.opened_connections_count);
}

TEST_F(net_load_test_clt, a_lot_of_client_connections_and_connections_closed_by_server)
{
  // Open connections
  t_connection_opener_1 connection_opener(m_tcp_server, CONNECTION_COUNT);
  parallel_exec([&] {
    while (connection_opener.open());
  });

  // Wait for all open requests to complete
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return CONNECTION_COUNT + RESERVED_CONN_CNT <= m_commands_handler.new_connection_counter() + connection_opener.error_count(); }));
  LOG_PRINT_L0("number of opened connections / fails (total): " << m_commands_handler.new_connection_counter() <<
    " / " << connection_opener.error_count() << " (" << (m_commands_handler.new_connection_counter() + connection_opener.error_count()) << ")");

  // Check
  ASSERT_GT(m_commands_handler.new_connection_counter(), RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() + connection_opener.error_count(), CONNECTION_COUNT + RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() - m_commands_handler.close_connection_counter(), m_tcp_server.get_config_object().get_connections_count());

  // Wait for server accepts all connections
  CMD_GET_STATISTICS::response srv_stat;
  int last_new_connection_counter = -1;
  busy_wait_for_server_statistics(srv_stat, [&last_new_connection_counter](const CMD_GET_STATISTICS::response& stat) {
    if (last_new_connection_counter == static_cast<int>(stat.new_connection_counter)) return true;
    else { last_new_connection_counter = static_cast<int>(stat.new_connection_counter); return false; }
  });

  // Close connections
  CMD_CLOSE_ALL_CONNECTIONS::request req;
  ASSERT_TRUE(epee::net_utils::notify_remote_command2(m_context, CMD_CLOSE_ALL_CONNECTIONS::ID, req, m_tcp_server.get_config_object()));

  // Wait for all opened connections to close
  busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT <= m_commands_handler.close_connection_counter(); });
  LOG_PRINT_L0("number of opened / closed connections: " << m_tcp_server.get_config_object().get_connections_count() <<
    " / " << m_commands_handler.close_connection_counter());

  // It's OK, if server didn't close all connections, because it could accept not all our connections
  ASSERT_LE(m_commands_handler.close_connection_counter(), m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT);
  ASSERT_LE(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());

  // Wait for server to handle all open and close requests
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status
  ASSERT_EQ(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, srv_stat.opened_connections_count);

  // Close rest connections
  m_tcp_server.get_config_object().foreach_connection([&](test_connection_context& ctx) {
    if (ctx.m_connection_id != m_context.m_connection_id)
    {
      CMD_DATA_REQUEST::request req;
      bool r = epee::net_utils::async_invoke_remote_command2<CMD_DATA_REQUEST::response>(ctx, CMD_DATA_REQUEST::ID, req,
        m_tcp_server.get_config_object(), [=](int code, const CMD_DATA_REQUEST::response& rsp, const test_connection_context&) {
          if (code <= 0)
          {
            LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST. code = " << code);
          }
      });
      if (!r)
        LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST");
    }
    return true;
  });

  // Wait for all opened connections to close
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT <= m_commands_handler.close_connection_counter(); }));
  LOG_PRINT_L0("number of opened / closed connections: " << m_tcp_server.get_config_object().get_connections_count() <<
    " / " << m_commands_handler.close_connection_counter());

  // Check
  ASSERT_EQ(m_commands_handler.close_connection_counter(), m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());
}

TEST_F(net_load_test_clt, permament_open_and_close_and_connections_closed_by_client)
{
  static const size_t MAX_OPENED_CONN_COUNT = 100;

  // Open/close connections
  t_connection_opener_2 connection_opener(m_tcp_server, CONNECTION_COUNT, MAX_OPENED_CONN_COUNT);
  parallel_exec([&] {
    while (connection_opener.open_and_close());
  });

  // Wait for all open requests to complete
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return CONNECTION_COUNT + RESERVED_CONN_CNT <= m_commands_handler.new_connection_counter() + connection_opener.error_count(); }));
  LOG_PRINT_L0("number of opened connections / fails (total): " << m_commands_handler.new_connection_counter() <<
    " / " << connection_opener.error_count() << " (" << (m_commands_handler.new_connection_counter() + connection_opener.error_count()) << ")");

  // Check
  ASSERT_GT(m_commands_handler.new_connection_counter(), RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() + connection_opener.error_count(), CONNECTION_COUNT + RESERVED_CONN_CNT);

  // Wait for all close requests to complete
  EXPECT_TRUE(busy_wait_for(4 * DEFAULT_OPERATION_TIMEOUT, [&](){ return connection_opener.opened_connection_count() <= MAX_OPENED_CONN_COUNT; }));
  LOG_PRINT_L0("actual number of opened connections: " << connection_opener.opened_connection_count());

  // Check
  ASSERT_EQ(MAX_OPENED_CONN_COUNT, connection_opener.opened_connection_count());

  connection_opener.close_remaining_connections();

  // Wait for all close requests to complete
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return m_commands_handler.new_connection_counter() <= m_commands_handler.close_connection_counter() + RESERVED_CONN_CNT; }));
  LOG_PRINT_L0("actual number of opened connections: " << connection_opener.opened_connection_count());

  ASSERT_EQ(m_commands_handler.new_connection_counter(), m_commands_handler.close_connection_counter() + RESERVED_CONN_CNT);
  ASSERT_EQ(0, connection_opener.opened_connection_count());
  ASSERT_EQ(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());

  // Wait for server to handle all open and close requests
  CMD_GET_STATISTICS::response srv_stat;
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status
  // It's OK, if server didn't close all opened connections, because of it could receive not all FIN packets
  ASSERT_LE(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_LE(RESERVED_CONN_CNT, srv_stat.opened_connections_count);

  // Request data from server, it causes to close rest connections
  ask_for_data_requests();

  // Wait for server to close rest connections
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status. All connections should be closed
  ASSERT_EQ(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, srv_stat.opened_connections_count);
}

TEST_F(net_load_test_clt, permament_open_and_close_and_connections_closed_by_server)
{
  static const size_t MAX_OPENED_CONN_COUNT = 100;

  // Init test
  std::atomic<int> test_state(0);
  CMD_START_OPEN_CLOSE_TEST::request req_start;
  req_start.open_request_target = CONNECTION_COUNT;
  req_start.max_opened_conn_count = MAX_OPENED_CONN_COUNT;
  ASSERT_TRUE(epee::net_utils::async_invoke_remote_command2<CMD_START_OPEN_CLOSE_TEST::response>(m_context, CMD_START_OPEN_CLOSE_TEST::ID, req_start,
    m_tcp_server.get_config_object(), [&](int code, const CMD_START_OPEN_CLOSE_TEST::response&, const test_connection_context&) {
      test_state.store(0 < code ? 1 : -1, std::memory_order_seq_cst);
  }));

  // Wait for server response
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&]{ return 1 == test_state.load(std::memory_order_seq_cst); }));
  ASSERT_EQ(1, test_state.load(std::memory_order_seq_cst));

  // Open connections
  t_connection_opener_1 connection_opener(m_tcp_server, CONNECTION_COUNT);
  parallel_exec([&] {
    while (connection_opener.open());
  });

  // Wait for all open requests to complete
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return CONNECTION_COUNT + RESERVED_CONN_CNT <= m_commands_handler.new_connection_counter() + connection_opener.error_count(); }));
  LOG_PRINT_L0("number of opened connections / fails (total): " << m_commands_handler.new_connection_counter() <<
    " / " << connection_opener.error_count() << " (" << (m_commands_handler.new_connection_counter() + connection_opener.error_count()) << ")");
  LOG_PRINT_L0("actual number of opened connections: " << m_tcp_server.get_config_object().get_connections_count());

  ASSERT_GT(m_commands_handler.new_connection_counter(), RESERVED_CONN_CNT);
  ASSERT_EQ(m_commands_handler.new_connection_counter() + connection_opener.error_count(), CONNECTION_COUNT + RESERVED_CONN_CNT);

  // Wait for server accepts all connections
  CMD_GET_STATISTICS::response srv_stat;
  int last_new_connection_counter = -1;
  busy_wait_for_server_statistics(srv_stat, [&last_new_connection_counter](const CMD_GET_STATISTICS::response& stat) {
    if (last_new_connection_counter == static_cast<int>(stat.new_connection_counter)) return true;
    else { last_new_connection_counter = static_cast<int>(stat.new_connection_counter); return false; }
  });

  // Ask server to close rest connections
  CMD_CLOSE_ALL_CONNECTIONS::request req;
  ASSERT_TRUE(epee::net_utils::notify_remote_command2(m_context, CMD_CLOSE_ALL_CONNECTIONS::ID, req, m_tcp_server.get_config_object()));

  // Wait for almost all connections to be closed by server
  busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return m_commands_handler.new_connection_counter() <= m_commands_handler.close_connection_counter() + RESERVED_CONN_CNT; });

  // It's OK, if there are opened connections, because server could accept not all our connections
  ASSERT_LE(m_commands_handler.close_connection_counter() + RESERVED_CONN_CNT, m_commands_handler.new_connection_counter());
  ASSERT_LE(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());

  // Wait for server to handle all open and close requests
  busy_wait_for_server_statistics(srv_stat, [](const CMD_GET_STATISTICS::response& stat) { return stat.new_connection_counter - RESERVED_CONN_CNT <= stat.close_connection_counter; });
  LOG_PRINT_L0("server statistics: " << srv_stat.to_string());

  // Check server status
  ASSERT_EQ(srv_stat.close_connection_counter, srv_stat.new_connection_counter - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, srv_stat.opened_connections_count);

  // Close rest connections
  m_tcp_server.get_config_object().foreach_connection([&](test_connection_context& ctx) {
    if (ctx.m_connection_id != m_context.m_connection_id)
    {
      CMD_DATA_REQUEST::request req;
      bool r = epee::net_utils::async_invoke_remote_command2<CMD_DATA_REQUEST::response>(ctx, CMD_DATA_REQUEST::ID, req,
        m_tcp_server.get_config_object(), [=](int code, const CMD_DATA_REQUEST::response& rsp, const test_connection_context&) {
          if (code <= 0)
          {
            LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST. code = " << code);
          }
      });
      if (!r)
        LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST");
    }
    return true;
  });

  // Wait for all opened connections to close
  EXPECT_TRUE(busy_wait_for(DEFAULT_OPERATION_TIMEOUT, [&](){ return m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT <= m_commands_handler.close_connection_counter(); }));
  LOG_PRINT_L0("number of opened / closed connections: " << m_tcp_server.get_config_object().get_connections_count() <<
    " / " << m_commands_handler.close_connection_counter());

  // Check
  ASSERT_EQ(m_commands_handler.close_connection_counter(), m_commands_handler.new_connection_counter() - RESERVED_CONN_CNT);
  ASSERT_EQ(RESERVED_CONN_CNT, m_tcp_server.get_config_object().get_connections_count());
}

int main(int argc, char** argv)
{
  TRY_ENTRY();
  tools::on_startup();
  epee::debug::get_set_enable_assert(true, false);
  //set up logging options
  mlog_configure(mlog_get_default_log_path("net_load_tests_clt.log"), true);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
  CATCH_ENTRY_L0("main", 1);
}
