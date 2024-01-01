// Copyright (c) 2016-2024, The Monero Project
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

#include "zmq_server.h"

#include <chrono>
#include <cstring>
#include <utility>
#include <stdexcept>
#include <system_error>

#include "byte_slice.h"
#include "rpc/zmq_pub.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.zmq"

namespace cryptonote
{

namespace
{
  constexpr const int num_zmq_threads = 1;
  constexpr const std::int64_t max_message_size = 10 * 1024 * 1024; // 10 MiB
  constexpr const std::chrono::seconds linger_timeout{2}; // wait period for pending out messages

  net::zmq::socket init_socket(void* context, int type, epee::span<const std::string> addresses)
  {
    if (context == nullptr)
      throw std::logic_error{"NULL context provided"};

    net::zmq::socket out{};
    out.reset(zmq_socket(context, type));
    if (!out)
    {
      MONERO_LOG_ZMQ_ERROR("Failed to create ZMQ socket");
      return nullptr;
    }

    if (zmq_setsockopt(out.get(), ZMQ_MAXMSGSIZE, std::addressof(max_message_size), sizeof(max_message_size)) != 0)
    {
      MONERO_LOG_ZMQ_ERROR("Failed to set maximum incoming message size");
      return nullptr;
    }

    static constexpr const int linger_value = std::chrono::milliseconds{linger_timeout}.count();
    if (zmq_setsockopt(out.get(), ZMQ_LINGER, std::addressof(linger_value), sizeof(linger_value)) != 0)
    {
      MONERO_LOG_ZMQ_ERROR("Failed to set linger timeout");
      return nullptr;
    }

    for (const std::string& address : addresses)
    {
      if (zmq_bind(out.get(), address.c_str()) < 0)
      {
        MONERO_LOG_ZMQ_ERROR("ZMQ bind failed");
        return nullptr;
      }
      MINFO("ZMQ now listening at " << address);
    }

    return out;
  }
} // anonymous

namespace rpc
{

ZmqServer::ZmqServer(RpcHandler& h) :
    handler(h),
    context(zmq_init(num_zmq_threads)),
    rep_socket(nullptr),
    pub_socket(nullptr),
    relay_socket(nullptr),
    shared_state(nullptr)
{
    if (!context)
        MONERO_ZMQ_THROW("Unable to create ZMQ context");
}

ZmqServer::~ZmqServer()
{
}

void ZmqServer::serve()
{
  try
  {
    // socket must close before `zmq_term` will exit.
    const net::zmq::socket rep = std::move(rep_socket);
    const net::zmq::socket pub = std::move(pub_socket);
    const net::zmq::socket relay = std::move(relay_socket);
    const std::shared_ptr<listener::zmq_pub> state = std::move(shared_state);

    const unsigned init_count = unsigned(bool(pub)) + bool(relay) + bool(state);
    if (!rep || (init_count && init_count != 3))
    {
      MERROR("ZMQ RPC server socket is null");
      return;
    }

    MINFO("ZMQ Server started");

    const int read_flags = pub ? ZMQ_DONTWAIT : 0;
    std::array<zmq_pollitem_t, 3> sockets =
    {{
      {relay.get(), 0, ZMQ_POLLIN, 0},
      {pub.get(), 0, ZMQ_POLLIN, 0},
      {rep.get(), 0, ZMQ_POLLIN, 0}
    }};

    /* This uses XPUB to watch for subscribers, to reduce CPU cycles for
       serialization when the data will be dropped. This is important for block
       serialization, which is done on the p2p threads currently (see
       zmq_pub.cpp).

       XPUB sockets are not thread-safe, so the p2p thread cannot write into
       the socket while we read here for subscribers. A ZMQ_PAIR socket is
       used for inproc notification. No data is every copied to kernel, it is
       all userspace messaging. */

    while (1)
    {
      if (pub)
        MONERO_UNWRAP(net::zmq::retry_op(zmq_poll, sockets.data(), sockets.size(), -1));

      if (sockets[0].revents)
        state->relay_to_pub(relay.get(), pub.get());

      if (sockets[1].revents)
        state->sub_request(MONERO_UNWRAP(net::zmq::receive(pub.get(), ZMQ_DONTWAIT)));

      if (!pub || sockets[2].revents)
      {
        std::string message = MONERO_UNWRAP(net::zmq::receive(rep.get(), read_flags));
        MDEBUG("Received RPC request: \"" << message << "\"");
        epee::byte_slice response = handler.handle(std::move(message));

        const boost::string_ref response_view{reinterpret_cast<const char*>(response.data()), response.size()};
        MDEBUG("Sending RPC reply: \"" << response_view << "\"");
        MONERO_UNWRAP(net::zmq::send(std::move(response), rep.get()));
      }
    }
  }
  catch (const std::system_error& e)
  {
    if (e.code() != net::zmq::make_error_code(ETERM))
      MERROR("ZMQ RPC Server Error: " << e.what());
  }
  catch (const std::exception& e)
  {
    MERROR("ZMQ RPC Server Error: " << e.what());
  }
  catch (...)
  {
    MERROR("Unknown error in ZMQ RPC server");
  }
}

void* ZmqServer::init_rpc(boost::string_ref address, boost::string_ref port)
{
  if (!context)
  {
    MERROR("ZMQ RPC Server already shutdown");
    return nullptr;
  }

  if (address.empty())
    address = "*";
  if (port.empty())
    port = "*";

  std::string bind_address = "tcp://";
  bind_address.append(address.data(), address.size());
  bind_address += ":";
  bind_address.append(port.data(), port.size());

  rep_socket = init_socket(context.get(), ZMQ_REP, {std::addressof(bind_address), 1});
  return bool(rep_socket) ? context.get() : nullptr;
}

std::shared_ptr<listener::zmq_pub> ZmqServer::init_pub(epee::span<const std::string> addresses)
{
  try
  {
    shared_state = std::make_shared<listener::zmq_pub>(context.get());
    pub_socket = init_socket(context.get(), ZMQ_XPUB, addresses);
    if (!pub_socket)
      throw std::runtime_error{"Unable to initialize ZMQ_XPUB socket"};

    const std::string relay_address[] = {listener::zmq_pub::relay_endpoint()};
    relay_socket = init_socket(context.get(), ZMQ_PAIR, relay_address);
    if (!relay_socket)
      throw std::runtime_error{"Unable to initialize ZMQ_PAIR relay"};
  }
  catch (const std::runtime_error& e)
  {
    shared_state = nullptr;
    pub_socket = nullptr;
    relay_socket = nullptr;
    MERROR("Failed to create ZMQ/Pub listener: " << e.what());
    return nullptr;
  }

  return shared_state;
}

void ZmqServer::run()
{
  run_thread = boost::thread(boost::bind(&ZmqServer::serve, this));
}

void ZmqServer::stop()
{
  if (!run_thread.joinable())
    return;

  context.reset(); // destroying context terminates all calls
  run_thread.join();
}

}  // namespace cryptonote

}  // namespace rpc
