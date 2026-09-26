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
// 

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#ifndef _WIN32
#include <sys/resource.h>
#endif
#include "gtest/gtest.h"
#include "misc_language.h"
#include "net/http_server_handlers_map2.h"
#include "net/http_server_impl_base.h"
#include "storages/portable_storage_template_helper.h"

namespace
{
  constexpr const std::size_t payload_size = 26 * 1024 * 1024;
  constexpr const std::size_t max_private_ips = 25;
  struct dummy
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload)
      END_KV_SERIALIZE_MAP()

      std::string payload;
    };
  };

  std::string make_payload()
  {
    dummy::request body{};
    const auto body_serialized = epee::serialization::store_t_to_binary(body);
    return std::string{
      reinterpret_cast<const char*>(body_serialized.data()),
      body_serialized.size()
    };
  }

  struct http_server :  epee::http_server_impl_base<http_server>
  {
    using connection_context =  epee::net_utils::connection_context_base;

    http_server()
      : epee::http_server_impl_base<http_server>(),
        dummy_size(payload_size)
    {}

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      MAP_URI_AUTO_BIN2("/dummy", on_dummy, dummy)
    END_URI_MAP2()

    bool on_dummy(const dummy::request&, dummy::response& res, const connection_context *ctx = NULL)
    {
      res.payload.resize(dummy_size.load(), 'f');
      return true;
    }

    std::atomic<std::size_t> dummy_size;
  };
} // anonymous

TEST(http_server, per_ip_timeout_shift)
{
  using connection = epee::net_utils::connection<epee::net_utils::http::http_custom_handler<epee::net_utils::connection_context_base>>;
  using epee::net_utils::detail::get_per_ip_timeout_shift;
  connection::shared_state state;
  const epee::net_utils::network_address local = epee::net_utils::ipv4_network_address{0x0100007f, 0};
  const epee::net_utils::network_address remote = epee::net_utils::ipv4_network_address{0x010200c0, 0};
  state.m_max_public_ip_connections = 3;
  state.m_max_private_ip_connections = 200;

  struct test_case
  {
    bool local;
    unsigned count;
    unsigned shift;
    unsigned expected;
  };
  const test_case cases[] = {
    {false, 1, 0, 0}, {false, 2, 1, 1}, {false, 3, 2, 2},
    {true, 0, 8, 0}, {true, 9, 8, 0}, {true, 24, 8, 0},
    {true, 25, 8, 1}, {true, 50, 8, 2}, {true, 121, 8, 4},
    {true, 150, 8, 6}, {true, 175, 8, 7}, {true, 199, 8, 7},
    {true, 200, 8, 8}, {true, 201, 8, 8}, {true, 200, 0, 0}
  };
  for (const auto &test : cases)
  {
    SCOPED_TRACE(test.count);
    const auto &address = test.local ? local : remote;
    state.m_connections[epee::net_utils::http::get_rpc_connection_limit_key(address)] = test.count;
    EXPECT_EQ(test.expected, get_per_ip_timeout_shift(state, address, test.shift));
  }

  state.m_max_private_ip_connections = 0;
  EXPECT_EQ(0u, get_per_ip_timeout_shift(state, local, 8));
  state.m_max_private_ip_connections = std::numeric_limits<unsigned>::max();
  state.m_connections[local.host_str()] = std::numeric_limits<unsigned>::max();
  EXPECT_EQ(8u, get_per_ip_timeout_shift(state, local, 8));
}

TEST(http_server, per_ip_timeout_shift_uses_server_count)
{
  using connection = epee::net_utils::connection<epee::net_utils::http::http_custom_handler<epee::net_utils::connection_context_base>>;
  using epee::net_utils::detail::get_per_ip_timeout_shift;
  const epee::net_utils::network_address address = epee::net_utils::ipv4_network_address{0x0100007f, 0};
  connection::shared_state first;
  connection::shared_state second;
  first.m_max_private_ip_connections = second.m_max_private_ip_connections = 200;
  first.m_connections[address.host_str()] = 25;

  EXPECT_EQ(1u, get_per_ip_timeout_shift(first, address, 8));
  EXPECT_EQ(0u, get_per_ip_timeout_shift(second, address, 8));
  EXPECT_TRUE(second.m_connections.empty());

  second.m_connections[address.host_str()] = 200;
  EXPECT_EQ(8u, get_per_ip_timeout_shift(second, address, 8));
  EXPECT_EQ(1u, get_per_ip_timeout_shift(first, address, 8));
  first.m_connections.clear();
  EXPECT_EQ(0u, get_per_ip_timeout_shift(first, address, 8));
  EXPECT_TRUE(first.m_connections.empty());
}

TEST(http_server, per_ip_timeout_shift_groups_public_ipv6)
{
  using connection = epee::net_utils::connection<epee::net_utils::http::http_custom_handler<epee::net_utils::connection_context_base>>;
  using epee::net_utils::detail::get_per_ip_timeout_shift;
  using epee::net_utils::ipv6_network_address;
  using boost::asio::ip::make_address_v6;
  connection::shared_state state;
  state.m_max_public_ip_connections = 200;
  state.m_max_private_ip_connections = 25;
  state.m_connections["2001:db8:abcd:1234::/64"] = 25;

  const ipv6_network_address first{make_address_v6("2001:db8:abcd:1234::1"), 1000};
  const ipv6_network_address second{make_address_v6("2001:db8:abcd:1234::2"), 2000};
  const ipv6_network_address other{make_address_v6("2001:db8:abcd:1235::1"), 1000};
  EXPECT_EQ(1u, get_per_ip_timeout_shift(state, first, 8));
  EXPECT_EQ(1u, get_per_ip_timeout_shift(state, second, 8));
  EXPECT_EQ(0u, get_per_ip_timeout_shift(state, other, 8));
  EXPECT_EQ(0u, get_per_ip_timeout_shift(state, first, 0));
  EXPECT_EQ(1u, state.m_connections.size());

  state.m_connections["2001:db8:abcd:1234::/64"] = 200;
  EXPECT_EQ(8u, get_per_ip_timeout_shift(state, second, 8));
  EXPECT_EQ(2u, get_per_ip_timeout_shift(state, second, 2));

  state.m_connections["::1"] = 25;
  EXPECT_EQ(8u, get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("::1"), 0}, 8));
  state.m_connections["fe80::1"] = 25;
  EXPECT_EQ(8u, get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("fe80::1"), 0}, 8));
  EXPECT_EQ(0u, get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("fe80::2"), 0}, 8));
}

TEST(http_server, timeout_shift_respects_server_capacity)
{
  using connection = epee::net_utils::connection<epee::net_utils::http::http_custom_handler<epee::net_utils::connection_context_base>>;
  using epee::net_utils::detail::get_per_ip_timeout_shift;
  const epee::net_utils::network_address address = epee::net_utils::ipv4_network_address{0x0100007f, 0};
  connection::shared_state state;
  state.m_max_private_ip_connections = 100;
  state.m_max_connections = 200;
  state.m_connections[address.host_str()] = 11;

  struct test_case
  {
    std::size_t count;
    unsigned expected;
  };
  const test_case cases[] = {
    {11, 0}, {24, 0}, {25, 1}, {50, 2}, {99, 3}, {100, 4},
    {124, 4}, {125, 5}, {149, 5}, {150, 6}, {174, 6}, {175, 7},
    {199, 7}, {200, 8}, {201, 8}
  };
  for (const auto &test : cases)
  {
    SCOPED_TRACE(test.count);
    state.m_connection_count = test.count;
    for (unsigned shift = 0; shift <= 8; ++shift)
    {
      SCOPED_TRACE(shift);
      EXPECT_EQ(std::min(shift, test.expected), get_per_ip_timeout_shift(state, address, shift));
    }
  }

  state.m_connection_count = 199;
  state.m_max_private_ip_connections = 0;
  EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
  state.m_connections.clear();
  EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
  EXPECT_TRUE(state.m_connections.empty());

  state.m_max_connections = std::numeric_limits<unsigned>::max();
  state.m_connection_count = state.m_max_connections - 1;
  EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
  state.m_max_connections = state.m_connection_count = 0;
  EXPECT_EQ(8u, get_per_ip_timeout_shift(state, address, 8));
  EXPECT_EQ(0u, get_per_ip_timeout_shift(state, address, 0));

  state.m_max_connections = 1000;
  state.m_connection_count = 125;
  state.m_max_private_ip_connections = 100;
  state.m_connections[address.host_str()] = 50;
  EXPECT_EQ(4u, get_per_ip_timeout_shift(state, address, 8));
}

TEST(http_server, initial_timeout_releases_capacity)
{
#ifndef _WIN32
  // Allow room for both client and server sockets, then restore the original limit.
  rlimit file_limit;
  ASSERT_EQ(0, getrlimit(RLIMIT_NOFILE, &file_limit));
  auto restore_file_limit = epee::misc_utils::create_scope_leave_handler([file_limit]{
    if (file_limit.rlim_cur < 1024)
      EXPECT_EQ(0, setrlimit(RLIMIT_NOFILE, &file_limit));
  });
  if (file_limit.rlim_cur < 1024)
  {
    ASSERT_GE(file_limit.rlim_max, 512u) << "This test needs at least 512 file descriptors";
    rlimit raised_limit = file_limit;
    raised_limit.rlim_cur = std::min<rlim_t>(1024, file_limit.rlim_max);
    ASSERT_EQ(0, setrlimit(RLIMIT_NOFILE, &raised_limit));
  }
#endif
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;
  struct server_t : http_server
  {
    using http_server::m_net_server;
  };

  server_t server;
  server.dummy_size = 1;
  ASSERT_TRUE(server.init(nullptr, "0", "127.0.0.1", "::", false, true, {}, {},
    epee::net_utils::ssl_support_t::e_ssl_support_disabled, 200, 200, 200));
  auto& context = server.m_net_server.get_io_context();
  auto& config = server.m_net_server.get_config_object();
  const tcp::endpoint endpoint{boost::asio::ip::make_address("127.0.0.1"),
    static_cast<unsigned short>(server.get_binded_port())};
  std::vector<tcp::socket> streams;
  for (std::size_t i = 0; i < 204; ++i)
  {
    streams.emplace_back(context);
    streams.back().connect(endpoint);
    context.poll();
  }
  ASSERT_EQ(200u, config.m_connection_count);

  // At capacity the last connection must use the counted load: 7 seconds, not 14.
  context.run_for(std::chrono::seconds(10));
  ASSERT_LT(config.m_connection_count, 200u);
  EXPECT_GE(config.m_connection_count, 150u);

  tcp::socket stream{context};
  stream.connect(endpoint);
  http::request<http::string_body> req{http::verb::get, "/dummy", 11};
  req.set(http::field::host, "127.0.0.1");
  req.body() = make_payload();
  req.prepare_payload();
  http::write(stream, req);

  bool received = false;
  boost::beast::flat_buffer buffer;
  http::response<http::string_body> response;
  http::async_read(stream, buffer, response,
    [&](const boost::system::error_code& error, std::size_t)
    {
      EXPECT_FALSE(bool(error));
      received = true;
    });
  context.run_for(std::chrono::seconds(2));
  EXPECT_TRUE(received);
  EXPECT_EQ(200u, response.result_int());
  stream.close();
  context.poll();
  server.send_stop_signal();
}

TEST(http_server, response_soft_limit)
{
  namespace http = boost::beast::http;

  http_server server{};
  server.init(nullptr, "8080");
  server.run(1, false);

  boost::system::error_code error{};
  boost::asio::io_context context{};
  boost::asio::ip::tcp::socket stream{context};
  stream.connect(
    boost::asio::ip::tcp::endpoint{
      boost::asio::ip::make_address("127.0.0.1"), 8080
    },
    error
  );
  EXPECT_FALSE(bool(error));

  http::request<http::string_body> req{http::verb::get, "/dummy", 11};
  req.set(http::field::host, "127.0.0.1");
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.body() = make_payload();
  req.prepare_payload();
  http::write(stream, req, error);
  EXPECT_FALSE(bool(error));

  {
    dummy::response payload{};
    boost::beast::flat_buffer buffer;
    http::response<http::basic_string_body<char>> res;
    http::read(stream, buffer, res, error);
    EXPECT_FALSE(bool(error));
    EXPECT_EQ(200u, res.result_int());
    EXPECT_TRUE(epee::serialization::load_t_from_binary(payload, res.body()));
    EXPECT_EQ(payload_size, std::count(payload.payload.begin(), payload.payload.end(), 'f'));
  }

  while (!error)
    http::write(stream, req, error);
  server.send_stop_signal();
}

TEST(http_server, private_ip_limit)
{
  namespace http = boost::beast::http;

  http_server server{};
  server.dummy_size = 1;
  server.init(nullptr, "8080");
  server.run(1, false);

  boost::system::error_code error{};
  boost::asio::io_context context{};

  http::request<http::string_body> req{http::verb::get, "/dummy", 11};
  req.set(http::field::host, "127.0.0.1");
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.body() = make_payload();
  req.prepare_payload();

  std::vector<boost::asio::ip::tcp::socket> streams{};
  for (std::size_t i = 0; i < max_private_ips; ++i)
  {
    streams.emplace_back(context);
    streams.back().connect(
      boost::asio::ip::tcp::endpoint{
        boost::asio::ip::make_address("127.0.0.1"), 8080
      },
      error
    );
    http::write(streams.back(), req, error);
    EXPECT_FALSE(bool(error));

    dummy::response payload{};
    boost::beast::flat_buffer buffer;
    http::response<http::basic_string_body<char>> res;

    http::read(streams.back(), buffer, res, error);
    EXPECT_FALSE(bool(error));
  }

  boost::asio::ip::tcp::socket stream{context};
  stream.connect(
    boost::asio::ip::tcp::endpoint{
      boost::asio::ip::make_address("127.0.0.1"), 8080
    },
    error
  );
  bool failed = bool(error);
  http::write(stream, req, error);
  failed |= bool(error);
  {
    dummy::response payload{};
    boost::beast::flat_buffer buffer;
    http::response<http::basic_string_body<char>> res;

    // make sure server ran async_accept code
    http::read(stream, buffer, res, error);
  }
  failed |= bool(error);
  EXPECT_TRUE(failed);
}

TEST(http_server, read_then_close)
{
  namespace http = boost::beast::http;

  http_server server{};
  server.dummy_size = 200000;
  server.init(nullptr, "8080");
  server.run(2, false); // need at least 2 threads to trigger issues

  bool failed_read = false;
  bool closed_all_connections = true;
  for (std::size_t j = 0; j < 1000; ++j)
  {
    boost::system::error_code error{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket stream{context};
    stream.connect(
      boost::asio::ip::tcp::endpoint{
        boost::asio::ip::make_address("127.0.0.1"), 8080
      },
      error
    );
    EXPECT_FALSE(bool(error));

    http::request<http::string_body> req{http::verb::get, "/dummy", 11};
    req.set(http::field::host, "127.0.0.1");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::connection, "close"); // tell server to close connection after sending all data to the client
    req.body() = make_payload();
    req.prepare_payload();

    dummy::response payload{};
    boost::beast::flat_buffer buffer;
    http::response_parser<http::basic_string_body<char>> parser;
    parser.body_limit(server.dummy_size + 1024);

    http::write(stream, req, error);
    EXPECT_FALSE(bool(error));

    http::read(stream, buffer, parser, error);

    // If the read fails, continue the loop still just to make sure the server can handle it
    failed_read |= bool(error);
    if (failed_read)
      continue;
    failed_read |= !(parser.is_done());
    if (failed_read)
      continue;
    const auto res = parser.release();
    failed_read |= res.result_int() != 200u
        || !(epee::serialization::load_t_from_binary(payload, res.body()))
        || (server.dummy_size != std::count(payload.payload.begin(), payload.payload.end(), 'f'));

    // See if the server closes the connection after handling the resp
    char buf[1];
    stream.read_some(boost::asio::buffer(buf), error);
    closed_all_connections &= error == boost::asio::error::eof;
  }

  // The client should have been able to read all data sent by the server across all requests
  EXPECT_FALSE(failed_read);

  // The server should have closed all connections
  EXPECT_TRUE(closed_all_connections);

  server.send_stop_signal();
}
