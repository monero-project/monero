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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

/*!
 * \file json_rpc.cpp
 * \brief Monero RPC deprecated
 * 
 * Uses net_skeleton (fossa) as the HTTP server to translate JSON RPC requests
 * into 0MQ IPC requests, sends them to the daemon, translates back 0MQ IPC responses
 * into JSON RPC responses all as per the old monero JSON RPC API.
 * 
 * Written for backwards compatiblity purposes.
 */

#include "daemon_deprecated_rpc.h"
#include <signal.h>
#include <iostream>

static bool execute = true;
void trap(int signal) {
  RPC::DaemonDeprecated::stop();
  execute = false;
}

int main() {
  int res = RPC::DaemonDeprecated::start();
  if (res == RPC::DaemonDeprecated::FAILURE_HTTP_SERVER) {
    std::cerr << "Couldn't start HTTP server\n";
    execute = false;
  } else if (res == RPC::DaemonDeprecated::FAILURE_DAEMON_NOT_RUNNING) {
    std::cerr << "Couldn't connect to daemon\n";
    execute = false;
  }
  signal(SIGINT, &trap);
  while (execute) {

  }
  std::cout << "out!\n";
  return 0;
}
