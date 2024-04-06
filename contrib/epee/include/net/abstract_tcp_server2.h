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


#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <atomic>
#include <cassert>
#include <map>
#include <memory>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp>
#include <boost/optional.hpp>
#include "byte_slice.h"
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
    virtual bool is_remote_host_allowed(const epee::net_utils::network_address &address, time_t *t = NULL)=0;
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
  private:
    using connection_t = connection<t_protocol_handler>;
    using connection_ptr = boost::shared_ptr<connection_t>;
    using ssl_support_t = epee::net_utils::ssl_support_t;
    using timer_t = boost::asio::steady_timer;
    using duration_t = timer_t::duration;
    using ec_t = boost::system::error_code;
    using handshake_t = boost::asio::ssl::stream_base::handshake_type;

    using io_context_t = boost::asio::io_service;
    using strand_t = boost::asio::io_service::strand;
    using socket_t = boost::asio::ip::tcp::socket;

    using network_throttle_t = epee::net_utils::network_throttle;
    using network_throttle_manager_t = epee::net_utils::network_throttle_manager;

    unsigned int host_count(int delta = 0);
    duration_t get_default_timeout();
    duration_t get_timeout_from_bytes_read(size_t bytes) const;

    void state_status_check();

    void start_timer(duration_t duration, bool add = {});
    void async_wait_timer();
    void cancel_timer();

    void start_handshake();
    void start_read();
    void start_write();
    void start_shutdown();
    void cancel_socket();

    void cancel_handler();

    void interrupt();
    void on_interrupted();

    void terminate();
    void on_terminating();

    bool send(epee::byte_slice message);
    bool start_internal(
      bool is_income,
      bool is_multithreaded,
      boost::optional<network_address> real_remote
    );

    enum status_t {
      TERMINATED,
      RUNNING,
      INTERRUPTED,
      TERMINATING,
      WASTED,
    };

    struct state_t {
      struct stat_t {
        struct {
          network_throttle_t throttle{"speed_in", "throttle_speed_in"};
        } in;
        struct {
          network_throttle_t throttle{"speed_out", "throttle_speed_out"};
        } out;
      };

      struct data_t {
        struct {
          std::array<uint8_t, 0x2000> buffer;
        } read;
        struct {
          std::deque<epee::byte_slice> queue;
          bool wait_consume;
        } write;
      };

      struct ssl_t {
        bool enabled;
        bool forced;
        bool detected;
        bool handshaked;
      };

      struct socket_status_t {
        bool connected;

        bool wait_handshake;
        bool cancel_handshake;

        bool wait_read;
        bool handle_read;
        bool cancel_read;

        bool wait_write;
        bool handle_write;
        bool cancel_write;

        bool wait_shutdown;
        bool cancel_shutdown;
      };

      struct timer_status_t {
        bool wait_expire;
        bool cancel_expire;
        bool reset_expire;
      };

      struct timers_status_t {
        struct throttle_t {
          timer_status_t in;
          timer_status_t out;
        };

        timer_status_t general;
        throttle_t throttle;
      };

      struct protocol_t {
        size_t reference_counter;
        bool released;
        bool initialized;

        bool wait_release;
        bool wait_init;
        size_t wait_callback;
      };

      std::mutex lock;
      std::condition_variable_any condition;
      status_t status;
      socket_status_t socket;
      ssl_t ssl;
      timers_status_t timers;
      protocol_t protocol;
      stat_t stat;
      data_t data;
    };

    struct timers_t {
      timers_t(io_context_t &io_context):
        general(io_context),
        throttle(io_context)
      {}
      struct throttle_t {
        throttle_t(io_context_t &io_context):
          in(io_context),
          out(io_context)
        {}
        timer_t in;
        timer_t out;
      };

      timer_t general;
      throttle_t throttle;
    };

    io_context_t &m_io_context;
    t_connection_type m_connection_type;
    t_connection_context m_conn_context{};
    strand_t m_strand;
    timers_t m_timers;
    connection_ptr self{};
    bool m_local{};
    std::string m_host{};
    state_t m_state{};
    t_protocol_handler m_handler;
  public:
    struct shared_state : connection_basic_shared_state, t_protocol_handler::config_type
    {
      shared_state()
        : connection_basic_shared_state(), t_protocol_handler::config_type(), pfilter(nullptr), stop_signal_sent(false)
      {}

      i_connection_filter* pfilter;
      bool stop_signal_sent;
    };

    /// Construct a connection with the given io_service.
    explicit connection( boost::asio::io_service& io_service,
                        std::shared_ptr<shared_state> state,
			t_connection_type connection_type,
			epee::net_utils::ssl_support_t ssl_support);

    explicit connection( boost::asio::ip::tcp::socket&& sock,
			 std::shared_ptr<shared_state> state,
			t_connection_type connection_type,
			epee::net_utils::ssl_support_t ssl_support);



    virtual ~connection() noexcept(false);

    /// Start the first asynchronous operation for the connection.
    bool start(bool is_income, bool is_multithreaded);

    // `real_remote` is the actual endpoint (if connection is to proxy, etc.)
    bool start(bool is_income, bool is_multithreaded, network_address real_remote);

    void get_context(t_connection_context& context_){context_ = m_conn_context;}

    void call_back_starter();
    
    void save_dbg_log();


		bool speed_limit_is_enabled() const; ///< tells us should we be sleeping here (e.g. do not sleep on RPC connections)

    bool cancel();
    
  private:
    //----------------- i_service_endpoint ---------------------
    virtual bool do_send(byte_slice message); ///< (see do_send from i_service_endpoint)
    virtual bool send_done();
    virtual bool close();
    virtual bool call_run_once_service_io();
    virtual bool request_callback();
    virtual boost::asio::io_service& get_io_service();
    virtual bool add_ref();
    virtual bool release();
    //------------------------------------------------------
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
    enum try_connect_result_t
    {
      CONNECT_SUCCESS,
      CONNECT_FAILURE,
      CONNECT_NO_SSL,
    };

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

    bool init_server(uint32_t port, const std::string& address = "0.0.0.0",
	uint32_t port_ipv6 = 0, const std::string& address_ipv6 = "::", bool use_ipv6 = false, bool require_ipv4 = true,
	ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect);
    bool init_server(const std::string port,  const std::string& address = "0.0.0.0",
	const std::string port_ipv6 = "", const std::string address_ipv6 = "::", bool use_ipv6 = false, bool require_ipv4 = true,
	ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect);

    /// Run the server's io_service loop.
    bool run_server(size_t threads_count, bool wait = true, const boost::thread::attributes& attrs = boost::thread::attributes());

    /// wait for service workers stop
    bool timed_wait_server_stop(uint64_t wait_mseconds);

    /// Stop the server.
    void send_stop_signal();

    bool is_stop_signal_sent() const noexcept { return m_stop_signal_sent; };

    const std::atomic<bool>& get_stop_signal() const noexcept { return m_stop_signal_sent; }

    void set_threads_prefix(const std::string& prefix_name);

    bool deinit_server(){return true;}

    size_t get_threads_count(){return m_threads_count;}

    void set_connection_filter(i_connection_filter* pfilter);

    void set_default_remote(epee::net_utils::network_address remote)
    {
      default_remote = std::move(remote);
    }

    bool add_connection(t_connection_context& out, boost::asio::ip::tcp::socket&& sock, network_address real_remote, epee::net_utils::ssl_support_t ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_autodetect);
    try_connect_result_t try_connect(connection_ptr new_connection_l, const std::string& adr, const std::string& port, boost::asio::ip::tcp::socket &sock_, const boost::asio::ip::tcp::endpoint &remote_endpoint, const std::string &bind_ip, uint32_t conn_timeout, epee::net_utils::ssl_support_t ssl_support);
    bool connect(const std::string& adr, const std::string& port, uint32_t conn_timeot, t_connection_context& cn, const std::string& bind_ip = "0.0.0.0", epee::net_utils::ssl_support_t ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_autodetect);
    template<class t_callback>
    bool connect_async(const std::string& adr, const std::string& port, uint32_t conn_timeot, const t_callback &cb, const std::string& bind_ip = "0.0.0.0", epee::net_utils::ssl_support_t ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_autodetect);

    boost::asio::ssl::context& get_ssl_context() noexcept
    {
      assert(m_state != nullptr);
      return m_state->ssl_context;
    }

    typename t_protocol_handler::config_type& get_config_object()
    {
      assert(m_state != nullptr); // always set in constructor
      return *m_state;
    }

    std::shared_ptr<typename t_protocol_handler::config_type> get_config_shared()
    {
      assert(m_state != nullptr); // always set in constructor
      return {m_state};
    }

    int get_binded_port(){return m_port;}
    int get_binded_port_ipv6(){return m_port_ipv6;}

    long get_connections_count() const
    {
      assert(m_state != nullptr); // always set in constructor
      auto connections_count = m_state->sock_count > 0 ? (m_state->sock_count - 1) : 0; // Socket count minus listening socket
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
      uint64_t m_period;
    };

    template<class t_handler>
    bool add_idle_handler(t_handler t_callback, uint64_t timeout_ms)
      {
        boost::shared_ptr<idle_callback_conext<t_handler>> ptr(new idle_callback_conext<t_handler>(io_service_, t_callback, timeout_ms));
        //needed call handler here ?...
        ptr->m_timer.expires_from_now(boost::posix_time::milliseconds(ptr->m_period));
        ptr->m_timer.async_wait(boost::bind(&boosted_tcp_server<t_protocol_handler>::global_timer_handler<t_handler>, this, ptr));
        return true;
      }

    template<class t_handler>
    bool global_timer_handler(/*const boost::system::error_code& err, */boost::shared_ptr<idle_callback_conext<t_handler>> ptr)
    {
      //if handler return false - he don't want to be called anymore
      if(!ptr->call_handler())
        return true;
      ptr->m_timer.expires_from_now(boost::posix_time::milliseconds(ptr->m_period));
      ptr->m_timer.async_wait(boost::bind(&boosted_tcp_server<t_protocol_handler>::global_timer_handler<t_handler>, this, ptr));
      return true;
    }

    template<class t_handler>
    bool async_call(t_handler t_callback)
    {
      io_service_.post(t_callback);
      return true;
    }

  private:
    /// Run the server's io_service loop.
    bool worker_thread();
    /// Handle completion of an asynchronous accept operation.
    void handle_accept_ipv4(const boost::system::error_code& e);
    void handle_accept_ipv6(const boost::system::error_code& e);
    void handle_accept(const boost::system::error_code& e, bool ipv6 = false);

    bool is_thread_worker();

    const std::shared_ptr<typename connection<t_protocol_handler>::shared_state> m_state;

    /// The io_service used to perform asynchronous operations.
    struct worker
    {
      worker()
        : io_service(), work(io_service)
      {}

      boost::asio::io_service io_service;
      boost::asio::io_service::work work;
    };
    std::unique_ptr<worker> m_io_service_local_instance;
    boost::asio::io_service& io_service_;    

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::acceptor acceptor_ipv6;
    epee::net_utils::network_address default_remote;

    std::atomic<bool> m_stop_signal_sent;
    uint32_t m_port;
    uint32_t m_port_ipv6;
    std::string m_address;
    std::string m_address_ipv6;
    bool m_use_ipv6;
    bool m_require_ipv4;
    std::string m_thread_name_prefix; //TODO: change to enum server_type, now used
    size_t m_threads_count;
    std::vector<boost::shared_ptr<boost::thread> > m_threads;
    boost::thread::id m_main_thread_id;
    critical_section m_threads_lock;
    std::atomic<uint32_t> m_thread_index;

    t_connection_type m_connection_type;

    /// The next connection to be accepted
    connection_ptr new_connection_;
    connection_ptr new_connection_ipv6;


    boost::mutex connections_mutex;
    std::set<connection_ptr> connections_;
  }; // class <>boosted_tcp_server


} // namespace
} // namespace

#include "abstract_tcp_server2.inl"

#endif
