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

#include "i2p_sam.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/filesystem/path.hpp>
#include <cctype>
#include <string>

#include "misc_log_ex.h"
#include "file_io_utils.h"
#include "crypto/crypto.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.i2p"

namespace net
{
namespace sam
{
    namespace
    {
        struct sam_category : boost::system::error_category
        {
            explicit sam_category() noexcept : boost::system::error_category() {}

            const char* name() const noexcept override
            {
                return "net::sam::error_category";
            }

            virtual std::string message(int value) const override
            {
                switch (sam::error(value))
                {
                case sam::error::cant_reach_peer:
                    return "Peer exists, but cannot be reached, or has an outdated router";
                case sam::error::i2p_error:
                    return "Encountered a generic I2P error";
                case sam::error::invalid_key:
                    return "Specified key is not valid";
                case sam::error::invalid_id:
                    return "Specified ID is not valid";
                case sam::error::timeout:
                    return "Timeout while waiting for an event";
                case sam::error::duplicated_dest:
                    return "Specified destination is already in use";
                case sam::error::key_not_found:
                    return "Naming system can't resolve the given name";
                case sam::error::peer_not_found:
                    return "Peer cannot be found on the network";
                case sam::error::leaseset_not_found:
                    return "Leaseset could not be found";
                case sam::error::duplicated_id:
                    return "Specified session ID is already in use";
                case sam::error::already_accepting:
                    return "Session is already accepting connections";
                case sam::error::noversion:
                    return "I2P router doesn't support SAM version";
                case sam::error::parse_failed:
                    return "Error while parsing response from router";

                default:
                    break;
                }
                return "Unknown net::sam::error";
            }

            boost::system::error_condition default_error_condition(int value) const noexcept override
            {
                switch (sam::error(value))
                {
                case sam::error::cant_reach_peer:
                case sam::error::leaseset_not_found:
                    return boost::system::errc::host_unreachable;
                case sam::error::duplicated_dest:
                case sam::error::duplicated_id:
                    return boost::system::errc::already_connected;
                case sam::error::already_accepting:
                    return boost::system::errc::connection_already_in_progress;
                case sam::error::invalid_key:
                case sam::error::invalid_id:
                    return boost::system::errc::io_error;
                case sam::error::timeout:
                    return boost::system::errc::timed_out;
                case sam::error::i2p_error:
                case sam::error::noversion:
                    return boost::system::errc::protocol_error;
                case sam::error::parse_failed:
                    return boost::system::errc::io_error;
                case sam::error::key_not_found:
                case sam::error::peer_not_found:
                    return boost::system::errc::address_not_available;

                default:
                    break;
                };

                return boost::system::error_condition{value, *this};
            }
        };
    } // namespace

    static constexpr std::size_t MAX_LINE_LEN = 4096;
    static constexpr std::size_t SESSION_ID_LEN = 10;
    static constexpr std::size_t RECONNECT_DELAY_SECONDS = 10;

    const boost::system::error_category& error_category() noexcept
    {
        static const sam_category instance{};
        return instance;
    }

    client::client(stream_type::socket&& socket)
        : socket_(std::move(socket))
        , strand_(socket_.get_executor())
        , state_(state::hello_version)
    {}

    control_socket::control_socket(const std::string& private_key, const std::string& data_dir, boost::asio::ip::tcp::socket&& socket)
        : client(std::move(socket))
        , private_key_(private_key)
        , data_dir_(data_dir)
        , session_socket_(socket_.get_executor())
    {}

    //! Strip characters that SAM bridges may include with CRLF line endings
    static void strip_newlines(std::string& line)
    {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        {
            line.pop_back();
        }
    }

    void client::async_write_command()
    {
        auto self = shared_from_this();

        boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
            boost::asio::bind_executor(strand_,
                [self](boost::system::error_code ec, std::size_t)
                {
                    self->handle_write(ec);
                }));
    }

    void client::async_read_line()
    {
        auto self = shared_from_this();

        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(line_, MAX_LINE_LEN), '\n',
            boost::asio::bind_executor(strand_,
                [self](boost::system::error_code ec, std::size_t bytes)
                {
                    self->handle_read(ec, bytes);
                }));
    }

    /**
     * The response to `STREAM ACCEPT` is unique in that the router will send the peer's Base64
     * destination as a single line, and then start sending that peer's stream data immediately
     * after. This means that some of the P2P data can get pulled into the line being parsed.
     */
    void client::async_read_stream_accept(std::size_t bytes_read)
    {
        if (bytes_read >= MAX_LINE_LEN)
        {
            auto ec = make_error_code(error::parse_failed);
            return done(ec);
        }

        auto self = shared_from_this();

        //! Read one byte at a time into `line_` so that we stop at the newline character.
        line_.push_back('\0');
        socket_.async_read_some(boost::asio::buffer(&line_.back(), 1),
            boost::asio::bind_executor(strand_,
                [self, bytes_read](boost::system::error_code ec, std::size_t)
                {
                    if (ec) return self->done(ec);

                    if (self->line_.back() != '\n') return self->async_read_stream_accept(bytes_read + 1);
                    strip_newlines(self->line_);

                    if (self->line_.empty())
                    {
                        auto ec = make_error_code(error::parse_failed);
                        self->line_.clear();
                        return self->done(ec);
                    }

                    if (has_prefix(self->line_, "STREAM STATUS"))
                    {
                        const auto status = parse_stream_status(self->line_);
                        self->line_.clear();

                        if (status) return self->done(status);
                        else
                        {
                            auto ec = make_error_code(error::parse_failed);
                            return self->done(ec);
                        }
                    }

                    if (self->line_.find(' ') != std::string::npos)
                    {
                        auto ec = make_error_code(error::parse_failed);
                        self->line_.clear();
                        return self->done(ec);
                    }

                    MDEBUG("SAM stream accepted data from incoming peer");

                    self->line_.clear();
                    self->stream_accept_listening_ = false;
                    self->next_state({});
                }));
    }

    void client::handle_write(boost::system::error_code ec)
    {
        if (ec) return done(ec);
        async_read_line();
    }

    void client::handle_read(boost::system::error_code ec, std::size_t bytes)
    {
        if (ec) return done(ec);

        std::string line = line_.substr(0, bytes);
        line_.erase(0, bytes);
        strip_newlines(line);

        auto result = parse_result(line);
        if (result) return done(result);

        next_state({});
    }

    struct client::send_hello_version
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec);
            self->set_write_buffer("HELLO VERSION MIN=3.1 MAX=3.1\n");
            self->async_write_command();
        }
    };

    static void send_session_create(const std::shared_ptr<client>& self, const std::string& id, const std::string& private_key)
    {
        MDEBUG("Sending SESSION CREATE command with ID=" << id);

        /**
         * Leaseset encryption type: MLKEM-768/ECIES-X25519
         * Reference: https://docs.i2pd.website/en/latest/user-guide/tunnels/#leaseset
         */
        self->set_write_buffer("SESSION CREATE STYLE=STREAM ID=" + id + " DESTINATION=" + private_key +
                               " i2cp.leaseSetEncType=6,4 inbound.quantity=3 outbound.quantity=3\n");
        self->async_write_command();
    }

    bool client::connect_and_send(std::shared_ptr<client> self_, const stream_type::endpoint& router)
    {
        if (!self_) return false;

        client& self = *self_;
        self.state_ = state::hello_version;

        self.socket_.async_connect(router,
            boost::asio::bind_executor(self.strand_, send_hello_version{std::move(self_)}));

        return true;
    }

    bool control_socket::connect_and_send(std::shared_ptr<control_socket> self_, const stream_type::endpoint& router)
    {
        if (!self_) return false;
        self_->router_endpoint_ = router;
        return client::connect_and_send(std::move(self_), router);
    }

    static void send_stream_connect(const std::shared_ptr<client>& self, const std::string& session_id, const std::string& destination)
    {
        MDEBUG("Sending STREAM CONNECT command with ID=" << session_id);

        self->set_write_buffer("STREAM CONNECT ID=" + session_id + " DESTINATION=" + destination + " SILENT=FALSE TIMEOUT=60\n");
        self->async_write_command();
    }

    static void send_stream_accept(const std::shared_ptr<client>& self, const std::string& session_id)
    {
        self->set_write_buffer("STREAM ACCEPT ID=" + session_id + " SILENT=FALSE\n");
        self->async_write_command();
    }

    void client::next_state(boost::system::error_code ec)
    {
        if (ec) return done(ec);

        auto self = shared_from_this();

        switch (state_)
        {
        case state::hello_version:
            state_ = state::stream_connect;
            boost::asio::dispatch(strand_,
                [self, this](){ send_stream_connect(self, session_id_, destination_); });
            return;

        case state::stream_connect:
            return done({});

        default:
            return done(make_error_code(error::parse_failed));
        }
    }

    void control_socket::async_reconnect()
    {
        auto self = shared_from_this();

        boost::system::error_code ec;
        socket_.close(ec);
        session_socket_.close(ec);

        line_.clear();
        stream_accept_phase_ = false;
        stream_accept_listening_ = false;
        state_ = state::hello_version;

        auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor());
        timer->expires_after(std::chrono::seconds(RECONNECT_DELAY_SECONDS));
        timer->async_wait(boost::asio::bind_executor(strand_,
            [self, this, timer](boost::system::error_code ec)
            {
                if (ec) return;
                socket_ = boost::asio::ip::tcp::socket{socket_.get_executor()};
                socket_.async_connect(router_endpoint_,
                    boost::asio::bind_executor(strand_, send_hello_version{self}));
            }));
    }

    void control_socket::next_state(boost::system::error_code ec)
    {
        if (ec) return done(ec);

        auto self = shared_from_this();

        switch (state_)
        {
        case state::hello_version:
            if (stream_accept_phase_)
            {
                state_ = state::stream_accept;
                boost::asio::dispatch(strand_,
                    [self, this](){ send_stream_accept(self, session_id_); });
            }
            else if (private_key_.empty())
            {
                state_ = state::dest_generate;
                boost::asio::dispatch(strand_, [self, this]()
                {
                    set_write_buffer("DEST GENERATE SIGNATURE_TYPE=7\n");
                    async_write_command();
                });
            }
            else
            {
                state_ = state::session_create;
                boost::asio::dispatch(strand_,
                    [self, this](){ send_session_create(self, session_id_, private_key_); });
            }
            return;

        case state::dest_generate:
            private_key_ = destination_;
            if (!save_private_key(data_dir_, private_key_))
            {
                auto ec = make_error_code(error::i2p_error);
                return done(ec);
            }

            socket_ = boost::asio::ip::tcp::socket{socket_.get_executor()};
            line_.clear();
            state_ = state::hello_version;
            socket_.async_connect(router_endpoint_,
                boost::asio::bind_executor(strand_, send_hello_version{self}));
            return;

        case state::session_create:
            session_socket_ = std::move(socket_);
            socket_ = boost::asio::ip::tcp::socket{session_socket_.get_executor()};
            line_.clear();
            stream_accept_phase_ = true;
            state_ = state::hello_version;
            socket_.async_connect(router_endpoint_,
                boost::asio::bind_executor(strand_, send_hello_version{self}));
            return;

        case state::stream_accept:
            if (stream_accept_listening_) return async_read_stream_accept();
            done({});

            socket_ = boost::asio::ip::tcp::socket{session_socket_.get_executor()};
            line_.clear();
            state_ = state::hello_version;
            socket_.async_connect(router_endpoint_,
                boost::asio::bind_executor(strand_, send_hello_version{self}));
            return;

        default:
            return done(make_error_code(error::parse_failed));
        }
    }

    boost::system::error_code get_value(const std::string& line, const std::string& key, std::string& out)
    {
        const std::string pattern = key + "=";

        std::size_t pos = line.find(pattern);
        while (pos != std::string::npos && pos != 0 && line[pos - 1] != ' ')
            pos = line.find(pattern, pos + 1);

        if (pos == std::string::npos)
            return make_error_code(error::parse_failed);

        pos += pattern.length();

        std::size_t end = line.find(' ', pos);
        if (end == std::string::npos)
            end = line.length();

        out = line.substr(pos, end - pos);

        strip_newlines(out);

        return {};
    }

    boost::system::error_code parse_result_code(const std::string& line)
    {
        std::string result;

        if (get_value(line, "RESULT", result)) return make_error_code(error::parse_failed);
        if (result == "OK") return {};

        if (result == "CANT_REACH_PEER") return make_error_code(error::cant_reach_peer);
        if (result == "I2P_ERROR") return make_error_code(error::i2p_error);
        if (result == "INVALID_KEY") return make_error_code(error::invalid_key);
        if (result == "INVALID_ID") return make_error_code(error::invalid_id);
        if (result == "TIMEOUT") return make_error_code(error::timeout);
        if (result == "DUPLICATED_DEST") return make_error_code(error::duplicated_dest);
        if (result == "DUPLICATED_ID") return make_error_code(error::duplicated_id);
        if (result == "KEY_NOT_FOUND") return make_error_code(error::key_not_found);
        if (result == "PEER_NOT_FOUND") return make_error_code(error::peer_not_found);
        if (result == "LEASESET_NOT_FOUND") return make_error_code(error::leaseset_not_found);
        if (result == "ALREADY_ACCEPTING") return make_error_code(error::already_accepting);
        if (result == "NOVERSION") return make_error_code(error::noversion);

        return make_error_code(error::parse_failed);
    }

    bool has_prefix(const std::string& line, const std::string& prefix)
    {
        if (line.compare(0, prefix.size(), prefix) != 0) return false;
        return line.size() == prefix.size() || line[prefix.size()] == ' ';
    }

    boost::system::error_code parse_hello_reply(const std::string& line)
    {
        if (!has_prefix(line, "HELLO REPLY")) return make_error_code(error::parse_failed);
        return parse_result_code(line);
    }

    boost::system::error_code parse_session_status(const std::string& line, std::string& destination)
    {
        if (!has_prefix(line, "SESSION STATUS")) return make_error_code(error::parse_failed);

        const auto ec = parse_result_code(line);
        if (ec) return ec;

        if (get_value(line, "DESTINATION", destination)) return make_error_code(error::parse_failed);
        return {};
    }

    boost::system::error_code parse_stream_status(const std::string& line)
    {
        if (!has_prefix(line, "STREAM STATUS")) return make_error_code(error::parse_failed);
        return parse_result_code(line);
    }

    boost::system::error_code parse_dest_reply(const std::string& line, std::string& private_key)
    {
        if (!has_prefix(line, "DEST REPLY")) return make_error_code(error::parse_failed);
        if (!get_value(line, "PRIV", private_key) && !private_key.empty()) return {};
        return parse_result_code(line);
    }

    boost::system::error_code client::parse_result(const std::string& line)
    {
        switch (state_)
        {
        case state::hello_version:
            return parse_hello_reply(line);

        case state::dest_generate:
            return parse_dest_reply(line, destination_);

        case state::session_create:
        {
            const auto ec = parse_session_status(line, destination_);
            if (ec) return ec;

            MDEBUG("Created session with DESTINATION=" << destination_.substr(0, 25) << "...");
            return {};
        }

        case state::stream_connect:
        {
            const auto ec = parse_stream_status(line);
            if (ec) return ec;

            MDEBUG("I2P SAM stream established to " << remote_peer_.str());
            return {};
        }

        case state::stream_accept:
        {
            const auto ec = parse_stream_status(line);
            if (ec) return ec;

            stream_accept_listening_ = true;
            return {};
        }

        default:
            return make_error_code(error::parse_failed);
        }
    }

    bool client::set_connect_command(const net::i2p_address& address)
    {
        if (address.is_unknown()) return false;

        remote_peer_ = address;
        destination_ = address.host_str();

        return true;
    }

    void client::async_close::operator()(boost::system::error_code ec)
    {
        if (self_ && ec != boost::system::errc::operation_canceled)
        {
            const std::shared_ptr<client> self = std::move(self_);
            boost::asio::dispatch(self->strand_, [self]()
            {
                if (self && self->socket_.is_open())
                {
                    boost::system::error_code ec_;
                    self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec_);
                    self->socket_.close(ec_);
                }
            });
        }
    }

    std::string sanitize_private_key(const std::string& key)
    {
        std::string result;
        result.reserve(key.size());
        for (std::size_t i = 0; i < key.size(); ++i)
        {
            if (!std::isspace(static_cast<unsigned char>(key[i])))
                result.push_back(key[i]);
        }

        return result;
    }

    static std::string key_file_path(const std::string& data_dir)
    {
        return (boost::filesystem::path(data_dir) / "i2p_private_key").string();
    }

    std::string private_key_from_file(const std::string& data_dir)
    {
        const std::string key_path = key_file_path(data_dir);

        std::string private_key{};

        if (epee::file_io_utils::is_file_exist(key_path))
        {
            if (!epee::file_io_utils::load_file_to_string(key_path, private_key))
            {
                throw std::runtime_error("Failed to load I2P destination key from " + key_path);
            }

            private_key = sanitize_private_key(std::move(private_key));

            MINFO("Read I2P destination key from file (" << key_path << "), length=" << private_key.size());
        }

        return private_key;
    }

    bool save_private_key(const std::string& data_dir, const std::string& private_key)
    {
        const std::string key_path = key_file_path(data_dir);

        if (!epee::file_io_utils::save_string_to_file(key_path, private_key))
        {
            MERROR("Failed to save I2P destination key to " << key_path);
            return false;
        }

        MINFO("Saved I2P destination key to " << key_path);
        return true;
    }

    std::string random_session_id()
    {
        static constexpr char alphabet[] = "0123456789abcdef";

        std::string result;
        result.reserve(SESSION_ID_LEN);
        for (std::size_t i = 0; i < SESSION_ID_LEN; ++i)
        {
            result += alphabet[crypto::rand<std::uint8_t>() & 0x0F];
        }

        return result;
    }
} // namespace net
} // namespace sam
