
#include "../../src/epee/network_throttle-advanced.h"

// network_throttle_manager


namespace epee
{
namespace net_utils
{

i_network_throttle & network_throttle_manager::get_global_throttle_in() { 
	std::call_once(m_once_get_global_throttle_in, [] { m_obj_get_global_throttle_in.reset(new network_throttle); }	);
	return * m_obj_get_global_throttle_in;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_in;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_in;



i_network_throttle & network_throttle_manager::get_global_throttle_inreq() { 
	std::call_once(m_once_get_global_throttle_inreq, [] { m_obj_get_global_throttle_inreq.reset(new network_throttle); }	);
	return * m_obj_get_global_throttle_inreq;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_inreq;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_inreq;


i_network_throttle & network_throttle_manager::get_global_throttle_out() { 
	std::call_once(m_once_get_global_throttle_out, [] { m_obj_get_global_throttle_out.reset(new network_throttle); }	);
	return * m_obj_get_global_throttle_out;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_out;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_out;



} // namespace 
} // namespace 





