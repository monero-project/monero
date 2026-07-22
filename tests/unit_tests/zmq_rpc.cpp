// Copyright (c) 2020-2024, The Monero Project

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

#include <boost/endian/conversion.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <vector>

#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/events.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "json_serialization.h"
#include "net/zmq.h"
#include "rpc/message.h"
#include "rpc/zmq_pub.h"
#include "rpc/zmq_restricted_methods.h"
#include "rpc/zmq_server.h"
#include "serialization/json_object.h"
#include "serialization/wire/json.h"
#include "serialization/wire/cbor.h"

#define MASSERT(...)                                                      \
  if (!(__VA_ARGS__))                                                     \
    return testing::AssertionFailure() << BOOST_PP_STRINGIZE(__VA_ARGS__)

namespace rapidjson
{
  std::ostream& operator<<(std::ostream& out, const Document& src)
  {
    OStreamWrapper buffer{out};
    PrettyWriter<OStreamWrapper> writer{buffer};
    src.Accept(writer);
    return out;
  }
}

TEST(ZmqFullMessage, InvalidRequest)
{
  EXPECT_THROW(
    (cryptonote::rpc::FullMessage{"{\"jsonrpc\":\"2.0\",\"id\":0,\"params\":[]}", true}),
    cryptonote::json::MISSING_KEY
  );
  EXPECT_THROW(
    (cryptonote::rpc::FullMessage{"{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":3,\"params\":[]}", true}),
    cryptonote::json::WRONG_TYPE
  );
}

TEST(ZmqFullMessage, Request)
{
  static constexpr const char request[] = "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"foo\",\"params\":[]}";
  EXPECT_NO_THROW(
    (cryptonote::rpc::FullMessage{request, true})
  );

  cryptonote::rpc::FullMessage parsed{request, true};
  EXPECT_STREQ("foo", parsed.getRequestType().c_str());
}

TEST(ZmqRestrictedMethods, BasicCoverage)
{
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("flush_txpool"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("get_peer_list"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("mining_status"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("relay_tx"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("save_bc"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("set_log_categories"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("set_log_level"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("start_mining"));
  EXPECT_TRUE(cryptonote::rpc::is_blocked_in_restricted_mode("stop_mining"));

  EXPECT_FALSE(cryptonote::rpc::is_blocked_in_restricted_mode("get_height"));
  EXPECT_FALSE(cryptonote::rpc::is_blocked_in_restricted_mode("get_info"));
  EXPECT_FALSE(cryptonote::rpc::is_blocked_in_restricted_mode("send_raw_tx"));
}

namespace
{
  using published_bytes = std::pair<std::string, std::string>;
  using published_json = std::pair<std::string, rapidjson::Document>;

  constexpr const char inproc_pub[] = "inproc://dummy_pub";

  std::string cbor_to_json(epee::span<const std::uint8_t> bytes)
  {
    const auto multibyte = [&bytes] (const auto minor)
    {
      using minor_type = decltype(minor);
      using payload_type = typename minor_type::type;
      if (bytes.size() < sizeof(payload_type))
        throw std::logic_error{"Invalid cbor - expected more bytes for size"};

      payload_type buf;
      std::memcpy(&buf, bytes.data(), sizeof(buf));
      bytes.remove_prefix(sizeof(buf));
      return boost::endian::big_to_native(buf);
    };

    std::vector<std::pair<std::uint64_t, wire::cbor::major>> stack{};
    wire::json_string_writer dest{};
    while (!bytes.empty())
    {
      const std::uint8_t tag = bytes[0];
      bytes.remove_prefix(1);
      const auto major = wire::cbor::major(tag & 0xE0);
      const std::uint8_t minor = tag & 0x1F;

      std::uint64_t size = minor;
      switch (minor)
      {
        default:
          break;
        case wire::cbor::uint8_t::value():
          size = multibyte(wire::cbor::uint8_t{});
          break;
        case wire::cbor::uint16_t::value():
          size = multibyte(wire::cbor::uint16_t{});
          break;
        case wire::cbor::uint32_t::value():
          size = multibyte(wire::cbor::uint32_t{});
          break;
        case wire::cbor::uint64_t::value():
          size = multibyte(wire::cbor::uint64_t{});
          break;
      }

      switch (major)
      {
        default:
          throw std::logic_error{"cbor conversion not supported for this major type"};
        case wire::cbor::major::positive_int:
          dest.unsigned_integer(size);
          break;
        case wire::cbor::major::negative_int:
          if (std::numeric_limits<std::intmax_t>::max() < size)
            throw std::logic_error{"cbor exceeded implementation signed limits"};
          dest.integer((-std::intmax_t(size)) - 1);
          break;
        case wire::cbor::major::bytes:
          if (bytes.size() < size)
            throw std::logic_error{"cbor missing bytes"};
          dest.binary({bytes.data(), size});
          bytes.remove_prefix(size);
          break;
        case wire::cbor::major::string:
          if (bytes.size() < size)
            throw std::logic_error{"cbor missing string"};
          if (!stack.empty() && stack.back().second == wire::cbor::major::map && stack.back().first % 2 == 0)
            dest.key({reinterpret_cast<const char*>(bytes.data()), size});
          else
            dest.string({reinterpret_cast<const char*>(bytes.data()), size});
          bytes.remove_prefix(size);
          break;
        case wire::cbor::major::array:
          dest.start_array(size);
          stack.push_back({size + 1, wire::cbor::major::array});
          break;
        case wire::cbor::major::map:
          dest.start_object(size);
          stack.push_back({size * 2 + 1, wire::cbor::major::map});
          break;
        case wire::cbor::major::real_and_special:
          switch (minor)
          {
            default:
              throw std::logic_error{"cbor conversion not supported for this major/minor type"};
            case wire::cbor::real64::value():
              double real;
              static_assert(sizeof(real) == sizeof(size));
              std::memcpy(&real, &size, sizeof(real));
              dest.real(real);
              break;
            case wire::cbor::true_::tag():
              dest.boolean(true);
              break;
            case wire::cbor::false_::tag():
              dest.boolean(false);
              break;
          }
      };

      while (!stack.empty() && !--stack.back().first)
      {
        switch (stack.back().second)
        {
          default:
            break;
          case wire::cbor::major::array:
            dest.end_array();
            break;
          case wire::cbor::major::map:
            dest.end_object();
            break;
        }
        stack.pop_back();
      }
    }

    if (!stack.empty())
      throw std::logic_error{"cbor expected more elements"};

    return dest.take_buffer();
  }

  net::zmq::socket create_socket(void* ctx, const char* address)
  {
    net::zmq::socket sock{zmq_socket(ctx, ZMQ_PAIR)};
    if (!sock)
      MONERO_ZMQ_THROW("failed to create socket");
    if (zmq_bind(sock.get(), address) != 0)
      MONERO_ZMQ_THROW("socket bind failure");
    return sock;
  }

  std::vector<std::string> get_messages(void* socket, int count = -1)
  {
    std::vector<std::string> out;
    for ( ; count || count < 0; --count)
    {
      expect<std::string> next = net::zmq::receive(socket, (count < 0 ? ZMQ_DONTWAIT : 0));
      if (next == net::zmq::make_error_code(EAGAIN))
        return out;
      out.push_back(std::move(*next));
    }
    return out;
  }

  std::vector<published_bytes> get_published(void* socket, int count = -1)
  {
    std::vector<published_bytes> out;

    const auto messages = get_messages(socket, count);
    out.reserve(messages.size());

    for (const std::string& message : messages)
    {
      const char* split = std::strchr(message.c_str(), ':');
      if (!split)
        throw std::runtime_error{"Invalid ZMQ/Pub message"};

      out.emplace_back(
        std::string{message.data(), split},
        std::string{split + 1, message.c_str() + message.size()}
      );
    }

    return out;
  }

  std::vector<published_json> to_published_json(const std::vector<published_bytes>& messages)
  {
    std::vector<published_json> out;
    out.reserve(messages.size());

    for (const published_bytes& message : messages)
    {
      out.emplace_back(message.first, rapidjson::Document{});
      if (out.back().second.Parse(message.second.c_str()).HasParseError())
        throw std::runtime_error{"Failed to parse ZMQ/Pub message"};
    }

    return out;
  }

  std::vector<published_bytes> published_cbor_to_json(const std::vector<published_bytes>& messages)
  {
    std::vector<published_bytes> out;
    out.reserve(messages.size());

    for (const published_bytes& message : messages)
    {
      if (message.first.substr(0, 5) != "cbor-")
        throw std::logic_error{"expected cbor prefix"};
      out.emplace_back("json-" + message.first.substr(5), cbor_to_json(epee::strspan<std::uint8_t>(message.second)));
    }

    return out;
  }

  testing::AssertionResult compare_json(const std::string expected_json, const rapidjson::Document& published)
  {
    rapidjson::Document expected;
    expected.Parse(expected_json.c_str());
    MASSERT(!expected.HasParseError());
    if (expected != published)
      return testing::AssertionFailure() << expected << " != " << published;
    return testing::AssertionSuccess();
  }

  testing::AssertionResult compare_full_txpool(epee::span<const cryptonote::txpool_event> events, const published_json& pub)
  {
    MASSERT(pub.first == "json-full-txpool_add");
    MASSERT(pub.second.IsArray());
    MASSERT(1 <= pub.second.Size());
    MASSERT(pub.second.Size() <= events.size());

    std::size_t i = 0;
    for (const cryptonote::txpool_event& event : events)
    {
      MASSERT(i <= pub.second.Size());
      if (!event.res)
        continue;

      cryptonote::transaction tx{};
      cryptonote::json::fromJsonValue(pub.second[i], tx);

      crypto::hash id{};
      MASSERT(cryptonote::get_transaction_hash(event.tx, id));
      MASSERT(cryptonote::get_transaction_hash(tx, id));
      MASSERT(event.tx.hash == tx.hash);
      ++i;
    }
    return testing::AssertionSuccess();
  }

  testing::AssertionResult compare_minimal_txpool(epee::span<const cryptonote::txpool_event> events, const published_json& pub)
  {
    MASSERT(pub.first == "json-minimal-txpool_add");
    MASSERT(pub.second.IsArray());
    MASSERT(1 <= pub.second.Size());
    MASSERT(pub.second.Size() <= events.size());

    std::size_t i = 0;
    for (const cryptonote::txpool_event& event : events)
    {
      MASSERT(i <= pub.second.Size());
      if (!event.res)
        continue;

      std::size_t actual_size = 0;
      crypto::hash actual_id{};

      MASSERT(pub.second[i].IsObject());
      GET_FROM_JSON_OBJECT(pub.second[i], actual_id, id);
      GET_FROM_JSON_OBJECT(pub.second[i], actual_size, blob_size);

      std::size_t expected_size = 0;
      crypto::hash expected_id{};
      MASSERT(cryptonote::get_transaction_hash(event.tx, expected_id, expected_size));
      MASSERT(expected_size == actual_size);
      MASSERT(expected_id == actual_id);
      ++i;
    }
    return testing::AssertionSuccess();
  }

  testing::AssertionResult compare_miner_data(const std::string expected, const published_json& pub)
  {
    MASSERT(pub.first == "json-full-miner_data");
    return compare_json(expected, pub.second);
  }

  testing::AssertionResult compare_full_block(const epee::span<const cryptonote::block> expected, const published_json& pub)
  {
    MASSERT(pub.first == "json-full-chain_main");
    MASSERT(pub.second.IsArray());

    std::vector<cryptonote::block> actual;
    cryptonote::json::fromJsonValue(pub.second, actual);

    MASSERT(expected.size() == actual.size());

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
      crypto::hash id;
      MASSERT(cryptonote::get_block_hash(expected[i], id));
      MASSERT(cryptonote::get_block_hash(actual[i], id));
      MASSERT(expected[i].hash == actual[i].hash);
    }

    return testing::AssertionSuccess();
  }

  testing::AssertionResult compare_minimal_block(std::size_t height, const epee::span<const cryptonote::block> expected, const published_json& pub)
  {
    MASSERT(pub.first == "json-minimal-chain_main");
    MASSERT(pub.second.IsObject());
    MASSERT(!expected.empty());

    std::size_t actual_height = 0;
    crypto::hash actual_prev_id{};
    std::vector<crypto::hash> actual_ids{};
    GET_FROM_JSON_OBJECT(pub.second, actual_height, first_height);
    GET_FROM_JSON_OBJECT(pub.second, actual_prev_id, first_prev_id);
    GET_FROM_JSON_OBJECT(pub.second, actual_ids, ids);

    MASSERT(height == actual_height);
    MASSERT(expected[0].prev_id == actual_prev_id);
    MASSERT(expected.size() == actual_ids.size());

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
      crypto::hash id;
      MASSERT(cryptonote::get_block_hash(expected[i], id));
      MASSERT(id == actual_ids[i]);
    }

    return testing::AssertionSuccess();
  }

  struct zmq_base : public testing::Test
  {
    cryptonote::account_base acct;

    zmq_base()
      : testing::Test(), acct()
    {
      acct.generate();
    }

    cryptonote::transaction make_miner_transaction()
    {
      return test::make_miner_transaction(acct.get_keys().m_account_address);
    }

    cryptonote::transaction make_transaction(const std::vector<cryptonote::account_public_address>& destinations)
    {
      return test::make_transaction(acct.get_keys(), {make_miner_transaction()}, destinations, true, true);
    }

    cryptonote::transaction make_transaction()
    {
      cryptonote::account_base temp_account;
      temp_account.generate();

      cryptonote::account_base temp_account2;
      temp_account2.generate();
 
      return make_transaction({temp_account.get_keys().m_account_address, temp_account2.get_keys().m_account_address});
    }

    cryptonote::block make_block()
    {
      cryptonote::block block{};
      block.major_version = 1;
      block.minor_version = 3;
      block.timestamp = 100;
      block.prev_id = crypto::rand<crypto::hash>();
      block.nonce = 100;
      block.miner_tx = make_miner_transaction();
      return block;
    }
  };

  struct zmq_pub : public zmq_base
  {
    net::zmq::context ctx;
    net::zmq::socket relay;
    net::zmq::socket dummy_pub;
    net::zmq::socket dummy_client;
    std::shared_ptr<cryptonote::listener::zmq_pub> pub;

    zmq_pub()
      : zmq_base(),
        ctx(zmq_init(1)),
        relay(create_socket(ctx.get(), cryptonote::listener::zmq_pub::relay_endpoint())),
        dummy_pub(create_socket(ctx.get(), inproc_pub)),
        dummy_client(zmq_socket(ctx.get(), ZMQ_PAIR)),
        pub(std::make_shared<cryptonote::listener::zmq_pub>(ctx.get()))
    {
      if (!dummy_client)
        MONERO_ZMQ_THROW("failed to create socket");
      if (zmq_connect(dummy_client.get(), inproc_pub) != 0)
        MONERO_ZMQ_THROW("failed to connect to dummy pub");
    }

    virtual void TearDown() override final
    {
      EXPECT_EQ(0u, get_messages(relay.get()).size());
      EXPECT_EQ(0u, get_messages(dummy_client.get()).size());
    }

    template<std::size_t N>
    bool sub_request(const char (&topic)[N])
    {
      return pub->sub_request({topic, N - 1});
    }
  };

  struct dummy_handler final : cryptonote::rpc::RpcHandler
  {
    dummy_handler()
      : cryptonote::rpc::RpcHandler()
    {}

    virtual epee::byte_slice handle(std::string&& request) override final
    {
      throw std::logic_error{"not implemented"};
    }
  };

  struct zmq_server : public zmq_base
  {
    dummy_handler handler;
    cryptonote::rpc::ZmqServer server;
    std::shared_ptr<cryptonote::listener::zmq_pub> pub;
    net::zmq::socket sub;

    zmq_server()
      : zmq_base(),
        handler(),
        server(handler),
        pub(),
        sub()
    {
      void* ctx = server.init_rpc({}, {});
      if (!ctx)
        throw std::runtime_error{"init_rpc failure"};

      const std::string endpoint = inproc_pub;
      pub = server.init_pub({std::addressof(endpoint), 1});
      if (!pub)
        throw std::runtime_error{"failed to initialize zmq/pub"};

      sub.reset(zmq_socket(ctx, ZMQ_SUB));
      if (!sub)
        MONERO_ZMQ_THROW("failed to create socket");
      if (zmq_connect(sub.get(), inproc_pub) != 0)
        MONERO_ZMQ_THROW("failed to connect to dummy pub");

      server.run();
    }

    virtual void TearDown() override final
    {
      EXPECT_EQ(0u, get_messages(sub.get()).size());
      sub.reset();
      pub.reset();
      server.stop();
    }

    template<std::size_t N>
    void subscribe(const char (&topic)[N])
    {
      if (zmq_setsockopt(sub.get(), ZMQ_SUBSCRIBE, topic, N - 1) != 0)
        MONERO_ZMQ_THROW("failed to subscribe");
    }
  };
}

TEST_F(zmq_pub, InvalidContext)
{
  EXPECT_THROW(cryptonote::listener::zmq_pub{nullptr}, std::logic_error);
}

TEST_F(zmq_pub, NoBlocking)
{
  EXPECT_FALSE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
}

TEST_F(zmq_pub, DefaultDrop)
{
  EXPECT_EQ(0u, pub->send_txpool_add({{make_transaction(), {}, true}}));

  const cryptonote::block bl = make_block();
  EXPECT_EQ(0u,pub->send_chain_main(5, {std::addressof(bl), 1}));
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(5, {std::addressof(bl), 1}));
}

TEST_F(zmq_pub, JsonFullTxpool)
{
  static constexpr const char topic[] = "\1json-full-txpool_add";

  ASSERT_TRUE(sub_request(topic));

  std::vector<cryptonote::txpool_event> events
  {
    {make_transaction(), {}, {}, {}, true}, {make_transaction(), {}, {}, {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
}

TEST_F(zmq_pub, CborFullTxpool)
{
  static constexpr const char topic[] = "\1cbor-full-txpool_add";

  ASSERT_TRUE(sub_request(topic));

  std::vector<cryptonote::txpool_event> events
  {
    {make_transaction(), {}, {}, {}, true}, {make_transaction(), {}, {}, {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
}


TEST_F(zmq_pub, JsonMinimalTxpool)
{
  static constexpr const char topic[] = "\1json-minimal-txpool_add";

  ASSERT_TRUE(sub_request(topic));

  const auto tx1 = make_transaction();
  const auto tx2 = make_transaction();
  std::size_t size1 = 0;
  std::size_t size2 = 0;
  crypto::hash id1{};
  crypto::hash id2{};

  EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
  EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

  std::vector<cryptonote::txpool_event> events
  {
    {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
}

TEST_F(zmq_pub, CborMinimalTxpool)
{
  static constexpr const char topic[] = "\1cbor-minimal-txpool_add";

  ASSERT_TRUE(sub_request(topic));

  const auto tx1 = make_transaction();
  const auto tx2 = make_transaction();
  std::size_t size1 = 0;
  std::size_t size2 = 0;
  crypto::hash id1{};
  crypto::hash id2{};

  EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
  EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

  std::vector<cryptonote::txpool_event> events
  {
    {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
}

TEST_F(zmq_pub, JsonFullChain)
{
  static constexpr const char topic[] = "\1json-full-chain_main";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

  EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, CborFullChain)
{
  static constexpr const char topic[] = "\1cbor-full-chain_main";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

  EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, JsonFullMinerData)
{
  static constexpr const char topic[] = "\1json-full-miner_data";
  ASSERT_TRUE(sub_request(topic));

  const auto hash = crypto::rand<crypto::hash>();
  const auto seed = crypto::rand<crypto::hash>();
  const cryptonote::difficulty_type difficulty = 500;
  const std::vector<cryptonote::tx_block_template_backlog_entry> txs = {
    {crypto::rand<crypto::hash>(), 7545, 455},
    {crypto::rand<crypto::hash>(), 755, 34545}
  };
  const std::string expected =
    R"({
      "major_version": 100,
      "height": 200,
      "prev_id": ")" + epee::to_hex::string(epee::as_byte_span(hash)) + R"(",
      "seed_hash": ")" + epee::to_hex::string(epee::as_byte_span(seed)) + R"(",
      "difficulty": ")" + cryptonote::hex(difficulty) + R"(",
      "median_weight": 400,
      "already_generated_coins": 10000,
      "tx_backlog": [
        {
          "id": ")" + epee::to_hex::string(epee::as_byte_span(txs.at(0).id)) + R"(",
          "weight": 7545,
          "fee": 455
        },{
          "id": ")" + epee::to_hex::string(epee::as_byte_span(txs.at(1).id)) + R"(",
          "weight": 755,
          "fee": 34545
        }
      ]
    })";
  EXPECT_EQ(1u, pub->send_miner_data(100, 200, hash, seed, difficulty, 400, 10000, txs));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(get_published(dummy_client.get()));
  ASSERT_EQ(1u, pubs.size());
  EXPECT_TRUE(compare_miner_data(expected, pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::miner_data{pub}(100, 200, hash, seed, difficulty, 400, 10000, txs));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  ASSERT_EQ(1u, pubs.size());
  EXPECT_TRUE(compare_miner_data(expected, pubs.front()));
}

TEST_F(zmq_pub, CborFullMinerData)
{
  static constexpr const char topic[] = "\1cbor-full-miner_data";
  ASSERT_TRUE(sub_request(topic));

  const auto hash = crypto::rand<crypto::hash>();
  const auto seed = crypto::rand<crypto::hash>();
  const cryptonote::difficulty_type difficulty = 500;
  const std::vector<cryptonote::tx_block_template_backlog_entry> txs = {
    {crypto::rand<crypto::hash>(), 7545, 455},
    {crypto::rand<crypto::hash>(), 755, 34545}
  };
  const std::string expected =
    R"({
      "major_version": 100,
      "height": 200,
      "prev_id": ")" + epee::to_hex::string(epee::as_byte_span(hash)) + R"(",
      "seed_hash": ")" + epee::to_hex::string(epee::as_byte_span(seed)) + R"(",
      "difficulty": ")" + cryptonote::hex(difficulty) + R"(",
      "median_weight": 400,
      "already_generated_coins": 10000,
      "tx_backlog": [
        {
          "id": ")" + epee::to_hex::string(epee::as_byte_span(txs.at(0).id)) + R"(",
          "weight": 7545,
          "fee": 455
        },{
          "id": ")" + epee::to_hex::string(epee::as_byte_span(txs.at(1).id)) + R"(",
          "weight": 755,
          "fee": 34545
        }
      ]
    })";
  EXPECT_EQ(1u, pub->send_miner_data(100, 200, hash, seed, difficulty, 400, 10000, txs));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  ASSERT_EQ(1u, pubs.size());
  EXPECT_TRUE(compare_miner_data(expected, pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::miner_data{pub}(100, 200, hash, seed, difficulty, 400, 10000, txs));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  ASSERT_EQ(1u, pubs.size());
  EXPECT_TRUE(compare_miner_data(expected, pubs.front()));
}

TEST_F(zmq_pub, JsonMinimalChain)
{
  static constexpr const char topic[] = "\1json-minimal-chain_main";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

  EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(get_published(dummy_client.get()));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, CborMinimalChain)
{
  static constexpr const char topic[] = "\1cbor-minimal-chain_main";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

  EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, JsonFullAll)
{
  static constexpr const char topic[] = "\1json-full";

  ASSERT_TRUE(sub_request(topic));
  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()))
    ;
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
  }
}

TEST_F(zmq_pub, CborFullAll)
{
  static constexpr const char topic[] = "\1cbor-full";

  ASSERT_TRUE(sub_request(topic));
  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())))
    ;
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
  }
}


TEST_F(zmq_pub, JsonMinimalAll)
{
  static constexpr const char topic[] = "\1json-minimal";

  ASSERT_TRUE(sub_request(topic));

  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.front()));
  }
}

TEST_F(zmq_pub, CborMinimalAll)
{
  static constexpr const char topic[] = "\1cbor-minimal";

  ASSERT_TRUE(sub_request(topic));

  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.front()));
  }
}

TEST_F(zmq_pub, JsonAll)
{
  static constexpr const char topic[] = "\1json";

  ASSERT_TRUE(sub_request(topic));

  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));
  }
  {
    const std::array<cryptonote::block, 1> blocks{{make_block()}};

    EXPECT_EQ(2u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(get_published(dummy_client.get()));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
    EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.back()));
  }
}

TEST_F(zmq_pub, CborAll)
{
  static constexpr const char topic[] = "\1cbor";

  ASSERT_TRUE(sub_request(topic));

  {
    const auto tx1 = make_transaction();
    const auto tx2 = make_transaction();
    std::size_t size1 = 0;
    std::size_t size2 = 0;
    crypto::hash id1{};
    crypto::hash id2{};

    EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
    EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

    std::vector<cryptonote::txpool_event> events
    {
      {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));
  }
  {
    const std::array<cryptonote::block, 1> blocks{{make_block()}};

    EXPECT_EQ(2u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = to_published_json(published_cbor_to_json(get_published(dummy_client.get())));
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
    EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.back()));
  }
}

TEST_F(zmq_pub, JsonChainWeakPtrSkip)
{
  static constexpr const char topic[] = "\1json";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 1> blocks{{make_block()}};

  pub.reset();
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
}

TEST_F(zmq_pub, JsonTxpoolWeakPtrSkip)
{
  static constexpr const char topic[] = "\1json";

  ASSERT_TRUE(sub_request(topic));

  std::vector<cryptonote::txpool_event> events
  {
    {make_transaction(), {}, {}, {}, true}, {make_transaction(), {}, {}, {}, true}
  };

  pub.reset();
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(std::move(events)));
}

TEST_F(zmq_server, pub)
{
  subscribe("json-minimal");

  const auto tx1 = make_transaction();
  const auto tx2 = make_transaction();
  std::size_t size1 = 0;
  std::size_t size2 = 0;
  crypto::hash id1{};
  crypto::hash id2{};

  EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
  EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

  std::vector<cryptonote::txpool_event> events
  {
    {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
  };

  const std::array<cryptonote::block, 1> blocks{{make_block()}};

  ASSERT_EQ(1u, pub->send_txpool_add(events));
  ASSERT_EQ(1u, pub->send_chain_main(200, epee::to_span(blocks)));

  auto pubs = to_published_json(get_published(sub.get(), 2));
  EXPECT_EQ(2u, pubs.size());
  ASSERT_LE(2u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  EXPECT_TRUE(compare_minimal_block(200, epee::to_span(blocks), pubs.back()));
}

TEST_F(zmq_server, pub_cbor)
{
  subscribe("cbor-minimal");

  const auto tx1 = make_transaction();
  const auto tx2 = make_transaction();
  std::size_t size1 = 0;
  std::size_t size2 = 0;
  crypto::hash id1{};
  crypto::hash id2{};

  EXPECT_TRUE(cryptonote::get_transaction_hash(tx1, id1, size1));
  EXPECT_TRUE(cryptonote::get_transaction_hash(tx2, id2, size2));

  std::vector<cryptonote::txpool_event> events
  {
    {tx1, id1, size1, {}, true}, {tx2, id2, size2, {}, true}
  };

  const std::array<cryptonote::block, 1> blocks{{make_block()}};

  ASSERT_EQ(1u, pub->send_txpool_add(events));
  ASSERT_EQ(1u, pub->send_chain_main(200, epee::to_span(blocks)));

  auto pubs = to_published_json(published_cbor_to_json(get_published(sub.get(), 2)));
  EXPECT_EQ(2u, pubs.size());
  ASSERT_LE(2u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  EXPECT_TRUE(compare_minimal_block(200, epee::to_span(blocks), pubs.back()));
}
