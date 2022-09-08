// Copyright (c) 2022, The Monero Project
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

#include <boost/asio/ip/address_v6.hpp>

#include "../model/deserializer.h"
#include "../model/serializer.h"
#include "../model/visitor.h"

namespace serde::internal
{
    constexpr size_t BOOST_IPV6_ADDR_SIZE = sizeof(boost::asio::ip::address_v6::bytes_type);
    static_assert(BOOST_IPV6_ADDR_SIZE == 16);

    struct BoostIpv6AddressVisitor: public model::RefVisitor<boost::asio::ip::address_v6>
    {
        BoostIpv6AddressVisitor(boost::asio::ip::address_v6& addr_ref):
            model::RefVisitor<boost::asio::ip::address_v6>(addr_ref)
        {}

        std::string expecting() const noexcept override final
        {
            return "IPv6 address bytes";
        }

        void visit_bytes(const const_byte_span& ip_bytes) override final
        {
            const size_t ip_size = ip_bytes.size();
            CHECK_AND_ASSERT_THROW_MES
            (
                ip_size == BOOST_IPV6_ADDR_SIZE,
                "ipv6 addr wrong size " << ip_size
            );
            boost::asio::ip::address_v6::bytes_type ipv6bytes;
            memcpy(&ipv6bytes[0], ip_bytes.begin(), BOOST_IPV6_ADDR_SIZE);
            this->visit(boost::asio::ip::address_v6(ipv6bytes));
        }
    };
} // namesapce serde::internal

namespace serde::model
{
    inline void serialize_default(const boost::asio::ip::address_v6& value, Serializer& serializer)
    {
        boost::asio::ip::address_v6::bytes_type ip_bytes = value.to_bytes();
        serializer.serialize_bytes({ip_bytes.begin(), ip_bytes.size()});
    }

    inline bool deserialize_default(Deserializer& deserializer, boost::asio::ip::address_v6& value)
    {
        internal::BoostIpv6AddressVisitor addr_visitor(value);
        deserializer.deserialize_bytes(addr_visitor);
        return addr_visitor.was_visited();
    }
} // namespace serde::model
