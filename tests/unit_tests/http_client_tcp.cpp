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

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>

#include "net/http_client.h"

namespace
{
namespace beast_http = boost::beast::http;
namespace http = epee::net_utils::http;

constexpr const std::chrono::seconds timeout{10};

class script_server
{
public:
  struct reply
  {
    std::string bytes;
    bool close = false;
  };

  explicit script_server(std::vector<std::vector<reply>> connections)
    : io_(), acceptor_(io_, {boost::asio::ip::make_address("127.0.0.1"), 0}),
      connections_(std::move(connections)), thread_([this] { run(); })
  {}

  ~script_server()
  {
    // Closing the acceptor from this thread may not wake a blocked `accept`, so
    // a connection does, if the worker is still waiting for one
    stopping_ = true;
    boost::system::error_code ignored;
    {
      boost::asio::io_context io;
      boost::asio::ip::tcp::socket wake{io};
      wake.connect(acceptor_.local_endpoint(), ignored);
    }
    thread_.join();
    acceptor_.close(ignored);
  }

  std::string port() const
  {
    return std::to_string(acceptor_.local_endpoint().port());
  }

  std::vector<beast_http::request<beast_http::string_body>> requests()
  {
    const std::lock_guard<std::mutex> lock{mutex_};
    return requests_;
  }

  std::size_t accepted()
  {
    const std::lock_guard<std::mutex> lock{mutex_};
    return accepted_;
  }

private:
  void run()
  {
    for (const auto& replies : connections_)
    {
      boost::asio::ip::tcp::socket socket{io_};
      boost::system::error_code error;
      acceptor_.accept(socket, error);
      if (error || stopping_)
        return;
      {
        const std::lock_guard<std::mutex> lock{mutex_};
        ++accepted_;
      }

      boost::beast::flat_buffer buffer;
      for (const auto& r : replies)
      {
        beast_http::request<beast_http::string_body> req;
        beast_http::read(socket, buffer, req, error);
        if (error)
          break;
        {
          const std::lock_guard<std::mutex> lock{mutex_};
          requests_.push_back(std::move(req));
        }
        boost::asio::write(socket, boost::asio::buffer(r.bytes), error);
        if (error || r.close)
          break;
      }
      socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
      socket.close(error);
    }
  }

  boost::asio::io_context io_;
  boost::asio::ip::tcp::acceptor acceptor_;
  const std::vector<std::vector<reply>> connections_;
  std::mutex mutex_;
  std::vector<beast_http::request<beast_http::string_body>> requests_;
  std::size_t accepted_ = 0;
  std::atomic<bool> stopping_{false};
  std::thread thread_; // last
};

void connect(http::http_simple_client& client, const script_server& server)
{
  client.set_server("127.0.0.1", server.port(), boost::none,
                    epee::net_utils::ssl_support_t::e_ssl_support_disabled);
}

std::string to_string(const boost::beast::string_view s)
{
  return {s.data(), s.size()};
}
} // anonymous

TEST(http_client_tcp, keep_alive_reuses_connection)
{
  script_server server{{{
    {"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 11\r\n\r\n{\"ok\":true}"},
    {"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nab\r\n1\r\nc\r\n0\r\n\r\n"},
  }}};

  http::http_simple_client client;
  connect(client, server);

  const http::http_response_info* info = nullptr;
  ASSERT_TRUE(client.invoke("/json_rpc", "POST", "{\"x\":1}", timeout, &info, {{"X-Test", "abc"}}));
  ASSERT_NE(nullptr, info);
  EXPECT_EQ(200, info->m_response_code);
  EXPECT_EQ("{\"ok\":true}", info->m_body);
  EXPECT_EQ("application/json", info->m_header_info.m_content_type);
  EXPECT_TRUE(client.is_connected());

  ASSERT_TRUE(client.invoke_get("/get_info", timeout, std::string{}, &info));
  EXPECT_EQ("abc", info->m_body);
  EXPECT_EQ(1u, server.accepted());

  const auto requests = server.requests();
  ASSERT_EQ(2u, requests.size());
  EXPECT_EQ(beast_http::verb::post, requests[0].method());
  EXPECT_EQ("/json_rpc", to_string(requests[0].target()));
  EXPECT_EQ(11u, requests[0].version());
  EXPECT_EQ("127.0.0.1", to_string(requests[0][beast_http::field::host]));
  EXPECT_EQ("abc", to_string(requests[0]["X-Test"]));
  EXPECT_EQ("{\"x\":1}", requests[0].body());
  EXPECT_EQ(beast_http::verb::get, requests[1].method());
  EXPECT_EQ("/get_info", to_string(requests[1].target()));
  EXPECT_EQ("0", to_string(requests[1][beast_http::field::content_length]));
  EXPECT_TRUE(requests[1].body().empty());
}

TEST(http_client_tcp, reconnects_after_close)
{
  const script_server::reply close{"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 1\r\n\r\na", true};
  script_server server{{{close}, {close}}};

  http::http_simple_client client;
  connect(client, server);

  const http::http_response_info* info = nullptr;
  ASSERT_TRUE(client.invoke_get("/a", timeout, std::string{}, &info));
  EXPECT_EQ("a", info->m_body);
  EXPECT_FALSE(client.is_connected());

  ASSERT_TRUE(client.invoke_get("/b", timeout, std::string{}, &info));
  EXPECT_EQ("a", info->m_body);
  EXPECT_EQ(2u, server.accepted());
}

TEST(http_client_tcp, rejects_truncated_body)
{
  script_server server{{{{"HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nabc", true}}}};

  http::http_simple_client client;
  connect(client, server);

  const http::http_response_info* info = nullptr;
  EXPECT_FALSE(client.invoke_get("/", timeout, std::string{}, &info));
  EXPECT_EQ(nullptr, info);
  EXPECT_FALSE(client.is_connected());
}

TEST(http_client_tcp, reads_close_delimited_body)
{
  script_server server{{{{"HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nuntil the end", true}}}};

  http::http_simple_client client;
  connect(client, server);

  const http::http_response_info* info = nullptr;
  ASSERT_TRUE(client.invoke_get("/", timeout, std::string{}, &info));
  EXPECT_EQ("until the end", info->m_body);
  EXPECT_FALSE(client.is_connected());
}

// Beast's default body limit is 8 MiB
TEST(http_client_tcp, reads_large_body)
{
  const std::string body(26 * 1024 * 1024, 'f');
  script_server server{{{
    {"HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body},
    {"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + epee::string_tools::to_string_hex(body.size()) +
       "\r\n" + body + "\r\n0\r\n\r\n"},
  }}};

  http::http_simple_client client;
  connect(client, server);

  const http::http_response_info* info = nullptr;
  ASSERT_TRUE(client.invoke_get("/getblocks.bin", timeout, std::string{}, &info));
  EXPECT_EQ(body.size(), info->m_body.size());
  EXPECT_TRUE(info->m_body == body);

  ASSERT_TRUE(client.invoke_get("/getblocks.bin", timeout, std::string{}, &info));
  EXPECT_EQ(body.size(), info->m_body.size());
  EXPECT_TRUE(info->m_body == body);
}

TEST(http_client_tcp, rejects_invalid_request)
{
  script_server server{{}};

  http::http_simple_client client;
  connect(client, server);

  EXPECT_FALSE(client.invoke("/ HTTP/1.1\r\nX-Evil: 1\r\n\r\nGET /", "GET", "", timeout));
  EXPECT_FALSE(client.invoke("/", "GET", "", timeout, nullptr, {{"X-Test", "a\r\nX-Evil: 1"}}));
  EXPECT_TRUE(server.requests().empty());
}
