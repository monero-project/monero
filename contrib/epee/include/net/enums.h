// Copyright (c) 2018-2022, The Monero Project

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

#pragma once

#include <boost/utility/string_ref.hpp>
#include <cstdint>

namespace epee
{
namespace net_utils
{
	enum class address_type : std::uint8_t
	{
		// Do not change values, this will break serialization
		invalid = 0,
		ipv4 = 1,
		ipv6 = 2,
		i2p = 3,
		tor = 4
	};

	enum class zone : std::uint8_t
	{
		invalid = 0,
		public_ = 1, // public is keyword
		i2p = 2,     // order from here changes priority of selection for origin TXes
		tor = 3
	};

	// implementations in src/net_utils_base.cpp

	//! \return String name of zone or "invalid" on error.
	const char* zone_to_string(zone value) noexcept;

	//! \return `zone` enum of `value` or `zone::invalid` on error.
	zone zone_from_string(boost::string_ref value) noexcept;
} // net_utils
} // epee

namespace std
{
	template<> struct hash<epee::net_utils::zone>
	{
		std::size_t operator()(const epee::net_utils::zone _z) const
		{
			return static_cast<std::size_t>(_z);
		}
	};
} // std
