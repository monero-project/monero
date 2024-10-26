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

#include <cstdint>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/utility/string_ref.hpp>
#include <memory>
#include <utility>

#include "net/fwd.h"
#include "span.h"

namespace epee
{
namespace net_utils
{
    class ipv4_network_address;
}
}

namespace net
{
namespace socks
{
    //! Supported socks variants.
    enum class version : std::uint8_t
    {
        v4 = 0,
        v4a,
        v4a_tor  //!< Extensions defined in Tor codebase
    };

    //! Possible errors with socks communication. Defined in https://www.openssh.com/txt/socks4.protocol
    enum class error : int
    {
        // 0 is reserved for success value
        // 1-256 -> reserved for error values from socks server (+1 from wire value).
        rejected = 92,
        identd_connection,
        identd_user,
        // Specific to application
        bad_read = 257,
        bad_write,
        unexpected_version
    };

    /* boost::system::error_code is extended for easier compatibility with
       boost::asio errors. If std::error_code is needed (with expect<T> for
       instance), then upgrade to boost 1.65+ or use conversion code in
       develop branch at boost/system/detail/std_interoperability.hpp */

    //! \return boost::system::error_category for net::socks namespace
    const boost::system::error_category& error_category() noexcept;

    //! \return net::socks::error as a boost::system::error_code.
    inline boost::system::error_code make_error_code(error value) noexcept
    {
        return boost::system::error_code{int(value), socks::error_category()};
    }

    //! Client support for socks connect and resolve commands.
    class client
    {
        boost::asio::ip::tcp::socket proxy_;
        boost::asio::io_service::strand strand_;
        std::uint16_t buffer_size_;
        std::uint8_t buffer_[1024];
        socks::version ver_;

        /*!
            Only invoked after `*send(...)` function completes or fails.
            `bool(error) == false` indicates success; `self.get()` is always
            `this` and allows implementations to skip
            `std::enable_shared_from_this<T>` (ASIO callbacks need shared_ptr).
            The design saves space and reduces cycles (everything uses moves,
            so no atomic operations are ever necessary).

            \param error when processing last command (if any).
            \param self `shared_ptr<client>` handle to `this`.
         */
        virtual void done(boost::system::error_code error, std::shared_ptr<client> self) = 0;

    public:
        using stream_type = boost::asio::ip::tcp;

        // defined in cpp
        struct write;
        struct read;
        struct completed;

        /*!
            \param proxy ownership is passed into `this`. Does not have to be
                in connected state.
            \param ver socks version for the connection.
        */
        explicit client(stream_type::socket&& proxy, socks::version ver);

        client(const client&) = delete;
        virtual ~client();
        client& operator=(const client&) = delete;

        //! \return Ownership of socks client socket object.
        stream_type::socket take_socket()
        {
            return stream_type::socket{std::move(proxy_)};
        }

        //! \return Socks version.
        socks::version socks_version() const noexcept { return ver_; }

        //! \return Contents of internal buffer.
        epee::span<const std::uint8_t> buffer() const noexcept
        {
            return {buffer_, buffer_size_};
        }

        //! \post `buffer.empty()`.
        void clear_command() noexcept { buffer_size_ = 0; }

        //! Try to set `address` as remote connection request.
        bool set_connect_command(const epee::net_utils::ipv4_network_address& address);

        //! Try to set `domain` + `port` as remote connection request.
        bool set_connect_command(boost::string_ref domain, std::uint16_t port);

        //! Try to set `address` as remote Tor hidden service connection request.
        bool set_connect_command(const net::tor_address& address);

        //! Try to set `address` as remote i2p hidden service connection request.
        bool set_connect_command(const net::i2p_address& address);

        //! Try to set `domain` as remote DNS A record lookup request.
        bool set_resolve_command(boost::string_ref domain);

        /*!
            Asynchronously connect to `proxy_address` then issue command in
            `buffer()`. The `done(...)` method will be invoked upon completion
            with `self` and potential `error`s.

            \note Must use one of the `self->set_*_command` calls before using
                this function.
            \note Only `async_close` can be invoked on `self` until the `done`
                callback is invoked.

            \param self ownership of object is given to function.
            \param proxy_address of the socks server.
            \return False if `self->buffer().empty()` (no command set).
         */
        static bool connect_and_send(std::shared_ptr<client> self, const stream_type::endpoint& proxy_address);

        /*!
            Assume existing connection to proxy server; asynchronously issue
            command in `buffer()`. The `done(...)` method will be invoked
            upon completion with `self` and potential `error`s.

            \note Must use one of the `self->set_*_command` calls before using
                the function.
            \note Only `async_close` can be invoked on `self` until the `done`
                callback is invoked.

            \param self ownership of object is given to function.
            \return False if `self->buffer().empty()` (no command set).
        */
        static bool send(std::shared_ptr<client> self);

        /*! Callback for closing socket. Thread-safe with `*send` functions;
            never blocks (uses strands). */
        struct async_close
        {
            std::shared_ptr<client> self_;
            void operator()(boost::system::error_code error = boost::system::error_code{});
        };

        //! Calls `async_close` on `self` at destruction. NOP if `nullptr`.
        struct close_on_exit
        {
            std::shared_ptr<client> self;
            ~close_on_exit() { async_close{std::move(self)}(); }
        };
    };

    template<typename Handler>
    class connect_client : public client
    {
        Handler handler_;

        virtual void done(boost::system::error_code error, std::shared_ptr<client>) override
        {
            handler_(error, take_socket());
        }

    public:
        explicit connect_client(stream_type::socket&& proxy, socks::version ver, Handler&& handler)
          : client(std::move(proxy), ver), handler_(std::move(handler))
        {}

        virtual ~connect_client() override {}
    };

    template<typename Handler>
    inline std::shared_ptr<client>
    make_connect_client(client::stream_type::socket&& proxy, socks::version ver, Handler handler)
    {
        return std::make_shared<connect_client<Handler>>(std::move(proxy), ver, std::move(handler));
    }
} // socks
} // net

namespace boost
{
namespace system
{
    template<>
    struct is_error_code_enum<net::socks::error>
      : true_type
    {};
} // system
} // boost
