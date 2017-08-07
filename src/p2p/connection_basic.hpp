/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief base for connection, contains e.g. the ratelimit hooks

// ! This file might contain variable names same as in template class connection<> 
// ! from files contrib/epee/include/net/abstract_tcp_server2.*
// ! I am not a lawyer; afaik APIs, var names etc are not copyrightable ;)
// ! (how ever if in some wonderful juristdictions that is not the case, then why not make another sub-class withat that members and licence it as epee part)
// ! Working on above premise, IF this is valid in your juristdictions, then consider this code as released as:

// Copyright (c) 2014-2017, The Monero Project
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

/* rfree: place for hanlers for the non-template base, can be used by connection<> template class in abstract_tcp_server2 file  */

#ifndef INCLUDED_p2p_connection_basic_hpp
#define INCLUDED_p2p_connection_basic_hpp


#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/thread/thread.hpp>

#include <memory>

#include "net/net_utils_base.h"
#include "syncobj.h"

namespace epee
{
namespace net_utils
{

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  /// Represents a single connection from a client.

class connection_basic_pimpl; // PIMPL for this class

  enum t_connection_type { // type of the connection (of this server), e.g. so that we will know how to limit it
	  e_connection_type_NET = 0, // default (not used?)
	  e_connection_type_RPC = 1, // the rpc commands  (probably not rate limited, not chunked, etc)
	  e_connection_type_P2P = 2  // to other p2p node (probably limited)
  };
  
  std::string to_string(t_connection_type type);

class connection_basic { // not-templated base class for rapid developmet of some code parts
	public:
		std::unique_ptr< connection_basic_pimpl > mI; // my Implementation

		// moved here from orginal connecton<> - common member variables that do not depend on template in connection<>
    volatile uint32_t m_want_close_connection;
    std::atomic<bool> m_was_shutdown;
    critical_section m_send_que_lock;
    std::list<std::string> m_send_que;
    volatile bool m_is_multithreaded;
    double m_start_time;
    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand strand_;
    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

		std::atomic<long> &m_ref_sock_count; // reference to external counter of existing sockets that we will ++/--
	public:
		// first counter is the ++/-- count of current sockets, the other socket_number is only-increasing ++ number generator
		connection_basic(boost::asio::io_service& io_service, std::atomic<long> &ref_sock_count, std::atomic<long> &sock_number);

		virtual ~connection_basic() noexcept(false);

		// various handlers to be called from connection class:
		void do_send_handler_write(const void * ptr , size_t cb);
		void do_send_handler_write_from_queue(const boost::system::error_code& e, size_t cb , int q_len); // from handle_write, sending next part

		void logger_handle_net_write(size_t size); // network data written
		void logger_handle_net_read(size_t size); // network data read

		void set_start_time();

		// config for rate limit
		
		static void set_rate_up_limit(uint64_t limit);
		static void set_rate_down_limit(uint64_t limit);
		static uint64_t get_rate_up_limit();
		static uint64_t get_rate_down_limit();

		// config misc
		static void set_tos_flag(int tos); // ToS / QoS flag
		static int get_tos_flag();

		// handlers and sleep
		void sleep_before_packet(size_t packet_size, int phase, int q_len); // execute a sleep ; phase is not really used now(?)
		static void save_limit_to_file(int limit); ///< for dr-monero
		static double get_sleep_time(size_t cb);
		
		static void set_save_graph(bool save_graph);
};

} // nameserver
} // nameserver

#endif


