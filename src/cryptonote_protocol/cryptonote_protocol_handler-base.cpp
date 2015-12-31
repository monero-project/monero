/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief This is the place to implement our handlers for protocol network actions, e.g. for ratelimit for download-requests

// Copyright (c) 2014-2016, The Monero Project
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
#include "misc_language.h"
#include "pragma_comp_defs.h"
#include <sstream>
#include <iomanip>
#include <algorithm>


#include <boost/asio/basic_socket.hpp>
#include <boost/asio/ip/unicast.hpp>

#include "../../src/cryptonote_protocol/cryptonote_protocol_handler.h"
#include "../../src/p2p/network_throttle.hpp"

#include "../../contrib/otshell_utils/utils.hpp"
using namespace nOT::nUtils;

#include "../../../src/cryptonote_core/cryptonote_core.h" // e.g. for the send_stop_signal()

// ################################################################################################
// ################################################################################################
// the "header part". Not separated out for .hpp because point of this modification is 
// to rebuild just 1 translation unit while working on this code.
// (But maybe common parts will be separated out later though - if needed)
// ################################################################################################
// ################################################################################################

namespace cryptonote {

class cryptonote_protocol_handler_base_pimpl { // placeholer if needed
	public:

};

} // namespace

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

namespace cryptonote { 

double cryptonote_protocol_handler_base::estimate_one_block_size() noexcept { // for estimating size of blocks to downloa
 const double size_min = 500; // XXX 500
 //const int history_len = 20; // how many blocks to average over

 double avg=0;
 try {
 avg = get_avg_block_size(/*history_len*/);
 } catch (...) { }
 avg = std::max( size_min , avg);
 return avg;
}

cryptonote_protocol_handler_base::cryptonote_protocol_handler_base() {
}

cryptonote_protocol_handler_base::~cryptonote_protocol_handler_base() {
}

void cryptonote_protocol_handler_base::handler_request_blocks_history(std::list<crypto::hash>& ids) {
	using namespace epee::net_utils;
	LOG_PRINT_L1("### ~~~RRRR~~~~ ### sending request (type 2), limit = " << ids.size());
	LOG_PRINT_RED("RATE LIMIT NOT IMPLEMENTED HERE YET (download at unlimited speed?)" , LOG_LEVEL_1);
	_note_c("net/req2", "### ~~~RRRR~~~~ ### sending request (type 2), limit = " << ids.size());
	// TODO
}

void cryptonote_protocol_handler_base::handler_response_blocks_now(size_t packet_size) { _scope_dbg1("");
	using namespace epee::net_utils;
	double delay=0; // will be calculated
	_dbg1("Packet size: " << packet_size);
	do
	{ // rate limiting
		//XXX 
		/*if (::cryptonote::core::get_is_stopping()) { 
			_dbg1("We are stopping - so abort sleep");
			return;
		}*/
		/*if (m_was_shutdown) { 
			_dbg2_c("net/netuse/sleep","m_was_shutdown - so abort sleep");
			return;
		}*/

		{ 
	  	CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_out );
			delay = network_throttle_manager::get_global_throttle_out().get_sleep_time_after_tick( packet_size ); // decission from global
		}

		
		delay *= 0.50;
		//delay = 0; // XXX
		if (delay > 0) {
			//delay += rand2*0.1;
            		long int ms = (long int)(delay * 1000);
			_info_c("net/sleep", "Sleeping in " << __FUNCTION__ << " for " << ms << " ms before packet_size="<<packet_size); // XXX debug sleep
			_dbg1_c("net/sleep/", "sleep in sleep_before_packet");
			_dbg2("Sleep for " << ms);
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

} // namespace


