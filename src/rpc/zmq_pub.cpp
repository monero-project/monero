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

#include "zmq_pub.h"

#include <algorithm>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/thread/locks.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <utility>

#include "common/expect.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/events.h"
#include "misc_log_ex.h"
#include "serialization/json_object.h"
#include "ringct/rctTypes.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.zmq"

namespace
{
  constexpr const char txpool_signal[] = "tx_signal";

  using chain_writer =  void(epee::byte_stream&, std::uint64_t, epee::span<const cryptonote::block>);
  using miner_writer =  void(epee::byte_stream&, uint8_t, uint64_t, const crypto::hash&, const crypto::hash&, cryptonote::difficulty_type, uint64_t, uint64_t, const std::vector<cryptonote::tx_block_template_backlog_entry>&);
  using txpool_writer = void(epee::byte_stream&, epee::span<const cryptonote::txpool_event>);

  template<typename F>
  struct context
  {
    char const* const name;
    F* generate_pub;
  };

  template<typename T>
  bool operator<(const context<T>& lhs, const context<T>& rhs) noexcept
  {
    return std::strcmp(lhs.name, rhs.name) < 0;
  }

  template<typename T>
  bool operator<(const context<T>& lhs, const boost::string_ref rhs) noexcept
  {
    return lhs.name < rhs;
  }

  struct is_valid
  {
    bool operator()(const cryptonote::txpool_event& event) const noexcept
    {
      return event.res;
    }
  };

  template<typename T, std::size_t N>
  void verify_sorted(const std::array<context<T>, N>& elems, const char* name)
  {
    auto unsorted = std::is_sorted_until(elems.begin(), elems.end());
    if (unsorted != elems.end())
      throw std::logic_error{name + std::string{" array is not properly sorted, see: "} + unsorted->name};
  }

  void write_header(epee::byte_stream& buf, const boost::string_ref name)
  {
    buf.write(name.data(), name.size());
    buf.put(':');
  }

  //! \return `name:...` where `...` is JSON and `name` is directly copied (no quotes - not JSON).
  template<typename T>
  void json_pub(epee::byte_stream& buf, const T value)
  {
    rapidjson::Writer<epee::byte_stream> dest{buf};
    using cryptonote::json::toJsonValue;
    toJsonValue(dest, value);
  }

  //! Object for "minimal" block serialization
  struct minimal_chain
  {
    const std::uint64_t height;
    const epee::span<const cryptonote::block> blocks;
  };

  //! Object for miner data serialization
  struct miner_data
  {
    uint8_t major_version;
    uint64_t height;
    const crypto::hash& prev_id;
    const crypto::hash& seed_hash;
    cryptonote::difficulty_type diff;
    uint64_t median_weight;
    uint64_t already_generated_coins;
    const std::vector<cryptonote::tx_block_template_backlog_entry>& tx_backlog;
  };

  //! Object for "minimal" tx serialization
  struct minimal_txpool
  {
    const cryptonote::transaction& tx;
    crypto::hash hash;
    uint64_t blob_size;
    uint64_t weight;
    uint64_t fee;
  };

  void toJsonValue(rapidjson::Writer<epee::byte_stream>& dest, const minimal_chain& self)
  {
    namespace adapt = boost::adaptors;

    const auto to_block_id = [](const cryptonote::block& bl)
    {
      crypto::hash id;
      if (!get_block_hash(bl, id))
        MERROR("ZMQ/Pub failure: get_block_hash");
      return id;
    };

    assert(!self.blocks.empty()); // checked in zmq_pub::send_chain_main

    dest.StartObject();
    INSERT_INTO_JSON_OBJECT(dest, first_height, self.height);
    INSERT_INTO_JSON_OBJECT(dest, first_prev_id, self.blocks[0].prev_id);
    INSERT_INTO_JSON_OBJECT(dest, ids, (self.blocks | adapt::transformed(to_block_id)));
    dest.EndObject();
  }

  void toJsonValue(rapidjson::Writer<epee::byte_stream>& dest, const miner_data& self)
  {
    dest.StartObject();
    INSERT_INTO_JSON_OBJECT(dest, major_version, self.major_version);
    INSERT_INTO_JSON_OBJECT(dest, height, self.height);
    INSERT_INTO_JSON_OBJECT(dest, prev_id, self.prev_id);
    INSERT_INTO_JSON_OBJECT(dest, seed_hash, self.seed_hash);
    INSERT_INTO_JSON_OBJECT(dest, difficulty, cryptonote::hex(self.diff));
    INSERT_INTO_JSON_OBJECT(dest, median_weight, self.median_weight);
    INSERT_INTO_JSON_OBJECT(dest, already_generated_coins, self.already_generated_coins);
    INSERT_INTO_JSON_OBJECT(dest, tx_backlog, self.tx_backlog);
    dest.EndObject();
  }

  void toJsonValue(rapidjson::Writer<epee::byte_stream>& dest, const minimal_txpool& self)
  {
    dest.StartObject();
    INSERT_INTO_JSON_OBJECT(dest, id, self.hash);
    INSERT_INTO_JSON_OBJECT(dest, blob_size, self.blob_size);
    INSERT_INTO_JSON_OBJECT(dest, weight, self.weight);
    INSERT_INTO_JSON_OBJECT(dest, fee, self.fee);
    dest.EndObject();
  }

  void json_full_chain(epee::byte_stream& buf, const std::uint64_t height, const epee::span<const cryptonote::block> blocks)
  {
    json_pub(buf, blocks);
  }

  void json_minimal_chain(epee::byte_stream& buf, const std::uint64_t height, const epee::span<const cryptonote::block> blocks)
  {
    json_pub(buf, minimal_chain{height, blocks});
  }

  void json_miner_data(epee::byte_stream& buf, uint8_t major_version, uint64_t height, const crypto::hash& prev_id, const crypto::hash& seed_hash, cryptonote::difficulty_type diff, uint64_t median_weight, uint64_t already_generated_coins, const std::vector<cryptonote::tx_block_template_backlog_entry>& tx_backlog)
  {
    json_pub(buf, miner_data{major_version, height, prev_id, seed_hash, diff, median_weight, already_generated_coins, tx_backlog});
  }

  // boost::adaptors are in place "views" - no copy/move takes place
  // moving transactions (via sort, etc.), is expensive!

  void json_full_txpool(epee::byte_stream& buf, epee::span<const cryptonote::txpool_event> txes)
  {
    namespace adapt = boost::adaptors;
    const auto to_full_tx = [](const cryptonote::txpool_event& event)
    {
      return event.tx;
    };
    json_pub(buf, (txes | adapt::filtered(is_valid{}) | adapt::transformed(to_full_tx)));
  }

  void json_minimal_txpool(epee::byte_stream& buf, epee::span<const cryptonote::txpool_event> txes)
  {
    namespace adapt = boost::adaptors;
    const auto to_minimal_tx = [](const cryptonote::txpool_event& event)
    {
      return minimal_txpool{event.tx, event.hash, event.blob_size, event.weight, cryptonote::get_tx_fee(event.tx)};
    };
    json_pub(buf, (txes | adapt::filtered(is_valid{}) | adapt::transformed(to_minimal_tx)));
  }

  constexpr const std::array<context<chain_writer>, 2> chain_contexts =
  {{
    {u8"json-full-chain_main", json_full_chain},
    {u8"json-minimal-chain_main", json_minimal_chain}
  }};

  constexpr const std::array<context<miner_writer>, 1> miner_contexts =
  {{
    {u8"json-full-miner_data", json_miner_data},
  }};

  constexpr const std::array<context<txpool_writer>, 2> txpool_contexts =
  {{
    {u8"json-full-txpool_add", json_full_txpool},
    {u8"json-minimal-txpool_add", json_minimal_txpool}
  }};

  template<typename T, std::size_t N>
  epee::span<const context<T>> get_range(const std::array<context<T>, N>& contexts, const boost::string_ref value)
  {
    const auto not_prefix = [](const boost::string_ref lhs, const context<T>& rhs)
    {
      return !(boost::string_ref{rhs.name}.starts_with(lhs));
    };

    const auto lower = std::lower_bound(contexts.begin(), contexts.end(), value);
    const auto upper = std::upper_bound(lower, contexts.end(), value, not_prefix);
    return {lower, std::size_t(upper - lower)};
  }

  template<std::size_t N, typename T>
  void add_subscriptions(std::array<std::size_t, N>& subs, const epee::span<const context<T>> range, context<T> const* const first)
  {
    assert(range.size() <= N);
    assert((unsigned long)(range.begin() - first) <= N - range.size());

    for (const auto& ctx : range)
    {
      const std::size_t i = std::addressof(ctx) - first;
      subs[i] = std::min(std::numeric_limits<std::size_t>::max() - 1, subs[i]) + 1;
    }
  }

  template<std::size_t N, typename T>
  void remove_subscriptions(std::array<std::size_t, N>& subs, const epee::span<const context<T>> range, context<T> const* const first)
  {
    assert(range.size() <= N);
    assert((unsigned long)(range.begin() - first) <= N - range.size());

    for (const auto& ctx : range)
    {
      const std::size_t i = std::addressof(ctx) - first;
      subs[i] = std::max(std::size_t(1), subs[i]) - 1;
    }
  }

  template<std::size_t N, typename T, typename... U>
  std::array<epee::byte_slice, N> make_pubs(const std::array<std::size_t, N>& subs, const std::array<context<T>, N>& contexts, U&&... args)
  {
    epee::byte_stream buf{};

    std::size_t last_offset = 0;
    std::array<std::size_t, N> offsets{{}};
    for (std::size_t i = 0; i < N; ++i)
    {
      if (subs[i])
      {
        write_header(buf, contexts[i].name);
        contexts[i].generate_pub(buf, std::forward<U>(args)...);
        offsets[i] = buf.size() - last_offset;
        last_offset = buf.size();
      }
    }

    epee::byte_slice bytes{std::move(buf)};
    std::array<epee::byte_slice, N> out;
    for (std::size_t i = 0; i < N; ++i)
      out[i] = bytes.take_slice(offsets[i]);

    return out;
  }

  template<std::size_t N>
  std::size_t send_messages(void* const socket, std::array<epee::byte_slice, N>& messages)
  {
    std::size_t count = 0;
    for (epee::byte_slice& message : messages)
    {
      if (!message.empty())
      {
        const expect<void> sent = net::zmq::send(std::move(message), socket, ZMQ_DONTWAIT);
        if (!sent)
          MERROR("Failed to send ZMQ/Pub message: " << sent.error().message());
        else
          ++count;
      }
    }
    return count;
  }

  expect<bool> relay_block_pub(void* const relay, void* const pub) noexcept
  {
    zmq_msg_t msg;
    zmq_msg_init(std::addressof(msg));
    MONERO_CHECK(net::zmq::retry_op(zmq_msg_recv, std::addressof(msg), relay, ZMQ_DONTWAIT));

    const boost::string_ref payload{
      reinterpret_cast<const char*>(zmq_msg_data(std::addressof(msg))),
      zmq_msg_size(std::addressof(msg))
    };

    if (payload == txpool_signal)
    {
      zmq_msg_close(std::addressof(msg));
      return false;
    }

    // forward block messages (serialized on P2P thread for now)
    const expect<void> sent = net::zmq::retry_op(zmq_msg_send, std::addressof(msg), pub, ZMQ_DONTWAIT);
    if (!sent)
    {
      zmq_msg_close(std::addressof(msg));
      return sent.error();
    }
    return true;
  }
} // anonymous

namespace cryptonote { namespace listener
{

zmq_pub::zmq_pub(void* context)
  : relay_(),
    chain_subs_{{0}},
    miner_subs_{{0}},
    txpool_subs_{{0}},
    sync_()
{
  if (!context)
    throw std::logic_error{"ZMQ context cannot be NULL"};

  verify_sorted(chain_contexts, "chain_contexts");
  verify_sorted(miner_contexts, "miner_contexts");
  verify_sorted(txpool_contexts, "txpool_contexts");

  relay_.reset(zmq_socket(context, ZMQ_PAIR));
  if (!relay_)
    MONERO_ZMQ_THROW("Failed to create relay socket");
  if (zmq_connect(relay_.get(), relay_endpoint()) != 0)
    MONERO_ZMQ_THROW("Failed to connect relay socket");
}

zmq_pub::~zmq_pub()
{}

bool zmq_pub::sub_request(boost::string_ref message)
{
  if (!message.empty())
  {
    const char tag = message[0];
    message.remove_prefix(1);

    const auto chain_range = get_range(chain_contexts, message);
    const auto miner_range = get_range(miner_contexts, message);
    const auto txpool_range = get_range(txpool_contexts, message);

    if (!chain_range.empty() || !miner_range.empty() || !txpool_range.empty())
    {
      MDEBUG("Client " << (tag ? "subscribed" : "unsubscribed") << " to " <<
             chain_range.size() << " chain topic(s), " << miner_range.size() << " miner topic(s) and " << txpool_range.size() << " txpool topic(s)");

      const boost::lock_guard<boost::mutex> lock{sync_};
      switch (tag)
      {
      case 0:
        remove_subscriptions(chain_subs_, chain_range, chain_contexts.begin());
        remove_subscriptions(miner_subs_, miner_range, miner_contexts.begin());
        remove_subscriptions(txpool_subs_, txpool_range, txpool_contexts.begin());
        return true;
      case 1:
        add_subscriptions(chain_subs_, chain_range, chain_contexts.begin());
        add_subscriptions(miner_subs_, miner_range, miner_contexts.begin());
        add_subscriptions(txpool_subs_, txpool_range, txpool_contexts.begin());
        return true;
      default:
        break;
      }
    }
  }
  MERROR("Invalid ZMQ/Sub message");
  return false;
}

bool zmq_pub::relay_to_pub(void* const relay, void* const pub)
{
  const expect<bool> relayed = relay_block_pub(relay, pub);
  if (!relayed)
  {
    MERROR("Error relaying ZMQ/Pub: " << relayed.error().message());
    return false;
  }

  if (!*relayed)
  {
    std::array<std::size_t, 2> subs;
    std::vector<cryptonote::txpool_event> events;
    {
      const boost::lock_guard<boost::mutex> lock{sync_};
      if (txes_.empty())
        return false;

      subs = txpool_subs_;
      events = std::move(txes_.front());
      txes_.pop_front();
    }
    auto messages = make_pubs(subs, txpool_contexts, epee::to_span(events));
    send_messages(pub, messages);
    MDEBUG("Sent txpool ZMQ/Pub");
  }
  else
    MDEBUG("Sent chain_main ZMQ/Pub");

  return true;
}

std::size_t zmq_pub::send_chain_main(const std::uint64_t height, const epee::span<const cryptonote::block> blocks)
{
  if (blocks.empty())
    return 0;

  /* Block format only sends one block at a time - multiple block notifications
     are less common and only occur on rollbacks. */

  boost::unique_lock<boost::mutex> guard{sync_};

  const auto subs_copy = chain_subs_;
  guard.unlock();

  for (const std::size_t sub : subs_copy)
  {
    if (sub)
    {
      /* cryptonote_core/blockchain.cpp cannot "give" us the block like core
         does for txpool events. Since copying the block is expensive anyway,
         serialization is done right here on the p2p thread (for now). */

        auto messages = make_pubs(subs_copy, chain_contexts, height, blocks);
        guard.lock();
        return send_messages(relay_.get(), messages);
    }
  }
  return 0;
}

std::size_t zmq_pub::send_miner_data(uint8_t major_version, uint64_t height, const crypto::hash& prev_id, const crypto::hash& seed_hash, difficulty_type diff, uint64_t median_weight, uint64_t already_generated_coins, const std::vector<tx_block_template_backlog_entry>& tx_backlog)
{
  boost::unique_lock<boost::mutex> guard{sync_};

  const auto subs_copy = miner_subs_;
  guard.unlock();

  for (const std::size_t sub : subs_copy)
  {
    if (sub)
    {
        auto messages = make_pubs(subs_copy, miner_contexts, major_version, height, prev_id, seed_hash, diff, median_weight, already_generated_coins, tx_backlog);
        guard.lock();
        return send_messages(relay_.get(), messages);
    }
  }
  return 0;
}

std::size_t zmq_pub::send_txpool_add(std::vector<txpool_event> txes)
{
  if (txes.empty())
    return 0;

  const boost::lock_guard<boost::mutex> lock{sync_};
  for (const std::size_t sub : txpool_subs_)
  {
    if (sub)
    {
      const expect<void> sent = net::zmq::retry_op(zmq_send_const, relay_.get(), txpool_signal, sizeof(txpool_signal) - 1, ZMQ_DONTWAIT);
      if (sent)
        txes_.emplace_back(std::move(txes));
      else
        MERROR("ZMQ/Pub failure, relay queue error: " << sent.error().message());
      return bool(sent);
    }
  }
  return 0;
}

void zmq_pub::chain_main::operator()(const std::uint64_t height, epee::span<const cryptonote::block> blocks) const
{
  const std::shared_ptr<zmq_pub> self = self_.lock();
  if (self)
    self->send_chain_main(height, blocks);
  else
    MERROR("Unable to send ZMQ/Pub - ZMQ server destroyed");
}

void zmq_pub::miner_data::operator()(uint8_t major_version, uint64_t height, const crypto::hash& prev_id, const crypto::hash& seed_hash, difficulty_type diff, uint64_t median_weight, uint64_t already_generated_coins, const std::vector<tx_block_template_backlog_entry>& tx_backlog) const
{
  const std::shared_ptr<zmq_pub> self = self_.lock();
  if (self)
    self->send_miner_data(major_version, height, prev_id, seed_hash, diff, median_weight, already_generated_coins, tx_backlog);
  else
    MERROR("Unable to send ZMQ/Pub - ZMQ server destroyed");
}

void zmq_pub::txpool_add::operator()(std::vector<cryptonote::txpool_event> txes) const
{
  const std::shared_ptr<zmq_pub> self = self_.lock();
  if (self)
      self->send_txpool_add(std::move(txes));
  else
    MERROR("Unable to send ZMQ/Pub - ZMQ server destroyed");
}

}}
