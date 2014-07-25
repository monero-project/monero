
// src/epee/network_throttle.h


#ifndef INCLUDED_epee_network_throttle
#define INCLUDED_epee_network_throttle

namespace epee
{
namespace net_utils
{

// just typedefs to in code define the units used. TODO later it will be enforced that casts to other numericals are only explicit to avoid mistakes? use boost::chrono?
typedef double network_speed_kbps;
typedef double network_time_seconds;

class i_network_throttle;

class network_throttle_manager {
	public:
		static i_network_throttle & get_global_throttle_in(); // singleton
		static i_network_throttle & get_global_throttle_in_req(); // singleton
		static i_network_throttle & get_global_throttle_out(); // singleton
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
		virtual void calculate_times(size_t packet_size, double &A, double &W, double &D, bool dbg) const =0; // assuming sending new package (or 0), calculate Average, Window, Delay
		virtual network_time_seconds get_sleep_time(size_t packet_size) const =0; // gets the D (recommended Delay time) from calc
		virtual network_time_seconds get_sleep_time_after_tick(size_t packet_size) =0; // ditto, but first tick the timer

		virtual size_t get_recommended_size_of_planned_transport() const =0; // what should be the recommended limit of data size that we can transport over current network_throttle in near future
};


// ... more in the -advanced.h file


} // namespace net_utils
} // namespace epee


#endif



