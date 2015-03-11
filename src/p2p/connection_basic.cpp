/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief base for connection, contains e.g. the ratelimit hooks

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

/* rfree: implementation for the non-template base, can be used by connection<> template class in abstract_tcp_server2 file  */

#include "connection_basic.hpp"

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

#include "../../contrib/epee/include/net/net_utils_base.h" 
#include "../../contrib/epee/include/misc_log_ex.h" 
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
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
#include "../../contrib/epee/include/net/abstract_tcp_server2.h"

#include "../../contrib/otshell_utils/utils.hpp"
using namespace nOT::nUtils;

// TODO:
#include "../../src/p2p/network_throttle-detail.hpp"
#include "../../src/cryptonote_core/cryptonote_core.h"

// ################################################################################################
// local (TU local) headers
// ################################################################################################

namespace epee
{
namespace net_utils
{


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
	_note("Spawned connection p2p#"<<mI->m_peer_number<<" currently we have sockets count:" << m_ref_sock_count);
	boost::filesystem::create_directories("log/dr-monero/net/");
	/*boost::asio::SettableSocketOption option;// = new boost::asio::SettableSocketOption();
	option.level(IPPROTO_IP);
	option.name(IP_TOS);
	option.value(&tos);
	option.size = sizeof(tos);
	socket_.set_option(option);*/
	// TODO socket options
}

connection_basic::~connection_basic() {
	_note("Destructing connection p2p#"<<mI->m_peer_number);
}

void connection_basic::set_rate_up_limit(uint64_t limit) {
	save_limit_to_file(limit);
	{
		// TODO remove __SCALING_FACTOR...
		const double SCALING_FACTOR = 2.25; // to acheve the best performance
		limit *= SCALING_FACTOR;
		CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
		network_throttle_manager::get_global_throttle_out().set_target_speed(limit);
	}
	//	connection_basic_pimpl::m_throttle_global.m_out.set_target_speed(limit);
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

void connection_basic::set_rate_limit(uint64_t limit) {
	// TODO
}
void connection_basic::set_kill_limit (uint64_t limit) {
       {
         CRITICAL_REGION_LOCAL(        network_throttle_manager::m_lock_get_global_throttle_in );
               network_throttle_manager::get_global_throttle_in().set_target_kill(limit);
       }

       {
         CRITICAL_REGION_LOCAL(        network_throttle_manager::m_lock_get_global_throttle_out );
               network_throttle_manager::get_global_throttle_out().set_target_kill(limit);
       }

       {
         CRITICAL_REGION_LOCAL(        network_throttle_manager::m_lock_get_global_throttle_inreq );
               network_throttle_manager::get_global_throttle_inreq().set_target_kill(limit);
       }
}

void connection_basic::save_limit_to_file(int limit) {
    // saving limit to file
    std::ofstream file;
    file.open("log/dr-monero/limit.info");
    file << limit;
}

void connection_basic::set_rate_autodetect(uint64_t limit) {
	// TODO
	LOG_PRINT_L0("inside connection_basic we set autodetect (this is additional notification)..");
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
		//XXX 
		/*if (::cryptonote::core::get_is_stopping()) { 
			_dbg1("We are stopping - so abort sleep");
			return;
		}*/
		if (m_was_shutdown) { 
			_dbg2("m_was_shutdown - so abort sleep");
			return;
		}

		{ 
	  	CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
			delay = network_throttle_manager::get_global_throttle_out().get_sleep_time_after_tick( packet_size ); // decission from global
		}

		
		delay *= 0.50;
		delay = 0; // XXX
		if (delay > 0) {
			//delay += rand2*0.1;
            		long int ms = (long int)(delay * 1000);
			_info_c("net/sleep", "Sleeping in " << __FUNCTION__ << " for " << ms << " ms before packet_size="<<packet_size); // XXX debug sleep
			_dbg1("sleep in sleep_before_packet");
			boost::this_thread::sleep(boost::posix_time::milliseconds( ms ) ); // TODO randomize sleeps
		}
	} while(delay > 0);

// XXX LATER XXX
	{
	  CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
		network_throttle_manager::get_global_throttle_out().handle_trafic_tcp( packet_size ); // increase counter - global
		//epee::critical_region_t<decltype(m_throttle_global_lock)> guard(m_throttle_global_lock); // *** critical *** 
		//m_throttle_global.m_out.handle_trafic_tcp( packet_size ); // increase counter - global
	}

}
void connection_basic::set_start_time() {
	CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
	m_start_time = network_throttle_manager::get_global_throttle_out().get_time_seconds();
}

void connection_basic::do_send_handler_start(const void* ptr , size_t cb ) {
	_fact_c("net/out/size", "*** do_sen() called for packet="<<cb<<" B");
    sleep_before_packet(cb,1,-1);
	// set_start_time();
}

void connection_basic::do_send_handler_delayed(const void* ptr , size_t cb ) {
	// CRITICAL_REGION_LOCAL(network_throttle_manager::m_lock_get_global_throttle_out);
	// auto sending_time = network_throttle_manager::get_global_throttle_out().get_time_seconds() - m_start_time; // wrong? --r
}

void connection_basic::do_send_handler_write(const void* ptr , size_t cb ) {
	sleep_before_packet(cb,1,-1);
	_info_c("net/out/size", "handler_write (direct) - before ASIO write, for packet="<<cb<<" B (after sleep)");
	set_start_time();
}

void connection_basic::do_send_handler_stop(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_after_write(const boost::system::error_code& e, size_t cb) {
	// CRITICAL_REGION_LOCAL(network_throttle_manager::m_lock_get_global_throttle_out);
	// auto sending_time = network_throttle_manager::get_global_throttle_out().get_time_seconds() - m_start_time;
	// lag: if current sending time > max sending time
	//if (sending_time > 0.1) network_throttle_manager::get_global_throttle_out().set_overheat(sending_time); // TODO

}

void connection_basic::do_send_handler_write_from_queue( const boost::system::error_code& e, size_t cb, int q_len ) {
	sleep_before_packet(cb,2,q_len);
	_info_c("net/out/size", "handler_write (after write, from queue="<<q_len<<") - before ASIO write, for packet="<<cb<<" B (after sleep)");

	set_start_time();
}

void connection_basic::do_read_handler_start(const boost::system::error_code& e, std::size_t bytes_transferred) { // from read, after read completion
	const size_t packet_size = bytes_transferred;
	{
	  CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_in );
	 // sleep_before_packet(packet_size * __SCALING_FACTOR, 1, -1); // TODO remove __SCALING_FACTOR
		network_throttle_manager::get_global_throttle_in().handle_trafic_tcp( packet_size ); // increase counter - global
		// epee::critical_region_t<decltype(mI->m_throttle_global_lock)> guard(mI->m_throttle_global_lock); // *** critical *** 
		// mI->m_throttle_global.m_in.handle_trafic_tcp( packet_size ); // increase counter - global	
	}
}

void connection_basic::logger_handle_net_peer(size_t size, bool io) { // network data written
        // TODO OPTIMIZE! do NOT reopen idiotically :)
        std::ostringstream oss;
    std::string filename;
    if (io) { // write
        double time = network_throttle_manager::get_global_throttle_in().get_time_seconds() ;
        oss << "log/dr-monero/net/in-peer-" << (mI->m_peer_number) << ".dat" << std::ends;
        filename = oss.str();
        network_throttle_manager::get_global_throttle_out().logger_handle_net(filename,time,size);
    }
    else { // read
        double time = network_throttle_manager::get_global_throttle_out().get_time_seconds() ;
        oss << "log/dr-monero/net/out-peer-" << (mI->m_peer_number) << ".dat" << std::ends;
        filename = oss.str();
        network_throttle_manager::get_global_throttle_in().logger_handle_net(filename,time,size);
    }
}

void connection_basic::logger_handle_net_read(size_t size) { // network data read
    std::string filename = "log/dr-monero/net/in-all.data";

    double time = network_throttle_manager::get_global_throttle_in().get_time_seconds() ;
    network_throttle_manager::get_global_throttle_in().logger_handle_net(filename, time, size);
    logger_handle_net_peer(size,0);
}

void connection_basic::logger_handle_net_write(size_t size) {
    std::string filename = "log/dr-monero/net/out-all.data";
    double time = network_throttle_manager::get_global_throttle_out().get_time_seconds() ;
    network_throttle_manager::get_global_throttle_out().logger_handle_net(filename, time, size);
    logger_handle_net_peer(size,1);

}

double connection_basic::get_sleep_time(size_t cb) {
    auto t = network_throttle_manager::get_global_throttle_out().get_sleep_time(cb);
    return t;
}


} // namespace
} // namespace

