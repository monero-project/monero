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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <atomic>

#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "include_base_utils.h"
#include "string_tools.h"
#include "net/levin_protocol_handler_async.h"
#include "net/abstract_tcp_server2.h"
#include "serialization/keyvalue_serialization.h"

#include "../unit_tests/unit_tests_utils.h"

namespace net_load_tests
{
  struct test_connection_context : epee::net_utils::connection_context_base
  {
    test_connection_context(): epee::net_utils::connection_context_base(boost::uuids::nil_uuid(), {}, false, false), m_closed(false) {}
    volatile bool m_closed;
  };

  typedef epee::levin::async_protocol_handler<test_connection_context> test_levin_protocol_handler;
  typedef epee::levin::async_protocol_handler_config<test_connection_context> test_levin_protocol_handler_config;
  typedef epee::net_utils::connection<test_levin_protocol_handler> test_connection;
  typedef epee::net_utils::boosted_tcp_server<test_levin_protocol_handler> test_tcp_server;

  struct test_levin_commands_handler : public epee::levin::levin_commands_handler<test_connection_context>
  {
    test_levin_commands_handler()
      //: m_return_code(LEVIN_OK)
      //, m_last_command(-1)
    {
    }

    virtual int invoke(int command, const epee::span<const uint8_t> in_buff, std::string& buff_out, test_connection_context& context)
    {
      //m_invoke_counter.inc();
      //std::unique_lock<std::mutex> lock(m_mutex);
      //m_last_command = command;
      //m_last_in_buf = in_buff;
      //buff_out = m_invoke_out_buf;
      //return m_return_code;
      return LEVIN_OK;
    }

    virtual int notify(int command, const epee::span<const uint8_t> in_buff, test_connection_context& context)
    {
      //m_notify_counter.inc();
      //std::unique_lock<std::mutex> lock(m_mutex);
      //m_last_command = command;
      //m_last_in_buf = in_buff;
      //return m_return_code;
      return LEVIN_OK;
    }

    virtual void callback(test_connection_context& context)
    {
      //m_callback_counter.inc();
      //std::cout << "test_levin_commands_handler::callback()" << std::endl;
    }

    virtual void on_connection_new(test_connection_context& context)
    {
      m_new_connection_counter.inc();
      //std::cout << "test_levin_commands_handler::on_connection_new()" << std::endl;
    }

    virtual void on_connection_close(test_connection_context& context)
    {
      m_close_connection_counter.inc();
      //std::cout << "test_levin_commands_handler::on_connection_close()" << std::endl;
    }

    //size_t invoke_counter() const { return m_invoke_counter.get(); }
    //size_t notify_counter() const { return m_notify_counter.get(); }
    //size_t callback_counter() const { return m_callback_counter.get(); }
    size_t new_connection_counter() const { return m_new_connection_counter.get(); }
    size_t close_connection_counter() const { return m_close_connection_counter.get(); }

    //int return_code() const { return m_return_code; }
    //void return_code(int v) { m_return_code = v; }

    //const std::string& invoke_out_buf() const { return m_invoke_out_buf; }
    //void invoke_out_buf(const std::string& v) { m_invoke_out_buf = v; }

    //int last_command() const { return m_last_command; }
    //const std::string& last_in_buf() const { return m_last_in_buf; }

  protected:
    //unit_test::call_counter m_invoke_counter;
    //unit_test::call_counter m_notify_counter;
    //unit_test::call_counter m_callback_counter;
    unit_test::call_counter m_new_connection_counter;
    unit_test::call_counter m_close_connection_counter;

    //std::mutex m_mutex;

    //int m_return_code;
    //std::string m_invoke_out_buf;

    //int m_last_command;
    //std::string m_last_in_buf;
  };

  class open_close_test_helper
  {
  public:
    open_close_test_helper(test_tcp_server& tcp_server, size_t open_request_target, size_t max_opened_connection_count)
      : m_tcp_server(tcp_server)
      , m_max_opened_connection_count(max_opened_connection_count)
      , m_opened_connection_count(0)
      , m_next_opened_conn_idx(0)
      , m_next_closed_conn_idx(0)
      , m_connections(open_request_target)
    {
      for (auto& conn_id : m_connections)
        conn_id = boost::uuids::nil_uuid();
    }

    bool handle_new_connection(const boost::uuids::uuid& connection_id, bool ignore_close_fails = false)
    {
      size_t idx = m_next_opened_conn_idx.fetch_add(1, std::memory_order_relaxed);
      if (idx >= m_connections.size())
      {
        LOG_PRINT_L0("ERROR: connections overflow");
        exit(1);
      }
      m_connections[idx] = connection_id;

      size_t prev_connection_count = m_opened_connection_count.fetch_add(1, std::memory_order_relaxed);
      if (m_max_opened_connection_count <= prev_connection_count)
      {
        return close_next_connection(ignore_close_fails);
      }

      return true;
    }

    void close_remaining_connections()
    {
      while (close_next_connection(false));
    }

    bool close_next_connection(bool ignore_close_fails)
    {
      size_t idx = m_next_closed_conn_idx.fetch_add(1, std::memory_order_relaxed);
      if (m_next_opened_conn_idx.load(std::memory_order_relaxed) <= idx)
      {
        LOG_PRINT_L0("Not enough opened connections");
        return false;
      }
      if (m_connections[idx].is_nil())
      {
        LOG_PRINT_L0("Connection isn't opened");
        return false;
      }
      if (!m_tcp_server.get_config_object().close(m_connections[idx]))
      {
        LOG_PRINT_L0("Close connection error: " << m_connections[idx]);
        if (!ignore_close_fails)
        {
          return false;
        }
      }

      m_connections[idx] = boost::uuids::nil_uuid();
      m_opened_connection_count.fetch_sub(1, std::memory_order_relaxed);
      return true;
    }

    size_t opened_connection_count() const { return m_opened_connection_count.load(std::memory_order_relaxed); }

  private:
    test_tcp_server& m_tcp_server;
    size_t m_max_opened_connection_count;
    std::atomic<size_t> m_opened_connection_count;
    std::atomic<size_t> m_next_opened_conn_idx;
    std::atomic<size_t> m_next_closed_conn_idx;
    std::vector<boost::uuids::uuid> m_connections;
  };

  const unsigned int min_thread_count = 2;
  const std::string clt_port("36230");
  const std::string srv_port("36231");

  enum command_ids
  {
    cmd_close_all_connections_id = 73564,
    cmd_start_open_close_test_id,
    cmd_get_statistics_id,
    cmd_reset_statistics_id,
    cmd_shutdown_id,
    cmd_send_data_requests_id,
    cmd_data_request_id
  };

  struct CMD_CLOSE_ALL_CONNECTIONS
  {
    const static int ID = cmd_close_all_connections_id;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CMD_START_OPEN_CLOSE_TEST
  {
    const static int ID = cmd_start_open_close_test_id;

    struct request
    {
      uint64_t open_request_target;
      uint64_t max_opened_conn_count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(open_request_target)
        KV_SERIALIZE(max_opened_conn_count)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CMD_GET_STATISTICS
  {
    const static int ID = cmd_get_statistics_id;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t opened_connections_count;
      uint64_t new_connection_counter;
      uint64_t close_connection_counter;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(opened_connections_count)
        KV_SERIALIZE(new_connection_counter)
        KV_SERIALIZE(close_connection_counter)
      END_KV_SERIALIZE_MAP()

      std::string to_string() const
      {
        std::stringstream ss;
        ss << "opened_connections_count = " << opened_connections_count <<
          ", new_connection_counter = " << new_connection_counter <<
          ", close_connection_counter = " << close_connection_counter;
        return ss.str();
      }
    };
  };

  struct CMD_RESET_STATISTICS
  {
    const static int ID = cmd_reset_statistics_id;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CMD_SHUTDOWN
  {
    const static int ID = cmd_shutdown_id;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CMD_SEND_DATA_REQUESTS
  {
    const static int ID = cmd_send_data_requests_id;

    struct request
    {
      uint64_t request_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(request_size)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CMD_DATA_REQUEST
  {
    const static int ID = cmd_data_request_id;

    struct request
    {
      std::string data;
      uint64_t response_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data)
      END_KV_SERIALIZE_MAP()
    };
  };
}
