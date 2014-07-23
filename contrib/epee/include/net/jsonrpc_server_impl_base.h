// Copyright (c) 2014, The Monero Project
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

#ifndef JSONRPC_SERVER_IMPL_BASE_H
#define	JSONRPC_SERVER_IMPL_BASE_H

#include <boost/thread.hpp>
#include <boost/bind.hpp> 

#include "net/jsonrpc_protocol_handler.h"
#include "net/jsonrpc_server_handlers_map.h"
#include "net/abstract_tcp_server2.h"

namespace epee
{

template<class t_child_class, class t_connection_context = epee::net_utils::connection_context_base>
  class jsonrpc_server_impl_base: public net_utils::jsonrpc2::i_jsonrpc2_server_handler<t_connection_context>
  {

  public:
    jsonrpc_server_impl_base()
        : m_net_server()
    {}

    explicit jsonrpc_server_impl_base(boost::asio::io_service& external_io_service)
        : m_net_server(external_io_service)
    {}

    bool init(const std::string& bind_port = "0", const std::string& bind_ip = "0.0.0.0")
    {
      //set self as callback handler
      m_net_server.get_config_object().m_phandler = static_cast<t_child_class*>(this);

      LOG_PRINT_L0("Binding on " << bind_ip << ":" << bind_port);
      bool res = m_net_server.init_server(bind_port, bind_ip);
      if (!res)
      {
        LOG_ERROR("Failed to bind server");
        return false;
      }
      return true;
    }

    bool run(size_t threads_count, bool wait = true)
    {
      //go to loop
      LOG_PRINT("Run net_service loop( " << threads_count << " threads)...", LOG_LEVEL_0);
      if(!m_net_server.run_server(threads_count, wait))
      {
        LOG_ERROR("Failed to run net tcp server!");
      }

      if(wait)
        LOG_PRINT("net_service loop stopped.", LOG_LEVEL_0);
      return true;
    }

    bool deinit()
    {
      return m_net_server.deinit_server();
    }

    bool timed_wait_server_stop(uint64_t ms)
    {
      return m_net_server.timed_wait_server_stop(ms);
    }

    bool send_stop_signal()
    {
      m_net_server.send_stop_signal();
      return true;
    }

    int get_binded_port()
    {
      return m_net_server.get_binded_port();
    }

  protected: 
    net_utils::boosted_tcp_server<net_utils::jsonrpc2::jsonrpc2_connection_handler<t_connection_context> > m_net_server;
  };

}

#endif	/* JSONRPC_SERVER_IMPL_BASE_H */

