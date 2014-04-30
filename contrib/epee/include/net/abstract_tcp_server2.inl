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



#include "net_utils_base.h"
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/chrono.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "misc_language.h"
#include "pragma_comp_defs.h"

PRAGMA_WARNING_PUSH
namespace epee
{
namespace net_utils
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
PRAGMA_WARNING_DISABLE_VS(4355)

  template<class t_protocol_handler>
  connection<t_protocol_handler>::connection(boost::asio::io_service& io_service,
    typename t_protocol_handler::config_type& config, volatile uint32_t& sock_count, i_connection_filter* &pfilter)
                          : strand_(io_service),
                            socket_(io_service),
                            m_protocol_handler(this, config, context), 
                            m_want_close_connection(0), 
                            m_was_shutdown(0), 
                            m_ref_sockets_count(sock_count), 
                            m_pfilter(pfilter)
  {
    boost::interprocess::ipcdetail::atomic_inc32(&m_ref_sockets_count);
  }
PRAGMA_WARNING_DISABLE_VS(4355)
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  connection<t_protocol_handler>::~connection()
  {
    if(!m_was_shutdown)
    {
      LOG_PRINT_L3("[sock " << socket_.native_handle() << "] Socket destroyed without shutdown.");
      shutdown();
    }

    LOG_PRINT_L3("[sock " << socket_.native_handle() << "] Socket destroyed");
    boost::interprocess::ipcdetail::atomic_dec32(&m_ref_sockets_count);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  boost::asio::ip::tcp::socket& connection<t_protocol_handler>::socket()
  {
    return socket_;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  boost::shared_ptr<connection<t_protocol_handler> > connection<t_protocol_handler>::safe_shared_from_this()
  {
    try
    {
      return connection<t_protocol_handler>::shared_from_this();
    }
    catch (const boost::bad_weak_ptr&)
    {
      // It happens when the connection is being deleted
      return boost::shared_ptr<connection<t_protocol_handler> >();
    }
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::start(bool is_income, bool is_multithreaded)
  {
    TRY_ENTRY();

    // Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
    auto self = safe_shared_from_this();
    if(!self)
      return false;

    m_is_multithreaded = is_multithreaded;

    boost::system::error_code ec;
    auto remote_ep = socket_.remote_endpoint(ec);
    CHECK_AND_NO_ASSERT_MES(!ec, false, "Failed to get remote endpoint: " << ec.message() << ':' << ec.value());

    auto local_ep = socket_.local_endpoint(ec);
    CHECK_AND_NO_ASSERT_MES(!ec, false, "Failed to get local endpoint: " << ec.message() << ':' << ec.value());

    context = boost::value_initialized<t_connection_context>();
    long ip_ = boost::asio::detail::socket_ops::host_to_network_long(remote_ep.address().to_v4().to_ulong());

    context.set_details(boost::uuids::random_generator()(), ip_, remote_ep.port(), is_income);
    LOG_PRINT_L3("[sock " << socket_.native_handle() << "] new connection from " << print_connection_context_short(context) <<
      " to " << local_ep.address().to_string() << ':' << local_ep.port() <<
      ", total sockets objects " << m_ref_sockets_count);

    if(m_pfilter && !m_pfilter->is_remote_ip_allowed(context.m_remote_ip))
    {
      LOG_PRINT_L2("[sock " << socket_.native_handle() << "] ip denied " << string_tools::get_ip_string_from_int32(context.m_remote_ip) << ", shutdowning connection");
      close();
      return false;
    }

    m_protocol_handler.after_init_connection();

    socket_.async_read_some(boost::asio::buffer(buffer_),
      strand_.wrap(
        boost::bind(&connection<t_protocol_handler>::handle_read, self,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred)));

    return true;

    CATCH_ENTRY_L0("connection<t_protocol_handler>::start()", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::request_callback()
  {
    TRY_ENTRY();
    LOG_PRINT_L2("[" << print_connection_context_short(context) << "] request_callback");
    // Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
    auto self = safe_shared_from_this();
    if(!self)
      return false;

    strand_.post(boost::bind(&connection<t_protocol_handler>::call_back_starter, self));
    CATCH_ENTRY_L0("connection<t_protocol_handler>::request_callback()", false);
    return true;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  boost::asio::io_service& connection<t_protocol_handler>::get_io_service()
  {
    return socket_.get_io_service();
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::add_ref()
  {
    TRY_ENTRY();
    LOG_PRINT_L4("[sock " << socket_.native_handle() << "] add_ref");
    CRITICAL_REGION_LOCAL(m_self_refs_lock);

    // Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
    auto self = safe_shared_from_this();
    if(!self)
      return false;
    if(m_was_shutdown)
      return false;
    m_self_refs.push_back(self);
    return true;
    CATCH_ENTRY_L0("connection<t_protocol_handler>::add_ref()", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::release()
  {
    TRY_ENTRY();
    boost::shared_ptr<connection<t_protocol_handler> >  back_connection_copy;
    LOG_PRINT_L4("[sock " << socket_.native_handle() << "] release");
    CRITICAL_REGION_BEGIN(m_self_refs_lock);
    CHECK_AND_ASSERT_MES(m_self_refs.size(), false, "[sock " << socket_.native_handle() << "] m_self_refs empty at connection<t_protocol_handler>::release() call");
    //erasing from container without additional copy can cause start deleting object, including m_self_refs
    back_connection_copy = m_self_refs.back();
    m_self_refs.pop_back();
    CRITICAL_REGION_END();
    return true;
    CATCH_ENTRY_L0("connection<t_protocol_handler>::release()", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void connection<t_protocol_handler>::call_back_starter()
  {
    TRY_ENTRY();
    LOG_PRINT_L2("[" << print_connection_context_short(context) << "] fired_callback");
    m_protocol_handler.handle_qued_callback();
    CATCH_ENTRY_L0("connection<t_protocol_handler>::call_back_starter()", void());
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void connection<t_protocol_handler>::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
  {
    TRY_ENTRY();
    LOG_PRINT_L4("[sock " << socket_.native_handle() << "] Async read calledback.");
    
    if (!e)
    {
      LOG_PRINT("[sock " << socket_.native_handle() << "] RECV " << bytes_transferred, LOG_LEVEL_4);
      context.m_last_recv = time(NULL);
      context.m_recv_cnt += bytes_transferred;
      bool recv_res = m_protocol_handler.handle_recv(buffer_.data(), bytes_transferred);
      if(!recv_res)
      {  
        LOG_PRINT("[sock " << socket_.native_handle() << "] protocol_want_close", LOG_LEVEL_4);

        //some error in protocol, protocol handler ask to close connection
        boost::interprocess::ipcdetail::atomic_write32(&m_want_close_connection, 1);
        bool do_shutdown = false;
        CRITICAL_REGION_BEGIN(m_send_que_lock);
        if(!m_send_que.size())
          do_shutdown = true;
        CRITICAL_REGION_END();
        if(do_shutdown)
          shutdown();
      }else
      {
        socket_.async_read_some(boost::asio::buffer(buffer_),
          strand_.wrap(
            boost::bind(&connection<t_protocol_handler>::handle_read, connection<t_protocol_handler>::shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred)));
        LOG_PRINT_L4("[sock " << socket_.native_handle() << "]Async read requested.");
      }
    }else
    {
      LOG_PRINT_L3("[sock " << socket_.native_handle() << "] Some not success at read: " << e.message() << ':' << e.value());
      if(e.value() != 2)
      {
        LOG_PRINT_L3("[sock " << socket_.native_handle() << "] Some problems at read: " << e.message() << ':' << e.value());
        shutdown();
      }
    }
    // If an error occurs then no new asynchronous operations are started. This
    // means that all shared_ptr references to the connection object will
    // disappear and the object will be destroyed automatically after this
    // handler returns. The connection class's destructor closes the socket.
    CATCH_ENTRY_L0("connection<t_protocol_handler>::handle_read", void());
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::call_run_once_service_io()
  {
    TRY_ENTRY();
    if(!m_is_multithreaded)
    {
      //single thread model, we can wait in blocked call
      size_t cnt = socket_.get_io_service().run_one();
      if(!cnt)//service is going to quit
        return false;
    }else
    {
      //multi thread model, we can't(!) wait in blocked call
      //so we make non blocking call and releasing CPU by calling sleep(0); 
      //if no handlers were called
      //TODO: Maybe we need to have have critical section + event + callback to upper protocol to
      //ask it inside(!) critical region if we still able to go in event wait...
      size_t cnt = socket_.get_io_service().poll_one();     
      if(!cnt)
        misc_utils::sleep_no_w(0);
    }
    
    return true;
    CATCH_ENTRY_L0("connection<t_protocol_handler>::call_run_once_service_io", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::do_send(const void* ptr, size_t cb)
  {
    TRY_ENTRY();
    // Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
    auto self = safe_shared_from_this();
    if(!self)
      return false;
    if(m_was_shutdown)
      return false;

    LOG_PRINT("[sock " << socket_.native_handle() << "] SEND " << cb, LOG_LEVEL_4);
    context.m_last_send = time(NULL);
    context.m_send_cnt += cb;
    //some data should be wrote to stream
    //request complete
    
    epee::critical_region_t<decltype(m_send_que_lock)> send_guard(m_send_que_lock);
    if(m_send_que.size() > ABSTRACT_SERVER_SEND_QUE_MAX_COUNT)
    {
      send_guard.unlock();
      LOG_ERROR("send que size is more than ABSTRACT_SERVER_SEND_QUE_MAX_COUNT(" << ABSTRACT_SERVER_SEND_QUE_MAX_COUNT << "), shutting down connection");
      close();
      return false;
    }

    m_send_que.resize(m_send_que.size()+1);
    m_send_que.back().assign((const char*)ptr, cb);
    
    if(m_send_que.size() > 1)
    {
      //active operation should be in progress, nothing to do, just wait last operation callback
    }else
    {
      //no active operation
      if(m_send_que.size()!=1)
      {
        LOG_ERROR("Looks like no active operations, but send que size != 1!!");
        return false;
      }

      boost::asio::async_write(socket_, boost::asio::buffer(m_send_que.front().data(), m_send_que.front().size()),
        //strand_.wrap(
        boost::bind(&connection<t_protocol_handler>::handle_write, self, _1, _2)
        //)
        );
      
      LOG_PRINT_L4("[sock " << socket_.native_handle() << "] Async send requested " << m_send_que.front().size());
    }

    return true;

    CATCH_ENTRY_L0("connection<t_protocol_handler>::do_send", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::shutdown()
  {
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    m_was_shutdown = true;
    m_protocol_handler.release_protocol();
    return true;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool connection<t_protocol_handler>::close()
  {
    TRY_ENTRY();
    LOG_PRINT_L4("[sock " << socket_.native_handle() << "] Que Shutdown called.");
    size_t send_que_size = 0;
    CRITICAL_REGION_BEGIN(m_send_que_lock);
    send_que_size = m_send_que.size();
    CRITICAL_REGION_END();
    boost::interprocess::ipcdetail::atomic_write32(&m_want_close_connection, 1);
    if(!send_que_size)
    {
      shutdown();
    }
    
    return true;
    CATCH_ENTRY_L0("connection<t_protocol_handler>::close", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void connection<t_protocol_handler>::handle_write(const boost::system::error_code& e, size_t cb)
  {
    TRY_ENTRY();
    LOG_PRINT_L4("[sock " << socket_.native_handle() << "] Async send calledback " << cb);

    if (e)
    {
      LOG_PRINT_L0("[sock " << socket_.native_handle() << "] Some problems at write: " << e.message() << ':' << e.value());
      shutdown();
      return;
    }

    bool do_shutdown = false;
    CRITICAL_REGION_BEGIN(m_send_que_lock);
    if(m_send_que.empty())
    {
      LOG_ERROR("[sock " << socket_.native_handle() << "] m_send_que.size() == 0 at handle_write!");
      return;
    }

    m_send_que.pop_front();
    if(m_send_que.empty())
    {
      if(boost::interprocess::ipcdetail::atomic_read32(&m_want_close_connection))
      {
        do_shutdown = true;
      }
    }else
    {
      //have more data to send
      boost::asio::async_write(socket_, boost::asio::buffer(m_send_que.front().data(), m_send_que.front().size()),
        //strand_.wrap(
          boost::bind(&connection<t_protocol_handler>::handle_write, connection<t_protocol_handler>::shared_from_this(), _1, _2));
        //);
    }
    CRITICAL_REGION_END();

    if(do_shutdown)
    {
      shutdown();
    }
    CATCH_ENTRY_L0("connection<t_protocol_handler>::handle_write", void());
  }
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::boosted_tcp_server():
    m_io_service_local_instance(new boost::asio::io_service()),
    io_service_(*m_io_service_local_instance.get()),
    acceptor_(io_service_),
    new_connection_(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter)), 
    m_stop_signal_sent(false), m_port(0), m_sockets_count(0), m_threads_count(0), m_pfilter(NULL), m_thread_index(0)
  {
    m_thread_name_prefix = "NET";
  }

  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::boosted_tcp_server(boost::asio::io_service& extarnal_io_service):
    io_service_(extarnal_io_service),
    acceptor_(io_service_),
    new_connection_(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter)), 
    m_stop_signal_sent(false), m_port(0), m_sockets_count(0), m_threads_count(0), m_pfilter(NULL), m_thread_index(0)
  {
    m_thread_name_prefix = "NET";
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::~boosted_tcp_server()
  {
    this->send_stop_signal();
    timed_wait_server_stop(10000);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::init_server(uint32_t port, const std::string address)
  {
    TRY_ENTRY();
    m_stop_signal_sent = false;
    m_port = port;
    m_address = address;
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    boost::asio::ip::tcp::endpoint binded_endpoint = acceptor_.local_endpoint();
    m_port = binded_endpoint.port();
    acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept, this,
      boost::asio::placeholders::error));

    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::init_server", false);
  }
  //-----------------------------------------------------------------------------
PUSH_WARNINGS
DISABLE_GCC_WARNING(maybe-uninitialized)
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::init_server(const std::string port, const std::string& address) 
  {
    uint32_t p = 0;

    if (port.size() && !string_tools::get_xtype_from_string(p, port)) {
      LOG_ERROR("Failed to convert port no = " << port);
      return false;
    }
    return this->init_server(p, address);
  }
POP_WARNINGS
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::worker_thread()
  {
    TRY_ENTRY();
    uint32_t local_thr_index = boost::interprocess::ipcdetail::atomic_inc32(&m_thread_index); 
    std::string thread_name = std::string("[") + m_thread_name_prefix;
    thread_name += boost::to_string(local_thr_index) + "]";
    log_space::log_singletone::set_thread_log_prefix(thread_name);
    while(!m_stop_signal_sent)
    {
      try
      {
        io_service_.run();
      }
      catch(const std::exception& ex)
      {
        LOG_ERROR("Exception at server worker thread, what=" << ex.what());
      }
      catch(...)
      {
        LOG_ERROR("Exception at server worker thread, unknown execption");
      }
    }
    LOG_PRINT_L4("Worker thread finished");
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::worker_thread", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::set_threads_prefix(const std::string& prefix_name)
  {
    m_thread_name_prefix = prefix_name;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::set_connection_filter(i_connection_filter* pfilter)
  {
    m_pfilter = pfilter;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::run_server(size_t threads_count, bool wait, const boost::thread::attributes& attrs)
  {
    TRY_ENTRY();
    m_threads_count = threads_count;
    m_main_thread_id = boost::this_thread::get_id();
    log_space::log_singletone::set_thread_log_prefix("[SRV_MAIN]");
    while(!m_stop_signal_sent)
    {

      // Create a pool of threads to run all of the io_services.
      CRITICAL_REGION_BEGIN(m_threads_lock);
      for (std::size_t i = 0; i < threads_count; ++i)
      {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
          attrs, boost::bind(&boosted_tcp_server<t_protocol_handler>::worker_thread, this)));
        m_threads.push_back(thread);
      }
      CRITICAL_REGION_END();
      // Wait for all threads in the pool to exit.
      if(wait)
      {
        for (std::size_t i = 0; i < m_threads.size(); ++i)
          m_threads[i]->join();
        m_threads.clear();

      }else
      {
        return true;
      }

      if(wait && !m_stop_signal_sent)
      {
        //some problems with the listening socket ?..
        LOG_PRINT_L0("Net service stopped without stop request, restarting...");
        if(!this->init_server(m_port, m_address))
        {
          LOG_PRINT_L0("Reiniting service failed, exit.");
          return false;
        }else
        {
          LOG_PRINT_L0("Reiniting OK.");
        }
      }
    }
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::run_server", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::is_thread_worker()
  {
    TRY_ENTRY();
    CRITICAL_REGION_LOCAL(m_threads_lock);
    BOOST_FOREACH(boost::shared_ptr<boost::thread>& thp,  m_threads)
    {
      if(thp->get_id() == boost::this_thread::get_id())
        return true;
    }
    if(m_threads_count == 1 && boost::this_thread::get_id() == m_main_thread_id)
      return true;
    return false;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::is_thread_worker", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop(uint64_t wait_mseconds)
  {
    TRY_ENTRY();
    boost::chrono::milliseconds ms(wait_mseconds);
    for (std::size_t i = 0; i < m_threads.size(); ++i)
    {
      if(m_threads[i]->joinable() && !m_threads[i]->try_join_for(ms))
      {
        LOG_PRINT_L0("Interrupting thread " << m_threads[i]->native_handle());
        m_threads[i]->interrupt();
      }
    }
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::send_stop_signal()
  {
    m_stop_signal_sent = true;
    TRY_ENTRY();
    io_service_.stop();
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::send_stop_signal()", void());
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::is_stop_signal_sent()
  {
    return m_stop_signal_sent;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::handle_accept(const boost::system::error_code& e)
  {
    TRY_ENTRY();
    if (!e)
    {
      connection_ptr conn(std::move(new_connection_));

      new_connection_.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter));
      acceptor_.async_accept(new_connection_->socket(),
        boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept, this,
        boost::asio::placeholders::error));

      bool r = conn->start(true, 1 < m_threads_count);
      if (!r)
        LOG_ERROR("[sock " << conn->socket().native_handle() << "] Failed to start connection, connections_count = " << m_sockets_count);
    }else
    {
      LOG_ERROR("Some problems at accept: " << e.message() << ", connections_count = " << m_sockets_count);
    }
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::handle_accept", void());
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::connect(const std::string& adr, const std::string& port, uint32_t conn_timeout, t_connection_context& conn_context, const std::string& bind_ip)
  {
    TRY_ENTRY();

    connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter) );
    boost::asio::ip::tcp::socket&  sock_ = new_connection_l->socket();
    
    //////////////////////////////////////////////////////////////////////////
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    if(iterator == end)
    {
      LOG_ERROR("Failed to resolve " << adr);
      return false;
    }
    //////////////////////////////////////////////////////////////////////////


    //boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::address::from_string(addr.c_str()), port);
    boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);
     
    sock_.open(remote_endpoint.protocol());
    if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "" )
    {
      boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(adr.c_str()), 0);
      sock_.bind(local_endpoint);
    }

    /*
    NOTICE: be careful to make sync connection from event handler: in case if all threads suddenly do sync connect, there will be no thread to dispatch events from io service.
    */

    boost::system::error_code ec = boost::asio::error::would_block;

    //have another free thread(s), work in wait mode, without event handling
    struct local_async_context
    {
      boost::system::error_code ec;
      boost::mutex connect_mut;
      boost::condition_variable cond;
    };

    boost::shared_ptr<local_async_context> local_shared_context(new local_async_context());
    local_shared_context->ec = boost::asio::error::would_block;
    boost::unique_lock<boost::mutex> lock(local_shared_context->connect_mut);
    auto connect_callback = [](boost::system::error_code ec_, boost::shared_ptr<local_async_context> shared_context)
    {
      shared_context->connect_mut.lock(); shared_context->ec = ec_; shared_context->connect_mut.unlock(); shared_context->cond.notify_one(); 
    };

    sock_.async_connect(remote_endpoint, boost::bind<void>(connect_callback, _1, local_shared_context));
    while(local_shared_context->ec == boost::asio::error::would_block)
    {
      bool r = local_shared_context->cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(conn_timeout));
      if(local_shared_context->ec == boost::asio::error::would_block && !r)
      {
        //timeout
        sock_.close();
        LOG_PRINT_L3("Failed to connect to " << adr << ":" << port << ", because of timeout (" << conn_timeout << ")");
        return false;
      }
    }
    ec = local_shared_context->ec;

    if (ec || !sock_.is_open())
    {
      LOG_PRINT("Some problems at connect, message: " << ec.message(), LOG_LEVEL_3);
      return false;
    }

    LOG_PRINT_L3("Connected success to " << adr << ':' << port);

    bool r = new_connection_l->start(false, 1 < m_threads_count);
    if (r)
    {
      new_connection_l->get_context(conn_context);
      //new_connection_l.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter));
    }
    else
    {
      LOG_ERROR("[sock " << new_connection_->socket().native_handle() << "] Failed to start connection, connections_count = " << m_sockets_count);
    }

    return r;

    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler> template<class t_callback>
  bool boosted_tcp_server<t_protocol_handler>::connect_async(const std::string& adr, const std::string& port, uint32_t conn_timeout, t_callback cb, const std::string& bind_ip)
  {
    TRY_ENTRY();    
    connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_config, m_sockets_count, m_pfilter) );
    boost::asio::ip::tcp::socket&  sock_ = new_connection_l->socket();
    
    //////////////////////////////////////////////////////////////////////////
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    if(iterator == end)
    {
      LOG_ERROR("Failed to resolve " << adr);
      return false;
    }
    //////////////////////////////////////////////////////////////////////////
    boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);
     
    sock_.open(remote_endpoint.protocol());
    if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "" )
    {
      boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(adr.c_str()), 0);
      sock_.bind(local_endpoint);
    }
    
    boost::shared_ptr<boost::asio::deadline_timer> sh_deadline(new boost::asio::deadline_timer(io_service_));
    //start deadline
    sh_deadline->expires_from_now(boost::posix_time::milliseconds(conn_timeout));
    sh_deadline->async_wait([=](const boost::system::error_code& error)
      {
          if(error != boost::asio::error::operation_aborted) 
          {
            LOG_PRINT_L3("Failed to connect to " << adr << ':' << port << ", because of timeout (" << conn_timeout << ")");
            new_connection_l->socket().close();
          }
      });
    //start async connect
    sock_.async_connect(remote_endpoint, [=](const boost::system::error_code& ec_)
      {
        t_connection_context conn_context = AUTO_VAL_INIT(conn_context);
        boost::system::error_code ignored_ec;
        boost::asio::ip::tcp::socket::endpoint_type lep = new_connection_l->socket().local_endpoint(ignored_ec);
        if(!ec_)
        {//success
          if(!sh_deadline->cancel())
          {
            cb(conn_context, boost::asio::error::operation_aborted);//this mean that deadline timer already queued callback with cancel operation, rare situation
          }else
          {
            LOG_PRINT_L3("[sock " << new_connection_l->socket().native_handle() << "] Connected success to " << adr << ':' << port <<
              " from " << lep.address().to_string() << ':' << lep.port());
            bool r = new_connection_l->start(false, 1 < m_threads_count);
            if (r)
            {
              new_connection_l->get_context(conn_context);
              cb(conn_context, ec_);
            }
            else
            {
              LOG_PRINT_L3("[sock " << new_connection_l->socket().native_handle() << "] Failed to start connection to " << adr << ':' << port);
              cb(conn_context, boost::asio::error::fault);
            }
          }
        }else
        {
          LOG_PRINT_L3("[sock " << new_connection_l->socket().native_handle() << "] Failed to connect to " << adr << ':' << port <<
            " from " << lep.address().to_string() << ':' << lep.port() << ": " << ec_.message() << ':' << ec_.value());
          cb(conn_context, ec_);
        }
      });
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect_async", false);
  }
}
}
PRAGMA_WARNING_POP
