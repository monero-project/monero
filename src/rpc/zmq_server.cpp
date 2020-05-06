// Copyright (c) 2016-2019, The Monero Project
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
#include <cstdint>
#include <system_error>

#include "byte_slice.h"

namespace cryptonote
{

namespace
{
  constexpr const int num_zmq_threads = 1;
  constexpr const std::int64_t max_message_size = 10 * 1024 * 1024; // 10 MiB
  constexpr const std::chrono::seconds linger_timeout{2}; // wait period for pending out messages
}

namespace rpc
{

ZmqServer::ZmqServer(RpcHandler& h) :
    handler(h),
    context(zmq_init(num_zmq_threads))
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
    const net::zmq::socket socket = std::move(rep_socket);
    if (!socket)
    {
      MERROR("ZMQ RPC server reply socket is null");
      return;
    }

    while (1)
    {
      const std::string message = MONERO_UNWRAP(net::zmq::receive(socket.get()));
      MDEBUG("Received RPC request: \"" << message << "\"");
      epee::byte_slice response = handler.handle(message);

      const boost::string_ref response_view{reinterpret_cast<const char*>(response.data()), response.size()};
      MDEBUG("Sending RPC reply: \"" << response_view << "\"");
      MONERO_UNWRAP(net::zmq::send(std::move(response), socket.get()));
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

bool ZmqServer::addIPCSocket(const boost::string_ref address, const boost::string_ref port)
{
  MERROR("ZmqServer::addIPCSocket not yet implemented!");
  return false;
}

bool ZmqServer::addTCPSocket(boost::string_ref address, boost::string_ref port)
{
  if (!context)
  {
    MERROR("ZMQ RPC Server already shutdown");
    return false;
  }

  rep_socket.reset(zmq_socket(context.get(), ZMQ_REP));
  if (!rep_socket)
  {
    MONERO_LOG_ZMQ_ERROR("ZMQ RPC Server socket create failed");
    return false;
  }

  if (zmq_setsockopt(rep_socket.get(), ZMQ_MAXMSGSIZE, std::addressof(max_message_size), sizeof(max_message_size)) != 0)
  {
    MONERO_LOG_ZMQ_ERROR("Failed to set maximum incoming message size");
    return false;
  }

  static constexpr const int linger_value = std::chrono::milliseconds{linger_timeout}.count();
  if (zmq_setsockopt(rep_socket.get(), ZMQ_LINGER, std::addressof(linger_value), sizeof(linger_value)) != 0)
  {
    MONERO_LOG_ZMQ_ERROR("Failed to set linger timeout");
    return false;
  }

  if (address.empty())
    address = "*";
  if (port.empty())
    port = "*";

  std::string bind_address = "tcp://";
  bind_address.append(address.data(), address.size());
  bind_address += ":";
  bind_address.append(port.data(), port.size());

  if (zmq_bind(rep_socket.get(), bind_address.c_str()) < 0)
  {
    MONERO_LOG_ZMQ_ERROR("ZMQ RPC Server bind failed");
    return false;
  }
  return true;
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
