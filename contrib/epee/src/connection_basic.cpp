/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief base for connection, contains e.g. the ratelimit hooks

// Copyright (c) 2014-2024, The Monero Project
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

/* rfree: implementation for the non-template base, can be used by connection<> template class in abstract_tcp_server2 file  */

#include "net/connection_basic.hpp"

#include "net/net_utils_base.h" 
#include "misc_log_ex.h" 
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include "misc_language.h"
#include <iomanip>

#include <boost/asio/basic_socket.hpp>

// TODO:
#include "net/network_throttle-detail.hpp"

#if BOOST_VERSION >= 107000
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.conn"

// ################################################################################################
// local (TU local) headers
// ################################################################################################

namespace epee
{
namespace net_utils
{

namespace
{
	boost::asio::ssl::context& get_context(connection_basic_shared_state* state)
	{
		CHECK_AND_ASSERT_THROW_MES(state != nullptr, "state shared_ptr cannot be null");
		return state->ssl_context;
	}
}

  std::string to_string(t_connection_type type)
  {
	  if (type == e_connection_type_NET)
		return std::string("NET");
	  else if (type == e_connection_type_RPC)
	    return std::string("RPC");
	  else if (type == e_connection_type_P2P)
	    return std::string("P2P");
	  
	  return std::string("UNKNOWN");
  }


/* ============================================================================ */

class connection_basic_pimpl {
	public:
		connection_basic_pimpl(const std::string &name);

		static int m_default_tos;

		network_throttle_bw m_throttle; // per-perr
    critical_section m_throttle_lock;

		int m_peer_number; // e.g. for debug/stats
};


} // namespace
} // namespace

// ################################################################################################
// The implementation part
// ################################################################################################

namespace epee
{
namespace net_utils
{

// ================================================================================================
// connection_basic_pimpl
// ================================================================================================
	
connection_basic_pimpl::connection_basic_pimpl(const std::string &name) : m_throttle(name), m_peer_number(0) { }

// ================================================================================================
// connection_basic
// ================================================================================================

// static variables:
int connection_basic_pimpl::m_default_tos;

// methods:
connection_basic::connection_basic(boost::asio::ip::tcp::socket&& sock, std::shared_ptr<connection_basic_shared_state> state, ssl_support_t ssl_support)
	:
	m_state(std::move(state)),
	mI( new connection_basic_pimpl("peer") ),
	strand_(GET_IO_SERVICE(sock)),
	socket_(GET_IO_SERVICE(sock), get_context(m_state.get())),
	m_want_close_connection(false),
	m_was_shutdown(false),
	m_is_multithreaded(false),
	m_ssl_support(ssl_support)
{
	// add nullptr checks if removed
	assert(m_state != nullptr); // release runtime check in get_context

        socket_.next_layer() = std::move(sock);

	++(m_state->sock_count); // increase the global counter
	mI->m_peer_number = m_state->sock_number.fetch_add(1); // use, and increase the generated number

	std::string remote_addr_str = "?";
	try { boost::system::error_code e; remote_addr_str = socket().remote_endpoint(e).address().to_string(); } catch(...){} ;

	_note("Spawned connection #"<<mI->m_peer_number<<" to " << remote_addr_str << " currently we have sockets count:" << m_state->sock_count);
}

connection_basic::connection_basic(boost::asio::io_service &io_service, std::shared_ptr<connection_basic_shared_state> state, ssl_support_t ssl_support)
	:
	m_state(std::move(state)),
	mI( new connection_basic_pimpl("peer") ),
	strand_(io_service),
	socket_(io_service, get_context(m_state.get())),
	m_want_close_connection(false),
	m_was_shutdown(false),
	m_is_multithreaded(false),
	m_ssl_support(ssl_support)
{
	// add nullptr checks if removed
	assert(m_state != nullptr); // release runtime check in get_context

	++(m_state->sock_count); // increase the global counter
	mI->m_peer_number = m_state->sock_number.fetch_add(1); // use, and increase the generated number

	std::string remote_addr_str = "?";
	try { boost::system::error_code e; remote_addr_str = socket().remote_endpoint(e).address().to_string(); } catch(...){} ;

	_note("Spawned connection #"<<mI->m_peer_number<<" to " << remote_addr_str << " currently we have sockets count:" << m_state->sock_count);
}

connection_basic::~connection_basic() noexcept(false) {
	--(m_state->sock_count);

	std::string remote_addr_str = "?";
	try { boost::system::error_code e; remote_addr_str = socket().remote_endpoint(e).address().to_string(); } catch(...){} ;
	_note("Destructing connection #"<<mI->m_peer_number << " to " << remote_addr_str);
}

void connection_basic::set_rate_up_limit(uint64_t limit) {
	{
		CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
		network_throttle_manager::get_global_throttle_out().set_target_speed(limit);
	}
	save_limit_to_file(limit);
}

void connection_basic::set_rate_down_limit(uint64_t limit) {
	{
	  CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_in );
		network_throttle_manager::get_global_throttle_in().set_target_speed(limit);
	}

	{
	  CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_inreq );
		network_throttle_manager::get_global_throttle_inreq().set_target_speed(limit);
	}
    save_limit_to_file(limit);
}

uint64_t connection_basic::get_rate_up_limit() {
    uint64_t limit;
    {
         CRITICAL_REGION_LOCAL( network_throttle_manager::m_lock_get_global_throttle_out );
         limit = network_throttle_manager::get_global_throttle_out().get_target_speed();
	}
    return limit;
}

uint64_t connection_basic::get_rate_down_limit() {
    uint64_t limit;
    {
         CRITICAL_REGION_LOCAL( network_throttle_manager::m_lock_get_global_throttle_in );
         limit = network_throttle_manager::get_global_throttle_in().get_target_speed();
	}
    return limit;
}

void connection_basic::save_limit_to_file(int limit) {
}
 
void connection_basic::set_tos_flag(int tos) {
	connection_basic_pimpl::m_default_tos = tos;
}

int connection_basic::get_tos_flag() {
	return connection_basic_pimpl::m_default_tos;
}

void connection_basic::sleep_before_packet(size_t packet_size, int phase,  int q_len) {
	double delay=0; // will be calculated
	do
	{ // rate limiting
		if (m_was_shutdown) { 
			_dbg2("m_was_shutdown - so abort sleep");
			return;
		}

		{ 
			CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
			delay = network_throttle_manager::get_global_throttle_out().get_sleep_time_after_tick( packet_size );
		}

		delay *= 0.50;
		if (delay > 0) {
            long int ms = (long int)(delay * 1000);
			MTRACE("Sleeping in " << __FUNCTION__ << " for " << ms << " ms before packet_size="<<packet_size); // debug sleep
			boost::this_thread::sleep(boost::posix_time::milliseconds( ms ) );
		}
	} while(delay > 0);

// XXX LATER XXX
	{
	  CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
		network_throttle_manager::get_global_throttle_out().handle_trafic_exact( packet_size ); // increase counter - global
	}

}

void connection_basic::do_send_handler_write(const void* ptr , size_t cb ) {
        // No sleeping here; sleeping is done once and for all in connection<t_protocol_handler>::handle_write
	MTRACE("handler_write (direct) - before ASIO write, for packet="<<cb<<" B (after sleep)");
}

void connection_basic::do_send_handler_write_from_queue( const boost::system::error_code& e, size_t cb, int q_len ) {
        // No sleeping here; sleeping is done once and for all in connection<t_protocol_handler>::handle_write
	MTRACE("handler_write (after write, from queue="<<q_len<<") - before ASIO write, for packet="<<cb<<" B (after sleep)");
}

void connection_basic::logger_handle_net_read(size_t size) { // network data read
}

void connection_basic::logger_handle_net_write(size_t size) {
}

double connection_basic::get_sleep_time(size_t cb) {
	CRITICAL_REGION_LOCAL(epee::net_utils::network_throttle_manager::network_throttle_manager::m_lock_get_global_throttle_out);
    auto t = network_throttle_manager::get_global_throttle_out().get_sleep_time(cb);
    return t;
}


} // namespace
} // namespace

