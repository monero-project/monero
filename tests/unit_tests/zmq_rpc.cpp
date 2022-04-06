// Copyright (c) 2020-2022, The Monero Project

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

#include <boost/preprocessor/stringize.hpp>
#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/events.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "json_serialization.h"
#include "net/zmq.h"
#include "rpc/message.h"
#include "rpc/zmq_pub.h"
#include "rpc/zmq_server.h"
#include "serialization/json_object.h"

#define MASSERT(...)                                                      \
  if (!(__VA_ARGS__))                                                     \
    return testing::AssertionFailure() << BOOST_PP_STRINGIZE(__VA_ARGS__)

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

namespace
{
  using published_json = std::pair<std::string, rapidjson::Document>;

  constexpr const char inproc_pub[] = "inproc://dummy_pub";

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

  std::vector<published_json> get_published(void* socket, int count = -1)
  {
    std::vector<published_json> out;

    const auto messages = get_messages(socket, count);
    out.reserve(messages.size());

    for (const std::string& message : messages)
    {
      const char* split = std::strchr(message.c_str(), ':');
      if (!split)
        throw std::runtime_error{"Invalid ZMQ/Pub message"};

      out.emplace_back();
      out.back().first = {message.c_str(), split};
      if (out.back().second.Parse(split + 1).HasParseError())
        throw std::runtime_error{"Failed to parse ZMQ/Pub message"};
    }

    return out;
  }

  testing::AssertionResult compare_full_txpool(epee::span<const cryptonote::txpool_event> events, const published_json& pub)
  {
    MASSERT(pub.first == "json-full-txpool_add");
    MASSERT(pub.second.IsArray());
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
      return make_transaction({temp_account.get_keys().m_account_address});
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
        throw std::runtime_error{"failed to initiaze zmq/pub"};

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
   {make_transaction(), {}, true}, {make_transaction(), {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
}

TEST_F(zmq_pub, JsonMinimalTxpool)
{
  static constexpr const char topic[] = "\1json-minimal-txpool_add";

  ASSERT_TRUE(sub_request(topic));

  std::vector<cryptonote::txpool_event> events
  {
   {make_transaction(), {}, true}, {make_transaction(), {}, true}
  };

  EXPECT_NO_THROW(pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_EQ(1u, pub->send_txpool_add(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

  events.at(0).res = false;
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
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

  auto pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, JsonMinimalChain)
{
  static constexpr const char topic[] = "\1json-minimal-chain_main";

  ASSERT_TRUE(sub_request(topic));

  const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

  EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  auto pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
  EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

  pubs = get_published(dummy_client.get());
  EXPECT_EQ(1u, pubs.size());
  ASSERT_LE(1u, pubs.size());
  EXPECT_TRUE(compare_minimal_block(533, epee::to_span(blocks), pubs.front()));
}

TEST_F(zmq_pub, JsonFullAll)
{
  static constexpr const char topic[] = "\1json-full";

  ASSERT_TRUE(sub_request(topic));
  {
    std::vector<cryptonote::txpool_event> events
    {
     {make_transaction(), {}, true}, {make_transaction(), {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
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
    std::vector<cryptonote::txpool_event> events
    {
     {make_transaction(), {}, true}, {make_transaction(), {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  }
  {
    const std::array<cryptonote::block, 2> blocks{{make_block(), make_block()}};

    EXPECT_EQ(1u, pub->send_chain_main(100, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(1u, pubs.size());
    ASSERT_LE(1u, pubs.size());
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.front()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
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
    std::vector<cryptonote::txpool_event> events
    {
     {make_transaction(), {}, true}, {make_transaction(), {}, true}
    };

    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_EQ(1u, pub->send_txpool_add(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_txpool(epee::to_span(events), pubs.front()));
    EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.back()));

    events.at(0).res = false;
    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(events));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
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

    auto pubs = get_published(dummy_client.get());
    EXPECT_EQ(2u, pubs.size());
    ASSERT_LE(2u, pubs.size());
    EXPECT_TRUE(compare_full_block(epee::to_span(blocks), pubs.front()));
    EXPECT_TRUE(compare_minimal_block(100, epee::to_span(blocks), pubs.back()));

    EXPECT_NO_THROW(cryptonote::listener::zmq_pub::chain_main{pub}(533, epee::to_span(blocks)));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));
    EXPECT_TRUE(pub->relay_to_pub(relay.get(), dummy_pub.get()));

    pubs = get_published(dummy_client.get());
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
    {make_transaction(), {}, true}, {make_transaction(), {}, true}
  };

  pub.reset();
  EXPECT_NO_THROW(cryptonote::listener::zmq_pub::txpool_add{pub}(std::move(events)));
}

TEST_F(zmq_server, pub)
{
  subscribe("json-minimal");

  std::vector<cryptonote::txpool_event> events
  {
   {make_transaction(), {}, true}, {make_transaction(), {}, true}
  };

  const std::array<cryptonote::block, 1> blocks{{make_block()}};

  ASSERT_EQ(1u, pub->send_txpool_add(events));
  ASSERT_EQ(1u, pub->send_chain_main(200, epee::to_span(blocks)));

  auto pubs = get_published(sub.get(), 2);
  EXPECT_EQ(2u, pubs.size());
  ASSERT_LE(2u, pubs.size());
  EXPECT_TRUE(compare_minimal_txpool(epee::to_span(events), pubs.front()));
  EXPECT_TRUE(compare_minimal_block(200, epee::to_span(blocks), pubs.back()));
}
