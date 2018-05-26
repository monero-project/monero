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

#include "net/levin_server_cp2.h"
#include "transport_defs.h"
#include "storages/levin_abstract_invoke2.h"

using namespace epee;

namespace demo
{

  class demo_levin_server: public levin::levin_commands_handler<>
  {
  public:
    bool run();
    bool init(const std::string& bind_port = "11112", const std::string& bind_ip = "0.0.0.0");
    bool deinit();
    bool send_stop_signal();
    bool is_stop(){return m_stop;}
    bool wait_stop(){return m_net_server.timed_wait_server_stop(100000);}
    net_utils::boosted_levin_async_server& get_server(){return m_net_server;}
  private: 


    CHAIN_LEVIN_INVOKE_MAP(); //move levin_commands_handler interface invoke(...) callbacks into invoke map
    CHAIN_LEVIN_NOTIFY_STUB(); //move levin_commands_handler interface notify(...) callbacks into nothing

    BEGIN_INVOKE_MAP2(demo_levin_server)
      HANDLE_INVOKE_T2(COMMAND_EXAMPLE_1, &demo_levin_server::handle_command_1)
      HANDLE_INVOKE_T2(COMMAND_EXAMPLE_2, &demo_levin_server::handle_command_2)
      HANDLE_NOTIFY_T2(COMMAND_EXAMPLE_1, &demo_levin_server::handle_notify_1)
      HANDLE_NOTIFY_T2(COMMAND_EXAMPLE_2, &demo_levin_server::handle_notify_2)
    END_INVOKE_MAP2()

    //----------------- commands handlers ----------------------------------------------
    int handle_command_1(int command, COMMAND_EXAMPLE_1::request& arg, COMMAND_EXAMPLE_1::response& rsp, const net_utils::connection_context_base& context);
    int handle_command_2(int command, COMMAND_EXAMPLE_2::request& arg, COMMAND_EXAMPLE_2::response& rsp, const net_utils::connection_context_base& context);
    int handle_notify_1(int command, COMMAND_EXAMPLE_1::request& arg, const net_utils::connection_context_base& context);
    int handle_notify_2(int command, COMMAND_EXAMPLE_2::request& arg, const net_utils::connection_context_base& context);
    //----------------------------------------------------------------------------------
    net_utils::boosted_levin_async_server m_net_server;
    std::atomic<bool> m_stop;

  };
}

