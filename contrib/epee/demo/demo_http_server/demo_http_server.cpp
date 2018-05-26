// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include "stdafx.h"
#include "console_handler.h"
#include "demo_http_server.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"


template<class t_request, class t_response>
bool communicate(const std::string url, t_request& req, t_response& rsp, const std::string& ip, const std::string& port, bool use_json, bool use_jrpc = false)
{
  epee::net_utils::http::http_simple_client http_client;
  bool r = http_client.connect(ip, port, 1000);
  CHECK_AND_ASSERT_MES(r, false, "failed to connect");
  if(use_json)
  {
    if(use_jrpc)
    {
      epee::json_rpc::request<t_request> req_t = AUTO_VAL_INIT(req_t);
      req_t.jsonrpc = "2.0";
      req_t.id = epee::serialization::storage_entry(10);
      req_t.method = "command_example_1";
      req_t.params = req;
      epee::json_rpc::response<t_response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
      if(!epee::net_utils::invoke_http_json_remote_command2("/request_json_rpc", req_t, resp_t, http_client))
      {
        return false;
      }
      rsp = resp_t.result;
      return true;
    }else
      return epee::net_utils::invoke_http_json_remote_command2(url, req, rsp, http_client);
  }
  else   
    return epee::net_utils::invoke_http_bin_remote_command2(url, req, rsp, http_client);
}


int main(int argc, char* argv[])
{
  TRY_ENTRY();
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::add_logger(LOGGER_FILE, 
    log_space::log_singletone::get_default_log_file().c_str(), 
    log_space::log_singletone::get_default_log_folder().c_str());



  LOG_PRINT("Demo server starting ...", LOG_LEVEL_0);


  demo::demo_http_server srv;

  start_default_console(&srv, "#");

  std::string bind_param = "0.0.0.0";
  std::string port = "83";

  if(!srv.init(port, bind_param))
  {
    LOG_ERROR("Failed to initialize srv!");
    return 1;
  }

  //log loop
  srv.run();
  size_t count = 0;
  while (!srv.is_stop())
  {
   
    demo::COMMAND_EXAMPLE_1::request req;
    req.sub = demo::get_test_data();
    demo::COMMAND_EXAMPLE_1::response rsp;
    bool r = false;
    if(count%2)
    {//invoke json 
      r = communicate("/request_api_json_1", req, rsp, "127.0.0.1", port, true, true);
    }else{
      r = communicate("/request_api_bin_1", req, rsp, "127.0.0.1", port, false);
    }
    CHECK_AND_ASSERT_MES(r, false, "failed to invoke http request");
    CHECK_AND_ASSERT_MES(rsp.m_success, false, "wrong response");
    CHECK_AND_ASSERT_MES(rsp.subs.size()==1, false, "wrong response");
    CHECK_AND_ASSERT_MES(rsp.subs.front() == demo::get_test_data(), false, "wrong response");
    //misc_utils::sleep_no_w(1000);
    ++count;
  }
  bool r = srv.wait_stop();
  CHECK_AND_ASSERT_MES(r, 1, "failed to wait server stop");
  srv.deinit();

  LOG_PRINT("Demo server stoped.", LOG_LEVEL_0);
  return 1;

  CATCH_ENTRY_L0("main", 1);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
namespace demo
{
  bool demo_http_server::init(const std::string& bind_port, const std::string& bind_ip)
  {


    //set self as callback handler
    m_net_server.get_config_object().m_phandler = this;

    //here set folder for hosting reqests
    m_net_server.get_config_object().m_folder = "";

    LOG_PRINT_L0("Binding on " << bind_ip << ":" << bind_port);
    return m_net_server.init_server(bind_port, bind_ip);
  }

  bool demo_http_server::run()
  {
    m_stop = false;
    //here you can set worker threads count
    int thrds_count = 4;

    //go to loop
    LOG_PRINT("Run net_service loop( " << thrds_count << " threads)...", LOG_LEVEL_0);
    if(!m_net_server.run_server(thrds_count, false))
    {
      LOG_ERROR("Failed to run net tcp server!");
    }

    return true;
  }

  bool demo_http_server::deinit()
  {
    return m_net_server.deinit_server();
  }

  bool demo_http_server::send_stop_signal()
  {
    m_stop = true;
    m_net_server.send_stop_signal();
    return true;
  }
 
  bool demo_http_server::on_requestr_uri_1(const net_utils::http::http_request_info& query_info, 
    net_utils::http::http_response_info& response,
    const net_utils::connection_context_base& m_conn_context)
  {
    return true;
  }


  bool demo_http_server::on_requestr_uri_2(const net_utils::http::http_request_info& query_info, 
    net_utils::http::http_response_info& response,
    const net_utils::connection_context_base& m_conn_context)
  {
    return true;
  }


  bool demo_http_server::on_hosting_request( const net_utils::http::http_request_info& query_info,
    net_utils::http::http_response_info& response,
    const net_utils::connection_context_base& m_conn_context)
  {
    //read file from filesystem here
    return true;
  }

  bool demo_http_server::on_request_api_1(const COMMAND_EXAMPLE_1::request& req, COMMAND_EXAMPLE_1::response& res, connection_context& ctxt)
  {
    CHECK_AND_ASSERT_MES(req.sub == demo::get_test_data(), false, "wrong request");
    res.m_success = true;
    res.subs.push_back(req.sub);
    return true;
  }

  bool demo_http_server::on_request_api_1_with_error(const COMMAND_EXAMPLE_1::request& req, COMMAND_EXAMPLE_1::response& res, epee::json_rpc::error& error_resp, connection_context& ctxt)
  {
    error_resp.code = 232432;
    error_resp.message = "bla bla bla";
    return false;
  }

  bool demo_http_server::on_request_api_2(const COMMAND_EXAMPLE_2::request& req, COMMAND_EXAMPLE_2::response& res, connection_context& ctxt)
  {
    return true;
  }
}
