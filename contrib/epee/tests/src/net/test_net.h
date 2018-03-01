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
#include "storages/abstract_invoke.h"

namespace epee
{
namespace StorageNamed
{
  typedef CInMemStorage DefaultStorageType;
}
namespace tests
{
  struct some_subdata
  {		

    std::string str1;
    std::list<uint64_t> array_of_id;

    BEGIN_NAMED_SERIALIZE_MAP()
      SERIALIZE_STL_ANSI_STRING(str1)
      SERIALIZE_STL_CONTAINER_POD(array_of_id)
    END_NAMED_SERIALIZE_MAP()
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_EXAMPLE_1
  {
    const static int ID = 1000;

    struct request
    {		

      std::string example_string_data;
      uint64_t example_id_data;
      some_subdata sub;

      BEGIN_NAMED_SERIALIZE_MAP()
        SERIALIZE_STL_ANSI_STRING(example_string_data)
        SERIALIZE_POD(example_id_data)
        SERIALIZE_T(sub)
      END_NAMED_SERIALIZE_MAP()
    };


    struct response
    {
      bool 	 m_success; 
      uint64_t example_id_data;
      std::list<some_subdata> subs;

      BEGIN_NAMED_SERIALIZE_MAP()
        SERIALIZE_POD(m_success)
        SERIALIZE_POD(example_id_data)
        SERIALIZE_STL_CONTAINER_T(subs)
      END_NAMED_SERIALIZE_MAP()
    };
  };

  struct COMMAND_EXAMPLE_2
  {
    const static int ID = 1001;

    struct request
    {		
      std::string example_string_data2;
      uint64_t example_id_data;

      BEGIN_NAMED_SERIALIZE_MAP()
        SERIALIZE_POD(example_id_data)
        SERIALIZE_STL_ANSI_STRING(example_string_data2)
      END_NAMED_SERIALIZE_MAP()
    };

    struct response
    {
      bool m_success; 
      uint64_t example_id_data;

      BEGIN_NAMED_SERIALIZE_MAP()
        SERIALIZE_POD(example_id_data)
        SERIALIZE_POD(m_success)
      END_NAMED_SERIALIZE_MAP()
    };
  };
  typedef boost::uuids::uuid uuid;

  class test_levin_server: public levin::levin_commands_handler<>
  {
    test_levin_server(const test_levin_server&){}
  public:
    test_levin_server(){}
    void set_thread_prefix(const std::string& pref)
    {
      m_net_server.set_threads_prefix(pref);
    }
    template<class calback_t>
    bool connect_async(const std::string adr, const std::string& port, uint32_t conn_timeot, calback_t cb, const std::string& bind_ip = "0.0.0.0")
    {
      return m_net_server.connect_async(adr, port, conn_timeot, cb, bind_ip);
    }

    bool connect(const std::string adr, const std::string& port, uint32_t conn_timeot, net_utils::connection_context_base& cn, const std::string& bind_ip = "0.0.0.0")
    {
      return m_net_server.connect(adr, port, conn_timeot, cn, bind_ip);
    }
    void close(net_utils::connection_context_base& cn)
    {
      m_net_server.get_config_object().close(cn.m_connection_id);
    }

    template<class t_request, class t_response>
    bool invoke(uuid con_id, int command, t_request& req, t_response& resp)
    {
      return invoke_remote_command(con_id, command, req, resp, m_net_server.get_config_object());
    }

    template< class t_response, class t_request, class callback_t>
    bool invoke_async(uuid con_id, int command, t_request& req, callback_t cb)
    {
      return async_invoke_remote_command<t_response>(con_id, command, req, m_net_server.get_config_object(), cb);
    }

    bool init(const std::string& bind_port = "", const std::string& bind_ip = "0.0.0.0")
    {
      m_net_server.get_config_object().set_handler(this);
      m_net_server.get_config_object().m_invoke_timeout = 1000;
      LOG_PRINT_L0("Binding on " << bind_ip << ":" << bind_port);
      return m_net_server.init_server(bind_port, bind_ip);
    }

    bool run()
    {
      //here you can set worker threads count
      int thrds_count = 4;

      //go to loop
      LOG_PRINT("Run net_service loop( " << thrds_count << " threads)...", LOG_LEVEL_0);
      if(!m_net_server.run_server(thrds_count))
      {
        LOG_ERROR("Failed to run net tcp server!");
      }

      LOG_PRINT("net_service loop stopped.", LOG_LEVEL_0);
      return true;
    }

    bool deinit()
    {
      return m_net_server.deinit_server();
    }

    bool send_stop_signal()
    {
      m_net_server.send_stop_signal();
      return true;
    }

    uint32_t get_binded_port()
    {
      return m_net_server.get_binded_port();
    }
  private: 


    CHAIN_LEVIN_INVOKE_TO_MAP(); //move levin_commands_handler interface invoke(...) callbacks into invoke map
    CHAIN_LEVIN_NOTIFY_TO_STUB(); //move levin_commands_handler interface notify(...) callbacks into nothing

    BEGIN_INVOKE_MAP(test_levin_server)
      HANDLE_INVOKE_T(COMMAND_EXAMPLE_1, &test_levin_server::handle_1)
      HANDLE_INVOKE_T(COMMAND_EXAMPLE_2, &test_levin_server::handle_2)
    END_INVOKE_MAP()

    //----------------- commands handlers ----------------------------------------------
    int handle_1(int command, COMMAND_EXAMPLE_1::request& arg, COMMAND_EXAMPLE_1::response& rsp, const net_utils::connection_context_base& context)
    {
      LOG_PRINT_L0("on_command_1: id " << arg.example_id_data << "---->>");      
      COMMAND_EXAMPLE_2::request arg_ = AUTO_VAL_INIT(arg_);
      arg_.example_id_data = arg.example_id_data;
      COMMAND_EXAMPLE_2::response rsp_ = AUTO_VAL_INIT(rsp_);
      invoke_async<COMMAND_EXAMPLE_2::response>(context.m_connection_id, COMMAND_EXAMPLE_2::ID, arg_, [](int code, const COMMAND_EXAMPLE_2::response& rsp, const net_utils::connection_context_base& context)
        {
          if(code < 0)
          {LOG_PRINT_RED_L0("on_command_1: command_2 failed to invoke");}
          else
          {LOG_PRINT_L0("on_command_1: command_2 response " << rsp.example_id_data);}
        });
      rsp.example_id_data = arg.example_id_data;
      LOG_PRINT_L0("on_command_1: id " << arg.example_id_data << "<<----");      
      return true;
    }
    int handle_2(int command, COMMAND_EXAMPLE_2::request& arg, COMMAND_EXAMPLE_2::response& rsp, const net_utils::connection_context_base& context)
    {
      LOG_PRINT_L0("on_command_2: id "<< arg.example_id_data);     
      rsp.example_id_data = arg.example_id_data;
      //misc_utils::sleep_no_w(6000);
      return true;
    }
    //----------------------------------------------------------------------------------
    net_utils::boosted_levin_async_server m_net_server;
  };


  inline 
    bool do_run_test_server()
  {

    test_levin_server srv1, srv2;


    std::string bind_param = "0.0.0.0";
    std::string port = "";

    if(!srv1.init(port, bind_param))
    {
      LOG_ERROR("Failed to initialize srv!");
      return 1;
    }

    if(!srv2.init(port, bind_param))
    {
      LOG_ERROR("Failed to initialize srv!");
      return 1;
    }

    srv1.set_thread_prefix("SRV_A");
    srv2.set_thread_prefix("SRV_B");

    boost::thread th1( boost::bind(&test_levin_server::run, &srv1));
    boost::thread th2( boost::bind(&test_levin_server::run, &srv2));

    LOG_PRINT_L0("Initialized servers, waiting for worker threads started...");
    misc_utils::sleep_no_w(1000);  


    LOG_PRINT_L0("Connecting to each other...");
    uint32_t port1 = srv1.get_binded_port();
    uint32_t port2 = srv2.get_binded_port();

    COMMAND_EXAMPLE_1::request arg;
    COMMAND_EXAMPLE_1::request resp;

    net_utils::connection_context_base cntxt_1;
    bool r = srv1.connect("127.0.0.1", string_tools::num_to_string_fast(port2), 5000, cntxt_1);
    CHECK_AND_ASSERT_MES(r, false, "connect to server failed");

    net_utils::connection_context_base cntxt_2;
    r = srv2.connect("127.0.0.1", string_tools::num_to_string_fast(port1), 5000, cntxt_2);
    CHECK_AND_ASSERT_MES(r, false, "connect to server failed");

    while(true)
    {
      LOG_PRINT_L0("Invoking from A to B...");
      int r = srv1.invoke(cntxt_1.m_connection_id, COMMAND_EXAMPLE_1::ID, arg, resp);
      if(r<=0)
      {
        LOG_ERROR("Failed tp invoke A to B");
        break;
      }

      LOG_PRINT_L0("Invoking from B to A...");
      r = srv2.invoke(cntxt_2.m_connection_id, COMMAND_EXAMPLE_1::ID, arg, resp);
      if(r<=0)
      {
        LOG_ERROR("Failed tp invoke B to A");
        break;
      }
    }
    srv1.send_stop_signal();
    srv2.send_stop_signal();
    th1.join();
    th1.join();

    return true;
  }



  inline bool do_test2_work_with_srv(test_levin_server& srv, int port)
  {
    uint64_t i = 0;
    boost::mutex wait_event;
    wait_event.lock();
    while(true)
    {
      net_utils::connection_context_base cntxt_local = AUTO_VAL_INIT(cntxt_local);
      bool r = srv.connect_async("127.0.0.1", string_tools::num_to_string_fast(port), 5000, [&srv, &port, &wait_event, &i, &cntxt_local](const net_utils::connection_context_base& cntxt, const boost::system::error_code& ec)
      {
        CHECK_AND_ASSERT_MES(!ec, void(), "Some problems at connect, message: " << ec.message() );
        cntxt_local = cntxt;
        LOG_PRINT_L0("Invoking command 1 to " << port);
        COMMAND_EXAMPLE_1::request arg = AUTO_VAL_INIT(arg);
        arg.example_id_data = i;
        /*vc2010 workaround*/
        int port_ = port;
        boost::mutex& wait_event_ = wait_event;
        int r = srv.invoke_async<COMMAND_EXAMPLE_1::request>(cntxt.m_connection_id, COMMAND_EXAMPLE_1::ID, arg, [port_, &wait_event_](int code, const COMMAND_EXAMPLE_1::request& rsp, const net_utils::connection_context_base& cntxt)
        {
            CHECK_AND_ASSERT_MES(code > 0, void(), "Failed to invoke"); 
            LOG_PRINT_L0("command 1 invoke to " << port_ << " OK.");
            wait_event_.unlock();
        });        
      });
      wait_event.lock();
      srv.close(cntxt_local);
      ++i;
    }
    return true;
  }

  inline 
    bool do_run_test_server_async_connect()
  {
    test_levin_server srv1, srv2;


    std::string bind_param = "0.0.0.0";
    std::string port = "";

    if(!srv1.init(port, bind_param))
    {
      LOG_ERROR("Failed to initialize srv!");
      return 1;
    }

    if(!srv2.init(port, bind_param))
    {
      LOG_ERROR("Failed to initialize srv!");
      return 1;
    }

    srv1.set_thread_prefix("SRV_A");
    srv2.set_thread_prefix("SRV_B");

    boost::thread thmain1( boost::bind(&test_levin_server::run, &srv1));
    boost::thread thmain2( boost::bind(&test_levin_server::run, &srv2));

    LOG_PRINT_L0("Initalized servers, waiting for worker threads started...");
    misc_utils::sleep_no_w(1000);  


    LOG_PRINT_L0("Connecting to each other...");
    uint32_t port1 = srv1.get_binded_port();
    uint32_t port2 = srv2.get_binded_port();

    COMMAND_EXAMPLE_1::request arg;
    COMMAND_EXAMPLE_1::request resp;


    boost::thread work_1( boost::bind(do_test2_work_with_srv, boost::ref(srv1), port2));
    boost::thread work_2( boost::bind(do_test2_work_with_srv, boost::ref(srv2), port1));
    boost::thread work_3( boost::bind(do_test2_work_with_srv, boost::ref(srv1), port2));
    boost::thread work_4( boost::bind(do_test2_work_with_srv, boost::ref(srv2), port1));
    boost::thread work_5( boost::bind(do_test2_work_with_srv, boost::ref(srv1), port2));
    boost::thread work_6( boost::bind(do_test2_work_with_srv, boost::ref(srv2), port1));
    boost::thread work_7( boost::bind(do_test2_work_with_srv, boost::ref(srv1), port2));
    boost::thread work_8( boost::bind(do_test2_work_with_srv, boost::ref(srv2), port1));


    work_1.join();
    work_2.join();
    srv1.send_stop_signal();
    srv2.send_stop_signal();
    thmain1.join();
    thmain2.join();

    return true;
  }

}
}
