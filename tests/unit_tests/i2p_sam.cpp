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
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "net/i2p_sam.h"
#include "string_coding.h"
#include "file_io_utils.h"

namespace
{
  using boost::asio::ip::tcp;

  std::string encode_i2p_base64(const std::string& binary)
  {
    return net::sam::swap_base64(epee::string_encoding::base64_encode(binary));
  }

  std::string make_destination_bytes(std::uint16_t cert_len, unsigned char fill = 0x11)
  {
    std::string bytes(std::size_t{387} + cert_len, static_cast<char>(fill));
    bytes[384] = static_cast<char>(cert_len ? 5 : 0); // KeyCertificate iff there's a payload
    bytes[385] = static_cast<char>((cert_len >> 8) & 0xFF);
    bytes[386] = static_cast<char>(cert_len & 0xFF);
    return bytes;
  }

  struct fake_sam_router
  {
    enum class accept_response
    {
      peer_destination_then_raw_bytes,
      peer_destination_split_then_raw_bytes,
      peer_destination_and_raw_bytes_same_write,
      stream_status_destination_and_raw_bytes_same_write,
      late_stream_status_error,
      peer_destination_line_too_long,
      eof_before_peer_destination_line,
      empty_peer_destination_line,
      second_stream_status_ok,
      peer_destination_with_ports,
    };

    boost::asio::io_context& io;
    tcp::acceptor acceptor;

    std::shared_ptr<tcp::socket> session_socket;
    std::shared_ptr<tcp::socket> accept_socket;

    std::string session_buffer;
    std::string accept_buffer;

    accept_response response;

    explicit fake_sam_router(
      boost::asio::io_context& io_,
      accept_response response_ = accept_response::peer_destination_then_raw_bytes
    )
      : io(io_)
      , acceptor(
          io_,
          tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), 0}
        )
      , session_socket(std::make_shared<tcp::socket>(io_))
      , accept_socket(std::make_shared<tcp::socket>(io_))
      , response(response_)
    {}

    tcp::endpoint endpoint() const
    {
      return acceptor.local_endpoint();
    }

    template<typename Handler>
    void read_line(
      const std::shared_ptr<tcp::socket>& socket,
      std::string& buffer,
      Handler handler
    )
    {
      boost::asio::async_read_until(
        *socket,
        boost::asio::dynamic_buffer(buffer),
        '\n',
        [socket, &buffer, handler = std::move(handler)]
        (boost::system::error_code ec, std::size_t bytes) mutable
        {
          ASSERT_FALSE(ec) << ec.message();

          std::string line = buffer.substr(0, bytes);
          buffer.erase(0, bytes);

          handler(std::move(line));
        });
    }

    template<typename Handler>
    void write(
      const std::shared_ptr<tcp::socket>& socket,
      std::string message,
      Handler handler
    )
    {
      auto data = std::make_shared<std::string>(std::move(message));

      boost::asio::async_write(
        *socket,
        boost::asio::buffer(*data),
        [socket, data, handler = std::move(handler)]
        (boost::system::error_code ec, std::size_t) mutable
        {
          ASSERT_FALSE(ec) << ec.message();
          handler();
        });
    }

    void write(
      const std::shared_ptr<tcp::socket>& socket,
      std::string message
    )
    {
      auto data = std::make_shared<std::string>(std::move(message));

      boost::asio::async_write(
        *socket,
        boost::asio::buffer(*data),
        [socket, data](boost::system::error_code ec, std::size_t)
        {
          ASSERT_FALSE(ec) << ec.message();
        });
    }

    void start()
    {
      accept_session_create_socket();
    }

    void accept_session_create_socket()
    {
      acceptor.async_accept(
        *session_socket,
        [this](boost::system::error_code ec)
        {
          ASSERT_FALSE(ec) << ec.message();

          // Client should send HELLO first.
          read_line(
            session_socket,
            session_buffer,
            [this](std::string hello)
            {
              EXPECT_EQ(hello, "HELLO VERSION MIN=3.1 MAX=3.1\n");

              write(
                session_socket,
                "HELLO REPLY RESULT=OK VERSION=3.1\n",
                [this]()
                {
                  read_session_create();
                });
            });
        });
    }

    void read_session_create()
    {
      read_line(
        session_socket,
        session_buffer,
        [this](std::string session_create)
        {
          EXPECT_NE(
            session_create.find("SESSION CREATE STYLE=STREAM ID=abcdefghij "),
            std::string::npos
          );

          EXPECT_NE(
            session_create.find("DESTINATION=FAKE_PRIVATE_KEY"),
            std::string::npos
          );

          EXPECT_NE(
            session_create.find("i2cp.leaseSetEncType=6,4"),
            std::string::npos
          );

          // After SESSION CREATE succeeds, the client reconnects and opens the accept socket.
          write(
            session_socket,
            "SESSION STATUS RESULT=OK DESTINATION=FAKE_PRIVATE_KEY\n",
            [this]()
            {
              accept_stream_accept_socket();
            });
        });
    }

    void accept_stream_accept_socket()
    {
      acceptor.async_accept(
        *accept_socket,
        [this](boost::system::error_code ec)
        {
          ASSERT_FALSE(ec) << ec.message();

          // New socket also starts with HELLO.
          read_line(
            accept_socket,
            accept_buffer,
            [this](std::string hello)
            {
              EXPECT_EQ(hello, "HELLO VERSION MIN=3.1 MAX=3.1\n");

              write(
                accept_socket,
                "HELLO REPLY RESULT=OK VERSION=3.1\n",
                [this]()
                {
                  read_stream_accept();
                });
            });
        });
    }

    void read_stream_accept()
    {
      read_line(
        accept_socket,
        accept_buffer,
        [this](std::string stream_accept)
        {
          EXPECT_EQ(stream_accept, "STREAM ACCEPT ID=abcdefghij SILENT=false\n");

          if (response == accept_response::stream_status_destination_and_raw_bytes_same_write)
          {
            // Combine the STREAM STATUS line, destination line, and raw P2P bytes into a
            // single write, so they can arrive in the same underlying socket read. This is
            // the over-read the byte-at-a-time destination reader cannot protect against,
            // since it happens while parsing the STREAM STATUS line itself.
            write(
              accept_socket,
              "STREAM STATUS RESULT=OK\nPEER_BASE64_DESTINATION\nraw-peer-bytes-without-newline"
            );
            return;
          }

          // First SAM status response: accept command is now pending.
          write(
            accept_socket,
            "STREAM STATUS RESULT=OK\n",
            [this]()
            {
              write_accept_response();
            });
        });
    }

    void write_accept_response()
    {
      switch (response)
      {
      case accept_response::peer_destination_then_raw_bytes:
        // Second line per SAMv3 spec with SILENT=false:
        // peer public destination line, then raw stream bytes.
        //
        // This line should be consumed by the SAM layer.
        write(
          accept_socket,
          "PEER_BASE64_DESTINATION\n",
          [this]()
          {
            // These bytes should NOT be consumed by the SAM parser.
            // No newline on purpose. A buggy read_until('\n') can hang or
            // overread/corrupt data.
            write(accept_socket, "raw-peer-bytes-without-newline");
          });
        return;

      case accept_response::peer_destination_split_then_raw_bytes:
        write(
          accept_socket,
          "PEER_",
          [this]()
          {
            write(
              accept_socket,
              "BASE64_",
              [this]()
              {
                write(
                  accept_socket,
                  "DESTINATION\n",
                  [this]()
                  {
                    write(accept_socket, "raw-peer-bytes-without-newline");
                  });
              });
          });
        return;

      case accept_response::peer_destination_and_raw_bytes_same_write:
        write(
          accept_socket,
          "PEER_BASE64_DESTINATION\nraw-peer-bytes-without-newline"
        );
        return;

      case accept_response::late_stream_status_error:
        write(
          accept_socket,
          "STREAM STATUS RESULT=I2P_ERROR MESSAGE=router-restarted\n"
        );
        return;

      case accept_response::peer_destination_line_too_long:
        write(accept_socket, std::string(4096, 'A') + "\n");
        return;

      case accept_response::eof_before_peer_destination_line:
      {
        boost::system::error_code ignored;
        accept_socket->shutdown(tcp::socket::shutdown_both, ignored);
        accept_socket->close(ignored);
        return;
      }

      case accept_response::empty_peer_destination_line:
        write(accept_socket, "\n");
        return;

      case accept_response::second_stream_status_ok:
        write(accept_socket, "STREAM STATUS RESULT=OK\n");
        return;

      case accept_response::peer_destination_with_ports:
        write(accept_socket, "PEER_BASE64_DESTINATION FROM_PORT=1 TO_PORT=2\n");
        return;

      case accept_response::stream_status_destination_and_raw_bytes_same_write:
        return;
      }
    }
  };

  void expect_handed_off_raw_peer_bytes(tcp::socket& socket)
  {
    ASSERT_TRUE(socket.is_open());

    // Verify that raw peer bytes are still available on the handed-off socket.
    // This is the key no-overread assertion.
    std::array<char, 64> buffer{};
    boost::system::error_code read_ec;

    const std::size_t n = socket.read_some(
      boost::asio::buffer(buffer),
      read_ec
    );

    ASSERT_FALSE(read_ec) << read_ec.message();

    const std::string received{buffer.data(), n};
    EXPECT_EQ(received, "raw-peer-bytes-without-newline");
  }

  template<typename Verify>
  void run_control_socket_accept(
    fake_sam_router::accept_response response,
    Verify verify
  )
  {
    boost::asio::io_context io;

    fake_sam_router router{io, response};

    bool handler_called = false;
    boost::system::error_code handler_ec;
    tcp::socket handed_off_socket{io};
    std::string handed_off_data;

    auto client = net::sam::make_control_client(
      "FAKE_PRIVATE_KEY",
      "",
      tcp::socket{io},
      [&](boost::system::error_code ec, tcp::socket socket, net::i2p_address, std::string leftover)
      {
        handler_called = true;
        handler_ec = ec;
        handed_off_socket = std::move(socket);
        handed_off_data = std::move(leftover);
        io.stop();
      });

    client->set_session_id("abcdefghij");

    router.start();

    ASSERT_TRUE(
      net::sam::control_socket::connect_and_send(client, router.endpoint())
    );

    boost::asio::steady_timer deadline{io};
    deadline.expires_after(std::chrono::seconds{3});
    deadline.async_wait(
      [&](boost::system::error_code ec)
      {
        if (!ec)
          io.stop();
      });

    io.run();

    verify(handler_called, handler_ec, handed_off_socket, handed_off_data);
  }

  constexpr const char fake_b32_dest[] = "abcdefghijklmnopqrstuvwxyz234567abcdefghijklmnopqrst.b32.i2p";

  struct fake_connect_router
  {
    boost::asio::io_context& io;
    tcp::acceptor acceptor;
    std::shared_ptr<tcp::socket> socket;
    std::string buffer;

    explicit fake_connect_router(boost::asio::io_context& io_)
      : io(io_)
      , acceptor(io_, tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), 0})
      , socket(std::make_shared<tcp::socket>(io_))
    {}

    tcp::endpoint endpoint() const { return acceptor.local_endpoint(); }

    template<typename Handler>
    void read_line(Handler handler)
    {
      boost::asio::async_read_until(
        *socket, boost::asio::dynamic_buffer(buffer), '\n',
        [this, handler = std::move(handler)](boost::system::error_code ec, std::size_t bytes) mutable
        {
          ASSERT_FALSE(ec) << ec.message();
          std::string line = buffer.substr(0, bytes);
          buffer.erase(0, bytes);
          handler(std::move(line));
        });
    }

    void write(std::string message)
    {
      auto data = std::make_shared<std::string>(std::move(message));
      boost::asio::async_write(
        *socket, boost::asio::buffer(*data),
        [socket = socket, data](boost::system::error_code ec, std::size_t) { ASSERT_FALSE(ec) << ec.message(); });
    }

    void start()
    {
      acceptor.async_accept(*socket, [this](boost::system::error_code ec)
      {
        ASSERT_FALSE(ec) << ec.message();
        read_line([this](std::string hello)
        {
          EXPECT_EQ(hello, "HELLO VERSION MIN=3.1 MAX=3.1\n");
          write("HELLO REPLY RESULT=OK VERSION=3.1\n");
          read_line([this](std::string stream_connect)
          {
            EXPECT_EQ(stream_connect, std::string("STREAM CONNECT ID=abcdefghij DESTINATION=") + fake_b32_dest + " SILENT=false\n");
            write("STREAM STATUS RESULT=OK\nhello-from-peer");
          });
        });
      });
    }
  };
}

TEST(i2p_sam, control_socket_accept_hands_off_after_peer_destination_line)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::peer_destination_then_raw_bytes,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket& handed_off_socket, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      ASSERT_FALSE(handler_ec) << handler_ec.message();
      expect_handed_off_raw_peer_bytes(handed_off_socket);
    });
}

TEST(i2p_sam, control_socket_accept_reports_late_stream_status_error)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::late_stream_status_error,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_EQ(handler_ec, net::sam::make_error_code(net::sam::error::i2p_error));
    });
}

TEST(i2p_sam, control_socket_accept_handles_split_peer_destination_line)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::peer_destination_split_then_raw_bytes,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket& handed_off_socket, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      ASSERT_FALSE(handler_ec) << handler_ec.message();
      expect_handed_off_raw_peer_bytes(handed_off_socket);
    });
}

TEST(i2p_sam, control_socket_accept_leaves_same_write_raw_peer_bytes_unread)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::peer_destination_and_raw_bytes_same_write,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket& handed_off_socket, const std::string& handed_off_data)
    {
      ASSERT_TRUE(handler_called);
      ASSERT_FALSE(handler_ec) << handler_ec.message();
      EXPECT_TRUE(handed_off_data.empty());
      expect_handed_off_raw_peer_bytes(handed_off_socket);
    });
}

TEST(i2p_sam, control_socket_accept_rejects_too_long_peer_destination_line)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::peer_destination_line_too_long,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_EQ(handler_ec, net::sam::make_error_code(net::sam::error::parse_failed));
    });
}

TEST(i2p_sam, control_socket_accept_reports_eof_before_peer_destination_line)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::eof_before_peer_destination_line,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_TRUE(handler_ec);
    });
}

TEST(i2p_sam, control_socket_accept_rejects_empty_peer_destination_line)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::empty_peer_destination_line,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_EQ(handler_ec, net::sam::make_error_code(net::sam::error::parse_failed));
    });
}

TEST(i2p_sam, control_socket_accept_rejects_second_stream_status_ok)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::second_stream_status_ok,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_TRUE(handler_ec);
    });
}

TEST(i2p_sam, control_socket_accept_rejects_peer_destination_with_sam32_ports)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::peer_destination_with_ports,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string&)
    {
      ASSERT_TRUE(handler_called);
      EXPECT_EQ(handler_ec, net::sam::make_error_code(net::sam::error::parse_failed));
    });
}

TEST(i2p_sam, control_socket_accept_hands_off_buffered_data_when_status_and_destination_arrive_together)
{
  run_control_socket_accept(
    fake_sam_router::accept_response::stream_status_destination_and_raw_bytes_same_write,
    [](bool handler_called, boost::system::error_code handler_ec, tcp::socket&, const std::string& handed_off_data)
    {
      ASSERT_TRUE(handler_called);
      ASSERT_FALSE(handler_ec) << handler_ec.message();
      EXPECT_EQ(handed_off_data, "raw-peer-bytes-without-newline");
    });
}

TEST(i2p_sam, get_value_ok)
{
    const std::string message = "HELLO REPLY RESULT=OK VERSION=3.1";
    std::string out{};
    auto ec = net::sam::get_value(message, "RESULT", out);
    EXPECT_FALSE(ec);
    EXPECT_EQ(out, "OK");

    const std::string message2 = "HELLO REPLY RESULT=OK VERSION=3.1\n";
    std::string out2{};
    auto ec2 = net::sam::get_value(message2, "RESULT", out2);
    EXPECT_FALSE(ec2);
    EXPECT_EQ(out2, "OK");

    const std::string message3 = "NAMING REPLY NAME=foobar\r\n";
    std::string out3{};
    auto ec3 = net::sam::get_value(message3, "NAME", out3);
    EXPECT_FALSE(ec3);
    EXPECT_EQ(out3, "foobar");
}

TEST(i2p_sam, get_value_error)
{
    // Nonexistent key
    const std::string message = "HELLO REPLY RESULT=OK VERSION=3.1";
    std::string out{};
    auto ec = net::sam::get_value(message, "NAME", out);
    EXPECT_TRUE(ec);

    // Nonexistent value
    const std::string message2 = "HELLO REPLY RESULT=OK VERSION=3.1";
    std::string out2{};
    auto ec2 = net::sam::get_value(message2, "HELLO", out2);
    EXPECT_TRUE(ec2);
}

TEST(i2p_sam, get_value_at_start_of_line)
{
    std::string out;
    auto ec = net::sam::get_value("RESULT=OK", "RESULT", out);

    EXPECT_FALSE(ec);
    EXPECT_EQ(out, "OK");
}

TEST(i2p_sam, get_value_after_space)
{
    std::string out;
    auto ec = net::sam::get_value("FOO RESULT=OKAY", "RESULT", out);

    EXPECT_FALSE(ec);
    EXPECT_EQ(out, "OKAY");
}

TEST(i2p_sam, get_value_empty_value)
{
    std::string out = "unchanged";
    auto ec = net::sam::get_value("RESULT=", "RESULT", out);

    EXPECT_FALSE(ec);
    EXPECT_EQ(out, "");
}

TEST(i2p_sam, get_value_does_not_match_key_inside_value)
{
    std::string out;
    auto ec = net::sam::get_value("MESSAGE=BAD_RESULT=OK", "RESULT", out);

    EXPECT_TRUE(ec);
    EXPECT_EQ(ec, net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, sanitize_private_key_strips_surrounding_whitespace)
{
    EXPECT_EQ(net::sam::sanitize_private_key("abc123\n"), "abc123");
    EXPECT_EQ(net::sam::sanitize_private_key("abc123\r\n"), "abc123");
    EXPECT_EQ(net::sam::sanitize_private_key("  abc123\t"), "abc123");
    EXPECT_EQ(net::sam::sanitize_private_key("\n\r abc1 23 \r\n"), "abc123");
}

TEST(i2p_sam, sanitize_private_key_strips_interior_whitespace)
{
    EXPECT_EQ(net::sam::sanitize_private_key("abc\n123\ndef"), "abc123def");
    EXPECT_EQ(net::sam::sanitize_private_key("abc 123\tdef"), "abc123def");
}

TEST(i2p_sam, sanitize_private_key_unchanged_and_empty)
{
    EXPECT_EQ(net::sam::sanitize_private_key("abc123"), "abc123");
    EXPECT_EQ(net::sam::sanitize_private_key(""), "");
    EXPECT_EQ(net::sam::sanitize_private_key("   \r \n\t"), "");
}

TEST(i2p_sam, parse_result_code)
{
    EXPECT_FALSE(net::sam::parse_result_code("RESULT=OK"));

    EXPECT_EQ(net::sam::parse_result_code("RESULT=I2P_ERROR"), net::sam::make_error_code(net::sam::error::i2p_error));
    EXPECT_EQ(net::sam::parse_result_code("RESULT=TIMEOUT"), net::sam::make_error_code(net::sam::error::timeout));
    EXPECT_EQ(net::sam::parse_result_code("RESULT=NOVERSION"), net::sam::make_error_code(net::sam::error::noversion));
    EXPECT_EQ(net::sam::parse_result_code("RESULT=DUPLICATED_ID"), net::sam::make_error_code(net::sam::error::duplicated_id));
    EXPECT_EQ(net::sam::parse_result_code("RESULT=ALREADY_ACCEPTING"), net::sam::make_error_code(net::sam::error::already_accepting));

    EXPECT_EQ(net::sam::parse_result_code("RESULT=AAAAAA121212"), net::sam::make_error_code(net::sam::error::parse_failed));
    EXPECT_EQ(net::sam::parse_result_code("HELLO REPLY VERSION=3.1"), net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, parse_hello_reply)
{
    EXPECT_FALSE(net::sam::parse_hello_reply("HELLO REPLY RESULT=OK VERSION=3.1"));

    EXPECT_EQ(net::sam::parse_hello_reply("STREAM STATUS RESULT=OK"), net::sam::make_error_code(net::sam::error::parse_failed));
    EXPECT_EQ(net::sam::parse_hello_reply("RESULT=OK"), net::sam::make_error_code(net::sam::error::parse_failed));

    EXPECT_EQ(net::sam::parse_hello_reply("HELLO REPLYX RESULT=OK"), net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, parse_session_status)
{
    std::string destination;
    EXPECT_FALSE(net::sam::parse_session_status("SESSION STATUS RESULT=OK DESTINATION=abc", destination));
    EXPECT_EQ(destination, "abc");

    std::string ignored;
    EXPECT_EQ(net::sam::parse_session_status("SESSION STATUS RESULT=I2P_ERROR DESTINATION=abc", ignored), net::sam::make_error_code(net::sam::error::i2p_error));

    EXPECT_EQ(net::sam::parse_session_status("HELLO REPLY RESULT=OK", ignored), net::sam::make_error_code(net::sam::error::parse_failed));
    EXPECT_EQ(net::sam::parse_session_status("SESSION STATUSX RESULT=OK DESTINATION=abc", ignored), net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, parse_stream_status)
{
    EXPECT_FALSE(net::sam::parse_stream_status("STREAM STATUS RESULT=OK"));

    EXPECT_EQ(net::sam::parse_stream_status("STREAM STATUS RESULT=CANT_REACH_PEER"), net::sam::make_error_code(net::sam::error::cant_reach_peer));

    EXPECT_EQ(net::sam::parse_stream_status("SESSION STATUS RESULT=OK"), net::sam::make_error_code(net::sam::error::parse_failed));
    EXPECT_EQ(net::sam::parse_stream_status("STREAM STATUSX RESULT=OK"), net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, parse_dest_reply)
{
    std::string priv;
    EXPECT_FALSE(net::sam::parse_dest_reply("DEST REPLY PUB=abc PRIV=def", priv));
    EXPECT_EQ(priv, "def");

    std::string priv_empty;
    EXPECT_EQ(net::sam::parse_dest_reply("DEST REPLY PUB=abc PRIV=", priv_empty), net::sam::make_error_code(net::sam::error::parse_failed));

    std::string priv_err;
    EXPECT_EQ(net::sam::parse_dest_reply("DEST REPLY RESULT=I2P_ERROR MESSAGE=foo", priv_err), net::sam::make_error_code(net::sam::error::i2p_error));

    std::string priv_wrong;
    EXPECT_EQ(net::sam::parse_dest_reply("HELLO REPLY RESULT=OK", priv_wrong), net::sam::make_error_code(net::sam::error::parse_failed));
    EXPECT_EQ(net::sam::parse_dest_reply("DEST REPLYX PUB=abc PRIV=def", priv_wrong), net::sam::make_error_code(net::sam::error::parse_failed));
}

TEST(i2p_sam, random_session_id)
{
    const std::string id = net::sam::random_session_id();
    EXPECT_TRUE(id.length() == 10);
    EXPECT_FALSE(id.empty());
}

TEST(i2p_sam, get_value_duplicate_key_returns_first_value)
{
    std::string out;
    auto ec = net::sam::get_value("RESULT=OK RESULT=I2P_ERROR", "RESULT", out);

    EXPECT_FALSE(ec);
    EXPECT_EQ(out, "OK");
}

TEST(i2p_sam, destination_to_address_rejects_empty_input)
{
    EXPECT_TRUE(net::sam::destination_to_address("").is_unknown());
}

TEST(i2p_sam, destination_to_address_is_deterministic_and_content_dependent)
{
    const std::string dest_a = make_destination_bytes(0, 0x11);
    const std::string dest_b = make_destination_bytes(0, 0x22);

    const auto addr_a1 = net::sam::destination_to_address(encode_i2p_base64(dest_a));
    const auto addr_a2 = net::sam::destination_to_address(encode_i2p_base64(dest_a));
    const auto addr_b = net::sam::destination_to_address(encode_i2p_base64(dest_b));

    EXPECT_FALSE(addr_a1.is_unknown());
    EXPECT_EQ(addr_a1, addr_a2);
    EXPECT_NE(addr_a1, addr_b);
}

TEST(i2p_sam, session_destination_to_address_ignores_appended_private_keys)
{
    const std::string dest = make_destination_bytes(0);
    const std::string private_keys(64, char(0x99));

    const auto from_bare_dest = net::sam::destination_to_address(encode_i2p_base64(dest));
    const auto from_private_destination =
        net::sam::session_destination_to_address(encode_i2p_base64(dest + private_keys));

    ASSERT_FALSE(from_bare_dest.is_unknown());
    EXPECT_EQ(from_bare_dest, from_private_destination);

    // Hashing the whole private destination (the prior bug) must give a different address.
    const auto from_whole_blob = net::sam::destination_to_address(encode_i2p_base64(dest + private_keys));
    EXPECT_NE(from_bare_dest, from_whole_blob);
}

TEST(i2p_sam, session_destination_to_address_respects_nonzero_certificate_length)
{
    const std::string dest = make_destination_bytes(4);
    const std::string private_keys(96, char(0x77));

    const auto from_bare_dest = net::sam::destination_to_address(encode_i2p_base64(dest));
    const auto from_private_destination =
        net::sam::session_destination_to_address(encode_i2p_base64(dest + private_keys));

    ASSERT_FALSE(from_bare_dest.is_unknown());
    EXPECT_EQ(from_bare_dest, from_private_destination);
}

TEST(i2p_sam, session_destination_to_address_rejects_too_short_input)
{
    EXPECT_TRUE(net::sam::session_destination_to_address(encode_i2p_base64(std::string(386, 'a'))).is_unknown());
    EXPECT_FALSE(net::sam::session_destination_to_address(encode_i2p_base64(make_destination_bytes(0))).is_unknown());
}

TEST(i2p_sam, session_destination_to_address_rejects_certificate_length_past_end)
{
    std::string dest = make_destination_bytes(0);
    dest[385] = char(0x27);
    dest[386] = char(0x10); // Certificate claims 10000 bytes that are not actually present.

    EXPECT_TRUE(net::sam::session_destination_to_address(encode_i2p_base64(dest)).is_unknown());
}

TEST(i2p_sam, connect_client_forwards_buffered_peer_bytes)
{
    boost::asio::io_context io;

    fake_connect_router router{io};

    bool handler_called = false;
    boost::system::error_code handler_ec;
    tcp::socket handed_off_socket{io};
    std::string handed_off_data;

    auto client = net::sam::make_connect_client(
      tcp::socket{io},
      [&](boost::system::error_code ec, tcp::socket socket, std::string leftover)
      {
        handler_called = true;
        handler_ec = ec;
        handed_off_socket = std::move(socket);
        handed_off_data = std::move(leftover);
        io.stop();
      });

    client->set_session_id("abcdefghij");

    const auto address = net::i2p_address::make(fake_b32_dest);
    ASSERT_TRUE(address);
    ASSERT_TRUE(client->set_connect_command(*address));

    router.start();

    ASSERT_TRUE(net::sam::client::connect_and_send(client, router.endpoint()));

    boost::asio::steady_timer deadline{io};
    deadline.expires_after(std::chrono::seconds{3});
    deadline.async_wait([&](boost::system::error_code ec) { if (!ec) io.stop(); });

    io.run();

    ASSERT_TRUE(handler_called);
    EXPECT_FALSE(handler_ec) << handler_ec.message();
    EXPECT_TRUE(handed_off_socket.is_open());
    EXPECT_EQ("hello-from-peer", handed_off_data);
}

TEST(i2p_sam, save_private_key_creates_file_with_owner_only_permissions)
{
    const boost::filesystem::path dir = boost::filesystem::temp_directory_path() /
      boost::filesystem::unique_path("i2p_sam_test-%%%%-%%%%-%%%%-%%%%");
    boost::filesystem::create_directories(dir);

    ASSERT_TRUE(net::sam::save_private_key(dir.string(), "FAKE_PRIVATE_KEY"));

    const boost::filesystem::path key_path = dir / "i2p_private_key";
    boost::system::error_code ec;

#ifndef _WIN32
    const auto perms = boost::filesystem::status(key_path, ec).permissions();
    ASSERT_FALSE(ec) << ec.message();

    EXPECT_EQ(boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write,
      perms & (boost::filesystem::perms::group_all | boost::filesystem::perms::others_all | boost::filesystem::perms::owner_all));
#endif

    std::string loaded;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(key_path.string(), loaded));
    EXPECT_EQ("FAKE_PRIVATE_KEY", loaded);

    boost::filesystem::remove_all(dir, ec);
}
