// Copyright (c) 2026, The Monero Project
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

//paired header
#include "device_io_tcp.hpp"

//local headers
#include "int-util.h"
#include "misc_log_ex.h"
#include "net/net_helper.h"

//third party headers
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.io"

namespace hw {
namespace io {
//-------------------------------------------------------------------------------------------------------------------
device_io_tcp::device_io_tcp(boost::asio::io_context &io_context):
    m_io_context(io_context),
    m_socket(m_io_context),
    m_timeout(DEFAULT_TIMEOUT)
{}
//-------------------------------------------------------------------------------------------------------------------
device_io_tcp::~device_io_tcp()
{}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::init()
{
    // no global setup
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::connect(void *params)
{
    static const conn_params_t conn_params{}; // default params
    const conn_params_t &params_cref = params ? reinterpret_cast<const conn_params_t &>(params) : conn_params;
    this->connect(params_cref);
}
//-------------------------------------------------------------------------------------------------------------------
bool device_io_tcp::connected() const
{
    return this->m_socket.is_open();
}
//-------------------------------------------------------------------------------------------------------------------
int  device_io_tcp::exchange(
    unsigned char * const command,
    unsigned int cmd_len,
    unsigned char *response,
    unsigned int max_resp_len,
    const bool user_input)
{
    boost::asio::steady_timer deadline(this->m_io_context);
    boost::system::error_code ec = boost::asio::error::would_block;
    std::size_t bytes_transfered = 0;
    const auto io_cb = [&](const boost::system::error_code ec_, const std::size_t bytes_transfered_)
    {
        ec = ec_;
        bytes_transfered = bytes_transfered_;
    };
    const auto block_io = [&ec, this]()
    {
        while (ec == boost::asio::error::would_block)
        {
            this->m_io_context.restart();
            this->m_io_context.run_one();
        }
    };
    const auto set_deadline = [&ec, &deadline, this]()
    {
        deadline.expires_after(std::chrono::seconds(this->m_timeout));
        deadline.async_wait([&ec](const boost::system::error_code ec_){
            if (ec_ != boost::asio::error::operation_aborted) ec = boost::asio::error::operation_aborted;
        });
    };

    // if command to send, then send...
    if (nullptr != command && 0 != cmd_len)
    {
        set_deadline();

        // write [payload length as big-endian 32-bit int, payload bytes...]
        const std::uint32_t cmd_len_be = SWAP32BE(static_cast<std::uint32_t>(cmd_len));
        const std::array<boost::asio::const_buffer, 2> send_buffers{{
            {&cmd_len_be, sizeof(cmd_len_be)},
            {command, cmd_len}
        }};
        boost::asio::async_write(this->m_socket, send_buffers, io_cb);
        block_io();

        // check failure
        CHECK_AND_ASSERT_MES(!ec, -1, "Got error code while writing ADPU to TCP: " << ec);
        CHECK_AND_ASSERT_MES(bytes_transfered == cmd_len + sizeof(uint32_t),
            -1, "Transferred the wrong number of bytes to ADPU TCP: " << bytes_transfered
                << " vs " << (cmd_len + sizeof(uint32_t)));
    }

    // do first read, waiting indefinitely if user_input=true
    std::uint32_t n_read_bytes = -1;
    {
        ec = boost::asio::error::would_block;
        bytes_transfered = 0;
        if (!user_input)
            set_deadline();

        // read in [response payload length as big-endian 32-bit int, beginnig of payload...]
        const std::array<boost::asio::mutable_buffer, 2> read_buffers{{
            {&n_read_bytes, sizeof(n_read_bytes)},
            {response, max_resp_len}
        }};
        boost::asio::async_read(this->m_socket, read_buffers, boost::asio::transfer_at_least(4), io_cb);
        block_io();

        // check I/O failure
        CHECK_AND_ASSERT_MES(!ec, -1, "Got error code while performing TCP read: " << ec);

        MDEBUG("Read " << bytes_transfered << " bytes from the TCP APDU device from first read");

        // get payload length and check against buffer length...
        CHECK_AND_ASSERT_MES(bytes_transfered >= sizeof(n_read_bytes),
            -1, "Did not read enough bytes to get ADPU payload length");

        // recv length is encoded as a 32-bit big-endian int, minus 2 for some reason
        n_read_bytes = SWAP32BE(n_read_bytes);
        n_read_bytes += 2;
        MDEBUG("ADPU payload length will be " << n_read_bytes << " bytes");

        CHECK_AND_ASSERT_MES(n_read_bytes <= max_resp_len, -1, "Not enough space in buffer for APDU payload");

        // buffer length is shrunk to payload length
        max_resp_len = n_read_bytes;
        bytes_transfered -= 4;
    }

    do
    {
        CHECK_AND_ASSERT_MES(bytes_transfered <= max_resp_len, -1, "Buffer overflow occurred on APDU TCP read ;(");
        response += bytes_transfered;
        max_resp_len -= bytes_transfered;

        // if ended a read on payload length line, then we are done
        if (!max_resp_len)
            break;

        ec = boost::asio::error::would_block;
        bytes_transfered = 0;
        set_deadline();

        // read in [rest of payload bytes...]
        boost::asio::async_read(this->m_socket, boost::asio::mutable_buffer(response, max_resp_len), io_cb);
        block_io();

        // check I/O failure
        CHECK_AND_ASSERT_MES(!ec, -1, "Got error code while performing TCP read: " << ec);
    }
    while (true);

    return n_read_bytes;
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::disconnect()
{
    if (this->m_socket.is_open())
    {
        this->m_socket.shutdown(boost::asio::socket_base::shutdown_both);
        this->m_socket.close();
    }
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::release()
{
    // no global resources to release
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::connect(const char *address, unsigned int port)
{
    // set timeout timer
    boost::asio::steady_timer deadline(this->m_io_context);
    deadline.expires_after(std::chrono::seconds(this->m_timeout));

    // blocking wait for DNS resolution and connection
    auto socket_future = epee::net_utils::direct_connect()(address, std::to_string(port), deadline);
    for (;;)
    {
        this->m_io_context.restart();
        this->m_io_context.run_one();

        if (socket_future.is_ready())
            break;
    }

    this->m_socket = socket_future.get();
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::connect(const conn_params_t &conn_params)
{
    this->connect(conn_params.address, conn_params.port);
}
//-------------------------------------------------------------------------------------------------------------------
void device_io_tcp::set_timeout(unsigned int timeout)
{
    this->m_timeout = timeout;
}
//-------------------------------------------------------------------------------------------------------------------
}  //namespace io
}  //namespace hw
