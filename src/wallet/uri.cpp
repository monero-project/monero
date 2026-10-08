// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
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

#include "uri.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"

namespace tools
{
namespace wallet
{
namespace
{
  bool validate_recipients(const payment_uri& payment, cryptonote::network_type nettype, std::string& error)
  {
    if (nettype != cryptonote::MAINNET && nettype != cryptonote::TESTNET && nettype != cryptonote::STAGENET)
    {
      error = "An explicit mainnet, testnet or stagenet network is required";
      return false;
    }
    for (const auto& recipient : payment.recipients)
    {
      cryptonote::address_parse_info info;
      if (!cryptonote::get_account_address_from_str(info, nettype, recipient.address))
      {
        error = "URI has wrong address: " + recipient.address;
        return false;
      }
    }
    return true;
  }
}

  std::string make_uri_multi(const payment_uri& payment, cryptonote::network_type nettype, std::string& error)
  {
    error.clear();
    if (!validate_recipients(payment, nettype, error))
      return {};
    return make_uri_syntax(payment, error);
  }

  bool parse_uri_multi(const std::string& uri, cryptonote::network_type nettype, payment_uri& payment, std::string& error)
  {
    payment = {};
    error.clear();
    payment_uri parsed;
    if (!parse_uri_syntax(uri, parsed, error) || !validate_recipients(parsed, nettype, error))
      return false;
    payment = std::move(parsed);
    return true;
  }
}
}
