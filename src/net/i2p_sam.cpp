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
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <random>

#include "misc_log_ex.h"
#include "file_io_utils.h"
#include "common/util.h"
#include "crypto/crypto.h"

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
                    return boost::system::errc::already_connected;
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
    static constexpr std::size_t MAX_NAMING_LOOKUP_ATTEMPTS = 5;
    static constexpr std::size_t NAMING_LOOKUP_RETRY_INTERVAL = 10;

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

    control_socket::control_socket(const std::string& private_key, boost::asio::ip::tcp::socket&& socket)
        : client(std::move(socket))
        , private_key_(private_key)
        , session_socket_(socket_.get_executor())
    {}

    //! Strip characters that SAM bridges may include with CRLF line endings
    void strip_newlines(std::string& line)
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
        if (result)
        {
            if (state_ == state::naming_lookup && (result == error::key_not_found || result == error::leaseset_not_found) &&
                naming_lookup_attempts_ < MAX_NAMING_LOOKUP_ATTEMPTS)
            {
                naming_lookup_attempts_ += 1;
                line_.clear();

                auto self = shared_from_this();
                auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor());
                timer->expires_after(std::chrono::seconds(NAMING_LOOKUP_RETRY_INTERVAL));
                timer->async_wait(boost::asio::bind_executor(strand_,
                    [self, timer](boost::system::error_code ec)
                    {
                        if (ec) return self->done(ec);
                        self->set_write_buffer("NAMING LOOKUP NAME=" + self->naming_lookup_ + '\n');
                        self->async_write_command();
                    }));

                return;
            }

            return done(result);
        }

        if (state_ == state::stream_accept && stream_accept_listening_)
        {
            async_read_line();
            return;
        }

        if (state_ == state::session_create)
        {
            auto self = shared_from_this();

            auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor());
            timer->expires_after(std::chrono::milliseconds(500));
            timer->async_wait(boost::asio::bind_executor(strand_,
                [self, timer](boost::system::error_code ec)
                {
                    if (ec)
                        self->done(ec);
                    else
                        self->next_state({});
                }));

            return;
        }

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

    struct control_socket::send_session_create
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& id, const std::string& private_key)
        {
            if (ec) return self->done(ec);

            const std::string cmd = "SESSION CREATE STYLE=STREAM ID=" + id + ' ' + "DESTINATION=" +
                                    private_key + ' ' + "i2cp.leaseSetEncType=6,4\n";

            MDEBUG("Sending SESSION CREATE command; ID=" << id);

            self->set_write_buffer(cmd);
            self->async_write_command();
        }
    };

    bool client::connect_and_send(std::shared_ptr<client> self_, const stream_type::endpoint& router)
    {
        if (!self_) return false;

        client& self = *self_;

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

    struct control_socket::send_dest_generate
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec);
            self->set_write_buffer("DEST GENERATE SIGNATURE_TYPE=7\n");
            self->async_write_command();
        }
    };

    struct client::send_naming_lookup
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& address)
        {
            if (ec) return self->done(ec);
            self->set_write_buffer("NAMING LOOKUP NAME=" + address + '\n');
            self->async_write_command();
        }
    };

    struct client::send_stream_connect
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& session_id, const std::string& destination)
        {
            if (ec) return self->done(ec);

            const std::string cmd = "STREAM CONNECT ID=" + session_id + " DESTINATION=" + destination + " SILENT=FALSE TIMEOUT=60\n";

            MDEBUG("Sending STREAM CONNECT command: " << cmd << " with ID of " << session_id);

            self->set_write_buffer(cmd);
            self->async_write_command();
        }
    };

    struct client::send_stream_accept
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& session_id)
        {
            if (ec) return self->done(ec);
            self->set_write_buffer("STREAM ACCEPT ID=" + session_id + " SILENT=FALSE\n");
            self->async_write_command();
        }
    };

    void client::next_state(boost::system::error_code ec)
    {
        if (ec) return done(ec);

        auto self = shared_from_this();

        switch (state_)
        {
        case state::hello_version:
            state_ = state::naming_lookup;
            boost::asio::dispatch(strand_,
                [self, this](){ send_naming_lookup{self}({}, naming_lookup_); });
            return;

        case state::naming_lookup:
            state_ = state::stream_connect;
            boost::asio::dispatch(strand_,
                [self, this](){ send_stream_connect{self}({}, session_id_, destination_); });
            return;

        default:
            MINFO("Done with SAM loop");
            done({});
            return;
        }
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
                    [self, this](){ send_stream_accept{self}({}, session_id_); });
            }
            else
            {
                state_ = state::session_create;
                boost::asio::dispatch(strand_,
                    [self, this](){ send_session_create{self}({}, session_id_, private_key_); });
            }
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
            done({});

            socket_ = boost::asio::ip::tcp::socket{session_socket_.get_executor()};
            line_.clear();
            stream_established_ = false;
            stream_accept_listening_ = false;
            state_ = state::hello_version;
            socket_.async_connect(router_endpoint_,
                boost::asio::bind_executor(strand_, send_hello_version{self}));
            return;

        default:
            MINFO("Done with SAM loop");
            done({});
            return;
        }
    }

    boost::system::error_code get_value(const std::string& line, const std::string& key, std::string& out)
    {
        MDEBUG("Parsing line (get_value): " << line);

        std::string pattern = key + "=";
        std::size_t pos = line.find(pattern);
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

        MDEBUG("Parsing line (parse_result_code): " << line);

        if (get_value(line, "RESULT", result)) return make_error_code(error::parse_failed);

        if (result == "OK") return {};

        if (result == "CANT_REACH_PEER") return make_error_code(error::cant_reach_peer);
        if (result == "I2P_ERROR") return make_error_code(error::i2p_error);
        if (result == "INVALID_KEY") return make_error_code(error::invalid_key);
        if (result == "INVALID_ID") return make_error_code(error::invalid_id);
        if (result == "TIMEOUT") return make_error_code(error::timeout);
        if (result == "DUPLICATED_DEST") return make_error_code(error::duplicated_dest);
        if (result == "KEY_NOT_FOUND") return make_error_code(error::key_not_found);
        if (result == "PEER_NOT_FOUND") return make_error_code(error::peer_not_found);
        if (result == "LEASESET_NOT_FOUND") return make_error_code(error::leaseset_not_found);
        if (result == "NOVERSION") return make_error_code(error::noversion);

        return make_error_code(error::parse_failed);
    }

    boost::system::error_code client::parse_result(const std::string& line)
    {
        switch (state_)
        {
        case state::hello_version:
        {
            if (line.find("HELLO REPLY") == std::string::npos)
                return make_error_code(error::parse_failed);

            return parse_result_code(line);
        }

        case state::naming_lookup:
        {
            boost::system::error_code ec;

            ec = get_value(line, "VALUE", destination_);
            if (!ec)
            {
                MDEBUG("Received naming lookup result VALUE=" << destination_);
                return {};
            }

            return parse_result_code(line);
        }

        case state::session_create:
        {
            boost::system::error_code ec;

            ec = get_value(line, "DESTINATION", destination_);
            if (!ec)
            {
                MDEBUG("Created session with DESTINATION=" << destination_);
                return {};
            }

            return parse_result_code(line);
        }

        case state::stream_connect:
        {
            auto ec = parse_result_code(line);
            if (ec) return ec;

            stream_established_ = true;
            MDEBUG("SAM stream established to " << remote_peer_.str());
            return {};
        }

        case state::stream_accept:
        {
            if (stream_accept_listening_)
            {
                stream_accept_listening_ = false;
                stream_established_ = true;
                MDEBUG("SAM stream accepted from incoming peer");
                return {};
            }

            auto ec = parse_result_code(line);
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
        if (address.is_unknown())
            return false;

        remote_peer_ = address;
        set_naming_lookup(address.host_str());

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

    std::string private_key_from_file(const std::string& data_dir)
    {
        const std::string key_path = data_dir + "/i2p_private_key";

        std::string private_key{};

        if (epee::file_io_utils::is_file_exist(key_path))
        {
            if (!epee::file_io_utils::load_file_to_string(key_path, private_key))
                throw std::runtime_error("Failed to load I2P destination key from " + key_path);

            MINFO("Read I2P destination key from file (" << key_path << "), length " << private_key.size());

            if (private_key.empty())
                throw std::runtime_error("I2P destination key is empty: " + key_path);
        }

        return private_key;
    }

    std::string random_session_id()
    {
        std::random_device rng;
        std::mt19937 gen(rng());
        std::uniform_int_distribution<> dist('a', 'z');

        std::string result;
        result.reserve(10);
        for (std::size_t i = 0; i < 10; ++i)
            result += static_cast<char>(dist(gen));

        return result;
    }
} // namespace net
} // namespace sam
