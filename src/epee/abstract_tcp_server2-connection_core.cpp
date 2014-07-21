
/* implementation for the non-template base, for the connection<> template class */

#include "abstract_tcp_server2-connection_core.precompile.hpp"

// ################################################################################################
// ################################################################################################
// the "header part". Not separeted out for .hpp because point of this modification is 
// to rebuild just 1 translation unit while working on this code.
// (But maybe common parts will be separated out later though - if needed)
// ################################################################################################
// ################################################################################################

namespace epee
{
namespace net_utils
{

// just typedefs to in code define the units used. TODO later it will be enforced that casts to other numericals are only explicit to avoid mistakes
typedef double network_speed_kbps;
typedef double network_time_seconds;

class network_throttle {

	struct packet_info {
		size_t m_size; // octets sent. Summary for given small-window (e.g. for all packaged in 1 second)
	//	network_time_seconds m_time; // some form of monotonic clock, in seconds 
	// ^--- time is now known by the network_throttle class from index and m_last_sample_time

		packet_info();
	};

	network_speed_kbps m_target_speed;

	size_t m_network_add_cost; // estimated add cost of headers 
	size_t m_network_minimal_segment; // estimated minimal cost of sending 1 byte to round up to

	const size_t m_window_size; // the number of samples to average over
	network_time_seconds m_slot_size; // the size of one slot. TODO: now hardcoded for 1 second e.g. in time_to_slot()
	// TODO for big window size, for performance better the substract on change of m_last_sample_time instead of recalculating average of eg >100 elements

	std::vector< packet_info > m_history; // the history of bw usage

	network_time_seconds m_last_sample_time; // time of last history[0] - so we know when to rotate the buffer
	
	bool m_any_packet_yet; // did we yet got any packet to count


	// each sample is now 1 second

	public:
		network_throttle();

		void set_target_speed( network_speed_kbps target );
		
		void handle_trafic_exact(size_t packet_size); // count the new traffic/packet; the size is exact considering all network costs
		void handle_trafic_tcp(size_t packet_size); // count the new traffic/packet; the size is as TCP, we will consider MTU etc

		network_time_seconds get_sleep_time() const; // ask how much seconds we should sleep now - in order to meet the speed limit. <0 means that we should not sleep

		network_time_seconds time_to_slot(network_time_seconds t) const { return std::floor( t ); } // convert exact time eg 13.7 to rounded time for slot number in history 13
};

struct network_throttle_bw {
	network_throttle m_in;
	network_throttle m_out;
};

/* ============================================================================ */

class connection_basic_pimpl {
	public:
		static network_throttle_bw m_throttle_global; // global across all peers (and all types since not-templated)
    static critical_section m_throttle_global_lock;

		network_throttle_bw m_throttle; // per-perr
    critical_section m_throttle_lock;
};






} // namespace
} // namespace






// ################################################################################################
// ################################################################################################
// The implementation part
// ################################################################################################
// ################################################################################################

namespace epee
{
namespace net_utils
{

// ================================================================================================
// network_throttle
// ================================================================================================

network_throttle::packet_info::packet_info() 
	: m_size(0)
{ 
}

network_throttle::network_throttle() 
	: m_window_size(10), 
	  m_history( m_window_size )
{
	m_network_add_cost = 64;
	m_network_minimal_segment = 128;
	m_any_packet_yet = false;
	m_slot_size = 1.0; // hard coded in few places
	m_target_speed = 2 * 1024;
}

void network_throttle::set_target_speed( network_speed_kbps target ) 
{
	m_target_speed = target;
}
			
void network_throttle::handle_trafic_exact(size_t packet_size) 
{
	double time_now = time( NULL ); // TODO
	network_time_seconds current_sample_time_slot = time_to_slot( time_now ); // T=13.7 --> 13  (for 1-second smallwindow)
	network_time_seconds last_sample_time_slot = time_to_slot( m_last_sample_time );

	LOG_PRINT_L2("Traffic (counter " << (void*)this << ") packet_size="<<packet_size);

	// TODO: in loop: rotate few seconds if needed (fill with 0 the seconds-slots with no events in them)
	while ( (!m_any_packet_yet) || (last_sample_time_slot < current_sample_time_slot))
	{
		// LOG_PRINT_L4("Moving counter buffer by 1 second " << last_sample_time_slot << " < " << current_sample_time_slot << " (last time " << m_last_sample_time<<")");
		// rotate buffer 
		for (size_t i=m_history.size()-1; i>=1; --i) m_history[i] = m_history[i-1];
		m_history[0] = packet_info();
		if (! m_any_packet_yet) 
		{
			m_last_sample_time = time_now;	
		}
		m_last_sample_time += 1;	last_sample_time_slot = time_to_slot( m_last_sample_time ); // increase and recalculate time, time slot
		m_any_packet_yet=true;
	}

	m_history[0].m_size += packet_size;
}

void network_throttle::handle_trafic_tcp(size_t packet_size)
{
	size_t all_size = packet_size + m_network_add_cost;
	all_size = std::max( m_network_minimal_segment , all_size);
	handle_trafic_exact( all_size );
}

network_time_seconds network_throttle::get_sleep_time() const 
{
	network_time_seconds window_len = (m_window_size-1) * m_slot_size ; // -1 since current slot is not finished
	window_len += (m_last_sample_time - time_to_slot(m_last_sample_time));  // add the time for current slot e.g. 13.7-13 = 0.7
	auto W = window_len;
	// window_len e.g. 5.7 because takes into account current slot time

	size_t E = 0; // summ of traffic till now
	for (auto sample : m_history) E += sample.m_size; 

	double A = E/W; // current avg. speed
	//for (int i=0; i<m_history(); ++i) E += sample.m_size;
	auto M = m_target_speed; // max
	double D = (E - M*W) / M;

	std::ostringstream oss; 
	oss << "["; 
	for (auto sample: m_history) oss << sample.m_size << " ";
	oss << "]" << std::ends;
	std::string history_str = oss.str();

	LOG_PRINT_L0(
		"Rate limit: " 
		<< "speed is A=" << std::setw(8) <<A<<" vs "
		<< "M=" << std::setw(8) <<M<<" "
		<< " so sleep for "
		<< "D=" << std::setw(8) <<D<<" "
		<< "E="<< std::setw(8) << E << " M=" << std::setw(8) << M <<" W="<< std::setw(8) << W << " "
		<< "history: " << std::setw(8) << history_str << " "
		<< "m_last_sample_time=" << std::setw(8) << m_last_sample_time
	);

	return D;
}

// ================================================================================================
// connection_basic
// ================================================================================================

// static variables:


network_throttle_bw connection_basic_pimpl::m_throttle_global;
critical_section connection_basic_pimpl::m_throttle_global_lock;

connection_basic::connection_basic(boost::asio::io_service& io_service, i_connection_filter* &pfilter)
	: 
	mI(new connection_basic_pimpl),
	strand_(io_service),
	socket_(io_service),
	m_want_close_connection(0), 
	m_was_shutdown(0), 
	m_pfilter(pfilter)
{ 
	boost::asio::SettableSocketOption option;// = new boost::asio::SettableSocketOption();
	option.level(IPPROTO_IP);
	option.name(IP_TOS);
	option.value(&tos);
	option.size = sizeof(tos);
	socket_.set_option(option);
}

connection_basic::~connection_basic() {
}


void connection_basic::set_rate_up_limit(uint64_t limit) {
	connection_basic_pimpl::m_throttle_global.m_out.set_target_speed(limit);
}

void connection_basic::set_rate_down_limit(uint64_t limit) {
	connection_basic_pimpl::m_throttle_global.m_in.set_target_speed(limit);
}

void connection_basic::set_rate_limit(uint64_t limit) {
	connection_basic_pimpl::m_throttle_global.m_in.set_target_speed(limit);
	connection_basic_pimpl::m_throttle_global.m_out.set_target_speed(limit);
}

void connection_basic::set_rate_autodetect(uint64_t limit) {
	// TODO
	LOG_PRINT_L0("inside connection_basic we set autodetect (this is additional notification).");
}

void connection_basic::do_send_handler_start(const void* ptr , size_t cb ) {
	{ // rate limiting
		double delay=0; // will be calculated
		{ // increase counter and decide
			epee::critical_region_t<decltype(mI->m_throttle_global_lock)> guard(mI->m_throttle_global_lock); // *** critical *** 
			// !! warning, m_throttle is NOT locked here (yet)
			mI->m_throttle_global.m_out.handle_trafic_tcp( cb ); // increase counter
			delay = mI->m_throttle_global.m_out.get_sleep_time();
		//	LOG_PRINT_L0("Delay should be:" << delay);
		}
		if (delay > 0) {
			double sleep_ms = delay * 1000. ;
	//		LOG_PRINT_L0("SLEEP *** for delay = " << delay << "( sleep_ms = " << sleep_ms << " )");
			boost::this_thread::sleep(boost::posix_time::milliseconds( sleep_ms ));
	//		LOG_PRINT_L0("SLEEP *** for delay = " << delay << " - done");
		}
	}
}

void connection_basic::do_send_handler_delayed(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_write(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_stop(const void* ptr , size_t cb ) {
	if (cb<1) std::cout<<"X"; // TODO
	if (cb<1) std::cout<<"X"; // TODO
}

void connection_basic::do_send_handler_after_write( const boost::system::error_code& e, size_t cb ) {
}

void connection_basic::do_send_handler_write_from_queue( const boost::system::error_code& e, size_t cb ) {
}


} // namespace
} // namespace

