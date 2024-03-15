// Copyright (c) 2018-2024, The Monero Project

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

#include "error.h"

#include <string>

namespace
{
    struct net_category : std::error_category
    {
        net_category() noexcept
          : std::error_category()
        {}

        const char* name() const noexcept override
        {
            return "net::error_category";
        }

        std::string message(int value) const override
        {
            switch (net::error(value))
            {
            case net::error::bogus_dnssec:
                return "Invalid response signature from DNSSEC enabled domain";
            case net::error::dns_query_failure:
                return "Failed to retrieve desired DNS record";
            case net::error::expected_tld:
                return "Expected top-level domain";
            case net::error::invalid_host:
                return "Host value is not valid";
            case net::error::invalid_i2p_address:
                return "Invalid I2P address";
            case net::error::invalid_mask:
                return "CIDR netmask outside of 0-32 range";
            case net::error::invalid_port:
                return "Invalid port value (expected 0-65535)";
            case net::error::invalid_tor_address:
                return "Invalid Tor address";
            case net::error::unsupported_address:
                return "Network address not supported";
            default:
                break;
            }

            return "Unknown net::error";
        }

        std::error_condition default_error_condition(int value) const noexcept override
        {
            switch (net::error(value))
            {
            case net::error::invalid_port:
            case net::error::invalid_mask:
                return std::errc::result_out_of_range;
            case net::error::expected_tld:
            case net::error::invalid_tor_address:
            default:
                break;
            }
            return std::error_condition{value, *this};
        }
    };
} // anonymous

namespace net
{
    std::error_category const& error_category() noexcept
    {
        static const net_category instance{};
        return instance;
    }
}
