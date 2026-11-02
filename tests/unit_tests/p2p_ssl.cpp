// Copyright (c) 2026, The Monero Project
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
#include <boost/asio/coroutine.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/thread/future.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include "gtest/gtest.h"

#include "net/abstract_tcp_server2.h"
#include "net/net_ssl.h"
#include "scope_guard.h"

namespace
{
  using tcp = boost::asio::ip::tcp;
  using error_promise = boost::promise<boost::system::error_code>;

  constexpr const std::chrono::seconds timeout{1};
  constexpr const boost::chrono::seconds fail_timeout{2};

  constexpr const std::size_t read_size = 1 * 1024;
  constexpr const std::uint32_t test_server_port = 5626;
  const std::string test_server_host("127.0.0.1");

  boost::system::error_code make_ssl_error(const int err) noexcept
  {
    return boost::asio::error::make_error_code(
      boost::asio::error::ssl_errors(int(ERR_PACK(ERR_LIB_SSL, 0, err)))
    );
  }

  boost::system::error_code cert_check_failed() noexcept
  { return make_ssl_error(SSL_R_CERTIFICATE_VERIFY_FAILED); }

  boost::system::error_code bad_version() noexcept
  { return make_ssl_error(SSL_R_WRONG_VERSION_NUMBER); }

  class connection
  {
    boost::asio::io_context::strand strand_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> sock_;
    boost::beast::flat_buffer bytes_;

    template<typename T, typename F>
    void async_read_some_t(T& sock, F&& f)
    {
      sock.async_read_some(
        bytes_.prepare(read_size),
        boost::asio::bind_executor(strand_, std::forward<F>(f))
      );
    }

  public:
    enum class status : std::uint8_t { not_ssl = 0, is_ssl, need_more };

    explicit connection(boost::asio::io_context& io, boost::asio::ssl::context& ssl)
      : strand_(io), sock_(io, ssl), bytes_()
    {}

    tcp::socket& socket() noexcept { return sock_.next_layer(); }
    const tcp::socket& socket() const noexcept { return sock_.next_layer(); }
    
    status is_ssl(const std::size_t bytes)
    {
      bytes_.commit(bytes);
      const auto src = bytes_.cdata();
      if (src.size() < epee::net_utils::get_ssl_magic_size())
        return status::need_more;
      return status(std::uint8_t(epee::net_utils::is_ssl(reinterpret_cast<const unsigned char*>(src.data()), src.size())));
    }

    template<typename F>
    void post(F&& f)
    {
      boost::asio::post(strand_, std::forward<F>(f));
    }

    template<typename F>
    void async_handshake(F&& f)
    {
      sock_.async_handshake(
        boost::asio::ssl::stream_base::server,
        boost::asio::bind_executor(strand_, std::forward<F>(f))
      );
    }

    template<typename F>
    void async_read_some(F&& f)
    {
      async_read_some_t(sock_.next_layer(), std::forward<F>(f));
    }

    template<typename F>
    void async_ssl_read_some(F&& f)
    {
      async_read_some_t(sock_, std::forward<F>(f));
    }
  };

  class clearnet : public boost::asio::coroutine
  {
    std::shared_ptr<connection> self_;

  public:
    explicit clearnet(std::shared_ptr<connection> self) noexcept
      : boost::asio::coroutine(), self_(std::move(self))
    {}

    void operator()(const boost::system::error_code error = {}, const std::size_t in = {})
    {
      if (!self_)
        throw std::logic_error{"unexpected nullptr"};
      if (error)
        return;

      BOOST_ASIO_CORO_REENTER(*this)
      {
        while (true)
          BOOST_ASIO_CORO_YIELD self_->async_read_some(*this);
      }
    }
  };

  class encrypted : public boost::asio::coroutine
  {
    std::shared_ptr<connection> self_;
    std::shared_ptr<error_promise> handshake_;

  public:
    explicit encrypted(std::shared_ptr<connection> self, std::shared_ptr<error_promise> handshake) noexcept
      : boost::asio::coroutine(), self_(std::move(self)), handshake_(std::move(handshake))
    {}

    void operator()(const boost::system::error_code error = {}, const std::size_t in = {})
    {
      if (!self_)
        throw std::logic_error{"unexpected nullptr"};
      if (!handshake_ && error)
        return;
      BOOST_ASIO_CORO_REENTER(*this)
      {
        BOOST_ASIO_CORO_YIELD self_->async_handshake(*this);
        if (handshake_)
          handshake_->set_value(error);

        handshake_.reset();
        while (true)
          BOOST_ASIO_CORO_YIELD self_->async_ssl_read_some(*this);
      }
    }
  };

  class remote_server
  {
    boost::asio::io_context io_;
    boost::asio::io_context::strand strand_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
    tcp::acceptor acceptor_;
    boost::asio::ssl::context ssl_;
    boost::thread thread_;
    boost::unique_future<boost::system::error_code> handshake_;
    boost::mutex sync_;
    std::atomic<unsigned> count_;
    const std::uint16_t dead_port_;
    const bool encrypted_;

    template<typename F>
    void async_accept(tcp::socket& sock, F&& f)
    {
      acceptor_.async_accept(
        sock, boost::asio::bind_executor(strand_, std::forward<F>(f))
      );
    }

    class accept_loop : public boost::asio::coroutine
    {
      remote_server* const self_;
      std::shared_ptr<connection> next_;
      std::shared_ptr<error_promise> promise_;

    public:
      explicit accept_loop(remote_server* const self) noexcept
        : boost::asio::coroutine(), self_(self), next_()
      {}

      void operator()(boost::system::error_code error = {})
      {
        if (error)
          return;

        BOOST_ASIO_CORO_REENTER(*this)
        {
          while (true)
          {
            next_ = std::make_shared<connection>(self_->io_, self_->ssl_);
            BOOST_ASIO_CORO_YIELD self_->async_accept(next_->socket(), *this);

            if (self_->encrypted_)
            {
              const boost::lock_guard<boost::mutex> lock{self_->sync_};
              auto promise = std::make_shared<error_promise>();
              self_->handshake_ = promise->get_future();
              next_->post(encrypted{next_, std::move(promise)});
            }
            else
              next_->post(clearnet{next_});

            ++(self_->count_);
          } 
        }
      }
    };

  public:
    explicit remote_server(const bool ssl)
      : io_(),
        strand_(io_),
        guard_(boost::asio::make_work_guard(io_)), 
        acceptor_(io_, tcp::endpoint(tcp::v4(), 0)),
        ssl_(epee::net_utils::ssl_options_t{epee::net_utils::ssl_support_t::e_ssl_support_enabled}.create_context()),
        thread_([this] { io_.run(); }),
        handshake_(),
        sync_(),
        count_(0),
        dead_port_(acceptor_.local_endpoint().port()),
        encrypted_(ssl)
    {
      acceptor_.close();
    }

    ~remote_server()
    {
      guard_.reset();
      io_.stop();
      thread_.join();
    }

    remote_server(const remote_server&) = delete;
    remote_server& operator=(const remote_server&) = delete;

    std::uint16_t port() const
    { 
      if (acceptor_.is_open())
        return acceptor_.local_endpoint().port();
      return dead_port_;
    }

    bool sync(const unsigned count) const
    {
      const auto start = boost::chrono::steady_clock::now();
      while (count_ < count)
      {
        if (fail_timeout <= boost::chrono::steady_clock::now() - start)
          return false;
        boost::this_thread::sleep_for(boost::chrono::milliseconds{10});
      }
      return true;
    }

    void start()
    {
      if (acceptor_.is_open()) return;
      acceptor_.open(tcp::v4());
      acceptor_.bind(tcp::endpoint(tcp::v4(), 0));
      acceptor_.listen(3);
      boost::asio::post(strand_, accept_loop{this}); 
    }

    std::vector<std::uint8_t> get_fingerprint()
    {
      namespace net = epee::net_utils;
      return net::convert_fingerprint(net::get_ssl_fingerprint(ssl_));
    }

    boost::system::error_code handshake()
    {
      boost::unique_future<boost::system::error_code> handshake;
      boost::unique_lock<boost::mutex> lock{sync_};
      handshake = std::move(handshake_);
      lock.unlock();
      if(handshake.valid() && handshake.wait_for(fail_timeout) == boost::future_status::ready)
        return handshake.get();
      return boost::asio::error::timed_out;
    }
  };

  struct test_connection_context : public epee::net_utils::connection_context_base
  {
    static constexpr size_t pad_max(int) noexcept { return 0; }
  };

  struct test_protocol_handler_config
  {
    epee::byte_slice out = nullptr;
    std::size_t connections = 0;

    bool after_init_connection(std::shared_ptr<epee::net_utils::i_service_endpoint> conn) noexcept
    {
      ++connections;
      if (conn && !out.empty())
        conn->do_send(std::move(out));
      out = nullptr;
      return true;
    }
  };

  struct test_protocol_handler
  {
    using connection_context = test_connection_context;
    using config_type = test_protocol_handler_config;

    test_protocol_handler(epee::net_utils::i_service_endpoint*, const config_type&, const connection_context&) noexcept
    {}

    void handle_qued_callback() noexcept
    {}

    static constexpr bool release_protocol() noexcept { return true; }
    static constexpr bool handle_recv(void const*, std::size_t) noexcept { return false; }
  };

  using local_server =
    epee::net_utils::boosted_tcp_server<test_protocol_handler>;
}

TEST(p2p_ssl, add_connection_to_no_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{false /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
  
  const auto run_test = [&] (const char* trace, const bool success, const net::ssl_support_t support = net::ssl_support_t::e_ssl_support_disabled)
    {
      SCOPED_TRACE(trace);
      test_connection_context ctx{};
      tcp::socket sock{local.get_io_context()};
      sock.connect(tcp::endpoint{boost::asio::ip::make_address_v4("127.0.0.1"), remote.port()}); 
      ASSERT_EQ(
        local.add_connection(
          ctx,
          std::move(sock),
          net::ipv4_network_address{MAKE_IP(127, 0, 0, 1), remote.port()},
          options,
          timeout
        ),
        success
      );
      if (success) { EXPECT_EQ(ctx.m_ssl, support); }
    };

  options = net::ssl_support_t::e_ssl_support_disabled;
  remote.start();
  run_test("connect success", true);
  EXPECT_TRUE(remote.sync(1));

  options = net::ssl_support_t::e_ssl_support_autodetect;
  options.verification = net::ssl_verification_t::none;
  run_test("autodetect success", false);
  EXPECT_TRUE(remote.sync(2));

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  run_test("encrypt fail", false);
  EXPECT_TRUE(remote.sync(3));

  EXPECT_EQ(local.get_config_object().connections, 1);
}

TEST(p2p_ssl, add_connection_to_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{true /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
 
  const auto run_test = [&] (const char* trace, const bool success, const net::ssl_support_t support = net::ssl_support_t::e_ssl_support_enabled)
    {
      SCOPED_TRACE(trace);
      test_connection_context ctx{};
      tcp::socket sock{local.get_io_context()};
      sock.connect(tcp::endpoint{boost::asio::ip::make_address_v4("127.0.0.1"), remote.port()}); 
      ASSERT_EQ(
        local.add_connection(
          ctx,
          std::move(sock),
          net::ipv4_network_address{MAKE_IP(127, 0, 0, 1), remote.port()},
          options,
          timeout
        ),
        success
      );
      if (success) { EXPECT_EQ(ctx.m_ssl, support); }
    };

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  remote.start();
  run_test("encrypt success", true);

  {
    auto fingerprint = remote.get_fingerprint();
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("encrypt+verify success", true);

    fingerprint.at(0)++;
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("verification fail", false);
  }

  options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect;
  options.verification = epee::net_utils::ssl_verification_t::none;
  run_test("autodetect success", true, net::ssl_support_t::e_ssl_support_autodetect);
  EXPECT_TRUE(remote.sync(4));
  EXPECT_EQ(remote.handshake(), boost::system::error_code{});

  local.get_config_object().out = epee::byte_slice{std::string(1024, 'A')};
  options = net::ssl_support_t::e_ssl_support_disabled;
  run_test("no encrypt", true, net::ssl_support_t::e_ssl_support_disabled);
  EXPECT_TRUE(remote.sync(5));
  EXPECT_EQ(remote.handshake(), bad_version());

  EXPECT_EQ(local.get_config_object().connections, 4);
}

TEST(p2p_ssl, connect_to_no_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{false /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
  
  const auto run_test = [&] (const char* trace, const bool success, const net::ssl_support_t support = net::ssl_support_t::e_ssl_support_disabled)
    {
      SCOPED_TRACE(trace);
      test_connection_context ctx{};       
      ASSERT_EQ(
        local.connect(
          "127.0.0.1",
          std::to_string(remote.port()),
          timeout,
          ctx,
          "0.0.0.0",
          options
        ),
        success
      );
      if (success) { EXPECT_EQ(ctx.m_ssl, support); }
    };

  options = net::ssl_support_t::e_ssl_support_disabled;
  run_test("connect fail", false);

  remote.start();
  run_test("connect success", true);
  EXPECT_TRUE(remote.sync(1));

  options = net::ssl_support_t::e_ssl_support_autodetect;
  options.verification = net::ssl_verification_t::none;
  run_test("autodetect success", true);
  EXPECT_TRUE(remote.sync(3));

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  run_test("encrypt fail", false);
  EXPECT_TRUE(remote.sync(4));

  EXPECT_EQ(local.get_config_object().connections, 2);
}

TEST(p2p_ssl, connect_to_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{true /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
 
  const auto run_test = [&] (const char* trace, const bool success, const net::ssl_support_t support = net::ssl_support_t::e_ssl_support_enabled)
    {
      SCOPED_TRACE(trace);
      test_connection_context ctx{};       
      ASSERT_EQ(
        local.connect(
          "127.0.0.1",
          std::to_string(remote.port()),
          timeout,
          ctx,
          "0.0.0.0",
          options
        ),
        success
      );
      if (success) { EXPECT_EQ(ctx.m_ssl, support); }
    };

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  run_test("connect fail", false);

  remote.start();
  run_test("encrypt success", true);
  EXPECT_TRUE(remote.sync(1));

  {
    auto fingerprint = remote.get_fingerprint();
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("encrypt+verify success", true);
    EXPECT_TRUE(remote.sync(2));

    fingerprint.at(0)++;
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("verification fail", false);
    EXPECT_TRUE(remote.sync(3));
  }

  options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect;
  options.verification = epee::net_utils::ssl_verification_t::none;
  run_test("autodetect success", true, net::ssl_support_t::e_ssl_support_autodetect);
  EXPECT_TRUE(remote.sync(4));
  EXPECT_EQ(remote.handshake(), boost::system::error_code{});

  local.get_config_object().out = epee::byte_slice{std::string(1024, 'A')};
  options = net::ssl_support_t::e_ssl_support_disabled;
  run_test("no encrypt", true, net::ssl_support_t::e_ssl_support_disabled);
  EXPECT_TRUE(remote.sync(5));
  EXPECT_EQ(remote.handshake(), bad_version());

  EXPECT_EQ(local.get_config_object().connections, 4);
}

TEST(p2p_ssl, async_connect_to_no_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{false /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
  
  const auto run_test = [&] (const char* trace, const boost::system::error_code& error, const bool ne = false)
    {
      SCOPED_TRACE(trace);
      const auto result = std::make_shared<error_promise>();
      auto future = result->get_future();
      ASSERT_TRUE(
        local.connect_async(
          "127.0.0.1",
          std::to_string(remote.port()),
          timeout,
          [=] (const auto&, const auto& error) { result->set_value(error); },
          "0.0.0.0",
          options
        )
      );
      ASSERT_EQ(future.wait_for(fail_timeout), boost::future_status::ready);
      if (ne) { EXPECT_NE(future.get(), error); }
      else { EXPECT_EQ(future.get(), error); }
    };

  options = net::ssl_support_t::e_ssl_support_disabled;
  run_test("connect fail", {}, true);

  remote.start();
  run_test("connect success", {});
  EXPECT_TRUE(remote.sync(1));

  options = net::ssl_support_t::e_ssl_support_autodetect;
  options.verification = net::ssl_verification_t::none;
  run_test("autodetect success", {});
  EXPECT_TRUE(remote.sync(3));

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  run_test("encrypt fail", {}, true);
  EXPECT_TRUE(remote.sync(4));

  EXPECT_EQ(local.get_config_object().connections, 2);
}

TEST(p2p_ssl, async_connect_to_ssl)
{
  namespace net = epee::net_utils;
  remote_server remote{true /*ssl*/};

  net::ssl_options_t options{net::ssl_support_t::e_ssl_support_autodetect};
  options.verification = net::ssl_verification_t::none;

  local_server local{net::e_connection_type_P2P};
  ASSERT_TRUE(local.init_server(test_server_port, test_server_host, 0, "::", false, true, options));
  ASSERT_TRUE(local.run_server(1, false));
  const epee::scope_guard deinit{[&] {
      local.send_stop_signal();
      local.timed_wait_server_stop(boost::chrono::milliseconds{fail_timeout}.count());
      local.deinit_server();
    }
  };
  
  const auto run_test = [&] (const char* trace, const boost::system::error_code& error, const bool ne = false)
    {
      SCOPED_TRACE(trace);
      const auto result = std::make_shared<error_promise>();
      auto future = result->get_future();
      ASSERT_TRUE(
        local.connect_async(
          "127.0.0.1",
          std::to_string(remote.port()),
          timeout,
          [=] (const auto&, const auto& error) { result->set_value(error); },
          "0.0.0.0",
          options
        )
      );
      ASSERT_EQ(future.wait_for(fail_timeout), boost::future_status::ready);
      if (ne) { EXPECT_NE(future.get(), error); }
      else { EXPECT_EQ(future.get(), error); }
    };

  options = net::ssl_support_t::e_ssl_support_enabled;
  options.verification = net::ssl_verification_t::none;
  run_test("connect fail", {}, true);

  remote.start();
  run_test("encrypt success", {});
  EXPECT_TRUE(remote.sync(1));

  {
    auto fingerprint = remote.get_fingerprint();
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("encrypt+verify success", {});
    EXPECT_TRUE(remote.sync(2));

    fingerprint.at(0)++;
    options = net::ssl_options_t{{fingerprint}, {}};
    run_test("verification fail", cert_check_failed());
    EXPECT_TRUE(remote.sync(3));
  }

  options = net::ssl_support_t::e_ssl_support_autodetect;
  options.verification = net::ssl_verification_t::none;
  run_test("autodetect success", {});
  EXPECT_TRUE(remote.sync(4));
  EXPECT_EQ(remote.handshake(), boost::system::error_code{});

  local.get_config_object().out = epee::byte_slice{std::string(1024, 'A')};
  options = net::ssl_support_t::e_ssl_support_disabled;
  run_test("no encrypt", {});
  EXPECT_TRUE(remote.sync(5));
  EXPECT_EQ(remote.handshake(), bad_version());

  EXPECT_EQ(local.get_config_object().connections, 4);
}
