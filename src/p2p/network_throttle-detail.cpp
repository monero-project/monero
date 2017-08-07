/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief implementaion for throttling of connection (count and rate-limit speed etc)

// Copyright (c) 2014-2017, The Monero Project
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

/* rfree: implementation for throttle details */

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
#include "misc_language.h"
#include "pragma_comp_defs.h"
#include <sstream>
#include <iomanip>
#include <algorithm>



#include <boost/asio/basic_socket.hpp>
#include <boost/asio/ip/unicast.hpp>
#include "net/abstract_tcp_server2.h"

// TODO:
#include "network_throttle-detail.hpp"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.throttle"

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


/* ============================================================================ */

class connection_basic_pimpl {
	public:
		connection_basic_pimpl(const std::string &name);

		static int m_default_tos;

		network_throttle_bw m_throttle; // per-perr
    critical_section m_throttle_lock;

		void _packet(size_t packet_size, int phase, int q_len); // execute a sleep ; phase is not really used now(?) could be used for different kinds of sleep e.g. direct/queue write
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

network_throttle::~network_throttle() { }

network_throttle::packet_info::packet_info() 
	: m_size(0)
{ 
}

network_throttle::network_throttle(const std::string &nameshort, const std::string &name, int window_size)
    : m_window_size( (window_size==-1) ? 10 : window_size  ),
	  m_history( m_window_size ), m_nameshort(nameshort)
{
	set_name(name);
	m_network_add_cost = 128;
	m_network_minimal_segment = 256;
	m_network_max_segment = 1024*1024;
	m_any_packet_yet = false;
	m_slot_size = 1.0; // hard coded in few places
	m_target_speed = 16 * 1024; // other defaults are probably defined in the command-line parsing code when this class is used e.g. as main global throttle
}

void network_throttle::set_name(const std::string &name) 
{
	m_name = name;
}

void network_throttle::set_target_speed( network_speed_kbps target ) 
{
    m_target_speed = target * 1024;
	MINFO("Setting LIMIT: " << target << " kbps");
	set_real_target_speed(target);
}

void network_throttle::set_real_target_speed( network_speed_kbps real_target )
{
	m_real_target_speed = real_target * 1024;
}

network_speed_kbps network_throttle::get_target_speed()
{
	return m_real_target_speed / 1024;
}
			
void network_throttle::tick()
{
	double time_now = get_time_seconds();
	if (!m_any_packet_yet) m_start_time = time_now; // starting now

	network_time_seconds current_sample_time_slot = time_to_slot( time_now ); // T=13.7 --> 13  (for 1-second smallwindow)
	network_time_seconds last_sample_time_slot = time_to_slot( m_last_sample_time );

	// moving to next position, and filling gaps
	// !! during this loop the m_last_sample_time and last_sample_time_slot mean the variable moved in +1
	// TODO optimize when moving few slots at once
	while ( (!m_any_packet_yet) || (last_sample_time_slot < current_sample_time_slot))
	{
		_dbg3("Moving counter buffer by 1 second " << last_sample_time_slot << " < " << current_sample_time_slot << " (last time " << m_last_sample_time<<")");
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
	tick();

	calculate_times_struct cts ;  calculate_times(packet_size, cts , false, -1);
	calculate_times_struct cts2;  calculate_times(packet_size, cts2, false, 5);
	m_history[0].m_size += packet_size;

	std::ostringstream oss; oss << "["; 	for (auto sample: m_history) oss << sample.m_size << " ";	 oss << "]" << std::ends;
	std::string history_str = oss.str();

	MTRACE("Throttle " << m_name << ": packet of ~"<<packet_size<<"b " << " (from "<<orginal_size<<" b)"
        << " Speed AVG=" << std::setw(4) <<  ((long int)(cts .average/1024)) <<"[w="<<cts .window<<"]"
        <<           " " << std::setw(4) <<  ((long int)(cts2.average/1024)) <<"[w="<<cts2.window<<"]"
				<<" / " << " Limit="<< ((long int)(m_target_speed/1024)) <<" KiB/sec "
				<< " " << history_str
		);
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

void network_throttle::logger_handle_net(const std::string &filename, double time, size_t size) {
    boost::mutex mutex;
    mutex.lock(); {
        std::fstream file;
        file.open(filename.c_str(), std::ios::app | std::ios::out );
        file.precision(6);
        if(!file.is_open())
            _warn("Can't open file " << filename);
        file << static_cast<int>(time) << " " << static_cast<double>(size/1024) << "\n";
        file.close();
    }  mutex.unlock();
}

// fine tune this to decide about sending speed:
network_time_seconds network_throttle::get_sleep_time(size_t packet_size) const 
{
	double D2=0;
	calculate_times_struct cts = { 0, 0, 0, 0};
	calculate_times(packet_size, cts, true, m_window_size); D2=cts.delay;
	return D2;
}

// MAIN LOGIC:
void network_throttle::calculate_times(size_t packet_size, calculate_times_struct &cts, bool dbg, double force_window) const 
{
    const double the_window_size = std::max( (double)m_window_size ,
		((force_window>0) ? force_window : m_window_size) 
	);

	if (!m_any_packet_yet) {
		cts.window=0; cts.average=0; cts.delay=0; 
		cts.recomendetDataSize = m_network_minimal_segment; // should be overrided by caller anyway
		return ; // no packet yet, I can not decide about sleep time
	}

	network_time_seconds window_len = (the_window_size-1) * m_slot_size ; // -1 since current slot is not finished
	window_len += (m_last_sample_time - time_to_slot(m_last_sample_time));  // add the time for current slot e.g. 13.7-13 = 0.7

	auto time_passed = get_time_seconds() - m_start_time;
	cts.window = std::max( std::min( window_len , time_passed ) , m_slot_size )  ; // window length resulting from size of history but limited by how long ago history was started,
	// also at least slot size (e.g. 1 second) to not be ridiculous
	// window_len e.g. 5.7 because takes into account current slot time

	size_t Epast = 0; // summ of traffic till now
	for (auto sample : m_history) Epast += sample.m_size; 

	const size_t E = Epast;
	const size_t Enow = Epast + packet_size ; // including the data we're about to send now

	const double M = m_target_speed; // max
	const double D1 = (Epast - M*cts.window) / M; // delay - how long to sleep to get back to target speed
	const double D2 = (Enow  - M*cts.window) / M; // delay - how long to sleep to get back to target speed (including current packet)

    cts.delay = (D1*0.80 + D2*0.20); // finall sleep depends on both with/without current packet
	//				update_overheat();
	cts.average = Epast/cts.window; // current avg. speed (for info)

	if (Epast <= 0) {
		if (cts.delay>=0) cts.delay = 0; // no traffic in history so we will not wait
	}

    double Wgood=-1;
    {	// how much data we recommend now to download
        Wgood = the_window_size + 1;
        cts.recomendetDataSize = M*cts.window - E;
    }

	if (dbg) {
		std::ostringstream oss; oss << "["; 	for (auto sample: m_history) oss << sample.m_size << " ";	 oss << "]" << std::ends;
		std::string history_str = oss.str();
		MTRACE((cts.delay > 0 ? "SLEEP" : "")
			<< "dbg " << m_name << ": " 
			<< "speed is A=" << std::setw(8) <<cts.average<<" vs "
			<< "Max=" << std::setw(8) <<M<<" "
			<< " so sleep: "
			<< "D=" << std::setw(8) <<cts.delay<<" sec "
			<< "E="<< std::setw(8) << E << " (Enow="<<std::setw(8)<<Enow<<") "
            << "M=" << std::setw(8) << M <<" W="<< std::setw(8) << cts.window << " "
            << "R=" << std::setw(8) << cts.recomendetDataSize << " Wgood" << std::setw(8) << Wgood << " "
			<< "History: " << std::setw(8) << history_str << " "
			<< "m_last_sample_time=" << std::setw(8) << m_last_sample_time
		);

	}
}

double network_throttle::get_time_seconds() const {
	#if defined(__APPLE__)
	auto point = std::chrono::system_clock::now();
	#else
	auto point = std::chrono::steady_clock::now();
	#endif
	auto time_from_epoh = point.time_since_epoch();
	auto ms = std::chrono::duration_cast< std::chrono::milliseconds >( time_from_epoh ).count();
	double ms_f = ms;
	return ms_f / 1000.;
}

size_t network_throttle::get_recommended_size_of_planned_transport_window(double force_window) const {
	calculate_times_struct cts = { 0, 0, 0, 0};
	network_throttle::calculate_times(0, cts, true, force_window);
	cts.recomendetDataSize += m_network_add_cost;
	if (cts.recomendetDataSize<0) cts.recomendetDataSize=0;
	if (cts.recomendetDataSize>m_network_max_segment) cts.recomendetDataSize=m_network_max_segment;
	size_t RI = (long int)cts.recomendetDataSize;
	return RI;
}

size_t network_throttle::get_recommended_size_of_planned_transport() const {
	size_t R1=0,R2=0,R3=0;
	R1 = get_recommended_size_of_planned_transport_window( -1 );
	R2 = get_recommended_size_of_planned_transport_window(m_window_size / 2);
	R3 = get_recommended_size_of_planned_transport_window( 5              );
	auto RM = std::min(R1, std::min(R2,R3));

	const double a1=20, a2=10, a3=10, am=10; // weight of the various windows in decisssion // TODO 70 => 20
	return (R1*a1 + R2*a2 + R3*a3 + RM*am) / (a1+a2+a3+am);
}

double network_throttle::get_current_speed() const {
	unsigned int bytes_transferred = 0;
	if (m_history.size() == 0 || m_slot_size == 0)
		return 0;
		
	auto it = m_history.begin();
	while (it < m_history.end() - 1)
	{
		bytes_transferred += it->m_size;
		it ++;
	}
	
	return bytes_transferred / ((m_history.size() - 1) * m_slot_size);
}

} // namespace
} // namespace

