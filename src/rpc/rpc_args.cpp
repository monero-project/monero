// Copyright (c) 2014-2017, The Monero Project
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
#include "rpc_args.h"

#include <boost/asio/ip/address.hpp>
#include "common/command_line.h"
#include "common/i18n.h"

namespace cryptonote
{
  rpc_args::descriptors::descriptors()
     : rpc_bind_ip({"rpc-bind-ip", rpc_args::tr("Specify ip to bind rpc server"), "127.0.0.1"})
     , rpc_login({"rpc-login", rpc_args::tr("Specify username[:password] required for RPC server"), "", true})
     , confirm_external_bind({"confirm-external-bind", rpc_args::tr("Confirm rpc-bind-ip value is NOT a loopback (local) IP")})
  {}

  const char* rpc_args::tr(const char* str) { return i18n_translate(str, "cryptonote::rpc_args"); }

  void rpc_args::init_options(boost::program_options::options_description& desc)
  {
    const descriptors arg{};
    command_line::add_arg(desc, arg.rpc_bind_ip);
    command_line::add_arg(desc, arg.rpc_login);
    command_line::add_arg(desc, arg.confirm_external_bind);
  }

  boost::optional<rpc_args> rpc_args::process(const boost::program_options::variables_map& vm)
  {
    const descriptors arg{};
    rpc_args config{};
    
    config.bind_ip = command_line::get_arg(vm, arg.rpc_bind_ip);
    if (!config.bind_ip.empty())
    {
      // always parse IP here for error consistency
      boost::system::error_code ec{};
      const auto parsed_ip = boost::asio::ip::address::from_string(config.bind_ip, ec);
      if (ec)
      {
        LOG_ERROR(tr("Invalid IP address given for --") << arg.rpc_bind_ip.name);
        return boost::none;
      }

      if (!parsed_ip.is_loopback() && !command_line::get_arg(vm, arg.confirm_external_bind))
      {
        LOG_ERROR(
          "--" << arg.rpc_bind_ip.name <<
          tr(" permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --") <<
          arg.confirm_external_bind.name
        );
        return boost::none;
      }
    }

    if (command_line::has_arg(vm, arg.rpc_login))
    {
      config.login = tools::login::parse(command_line::get_arg(vm, arg.rpc_login), true, "RPC server password");
      if (!config.login)
        return boost::none;

      if (config.login->username.empty())
      {
        LOG_ERROR(tr("Username specified with --") << arg.rpc_login.name << tr(" cannot be empty"));
        return boost::none;
      }
    }

    return {std::move(config)};
  }
}
