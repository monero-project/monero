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

#include <boost/chrono/chrono.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <condition_variable>
#include <mutex>

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "string_tools.h"
#include "net/abstract_tcp_server2.h"
#include "net/levin_protocol_handler_async.h"

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
  using io_context_t = boost::asio::io_service;
  using endpoint_t = boost::asio::ip::tcp::endpoint;
  using work_t = boost::asio::io_service::work;
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
  work_ptr work(std::make_shared<work_t>(io_context));

  workers_t workers;
  while (workers.size() < 4) {
    workers.emplace_back([&io_context]{
      io_context.run();
    });
  }

  endpoint_t endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5262);
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

  io_context.post([&io_context, &work, &endpoint, &server]{
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
      shared_state->close(context.m_connection_id);
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
          shared_state->close(t);

      shared_state->close(context.m_connection_id);
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
      shared_state->close(context.m_connection_id);
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
      shared_state->close(tag);
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
          io_context.post([conn]{
            conn->cancel();
          });
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
          io_context.post([conn]{
            conn->cancel();
          });
          conn.reset();
          s->close(tag);
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
  using io_context_t = boost::asio::io_service;
  using endpoint_t = boost::asio::ip::tcp::endpoint;
  using server_t = epee::net_utils::boosted_tcp_server<handler_t>;
  using socket_t = boost::asio::ip::tcp::socket;
  using ssl_socket_t = boost::asio::ssl::stream<socket_t>;
  using ssl_context_t = boost::asio::ssl::context;
  using ec_t = boost::system::error_code;

  endpoint_t endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5263);
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
  using io_context_t = boost::asio::io_service;
  using work_t = boost::asio::io_service::work;
  using work_ptr = std::shared_ptr<work_t>;
  using workers_t = std::vector<std::thread>;
  using socket_t = boost::asio::ip::tcp::socket;
  using ssl_socket_t = boost::asio::ssl::stream<socket_t>;
  using ssl_socket_ptr = std::unique_ptr<ssl_socket_t>;
  using ssl_options_t = epee::net_utils::ssl_options_t;
  io_context_t io_context;
  work_ptr work(std::make_shared<work_t>(io_context));
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
      io_context.post([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      });
    }
    EXPECT_EQ(
      ssl_options.handshake(
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
          socket->close();
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

  endpoint_t endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5262);
  server_t server(epee::net_utils::e_connection_type_P2P);
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
