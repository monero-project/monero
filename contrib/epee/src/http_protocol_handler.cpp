// Copyright (c) 2026, The Monero Project
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

#include "net/http_protocol_handler.h"

#include <algorithm>
#include <cstdint>

namespace epee { namespace net_utils { namespace http
{
	std::string get_rpc_connection_limit_key(const net_utils::network_address& address)
	{
		if (address.get_type_id() == net_utils::ipv6_network_address::get_type_id())
		{
			const boost::asio::ip::address_v6 ip = address.as<const net_utils::ipv6_network_address>().ip();
			if (net_utils::should_group_ipv6_by_prefix(ip))
				return net_utils::get_ipv6_subnet_address(ip, 64).to_string() + "/64";
		}

		return address.host_str();
	}

	unsigned http_server_config::get_timeout_shift(const net_utils::network_address& address, unsigned shift)
	{
		if (!shift)
			return 0;

		CRITICAL_REGION_LOCAL(m_lock);
		if (m_connection_count >= m_max_connections)
			return shift;

		const bool local = address.is_loopback() || address.is_local();
		const std::size_t limit = local ? m_max_private_ip_connections : m_max_public_ip_connections;
		const auto elem = m_connections.find(get_rpc_connection_limit_key(address));
		const std::uint64_t host_shift =
			(limit && elem != m_connections.end()) ? (std::uint64_t(elem->second) * 8) / limit : 0;

		// Limit timeout relief as this server fills so idle connections release slots.
		const std::uint64_t total_shift = (std::uint64_t(m_connection_count) * 8) / m_max_connections;
		return static_cast<unsigned>(std::min<std::uint64_t>(shift, std::max(host_shift, total_shift)));
	}
}}}
