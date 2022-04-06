/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief base for connection, contains e.g. the ratelimit hooks

// ! This file might contain variable names same as in template class connection<> 
// ! from files contrib/epee/include/net/abstract_tcp_server2.*
// ! I am not a lawyer; afaik APIs, var names etc are not copyrightable ;)
// ! (how ever if in some wonderful juristdictions that is not the case, then why not make another sub-class withat that members and licence it as epee part)
// ! Working on above premise, IF this is valid in your juristdictions, then consider this code as released as:

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

/* rfree: place for hanlers for the non-template base, can be used by connection<> template class in abstract_tcp_server2 file  */

#ifndef INCLUDED_p2p_connection_basic_hpp
#define INCLUDED_p2p_connection_basic_hpp


#include <string>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "byte_slice.h"
#include "net/net_utils_base.h"
#include "net/net_ssl.h"
#include "syncobj.h"

namespace epee
{
namespace net_utils
{

	class connection_basic_shared_state
	{
		ssl_options_t ssl_options_;
	public:
		boost::asio::ssl::context ssl_context;
		std::atomic<long> sock_count;
		std::atomic<long> sock_number;

		connection_basic_shared_state()
		  : ssl_options_(ssl_support_t::e_ssl_support_disabled),
		    ssl_context(boost::asio::ssl::context::tlsv12),
		    sock_count(0),
		    sock_number(0)
		{}

		void configure_ssl(ssl_options_t src)
		{
			ssl_options_ = std::move(src);
			ssl_context = ssl_options_.create_context();
		}

		const ssl_options_t& ssl_options() const noexcept { return ssl_options_; }
	};

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
		// beware of removing const, net_utils::connection is sketchily doing a cast to prevent storing ptr twice
		const std::shared_ptr<connection_basic_shared_state> m_state;
	public:

		std::unique_ptr< connection_basic_pimpl > mI; // my Implementation

		// moved here from orginal connecton<> - common member variables that do not depend on template in connection<>
    std::atomic<bool> m_want_close_connection;
    std::atomic<bool> m_was_shutdown;
    critical_section m_send_que_lock;
    std::deque<byte_slice> m_send_que;
    volatile bool m_is_multithreaded;
    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand strand_;
    /// Socket for the connection.
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    ssl_support_t m_ssl_support;

	public:
		// first counter is the ++/-- count of current sockets, the other socket_number is only-increasing ++ number generator
		connection_basic(boost::asio::ip::tcp::socket&& socket, std::shared_ptr<connection_basic_shared_state> state, ssl_support_t ssl_support);
		connection_basic(boost::asio::io_service &io_service, std::shared_ptr<connection_basic_shared_state> state, ssl_support_t ssl_support);

		virtual ~connection_basic() noexcept(false);

                //! \return `shared_state` object passed in construction (ptr never changes).
		connection_basic_shared_state& get_state() noexcept { return *m_state; /* verified in constructor */ }
		connection_basic(boost::asio::io_service& io_service, std::atomic<long> &ref_sock_count, std::atomic<long> &sock_number, ssl_support_t ssl);

		boost::asio::ip::tcp::socket& socket() { return socket_.next_layer(); }
		ssl_support_t get_ssl_support() const { return m_ssl_support; }
		void disable_ssl() { m_ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_disabled; }

		bool handshake(boost::asio::ssl::stream_base::handshake_type type, boost::asio::const_buffer buffer = {})
		{
			//m_state != nullptr verified in constructor
			return m_state->ssl_options().handshake(socket_, type, buffer);
		}

		template<typename MutableBufferSequence, typename ReadHandler>
		void async_read_some(const MutableBufferSequence &buffers, ReadHandler &&handler)
		{
			if (m_ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_enabled)
				socket_.async_read_some(buffers, std::forward<ReadHandler>(handler));
			else
				socket().async_read_some(buffers, std::forward<ReadHandler>(handler));
		}

		template<typename ConstBufferSequence, typename WriteHandler>
		void async_write_some(const ConstBufferSequence &buffers, WriteHandler &&handler)
		{
			if (m_ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_enabled)
				socket_.async_write_some(buffers, std::forward<WriteHandler>(handler));
			else
				socket().async_write_some(buffers, std::forward<WriteHandler>(handler));
		}

		template<typename ConstBufferSequence, typename WriteHandler>
		void async_write(const ConstBufferSequence &buffers, WriteHandler &&handler)
		{
			if (m_ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_enabled)
				boost::asio::async_write(socket_, buffers, std::forward<WriteHandler>(handler));
			else
				boost::asio::async_write(socket(), buffers, std::forward<WriteHandler>(handler));
		}

		// various handlers to be called from connection class:
		void do_send_handler_write(const void * ptr , size_t cb);
		void do_send_handler_write_from_queue(const boost::system::error_code& e, size_t cb , int q_len); // from handle_write, sending next part

		void logger_handle_net_write(size_t size); // network data written
		void logger_handle_net_read(size_t size); // network data read

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
};

} // nameserver
} // nameserver

#endif


