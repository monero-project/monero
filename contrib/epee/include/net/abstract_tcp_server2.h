/**
@file
@author from CrypoNote (see copyright below; Andrey N. Sabelnikov)
@monero rfree
@brief the connection templated-class for one peer connection
*/
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



#ifndef _ABSTRACT_TCP_SERVER2_H_ 
#define _ABSTRACT_TCP_SERVER2_H_ 


#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <atomic>
#include <map>
#include <memory>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/thread/thread.hpp>
#include "net_utils_base.h"
#include "syncobj.h"
#include "connection_basic.hpp"
#include "network_throttle-detail.hpp"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

#define ABSTRACT_SERVER_SEND_QUE_MAX_COUNT 1000

namespace epee
{
namespace net_utils
{

  struct i_connection_filter
  {
    virtual bool is_remote_host_allowed(const epee::net_utils::network_address &address)=0;
  protected:
    virtual ~i_connection_filter(){}
  };
  

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  /// Represents a single connection from a client.
  template<class t_protocol_handler>
  class connection
    : public boost::enable_shared_from_this<connection<t_protocol_handler> >,
    private boost::noncopyable, 
    public i_service_endpoint,
    public connection_basic
  {
  public:
    typedef typename t_protocol_handler::connection_context t_connection_context;
    /// Construct a connection with the given io_service.
   
    explicit connection( boost::asio::io_service& io_service,
			typename t_protocol_handler::config_type& config, 
			std::atomic<long> &ref_sock_count,  // the ++/-- counter 
			std::atomic<long> &sock_number, // the only increasing ++ number generator
			i_connection_filter * &pfilter
			,t_connection_type connection_type);

    virtual ~connection() noexcept(false);
    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket();

    /// Start the first asynchronous operation for the connection.
    bool start(bool is_income, bool is_multithreaded);

    void get_context(t_connection_context& context_){context_ = context;}

    void call_back_starter();
    
    void save_dbg_log();


		bool speed_limit_is_enabled() const; ///< tells us should we be sleeping here (e.g. do not sleep on RPC connections)

    bool cancel();
    
  private:
    //----------------- i_service_endpoint ---------------------
    virtual bool do_send(const void* ptr, size_t cb); ///< (see do_send from i_service_endpoint)
    virtual bool do_send_chunk(const void* ptr, size_t cb); ///< will send (or queue) a part of data
    virtual bool close();
    virtual bool call_run_once_service_io();
    virtual bool request_callback();
    virtual boost::asio::io_service& get_io_service();
    virtual bool add_ref();
    virtual bool release();
    //------------------------------------------------------
    boost::shared_ptr<connection<t_protocol_handler> > safe_shared_from_this();
    bool shutdown();
    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
      std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e, size_t cb);

    /// reset connection timeout timer and callback
    void reset_timer(boost::posix_time::milliseconds ms, bool add);
    boost::posix_time::milliseconds get_default_time() const;
    boost::posix_time::milliseconds get_timeout_from_bytes_read(size_t bytes) const;

    /// Buffer for incoming data.
    boost::array<char, 8192> buffer_;
    //boost::array<char, 1024> buffer_;

    t_connection_context context;
    i_connection_filter* &m_pfilter;

	// TODO what do they mean about wait on destructor?? --rfree :
    //this should be the last one, because it could be wait on destructor, while other activities possible on other threads
    t_protocol_handler m_protocol_handler;
    //typename t_protocol_handler::config_type m_dummy_config;
    std::list<boost::shared_ptr<connection<t_protocol_handler> > > m_self_refs; // add_ref/release support
    critical_section m_self_refs_lock;
    critical_section m_chunking_lock; // held while we add small chunks of the big do_send() to small do_send_chunk()
    
    t_connection_type m_connection_type;
    
    // for calculate speed (last 60 sec)
    network_throttle m_throttle_speed_in;
    network_throttle m_throttle_speed_out;
    boost::mutex m_throttle_speed_in_mutex;
    boost::mutex m_throttle_speed_out_mutex;

    boost::asio::deadline_timer m_timer;
    bool m_local;

	public:
			void setRpcStation();
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_protocol_handler>
  class boosted_tcp_server
    : private boost::noncopyable
  {
  public:
    typedef boost::shared_ptr<connection<t_protocol_handler> > connection_ptr;
    typedef typename t_protocol_handler::connection_context t_connection_context;
    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.

    boosted_tcp_server(t_connection_type connection_type);
    explicit boosted_tcp_server(boost::asio::io_service& external_io_service, t_connection_type connection_type);
    ~boosted_tcp_server();
    
    std::map<std::string, t_connection_type> server_type_map;
    void create_server_type_map();

    bool init_server(uint32_t port, const std::string address = "0.0.0.0");
    bool init_server(const std::string port,  const std::string& address = "0.0.0.0");

    /// Run the server's io_service loop.
    bool run_server(size_t threads_count, bool wait = true, const boost::thread::attributes& attrs = boost::thread::attributes());

    /// wait for service workers stop
    bool timed_wait_server_stop(uint64_t wait_mseconds);

    /// Stop the server.
    void send_stop_signal();

    bool is_stop_signal_sent();

    void set_threads_prefix(const std::string& prefix_name);

    bool deinit_server(){return true;}

    size_t get_threads_count(){return m_threads_count;}

    void set_connection_filter(i_connection_filter* pfilter);

    bool connect(const std::string& adr, const std::string& port, uint32_t conn_timeot, t_connection_context& cn, const std::string& bind_ip = "0.0.0.0");
    template<class t_callback>
    bool connect_async(const std::string& adr, const std::string& port, uint32_t conn_timeot, const t_callback &cb, const std::string& bind_ip = "0.0.0.0");

    typename t_protocol_handler::config_type& get_config_object(){return m_config;}

    int get_binded_port(){return m_port;}

    long get_connections_count() const
    {
      auto connections_count = (m_sock_count > 0) ? (m_sock_count - 1) : 0; // Socket count minus listening socket
      return connections_count;
    }

    boost::asio::io_service& get_io_service(){return io_service_;}

    struct idle_callback_conext_base
    {
      virtual ~idle_callback_conext_base(){}

      virtual bool call_handler(){return true;}

      idle_callback_conext_base(boost::asio::io_service& io_serice):
                                                          m_timer(io_serice)
      {}
      boost::asio::deadline_timer m_timer;
      uint64_t m_period;
    };

    template <class t_handler>
    struct idle_callback_conext: public idle_callback_conext_base
    {
      idle_callback_conext(boost::asio::io_service& io_serice, t_handler& h, uint64_t period):
                                                    idle_callback_conext_base(io_serice),
                                                    m_handler(h)
      {this->m_period = period;}

      t_handler m_handler;
      virtual bool call_handler()
      {
        return m_handler();
      }
    };

    template<class t_handler>
    bool add_idle_handler(t_handler t_callback, uint64_t timeout_ms)
      {
        boost::shared_ptr<idle_callback_conext_base> ptr(new idle_callback_conext<t_handler>(io_service_, t_callback, timeout_ms));
        //needed call handler here ?...
        ptr->m_timer.expires_from_now(boost::posix_time::milliseconds(ptr->m_period));
        ptr->m_timer.async_wait(boost::bind(&boosted_tcp_server<t_protocol_handler>::global_timer_handler, this, ptr));
        return true;
      }

    bool global_timer_handler(/*const boost::system::error_code& err, */boost::shared_ptr<idle_callback_conext_base> ptr)
    {
      //if handler return false - he don't want to be called anymore
      if(!ptr->call_handler())
        return true;
      ptr->m_timer.expires_from_now(boost::posix_time::milliseconds(ptr->m_period));
      ptr->m_timer.async_wait(boost::bind(&boosted_tcp_server<t_protocol_handler>::global_timer_handler, this, ptr));
      return true;
    }

    template<class t_handler>
    bool async_call(t_handler t_callback)
    {
      io_service_.post(t_callback);
      return true;
    }

  protected:
    typename t_protocol_handler::config_type m_config;

  private:
    /// Run the server's io_service loop.
    bool worker_thread();
    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e);

    bool is_thread_worker();

    /// The io_service used to perform asynchronous operations.
    std::unique_ptr<boost::asio::io_service> m_io_service_local_instance;
    boost::asio::io_service& io_service_;    

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;

    std::atomic<bool> m_stop_signal_sent;
    uint32_t m_port;
	std::atomic<long> m_sock_count;
	std::atomic<long> m_sock_number;
    std::string m_address;
    std::string m_thread_name_prefix; //TODO: change to enum server_type, now used
    size_t m_threads_count;
    i_connection_filter* m_pfilter;
    std::vector<boost::shared_ptr<boost::thread> > m_threads;
    boost::thread::id m_main_thread_id;
    critical_section m_threads_lock;
    volatile uint32_t m_thread_index; // TODO change to std::atomic

    t_connection_type m_connection_type;

    /// The next connection to be accepted
    connection_ptr new_connection_;

    boost::mutex connections_mutex;
    std::set<connection_ptr> connections_;

  }; // class <>boosted_tcp_server


} // namespace
} // namespace

#include "abstract_tcp_server2.inl"

#endif
