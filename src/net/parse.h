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

#include <boost/asio/ip/tcp.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <optional>

#include "common/expect.h"
#include "net/fwd.h"
#include "net/net_utils_base.h"

namespace net
{
    //! \brief Separates scheme, authority, and path sections of a URI.
    struct scheme_and_authority
    {
        //! \param uri with optional scheme, authority, and optional path. No URNs.
        explicit scheme_and_authority(boost::string_ref uri);

        std::string scheme;
        std::string authority;
    };

    //! \brief Separates the userinfo and host+port from URI authority.
    struct userinfo_and_hostport
    {
        //! \param authority portion of a URI.
        explicit userinfo_and_hostport(boost::string_ref authority);

        std::string userinfo;
        std::string hostport;
    };

    //! \brief Separates the user and pass sections from URI userinfo.
    struct user_and_pass
    {
        user_and_pass()
          : user(), pass()
        {}

        /*!
         * \param userinfo section of a URI.
         * \return User and pass with percent encoding removed. `std::nullopt`
         *    if bad percent encoding
         */
        static std::optional<user_and_pass> get(boost::string_ref userinfo);

        std::string user;
        std::string pass;
    };

    //! \brief Separates scheme, user, pass, and host+port sections of a URI.
    struct uri_components
    {
        uri_components()
          : scheme(), userinfo(), hostport()
        {}

        /*!
         * \param uri with optional scheme, optional user, optional pass,
         *    authority, and optional path. URN not supported.
         * \return Scheme, user, pass, and host+port sections of a URI with
         *    percent encoding removed on user and pass. `std::nullopt` if
         *    bad percent encoding.
         */
        static std::optional<uri_components> get(boost::string_ref uri);

        std::string scheme;
        user_and_pass userinfo;
        std::string hostport;
    };

    /*!
     * \brief Takes a valid address string (IP, Tor, I2P, or DNS name) and splits it into host and port
     *
     * The host of an IPv6 addresses in the format "[x:x:..:x]:port" will have the braces stripped.
     * For example, when the address is "[ffff::2023]", host will be set to "ffff::2023".
     *
     * \param address The address string one wants to split
     * \param[out] host The host part of the address string. Is always set.
     * \param[out] port The port part of the address string. Is only set when address string contains a port.
    */
    void get_network_address_host_and_port(const std::string& address, std::string& host, std::string& port);

    /*!
      Identifies onion, i2p and IPv4 addresses and returns them as a generic
      `network_address`. If the type is unsupported, it might be a hostname,
      and `error() == net::error::kUnsupportedAddress` is returned.

      \param address An onion address, i2p address, ipv4 address or hostname. Hostname
          will return an error.
      \param default_port If `address` does not specify a port, this value
          will be used.

      \return A tor or IPv4 address, else error.
    */
    expect<epee::net_utils::network_address>
        get_network_address(boost::string_ref address, std::uint16_t default_port);

    /*!
      Identifies an IPv4 subnet in CIDR notatioa and returns it as a generic
      `network_address`. If the type is unsupported, it might be a hostname,
      and `error() == net::error::kUnsupportedAddress` is returned.

      \param address An ipv4 address.
      \param allow_implicit_32 whether to accept "raw" IPv4 addresses, with CIDR notation

      \return A tor or IPv4 address, else error.
    */
    expect<epee::net_utils::ipv4_network_subnet>
        get_ipv4_subnet_address(boost::string_ref address, bool allow_implicit_32 = false);

    expect<boost::asio::ip::tcp::endpoint> get_tcp_endpoint(const boost::string_ref address);

    namespace socks
    {
        //! \brief Separates TCP address, user+pass, and socks version
        struct endpoint
        {
            endpoint();
            explicit endpoint(const boost::asio::ip::tcp::endpoint& address);

            //! \param uri with optional scheme, optional userinfo, and host+port.
            static expect<endpoint> get(boost::string_ref uri);

            boost::asio::ip::tcp::endpoint address;
            user_and_pass userinfo;
            version ver;
        };
    }
}

