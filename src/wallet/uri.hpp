// Copyright (c) 2018-2024, The Monero Project

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

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <cstdint>
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

#include <boost/multiprecision/cpp_int.hpp>
using uint128 = boost::multiprecision::uint128_t;

namespace tools {
namespace wallet {

struct tx_amount {
  uint128 amount;
  std::string currency;

  tx_amount() = default;
  tx_amount(uint128 amount, std::string currency) : amount(amount), currency(std::move(currency)) {}
};

std::string make_uri(const std::vector<std::string> &addresses, const std::vector<tx_amount> &amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type = cryptonote::UNDEFINED);
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<uint64_t> &xmr_amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type = cryptonote::UNDEFINED);
bool parse_uri(const std::string &uri, std::vector<std::string> &addresses, std::vector<tx_amount> &amounts, std::vector<std::string> &recipient_names, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type network_type = cryptonote::UNDEFINED);
bool parse_uri_to_dests(const std::string &uri, std::vector<cryptonote::tx_destination_entry> &destinations, std::optional<crypto::hash8> &short_payment_id, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type expected_network_type = cryptonote::network_type::UNDEFINED, std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm = nullptr);

} // namespace wallet
} // namespace tools
