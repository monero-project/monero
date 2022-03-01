// Copyright (c) 2016-2022, The Monero Project
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

#pragma once

#include <boost/thread/thread.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <memory>
#include <string>

#include "common/command_line.h"
#include "cryptonote_basic/fwd.h"
#include "net/zmq.h"
#include "rpc/fwd.h"
#include "rpc/rpc_handler.h"
#include "span.h"

namespace cryptonote
{

namespace rpc
{

class ZmqServer final
{
  public:

    ZmqServer(RpcHandler& h);

    ~ZmqServer();

    void serve();

    //! \return ZMQ context on success, `nullptr` on failure
    void* init_rpc(boost::string_ref address, boost::string_ref port);

    //! \return `nullptr` on errors.
    std::shared_ptr<listener::zmq_pub> init_pub(epee::span<const std::string> addresses);

    void run();
    void stop();

  private:
    RpcHandler& handler;

    net::zmq::context context;

    boost::thread run_thread;

    net::zmq::socket rep_socket;
    net::zmq::socket pub_socket;
    net::zmq::socket relay_socket;
    std::shared_ptr<listener::zmq_pub> shared_state;
};

}  // namespace cryptonote

}  // namespace rpc
