/**
@file
@details


Passing RPC commands:

@image html images/other/runtime-commands.png

*/

// Copyright (c) 2014-2024, The Monero Project
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

#include <boost/optional/optional_fwd.hpp>
#include "common/common_fwd.h"
#include "console_handler.h"
#include "daemon/command_parser_executor.h"

namespace daemonize {

class t_command_server {
private:
  t_command_parser_executor m_parser;
  epee::console_handlers_binder m_command_lookup;
  bool m_is_rpc;

public:
  t_command_server(
      uint32_t ip
    , uint16_t port
    , const boost::optional<tools::login>& login
    , const epee::net_utils::ssl_options_t& ssl_options
    , bool is_rpc = true
    , cryptonote::core_rpc_server* rpc_server = NULL
    );

  bool process_command_str(const std::string& cmd);

  bool process_command_vec(const std::vector<std::string>& cmd);

  bool start_handling(std::function<void(void)> exit_handler = NULL);

  void stop_handling();

private:
  bool help(const std::vector<std::string>& args);
  bool apropos(const std::vector<std::string>& args);

  std::string get_commands_str();
  std::string get_command_usage(const std::vector<std::string> &args);
};

} // namespace daemonize
