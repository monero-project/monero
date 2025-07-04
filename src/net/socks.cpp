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

#include "socks.h"

#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <cstring>
#include <limits>
#include <numeric>
#include <string>

#include "net/parse.h"
#include "net/net_utils_base.h"
#include "net/tor_address.h"
#include "net/i2p_address.h"

namespace net
{
namespace socks
{
    namespace
    {
        constexpr const std::uint8_t v4_connect_command = 1;
        constexpr const std::uint8_t v4tor_resolve_command = 0xf0;
        constexpr const std::uint8_t v4_request_granted = 90;

        constexpr const std::uint8_t v5_noauth_method = 0;
        constexpr const std::uint8_t v5_userpass_method = 2;
        constexpr const std::uint8_t v5_connect_command = 1;
        constexpr const std::uint8_t v5_reserved = 0;
        constexpr const std::uint8_t v5_ipv4_type = 1;
        constexpr const std::uint8_t v5_domain_type = 3;
        constexpr const std::uint8_t v5_ipv6_type = 4;
        constexpr const std::uint8_t v5_reply_success = 0;
        constexpr const std::uint8_t v5_userpass_version = 1;

        struct v4_header
        {
            std::uint8_t version;
            std::uint8_t command_code;
            boost::endian::big_uint16_t port;
            boost::endian::big_uint32_t ip;
        };

        struct v5_noauth_initial
        {
            std::uint8_t version;
            std::uint8_t n_methods;
            std::uint8_t method;

            static constexpr v5_noauth_initial make() noexcept
            {
                return {5, 1, v5_noauth_method};
            }
        };

        struct v5_auth_initial
        {
            std::uint8_t version;
            std::uint8_t n_methods;
            std::uint8_t method1;
            std::uint8_t method2;

            static constexpr v5_auth_initial make() noexcept
            {
                return {5, 2, v5_noauth_method, v5_userpass_method};
            }
        };

        struct v5_response_initial
        {
            std::uint8_t version;
            std::uint8_t method;
        };

        struct v5_ipv4_connect
        {
            std::uint8_t version;
            std::uint8_t command;
            std::uint8_t reserved;
            std::uint8_t type;
            boost::endian::big_uint32_t ip;
            boost::endian::big_uint16_t port;

            static v5_ipv4_connect make(const std::uint32_t ip, const std::uint16_t port) noexcept
            {
                return {5, v5_connect_command, v5_reserved, v5_ipv4_type, ip, port};
            }
        };

        struct v5_domain_connect
        {
            std::uint8_t version;
            std::uint8_t command;
            std::uint8_t reserved;
            std::uint8_t type;
            std::uint8_t length;

            static constexpr v5_domain_connect make(const std::uint8_t length) noexcept
            {
                return {5, v5_connect_command, v5_reserved, v5_domain_type, length};
            }
        };

        struct v5_ipv6_connect
        {
            std::uint8_t version;
            std::uint8_t command;
            std::uint8_t reserved;
            std::uint8_t type;
            char ip[16];
            boost::endian::big_uint16_t port;

            static v5_ipv6_connect make(const boost::asio::ip::address_v6& ip, const std::uint16_t port)
            {
                v5_ipv6_connect out{5, v5_connect_command, v5_reserved, v5_ipv6_type};
                out.port = port;

                const auto ip_bytes = ip.to_bytes();
                static_assert(sizeof(out.ip) == sizeof(ip_bytes), "unexpected ipv6 bytes size");
                std::memcpy(std::addressof(out.ip), std::addressof(ip_bytes), sizeof(out.ip));

                return out;
            }
        };

        struct v5_response_auth
        {
            std::uint8_t version;
            std::uint8_t status;
        };

        struct v5_response_connect
        {
            std::uint8_t version;
            std::uint8_t reply;
            std::uint8_t reserved;
            std::uint8_t type;
        };

        struct v5_response_ipv4
        {
            boost::endian::big_uint32_t ip;
            boost::endian::big_uint16_t port;
        };

        struct v5_response_ipv6
        {
            char ip[16];
            boost::endian::big_uint16_t port;
        };

        std::size_t write_domain_header(epee::span<std::uint8_t> out, const std::uint8_t command, const std::uint16_t port, const boost::string_ref domain)
        {
            if (std::numeric_limits<std::size_t>::max() - sizeof(v4_header) - 2 < domain.size())
                return 0;

            const std::size_t buf_size = sizeof(v4_header) + domain.size() + 2;
            if (out.size() < buf_size)
                return 0;

            // version 4, 1 indicates invalid ip for domain extension
            const v4_header temp{4, command, port, std::uint32_t(1)};
            std::memcpy(out.data(), std::addressof(temp), sizeof(temp));
            out.remove_prefix(sizeof(temp));

            *(out.data()) = 0;
            out.remove_prefix(1);

            std::memcpy(out.data(), domain.data(), domain.size());
            out.remove_prefix(domain.size());

            *(out.data()) = 0;
            return buf_size;
        }

        std::size_t write_v5_userpass(epee::span<std::uint8_t> out, const user_and_pass& userinfo)
        {
            static constexpr const std::uint8_t max_length = std::numeric_limits<std::uint8_t>::max();
            if (max_length < userinfo.user.size())
                return 0;
            if (max_length < userinfo.pass.size())
                return 0;

            static_assert(max_length < std::numeric_limits<std::size_t>::max());
            static_assert(max_length < std::numeric_limits<std::size_t>::max() - max_length);
            static_assert(3 <= std::numeric_limits<std::size_t>::max() - max_length - max_length);

            if (out.size() < 3 + userinfo.user.size() + userinfo.pass.size())
                return 0;

            const std::size_t initial = out.size();

            out[0] = v5_userpass_version;
            out[1] = std::uint8_t(userinfo.user.size());
            out.remove_prefix(2);

            std::memcpy(out.data(), userinfo.user.data(), userinfo.user.size());
            out.remove_prefix(userinfo.user.size());

            out[0] = std::uint8_t(userinfo.pass.size());
            out.remove_prefix(1);

            std::memcpy(out.data(), userinfo.pass.data(), userinfo.pass.size());
            out.remove_prefix(userinfo.pass.size());
            return initial - out.size();
        }

        std::array<std::uint16_t, 2> write_v5_initial(epee::span<std::uint8_t> out, const user_and_pass* userinfo)
        {
            std::array<std::uint16_t, 2> sizes{{}};

            if (userinfo && (!userinfo->user.empty() || !userinfo->pass.empty()))
            {
                const auto header = v5_auth_initial::make();
                if (out.size() < sizeof(header))
                    return sizes;
                std::memcpy(out.data(), std::addressof(header), sizeof(header));
                out.remove_prefix(sizeof(header));

                const std::size_t auth = write_v5_userpass(out, *userinfo);
                if (!auth)
                    return sizes;
                out.remove_prefix(auth);

                std::get<0>(sizes) = sizeof(header);
                std::get<1>(sizes) = auth;
            }
            else
            {
                const auto header = v5_noauth_initial::make();
                if (out.size() < sizeof(header))
                    return sizes;
                std::memcpy(out.data(), std::addressof(header), sizeof(header));
                out.remove_prefix(sizeof(header));

                std::get<0>(sizes) = sizeof(header);
            }

            return sizes;
        }

        template<typename T>
        std::array<std::uint16_t, 3> write_v5_address_connect(epee::span<std::uint8_t> out, const T& address, const user_and_pass* userinfo)
        {
            std::array<std::uint16_t, 3> sizes{{}};

            const auto result = write_v5_initial(out, userinfo);
            if (!std::get<0>(result))
                return sizes;

            for (std::size_t length : result)
                out.remove_prefix(length);

            if (out.size() < sizeof(address))
                return sizes;
            std::memcpy(out.data(), std::addressof(address), sizeof(address));

            std::get<0>(sizes) = std::get<0>(result);
            std::get<1>(sizes) = std::get<1>(result);
            std::get<2>(sizes) = sizeof(address);
            return sizes;
        }

        std::array<std::uint16_t, 3> write_v5_domain_connect(epee::span<std::uint8_t> out, const std::uint16_t port, const boost::string_ref domain, const user_and_pass* userinfo)
        {
            std::array<std::uint16_t, 3> sizes{{}};
            if (std::numeric_limits<std::uint8_t>::max() < domain.size())
                return sizes;

            const auto result = write_v5_initial(out, userinfo);
            if (!std::get<0>(result))
                return sizes;

            for (std::size_t length : result)
                out.remove_prefix(length);

            const auto request = v5_domain_connect::make(std::uint8_t(domain.size()));
            static_assert(sizeof(port) <= std::numeric_limits<std::size_t>::max() - sizeof(request));
            if (std::numeric_limits<std::size_t>::max() - sizeof(request) - sizeof(port) < domain.size())
                return sizes;

            const std::size_t last_size = sizeof(request) + sizeof(port) + domain.size();
            if (out.size() < last_size)
                return sizes;

            std::memcpy(out.data(), std::addressof(request), sizeof(request));
            out.remove_prefix(sizeof(request));

            std::memcpy(out.data(), domain.data(), domain.size());
            out.remove_prefix(domain.size());

            const boost::endian::big_uint16_t big_port{port};
            std::memcpy(out.data(), std::addressof(big_port), sizeof(big_port));

            std::get<0>(sizes) = std::get<0>(result);
            std::get<1>(sizes) = std::get<1>(result);
            std::get<2>(sizes) = last_size;
            return sizes;
        }

        struct socks_category : boost::system::error_category
        {
            explicit socks_category() noexcept
              : boost::system::error_category()
            {}

            const char* name() const noexcept override
            {
                return "net::socks::error_category";
            }

            virtual std::string message(int value) const override
            {
                switch (socks::error(value))
                {
                case socks::error::general_failure:
                    return "Socks general server failure";
                case socks::error::not_allowed:
                    return "Socks connection not allowed by ruleset";
                case socks::error::network_unreachable:
                    return "Socks network unreachable";
                case socks::error::host_unreachable:
                    return "Socks host unreachable";
                case socks::error::connection_refused:
                    return "Socks connection refused";
                case socks::error::ttl_expired:
                    return "Socks TTL expired";
                case socks::error::command_not_supported:
                    return "Socks command not supported";
                case socks::error::address_type_not_supported:
                    return "Socks address type not supported";

                case socks::error::rejected:
                    return "Socks request rejected or failed";
                case socks::error::identd_connection:
                    return "Socks request rejected because server cannot connect to identd on the client";
                case socks::error::identd_user:
                    return "Socks request rejected because the client program and identd report different user-ids";

                case socks::error::auth_failure:
                    return "Socks authentication failure";
                case socks::error::bad_read:
                    return "Socks boost::async_read read fewer bytes than expected";
                case socks::error::bad_write:
                    return "Socks boost::async_write wrote fewer bytes than expected";
                case socks::error::unexpected_version:
                    return "Socks server returned unexpected version in reply";

                default:
                    break;
                }
                return "Unknown net::socks::error";
            }

            boost::system::error_condition default_error_condition(int value) const noexcept override
            {
                switch (socks::error(value))
                {
                case socks::error::network_unreachable:
                    return boost::system::errc::host_unreachable;
                case socks::error::connection_refused:
                    return boost::system::errc::connection_refused;
                case socks::error::bad_read:
                case socks::error::bad_write:
                    return boost::system::errc::io_error;
                case socks::error::unexpected_version:
                    return boost::system::errc::protocol_error;
                default:
                    break;
                };
                if (1 <= value && value <= 256)
                    return boost::system::errc::protocol_error;

                return boost::system::error_condition{value, *this};
            }
        };
    }

    const boost::system::error_category& error_category() noexcept
    {
        static const socks_category instance{};
        return instance;
    }

    struct client::completed
    {
        std::shared_ptr<client> self_;

        void operator()(const boost::system::error_code error, const std::size_t bytes) const
        {
            static_assert(1 < sizeof(self_->buffer_), "buffer too small for v4 response");

            if (self_)
            {
                client& self = *self_;
                std::get<0>(self.buffer_size_) = std::min(bytes, sizeof(self.buffer_));

                if (error)
                    self.done(error, self_);
                else if (std::get<0>(self.buffer_size_) < sizeof(v4_header))
                    self.done(socks::error::bad_read, self_);
                else if (self.buffer_[0] != 0) // response version
                    self.done(socks::error::unexpected_version, self_);
                else if (self.buffer_[1] != v4_request_granted)
                    self.done(socks::error(int(self.buffer_[1]) + 1), self_);
                else
                    self.done(boost::system::error_code{}, self_);
            }
        }
    };

    struct client::read
    {
        std::shared_ptr<client> self_;

        static boost::asio::mutable_buffer get_buffer(client& self) noexcept
        {
            static_assert(sizeof(v4_header) <= sizeof(self.buffer_), "buffer too small for v4 response");
            std::get<0>(self.buffer_size_) = sizeof(v4_header);
            return boost::asio::buffer(self.buffer_, sizeof(v4_header));
        }

        void operator()(const boost::system::error_code error, const std::size_t bytes)
        {
            if (self_)
            {
                client& self = *self_;
                if (error)
                    self.done(error, self_);
                else if (bytes < std::get<0>(self.buffer_size_))
                    self.done(socks::error::bad_write, self_);
                else
                    boost::asio::async_read(self.proxy_, get_buffer(self), boost::asio::bind_executor(self.strand_, completed{std::move(self_)}));
            }
        }
    };

    struct client::process_v5 : boost::asio::coroutine
    {
        std::shared_ptr<client> self_;

        explicit process_v5(std::shared_ptr<client> self)
          : boost::asio::coroutine(), self_(std::move(self))
        {}

        static boost::asio::mutable_buffer get_read_buffer(client& self, const std::size_t size)
        {
            const std::size_t offset =
                std::accumulate(self.buffer_size_.begin(), self.buffer_size_.end(), std::size_t(0));
            if (sizeof(self.buffer_) < offset || sizeof(self.buffer_) - offset < size)
                throw std::runtime_error{"Not enough room for reading socks v5 buffer"};
            return boost::asio::buffer(self.buffer_ + offset, size);
        }

        template<unsigned I>
        static boost::asio::const_buffer get_write_buffer(const client& self) noexcept
        {
            const std::size_t offset =
                std::accumulate(self.buffer_size_.begin(), self.buffer_size_.begin() + I, std::size_t(0));
            return boost::asio::buffer(
                self.buffer_ + offset, std::get<I>(self.buffer_size_)
            );
        }

        void operator()(const boost::system::error_code error, std::size_t bytes)
        {
            if (!self_)
                return;

            client& self = *self_;
            if (error)
            {
                self.done(error, self_);
                return;
            }

            bool send_userpass = false;
            BOOST_ASIO_CORO_REENTER(this)
            {
                // initial header already written

                BOOST_ASIO_CORO_YIELD boost::asio::async_read(
                    self.proxy_,
                    get_read_buffer(self, sizeof(v5_response_initial)),
                    boost::asio::bind_executor(self.strand_, std::move(*this))
                );
                {
                    v5_response_initial header{};

                    assert(bytes == sizeof(header));
                    const auto buf = get_read_buffer(self, sizeof(header));
                    std::memcpy(std::addressof(header), buf.data(), sizeof(header));
                    if (header.version != 5)
                    {
                        self.done(socks::error::unexpected_version, self_);
                        return;
                    }
                    if (header.method != v5_noauth_method && header.method != v5_userpass_method)
                    {
                        self.done(socks::error::auth_failure, self_);
                        return;
                    }
                    send_userpass = (header.method == v5_userpass_method);
                }

                if (send_userpass)
                {
                    if (!std::get<1>(self.buffer_size_))
                    {
                        self.done(socks::error::auth_failure, self_);
                        return;
                    }

                    BOOST_ASIO_CORO_YIELD boost::asio::async_write(
                        self.proxy_, get_write_buffer<1>(self), boost::asio::bind_executor(self.strand_, std::move(*this))
                    );
                    assert(bytes == std::get<1>(self.buffer_size_));

                    BOOST_ASIO_CORO_YIELD boost::asio::async_read(
                        self.proxy_,
                        get_read_buffer(self, sizeof(v5_response_auth)),
                        boost::asio::bind_executor(self.strand_, std::move(*this))
                    );
                    {
                        v5_response_auth header{};

                        assert(bytes == sizeof(header));
                        const auto buf = get_read_buffer(self, sizeof(header));
                        std::memcpy(std::addressof(header), buf.data(), sizeof(header));
                        if (header.version != v5_userpass_version)
                        {
                            self.done(socks::error::unexpected_version, self_);
                            return;
                        }
                        if (header.status != v5_reply_success)
                        {
                            self.done(socks::error::auth_failure, self_);
                            return;
                        }
                    }
                }

                BOOST_ASIO_CORO_YIELD boost::asio::async_write(
                    self.proxy_, get_write_buffer<2>(self), boost::asio::bind_executor(self.strand_, std::move(*this))
                );
                assert(bytes == std::get<2>(self.buffer_size_));

                self.buffer_size_ = {};
                BOOST_ASIO_CORO_YIELD boost::asio::async_read(
                    self.proxy_,
                    get_read_buffer(self, sizeof(v5_response_connect)),
                    boost::asio::bind_executor(self.strand_, std::move(*this))
                );
                {
                    v5_response_connect header{};

                    assert(bytes == sizeof(header));
                    const auto buf = get_read_buffer(self, sizeof(header));
                    std::memcpy(std::addressof(header), buf.data(), sizeof(header));
                    if (header.version != 5)
                    {
                        self.done(socks::error::unexpected_version, self_);
                        return;
                    }
                    if (header.reply != v5_reply_success)
                    {
                        self.done(socks::error(int(header.reply)), self_);
                        return;
                    }

                    if (header.type == v5_ipv4_type)
                        bytes = sizeof(v5_response_ipv4);
                    else if (header.type == v5_ipv6_type)
                        bytes = sizeof(v5_response_ipv6);
                    else
                    {
                        self.done(socks::error::unexpected_version, self_);
                        return;
                    }
                }

                std::get<0>(self.buffer_size_) = sizeof(v5_response_connect);
                BOOST_ASIO_CORO_YIELD boost::asio::async_read(
                    self.proxy_,
                    get_read_buffer(self, bytes),
                    boost::asio::bind_executor(self.strand_, std::move(*this))
                );
                std::get<0>(self.buffer_size_) =
                    std::min(sizeof(self.buffer_), sizeof(v5_response_connect) + bytes);
                self.done(error, self_);
            }
        }
    };

    struct client::write
    {
        std::shared_ptr<client> self_;

        static boost::asio::const_buffer get_buffer(client const& self) noexcept
        {
            return boost::asio::buffer(self.buffer_, std::get<0>(self.buffer_size_));
        }

        void operator()(const boost::system::error_code error)
        {
            if (self_)
            {
                client& self = *self_;
                if (error)
                    self.done(error, self_);
                else if (self.ver_ == version::v5)
                    boost::asio::async_write(self.proxy_, get_buffer(self), boost::asio::bind_executor(self.strand_, process_v5{std::move(self_)}));
                else
                    boost::asio::async_write(self.proxy_, get_buffer(self), boost::asio::bind_executor(self.strand_, read{std::move(self_)}));
            }
        }
    };



    client::client(stream_type::socket&& proxy, socks::version ver)
      : proxy_(std::move(proxy)), strand_(proxy_.get_executor()), buffer_size_{{}}, buffer_(), ver_(ver)
    {}

    client::~client() {}

    bool client::set_connect_command(const epee::net_utils::ipv4_network_address& address, const user_and_pass* userinfo)
    {
        switch (socks_version())
        {
        case version::v4:
        case version::v4a:
        case version::v4a_tor:
            break;
        case version::v5:
            buffer_size_ = write_v5_address_connect(
                buffer_,
                v5_ipv4_connect::make(boost::endian::big_to_native(address.ip()), address.port()),
                userinfo
            );
            return std::get<0>(buffer_size_) != 0;
        default:
            return false;
        }

        static_assert(sizeof(v4_header) < sizeof(buffer_), "buffer size too small for request");
        static_assert(0 < sizeof(buffer_), "buffer size too small for null termination"); 

        if (userinfo && (!userinfo->user.empty() || !userinfo->pass.empty()))
            return false;

        // version 4
        const v4_header temp{4, v4_connect_command, address.port(), boost::endian::big_to_native(address.ip())};
        std::memcpy(std::addressof(buffer_), std::addressof(temp), sizeof(temp));
        buffer_[sizeof(temp)] = 0;

        buffer_size_ = {};
        std::get<0>(buffer_size_) = sizeof(temp) + 1;

        return true;
    }

    bool client::set_connect_command(const epee::net_utils::ipv6_network_address& address, const user_and_pass* userinfo)
    {
        if (socks_version() != version::v5)
            return false;
        buffer_size_ = write_v5_address_connect(
            buffer_, v5_ipv6_connect::make(address.ip(), address.port()), userinfo
        );
        return std::get<0>(buffer_size_) != 0;
    }

    bool client::set_connect_command(const boost::string_ref domain, std::uint16_t port, const user_and_pass* userinfo)
    {
        switch (socks_version())
        {
        case version::v4a:
        case version::v4a_tor:
            break;
        case version::v5:
            buffer_size_ = write_v5_domain_connect(buffer_, port, domain, userinfo);
            return std::get<0>(buffer_size_) != 0;
        default:
            return false;
        }

        if (userinfo && (!userinfo->user.empty() || !userinfo->pass.empty()))
            return false;

        const std::size_t buf_used = write_domain_header(buffer_, v4_connect_command, port, domain);
        buffer_size_ = {};
        std::get<0>(buffer_size_) = buf_used;
        return buf_used != 0;
    }

    bool client::set_connect_command(const net::tor_address& address)
    {
        if (!address.is_unknown())
            return set_connect_command(address.host_str(), address.port());
        return false;
    }

    bool client::set_connect_command(const net::i2p_address& address, const user_and_pass* userinfo)
    {
        if (!address.is_unknown())
            return set_connect_command(address.host_str(), address.port(), userinfo);
        return false;
    }

    bool client::set_resolve_command(boost::string_ref domain)
    {
        if (socks_version() != version::v4a_tor)
            return false;

        const std::size_t buf_used = write_domain_header(buffer_, v4tor_resolve_command, 0, domain);
        buffer_size_ = {};
        std::get<0>(buffer_size_) = buf_used;
        return buf_used != 0;
    }

    bool client::connect_and_send(std::shared_ptr<client> self, const stream_type::endpoint& proxy_address)
    {
        if (self && std::get<0>(self->buffer_size_))
        {
            client& alias = *self;
            alias.proxy_.async_connect(proxy_address, boost::asio::bind_executor(alias.strand_, write{std::move(self)}));
            return true;
        }
        return false;
    }

    bool client::send(std::shared_ptr<client> self)
    {
        if (self && std::get<0>(self->buffer_size_))
        {
            client& alias = *self;
            if (alias.ver_ == version::v5)
                boost::asio::async_write(alias.proxy_, write::get_buffer(alias), boost::asio::bind_executor(alias.strand_, process_v5{std::move(self)}));
            else
                boost::asio::async_write(alias.proxy_, write::get_buffer(alias), boost::asio::bind_executor(alias.strand_, read{std::move(self)}));
            return true;
        }
        return false;
    }

    void client::async_close::operator()(boost::system::error_code error)
    {
        if (self_ && error != boost::system::errc::operation_canceled)
        {
            const std::shared_ptr<client> self = std::move(self_);
            boost::asio::dispatch(self->strand_, [self] ()
            {
                if (self && self->proxy_.is_open())
                {
                    boost::system::error_code ec;
                    self->proxy_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    self->proxy_.close(ec);
                }
            });
        }
    }
} // socks
} // net
