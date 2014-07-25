
// src/epee/network_throttle.h


#ifndef INCLUDED_epee_network_throttle
#define INCLUDED_epee_network_throttle

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

#include <memory>
#include <mutex>

namespace epee
{
namespace net_utils
{

// just typedefs to in code define the units used. TODO later it will be enforced that casts to other numericals are only explicit to avoid mistakes? use boost::chrono?
typedef double network_speed_kbps;
typedef double network_time_seconds;

class i_network_throttle;


namespace cryptonote { class cryptonote_protocol_handler_base; }; // a friend class

class network_throttle_manager {
	// provides global (singleton) in/inreq/out throttle access

	// [[note1]] see also http://www.nuonsoft.com/blog/2012/10/21/implementing-a-thread-safe-singleton-with-c11/
	// [[note2]] _inreq is the requested in traffic - we anticipate we will get in-bound traffic soon as result of what we do (e.g. that we sent network downloads requests)
	
	//protected:
	public: // XXX
		// [[note1]]
		static std::once_flag m_once_get_global_throttle_in; 
		static std::once_flag m_once_get_global_throttle_inreq; // [[note2]]
		static std::once_flag m_once_get_global_throttle_out;
		static std::unique_ptr<i_network_throttle> m_obj_get_global_throttle_in;
		static std::unique_ptr<i_network_throttle> m_obj_get_global_throttle_inreq;
		static std::unique_ptr<i_network_throttle> m_obj_get_global_throttle_out;

    static critical_section m_lock_get_global_throttle_in;
    static critical_section m_lock_get_global_throttle_inreq;
    static critical_section m_lock_get_global_throttle_out;

		friend class cryptonote::cryptonote_protocol_handler_base; // FRIEND - to directly access global throttle-s. !! REMEMBER TO USE LOCKS!
		friend class connection_basic; // FRIEND - to directly access global throttle-s. !! REMEMBER TO USE LOCKS!
		friend class connection_basic_pimpl; // ditto

		static int xxx;

	public: // XXX
	//protected:
		static i_network_throttle & get_global_throttle_in(); // singleton ; for friend class ; caller MUST use proper locks! like m_lock_get_global_throttle_in
		static i_network_throttle & get_global_throttle_inreq(); // ditto ; use lock ... _inreq obviously
		static i_network_throttle & get_global_throttle_out(); // ditto ; use lock ... _out obviously
};


class i_network_throttle {
	public:
		virtual void set_name(const std::string &name)=0;
		virtual void set_target_speed( network_speed_kbps target )=0;

		virtual void handle_trafic_exact(size_t packet_size) =0; // count the new traffic/packet; the size is exact considering all network costs
		virtual void handle_trafic_tcp(size_t packet_size) =0; // count the new traffic/packet; the size is as TCP, we will consider MTU etc
		virtual void handle_congestion(double overheat) =0; // call this when congestion is detected; see example use
		virtual void tick() =0; // poke and update timers/history
		
		// time calculations:
		virtual void calculate_times(size_t packet_size, double &A, double &W, double &D, double &R, bool dbg) const =0; // assuming sending new package (or 0), calculate:
		// Average, Window, Delay, Recommended data size

		// Average speed, Window size, recommended Delay to sleep now, Recommended size of data to send now

		virtual network_time_seconds get_sleep_time(size_t packet_size) const =0; // gets the D (recommended Delay time) from calc
		virtual network_time_seconds get_sleep_time_after_tick(size_t packet_size) =0; // ditto, but first tick the timer

		virtual size_t get_recommended_size_of_planned_transport() const =0; // what should be the recommended limit of data size that we can transport over current network_throttle in near future
};


// ... more in the -advanced.h file


} // namespace net_utils
} // namespace epee


#endif



