// Copyright (c) 2014-2024, The Monero Project
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
#include "common/util.h"
#include "net/net_parse_helpers.h"
#include "ssl_options.h"

#include <boost/algorithm/string/predicate.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "WalletAPI"

using namespace std;

namespace Monero {
namespace Utils {

bool isAddressLocal(const std::string &address)
{ 
    try {
        return tools::is_local_address(address);
    } catch (const std::exception &e) {
        MERROR("error: " << e.what());
        return false;
    }
}

epee::net_utils::ssl_options_t sslOptionsForDaemon(const std::string &daemon_address, bool use_ssl)
{
    epee::net_utils::http::url_content parsed{};
    if (!epee::net_utils::parse_url(daemon_address, parsed))
        return epee::net_utils::ssl_support_t::e_ssl_support_autodetect;

    // clearnet keeps autodetect: use_ssl defaults to false, honouring it would drop TLS to https daemons
    if (!tools::is_privacy_preserving_network(parsed.host))
        return epee::net_utils::ssl_support_t::e_ssl_support_autodetect;

    const bool wants_ssl = use_ssl || boost::iequals(parsed.schema, "https", std::locale::classic());
    if (!wants_ssl)
    {
        MINFO("TLS handshake skipped for Tor/I2P daemon " << parsed.host);
        return epee::net_utils::ssl_support_t::e_ssl_support_disabled;
    }

    // no peer verification: this API cannot carry a CA or fingerprint yet, monerod's default
    // certificate is self-signed, and a .onion/.i2p name is self-authenticating (has_strong_verification)
    MINFO("TLS required for Tor/I2P daemon " << parsed.host << ", certificate verification off");
    epee::net_utils::ssl_options_t options(epee::net_utils::ssl_support_t::e_ssl_support_enabled);
    options.verification = epee::net_utils::ssl_verification_t::none;
    return options;
}

void onStartup()
{
    tools::on_startup();
#ifdef NDEBUG
    tools::disable_core_dumps();
#endif
}

}


} // namespace
