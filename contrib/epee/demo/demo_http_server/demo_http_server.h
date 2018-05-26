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



#pragma once
#include <boost/thread.hpp>
#include <boost/bind.hpp> 

#include "net/http_server_cp2.h"
#include "transport_defs.h"
#include "net/http_server_handlers_map2.h"

using namespace epee;

namespace demo
{

  class demo_http_server: public net_utils::http::i_http_server_handler<epee::net_utils::connection_context_base>
  {
  public:
	  typedef epee::net_utils::connection_context_base connection_context;

    demo_http_server():m_stop(false){}
    bool run();
    bool init(const std::string& bind_port = "11112", const std::string& bind_ip = "0.0.0.0");
    bool deinit();
    bool send_stop_signal();
    bool is_stop(){return m_stop;}
    bool wait_stop(){return m_net_server.timed_wait_server_stop(100000);}
  private: 


    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      MAP_URI2("/requestr_uri_1", on_requestr_uri_1)
      MAP_URI2("/requestr_uri_2", on_requestr_uri_1)
      //MAP_URI_AUTO_XML2("/request_api_xml_1", on_request_api_1, COMMAND_EXAMPLE_1)
      //MAP_URI_AUTO_XML2("/request_api_xml_2", on_request_api_2, COMMAND_EXAMPLE_2)
      MAP_URI_AUTO_JON2("/request_api_json_1", on_request_api_1, COMMAND_EXAMPLE_1)
      MAP_URI_AUTO_JON2("/request_api_json_2", on_request_api_2, COMMAND_EXAMPLE_2)
      MAP_URI_AUTO_BIN2("/request_api_bin_1", on_request_api_1, COMMAND_EXAMPLE_1)
      MAP_URI_AUTO_BIN2("/request_api_bin_2", on_request_api_2, COMMAND_EXAMPLE_2)      
      BEGIN_JSON_RPC_MAP("/request_json_rpc")
        MAP_JON_RPC("command_example_1", on_request_api_1, COMMAND_EXAMPLE_1)
        MAP_JON_RPC("command_example_2", on_request_api_2, COMMAND_EXAMPLE_2)
        MAP_JON_RPC_WE("command_example_1_we", on_request_api_1_with_error, COMMAND_EXAMPLE_1)
      END_JSON_RPC_MAP()
      CHAIN_URI_MAP2(on_hosting_request)
    END_URI_MAP2()



    bool on_requestr_uri_1(const net_utils::http::http_request_info& query_info, 
                             net_utils::http::http_response_info& response,
                             const net_utils::connection_context_base& m_conn_context);


    bool on_requestr_uri_2(const net_utils::http::http_request_info& query_info, 
                             net_utils::http::http_response_info& response,
                             const net_utils::connection_context_base& m_conn_context);




    bool on_hosting_request( const net_utils::http::http_request_info& query_info,
                             net_utils::http::http_response_info& response,
                             const net_utils::connection_context_base& m_conn_context);

    bool on_request_api_1(const COMMAND_EXAMPLE_1::request& req, COMMAND_EXAMPLE_1::response& res, connection_context& ctxt);
    bool on_request_api_2(const COMMAND_EXAMPLE_2::request& req, COMMAND_EXAMPLE_2::response& res, connection_context& ctxt);

    bool on_request_api_1_with_error(const COMMAND_EXAMPLE_1::request& req, COMMAND_EXAMPLE_1::response& res, epee::json_rpc::error& error_resp, connection_context& ctxt);

    net_utils::boosted_http_server_custum_handling m_net_server;
    std::atomic<bool> m_stop;
  };
}

