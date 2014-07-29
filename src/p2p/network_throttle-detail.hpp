
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
// 

/* rfree: throttle details, implementing rate limiting */


#ifndef INCLUDED_src_p2p_throttle_detail_hpp
#define INCLUDED_src_p2p_throttle_detail_hpp

#include "../../src/p2p/network_throttle.hpp"

namespace epee
{
namespace net_utils
{


class network_throttle : public i_network_throttle {
	private:
		struct packet_info {
			size_t m_size; // octets sent. Summary for given small-window (e.g. for all packaged in 1 second)
			packet_info();
		};


		network_speed_kbps m_target_speed;
		size_t m_network_add_cost; // estimated add cost of headers 
		size_t m_network_minimal_segment; // estimated minimal cost of sending 1 byte to round up to
		size_t m_network_max_segment; // recommended max size of 1 TCP transmission

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
		network_throttle(const std::string &name, int window_size=-1);
		virtual ~network_throttle();
		virtual void set_name(const std::string &name);
		virtual void set_target_speed( network_speed_kbps target );
		
		// add information about events:
		virtual void handle_trafic_exact(size_t packet_size); // count the new traffic/packet; the size is exact considering all network costs
		virtual void handle_trafic_tcp(size_t packet_size); // count the new traffic/packet; the size is as TCP, we will consider MTU etc
		virtual void handle_congestion(double overheat); // call this when congestion is detected; see example use
		virtual void tick(); // poke and update timers/history
		

		virtual double get_time_seconds() const ; // a timer
		virtual void set_overheat();
		virtual void update_overheat(double lag);

		// time calculations:
		virtual void calculate_times(size_t packet_size, double &A, double &W, double &D, double &R, bool dbg, double force_window) const; // (see base class for info)
		virtual network_time_seconds get_sleep_time(size_t packet_size) const; // gets the D (recommended Delay time) from calc
		virtual network_time_seconds get_sleep_time_after_tick(size_t packet_size); // ditto, but first tick the timer

		virtual size_t get_recommended_size_of_planned_transport() const; 
		virtual size_t get_recommended_size_of_planned_transport_window(double force_window) const; 
		//virtual void add_planned_transport(size_t size);
	private:
		virtual network_time_seconds time_to_slot(network_time_seconds t) const { return std::floor( t ); } // convert exact time eg 13.7 to rounded time for slot number in history 13
		virtual void _handle_trafic_exact(size_t packet_size, size_t orginal_size); 
};


struct network_throttle_bw {
	public:
		network_throttle m_in;
		network_throttle m_inreq;
		network_throttle m_out;

	public:
		network_throttle_bw(const std::string &name1);
};



} // namespace net_utils
} // namespace epee


#endif


