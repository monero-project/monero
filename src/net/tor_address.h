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

#pragma once

#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <string>

#include "common/expect.h"
#include "net/enums.h"
#include "net/error.h"

namespace epee
{
namespace serialization
{
    class portable_storage;
    struct section;
}
}

namespace net
{
    //! Tor onion address; internal format not condensed/decoded.
    class tor_address
    {
        std::uint16_t port_;
        char host_[63]; // null-terminated

        //! Keep in private, `host.size()` has no runtime check
        tor_address(boost::string_ref host, std::uint16_t port) noexcept;

    public:
        //! \return Size of internal buffer for host.
        static constexpr std::size_t buffer_size() noexcept { return sizeof(host_); }

        //! \return `<unknown tor host>`.
        static const char* unknown_str() noexcept;

        //! An object with `port() == 0` and `host_str() == unknown_str()`.
        tor_address() noexcept;

        //! \return A default constructed `tor_address` object.
        static tor_address unknown() noexcept { return tor_address{}; }

        /*!
            Parse `address` in onion v3 format with (i.e. x.onion:80)
            with `default_port` being used iff port is not specified in
            `address`.
        */
        static expect<tor_address> make(boost::string_ref address, std::uint16_t default_port = 0);

        //! Load from epee p2p format, and \return false if not valid tor address
        bool _load(epee::serialization::portable_storage& src, epee::serialization::section* hparent);

        //! Store in epee p2p format
        bool store(epee::serialization::portable_storage& dest, epee::serialization::section* hparent) const;

        // Moves and  copies are currently identical

        tor_address(const tor_address& rhs) noexcept;
        ~tor_address() = default;
        tor_address& operator=(const tor_address& rhs) noexcept;

        //! \return True if default constructed or via `unknown()`.
        bool is_unknown() const noexcept;

        bool equal(const tor_address& rhs) const noexcept;
        bool less(const tor_address& rhs) const noexcept;

        //! \return True if onion addresses are identical.
        bool is_same_host(const tor_address& rhs) const noexcept;

        //! \return `x.onion` or `x.onion:z` if `port() != 0`.
        std::string str() const;

        //! \return Null-terminated `x.onion` value or `unknown_str()`.
        const char* host_str() const noexcept { return host_; }

        //! \return Port value or `0` if unspecified.
        std::uint16_t port() const noexcept { return port_; }

        static constexpr bool is_loopback() noexcept { return false; }
        static constexpr bool is_local() noexcept { return false; }

        static constexpr epee::net_utils::address_type get_type_id() noexcept
        {
            return epee::net_utils::address_type::tor;
        }

        static constexpr epee::net_utils::zone get_zone() noexcept
        {
            return epee::net_utils::zone::tor;
        }

        //! \return `!is_unknown()`.
        bool is_blockable() const noexcept { return !is_unknown(); }
    };

    inline bool operator==(const tor_address& lhs, const tor_address& rhs) noexcept
    {
        return lhs.equal(rhs);
    }

    inline bool operator!=(const tor_address& lhs, const tor_address& rhs) noexcept
    {
        return !lhs.equal(rhs);
    }

    inline bool operator<(const tor_address& lhs, const tor_address& rhs) noexcept
    {
        return lhs.less(rhs);
    }
} // net
