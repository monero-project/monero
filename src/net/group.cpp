// Copyright (c) 2014-2021, The Monero Project
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

#include "group.h"

namespace net
{
namespace group
{

uint32_t get_group(const epee::net_utils::network_address& address)
{
    if (address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
    {
        const epee::net_utils::network_address na = address;
        const uint32_t actual_ip = na.as<const epee::net_utils::ipv4_network_address>().ip();
        return actual_ip & 0x0000ffff;
    }
    else if (address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
    {
        const epee::net_utils::network_address na = address;
        const boost::asio::ip::address_v6 &actual_ip = na.as<const epee::net_utils::ipv6_network_address>().ip();
        if (actual_ip.is_v4_mapped())
        {
          boost::asio::ip::address_v4 v4ip = make_address_v4_from_v6(actual_ip);
          uint32_t actual_ipv4;
          memcpy(&actual_ipv4, v4ip.to_bytes().data(), sizeof(actual_ipv4));
          return actual_ipv4 & ntohl(0xffff0000);
        }
    }
    return 0;
}

} // group
} // net
