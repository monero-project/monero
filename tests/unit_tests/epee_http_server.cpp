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

#include <atomic>
#include <cstdint>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "gtest/gtest.h"
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

  struct json_rpc_dummy
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()

      std::uint64_t value;
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()

      std::uint64_t value;
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
        dummy_size(payload_size),
        json_rpc_calls(0)
    {}

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      MAP_URI_AUTO_BIN2("/dummy", on_dummy, dummy)
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC("echo", on_echo, json_rpc_dummy)
        MAP_JON_RPC("fail", on_fail, json_rpc_dummy)
        MAP_JON_RPC_WE("fail_with_error", on_fail_with_error, json_rpc_dummy)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

    bool on_dummy(const dummy::request&, dummy::response& res, const connection_context *ctx = NULL)
    {
      res.payload.resize(dummy_size.load(), 'f');
      return true;
    }

    bool on_echo(
        const json_rpc_dummy::request& req,
        json_rpc_dummy::response& res,
        const connection_context *ctx = NULL)
    {
      ++json_rpc_calls;
      res.value = req.value;
      return true;
    }

    bool on_fail(const json_rpc_dummy::request&, json_rpc_dummy::response&, const connection_context *ctx = NULL)
    {
      ++json_rpc_calls;
      return false;
    }

    bool on_fail_with_error(
        const json_rpc_dummy::request&,
        json_rpc_dummy::response&,
        epee::json_rpc::error&,
        const connection_context *ctx = NULL)
    {
      ++json_rpc_calls;
      return false;
    }

    std::atomic<std::size_t> dummy_size;
    std::atomic<std::size_t> json_rpc_calls;
  };

  epee::net_utils::http::http_response_info invoke_json_rpc(http_server& server, const char* body)
  {
    epee::net_utils::http::http_request_info request{};
    request.m_URI = "/json_rpc";
    request.m_body = body;

    epee::net_utils::http::http_response_info response{};
    http_server::connection_context context{};
    EXPECT_TRUE(server.handle_http_request(request, response, context));
    return response;
  }
} // anonymous

TEST(http_server, json_rpc_notifications)
{
  http_server server{};

  const auto notification = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","method":"echo","params":{"value":42}})");
  EXPECT_EQ(200, notification.m_response_code);
  EXPECT_TRUE(notification.m_body.empty());
  EXPECT_EQ(1, server.json_rpc_calls);

  const auto failed_notification = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","method":"fail","params":{"value":42}})");
  EXPECT_EQ(200, failed_notification.m_response_code);
  EXPECT_TRUE(failed_notification.m_body.empty());
  EXPECT_EQ(2, server.json_rpc_calls);

  const auto failed_with_error_notification = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","method":"fail_with_error","params":{"value":42}})");
  EXPECT_EQ(200, failed_with_error_notification.m_response_code);
  EXPECT_TRUE(failed_with_error_notification.m_body.empty());
  EXPECT_EQ(3, server.json_rpc_calls);

  const auto unknown_notification = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","method":"unknown","params":{}})");
  EXPECT_EQ(200, unknown_notification.m_response_code);
  EXPECT_TRUE(unknown_notification.m_body.empty());
  EXPECT_EQ(3, server.json_rpc_calls);

  const auto invalid_notification = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","method":"echo","params":{"value":"invalid"}})");
  EXPECT_EQ(200, invalid_notification.m_response_code);
  EXPECT_TRUE(invalid_notification.m_body.empty());
  EXPECT_EQ(3, server.json_rpc_calls);

  const auto request = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","id":7,"method":"echo","params":{"value":42}})");
  EXPECT_EQ(200, request.m_response_code);
  EXPECT_FALSE(request.m_body.empty());
  EXPECT_EQ(4, server.json_rpc_calls);

  epee::serialization::portable_storage response{};
  ASSERT_TRUE(response.load_from_json(request.m_body));
  std::uint64_t response_id = 0;
  ASSERT_TRUE(response.get_value("id", response_id, nullptr));
  EXPECT_EQ(7, response_id);
  const auto result = response.open_section("result", nullptr);
  ASSERT_NE(nullptr, result);
  std::uint64_t response_value = 0;
  ASSERT_TRUE(response.get_value("value", response_value, result));
  EXPECT_EQ(42, response_value);

  const auto null_id = invoke_json_rpc(
      server, R"({"jsonrpc":"2.0","id":null,"method":"echo","params":{"value":42}})");
  EXPECT_EQ(200, null_id.m_response_code);
  EXPECT_FALSE(null_id.m_body.empty());
  EXPECT_EQ(5, server.json_rpc_calls);
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
    http::response_parser<http::basic_string_body<char>> parser;
    parser.body_limit(payload_size + 1024);
    http::read(stream, buffer, parser, error);
    EXPECT_FALSE(bool(error));
    ASSERT_TRUE(parser.is_done());
    const auto res = parser.release();
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

    boost::beast::flat_buffer buffer;
    http::response_parser<http::basic_string_body<char>> parser;
    parser.body_limit(payload_size + 1024);

    http::read(streams.back(), buffer, parser, error);
    EXPECT_FALSE(bool(error));
    EXPECT_TRUE(parser.is_done());
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
    boost::beast::flat_buffer buffer;
    http::response_parser<http::basic_string_body<char>> parser;
    parser.body_limit(payload_size + 1024);

    // make sure server ran async_accept code
    http::read(stream, buffer, parser, error);
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
