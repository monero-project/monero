
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


} // namespace 
} // namespace 





