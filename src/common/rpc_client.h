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

#include <boost/optional/optional.hpp>
#include <thread>
#include <chrono>
#include <random>

#include "common/http_connection.h"
#include "common/scoped_message_writer.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"
#include "net/http_auth.h"
#include "net/http_client.h"
#include "net/net_ssl.h"
#include "string_tools.h"

namespace tools
{
  // ----------------------------------------
  // Result Types
  // ----------------------------------------
  enum class rpc_result_code
  {
    OK,
    BUSY,
    NETWORK_ERROR,
    RPC_ERROR,
    TIMEOUT
  };

  template <typename T>
  struct rpc_result
  {
    rpc_result_code code;
    T data;
    std::string message;

    bool is_ok() const { return code == rpc_result_code::OK; }
  };

  // ----------------------------------------
  // Retry Policy
  // ----------------------------------------
  struct retry_policy
  {
    int max_retries = 5;
    int base_delay_ms = 500;
    bool exponential_backoff = true;
    bool jitter = true;

    int compute_delay(int attempt) const
    {
      int delay = base_delay_ms;

      if (exponential_backoff)
        delay *= (1 << attempt);

      if (jitter)
      {
        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, delay / 2);
        delay += dist(rng);
      }

      return delay;
    }
  };

  // ----------------------------------------
  // RPC Client
  // ----------------------------------------
  class t_rpc_client final
  {
  private:
    epee::net_utils::http::http_simple_client m_http_client;
    retry_policy m_policy;

  public:
    t_rpc_client(
        uint32_t ip,
        uint16_t port,
        boost::optional<epee::net_utils::http::login> user,
        epee::net_utils::ssl_options_t ssl_options,
        retry_policy policy = {})
        : m_http_client{}, m_policy(policy)
    {
      m_http_client.set_server(
          epee::string_tools::get_ip_string_from_int32(ip),
          std::to_string(port),
          std::move(user),
          std::move(ssl_options));
    }

  private:
    // ----------------------------------------
    // Core Retry Executor
    // ----------------------------------------
    template <typename T_res, typename Fn>
    rpc_result<T_res> execute_with_retry(Fn &&fn, const std::string &fail_msg)
    {
      for (int attempt = 0; attempt < m_policy.max_retries; ++attempt)
      {
        t_http_connection connection(&m_http_client);

        if (!connection.is_open())
        {
          return {rpc_result_code::NETWORK_ERROR, {}, "Connection failed"};
        }

        T_res res{};
        bool ok = fn(res);

        if (!ok)
        {
          return {rpc_result_code::NETWORK_ERROR, {}, fail_msg + " -- request failed"};
        }

        if (res.status == CORE_RPC_STATUS_OK)
        {
          return {rpc_result_code::OK, res, ""};
        }

        if (res.status == CORE_RPC_STATUS_BUSY)
        {
          if (attempt < m_policy.max_retries - 1)
          {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_policy.compute_delay(attempt)));
            continue;
          }

          return {rpc_result_code::BUSY, res, "Daemon busy after retries"};
        }

        return {rpc_result_code::RPC_ERROR, res, res.status};
      }

      return {rpc_result_code::TIMEOUT, {}, "Exceeded retry attempts"};
    }

  public:
    // JSON RPC
    template <typename T_req, typename T_res>
    rpc_result<T_res> json_rpc_request(
        T_req &req,
        const std::string &method_name,
        const std::string &fail_msg)
    {
      return execute_with_retry<T_res>(
          [&](T_res &res)
          {
            return epee::net_utils::invoke_http_json_rpc(
                "/json_rpc",
                method_name,
                req,
                res,
                m_http_client,
                t_http_connection::TIMEOUT());
          },
          fail_msg);
    }

    // REST-style RPC
    template <typename T_req, typename T_res>
    rpc_result<T_res> rpc_request(
        T_req &req,
        const std::string &relative_url,
        const std::string &fail_msg)
    {
      return execute_with_retry<T_res>(
          [&](T_res &res)
          {
            return epee::net_utils::invoke_http_json(
                relative_url,
                req,
                res,
                m_http_client,
                t_http_connection::TIMEOUT());
          },
          fail_msg);
    }

    // lightweight (no retry, no status handling)
    template <typename T_req, typename T_res>
    bool basic_json_rpc_request(
        T_req &req,
        T_res &res,
        const std::string &method_name)
    {
      t_http_connection connection(&m_http_client);

      if (!connection.is_open())
      {
        fail_msg_writer() << "Couldn't connect to daemon: "
                          << m_http_client.get_host() << ":"
                          << m_http_client.get_port();
        return false;
      }

      return epee::net_utils::invoke_http_json_rpc(
          "/json_rpc",
          method_name,
          req,
          res,
          m_http_client,
          t_http_connection::TIMEOUT());
    }

    bool check_connection()
    {
      t_http_connection connection(&m_http_client);
      return connection.is_open();
    }
  };
}