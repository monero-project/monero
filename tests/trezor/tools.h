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

#include "misc_log_ex.h"
#include "daemon/daemon.h"
#include "rpc/daemon_handler.h"
#include "rpc/zmq_server.h"
#include "common/password.h"
#include "common/util.h"
#include "daemon/core.h"
#include "daemon/p2p.h"
#include "daemon/protocol.h"
#include "daemon/rpc.h"
#include "daemon/command_server.h"
#include "daemon/command_server.h"
#include "daemon/command_line_args.h"
#include "version.h"

namespace tools {

class options {
public:
  static void set_option(boost::program_options::variables_map &vm, const std::string &key, const boost::program_options::variable_value &pv);
  static void build_options(boost::program_options::variables_map & vm, const boost::program_options::options_description & desc_params);

  template<typename T, bool required, bool dependent, int NUM_DEPS>
  static void set_option(boost::program_options::variables_map &vm, const command_line::arg_descriptor<T, required, dependent, NUM_DEPS> &arg, const boost::program_options::variable_value &pv)
  {
    set_option(vm, arg.name, pv);
  }
};

};

