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

#include <system_error>
#include <type_traits>

namespace net
{
    //! General net errors
    enum class error : int
    {
        // 0 reserved for success (as per expect<T>)
        bogus_dnssec = 1,   //!< Invalid response signature from DNSSEC enabled domain
        dns_query_failure,  //!< Failed to retrieve desired DNS record
        expected_tld,       //!< Expected a tld
        invalid_host,       //!< Hostname is not valid
        invalid_i2p_address,
        invalid_mask,       //!< Outside of 0-32 range
        invalid_port,       //!< Outside of 0-65535 range
        invalid_tor_address,//!< Invalid base32 or length
        unsupported_address,//!< Type not supported by `get_network_address`

    };

    //! \return `std::error_category` for `net` namespace.
    std::error_category const& error_category() noexcept;

    //! \return `net::error` as a `std::error_code` value.
    inline std::error_code make_error_code(error value) noexcept
    {
        return std::error_code{int(value), error_category()};
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<::net::error>
      : true_type
    {};
}
