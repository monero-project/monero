// Copyright (c) 2014, The Monero Project
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

#include "rpcwallet/wallet_daemon.h"

#include "common/util.h"
#include "rpcwallet/wallet_rpc_server.h"

namespace tools {

t_wallet_daemon::t_wallet_daemon(
      std::string wallet_file
    , std::string wallet_password
    , std::string daemon_address
    , std::string bind_ip
    , std::string port
  )
  : mp_server{new wallet_rpc_server{
      wallet_file
    , wallet_password
    , daemon_address
    , std::move(bind_ip)
    , std::move(port)
    }}
{}

t_wallet_daemon::~t_wallet_daemon() = default;

// MSVC is brain-dead and can't default this...
t_wallet_daemon::t_wallet_daemon(t_wallet_daemon && other)
{
  if (this != &other)
  {
    mp_server = std::move(other.mp_server);
    other.mp_server.reset(nullptr);
  }
}

// or this
t_wallet_daemon & t_wallet_daemon::operator=(t_wallet_daemon && other)
{
  if (this != &other)
  {
    mp_server = std::move(other.mp_server);
    other.mp_server.reset(nullptr);
  }
  return *this;
}

bool t_wallet_daemon::run()
{
  if (nullptr == mp_server)
  {
    throw std::runtime_error{"Can't run stopped wallet daemon"};
  }

  tools::signal_handler::install(std::bind(&tools::t_wallet_daemon::stop, this));

  mp_server->run();
  return true;
}

void t_wallet_daemon::stop()
{
  if (nullptr == mp_server)
  {
    throw std::runtime_error{"Can't stop stopped wallet daemon"};
  }
  mp_server->stop();
  mp_server.reset(nullptr); // Ensure resources are cleaned up before we return
}

} // namespace daemonize
