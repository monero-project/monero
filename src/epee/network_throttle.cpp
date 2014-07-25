
#include "../../src/epee/network_throttle-advanced.h"

// network_throttle_manager


namespace epee
{
namespace net_utils
{

// static:
critical_section network_throttle_manager::m_lock_get_global_throttle_in;
critical_section network_throttle_manager::m_lock_get_global_throttle_inreq;
critical_section network_throttle_manager::m_lock_get_global_throttle_out;

int network_throttle_manager::xxx;


// methods:
i_network_throttle & network_throttle_manager::get_global_throttle_in() { 

	std::call_once(m_once_get_global_throttle_in, [] { m_obj_get_global_throttle_in.reset(new network_throttle("<<<<<< global-IN-DOWNLOAD",60)); }	);
	return * m_obj_get_global_throttle_in;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_in;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_in;



i_network_throttle & network_throttle_manager::get_global_throttle_inreq() { 
	std::call_once(m_once_get_global_throttle_inreq, [] { m_obj_get_global_throttle_inreq.reset(new network_throttle("<<<====== global-IN-DOWNLOAD-REQUESTED",40)); }	);
	return * m_obj_get_global_throttle_inreq;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_inreq;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_inreq;


i_network_throttle & network_throttle_manager::get_global_throttle_out() { 
	std::call_once(m_once_get_global_throttle_out, [] { m_obj_get_global_throttle_out.reset(new network_throttle(">>>>>>>>> global-OUT",20)); }	);
	return * m_obj_get_global_throttle_out;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_out;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_out;




network_throttle_bw::network_throttle_bw(const std::string &name1) 
	: m_in(name1+"-DOWNLOAD"), m_inreq(name1+"-DOWNLOAD-REQUESTS"), m_out(name1+"-UPLOAD")
{ }




} // namespace 
} // namespace 





