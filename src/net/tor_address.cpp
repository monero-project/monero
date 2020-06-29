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

#include "tor_address.h"

#include <algorithm>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <cassert>
#include <cstring>
#include <limits>

#include "net/error.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage.h"
#include "string_tools.h"

namespace net
{
    namespace
    {
        constexpr const char tld[] = u8".onion";
        constexpr const char unknown_host[] = "<unknown tor host>";

        constexpr const unsigned v2_length = 16;
        constexpr const unsigned v3_length = 56;

        constexpr const char base32_alphabet[] =
            u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz234567";

        expect<void> host_check(boost::string_ref host) noexcept
        {
            if (!host.ends_with(tld))
                return {net::error::expected_tld};

            host.remove_suffix(sizeof(tld) - 1);

            //! \TODO v3 has checksum, base32 decoding is required to verify it
            if (host.size() != v2_length && host.size() != v3_length)
                return {net::error::invalid_tor_address};
            if (host.find_first_not_of(base32_alphabet) != boost::string_ref::npos)
                return {net::error::invalid_tor_address};

            return success();
        }

        struct tor_serialized
        {
            std::string host;
            std::uint16_t port;

            BEGIN_KV_SERIALIZE_MAP()
                KV_SERIALIZE(host)
                KV_SERIALIZE(port)
            END_KV_SERIALIZE_MAP()
        };
    }

    tor_address::tor_address(const boost::string_ref host, const std::uint16_t port) noexcept
      : port_(port)
    {
        // this is a private constructor, throw if moved to public
        assert(host.size() < sizeof(host_));

        const std::size_t length = std::min(sizeof(host_) - 1, host.size());
        std::memcpy(host_, host.data(), length);
        std::memset(host_ + length, 0, sizeof(host_) - length);
    }

    const char* tor_address::unknown_str() noexcept
    {
        return unknown_host;
    }

    tor_address::tor_address() noexcept
      : port_(0)
    {
        static_assert(sizeof(unknown_host) <= sizeof(host_), "bad buffer size");
        std::memcpy(host_, unknown_host, sizeof(unknown_host));
        std::memset(host_ + sizeof(unknown_host), 0, sizeof(host_) - sizeof(unknown_host));
    }

    expect<tor_address> tor_address::make(const boost::string_ref address, const std::uint16_t default_port)
    {
        boost::string_ref host = address.substr(0, address.rfind(':'));
        const boost::string_ref port =
            address.substr(host.size() + (host.size() == address.size() ? 0 : 1));

        MONERO_CHECK(host_check(host));

        std::uint16_t porti = default_port;
        if (!port.empty() && !epee::string_tools::get_xtype_from_string(porti, std::string{port}))
            return {net::error::invalid_port};

        static_assert(v2_length <= v3_length, "bad internal host size");
        static_assert(v3_length + sizeof(tld) == sizeof(tor_address::host_), "bad internal host size");
        return tor_address{host, porti};
    }

    bool tor_address::_load(epee::serialization::portable_storage& src, epee::serialization::section* hparent)
    {
        tor_serialized in{};
        if (in._load(src, hparent) && in.host.size() < sizeof(host_) && (in.host == unknown_host || !host_check(in.host).has_error()))
        {
            std::memcpy(host_, in.host.data(), in.host.size());
            std::memset(host_ + in.host.size(), 0, sizeof(host_) - in.host.size());
            port_ = in.port;
            return true;
        }
        static_assert(sizeof(unknown_host) <= sizeof(host_), "bad buffer size");
        std::memcpy(host_, unknown_host, sizeof(unknown_host)); // include null terminator
        port_ = 0;
        return false;
    }

    bool tor_address::store(epee::serialization::portable_storage& dest, epee::serialization::section* hparent) const
    {
        const tor_serialized out{std::string{host_}, port_};
        return out.store(dest, hparent);
    }

    tor_address::tor_address(const tor_address& rhs) noexcept
      : port_(rhs.port_)
    {
        std::memcpy(host_, rhs.host_, sizeof(host_));
    }

    tor_address& tor_address::operator=(const tor_address& rhs) noexcept
    {
        if (this != std::addressof(rhs))
        {
            port_ = rhs.port_;
            std::memcpy(host_, rhs.host_, sizeof(host_));
        }
        return *this;
    }

    bool tor_address::is_unknown() const noexcept
    {
        static_assert(1 <= sizeof(host_), "host size too small");
        return host_[0] == '<'; // character is not allowed otherwise
    }

    bool tor_address::equal(const tor_address& rhs) const noexcept
    {
        return port_ == rhs.port_ && is_same_host(rhs);
    }

    bool tor_address::less(const tor_address& rhs) const noexcept
    {
        int res = std::strcmp(host_str(), rhs.host_str());
        return res < 0 || (res == 0 && port() < rhs.port());
    }

    bool tor_address::is_same_host(const tor_address& rhs) const noexcept
    {
        //! \TODO v2 and v3 should be comparable - requires base32
        return std::strcmp(host_str(), rhs.host_str()) == 0;
    }

    std::string tor_address::str() const
    {
        const std::size_t host_length = std::strlen(host_str());
        const std::size_t port_length =
            port_ == 0 ? 0 : std::numeric_limits<std::uint16_t>::digits10 + 2;

        std::string out{};
        out.reserve(host_length + port_length);
        out.assign(host_str(), host_length);

        if (port_ != 0)
        {
            out.push_back(':');
            namespace karma = boost::spirit::karma;
            karma::generate(std::back_inserter(out), karma::ushort_, port());
        }
        return out;
    }
}
