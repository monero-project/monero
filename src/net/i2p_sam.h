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
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <cstdint>

#include "net/i2p_address.h"
#include "net/parse.h"
#include "net/net_utils_base.h"

namespace net
{
namespace sam
{
    /**
     * Possible SAM protocol errors. Defined here:
     * https://i2p.net/en/docs/api/samv3/
     */
    enum class error : int
    {
        cant_reach_peer = 1, ///< Peer exists, but cannot be reached
        i2p_error,           ///< A generic I2P error
        invalid_key,         ///< Specified key is not valid
        invalid_id,          ///< Invalid stream ID
        timeout,             ///< Timeout while waiting for an event
        duplicated_dest,     ///< Specified destination is already in use
        key_not_found,       ///< Naming system couldn't resolve the given name
        peer_not_found,      ///< Peer couldn't be found on the network
        leaseset_not_found,  ///< Leaseset couldn't be found
        duplicated_id,       ///< Session ID is already in use
        already_accepting,   ///< There is already a pending `STREAM ACCEPT` for this session ID
        noversion,           ///< I2P router does not support SAM version

        parse_failed        ///< Not a standard SAM error code; used for parsing
    };

    //! \return boost::system::error_category for net::sam namespace
    const boost::system::error_category& error_category() noexcept;

    //! \return net::sam::error as a boost::system::error_code.
    inline boost::system::error_code make_error_code(error value) noexcept
    {
        return boost::system::error_code{int(value), sam::error_category()};
    }

    //! States of the SAM connection process.
    enum class state : int
    {
        hello_version,
        dest_generate,
        session_create,
        stream_accept,
        stream_connect
    };

    //! A connection to an I2P network peer or the router's SAM bridge.
    class client : public std::enable_shared_from_this<client>
    {
    protected:
        boost::asio::ip::tcp::socket socket_;
        boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> strand_;

        std::string line_;
        std::string write_buffer_;

        //! The public address or Base64-encoded destination of an I2P router.
        std::string destination_;

        //! Randomly generated global session ID.
        std::string session_id_;

        //! I2P network address of the remote peer (if applicable).
        net::i2p_address remote_peer_;

        bool stream_accept_listening_{false};

        //! Current state of the connection process.
        state state_;

    public:
        using stream_type = boost::asio::ip::tcp;

        //! Invoked after `send` function completes or fails.
        virtual void done(boost::system::error_code error) = 0;

        // defined in cpp
        struct send_hello_version;

        /**
         * Socket ownership is passed into this.
         * Does not have to be in connected state.
         */
        explicit client(stream_type::socket&& socket);

        client(const client&) = delete;
        virtual ~client() = default;
        client& operator=(const client&) = delete;

        //! \return Ownership of client socket object.
        stream_type::socket take_socket()
        {
            return stream_type::socket{std::move(socket_)};
        }

        const net::i2p_address& remote_address() const noexcept { return remote_peer_; }

        bool set_connect_command(const net::i2p_address& address);
        void set_session_id(const std::string& id) { session_id_ = id; }

        void set_write_buffer(std::string in) { write_buffer_ = std::move(in); }

        static bool connect_and_send(std::shared_ptr<client> self_, const stream_type::endpoint& router);

        /*! Callback for closing socket. Thread-safe with `*send` functions;
            never blocks (uses strands). */
        struct async_close
        {
            std::shared_ptr<client> self_;
            void operator()(boost::system::error_code ec = boost::system::error_code{});
        };

        //! Calls `async_close` on `self` at destruction. NOP if `nullptr`.
        struct close_on_exit
        {
            std::shared_ptr<client> self;
            ~close_on_exit() { async_close{std::move(self)}(); }
        };

        void async_write_command();
        void handle_write(boost::system::error_code ec);
        void handle_read(boost::system::error_code ec, std::size_t bytes);

    protected:
        void async_read_stream_accept(std::size_t bytes_read = 0);

    private:
        virtual void next_state(boost::system::error_code ec);

        void async_read_line();

        //! Parse response from router to get error code
        boost::system::error_code parse_result(const std::string& line);
    };

    //! A connection to an I2P router's SAM bridge.
    class control_socket : public client
    {
        //! Our private destination key; generated or retrieved from a file.
        std::string private_key_;

        //! Directory used to store the private destination key from `DEST GENERATE`.
        std::string data_dir_;

        //! The local SAM bridge endpoint, stored so we can reconnect for `STREAM ACCEPT`.
        boost::asio::ip::tcp::endpoint router_endpoint_;

        //! Keeps the `SESSION CREATE` socket alive while we open a second connection.
        boost::asio::ip::tcp::socket session_socket_;

        //! True after `SESSION CREATE` completes; read from outside the strand, so it's atomic.
        std::atomic<bool> session_established_{false};

        std::function<void(const net::i2p_address&)> destination_handler_;

        void next_state(boost::system::error_code ec) override;

    public:
        //! Tears down the session entirely and starts over.
        void async_reconnect();

        //! Retries the accept connection, keeping the current session.
        void retry_accept();

        /**
         * Retries the accept connection if a session has already been created; otherwise, falls
         * back to a full async reconnect.
         */
        void reconnect_after_error();

        void set_destination_handler(std::function<void(const net::i2p_address&)> handler) { destination_handler_ = std::move(handler); }

        bool session_established() const noexcept { return session_established_; }

        /**
         * Socket ownership is passed into this.
         * Does not have to be in connected state.
         */
        explicit control_socket(const std::string& private_key, const std::string& data_dir, boost::asio::ip::tcp::socket&& socket);

        static bool connect_and_send(std::shared_ptr<control_socket> self_, const boost::asio::ip::tcp::endpoint& router);
    };

    /**
     * @brief Removes whitespace and newlines from a saved private key file.
     * @param key The raw key contents of the saved file.
     * @return The sanitized key.
     */
    std::string sanitize_private_key(const std::string& key);

    /**
     * @brief Loads the private I2P destination key from a file.
     * @param data_dir The directory to use for loading/storing the key file.
     * @return The private key, as a string.
     */
    std::string private_key_from_file(const std::string& data_dir);

    /**
     * @brief Generates a random SAM session ID.
     * @return A string consisting of 10 random hex characters.
     */
    std::string random_session_id();

    /**
     * @brief Swaps between standard Base64 and I2P-style Base64.
     * @param in The string to convert between Base64 variants.
     * @return The converted string.
     */
    std::string swap_base64(const std::string& in);

    /**
     * @brief Derives the public Base32 I2P address from a full Base64-encoded destination.
     * @param destination The destination string, encoded in I2P-style Base64.
     * @return The address derived from the destination.
     */
    net::i2p_address destination_to_address(const std::string& destination);

    boost::system::error_code get_value(const std::string& line, const std::string& key, std::string& out);
    boost::system::error_code parse_result_code(const std::string& line);

    boost::system::error_code parse_hello_reply(const std::string& line);
    boost::system::error_code parse_session_status(const std::string& line, std::string& destination);
    boost::system::error_code parse_stream_status(const std::string& line);
    boost::system::error_code parse_dest_reply(const std::string& line, std::string& private_key);

    bool has_prefix(const std::string& line, const std::string& prefix);
    bool save_private_key(const std::string& data_dir, const std::string& private_key);

    template<typename Handler>
    class connect_client : public client
    {
        Handler handler_;

        virtual void done(boost::system::error_code error) override
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
//----------------------------------------------------------------------------------------------------
    template<typename Handler>
    class control_client : public control_socket
    {
        Handler handler_;

        virtual void done(boost::system::error_code error) override
        {
            handler_(error, take_socket(), remote_address());
            if (error) reconnect_after_error();
        }

    public:
        explicit control_client(const std::string& private_key, const std::string& data_dir, boost::asio::ip::tcp::socket&& router, Handler&& handler)
            : control_socket(private_key, data_dir, std::move(router))
            , handler_(std::move(handler))
        {}

        virtual ~control_client() override {}
    };

    template<typename Handler>
    inline std::shared_ptr<control_socket> make_control_client(const std::string& private_key, const std::string& data_dir, boost::asio::ip::tcp::socket&& router, Handler handler)
    {
        return std::make_shared<control_client<Handler>>(private_key, data_dir, std::move(router), std::move(handler));
    }
} // namespace net
} // namespace sam

namespace boost
{
namespace system
{
    template<>
    struct is_error_code_enum<net::sam::error>
        : true_type
    {};
} // namespace system
} // namespace boost
