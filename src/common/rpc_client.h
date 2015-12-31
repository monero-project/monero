// Copyright (c) 2014-2016, The Monero Project
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

#include "common/http_connection.h"
#include "common/scoped_message_writer.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"
#include "net/http_client.h"
#include "string_tools.h"
#include <boost/lexical_cast.hpp>

namespace tools
{
  class t_rpc_client final
  {
  private:
    epee::net_utils::http::http_simple_client m_http_client;
    uint32_t m_ip;
    uint16_t m_port;
  public:
    t_rpc_client(
        uint32_t ip
      , uint16_t port
      )
      : m_http_client{}
      , m_ip{ip}
      , m_port{port}
    {}

    std::string build_url(std::string const & relative_url) const
    {
      std::string result =
              "http://"
             + epee::string_tools::get_ip_string_from_int32(m_ip)
             + ":"
             + boost::lexical_cast<std::string>(m_port)
             + relative_url;
      return result;
    }

    template <typename T_req, typename T_res>
    bool basic_json_rpc_request(
        T_req & req
      , T_res & res
      , std::string const & method_name
      )
    {
      std::string rpc_url = build_url("/json_rpc");
      t_http_connection connection(&m_http_client, m_ip, m_port);

      bool ok = connection.is_open();
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon";
        return false;
      }
      ok = ok && epee::net_utils::invoke_http_json_rpc(rpc_url, method_name, req, res, m_http_client);
      if (!ok)
      {
        fail_msg_writer() << "Daemon request failed";
        return false;
      }
      else
      {
        return true;
      }
    }

    template <typename T_req, typename T_res>
    bool json_rpc_request(
        T_req & req
      , T_res & res
      , std::string const & method_name
      , std::string const & fail_msg
      )
    {
      std::string rpc_url = build_url("/json_rpc");
      t_http_connection connection(&m_http_client, m_ip, m_port);

      bool ok = connection.is_open();
      ok = ok && epee::net_utils::invoke_http_json_rpc(rpc_url, method_name, req, res, m_http_client);
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon";
        return false;
      }
      else if (res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
      {
        fail_msg_writer() << fail_msg << " -- " << res.status;
        return false;
      }
      else
      {
        return true;
      }
    }

    template <typename T_req, typename T_res>
    bool rpc_request(
        T_req & req
      , T_res & res
      , std::string const & relative_url
      , std::string const & fail_msg
      )
    {
      std::string rpc_url = build_url(relative_url);
      t_http_connection connection(&m_http_client, m_ip, m_port);

      bool ok = connection.is_open();
      ok = ok && epee::net_utils::invoke_http_json_remote_command2(rpc_url, req, res, m_http_client);
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon";
        return false;
      }
      else if (res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
      {
        fail_msg_writer() << fail_msg << " -- " << res.status;
        return false;
      }
      else
      {
        return true;
      }
    }

    bool check_connection()
    {
      t_http_connection connection(&m_http_client, m_ip, m_port);
      return connection.is_open();
    }
  };
}
