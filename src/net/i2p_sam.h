// Copyright (c) 2018-2026, The Monero Project
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
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <memory>
#include <utility>
#include <array>
#include <cstdint>

#include "net/parse.h"
#include "net/net_utils_base.h"

namespace net::sam
{
    /**
     * Possible SAM protocol errors. Defined here:
     * https://i2p.net/en/docs/api/samv3/
     */
    enum class error : int
    {
        cant_reach_peer,    ///< Peer exists, but cannot be reached
        i2p_error,          ///< A generic I2P error
        invalid_key,        ///< Specified key is not valid
        invalid_id,         ///< Invalid stream ID
        timeout,            ///< Timeout while waiting for an event
        duplicated_dest,    ///< Specified destination is already in use
        key_not_found,      ///< Naming system couldn't resolve the given name
        peer_not_found,     ///< Peer couldn't be found on the network
        leaseset_not_found, ///< Leaseset couldn't be found
        noversion,          ///< I2P router does not support SAM version

        parse_failed        ///< Not a standard SAM error code; used for parsing
    };

    //! \return boost::system::error_category for net::sam namespace
    const boost::system::error_category& error_category() noexcept;

    //! \return net::sam::error as a boost::system::error_code.
    inline boost::system::error_code make_error_code(error value) noexcept
    {
        return boost::system::error_code{int(value), sam::error_category()};
    }

    /**
     * Represents an I2P connection, either directly to the router's SAM bridge,
     * or to a network peer (via a proxy set up by the router).
     *
     * This is invoked by code in `p2p/net_node.{cpp,h,inl}`.
     */
    class client : public std::enable_shared_from_this<client>
    {
        boost::asio::ip::tcp::socket socket_;
        boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> strand_;

        std::string line_;

        //! The SAM session ID (global).
        std::string session_id_;

        //! I2P address stored for querying using the `NAMING LOOKUP` command.
        std::string naming_lookup_;

        //! Base64-encoded I2P destination; result of `NAMING LOOKUP` command.
        std::string destination_;

        //! The public I2P address of this node.
        std::string public_key_;

        //! Our I2P private key; retrieved from a file.
        std::string private_key_;

        //! Current state of the connection process.
        enum class state : std::uint8_t
        {
            hello_version,
            dest_generate,
            naming_lookup,
            session_create,
            stream_connect,
            stream_accept
        };

        state state_;

        //! Invoked after `send` function completes or fails.
        virtual void done(boost::system::error_code error, const std::shared_ptr<client>& self) = 0;

    public:
        using stream_type = boost::asio::ip::tcp;

        // defined in cpp
        struct send_hello_version;
        struct send_dest_generate;
        struct send_naming_lookup;
        struct send_session_create;
        struct send_stream_connect;
        struct send_stream_accept;

        /**
         * Socket ownership is passed into this.
         * Does not have to be in connected state.
         */
        explicit client(stream_type::socket&& socket);

        client(const client&) = delete;
        virtual ~client();
        client& operator=(const client&) = delete;

        //! \return Ownership of socks client socket object.
        stream_type::socket take_socket()
        {
            return stream_type::socket{std::move(socket_)};
        }

        void set_session_id(std::string id) { session_id_ = std::move(id); }
        void set_destination(std::string dest) { destination_ = std::move(dest); }
        void set_naming_lookup(std::string address) { naming_lookup_ = std::move(address); }

        bool set_connect_command(const net::i2p_address& address);

        static bool connect_and_send(std::shared_ptr<client> self_, const stream_type::endpoint& router);
        static bool generate_destination(std::shared_ptr<client> self_, const stream_type::endpoint& router);

        struct async_close
        {
            std::shared_ptr<client> self_;
            void operator()(boost::system::error_code ec = boost::system::error_code{});
        };

        struct close_on_exit
        {
            std::shared_ptr<client> self;
            ~close_on_exit() { async_close{std::move(self)}(); }
        };

    private:
        struct read_line;
        struct parse_line;

        void next_state(const std::shared_ptr<client>& self, boost::system::error_code ec);

        //! Parse response from router to get error code
        boost::system::error_code parse_result(const std::string& line);

        /**
         * @brief Attempts to load the private I2P destination key from a file;
         *        generates and stores a new destination if not existent.
         * @return The contents of the file as a string.
         */
        std::string private_key_from_file();
    };

    template<typename Handler>
    class connect_client : public client
    {
        Handler handler_;

        virtual void done(boost::system::error_code error, const std::shared_ptr<client>&) override
        {
            handler_(error, take_socket());
        }

    public:
        explicit connect_client(stream_type::socket&& router, Handler&& handler)
            : client(std::move(router))
            , handler_(std::move(handler))
        {}

        virtual ~connect_client() override {}
    };

    template<typename Handler>
    inline std::shared_ptr<client> make_connect_client(client::stream_type::socket&& router, Handler handler)
    {
        return std::make_shared<connect_client<Handler>>(std::move(router), std::move(handler));
    }
} // namespace net::sam

namespace boost::system
{
    template<>
    struct is_error_code_enum<net::sam::error>
        : true_type
    {};
} // namespace boost::system
