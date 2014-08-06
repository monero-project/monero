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

/* rfree: throttle basic interface and manager */

#include "../../src/p2p/network_throttle-detail.hpp"

namespace epee
{
namespace net_utils
{

// ================================================================================================
// network_throttle_manager
// ================================================================================================

// ================================================================================================
// static:
critical_section network_throttle_manager::m_lock_get_global_throttle_in;
critical_section network_throttle_manager::m_lock_get_global_throttle_inreq;
critical_section network_throttle_manager::m_lock_get_global_throttle_out;

int network_throttle_manager::xxx;


// ================================================================================================
// methods:
i_network_throttle & network_throttle_manager::get_global_throttle_in() { 

	std::call_once(m_once_get_global_throttle_in, [] { m_obj_get_global_throttle_in.reset(new network_throttle("in/all","<<< global-IN",60)); }	);
	return * m_obj_get_global_throttle_in;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_in;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_in;



i_network_throttle & network_throttle_manager::get_global_throttle_inreq() { 
	std::call_once(m_once_get_global_throttle_inreq, [] { m_obj_get_global_throttle_inreq.reset(new network_throttle("inreq/all", "<== global-IN-REQ",40)); }	);
	return * m_obj_get_global_throttle_inreq;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_inreq;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_inreq;


i_network_throttle & network_throttle_manager::get_global_throttle_out() { 
	std::call_once(m_once_get_global_throttle_out, [] { m_obj_get_global_throttle_out.reset(new network_throttle("out/all", ">>> global-OUT",20)); }	);
	return * m_obj_get_global_throttle_out;
}
std::once_flag network_throttle_manager::m_once_get_global_throttle_out;
std::unique_ptr<i_network_throttle> network_throttle_manager::m_obj_get_global_throttle_out;




network_throttle_bw::network_throttle_bw(const std::string &name1) 
	: m_in("in/"+name1, name1+"-DOWNLOAD"), m_inreq("inreq/"+name1, name1+"-DOWNLOAD-REQUESTS"), m_out("out/"+name1, name1+"-UPLOAD")
{ }




} // namespace 
} // namespace 





