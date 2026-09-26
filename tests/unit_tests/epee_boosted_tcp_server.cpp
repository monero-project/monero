// Copyright (c) 2014-2022, The Monero Project
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

#include <algorithm>
#include <boost/asio/post.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "include_base_utils.h"
#include "string_tools.h"
#include "net/abstract_tcp_server2.h"
#include "net/levin_protocol_handler_async.h"
#include "p2p/net_node.h"

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
    ASSERT_TRUE(cond.wait_for(lock, boost::chrono::seconds(5), [&counter]{ return counter == 4; }));
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
    ASSERT_TRUE(cond.wait_for(lock, boost::chrono::seconds(5), [&counter]{ return counter == 4; }));
  }

  srv.send_stop_signal();
  ASSERT_TRUE(srv.timed_wait_server_stop(5 * 1000));
  ASSERT_TRUE(srv.deinit_server());
}


TEST(test_epee_connection, test_lifetime)
{
  struct context_t: epee::net_utils::connection_context_base {
    static constexpr size_t get_max_bytes(int) noexcept { return -1; }
    static constexpr int handshake_command() noexcept { return 1001; }
    static constexpr bool handshake_complete() noexcept { return true; }
  };

  using functional_obj_t = std::function<void ()>;
  struct command_handler_t: epee::levin::levin_commands_handler<context_t> {
    size_t delay;
    functional_obj_t on_connection_close_f;
    command_handler_t(size_t delay = 0,
      functional_obj_t on_connection_close_f = nullptr
    ):
      delay(delay),
      on_connection_close_f(on_connection_close_f)
    {}
    virtual int invoke(int, const epee::span<const uint8_t>, epee::byte_stream&, context_t&) override { epee::misc_utils::sleep_no_w(delay); return {}; }
    virtual int notify(int, const epee::span<const uint8_t>, context_t&) override { return {}; }
    virtual void callback(context_t&) override {}
    virtual void on_connection_new(context_t&) override {}
    virtual void on_connection_close(context_t&) override {
      if (on_connection_close_f)
        on_connection_close_f();
    }
    virtual ~command_handler_t() override {}
    static void destroy(epee::levin::levin_commands_handler<context_t>* ptr) { delete ptr; }
  };

  using handler_t = epee::levin::async_protocol_handler<context_t>;
  using connection_t = epee::net_utils::connection<handler_t>;
  using connection_ptr = boost::shared_ptr<connection_t>;
  using shared_state_t = typename connection_t::shared_state;
  using shared_state_ptr = std::shared_ptr<shared_state_t>;
  using shared_states_t = std::vector<shared_state_ptr>;
  using tag_t = boost::uuids::uuid;
  using tags_t = std::vector<tag_t>;
  using io_context_t = boost::asio::io_context;
  using endpoint_t = boost::asio::ip::tcp::endpoint;
  using work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  using work_ptr = std::shared_ptr<work_t>;
  using workers_t = std::vector<std::thread>;
  using server_t = epee::net_utils::boosted_tcp_server<handler_t>;
  using lock_t = std::mutex;
  using lock_guard_t = std::lock_guard<lock_t>;
  using connection_weak_ptr = boost::weak_ptr<connection_t>;
  struct shared_conn_t {
    lock_t lock;
    connection_weak_ptr conn;
  };
  using shared_conn_ptr = std::shared_ptr<shared_conn_t>;

  io_context_t io_context;
  work_ptr work(std::make_shared<work_t>(io_context.get_executor()));

  workers_t workers;
  while (workers.size() < 4) {
    workers.emplace_back([&io_context]{
      io_context.run();
    });
  }

  endpoint_t endpoint(boost::asio::ip::make_address("127.0.0.1"), 5262);
  server_t server(epee::net_utils::e_connection_type_P2P);
  server.init_server(endpoint.port(),
    endpoint.address().to_string(),
    0,
    "",
    false,
    true,
    epee::net_utils::ssl_support_t::e_ssl_support_disabled
  );
  server.run_server(2, false);
  server.get_config_shared()->set_handler(new command_handler_t, &command_handler_t::destroy);

  boost::asio::post(io_context, [&io_context, &work, &endpoint, &server]{
    auto scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&work]{
      work.reset();
    });

    shared_state_ptr shared_state(std::make_shared<shared_state_t>());
    shared_state->set_handler(new command_handler_t, &command_handler_t::destroy);

    auto create_connection = [&io_context, &endpoint, &shared_state] {
        connection_ptr conn(new connection_t(io_context, shared_state, {}, {}));
        conn->socket().connect(endpoint);
        conn->start({}, {});
        context_t context;
        conn->get_context(context);
        auto tag = context.m_connection_id;
        return tag;
    };

    ASSERT_TRUE(shared_state->get_connections_count() == 0);
    auto tag = create_connection();
    ASSERT_TRUE(shared_state->get_connections_count() == 1);
    bool success = shared_state->for_connection(tag, [shared_state](context_t& context){
      shared_state->close(context.m_connection_id, true);
      context.m_remote_address.get_zone();
      return true;
    });
    ASSERT_TRUE(success);

    ASSERT_TRUE(shared_state->get_connections_count() == 0);
    constexpr auto N = 8;
    tags_t tags(N);
    for(auto &t: tags)
      t = create_connection();
    ASSERT_TRUE(shared_state->get_connections_count() == N);
    size_t index = 0;
    success = shared_state->foreach_connection([&index, shared_state, &tags, &create_connection](context_t& context){
      if (!index)
        for (const auto &t: tags)
          shared_state->close(t, true);

      shared_state->close(context.m_connection_id, true);
      context.m_remote_address.get_zone();
      ++index;

      for(auto i = 0; i < N; ++i)
        create_connection();
      return true;
    });
    ASSERT_TRUE(success);
    ASSERT_TRUE(index == N);
    ASSERT_TRUE(shared_state->get_connections_count() == N * N);

    index = 0;
    success = shared_state->foreach_connection([&index, shared_state](context_t& context){
      shared_state->close(context.m_connection_id, true);
      context.m_remote_address.get_zone();
      ++index;
      return true;
    });
    ASSERT_TRUE(success);
    ASSERT_TRUE(index == N * N);
    ASSERT_TRUE(shared_state->get_connections_count() == 0);

    while (shared_state->sock_count);
    ASSERT_TRUE(shared_state->get_connections_count() == 0);
    constexpr auto DELAY = 30;
    constexpr auto TIMEOUT = 1;
    while (server.get_connections_count()) {
      server.get_config_shared()->del_in_connections(
        server.get_config_shared()->get_in_connections_count()
      );
    }
    server.get_config_shared()->set_handler(new command_handler_t(DELAY), &command_handler_t::destroy);
    for (auto i = 0; i < N; ++i) {
      tag = create_connection();
      ASSERT_TRUE(shared_state->get_connections_count() == 1);
      success = shared_state->invoke_async(1, epee::levin::message_writer{}, tag, [](int, const epee::span<const uint8_t>, context_t&){}, TIMEOUT);
      ASSERT_TRUE(success);
      while (shared_state->sock_count == 1) {
        success = shared_state->foreach_connection([&shared_state, &tag](context_t&){
          return shared_state->request_callback(tag);
        });
        ASSERT_TRUE(success);
      }
      shared_state->close(tag, true);
      ASSERT_TRUE(shared_state->get_connections_count() == 0);
    }

    while (shared_state->sock_count);
    constexpr auto ZERO_DELAY = 0;
    size_t counter = 0;
    shared_state->set_handler(new command_handler_t(ZERO_DELAY,
        [&counter]{
          ASSERT_TRUE(counter++ == 0);
        }
      ),
      &command_handler_t::destroy
    );
    connection_ptr conn(new connection_t(io_context, shared_state, {}, {}));
    conn->socket().connect(endpoint);
    conn->start({}, {});
    ASSERT_TRUE(shared_state->get_connections_count() == 1);
    shared_state->del_out_connections(1);
    ASSERT_TRUE(shared_state->get_connections_count() == 0);
    conn.reset();

    while (shared_state->sock_count);
    shared_conn_ptr shared_conn(std::make_shared<shared_conn_t>());
    shared_state->set_handler(new command_handler_t(ZERO_DELAY,
        [shared_state, shared_conn]{
          {
            connection_ptr conn;
            {
              lock_guard_t guard(shared_conn->lock);
              conn = shared_conn->conn.lock();
              shared_conn->conn.reset();
            }
            if (conn)
              conn->cancel();
          }
          const auto success = shared_state->foreach_connection([](context_t&){
            return true;
          });
          ASSERT_TRUE(success);
        }
      ),
      &command_handler_t::destroy
    );
    for (auto i = 0; i < N * N * N; ++i) {
      {
        connection_ptr conn(new connection_t(io_context, shared_state, {}, {}));
        conn->socket().connect(endpoint);
        conn->start({}, {});
        lock_guard_t guard(shared_conn->lock);
        shared_conn->conn = conn;
      }
      ASSERT_TRUE(shared_state->get_connections_count() == 1);
      shared_state->del_out_connections(1);
      while (shared_state->sock_count);
      ASSERT_TRUE(shared_state->get_connections_count() == 0);
    }

    shared_states_t shared_states;
    while (shared_states.size() < 2) {
      shared_states.emplace_back(std::make_shared<shared_state_t>());
      shared_states.back()->set_handler(new command_handler_t(ZERO_DELAY,
          [&shared_states]{
            for (auto &s: shared_states) {
              auto success = s->foreach_connection([](context_t&){
                return true;
              });
              ASSERT_TRUE(success);
            }
          }
        ),
        &command_handler_t::destroy
      );
    }
    workers_t workers;

    for (auto &s: shared_states) {
      workers.emplace_back([&io_context, &s, &endpoint]{
        for (auto i = 0; i < N * N; ++i) {
          connection_ptr conn(new connection_t(io_context, s, {}, {}));
          conn->socket().connect(endpoint);
          conn->start({}, {});
          boost::asio::post(io_context, [conn] { conn->cancel(); });
          conn.reset();
          s->del_out_connections(1);
          while (s->sock_count);
        }
      });
    }
    for (;workers.size(); workers.pop_back())
      workers.back().join();

    for (auto &s: shared_states) {
      workers.emplace_back([&io_context, &s, &endpoint]{
        for (auto i = 0; i < N * N; ++i) {
          connection_ptr conn(new connection_t(io_context, s, {}, {}));
          conn->socket().connect(endpoint);
          conn->start({}, {});
          conn->cancel();
          while (conn.use_count() > 1);
          s->foreach_connection([&io_context, &s, &endpoint, &conn](context_t& context){
            conn.reset(new connection_t(io_context, s, {}, {}));
            conn->socket().connect(endpoint);
            conn->start({}, {});
            conn->cancel();
            while (conn.use_count() > 1);
            conn.reset();
            return true;
          });
          while (s->sock_count);
        }
      });
    }
    for (;workers.size(); workers.pop_back())
      workers.back().join();

    for (auto &s: shared_states) {
      workers.emplace_back([&io_context, &s, &endpoint]{
        for (auto i = 0; i < N; ++i) {
          connection_ptr conn(new connection_t(io_context, s, {}, {}));
          conn->socket().connect(endpoint);
          conn->start({}, {});
          context_t context;
          conn->get_context(context);
          auto tag = context.m_connection_id;
          conn->cancel();
          while (conn.use_count() > 1);
          s->for_connection(tag, [&io_context, &s, &endpoint, &conn](context_t& context){
            conn.reset(new connection_t(io_context, s, {}, {}));
            conn->socket().connect(endpoint);
            conn->start({}, {});
            conn->cancel();
            while (conn.use_count() > 1);
            conn.reset();
            return true;
          });
          while (s->sock_count);
        }
      });
    }
    for (;workers.size(); workers.pop_back())
      workers.back().join();

    for (auto &s: shared_states) {
      workers.emplace_back([&io_context, &s, &endpoint]{
        for (auto i = 0; i < N; ++i) {
          connection_ptr conn(new connection_t(io_context, s, {}, {}));
          conn->socket().connect(endpoint);
          conn->start({}, {});
          context_t context;
          conn->get_context(context);
          auto tag = context.m_connection_id;
          boost::asio::post(io_context, [conn] { conn->cancel(); });
          conn.reset();
          s->close(tag, true);
          while (s->sock_count);
        }
      });
    }
    for (;workers.size(); workers.pop_back())
      workers.back().join();
    while (server.get_connections_count()) {
      server.get_config_shared()->del_in_connections(
        server.get_config_shared()->get_in_connections_count()
      );
    }
  });

  for (auto& w: workers) {
    w.join();
  }
  server.send_stop_signal();
  server.timed_wait_server_stop(5 * 1000);
  server.deinit_server();
}

TEST(test_epee_connection, ssl_shutdown)
{
  struct context_t: epee::net_utils::connection_context_base {
    static constexpr size_t get_max_bytes(int) noexcept { return -1; }
    static constexpr int handshake_command() noexcept { return 1001; }
    static constexpr bool handshake_complete() noexcept { return true; }
  };

  struct command_handler_t: epee::levin::levin_commands_handler<context_t> {
    virtual int invoke(int, const epee::span<const uint8_t>, epee::byte_stream&, context_t&) override { return {}; }
    virtual int notify(int, const epee::span<const uint8_t>, context_t&) override { return {}; }
    virtual void callback(context_t&) override {}
    virtual void on_connection_new(context_t&) override {}
    virtual void on_connection_close(context_t&) override { }
    virtual ~command_handler_t() override {}
    static void destroy(epee::levin::levin_commands_handler<context_t>* ptr) { delete ptr; }
  };

  using handler_t = epee::levin::async_protocol_handler<context_t>;
  using io_context_t = boost::asio::io_context;
  using endpoint_t = boost::asio::ip::tcp::endpoint;
  using server_t = epee::net_utils::boosted_tcp_server<handler_t>;
  using socket_t = boost::asio::ip::tcp::socket;
  using ssl_socket_t = boost::asio::ssl::stream<socket_t>;
  using ssl_context_t = boost::asio::ssl::context;
  using ec_t = boost::system::error_code;

  endpoint_t endpoint(boost::asio::ip::make_address("127.0.0.1"), 5263);
  server_t server(epee::net_utils::e_connection_type_P2P);
  server.init_server(endpoint.port(),
    endpoint.address().to_string(),
    0,
    "",
    false,
    true,
    epee::net_utils::ssl_support_t::e_ssl_support_enabled
  );
  server.get_config_shared()->set_handler(new command_handler_t, &command_handler_t::destroy);
  server.run_server(2, false);

  ssl_context_t ssl_context{boost::asio::ssl::context::sslv23};
  io_context_t io_context;
  ssl_socket_t socket(io_context, ssl_context);
  ec_t ec;
  socket.next_layer().connect(endpoint, ec);
  EXPECT_EQ(ec.value(), 0);
  socket.handshake(boost::asio::ssl::stream_base::client, ec);
  EXPECT_EQ(ec.value(), 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  while (server.get_config_shared()->get_connections_count() < 1);
  server.get_config_shared()->del_in_connections(1);
  while (server.get_config_shared()->get_connections_count() > 0);
  server.send_stop_signal();
  EXPECT_TRUE(server.timed_wait_server_stop(5 * 1000));
  server.deinit_server();
  socket.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  socket.next_layer().close(ec);
  socket.shutdown(ec);
}

TEST(test_epee_connection, ssl_handshake)
{
  using io_context_t = boost::asio::io_context;
  using work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  using work_ptr = std::shared_ptr<work_t>;
  using workers_t = std::vector<std::thread>;
  using socket_t = boost::asio::ip::tcp::socket;
  using ssl_socket_t = boost::asio::ssl::stream<socket_t>;
  using ssl_socket_ptr = std::unique_ptr<ssl_socket_t>;
  using ssl_options_t = epee::net_utils::ssl_options_t;
  io_context_t io_context;
  work_ptr work(std::make_shared<work_t>(io_context.get_executor()));
  workers_t workers;
  auto constexpr N = 2;
  while (workers.size() < N) {
    workers.emplace_back([&io_context]{
      io_context.run();
    });
  }
  ssl_options_t ssl_options{{}};
  auto ssl_context = ssl_options.create_context();
  for (size_t i = 0; i < N * N * N; ++i) {
    ssl_socket_ptr ssl_socket(new ssl_socket_t(io_context, ssl_context));
    ssl_socket->next_layer().open(boost::asio::ip::tcp::v4());
    for (size_t i = 0; i < N; ++i) {
      boost::asio::post(
        io_context,
        [] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
      );
    }
    EXPECT_EQ(
      ssl_options.handshake(
        io_context,
        *ssl_socket,
        ssl_socket_t::server,
        {},
        {},
        std::chrono::milliseconds(0)
      ),
      false
    );
    ssl_socket->next_layer().close();
    ssl_socket.reset();
  }
  work.reset();
  for (;workers.size(); workers.pop_back())
    workers.back().join();
}


TEST(boosted_tcp_server, strand_deadlock)
{
  using context_t = epee::net_utils::connection_context_base;
  using lock_t = std::mutex;
  using unique_lock_t = std::unique_lock<lock_t>;

  struct config_t {
    using condition_t = std::condition_variable_any;
    using lock_guard_t = std::lock_guard<lock_t>;
    void notify_success()
    {
      lock_guard_t guard(lock);
      success = true;
      condition.notify_all();
    }
    lock_t lock;
    condition_t condition;
    bool success;
  };

  struct handler_t {
    using config_type = config_t;
    using connection_context = context_t;
    using byte_slice_t = epee::byte_slice;
    using socket_t = epee::net_utils::i_service_endpoint;

    handler_t(socket_t *socket, config_t &config, context_t &context):
      socket(socket),
      config(config),
      context(context)
    {}
    void after_init_connection()
    {
      unique_lock_t guard(lock);
      if (!context.m_is_income) {
        guard.unlock();
        socket->do_send(byte_slice_t{"."});
      }
    }
    void handle_qued_callback()
    {
    }
    bool handle_recv(const char *data, size_t bytes_transferred)
    {
      unique_lock_t guard(lock);
      if (!context.m_is_income) {
        if (context.m_recv_cnt == 1024) {
          guard.unlock();
          socket->do_send(byte_slice_t{"."});
        }
      }
      else {
        if (context.m_recv_cnt == 1) {
          for(size_t i = 0; i < 1024; ++i) {
            guard.unlock();
            socket->do_send(byte_slice_t{"."});
            guard.lock();
          }
        }
        else if(context.m_recv_cnt == 2) {
          guard.unlock();
          socket->close(false);
        }
      }
      return true;
    }
    void release_protocol()
    {
      unique_lock_t guard(lock);
      if(!context.m_is_income
        && context.m_recv_cnt == 1024
        && context.m_send_cnt == 2
      ) {
        guard.unlock();
        config.notify_success();
      }
    }

    lock_t lock;
    socket_t *socket;
    config_t &config;
    context_t &context;
  };

  using server_t = epee::net_utils::boosted_tcp_server<handler_t>;
  using endpoint_t = boost::asio::ip::tcp::endpoint;

  endpoint_t endpoint(boost::asio::ip::make_address("127.0.0.1"), 5262);
  server_t server(epee::net_utils::e_connection_type_RPC);
  server.init_server(
    endpoint.port(),
    endpoint.address().to_string(),
    {},
    {},
    {},
    true,
    epee::net_utils::ssl_support_t::e_ssl_support_disabled
  );
  server.run_server(2, {});
  server.async_call(
    [&]{
      context_t context;
      ASSERT_TRUE(
        server.connect(
          endpoint.address().to_string(),
          std::to_string(endpoint.port()),
          5,
          context,
          "0.0.0.0",
          epee::net_utils::ssl_support_t::e_ssl_support_disabled
        )
      );
    }
  );
  {
    unique_lock_t guard(server.get_config_object().lock);
    EXPECT_TRUE(
      server.get_config_object().condition.wait_for(
        guard,
        std::chrono::seconds(5),
        [&] { return server.get_config_object().success; }
      )
    );
  }

  server.send_stop_signal();
  server.timed_wait_server_stop(5 * 1000);
  server.deinit_server();
}

TEST(boosted_tcp_server, shutdown)
{
  struct context_t: epee::net_utils::connection_context_base {
    static constexpr size_t get_max_bytes(int) noexcept { return -1; }
    static constexpr int handshake_command() noexcept { return 1001; }
    static constexpr bool handshake_complete() noexcept { return true; }
  };

  struct config_t : epee::levin::async_protocol_handler_config<context_t> {
    void received_handshake() { handshake_received.raise(); }
    epee::simple_event handshake_received;
  };

  struct handler_t : epee::levin::async_protocol_handler<context_t> {
    using config_type = config_t;
    using connection_context = context_t;
    using epee::levin::async_protocol_handler<context_t>::async_protocol_handler;

    bool handle_recv(const void *data, size_t bytes_transferred)
    {
      // We don't respond to the handshake (the async_invoke_remote_command2 is waiting for a response)
      MINFO("handle_recv just came in");
      config_t* config = dynamic_cast<config_t*>(&m_config);
      if (config == nullptr)
        throw std::runtime_error("m_config must be of type config_t");
      config->received_handshake();
      return true;
    }
  };

  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 5262);
  epee::net_utils::boosted_tcp_server<handler_t> server(epee::net_utils::e_connection_type_P2P);
  server.init_server(
    endpoint.port(),
    endpoint.address().to_string(),
    {},
    {},
    {},
    true,
    epee::net_utils::ssl_support_t::e_ssl_support_disabled
  );

  // Run the server in a thread and wait for it to start
  MINFO("Starting the server");
  std::thread running_server([&]{server.run_server(2, true/*wait*/);} );

  // Have the server connect to itself
  MINFO("Connecting the server to itself");
  context_t context;
  {
    epee::simple_event connected;
    server.async_call(
      [&]{
        ASSERT_TRUE(
          server.connect(
            endpoint.address().to_string(),
            std::to_string(endpoint.port()),
            5,
            context,
            "0.0.0.0",
            epee::net_utils::ssl_support_t::e_ssl_support_disabled
          )
        );
        connected.raise();
      }
    );
    connected.wait();
  }

  // Invoke handshake to the connection, and wait for cb cancel in a separate thread
  MINFO("Invoking handshake");
  epee::simple_event ev;
  {
    using COMMAND_HANDSHAKE = nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>;
    COMMAND_HANDSHAKE::request arg;
    bool r = epee::net_utils::async_invoke_remote_command2<COMMAND_HANDSHAKE::response>(context, COMMAND_HANDSHAKE::ID, arg, server.get_config_object(),
      [&ev](int code, const COMMAND_HANDSHAKE::response&, context_t&)
    {
      ASSERT_EQ(code, LEVIN_ERROR_CONNECTION_DESTROYED);
      ev.raise();
    }, P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT);
    ASSERT_TRUE(r);

    MINFO("Waiting for handshake invocation to be received");
    server.get_config_object().handshake_received.wait();
  }

  MINFO("Stopping the server");
  server.mark_stop_signal_sent();
  server.close_server_connections();
  server.get_config_object().close(context.m_connection_id, true/*wait_for_shutdown*/);
  server.stop_io_context();
  running_server.join();

  MINFO("Waiting for handshake to cancel");
  ev.wait();
}

TEST(boosted_tcp_server, write_failure)
{
  using context_t = epee::net_utils::connection_context_base;

  struct config_t {};

  struct handler_t {
    using config_type = config_t;
    using connection_context = context_t;
    using socket_t = epee::net_utils::i_service_endpoint;

    handler_t(socket_t *socket, config_t &config, context_t &):
      config(config)
    {}
    void after_init_connection()
    {}

    void handle_qued_callback()
    {}

    bool handle_recv(const char *data, size_t bytes_transferred)
    {
      throw std::runtime_error{"UNEXPECTED!"};
    }

    void release_protocol()
    {}

    config_t &config;
  };


  using byte_slice_t = epee::byte_slice;
  using connection_t = epee::net_utils::connection<handler_t>;
  using shared_t = connection_t::shared_state;
  using tcp_t = boost::asio::ip::tcp;
  using endpoint_t = tcp_t::endpoint;
  using socket_t = tcp_t::socket;
  using acceptor_t = tcp_t::acceptor;

  const endpoint_t endpoint{boost::asio::ip::make_address("127.0.0.1"), 5262};
  boost::asio::io_context context{};
  acceptor_t acceptor{context};
  acceptor.open(endpoint.protocol());
#if !defined(_WIN32)
  acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
#endif
  acceptor.bind(endpoint);
  acceptor.listen();

  socket_t in_socket{context};

  boost::shared_ptr<connection_t> out_connection;
  const auto shared = std::make_shared<shared_t>();
  const auto make_connection = [&] {
    in_socket = socket_t{context};
    acceptor.async_accept(in_socket, [] (auto error) { EXPECT_TRUE(!error); });

    socket_t out_socket{context};
    out_socket.async_connect(endpoint, [] (auto error) { EXPECT_TRUE(!error); });

    context.restart();
    ASSERT_EQ(2u, context.run()); // connect and accept

    out_connection = boost::make_shared<connection_t>(
      context,
      std::move(out_socket),
      shared,
      epee::net_utils::e_connection_type_P2P,
      epee::net_utils::ssl_support_t::e_ssl_support_disabled
    );
    EXPECT_TRUE(out_connection->start(false, true));
  };

  make_connection();
  {
    const byte_slice_t payload{"."};
    epee::net_utils::i_service_endpoint& out{*out_connection};
    static_assert(ABSTRACT_SERVER_SEND_QUE_MAX_COUNT < std::numeric_limits<std::size_t>::max(), "");
    for (std::size_t i = 0; i <= ABSTRACT_SERVER_SEND_QUE_MAX_COUNT; ++i)
      EXPECT_TRUE(out.do_send(payload.clone()));
    EXPECT_FALSE(out.do_send(payload.clone()));
  }
  context.restart();
  EXPECT_LE(1u, context.run());
  EXPECT_EQ(connection_t::WASTED, out_connection->get_status());

  make_connection();
  {
    const byte_slice_t spayload{"."};
    const byte_slice_t lpayload{std::string(std::size_t(3 * 128 * 1024), '.')};
    epee::net_utils::i_service_endpoint& out{*out_connection};
    for (std::size_t i = 0; i < ABSTRACT_SERVER_SEND_QUE_MAX_COUNT; ++i)
      EXPECT_TRUE(out.do_send(spayload.clone()));
    EXPECT_FALSE(out.do_send(lpayload.clone()));
  }
  context.restart();
  EXPECT_LE(1u, context.run());
  EXPECT_EQ(connection_t::WASTED, out_connection->get_status());
}


namespace
{
  struct rpc_timeout_config
  {
    std::chrono::milliseconds delay{0};
    bool pipelined = false;
    size_t response_size = 64 * 1024;
    size_t received = 0;
    std::promise<void> queued;
  };

  struct slow_rpc_handler
  {
    using config_type = rpc_timeout_config;
    using connection_context = epee::net_utils::connection_context_base;

    slow_rpc_handler(epee::net_utils::i_service_endpoint* socket, config_type& config, connection_context&):
      socket(socket), config(config)
    {}

    void after_init_connection() {}
    void handle_qued_callback() {}
    void release_protocol() {}

    bool handle_recv(const void*, size_t bytes)
    {
      config.received += bytes;
      if (!config.response_size)
        return true;
      const auto reply = [this](size_t size) {
        EXPECT_TRUE(socket->do_send(epee::byte_slice{std::string(size, '.')}));
      };
      if (config.pipelined)
        reply(config.response_size);
      std::this_thread::sleep_for(config.delay);
      reply(config.pipelined ? 1 : config.response_size);
      config.queued.set_value();
      return true;
    }

    epee::net_utils::i_service_endpoint* socket;
    config_type& config;
  };

  struct rpc_timeout_load
  {
    using tcp = boost::asio::ip::tcp;
    using connection_t = epee::net_utils::connection<slow_rpc_handler>;

    rpc_timeout_load(tcp::acceptor& acceptor, const std::shared_ptr<connection_t::shared_state>& shared,
      const epee::net_utils::ipv4_network_address& remote, unsigned same_host)
    {
      // Create enough connections to apply the per-IP timeout cap. Only the
      // same-host peers need to be connected; keep their workers idle.
      for (unsigned i = 0; i < 120; ++i)
      {
        tcp::socket socket{context};
        if (i < same_host)
        {
          peers.emplace_back(context);
          peers.back().connect(acceptor.local_endpoint());
          acceptor.accept(socket);
        }
        connections.push_back(boost::make_shared<connection_t>(context, std::move(socket), shared,
          epee::net_utils::e_connection_type_RPC, epee::net_utils::ssl_support_t::e_ssl_support_disabled));
        if (i < same_host)
        {
          EXPECT_TRUE(connections.back()->start(true, false, remote));
        }
      }
    }

    ~rpc_timeout_load()
    {
      for (auto& connection : connections)
        static_cast<epee::net_utils::i_service_endpoint&>(*connection).close(false);
      context.run_for(std::chrono::seconds{1});
      context.stop();
    }

    boost::asio::io_context context;
    std::vector<boost::shared_ptr<connection_t>> connections;
    std::vector<tcp::socket> peers;
  };

  enum class rpc_handler_case { two_workers, pipelined, one_worker, slow_reader };
  class rpc_handler_timeout : public testing::TestWithParam<rpc_handler_case> {};
}

TEST_P(rpc_handler_timeout, response_is_not_dropped)
{
  using tcp = boost::asio::ip::tcp;
  using connection_t = epee::net_utils::connection<slow_rpc_handler>;

  boost::asio::io_context context;
  tcp::acceptor acceptor{context, tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), 0}};
  // Set the peer's receive buffer before connecting so the response cannot fit
  // in the advertised window.
  tcp::socket peer{context};
  peer.open(tcp::v4());
  peer.set_option(tcp::socket::receive_buffer_size{4096});
  peer.connect(acceptor.local_endpoint());
  tcp::socket socket{context};
  acceptor.accept(socket);
  socket.set_option(tcp::socket::send_buffer_size{4096});

  const bool one_worker = GetParam() == rpc_handler_case::one_worker;
  const bool slow_reader = GetParam() == rpc_handler_case::slow_reader;
  const auto shared = std::make_shared<connection_t::shared_state>();
  shared->pipelined = GetParam() == rpc_handler_case::pipelined;
  // With one worker, adding the write allowance still leaves the old deadline
  // less than 1s overdue, so late-timer recovery cannot hide a missing clamp.
  shared->delay = std::chrono::milliseconds{shared->pipelined ? 74000 : one_worker ? 33500 : 12000};
  if (shared->pipelined)
    shared->response_size = 8 * 1024 * 1024;
  if (slow_reader)
  {
    shared->delay = std::chrono::seconds{0};
    shared->response_size = 8 * 1024 * 1024;
  }
  auto queued = shared->queued.get_future();
  const auto connection = boost::make_shared<connection_t>(context, std::move(socket), shared,
    epee::net_utils::e_connection_type_RPC, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  uint32_t ip = 0;
  ASSERT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "8.8.4.4"));
  const epee::net_utils::ipv4_network_address remote{ip, acceptor.local_endpoint().port()};
  // Four same-host connections cap timeouts at 37.5s. Finish just before
  // the second full write allowance ends, then drain the replies at 512 KiB/s.
  std::unique_ptr<rpc_timeout_load> load;
  if (shared->pipelined)
    load.reset(new rpc_timeout_load(acceptor, shared, remote, 3));
  ASSERT_TRUE(connection->start(true, true, remote));
  boost::asio::write(peer, boost::asio::buffer("?", 1));

  std::vector<std::thread> workers;
  for (unsigned i = 0; i < (one_worker ? 1u : 2u); ++i)
    workers.emplace_back([&context] { context.run(); });

  const auto ready = queued.wait_for(std::chrono::seconds{90});
  EXPECT_EQ(std::future_status::ready, ready);
  if (ready == std::future_status::ready)
  {
    // Let an overdue timer run before draining the response, including on slow CI.
    std::this_thread::sleep_for(std::chrono::seconds{slow_reader ? 12 : 1});
    // A tiny second reply must also grant time for the first reply still in flight.
    peer.non_blocking(true);
    std::string reply(shared->response_size + (shared->pipelined ? 1 : 0), '\0');
    size_t received = 0;
    boost::system::error_code error;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{45};
    while (received < reply.size() && std::chrono::steady_clock::now() < deadline)
    {
      const size_t bytes = peer.read_some(boost::asio::buffer(&reply[received], std::min(reply.size() - received, size_t{4096})), error);
      received += bytes;
      if (error == boost::asio::error::would_block || error == boost::asio::error::try_again)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
      else if (error)
        break;
      else if (shared->pipelined || one_worker)
        std::this_thread::sleep_for(std::chrono::duration<double>{bytes / ((one_worker ? 40.0 : 512.0) * 1024)});
    }
    EXPECT_EQ(reply.size(), received);
    EXPECT_EQ(reply.size(), std::count(reply.begin(), reply.end(), '.'));
  }

  context.stop();
  for (auto& worker : workers)
    worker.join();
  static_cast<epee::net_utils::i_service_endpoint&>(*connection).close(false);
  context.restart();
  context.run_for(std::chrono::seconds{1});
  context.stop();
}

INSTANTIATE_TEST_CASE_P(boosted_tcp_server, rpc_handler_timeout,
  testing::Values(rpc_handler_case::two_workers, rpc_handler_case::pipelined, rpc_handler_case::one_worker, rpc_handler_case::slow_reader));

namespace
{
  enum class rpc_write_case { delayed_completion, delayed_timer, unresponsive_peer, delayed_read, unresponsive_busy, delayed_read_large };
  class rpc_write_timeout : public testing::TestWithParam<rpc_write_case> {};
}

TEST_P(rpc_write_timeout, pending_response)
{
  using tcp = boost::asio::ip::tcp;
  using connection_t = epee::net_utils::connection<slow_rpc_handler>;

  boost::asio::io_context context;
  tcp::acceptor acceptor{context, tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), 0}};
  tcp::socket peer{context};
  peer.open(tcp::v4());
  peer.set_option(tcp::socket::receive_buffer_size{4096});
  peer.connect(acceptor.local_endpoint());
  tcp::socket socket{context};
  acceptor.accept(socket);
  socket.set_option(tcp::socket::send_buffer_size{4096});

  const auto shared = std::make_shared<connection_t::shared_state>();
  uint32_t ip = 0;
  ASSERT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "8.8.4.4"));
  const epee::net_utils::ipv4_network_address remote{ip, acceptor.local_endpoint().port()};
  const auto connection = boost::make_shared<connection_t>(context, std::move(socket), shared,
    epee::net_utils::e_connection_type_RPC, epee::net_utils::ssl_support_t::e_ssl_support_disabled);

  const bool delayed_completion = GetParam() == rpc_write_case::delayed_completion;
  const bool delayed_read = GetParam() == rpc_write_case::delayed_read || GetParam() == rpc_write_case::delayed_read_large;
  const size_t request_size = GetParam() == rpc_write_case::delayed_read_large ? 8000 : 100;
  const bool unresponsive_busy = GetParam() == rpc_write_case::unresponsive_busy;
  const bool unresponsive = GetParam() == rpc_write_case::unresponsive_peer || unresponsive_busy;
  // Nine same-host connections cap the timeout at 300s / 256.
  std::unique_ptr<rpc_timeout_load> load;
  if (delayed_completion || delayed_read || unresponsive)
    load.reset(new rpc_timeout_load(acceptor, shared, remote, 8));
  ASSERT_TRUE(connection->start(true, false, remote));

  epee::net_utils::i_service_endpoint& endpoint = *connection;
  // A drained reply must restore the allowance for another worker stall.
  for (unsigned round = 0; round < (delayed_read ? 2u : 1u); ++round)
  {
    // The small first write completes in the kernel, but its callback cannot run
    // until workers resume. The second response must then receive a fresh timeout.
    if (delayed_completion)
    {
      ASSERT_TRUE(endpoint.do_send(epee::byte_slice{std::string(1, '.')}));
    }
    // Leave a write pending even if the OS enlarges the socket buffers.
    const size_t response_size = delayed_read ? 1024 * 1024 : unresponsive ? 256 * 1024 : 64 * 1024;
    ASSERT_TRUE(endpoint.do_send(epee::byte_slice{std::string(response_size, '.')}));

    if (unresponsive)
    {
      if (unresponsive_busy)
      {
        context.poll();
        // Repeated worker stalls must not keep an unread response alive forever.
        for (unsigned i = 0; i < 2; ++i)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{2500});
          context.restart();
          context.run_for(std::chrono::milliseconds{200});
          if (i == 0)
          {
            EXPECT_EQ(connection_t::RUNNING, connection->get_status());
          }
        }
      }
      else
        context.run_for(std::chrono::seconds{5});
      EXPECT_EQ(connection_t::WASTED, connection->get_status());
    }
    else
    {
      // An idle event loop models all workers occupied by other RPC handlers.
      // Leave the first write completion queued, or let the incomplete write wait.
      if (delayed_read)
        context.run_for(std::chrono::milliseconds{500});
      else if (!delayed_completion)
        context.poll();
      if (delayed_read)
      {
        // A partial next request must not hide lateness or grant write time.
        shared->response_size = 0;
        boost::asio::write(peer, boost::asio::buffer(std::string(request_size, '?')));
      }
      // Keep the completion less than 1s overdue after adding the capped timeout,
      // so recovery for a late timer cannot conceal a broken write allowance.
      std::this_thread::sleep_for(std::chrono::milliseconds{(delayed_completion || delayed_read) ? 2700 : 35000});
      context.restart();
      context.run_for(std::chrono::milliseconds{200});
      EXPECT_EQ(connection_t::RUNNING, connection->get_status());
      if (delayed_read)
      {
        EXPECT_EQ(request_size * (round + 1), shared->received);
      }
      context.restart();
      std::thread worker([&context] { context.run(); });
      peer.non_blocking(true);
      std::string reply(response_size + (delayed_completion ? 1 : 0), '\0');
      size_t received = 0;
      boost::system::error_code error;
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
      while (received < reply.size() && std::chrono::steady_clock::now() < deadline)
      {
        received += peer.read_some(boost::asio::buffer(&reply[received], reply.size() - received), error);
        if (error == boost::asio::error::would_block || error == boost::asio::error::try_again)
          std::this_thread::sleep_for(std::chrono::milliseconds{1});
        else if (error)
          break;
      }
      EXPECT_EQ(reply.size(), received);
      EXPECT_EQ(reply.size(), std::count(reply.begin(), reply.end(), '.'));
      context.stop();
      worker.join();
      // Process the completed write before stalling the next reply.
      context.restart();
      context.run_for(std::chrono::milliseconds{50});
    }
  }

  endpoint.close(false);
  context.restart();
  context.run_for(std::chrono::seconds{1});
  context.stop();
}

INSTANTIATE_TEST_CASE_P(boosted_tcp_server, rpc_write_timeout,
  testing::Values(rpc_write_case::delayed_completion, rpc_write_case::delayed_timer,
    rpc_write_case::unresponsive_peer, rpc_write_case::delayed_read, rpc_write_case::unresponsive_busy,
    rpc_write_case::delayed_read_large));
