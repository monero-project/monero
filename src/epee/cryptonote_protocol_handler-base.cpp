
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
#include "../../src/epee/network_throttle.h"

/* implementation for the non-template base, for the connection<> template class */


// ################################################################################################
// ################################################################################################
// the "header part". Not separeted out for .hpp because point of this modification is 
// to rebuild just 1 translation unit while working on this code.
// (But maybe common parts will be separated out later though - if needed)
// ################################################################################################
// ################################################################################################

namespace cryptonote {

class cryptonote_protocol_handler_base_pimpl { 
	public:

};

}

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

namespace cryptonote { 


cryptonote_protocol_handler_base::cryptonote_protocol_handler_base() {
}

cryptonote_protocol_handler_base::~cryptonote_protocol_handler_base() {
}

void cryptonote_protocol_handler_base::handler_request_blocks_now(size_t &count_limit) {
	using namespace epee::net_utils;
	size_t est_req_size=0; //  how much data are we now requesting (to be soon send to us)

	//auto default_count_limit = count_limit;

	network_throttle_manager::xxx = 3;


	bool allowed_now = false; // are we now allowed to request or are we limited still

	while (!allowed_now) {

		{
			size_t size_limit1=0, size_limit2=0;
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
			size_t one_block_estimated_size = 8000; // TODO estimate better

			size_t limit_small = std::min( size_limit1 , size_limit2 );
			size_t size_limit = limit_small/3 + size_limit1/3 + size_limit2/3;
			if (limit_small <= 0) size_limit = 0;
			const double estimated_peers = 1.2; // how many peers/threads we want to talk to, in order to not grab entire b/w by 1 thread
			const double knob = 1.000;
			size_limit /= (estimated_peers / estimated_peers) * knob;
			LOG_PRINT_RED("calculating REQUEST size:" << size_limit1 << " " << size_limit2 << " small=" << limit_small << " final size_limit="<<size_limit , LOG_LEVEL_0);

			count_limit = size_limit / one_block_estimated_size;
			// TODO sleep if we decided we can not take ANY block now? (sleep NOT IN LOCK!!)
			count_limit = std::min((size_t)200, std::max((size_t)1, (size_t)count_limit)); // download at least 1 block

			est_req_size = count_limit * one_block_estimated_size ; // how much data did we just requested?
		}

		if (count_limit > 1) allowed_now = true;
		if (!allowed_now) boost::this_thread::sleep(boost::posix_time::milliseconds( 2000 ) ); // TODO randomize sleeps
	}
	// done waiting&sleeping ^

	// ok we are allowed to send now
	{
		CRITICAL_REGION_LOCAL(	network_throttle_manager::m_lock_get_global_throttle_inreq );
		network_throttle_manager::get_global_throttle_inreq().handle_trafic_tcp( est_req_size ); // increase countere of the global requested input
	}


	LOG_PRINT_YELLOW("### RRRR ### sending request (type 1), CALCULATED limit = " << count_limit << " = estimated " << est_req_size << " b", LOG_LEVEL_0);	
}

void cryptonote_protocol_handler_base::handler_request_blocks_history(std::list<crypto::hash>& ids) {
	using namespace epee::net_utils;
	LOG_PRINT_L0("### ~~~RRRR~~~~ ### sending request (type 2), limit = " << ids.size());
	LOG_PRINT_RED("RATE LIMIT NOT IMPLEMENTED HERE YET (download at unlimited speed?)" , LOG_LEVEL_0);
	// TODO
}

}


