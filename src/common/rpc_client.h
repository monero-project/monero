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
  class t_rpc_client final
  {
  private:
    epee::net_utils::http::http_simple_client m_http_client;
  public:
    t_rpc_client(
        uint32_t ip
      , uint16_t port
      , boost::optional<epee::net_utils::http::login> user
      , epee::net_utils::ssl_options_t ssl_options
      )
      : m_http_client{}
    {
      m_http_client.set_server(
        epee::string_tools::get_ip_string_from_int32(ip), std::to_string(port), std::move(user), std::move(ssl_options)
      );
    }

    t_rpc_client(
        const std::string &daemon_addr
      , boost::optional<epee::net_utils::http::login> user
      , epee::net_utils::ssl_options_t ssl_options
      )
      : m_http_client{}
    {
      m_http_client.set_server(
        daemon_addr, std::move(user), std::move(ssl_options)
      );
    }

    template <typename T_req, typename T_res>
    bool basic_json_rpc_request(
        T_req & req
      , T_res & res
      , std::string const & method_name
      )
    {
      t_http_connection connection(&m_http_client);

      bool ok = connection.is_open();
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
        return false;
      }
      ok = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, res, m_http_client, t_http_connection::TIMEOUT());
      if (!ok)
      {
        fail_msg_writer() << "basic_json_rpc_request: Daemon request failed";
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
      t_http_connection connection(&m_http_client);

      bool ok = connection.is_open();
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
        return false;
      }
      ok = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, res, m_http_client, t_http_connection::TIMEOUT());
      if (!ok || res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
      {
        fail_msg_writer() << fail_msg << " -- json_rpc_request: " << res.status;
        return false;
      }
      else
      {
        return true;
      }
    }

    template <typename T_req, typename T_res>
    bool basic_rpc_request(
        T_req & req
      , T_res & res
      , std::string const & relative_url
      )
    {
      t_http_connection connection(&m_http_client);

      bool ok = connection.is_open();
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
        return false;
      }
      ok = epee::net_utils::invoke_http_json(relative_url, req, res, m_http_client, t_http_connection::TIMEOUT());
      if (!ok)
      {
        fail_msg_writer() << "basic_rpc_request: Daemon request failed";
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
      t_http_connection connection(&m_http_client);

      bool ok = connection.is_open();
      if (!ok)
      {
        fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
        return false;
      }
      ok = epee::net_utils::invoke_http_json(relative_url, req, res, m_http_client, t_http_connection::TIMEOUT());
      if (!ok || res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
      {
        fail_msg_writer() << fail_msg << "-- rpc_request: " << res.status;
        return false;
      }
      else
      {
        return true;
      }
    }

    bool check_connection()
    {
      t_http_connection connection(&m_http_client);
      return connection.is_open();
    }
  };

  class t_daemon_rpc_client final
  {
  private:
    t_rpc_client m_rpc_client;
  public:
    t_daemon_rpc_client(
        const std::string &daemon_addr
      , const boost::optional<epee::net_utils::http::login> &daemon_login
      , const epee::net_utils::ssl_options_t ssl_support
      )
      : m_rpc_client{daemon_addr, daemon_login, ssl_support}
    {
    }

    cryptonote::COMMAND_RPC_GET_HEIGHT::response get_height()
    {
      cryptonote::COMMAND_RPC_GET_HEIGHT::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_GET_HEIGHT::response res = AUTO_VAL_INIT(res);
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_rpc_request(req, res, "/get_height"), "failed to get height");
      return res;
    }

    cryptonote::COMMAND_RPC_POP_BLOCKS::response pop_blocks(uint64_t nblocks = 1)
    {
      cryptonote::COMMAND_RPC_POP_BLOCKS::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_POP_BLOCKS::response res = AUTO_VAL_INIT(res);
      req.nblocks = nblocks;
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_rpc_request(req, res, "/pop_blocks"), "failed to pop blocks");
      return res;
    }

    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response get_transactions(const std::vector<std::string> &txs_hashes)
    {
      cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res = AUTO_VAL_INIT(res);
      req.txs_hashes = txs_hashes;
      req.decode_as_json = false;
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_rpc_request(req, res, "/get_transactions"), "failed to get transactions");
      return res;
    }

    cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::response flush_txpool()
    {
      cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::response res = AUTO_VAL_INIT(res);
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_json_rpc_request(req, res, "flush_txpool"), "failed to flush txpool");
      return res;
    }

    cryptonote::COMMAND_RPC_GENERATEBLOCKS::response generateblocks(const std::string &address, uint64_t amount_of_blocks)
    {
      cryptonote::COMMAND_RPC_GENERATEBLOCKS::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_GENERATEBLOCKS::response res = AUTO_VAL_INIT(res);
      req.amount_of_blocks = amount_of_blocks;
      req.wallet_address = address;
      req.prev_block = "";
      req.starting_nonce = 0;
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_json_rpc_request(req, res, "generateblocks"), "failed to generate blocks");
      return res;
    }

    cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::response get_last_block_header()
    {
      cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::request req = AUTO_VAL_INIT(req);
      cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::response res = AUTO_VAL_INIT(res);
      req.client = "";
      CHECK_AND_ASSERT_THROW_MES(m_rpc_client.basic_json_rpc_request(req, res, "get_last_block_header"), "failed to get last block header");
      return res;
    }
  };
}
