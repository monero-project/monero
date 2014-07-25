
#ifndef INCLUDED_epee_network_throttle_advanced
#define INCLUDED_epee_network_throttle_advanced

#include "../../src/epee/network_throttle.h"

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
		network_throttle(const std::string &name);
		virtual ~network_throttle();
		virtual void set_name(const std::string &name);
		virtual void set_target_speed( network_speed_kbps target );
		
		// add information about events:
		virtual void handle_trafic_exact(size_t packet_size); // count the new traffic/packet; the size is exact considering all network costs
		virtual void handle_trafic_tcp(size_t packet_size); // count the new traffic/packet; the size is as TCP, we will consider MTU etc
		virtual void handle_congestion(double overheat); // call this when congestion is detected; see example use
		virtual void tick(); // poke and update timers/history
		
		// time calculations:
		virtual void calculate_times(size_t packet_size, double &A, double &W, double &D, bool dbg) const; // assuming sending new package (or 0), calculate Average, Window, Delay
		virtual network_time_seconds get_sleep_time(size_t packet_size) const; // gets the D (recommended Delay time) from calc
		virtual network_time_seconds get_sleep_time_after_tick(size_t packet_size); // ditto, but first tick the timer

		static double my_time_seconds(); // a timer

		virtual size_t get_recommended_size_of_planned_transport() const; 
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


