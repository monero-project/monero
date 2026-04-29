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

// Ledger APDU I/O interface for TCP, typically used by Speculos

#pragma once

//local headers
#include "device_io.hpp"

//third party headers
#include <boost/asio/ip/tcp.hpp>

//standard headers

//forward declarations

namespace hw {
namespace io {
/** Speculous TCP commands are formatted as follows:
 *
 * |---------------------------------------------|
 * |           4 bytes              |  len bytes |
 * |--------------------------------|------------|
 * |  len of payload as big-endian  |  payload   |
 * |---------------------------------------------|
 */

class device_io_tcp final: public device_io
{
public:
    static constexpr const char DEFAULT_ADDRESS[] = "127.0.0.1";
    static constexpr unsigned int DEFAULT_PORT = 9999;
    static constexpr unsigned int DEFAULT_TIMEOUT = 60; // seconds

    struct conn_params_t
    {
        const char *address = DEFAULT_ADDRESS;
        unsigned int port = DEFAULT_PORT;
    };

    device_io_tcp(boost::asio::io_context &io_context);
    ~device_io_tcp();

    void init() final;
    void connect(void *params) final;
    bool connected() const final;
    int  exchange(unsigned char *command,
        unsigned int cmd_len,
        unsigned char *response,
        unsigned int max_resp_len,
        bool user_input) final;
    void disconnect() final;
    void release() final;

    void connect(const char *address, unsigned int port);
    void connect(const conn_params_t &conn_params);
    void set_timeout(unsigned int timeout);

private:
    boost::asio::io_context &m_io_context;
    boost::asio::ip::tcp::socket m_socket;
    unsigned int m_timeout;
}; //class device_io_tcp
}  //namespace io
}  //namespace hw
