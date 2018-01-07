/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief base for connection, contains e.g. the ratelimit hooks

// Copyright (c) 2014-2018, The Monero Project
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

#include "syncobj.h"

#include "net/net_utils_base.h" 
#include "misc_log_ex.h" 
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/chrono.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include "misc_language.h"
#include "pragma_comp_defs.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/ip/unicast.hpp>
#include "net/abstract_tcp_server2.h"

// TODO:
#include "net/network_throttle-detail.hpp"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.p2p"

// ################################################################################################
// local (TU local) headers
// ################################################################################################

namespace epee
{
namespace net_utils
{

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
	
connection_basic_pimpl::connection_basic_pimpl(const std::string &name) : m_throttle(name) { }

// ================================================================================================
// connection_basic
// ================================================================================================

// static variables:
int connection_basic_pimpl::m_default_tos;

// methods:
connection_basic::connection_basic(boost::asio::io_service& io_service, std::atomic<long> &ref_sock_count, std::atomic<long> &sock_number)
	: 
	mI( new connection_basic_pimpl("peer") ),
	strand_(io_service),
	socket_(io_service),
	m_want_close_connection(false), 
	m_was_shutdown(false),
	m_ref_sock_count(ref_sock_count)
{ 
	++ref_sock_count; // increase the global counter
	mI->m_peer_number = sock_number.fetch_add(1); // use, and increase the generated number

	std::string remote_addr_str = "?";
	try { boost::system::error_code e; remote_addr_str = socket_.remote_endpoint(e).address().to_string(); } catch(...){} ;

	_note("Spawned connection p2p#"<<mI->m_peer_number<<" to " << remote_addr_str << " currently we have sockets count:" << m_ref_sock_count);
	//boost::filesystem::create_directories("log/dr-monero/net/");
}

connection_basic::~connection_basic() noexcept(false) {
	std::string remote_addr_str = "?";
	m_ref_sock_count--;
	try { boost::system::error_code e; remote_addr_str = socket_.remote_endpoint(e).address().to_string(); } catch(...){} ;
	_note("Destructing connection p2p#"<<mI->m_peer_number << " to " << remote_addr_str);
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
void connection_basic::set_start_time() {
	CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
	m_start_time = network_throttle_manager::get_global_throttle_out().get_time_seconds();
}

void connection_basic::do_send_handler_write(const void* ptr , size_t cb ) {
        // No sleeping here; sleeping is done once and for all in connection<t_protocol_handler>::handle_write
	MTRACE("handler_write (direct) - before ASIO write, for packet="<<cb<<" B (after sleep)");
	set_start_time();
}

void connection_basic::do_send_handler_write_from_queue( const boost::system::error_code& e, size_t cb, int q_len ) {
        // No sleeping here; sleeping is done once and for all in connection<t_protocol_handler>::handle_write
	MTRACE("handler_write (after write, from queue="<<q_len<<") - before ASIO write, for packet="<<cb<<" B (after sleep)");

	set_start_time();
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

void connection_basic::set_save_graph(bool save_graph) {
}


} // namespace
} // namespace

