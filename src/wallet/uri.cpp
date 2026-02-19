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

#include "uri.hpp"
#include <algorithm>
#include <array>
#include <string_view>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include "wallet/wallet2.h"
#include "net/abstract_http_client.h"

namespace {
std::optional<cryptonote::address_parse_info> validate_address(const std::string &address, const cryptonote::network_type expected_network_type)
{
  const std::array<cryptonote::network_type, 4> nettypes = {
    cryptonote::network_type::MAINNET,
    cryptonote::network_type::TESTNET,
    cryptonote::network_type::STAGENET,
    cryptonote::network_type::FAKECHAIN
  };

  cryptonote::address_parse_info info;
  for (auto n : nettypes) {
    if (expected_network_type != cryptonote::network_type::UNDEFINED && n != expected_network_type)
      continue;

    if (cryptonote::get_account_address_from_str(info, n, address))
      return info;
  }
  return std::nullopt;
}
//----------------------------------------------------------------------------------------------------
struct currency_info {
  std::string name;
  unsigned decimals;
  uint128 scale;
};
//----------------------------------------------------------------------------------------------------
const std::array<currency_info, 5> currencies = {{
  {"BTC", 8, uint128("100000000") },
  {"ETH", 18, uint128("1000000000000000000") },
  {"EUR", 2, uint128("100") },
  {"USD", 2, uint128("100") },
  {"XMR", 12, uint128("1000000000000") },
}};
//----------------------------------------------------------------------------------------------------
const currency_info* get_currency_info(const std::string& currency)
{
  std::string up(currency);
  boost::trim(up);
  boost::to_upper(up);

  if (up.empty())
    return nullptr;

  for (auto& c : currencies) {
    if (c.name == up)
      return &c;
  }
  return nullptr;
}
//----------------------------------------------------------------------------------------------------
std::optional<std::string> parse_decimal_amount(const std::string& decimal_str, const currency_info& info, uint128& out_amount)
{
  std::string s(decimal_str);
  boost::trim(s);
  if (s.empty() || s == "." || s == "+." || s == "-.")
    return "Invalid amount format";
  if (s.front() == '+' || s.front() == '-')
    return "Signed amounts not supported";

  size_t dot = s.find('.');
  std::string integer_part = (dot == std::string::npos) ? s : s.substr(0, dot);
  std::string frac_part = (dot == std::string::npos) ? "" : s.substr(dot + 1);
  if (frac_part.size() > info.decimals)
    return "Too many fractional digits";

  uint128 integer_val = 0;
  if (!integer_part.empty()) {
    try { integer_val = uint128(integer_part); }
    catch (...) { return "Invalid integer part"; }
  }
  uint128 frac_val = 0;
  if (!frac_part.empty()) {
    try { frac_val = uint128(frac_part); }
    catch (...) { return "Invalid fractional part"; }
  }

  uint128 factor = info.scale;
  for (size_t i = 0; i < frac_part.size(); ++i)
    factor /= 10;

  const uint128 MAX = ~uint128(0);
  if (info.scale != 0 && integer_val > MAX / info.scale)
    return "Amount overflows 128-bit range";
  uint128 integer_scaled = integer_val * info.scale;
  if (factor != 0 && frac_val > MAX / factor)
    return "Amount overflows 128-bit range";
  uint128 frac_scaled = frac_val * factor;
  if (integer_scaled > MAX - frac_scaled)
    return "Amount overflows 128-bit range";
  out_amount = integer_scaled + frac_scaled;
  return std::nullopt;
}
//----------------------------------------------------------------------------------------------------
std::string format_amount(uint128 amount, const currency_info& info, bool currency_suffix = true)
{
  // preserve trailing zeroes in order to be backwards compatible with legacy make_uri
  if (info.name == "XMR")
    return cryptonote::print_money(amount, info.decimals) + (currency_suffix ? " XMR" : "");
  uint128 integer_part = amount / info.scale;
  uint128 frac_part = amount % info.scale;
  std::string out = integer_part.str();

  if (frac_part != 0) {
    std::string frac = frac_part.str();
    if (frac.size() < info.decimals)
      frac.insert(0, info.decimals - frac.size(), '0');

    while (!frac.empty() && frac.back() == '0')
      frac.pop_back();

    if (!frac.empty())
      out += "." + frac;
  }
  return currency_suffix ? out + " " + info.name : out;
}
//----------------------------------------------------------------------------------------------------
std::optional<std::string> parse_amount_string(std::string amount_str_with_unit, tools::wallet::tx_amount& out_tx_amount)
{
  const std::string amount_regex = R"([0-9]+(?:\.[0-9]+)?)", unit_regex = R"([A-Za-z]+)";
  static const boost::regex re("^\\s*(" + amount_regex + ")\\s*(" + unit_regex + ")?\\s*$");
  boost::smatch m;
  if (!boost::regex_match(amount_str_with_unit, m, re))
    return "Invalid amount format";

  const std::string raw_amount_str = m[1].str();
  std::string unit = m[2].matched ? m[2].str() : "XMR";
  auto currency = get_currency_info(unit);
  if (!currency)
    return "Unsupported currency: " + unit;

  if (currency->name == "XMR") {
    uint64_t atomic = 0;
    if (!cryptonote::parse_amount(atomic, raw_amount_str))
      return "Invalid XMR amount: " + raw_amount_str;
        
    out_tx_amount = tools::wallet::tx_amount{uint128(atomic), "XMR"};
    return std::nullopt;
  }
  else {
    uint128 amount;
    auto err = parse_decimal_amount(raw_amount_str, *currency, amount);
    if (err)
      return err;

    out_tx_amount = tools::wallet::tx_amount{amount, currency->name};
  }
  return std::nullopt;
}
} // anonymous namespace

namespace tools {
namespace wallet {

std::string make_uri(const std::vector<std::string> &addresses, const std::vector<tx_amount> &amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type)
{
  if (addresses.empty()) {
    error = "No recipient information like addresses are provided.";
    return {};
  }

  if (!validate_address(addresses[0], network_type)) {
    error = "Invalid address: " + addresses[0];
    return {};
  }

  std::string uri = "monero:" + addresses[0];
  std::vector<std::pair<std::string, std::string>> params;
  const bool single = (addresses.size() == 1);
  bool has_version = false;
  if ((addresses.size() > 1) || (single && !amounts.empty() && amounts[0].currency != "XMR")) {
    params.emplace_back("version", "2.0");
    has_version = true;
  }

  auto encode_value = [](const std::string &v) -> std::string {
    std::string out = epee::net_utils::conver_to_url_format(v);
    boost::algorithm::replace_all(out, "?", "%3F");
    boost::algorithm::replace_all(out, "=", "%3D");
    boost::algorithm::replace_all(out, "/", "%2F");
    return out;
  };

  for (size_t i = 0; i < addresses.size(); ++i) {
    if (i > 0) {
      if (!validate_address(addresses[i], network_type)) {
        error = "Invalid address: " + addresses[i];
        return {};
      }
      params.emplace_back("address", addresses[i]);
    }
    
    if (i < amounts.size() && amounts[i].amount > 0) {
      auto currency_info = get_currency_info(amounts[i].currency);
      if (!currency_info) {
        error = "Currency not found: " + amounts[i].currency;
        return {};
      }
      params.emplace_back((single && i == 0) ? "tx_amount" : "amount", format_amount(amounts[i].amount, *currency_info, has_version));
    }
                
    if (i < recipient_names.size() && !recipient_names[i].empty())
      params.emplace_back((single && i == 0) ? "recipient_name" : "label", encode_value(recipient_names[i]));
  }
        
  if (!tx_description.empty())
    params.emplace_back("tx_description", encode_value(tx_description));
  
  if (!params.empty()) {
    uri.push_back('?');
    bool first = true;
    for (auto &p : params) {
      if (!first) uri.push_back('&');
      first = false;
      uri += p.first + '=' + p.second;
    }
  }
  return uri;
}
//----------------------------------------------------------------------------------------------------
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<uint64_t> &xmr_amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type)
{
  std::vector<tx_amount> amounts;
  std::transform(xmr_amounts.begin(), xmr_amounts.end(), std::back_inserter(amounts),
    [](uint64_t amt) { return tx_amount{uint128(amt), "XMR"}; });
    
  return make_uri(addresses, amounts, recipient_names, tx_description, error, network_type);
}
//----------------------------------------------------------------------------------------------------
bool parse_uri(const std::string &uri, std::vector<std::string> &addresses, std::vector<tx_amount> &amounts, std::vector<std::string> &recipient_names, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type network_type)
{
  if (uri.rfind("monero:", 0) != 0) {
    error = "URI has wrong scheme (expected \"monero:\"): " + uri;
    return false;
  }

  std::string_view rest(uri);
  rest.remove_prefix(strlen("monero:"));

  std::string_view path, query;
  auto qpos = rest.find('?');
  if (qpos == std::string_view::npos)
    path = rest;
  else {
    path = rest.substr(0, qpos);
    query = rest.substr(qpos + 1);
  }

  if (path.empty()) {
    error = "URI missing address";
    return false;
  }
    
  auto info_opt = validate_address(std::string(path), network_type);
  if (!info_opt) {
    error = "Invalid address in URI";
    return false;
  }
  
  addresses.emplace_back(path);
  amounts.emplace_back(uint128(0), "XMR");
  recipient_names.emplace_back();
    
  bool integrated_seen = info_opt->has_payment_id;
  size_t current_output = 0;

  std::string_view qv = query;
  while (!qv.empty()) {
    size_t sep = qv.find_first_of("&");
    std::string_view part = (sep == std::string_view::npos) ? qv : qv.substr(0, sep);
    qv.remove_prefix((sep == std::string_view::npos) ? qv.size() : sep + 1);
    if (part.empty()) continue;
    
    size_t eq = part.find('=');
    std::string_view raw_key = part.substr(0, eq);
    std::string_view raw_val = (eq == std::string_view::npos) ? std::string_view{} : part.substr(eq + 1);
    std::string key = epee::net_utils::convert_from_url_format(std::string(raw_key));
    boost::to_lower(key);
    std::string value = epee::net_utils::convert_from_url_format(std::string(raw_val));
    
    if (key == "version") {
      if (value != "2.0") {
        error = "Unsupported version: " + value;
        return false;
      }
      continue;
    }
        
    if (key == "address") {
      auto addr_info = validate_address(value, network_type);
      if (!addr_info) {
        error = "Invalid address: " + value;
        return false;
      }
      
      if (addr_info->has_payment_id) {
        if (integrated_seen) {
          error = "Multiple integrated addresses not supported. ";
          return false;
        }
        integrated_seen = true;
      }
            
      addresses.push_back(value);
      amounts.push_back({uint128(0), "XMR"});
      recipient_names.push_back("");
      current_output = addresses.size() - 1;
      continue;
    }
        
    if (key == "amount" || key == "tx_amount") {
      tx_amount amt;
      auto err = parse_amount_string(value, amt);
      if (err) {
        error = *err;
        return false;
      }
      amounts[current_output] = amt;
      continue;
    }
        
    if (key == "label" || key == "recipient_name") {
      recipient_names[current_output] = value;
      continue;
    }
        
    if (key == "tx_description") {
      tx_description = value;
      continue;
    }
        
    unknown_parameters.push_back(key + "=" + value);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool parse_uri_to_dests(const std::string &uri, std::vector<cryptonote::tx_destination_entry> &destinations, std::optional<crypto::hash8> &short_payment_id, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type expected_network_type, std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm)
{
  std::vector<std::string> addresses, recipient_names;
  std::vector<tx_amount> amounts;
  std::string tx_payment_id;

  if (!parse_uri(uri, addresses, amounts, recipient_names, tx_description, unknown_parameters, error, expected_network_type))
    return false;

  destinations.clear();
  short_payment_id.reset();

  auto it = std::find_if(unknown_parameters.begin(), unknown_parameters.end(), 
    [](const std::string& p) { return p.rfind("tx_payment_id=", 0) == 0; });
  if (it != unknown_parameters.end()) {
    tx_payment_id = it->substr(strlen("tx_payment_id="));
    unknown_parameters.erase(it);
  }

  auto get_address_info = [&](const std::string& addr, cryptonote::address_parse_info& out) -> bool {
    if (dns_confirm)
      return cryptonote::get_account_address_from_str_or_url(out, expected_network_type, addr, dns_confirm);
    if (auto info_opt = validate_address(addr, expected_network_type)) {
      out = *info_opt;
      return true;
    }
    return false;
  };


  for (size_t i = 0; i < addresses.size(); ++i)
  {
    cryptonote::address_parse_info info;
    if (!get_address_info(addresses[i], info)) {
      error = "Failed to validate address: " + addresses[i];
      return false;
    }

    if (info.has_payment_id) {
      if (!short_payment_id || *short_payment_id == info.payment_id)
        short_payment_id = info.payment_id;
      else {
        error = "Multiple integrated addresses with different payment IDs are not supported";
        return false;
      }
    }

    if (amounts[i].currency != "XMR") {
      error = "parse_uri_to_dests only supports XMR amounts";
      return false;
    }

    if (amounts[i].amount > std::numeric_limits<uint64_t>::max()) {
      error = "XMR amount in URI exceeds uint64 range";
      return false;
    }
    
    cryptonote::tx_destination_entry dest;
    dest.amount = static_cast<uint64_t>(amounts[i].amount);
    dest.addr = info.address;
    dest.is_subaddress = info.is_subaddress;
    dest.original = addresses[i];
    dest.is_integrated = info.has_payment_id;
    destinations.push_back(dest);
  }

  if (!tx_payment_id.empty()) {
    if (destinations.size() > 1) {
      error = "A single transaction-level payment id (tx_payment_id) cannot be used with multiple outputs";
      return false;
    }

    if (short_payment_id) {
      error = "Both integrated address and tx_payment_id parameter cannot be used together";
      return false;
    }

    crypto::hash8 pid8;
    crypto::hash pid32;

    if (tools::wallet2::parse_short_payment_id(tx_payment_id, pid8)) {
      short_payment_id = pid8;
    } else if (tools::wallet2::parse_long_payment_id(tx_payment_id, pid32)) {
      error = "Long payment IDs are obsolete and not supported";
      return false;
    } else {
      error = "Invalid payment ID format in tx_payment_id parameter";
      return false;
    }
  }

  return true;
}
} // namespace wallet
} // namespace tools
