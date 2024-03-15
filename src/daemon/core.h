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

#include "blocks/blocks.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "misc_log_ex.h"
#include "daemon/command_line_args.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{

class t_core final
{
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    cryptonote::core::init_options(option_spec);
  }
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  cryptonote::core m_core;
  // TEMPORARY HACK - Yes, this creates a copy, but otherwise the original
  // variable map could go out of scope before the run method is called
  boost::program_options::variables_map const m_vm_HACK;
public:
  t_core(
      boost::program_options::variables_map const & vm
    )
    : m_core{nullptr}
    , m_vm_HACK{vm}
  {
    //initialize core here
    MGINFO("Initializing core...");
#if defined(PER_BLOCK_CHECKPOINT)
    const cryptonote::GetCheckpointsCallback& get_checkpoints = blocks::GetCheckpointsData;
#else
    const cryptonote::GetCheckpointsCallback& get_checkpoints = nullptr;
#endif

    if (command_line::is_arg_defaulted(vm, daemon_args::arg_proxy) && command_line::get_arg(vm, daemon_args::arg_proxy_allow_dns_leaks)) {
      MLOG_RED(el::Level::Warning, "--" << daemon_args::arg_proxy_allow_dns_leaks.name << " is enabled, but --"
        << daemon_args::arg_proxy.name << " is not specified.");
    }

    const bool allow_dns = command_line::is_arg_defaulted(vm, daemon_args::arg_proxy) || command_line::get_arg(vm, daemon_args::arg_proxy_allow_dns_leaks);
    if (!m_core.init(m_vm_HACK, nullptr, get_checkpoints, allow_dns))
    {
      throw std::runtime_error("Failed to initialize core");
    }
    MGINFO("Core initialized OK");
  }

  // TODO - get rid of circular dependencies in internals
  void set_protocol(t_protocol_raw & protocol)
  {
    m_core.set_cryptonote_protocol(&protocol);
  }

  bool run()
  {
    return true;
  }

  cryptonote::core & get()
  {
    return m_core;
  }

  ~t_core()
  {
    MGINFO("Deinitializing core...");
    try {
      m_core.deinit();
      m_core.set_cryptonote_protocol(nullptr);
    } catch (...) {
      MERROR("Failed to deinitialize core...");
    }
  }
};

}
