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
#include <numeric>
#include <string>
#include <random>

#include "net/i2p_address.h"
#include "misc_log_ex.h"
#include "file_io_utils.h"
#include "common/util.h"
#include "crypto/crypto.h"
#include "hex.h"

namespace net::sam
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
                    return "Peer exists, but cannot be reached";
                case sam::error::i2p_error:
                    return "Generic I2P error";
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
                    return boost::system::errc::host_unreachable;
                case sam::error::duplicated_dest:
                    return boost::system::errc::already_connected;
                case sam::error::invalid_key:
                case sam::error::invalid_id:
                    return boost::system::errc::io_error;
                case sam::error::timeout:
                    return boost::system::errc::timed_out;
                case sam::error::i2p_error:
                case sam::error::leaseset_not_found:
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

    const boost::system::error_category& error_category() noexcept
    {
        static const sam_category instance{};
        return instance;
    }

    /**
     * @brief Generates a random SAM session ID.
     * @return A random 10-character string.
     */
    std::string random_session_id()
    {
        std::random_device rng;
        std::mt19937 gen(rng());
        std::uniform_int_distribution<> dist('a', 'z');

        std::string result;
        result.reserve(10);
        for (std::size_t i = 0; i < 10; ++i)
        {
            result += static_cast<char>(dist(gen));
        }

        return result;
    }

    client::client(stream_type::socket&& socket)
        : socket_(std::move(socket))
        , strand_(socket_.get_executor())
        , state_(state::hello_version)
        , session_id_(random_session_id())
        , transient_(true)
    {}

    client::~client() = default;

    struct client::read_line
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, std::size_t bytes)
        {
            if (ec) return self->done(ec, self);

            if (bytes == 0)
            {
                boost::asio::async_read_until(self->socket_, boost::asio::dynamic_buffer(self->line_), '\n',
                    boost::asio::bind_executor(self->strand_, *this));
            }
            else
            {
                std::string line = self->line_.substr(0, bytes);
                self->line_.erase(0, bytes);

                auto result = self->parse_result(line);
                if (result) return self->done(result, self);

                self->next_state(self, {});
            }
        }
    };

    struct client::send_hello_version
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec, self);
            //! Reference: https://i2p.net/en/docs/api/samv3/#version-31-changes
            const std::string cmd = "HELLO VERSION MIN=3.1 MAX=3.1\n";
            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
        }
    };

    // TODO use non-transient destinations
    struct client::send_session_create
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& id)
        {
            if (ec) return self->done(ec, self);

            /**
             * Reference for signature/leaseset types:
             * https://docs.i2pd.website/en/latest/user-guide/tunnels/#
             */
            std::string cmd;
            cmd.append("SESSION CREATE STYLE=STREAM ID=" + id + ' ');
            cmd.append("DESTINATION=TRANSIENT SIGNATURE_TYPE=7 ");
            cmd.append("i2cp.leaseSetEncType=6,4 inbound.quantity=2 outbound.quantity=2\n");

            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
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

    struct client::send_dest_generate
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec, self);
            //! Reference: https://i2p.net/en/docs/api/samv3/#signature-and-encryption-types
            const std::string cmd = "DEST GENERATE SIGNATURE_TYPE=7\n";
            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
        }
    };

    bool client::generate_destination(std::shared_ptr<client> self_, const stream_type::endpoint& router)
    {
        if (!self_) return false;

        client& self = *self_;

        self.socket_.async_connect(router,
            boost::asio::bind_executor(self.strand_, send_dest_generate{std::move(self_)}));

        return true;
    }

    struct client::send_naming_lookup
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec, const std::string& address)
        {
            if (ec) return self->done(ec, self);
            const std::string cmd = "NAMING LOOKUP NAME=" + address + '\n';
            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
        }
    };

    struct client::send_stream_connect
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec, self);

            const std::string cmd = "STREAM CONNECT ID=" + self->session_id_ + " DESTINATION=" + self->destination_ + " SILENT=FALSE\n";

            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
        }
    };

    struct client::send_stream_accept
    {
        std::shared_ptr<client> self;

        void operator()(boost::system::error_code ec)
        {
            if (ec) return self->done(ec, self);

            const std::string cmd = "STREAM ACCEPT ID=" + self->session_id_ + " SILENT=FALSE\n";

            boost::asio::async_write(self->socket_, boost::asio::buffer(cmd),
                boost::asio::bind_executor(self->strand_, read_line{self}));
        }
    };

    void client::next_state(const std::shared_ptr<client>& self, boost::system::error_code ec)
    {
        if (ec) return done(ec, self);

        switch (state_)
        {
        case state::hello_version:
            state_ = state::session_create;
            boost::asio::dispatch(strand_,
                [self](){ send_session_create{self}({}, self->session_id_); });
            return;

//        case state::dest_generate:
//            if (!naming_lookup_.empty())
//            {
//                state_ = state::naming_lookup;
//                boost::asio::dispatch(strand_,
//                    [self](){ send_naming_lookup{self}({}, self->naming_lookup_); });
//            }
//            else
//            {
//                state_ = state::session_create;
//                boost::asio::dispatch(strand_,
//                    [self](){ send_session_create{self}({}, self->session_id_, self->public_key_); });
//            }
//            return;

        case state::session_create:
            if (!naming_lookup_.empty())
            {
                state_ = state::naming_lookup;
                boost::asio::dispatch(strand_,
                    [self](){ send_naming_lookup{self}({}, self->naming_lookup_); });
            }
            else if (!destination_.empty())
            {
                state_ = state::stream_connect;
                boost::asio::dispatch(strand_,
                    [self](){ send_stream_connect{self}({}); });
            }
            else
            {
                state_ = state::stream_accept;
                boost::asio::dispatch(strand_,
                    [self](){ send_stream_accept{self}({}); });
            }
            return;

        case state::naming_lookup:
            state_ = state::stream_accept;
            boost::asio::dispatch(strand_,
                [self](){ send_stream_accept{self}({}); });
            return;

        case state::stream_accept:
            state_ = state::stream_connect;
            boost::asio::dispatch(strand_,
                [self](){ send_stream_connect{self}({}); });
            return;

        default:
            MINFO("Done with SAM loop");
            done({}, self);
            return;
        }

        MDEBUG("SAM state changed to: " << static_cast<int>(state_));
    }

    boost::system::error_code get_value(const std::string& line, const std::string& key, std::string& out)
    {
        std::string pattern = key + "=";
        std::size_t pos = line.find(pattern);
        if (pos == std::string::npos)
            return make_error_code(error::parse_failed);

        pos += pattern.length();

        std::size_t end = line.find(' ', pos);
        if (end == std::string::npos)
            end = line.length();

        out = line.substr(pos, end - pos);
        return {};
    }

    boost::system::error_code parse_result_code(const std::string& line)
    {
        std::string result;

        if (get_value(line, "RESULT", result))
            return make_error_code(error::parse_failed);

        if (result == "OK") return {};

        if (line.find("CANT_REACH_PEER") != std::string::npos)
            return make_error_code(error::cant_reach_peer);

        if (line.find("I2P_ERROR") != std::string::npos)
            return make_error_code(error::i2p_error);

        if (line.find("INVALID_KEY") != std::string::npos)
            return make_error_code(error::invalid_key);

        if (line.find("INVALID_ID") != std::string::npos)
            return make_error_code(error::invalid_id);

        if (line.find("TIMEOUT") != std::string::npos)
            return make_error_code(error::timeout);

        if (line.find("DUPLICATED_DEST") != std::string::npos)
            return make_error_code(error::duplicated_dest);

        if (line.find("KEY_NOT_FOUND") != std::string::npos)
            return make_error_code(error::key_not_found);

        if (line.find("PEER_NOT_FOUND") != std::string::npos)
            return make_error_code(error::peer_not_found);

        if (line.find("LEASESET_NOT_FOUND") != std::string::npos)
            return make_error_code(error::leaseset_not_found);

        if (line.find("NOVERSION") != std::string::npos)
            return make_error_code(error::noversion);

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

        case state::dest_generate:
        {
            boost::system::error_code ec1, ec2;

            ec1 = get_value(line, "PUB", public_key_);
            ec2 = get_value(line, "PRIV", private_key_);

            if (!ec1 && !ec2)
            {
                MDEBUG("Generated I2P destination PUB=" << public_key_);
                return{};
            }

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
                MDEBUG("Created session DESTINATION=" << destination_);
                return {};
            }

            return parse_result_code(line);
        }

        case state::stream_connect:
        case state::stream_accept:
        {
            return parse_result_code(line);
        }

        default:
            return make_error_code(error::parse_failed);
        }
    }

    bool client::set_connect_command(const net::i2p_address& address)
    {
        if (address.is_unknown())
            return false;

        set_naming_lookup(address.host_str());
        set_destination("");
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

    std::string client::private_key_from_file()
    {
        const std::string data_dir = tools::get_default_data_dir();
        const std::string key_path = data_dir + "/i2p_private_key";

        std::string private_key;

        if (epee::file_io_utils::is_file_exist(key_path))
        {
            if (!epee::file_io_utils::load_file_to_string(key_path, private_key))
                throw std::runtime_error("Failed to load I2P destination key from " + key_path);

            if (private_key.empty())
                throw std::runtime_error("I2P destination key is empty: " + key_path);
        }
        else
        {
            send_dest_generate();

            if (private_key.empty())
                throw std::runtime_error("Failed to generate I2P destination");

            if (!epee::file_io_utils::save_string_to_file(key_path, private_key))
                throw std::runtime_error("Failed to save I2P destination key to " + key_path);
        }

        return private_key;
    }
} // namespace net::sam
