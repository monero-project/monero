// Copyright (c) 2018, The Monero Project
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

#include "parse.h"

#include "net/tor_address.h"
#include "net/i2p_address.h"
#include "string_tools.h"

namespace net
{
    expect<epee::net_utils::network_address>
    get_network_address(const boost::string_ref address, const std::uint16_t default_port)
    {
        const boost::string_ref host = address.substr(0, address.rfind(':'));

        if (host.empty())
            return make_error_code(net::error::invalid_host);
        if (host.ends_with(".onion"))
            return tor_address::make(address, default_port);
        if (host.ends_with(".i2p"))
            return i2p_address::make(address, default_port);

        std::uint16_t port = default_port;
        if (host.size() < address.size())
        {
            if (!epee::string_tools::get_xtype_from_string(port, std::string{address.substr(host.size() + 1)}))
                return make_error_code(net::error::invalid_port);
        }

        std::uint32_t ip = 0;
        if (epee::string_tools::get_ip_int32_from_string(ip, std::string{host}))
            return {epee::net_utils::ipv4_network_address{ip, port}};
        return make_error_code(net::error::unsupported_address);
    }
}
