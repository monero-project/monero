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
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <cstring>
#include <limits>
#include <string>

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

        struct v4_header
        {
            std::uint8_t version;
            std::uint8_t command_code;
            boost::endian::big_uint16_t port;
            boost::endian::big_uint32_t ip;
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
                case socks::error::rejected:
                    return "Socks request rejected or failed";
                case socks::error::identd_connection:
                    return "Socks request rejected because server cannot connect to identd on the client";
                case socks::error::identd_user:
                    return "Socks request rejected because the client program and identd report different user-ids";

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
                self.buffer_size_ = std::min(bytes, sizeof(self.buffer_));

                if (error)
                    self.done(error, std::move(self_));
                else if (self.buffer().size() < sizeof(v4_header))
                    self.done(socks::error::bad_read, std::move(self_));
                else if (self.buffer_[0] != 0) // response version
                    self.done(socks::error::unexpected_version, std::move(self_));
                else if (self.buffer_[1] != v4_request_granted)
                    self.done(socks::error(int(self.buffer_[1]) + 1), std::move(self_));
                else
                    self.done(boost::system::error_code{}, std::move(self_));
            }
        }
    };

    struct client::read
    {
        std::shared_ptr<client> self_;

        static boost::asio::mutable_buffers_1 get_buffer(client& self) noexcept
        {
            static_assert(sizeof(v4_header) <= sizeof(self.buffer_), "buffer too small for v4 response");
            return boost::asio::buffer(self.buffer_, sizeof(v4_header));
        }

        void operator()(const boost::system::error_code error, const std::size_t bytes)
        {
            if (self_)
            {
                client& self = *self_;
                if (error)
                    self.done(error, std::move(self_));
                else if (bytes < self.buffer().size())
                    self.done(socks::error::bad_write, std::move(self_));
                else
                    boost::asio::async_read(self.proxy_, get_buffer(self), self.strand_.wrap(completed{std::move(self_)}));
            }
        }
    };

    struct client::write
    {
        std::shared_ptr<client> self_;

        static boost::asio::const_buffers_1 get_buffer(client const& self) noexcept
        {
            return boost::asio::buffer(self.buffer_, self.buffer_size_);
        }

        void operator()(const boost::system::error_code error)
        {
            if (self_)
            {
                client& self = *self_;
                if (error)
                    self.done(error, std::move(self_));
                else
                    boost::asio::async_write(self.proxy_, get_buffer(self), self.strand_.wrap(read{std::move(self_)}));
            }
        }
    };

    client::client(stream_type::socket&& proxy, socks::version ver)
      : proxy_(std::move(proxy)), strand_(GET_IO_SERVICE(proxy_)), buffer_size_(0), buffer_(), ver_(ver)
    {}

    client::~client() {}

    bool client::set_connect_command(const epee::net_utils::ipv4_network_address& address)
    {
        switch (socks_version())
        {
        case version::v4:
        case version::v4a:
        case version::v4a_tor:
            break;
        default:
            return false;
        }

        static_assert(sizeof(v4_header) < sizeof(buffer_), "buffer size too small for request");
        static_assert(0 < sizeof(buffer_), "buffer size too small for null termination"); 

        // version 4
        const v4_header temp{4, v4_connect_command, address.port(), boost::endian::big_to_native(address.ip())};
        std::memcpy(std::addressof(buffer_), std::addressof(temp), sizeof(temp));
        buffer_[sizeof(temp)] = 0;
        buffer_size_ = sizeof(temp) + 1;

        return true;
    }

    bool client::set_connect_command(const boost::string_ref domain, std::uint16_t port)
    {
        switch (socks_version())
        {
        case version::v4a:
        case version::v4a_tor:
            break;

        default:
            return false;
        }

        const std::size_t buf_used = write_domain_header(buffer_, v4_connect_command, port, domain);
        buffer_size_ = buf_used;
        return buf_used != 0;
    }

    bool client::set_connect_command(const net::tor_address& address)
    {
        if (!address.is_unknown())
            return set_connect_command(address.host_str(), address.port());
        return false;
    }

    bool client::set_connect_command(const net::i2p_address& address)
    {
        if (!address.is_unknown())
            return set_connect_command(address.host_str(), address.port());
        return false;
    }

    bool client::set_resolve_command(boost::string_ref domain)
    {
        if (socks_version() != version::v4a_tor)
            return false;

        const std::size_t buf_used = write_domain_header(buffer_, v4tor_resolve_command, 0, domain);
        buffer_size_ = buf_used;
        return buf_used != 0;
    }

    bool client::connect_and_send(std::shared_ptr<client> self, const stream_type::endpoint& proxy_address)
    {
        if (self && !self->buffer().empty())
        {
            client& alias = *self;
            alias.proxy_.async_connect(proxy_address, alias.strand_.wrap(write{std::move(self)}));
            return true;
        }
        return false;
    }

    bool client::send(std::shared_ptr<client> self)
    {
        if (self && !self->buffer().empty())
        {
            client& alias = *self;
            boost::asio::async_write(alias.proxy_, write::get_buffer(alias), alias.strand_.wrap(read{std::move(self)}));
            return true;
        }
        return false;
    }

    void client::async_close::operator()(boost::system::error_code error)
    {
        if (self_ && error != boost::system::errc::operation_canceled)
        {
            const std::shared_ptr<client> self = std::move(self_);
            self->strand_.dispatch([self] ()
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
