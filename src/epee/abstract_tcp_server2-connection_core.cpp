
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
	private:
		struct packet_info {
			size_t m_size; // octets sent. Summary for given small-window (e.g. for all packaged in 1 second)
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
		network_time_seconds m_start_time; // when we were created
		bool m_any_packet_yet; // did we yet got any packet to count

		double m_overheat; // last overheat
		double m_overheat_time; // time in seconds after epoch

		std::string m_name; // my name for debug and logs

	// each sample is now 1 second
	public:
		network_throttle();
		void set_name(const std::string &name);
		void set_target_speed( network_speed_kbps target );
		
		// add information about events:
		void handle_trafic_exact(size_t packet_size); // count the new traffic/packet; the size is exact considering all network costs
		void handle_trafic_tcp(size_t packet_size); // count the new traffic/packet; the size is as TCP, we will consider MTU etc
		void handle_congestion(double overheat); // call this when congestion is detected; see example use
		void tick(); // poke and update timers/history
		
		// time calculations:
		void calculate_times(size_t packet_size, double &A, double &W, double &D, bool dbg) const; // assuming sending new package (or 0), calculate Average, Window, Delay
		network_time_seconds get_sleep_time(size_t packet_size) const; // gets the D (recommended Delay time) from calc
		network_time_seconds get_sleep_time_after_tick(size_t packet_size); // ditto, but first tick the timer

		static double my_time_seconds(); // a timer

	private:
		network_time_seconds time_to_slot(network_time_seconds t) const { return std::floor( t ); } // convert exact time eg 13.7 to rounded time for slot number in history 13
		void _handle_trafic_exact(size_t packet_size, size_t orginal_size); 
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
		static int m_default_tos;

		network_throttle_bw m_throttle; // per-perr
    critical_section m_throttle_lock;

		void sleep_before_packet(size_t packet_size, int phase); // execute a sleep
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
	m_network_add_cost = 128;
	m_network_minimal_segment = 256;
	m_any_packet_yet = false;
	m_slot_size = 1.0; // hard coded in few places
	m_target_speed = 2 * 1024;
}

void network_throttle::set_name(const std::string &name) 
{
	m_name = name;
}

void network_throttle::set_target_speed( network_speed_kbps target ) 
{
	m_target_speed = target;
}
			
void network_throttle::tick()
{
	double time_now = my_time_seconds();
	if (!m_any_packet_yet) m_start_time = time_now; // starting now

	network_time_seconds current_sample_time_slot = time_to_slot( time_now ); // T=13.7 --> 13  (for 1-second smallwindow)
	network_time_seconds last_sample_time_slot = time_to_slot( m_last_sample_time );

	// moving to next position, and filling gaps
	// !! during this loop the m_last_sample_time and last_sample_time_slot mean the variable moved in +1
	// TODO optimize when moving few slots at once
	while ( (!m_any_packet_yet) || (last_sample_time_slot < current_sample_time_slot))
	{
		LOG_PRINT_L0("Moving counter buffer by 1 second " << last_sample_time_slot << " < " << current_sample_time_slot << " (last time " << m_last_sample_time<<")");
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
	m_last_sample_time = time_now; // the real exact last time
}

void network_throttle::handle_trafic_exact(size_t packet_size) 
{
	_handle_trafic_exact(packet_size, packet_size);
}

void network_throttle::_handle_trafic_exact(size_t packet_size, size_t orginal_size)
{
	double A,W,D;
	tick();
	calculate_times(packet_size, A,W,D, false);
	m_history[0].m_size += packet_size;
	LOG_PRINT_L0("Throttle " << m_name << ": packet of ~"<<packet_size<<"b " << " (from "<<orginal_size<<" b)" << " Speed AVG="<<A<<" bit/s (window="<<W<<" s)"); // XXX
}

void network_throttle::handle_trafic_tcp(size_t packet_size)
{
	size_t all_size = packet_size + m_network_add_cost;
	all_size = std::max( m_network_minimal_segment , all_size);
	_handle_trafic_exact( all_size , packet_size );
}

network_time_seconds network_throttle::get_sleep_time_after_tick(size_t packet_size) {
	tick();
	return get_sleep_time(packet_size);
}

network_time_seconds network_throttle::get_sleep_time(size_t packet_size) const 
{
	double A=0, W=0, D=0;
	calculate_times(packet_size, A,W,D, true);
	return D;
}

void network_throttle::calculate_times(size_t packet_size, double &A, double &W, double &D, bool dbg) const 
{
	if (!m_any_packet_yet) {
		W=0; A=0; D=0;
		return ; // no packet yet, I can not decide about sleep time
	}

	network_time_seconds window_len = (m_window_size-1) * m_slot_size ; // -1 since current slot is not finished
	window_len += (m_last_sample_time - time_to_slot(m_last_sample_time));  // add the time for current slot e.g. 13.7-13 = 0.7

	auto time_passed = my_time_seconds() - m_start_time;
	W = std::max( std::min( window_len , time_passed ) , m_slot_size )  ; // window length resulting from size of history but limited by how long ago history was started,
	// also at least slot size (e.g. 1 second) to not be ridiculous
	// window_len e.g. 5.7 because takes into account current slot time

	size_t Epast = 0; // summ of traffic till now
	for (auto sample : m_history) Epast += sample.m_size; 
	const size_t E = Epast;
	const size_t Enow = Epast + packet_size ; // including the data we're about to send now

	const double M = m_target_speed; // max
	const double D1 = (Epast - M*W) / M; // delay - how long to sleep to get back to target speed
	const double D2 = (Enow  - M*W) / M; // delay - how long to sleep to get back to target speed (including current packet)
	
	D = (D1*0.75 + D2*0.25); // finall sleep depends on both with/without current packet

	A = Epast/W; // current avg. speed (for info)

	if (dbg) {
		std::ostringstream oss; 
		oss << "["; 
		for (auto sample: m_history) oss << sample.m_size << " ";
		oss << "]" << std::ends;
		std::string history_str = oss.str();
		LOG_PRINT_L0(
			"... calculating net rate limit: " 
			<< "speed is A=" << std::setw(8) <<A<<" vs "
			<< "Max=" << std::setw(8) <<M<<" "
			<< " so sleep: "
			<< "D=" << std::setw(8) <<D<<" sec "
//			<< "D1=" << std::setw(8) <<D1<<" "
//			<< "D2=" << std::setw(8) <<D2<<" "
			<< "E="<< std::setw(8) << E << " "
			<< "M=" << std::setw(8) << M <<" W="<< std::setw(8) << W << " "
			<< "History: " << std::setw(8) << history_str << " "
			<< "m_last_sample_time=" << std::setw(8) << m_last_sample_time
		);

	}
}

double network_throttle::my_time_seconds() {
	using namespace boost::chrono;
	auto point = steady_clock::now();
	auto time_from_epoh = point.time_since_epoch();
	auto ms = duration_cast< milliseconds >( time_from_epoh ).count();
	double ms_f = ms;
	return ms_f / 1000.;
}

// ================================================================================================
// connection_basic
// ================================================================================================

// static variables:


network_throttle_bw connection_basic_pimpl::m_throttle_global;
critical_section connection_basic_pimpl::m_throttle_global_lock;
int connection_basic_pimpl::m_default_tos;

connection_basic::connection_basic(boost::asio::io_service& io_service, i_connection_filter* &pfilter)
	: 
	mI(new connection_basic_pimpl),
	strand_(io_service),
	socket_(io_service),
	m_want_close_connection(0), 
	m_was_shutdown(0), 
	m_pfilter(pfilter)
{ 
	mI->m_throttle_global.m_in.set_name("global-IN-DOWNLOAD");
	mI->m_throttle_global.m_out.set_name("global-OUT-REQUEST");

	/*boost::asio::SettableSocketOption option;// = new boost::asio::SettableSocketOption();
	option.level(IPPROTO_IP);
	option.name(IP_TOS);
	option.value(&tos);
	option.size = sizeof(tos);
	socket_.set_option(option);*/
	// TODO socket options
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
	LOG_PRINT_L0("inside connection_basic we set autodetect (this is additional notification)..");
}
 
void connection_basic::set_tos_flag(int tos) {
	connection_basic_pimpl::m_default_tos = tos;
}

void connection_basic_pimpl::sleep_before_packet(size_t packet_size, int phase) {
	{
		epee::critical_region_t<decltype(m_throttle_global_lock)> guard(m_throttle_global_lock); // *** critical *** 
		m_throttle_global.m_out.handle_trafic_tcp( packet_size ); // increase counter - global
	}

	double delay=0; // will be calculated
	do
	{ // rate limiting
		{ 
			epee::critical_region_t<decltype(m_throttle_global_lock)> guard(m_throttle_global_lock); // *** critical ***
			delay = m_throttle_global.m_out.get_sleep_time_after_tick( 0 ); // decission from global
		}

		if (delay > 0) { boost::this_thread::sleep(boost::posix_time::milliseconds( (int)(delay * 1000) ));	}
	} while(delay > 0);
}

void connection_basic::do_send_handler_start(const void* ptr , size_t cb ) {
	mI->sleep_before_packet(cb,1);
}

void connection_basic::do_send_handler_delayed(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_write(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_stop(const void* ptr , size_t cb ) {
}

void connection_basic::do_send_handler_after_write( const boost::system::error_code& e, size_t cb ) {
	// dela = end-start;

}

void connection_basic::do_send_handler_write_from_queue( const boost::system::error_code& e, size_t cb ) {
	// start_time = now();
	mI->sleep_before_packet(cb,2);
}

void connection_basic::do_read_handler_start(const boost::system::error_code& e, std::size_t bytes_transferred) { // from read, after read completion
	const size_t packet_size = bytes_transferred;
	{
		epee::critical_region_t<decltype(mI->m_throttle_global_lock)> guard(mI->m_throttle_global_lock); // *** critical *** 
//		mI->m_throttle_global.m_in.handle_trafic_tcp( packet_size ); // increase counter - global	
	}
}

} // namespace
} // namespace

