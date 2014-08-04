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

/*
 * rfree: This is the place to implement our new handlers that will be called from son the full templates class.
 * rfree: I use this in e.g. rate limiting code.
 */

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

#include "../../external/otshell_utils/utils.hpp"
using namespace nOT::nUtils;


// ################################################################################################
// ################################################################################################
// the "header part". Not separeted out for .hpp because point of this modification is 
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
 const double size_min = 500;
 const int history_len = 20; // how many blocks to average over

 double avg=0;
 try {
 avg = get_avg_block_size(history_len);
 } catch (...) { }
 avg = std::max( size_min , avg);
 return avg;
}

cryptonote_protocol_handler_base::cryptonote_protocol_handler_base() {
}

cryptonote_protocol_handler_base::~cryptonote_protocol_handler_base() {
}

void cryptonote_protocol_handler_base::handler_request_blocks_now(size_t &count_limit) {
	using namespace epee::net_utils;
	size_t est_req_size=0; //  how much data are we now requesting (to be soon send to us)

	const auto count_limit_default = count_limit;

	bool allowed_now = false; // are we now allowed to request or are we limited still


	while (!allowed_now) {

			LOG_PRINT_RED("[DBG]" << get_avg_block_size(1), LOG_LEVEL_0);
		{
			long int size_limit1=0, size_limit2=0;
			LOG_PRINT_RED("calculating REQUEST size:", LOG_LEVEL_0);
			{
				CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_in );
				network_throttle_manager::get_global_throttle_in().tick();
				size_limit1 = network_throttle_manager::get_global_throttle_in().get_recommended_size_of_planned_transport();
			}
			{
				CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_inreq );
				network_throttle_manager::get_global_throttle_inreq().tick();
				size_limit2 = network_throttle_manager::get_global_throttle_inreq().get_recommended_size_of_planned_transport();
			}
			long int one_block_estimated_size = 8000; // TODO estimate better

			long int limit_small = std::min( size_limit1 , size_limit2 );
			long int size_limit = limit_small/3 + size_limit1/3 + size_limit2/3;
			if (limit_small <= 0) size_limit = 0;
			const double estimated_peers = 1.2; // how many peers/threads we want to talk to, in order to not grab entire b/w by 1 thread
			const double knob = 1.000;
			size_limit /= (estimated_peers / estimated_peers) * knob;
			_note_c("net/req-calc" , "calculating REQUEST size:" << size_limit1 << " " << size_limit2 << " small=" << limit_small << " final size_limit="<<size_limit);

			double L = size_limit / one_block_estimated_size; // calculating item limit (some heuristics)
			LOG_PRINT_RED("L1 = " << L , LOG_LEVEL_0);
			//double L2=0; if (L>1) L2=std::log(L);
			//L = L/10. + L2*5;
			LOG_PRINT_RED("L2 = " << L , LOG_LEVEL_0);
			L = std::min( (double)count_limit_default, (double)L);
			LOG_PRINT_RED("L3 = " << L , LOG_LEVEL_0);

			const long int hard_limit = 50; // never get more blocks at once ; TODO depend on speed limit. Must be low or limiting is too bursty.

			L = std::min(L, (double) hard_limit);

			count_limit = (int)L;

			est_req_size = count_limit * one_block_estimated_size ; // how much data did we just requested?
		}

		if (count_limit > 0) allowed_now = true;
		if (!allowed_now) boost::this_thread::sleep(boost::posix_time::milliseconds( 2000 ) ); // TODO randomize sleeps
	}
	// done waiting&sleeping ^

	// ok we are allowed to send now
	{
		CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_inreq );
		network_throttle_manager::get_global_throttle_inreq().handle_trafic_tcp( est_req_size ); // increase countere of the global requested input
	}

	// TODO remove debug
	LOG_PRINT_RED("\n", LOG_LEVEL_0);
	LOG_PRINT_YELLOW("*************************************************************************", LOG_LEVEL_0);
	LOG_PRINT_RED("### RRRR ### sending request (type 1), CALCULATED limit = " << count_limit << " = estimated " << est_req_size << " b", LOG_LEVEL_0);
	get_logreq() << "### RRRR ### sending request (type 1), CALCULATED limit = " << count_limit << " = estimated " << est_req_size << " b " << std::endl;
	LOG_PRINT_YELLOW("*************************************************************************", LOG_LEVEL_0);
	LOG_PRINT_RED("\n", LOG_LEVEL_0);

	_mark_c("net/req", "### RRRR ### sending request (type 1), CALCULATED limit = " << count_limit << " = estimated " << est_req_size << " b");
}

void cryptonote_protocol_handler_base::handler_request_blocks_history(std::list<crypto::hash>& ids) {
	using namespace epee::net_utils;
	LOG_PRINT_L0("### ~~~RRRR~~~~ ### sending request (type 2), limit = " << ids.size());
	LOG_PRINT_RED("RATE LIMIT NOT IMPLEMENTED HERE YET (download at unlimited speed?)" , LOG_LEVEL_0);
	// TODO
}

} // namespace


