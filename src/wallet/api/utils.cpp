// Copyright (c) 2014-2016, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers



#include "include_base_utils.h"                     // LOG_PRINT_x
#include "net/http_client.h"                        // epee::net_utils::...
#include <boost/asio.hpp>

using namespace std;

namespace Monero {
namespace Utils {


// copy-pasted from simplewallet.

bool isAddressLocal(const std::string &address)
{
    // extract host
    epee::net_utils::http::url_content u_c;
    if (!epee::net_utils::parse_url(address, u_c))
    {
        LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
        return false;
    }
    if (u_c.host.empty())
    {
        LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
        return false;
    }
    // resolver::resolve can throw an exception
    try {
        // resolve to IP
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(u_c.host, "", boost::asio::ip::tcp::resolver::query::canonical_name);
        boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
        while (i != boost::asio::ip::tcp::resolver::iterator())
        {
            const boost::asio::ip::tcp::endpoint &ep = *i;
            if (ep.address().is_loopback())
                return true;
            ++i;
        }
    } catch (const boost::system::system_error &e) {
        LOG_ERROR("Failed to resolve " << address << ", :" << e.what());
    }

    return false;
}

}


} // namespace

namespace Bitmonero = Monero;
