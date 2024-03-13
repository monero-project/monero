// Copyright (c) 2019-2023, The Monero Project
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

#include "i2p_address.h"

#include <algorithm>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <cassert>
#include <cstring>
#include <limits>

#include "net/error.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage.h"
#include "string_tools_lexical.h"

namespace net
{
    namespace
    {
        // !TODO only b32 addresses right now
        constexpr const char tld[] = u8".b32.i2p";
        constexpr const char unknown_host[] = "<unknown i2p host>";

        constexpr const unsigned b32_length = 52;

        constexpr const char base32_alphabet[] =
            u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz234567";

        expect<void> host_check(boost::string_ref host) noexcept
        {
            if (!host.ends_with(tld))
                return {net::error::expected_tld};

            host.remove_suffix(sizeof(tld) - 1);

            if (host.size() != b32_length)
                return {net::error::invalid_i2p_address};
            if (host.find_first_not_of(base32_alphabet) != boost::string_ref::npos)
                return {net::error::invalid_i2p_address};

            return success();
        }

        struct i2p_serialized
        {
            std::string host;
            std::uint16_t port; //! Leave for compatability with older clients

            BEGIN_KV_SERIALIZE_MAP()
                KV_SERIALIZE(host)
                KV_SERIALIZE(port)
            END_KV_SERIALIZE_MAP()
        };
    }

    i2p_address::i2p_address(const boost::string_ref host) noexcept
    {
        // this is a private constructor, throw if moved to public
        assert(host.size() < sizeof(host_));

        const std::size_t length = std::min(sizeof(host_) - 1, host.size());
        std::memcpy(host_, host.data(), length);
        std::memset(host_ + length, 0, sizeof(host_) - length);
    }

    const char* i2p_address::unknown_str() noexcept
    {
        return unknown_host;
    }

    i2p_address::i2p_address() noexcept
    {
        static_assert(sizeof(unknown_host) <= sizeof(host_), "bad buffer size");
        std::memcpy(host_, unknown_host, sizeof(unknown_host));
        std::memset(host_ + sizeof(unknown_host), 0, sizeof(host_) - sizeof(unknown_host));
    }

    expect<i2p_address> i2p_address::make(const boost::string_ref address)
    {
        boost::string_ref host = address.substr(0, address.rfind(':'));
        MONERO_CHECK(host_check(host));

        static_assert(b32_length + sizeof(tld) == sizeof(i2p_address::host_), "bad internal host size");
        return i2p_address{host};
    }

    bool i2p_address::_load(epee::serialization::portable_storage& src, epee::serialization::section* hparent)
    {
        i2p_serialized in{};
        if (in._load(src, hparent) && in.host.size() < sizeof(host_) && (in.host == unknown_host || !host_check(in.host).has_error()))
        {
            std::memcpy(host_, in.host.data(), in.host.size());
            std::memset(host_ + in.host.size(), 0, sizeof(host_) - in.host.size());
            return true;
        }
        static_assert(sizeof(unknown_host) <= sizeof(host_), "bad buffer size");
        std::memcpy(host_, unknown_host, sizeof(unknown_host)); // include null terminator
        return false;
    }

    bool i2p_address::store(epee::serialization::portable_storage& dest, epee::serialization::section* hparent) const
    {
        // Set port to 1 for backwards compatability; zero is invalid port
        const i2p_serialized out{std::string{host_}, 1};
        return out.store(dest, hparent);
    }

    i2p_address::i2p_address(const i2p_address& rhs) noexcept
    {
        std::memcpy(host_, rhs.host_, sizeof(host_));
    }

    i2p_address& i2p_address::operator=(const i2p_address& rhs) noexcept
    {
        if (this != std::addressof(rhs))
        {
            std::memcpy(host_, rhs.host_, sizeof(host_));
        }
        return *this;
    }

    bool i2p_address::is_unknown() const noexcept
    {
        static_assert(1 <= sizeof(host_), "host size too small");
        return host_[0] == '<'; // character is not allowed otherwise
    }

    bool i2p_address::equal(const i2p_address& rhs) const noexcept
    {
        return is_same_host(rhs);
    }

    bool i2p_address::less(const i2p_address& rhs) const noexcept
    {
        return std::strcmp(host_str(), rhs.host_str()) < 0;
    }

    bool i2p_address::is_same_host(const i2p_address& rhs) const noexcept
    {
        return std::strcmp(host_str(), rhs.host_str()) == 0;
    }

    std::string i2p_address::str() const
    {
        return host_str();
    }
}
