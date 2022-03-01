// Copyright (c) 2014-2022, The Monero Project
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

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "include_base_utils.h"
#include "misc_log_ex.h"
#include "storages/levin_abstract_invoke2.h"
#include "common/util.h"

#include "net_load_tests.h"

using namespace net_load_tests;

#define EXIT_ON_ERROR(cond) { if (!(cond)) { LOG_PRINT_L0("ERROR: " << #cond); exit(1); } else {} }

namespace
{
  struct srv_levin_commands_handler : public test_levin_commands_handler
  {
    srv_levin_commands_handler(test_tcp_server& tcp_server)
      : m_tcp_server(tcp_server)
      , m_open_close_test_conn_id(boost::uuids::nil_uuid())
    {
    }

    virtual void on_connection_new(test_connection_context& context)
    {
      test_levin_commands_handler::on_connection_new(context);
      context.m_closed = false;

      //std::this_thread::sleep_for(std::chrono::milliseconds(10));

      boost::unique_lock<boost::mutex> lock(m_open_close_test_mutex);
      if (!m_open_close_test_conn_id.is_nil())
      {
        EXIT_ON_ERROR(m_open_close_test_helper->handle_new_connection(context.m_connection_id, true));
      }
    }

    virtual void on_connection_close(test_connection_context& context)
    {
      test_levin_commands_handler::on_connection_close(context);

      boost::unique_lock<boost::mutex> lock(m_open_close_test_mutex);
      if (context.m_connection_id == m_open_close_test_conn_id)
      {
        LOG_PRINT_L0("Stop open/close test");
        m_open_close_test_conn_id = boost::uuids::nil_uuid();
        m_open_close_test_helper.reset(0);
      }
    }

    CHAIN_LEVIN_INVOKE_MAP2(test_connection_context);
    CHAIN_LEVIN_NOTIFY_MAP2(test_connection_context);

    BEGIN_INVOKE_MAP2(srv_levin_commands_handler)
      HANDLE_NOTIFY_T2(CMD_CLOSE_ALL_CONNECTIONS, &srv_levin_commands_handler::handle_close_all_connections)
      HANDLE_NOTIFY_T2(CMD_SHUTDOWN, &srv_levin_commands_handler::handle_shutdown)
      HANDLE_NOTIFY_T2(CMD_SEND_DATA_REQUESTS, &srv_levin_commands_handler::handle_send_data_requests)
      HANDLE_INVOKE_T2(CMD_GET_STATISTICS, &srv_levin_commands_handler::handle_get_statistics)
      HANDLE_INVOKE_T2(CMD_RESET_STATISTICS, &srv_levin_commands_handler::handle_reset_statistics)
      HANDLE_INVOKE_T2(CMD_START_OPEN_CLOSE_TEST, &srv_levin_commands_handler::handle_start_open_close_test)
    END_INVOKE_MAP2()

    int handle_close_all_connections(int command, const CMD_CLOSE_ALL_CONNECTIONS::request& req, test_connection_context& context)
    {
      close_connections(context.m_connection_id);
      return 1;
    }

    int handle_get_statistics(int command, const CMD_GET_STATISTICS::request&, CMD_GET_STATISTICS::response& rsp, test_connection_context& /*context*/)
    {
      rsp.opened_connections_count = m_tcp_server.get_config_object().get_connections_count();
      rsp.new_connection_counter = new_connection_counter();
      rsp.close_connection_counter = close_connection_counter();
      LOG_PRINT_L0("Statistics: " << rsp.to_string());
      return 1;
    }

    int handle_reset_statistics(int command, const CMD_RESET_STATISTICS::request&, CMD_RESET_STATISTICS::response& /*rsp*/, test_connection_context& /*context*/)
    {
      m_new_connection_counter.reset();
      m_new_connection_counter.inc();
      m_close_connection_counter.reset();
      return 1;
    }

    int handle_start_open_close_test(int command, const CMD_START_OPEN_CLOSE_TEST::request& req, CMD_START_OPEN_CLOSE_TEST::response&, test_connection_context& context)
    {
      boost::unique_lock<boost::mutex> lock(m_open_close_test_mutex);
      if (0 == m_open_close_test_helper.get())
      {
        LOG_PRINT_L0("Start open/close test (" << req.open_request_target << ", " << req.max_opened_conn_count << ")");

        m_open_close_test_conn_id = context.m_connection_id;
        m_open_close_test_helper.reset(new open_close_test_helper(m_tcp_server, req.open_request_target, req.max_opened_conn_count));
        return 1;
      }
      else
      {
        return -1;
      }
    }

    int handle_shutdown(int command, const CMD_SHUTDOWN::request& req, test_connection_context& /*context*/)
    {
      LOG_PRINT_L0("Got shutdown request. Shutting down...");
      m_tcp_server.send_stop_signal();
      return 1;
    }

    int handle_send_data_requests(int /*command*/, const CMD_SEND_DATA_REQUESTS::request& req, test_connection_context& context)
    {
      boost::uuids::uuid cmd_conn_id = context.m_connection_id;
      m_tcp_server.get_config_object().foreach_connection([&](test_connection_context& ctx) {
        if (ctx.m_connection_id != cmd_conn_id)
        {
          CMD_DATA_REQUEST::request req2;
          req2.data.resize(req.request_size);

          bool r = epee::net_utils::async_invoke_remote_command2<CMD_DATA_REQUEST::response>(ctx, CMD_DATA_REQUEST::ID, req2,
            m_tcp_server.get_config_object(), [=](int code, const CMD_DATA_REQUEST::response& rsp, const test_connection_context&) {
              if (code <= 0)
              {
                LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST. code = " << code);
              }
          });
          if (!r)
            LOG_PRINT_L0("Failed to invoke CMD_DATA_REQUEST");
        }
        return true;
      });

      return 1;
    }

  private:
    void close_connections(boost::uuids::uuid cmd_conn_id)
    {
      LOG_PRINT_L0("Closing connections. Number of opened connections: " << m_tcp_server.get_config_object().get_connections_count());

      size_t count = 0;
      m_tcp_server.get_config_object().foreach_connection([&](test_connection_context& ctx) {
        if (ctx.m_connection_id != cmd_conn_id)
        {
          ++count;
          if (!ctx.m_closed)
          {
            ctx.m_closed = true;
            m_tcp_server.get_config_object().close(ctx.m_connection_id);
          }
          else
          {
            LOG_PRINT_L0(count << " connection already closed");
          }
        }
        return true;
      });

      if (0 < count)
      {
        // Perhaps not all connections were closed, try to close it after 7 seconds
        boost::shared_ptr<boost::asio::deadline_timer> sh_deadline(new boost::asio::deadline_timer(m_tcp_server.get_io_service(), boost::posix_time::seconds(7)));
        sh_deadline->async_wait([=](const boost::system::error_code& ec)
        {
          boost::shared_ptr<boost::asio::deadline_timer> t = sh_deadline; // Capture sh_deadline
          if (!ec)
          {
            close_connections(cmd_conn_id);
          }
          else
          {
            LOG_PRINT_L0("ERROR: " << ec.message() << ':' << ec.value());
          }
        });
      }
    }

  private:
    test_tcp_server& m_tcp_server;

    boost::uuids::uuid m_open_close_test_conn_id;
    boost::mutex m_open_close_test_mutex;
    std::unique_ptr<open_close_test_helper> m_open_close_test_helper;
  };
}

int main(int argc, char** argv)
{
  TRY_ENTRY();
  tools::on_startup();
  //set up logging options
  mlog_configure(mlog_get_default_log_path("net_load_tests_srv.log"), true);

  size_t thread_count = (std::max)(min_thread_count, boost::thread::hardware_concurrency() / 2);

  test_tcp_server tcp_server(epee::net_utils::e_connection_type_RPC);
  if (!tcp_server.init_server(srv_port, "127.0.0.1"))
    return 1;

  srv_levin_commands_handler *commands_handler = new srv_levin_commands_handler(tcp_server);
  tcp_server.get_config_object().set_handler(commands_handler, [](epee::levin::levin_commands_handler<test_connection_context> *handler) { delete handler; });
  tcp_server.get_config_object().m_invoke_timeout = 10000;
  //tcp_server.get_config_object().m_max_packet_size = max_packet_size;

  if (!tcp_server.run_server(thread_count, true))
    return 2;
  return 0;
  CATCH_ENTRY_L0("main", 1);
}
