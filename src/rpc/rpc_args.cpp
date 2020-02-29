// Copyright (c) 2014-2019, The Monero Project
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

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include "common/command_line.h"
#include "common/i18n.h"
#include "hex.h"

namespace cryptonote
{
  namespace
  {
    boost::optional<epee::net_utils::ssl_options_t> do_process_ssl(const boost::program_options::variables_map& vm, const rpc_args::descriptors& arg, const bool any_cert_option)
    {
      bool ssl_required = false;
      epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_enabled;
      if (any_cert_option && command_line::get_arg(vm, arg.rpc_ssl_allow_any_cert))
        ssl_options.verification = epee::net_utils::ssl_verification_t::none;
      else
      {
        std::string ssl_ca_file = command_line::get_arg(vm, arg.rpc_ssl_ca_certificates);
        const std::vector<std::string> ssl_allowed_fingerprints = command_line::get_arg(vm, arg.rpc_ssl_allowed_fingerprints);

        std::vector<std::vector<uint8_t>> allowed_fingerprints{ ssl_allowed_fingerprints.size() };
        std::transform(ssl_allowed_fingerprints.begin(), ssl_allowed_fingerprints.end(), allowed_fingerprints.begin(), epee::from_hex::vector);
        for (const auto &fpr: allowed_fingerprints)
        {
          if (fpr.size() != SSL_FINGERPRINT_SIZE)
          {
            MERROR("SHA-256 fingerprint should be " BOOST_PP_STRINGIZE(SSL_FINGERPRINT_SIZE) " bytes long.");
            return boost::none;
          }
        }

        if (!allowed_fingerprints.empty() || !ssl_ca_file.empty())
        {
          ssl_required = true;
          ssl_options = epee::net_utils::ssl_options_t{
            std::move(allowed_fingerprints), std::move(ssl_ca_file)
          };

          if (command_line::get_arg(vm, arg.rpc_ssl_allow_chained))
            ssl_options.verification = epee::net_utils::ssl_verification_t::user_ca;
        }
      }

      // user specified CA file or fingeprints implies enabled SSL by default
      if (!ssl_required && !epee::net_utils::ssl_support_from_string(ssl_options.support, command_line::get_arg(vm, arg.rpc_ssl)))
      {
        MERROR("Invalid argument for " << std::string(arg.rpc_ssl.name));
        return boost::none;
      }

      ssl_options.auth = epee::net_utils::ssl_authentication_t{
        command_line::get_arg(vm, arg.rpc_ssl_private_key), command_line::get_arg(vm, arg.rpc_ssl_certificate)
      };

      return {std::move(ssl_options)};
    }
  } // anonymous

  rpc_args::descriptors::descriptors()
     : rpc_bind_ip({"rpc-bind-ip", rpc_args::tr("Specify IP to bind RPC server"), "127.0.0.1"})
     , rpc_bind_ipv6_address({"rpc-bind-ipv6-address", rpc_args::tr("Specify IPv6 address to bind RPC server"), "::1"})
     , rpc_use_ipv6({"rpc-use-ipv6", rpc_args::tr("Allow IPv6 for RPC"), false})
     , rpc_ignore_ipv4({"rpc-ignore-ipv4", rpc_args::tr("Ignore unsuccessful IPv4 bind for RPC"), false})
     , rpc_login({"rpc-login", rpc_args::tr("Specify username[:password] required for RPC server"), "", true})
     , confirm_external_bind({"confirm-external-bind", rpc_args::tr("Confirm rpc-bind-ip value is NOT a loopback (local) IP")})
     , rpc_access_control_origins({"rpc-access-control-origins", rpc_args::tr("Specify a comma separated list of origins to allow cross origin resource sharing"), ""})
     , rpc_ssl({"rpc-ssl", rpc_args::tr("Enable SSL on RPC connections: enabled|disabled|autodetect"), "autodetect"})
     , rpc_ssl_private_key({"rpc-ssl-private-key", rpc_args::tr("Path to a PEM format private key"), ""})
     , rpc_ssl_certificate({"rpc-ssl-certificate", rpc_args::tr("Path to a PEM format certificate"), ""})
     , rpc_ssl_ca_certificates({"rpc-ssl-ca-certificates", rpc_args::tr("Path to file containing concatenated PEM format certificate(s) to replace system CA(s)."), ""})
     , rpc_ssl_allowed_fingerprints({"rpc-ssl-allowed-fingerprints", rpc_args::tr("List of certificate fingerprints to allow")})
     , rpc_ssl_allow_chained({"rpc-ssl-allow-chained", rpc_args::tr("Allow user (via --rpc-ssl-certificates) chain certificates"), false})
     , rpc_ssl_allow_any_cert({"rpc-ssl-allow-any-cert", rpc_args::tr("Allow any peer certificate"), false})
     , disable_rpc_ban({"disable-rpc-ban", rpc_args::tr("Do not ban hosts on RPC errors"), false, false})
  {}

  const char* rpc_args::tr(const char* str) { return i18n_translate(str, "cryptonote::rpc_args"); }

  void rpc_args::init_options(boost::program_options::options_description& desc, const bool any_cert_option)
  {
    const descriptors arg{};
    command_line::add_arg(desc, arg.rpc_bind_ip);
    command_line::add_arg(desc, arg.rpc_bind_ipv6_address);
    command_line::add_arg(desc, arg.rpc_use_ipv6);
    command_line::add_arg(desc, arg.rpc_ignore_ipv4);
    command_line::add_arg(desc, arg.rpc_login);
    command_line::add_arg(desc, arg.confirm_external_bind);
    command_line::add_arg(desc, arg.rpc_access_control_origins);
    command_line::add_arg(desc, arg.rpc_ssl);
    command_line::add_arg(desc, arg.rpc_ssl_private_key);
    command_line::add_arg(desc, arg.rpc_ssl_certificate);
    command_line::add_arg(desc, arg.rpc_ssl_ca_certificates);
    command_line::add_arg(desc, arg.rpc_ssl_allowed_fingerprints);
    command_line::add_arg(desc, arg.rpc_ssl_allow_chained);
    command_line::add_arg(desc, arg.disable_rpc_ban);
    if (any_cert_option)
      command_line::add_arg(desc, arg.rpc_ssl_allow_any_cert);
  }

  boost::optional<rpc_args> rpc_args::process(const boost::program_options::variables_map& vm, const bool any_cert_option)
  {
    const descriptors arg{};
    rpc_args config{};
    
    config.bind_ip = command_line::get_arg(vm, arg.rpc_bind_ip);
    config.bind_ipv6_address = command_line::get_arg(vm, arg.rpc_bind_ipv6_address);
    config.use_ipv6 = command_line::get_arg(vm, arg.rpc_use_ipv6);
    config.require_ipv4 = !command_line::get_arg(vm, arg.rpc_ignore_ipv4);
    config.disable_rpc_ban = command_line::get_arg(vm, arg.disable_rpc_ban);
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
    if (!config.bind_ipv6_address.empty())
    {
      // allow square braces, but remove them here if present
      if (config.bind_ipv6_address.find('[') != std::string::npos)
      {
        config.bind_ipv6_address = config.bind_ipv6_address.substr(1, config.bind_ipv6_address.size() - 2);
      }


      // always parse IP here for error consistency
      boost::system::error_code ec{};
      const auto parsed_ip = boost::asio::ip::address::from_string(config.bind_ipv6_address, ec);
      if (ec)
      {
        LOG_ERROR(tr("Invalid IP address given for --") << arg.rpc_bind_ipv6_address.name);
        return boost::none;
      }

      if (!parsed_ip.is_loopback() && !command_line::get_arg(vm, arg.confirm_external_bind))
      {
        LOG_ERROR(
          "--" << arg.rpc_bind_ipv6_address.name <<
          tr(" permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --") <<
          arg.confirm_external_bind.name
        );
        return boost::none;
      }
    }

    const char *env_rpc_login = nullptr;
    const bool has_rpc_arg = command_line::has_arg(vm, arg.rpc_login);
    const bool use_rpc_env = !has_rpc_arg && (env_rpc_login = getenv("RPC_LOGIN")) != nullptr && strlen(env_rpc_login) > 0;
    boost::optional<tools::login> login{};
    if (has_rpc_arg || use_rpc_env)
    {
      config.login = tools::login::parse(
          has_rpc_arg ? command_line::get_arg(vm, arg.rpc_login) : std::string(env_rpc_login), true, [](bool verify) {
            return tools::password_container::prompt(verify, "RPC server password");
          });

      if (!config.login)
        return boost::none;

      if (config.login->username.empty())
      {
        LOG_ERROR(tr("Username specified with --") << arg.rpc_login.name << tr(" cannot be empty"));
        return boost::none;
      }
    }

    auto access_control_origins_input = command_line::get_arg(vm, arg.rpc_access_control_origins);
    if (!access_control_origins_input.empty())
    {
      if (!config.login)
      {
        LOG_ERROR(arg.rpc_access_control_origins.name  << tr(" requires RPC server password --") << arg.rpc_login.name << tr(" cannot be empty"));
        return boost::none;
      }

      std::vector<std::string> access_control_origins;
      boost::split(access_control_origins, access_control_origins_input, boost::is_any_of(","));
      std::for_each(access_control_origins.begin(), access_control_origins.end(), boost::bind(&boost::trim<std::string>, _1, std::locale::classic()));
      config.access_control_origins = std::move(access_control_origins);
    }

    auto ssl_options = do_process_ssl(vm, arg, any_cert_option);
    if (!ssl_options)
      return boost::none;
    config.ssl_options = std::move(*ssl_options);

    return {std::move(config)};
  }

  boost::optional<epee::net_utils::ssl_options_t> rpc_args::process_ssl(const boost::program_options::variables_map& vm, const bool any_cert_option)
  {
    const descriptors arg{};
    return do_process_ssl(vm, arg, any_cert_option);
  }
}
