// Copyright (c) 2019-2024, The Monero Project
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
        /**
         * Naming rules defined here:
         * https://i2p.net/en/docs/overview/naming/#naming-rules
         */
        constexpr const char tld_b32[] = u8".b32.i2p";
        constexpr const char tld_i2p[] = u8".i2p";
        constexpr const char unknown_host[] = "<unknown i2p host>";

        //! Lengths do not include TLD
        constexpr const unsigned b32_length = 52;
        constexpr const unsigned i2p_max_length = 63;

        constexpr const char base32_alphabet[] =
            u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz234567";

        //! Validate a Base32 address (.b32.i2p)
        expect<void> host_check_b32(boost::string_ref host) noexcept
        {
            if (!host.ends_with(tld_b32))
                return {net::error::expected_tld};

            host.remove_suffix(sizeof(tld_b32) - 1);

            if (host.size() != b32_length)
                return {net::error::invalid_i2p_address};
            if (host.find_first_not_of(base32_alphabet) != boost::string_ref::npos)
                return {net::error::invalid_i2p_address};

            return success();
        }

        //! Validate a human-readable hostname (.i2p)
        expect<void> host_check_human(boost::string_ref host) noexcept
        {
            if (!host.ends_with(tld_i2p))
                return {net::error::expected_tld};

            if (host.size() - (sizeof(tld_i2p) - 1) > i2p_max_length)
                return {net::error::invalid_i2p_address};

            //! Reject addresses starting with a dot or hyphen
            if (host.empty() || host.front() == '.' || host.front() == '-')
                return {net::error::invalid_i2p_address};

            //! Reject addresses containing invalid characters
            for (char c : host)
            {
                if (!(std::islower(c) || std::isdigit(c) || c == '.' || c == '-'))
                    return {net::error::invalid_i2p_address};
            }

            //! Reject addresses with two dots in a row, a dot followed by a hyphen, or vice versa
            if (host.find("..") != boost::string_ref::npos ||
                host.find(".-") != boost::string_ref::npos ||
                host.find("-.") != boost::string_ref::npos)
                return {net::error::invalid_i2p_address};

            //! Reject Base32 addresses 'disguised' as human-readable ones
            if (host.ends_with(tld_b32))
                return {net::error::invalid_i2p_address};

            return success();
        }

        //! Use appropriate function for address validation
        expect<void> host_check(boost::string_ref host) noexcept
        {
            if (host.ends_with(tld_b32))
            {
                return host_check_b32(host);
            }
            else if (host.ends_with(tld_i2p))
            {
                return host_check_human(host);
            }

            return {net::error::expected_tld};
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

        static_assert(sizeof(i2p_address::host_) > b32_length &&
                      sizeof(i2p_address::host_) > i2p_max_length);

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
