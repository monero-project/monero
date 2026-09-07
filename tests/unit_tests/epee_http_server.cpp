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
  // Reading a complete oversized response must allow another on the same
  // keep-alive connection, even with only one worker available.
  for (unsigned i = 0; i < 4; ++i)
  {
    http::write(stream, req, error);
    ASSERT_FALSE(bool(error));

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

TEST(http_server, send_failure_stops_requests)
{
  namespace http = epee::net_utils::http;

  struct endpoint_t final : epee::net_utils::i_service_endpoint
  {
    bool do_send(epee::byte_slice) override
    {
      ++send_count;
      return allow_send;
    }
    bool send_done() override { ++done_count; return true; }
    bool close(bool) override { return true; }
    bool call_run_once_service_io() override { return true; }
    bool request_callback() override { return true; }
    boost::asio::io_context& get_io_context() override { return context; }

    boost::asio::io_context context;
    bool allow_send = true;
    std::size_t send_count = 0;
    std::size_t done_count = 0;
  };

  struct handler_t final : http::simple_http_connection_handler<>
  {
    using http::simple_http_connection_handler<>::simple_http_connection_handler;

    bool handle_request(const http::http_request_info&, http::http_response_info& response) override
    {
      ++request_count;
      response.m_response_code = 200;
      response.m_response_comment = "OK";
      return true;
    }

    std::size_t request_count = 0;
  };

  // Exercise all three response paths: absent, empty, and nonempty bodies.
  const std::string requests[] = {
    "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
    "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n",
    "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1\r\n\r\nx"
  };
  for (const auto& request : requests)
  {
    for (const auto& input : {request, request + request})
    {
      for (const std::size_t successful_requests : {0u, 1u})
      {
        SCOPED_TRACE(input);
        SCOPED_TRACE(successful_requests);
        endpoint_t endpoint;
        http::http_server_config config;
        epee::net_utils::connection_context_base context;
        handler_t handler{&endpoint, config, context};

        for (std::size_t i = 0; i < successful_requests; ++i)
          ASSERT_TRUE(handler.handle_recv(request.data(), request.size()));

        endpoint.allow_send = false;
        // Check both an exhausted input buffer and another pipelined request.
        EXPECT_FALSE(handler.handle_recv(input.data(), input.size()));
        EXPECT_EQ(successful_requests + 1, handler.request_count);
        EXPECT_EQ(successful_requests + 1, endpoint.send_count);
        EXPECT_EQ(successful_requests, endpoint.done_count);

        // A failed response must leave the parser in its terminal error state.
        EXPECT_FALSE(handler.handle_recv(request.data(), request.size()));
        EXPECT_EQ(successful_requests + 1, handler.request_count);
        EXPECT_EQ(successful_requests + 1, endpoint.send_count);
      }
    }
  }
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
