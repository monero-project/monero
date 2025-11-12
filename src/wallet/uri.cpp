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
#include <set>
#include <algorithm>
#include <unordered_map>
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp> 
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include "wallet/wallet2.h"
#include "net/http_client.h"

namespace tools {
namespace wallet {

//----------------------------------------------------------------------------------------------------
uint128 max_u128() 
{
#if HAVE_INT128
    return (uint128)(~(uint128)0);
#else
    uint128 m = (uint128)0;
    m = ~m;
    return m;
#endif
}
//----------------------------------------------------------------------------------------------------
std::string to_string_u128(uint128 v)
{
#if HAVE_INT128
  if (v == 0) 
  {
    return "0";
  }

  std::string s;
  while (v > 0) 
  {
    unsigned digit = static_cast<unsigned>(v % 10);
    s.push_back(char('0' + digit));
    v /= 10;
  }
  std::reverse(s.begin(), s.end());
  return s;
#else
  return v.convert_to<std::string>();
#endif
}
//----------------------------------------------------------------------------------------------------
bool parse_u128(const std::string &u128_str, uint128 &out)
{
  std::string s = boost::algorithm::trim_copy(u128_str);
  if (s.empty())
  {
    return false;
  }

  size_t i = 0;
  if (s[i] == '+')
  {
    ++i;
    if (i >= s.size()) 
    {
      return false;
    }
  }
  if (s[i] == '-') 
  {
    return false; 
  }

  out = (uint128)0;
  const uint128 U128_MAX = tools::wallet::max_u128();
  const uint128 max_div_10 = U128_MAX / (uint128)10;
  const uint128 max_mod_10 = U128_MAX % (uint128)10;
  for (; i < s.size(); ++i)
  {
    char c = s[i];
    if (c < '0' || c > '9') 
    {
      return false;
    }
    uint128 digit = (uint128)(c - '0');
    if (out > max_div_10)
    {
      return false;
    }
    if (out == max_div_10 && digit > max_mod_10) 
    {
      return false;
    }
    out = out * (uint128)10 + digit;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
} // namespace wallet
} // namespace tools

namespace {
std::string convert_to_url_format(const std::string &uri)
{
  std::string s = epee::net_utils::conver_to_url_format(uri);

  // replace '=' with "%3D" and '?' with "%3F"
  std::string result;
  result.reserve(s.size());

  for (char c : s)
  {
    if (c == '=')
    {
      result.append("%3D");
    }
    else if (c == '?')
    {
      result.append("%3F");
      }
      
    else
    {
      result.push_back(c);
    }      
  }

  return result;
}
//----------------------------------------------------------------------------------------------------
bool validate_address(const std::string &address, const cryptonote::network_type expected_network_type, bool* is_integrated = nullptr, cryptonote::address_parse_info* info_out = nullptr)
{
  cryptonote::address_parse_info info;

  const std::array<cryptonote::network_type, 4> nettypes = {
    cryptonote::network_type::MAINNET,
    cryptonote::network_type::TESTNET,
    cryptonote::network_type::STAGENET,
    cryptonote::network_type::FAKECHAIN
  };

  std::vector<cryptonote::network_type> nettypes_to_check;
  if (expected_network_type == cryptonote::network_type::UNDEFINED) 
  {
    nettypes_to_check.assign(nettypes.begin(), nettypes.end());
  } 
  else 
  {
    nettypes_to_check.push_back(expected_network_type);
  }

  for (auto n : nettypes_to_check)
  {
    if (cryptonote::get_account_address_from_str(info, n, address))
    {
      if (is_integrated) 
        *is_integrated = info.has_payment_id;
      if (info_out) 
        *info_out = info;
      return true;
    }
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
struct currency_info {
  unsigned decimals;
  uint64_t scale;
};
//----------------------------------------------------------------------------------------------------
const std::unordered_map<std::string, currency_info> &currency_table() {
  static const std::unordered_map<std::string, currency_info> table = {
    {"XMR", {12, 1000000000000ULL}},
    {"BTC", {8,  100000000ULL}},
    {"ETH", {18, 1000000000000000000ULL}},
    {"USD", {2,  100ULL}},
    {"EUR", {2,  100ULL}},
  };
  return table;
}
//----------------------------------------------------------------------------------------------------
bool get_currency_scale_and_decimals(const std::string &currency, uint128 &scale_out, unsigned &decimals_out)
{
  auto it = currency_table().find(currency);
  if (it == currency_table().end()) 
  {
    return false;
  }
  decimals_out = it->second.decimals;
  scale_out = (uint128)it->second.scale;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool map_unit_suffix_to_currency(const std::string &unit, std::string &currency_out)
{
  if (unit.empty()) 
  {
    currency_out = "XMR";
    return true;
  }
  std::string up = unit;
  std::transform(up.begin(), up.end(), up.begin(),
    [](unsigned char c){ return std::toupper(c); });

  auto it = currency_table().find(up);
  if (it == currency_table().end()) 
  {
    return false;
  }
  currency_out = up;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool parse_decimal_to_minor_units(const std::string &decimal_str, const std::string &currency, uint128 &out_amount, std::string &error)
{
  std::string s = decimal_str;
  boost::trim(s);
  if (s.empty())
  {
    error = "Empty amount value";
    return false;
  }

  size_t dot = s.find('.');
  std::string integer_part = (dot == std::string::npos) ? s : s.substr(0, dot);
  std::string fractional_part = (dot == std::string::npos) ? std::string() : s.substr(dot + 1);

  if (integer_part.empty()) integer_part = "0";

  if (!std::all_of(integer_part.begin(), integer_part.end(), [](unsigned char c){ return std::isdigit(c); })) 
  {
    error = "Invalid characters in amount integer part";
    return false;
  }
  
  if (!fractional_part.empty() && !std::all_of(fractional_part.begin(), fractional_part.end(), [](unsigned char c){ return std::isdigit(c); })) 
  {
    error = "Invalid characters in amount fractional part";
    return false;
  }

  auto it = currency_table().find(currency);
  if (it == currency_table().end()) 
  {
    error = std::string("Unsupported currency: ") + currency;
    return false;
  }

  unsigned max_decimals = it->second.decimals;
  uint128 scale = (uint128)it->second.scale;

  if (fractional_part.size() > max_decimals) {
    error = std::string("Too many fractional digits for ") + currency + " (max " + std::to_string(max_decimals) + ")";
    return false;
  }

  uint128 ip = 0;
  for (char c : integer_part) 
  {
    ip = ip * (uint128)10 + (uint128)(c - '0');
  }

  uint128 fp = 0;
  for (char c : fractional_part) 
  {
    fp = fp * (uint128)10 + (uint128)(c - '0');
  }

  unsigned pad = max_decimals - static_cast<unsigned>(fractional_part.size());
  for (unsigned i = 0; i < pad; ++i)
  {
    fp *= (uint128)10;
  }

  uint128 U128MAX = tools::wallet::max_u128();

  if (ip != (uint128)0) {
    if (ip > U128MAX / scale) {
      error = "Integer part too large (would overflow)";
      return false;
    }
  }
  uint128 total = ip * scale;

  if (fp > U128MAX - total) {
    error = "Amount fractional part causes overflow";
    return false;
  }
  total += fp;

  out_amount = total;
  return true;
}
//----------------------------------------------------------------------------------------------------
std::string format_minor_units_to_decimal_and_unit(uint128 minor_units, const std::string &currency)
{
  uint128 scale;
  unsigned max_decimals;
  if (!get_currency_scale_and_decimals(currency, scale, max_decimals))
  {
    return tools::wallet::to_string_u128(minor_units) + currency;
  }

  uint128 integer_part = minor_units / scale;
  uint128 fractional_part = minor_units % scale;
  std::string integer_part_str = tools::wallet::to_string_u128(integer_part);
  if (max_decimals == 0) 
  {
    return integer_part_str + currency;
  }

  std::string frac_str = tools::wallet::to_string_u128(fractional_part);

  if (frac_str.size() < max_decimals) 
  {
    std::string pad(max_decimals - frac_str.size(), '0');
    frac_str = pad + frac_str;
  }

  size_t last_nonzero = frac_str.find_last_not_of('0');
  if (last_nonzero == std::string::npos) 
  {
    return integer_part_str + currency;
  }

  std::string frac_trimmed = frac_str.substr(0, last_nonzero + 1);
  return integer_part_str + "." + frac_trimmed + currency;
}
//----------------------------------------------------------------------------------------------------
bool parse_amount_string_to_tx_amount(const std::string &amount_str, tools::wallet::tx_amount &out, std::string &error)
{
  std::string s = amount_str;
  boost::trim(s);
  if (s.empty()) 
  { 
    error = "Empty amount string";
    return false; 
  }

  size_t pos = s.size();
  while (pos > 0 && std::isalpha(static_cast<unsigned char>(s[pos - 1]))) --pos;

  std::string number = s.substr(0, pos);
  std::string unit = s.substr(pos);

  std::string unit_upper = unit;
  boost::to_upper(unit_upper);
  
  std::string currency;
  if (unit_upper.empty())
  {
    currency = "XMR";
  }
  else 
  {
    if (!map_unit_suffix_to_currency(unit_upper, currency)) 
    {
      error = std::string("Unsupported amount unit: ") + unit_upper;
      return false;
    }
  }

  if (currency == "XMR") {
    if (number.empty()) 
    {
      error = "Invalid XMR amount";
      return false;
    }
    boost::trim(number);

    uint64_t atomic = 0;
    if (!cryptonote::parse_amount(atomic, number)) 
    {
      error = std::string("Invalid XMR amount: ") + number;
      return false;
    }
    
    out = { (uint128)atomic, "XMR" };
    return true;
  }

  uint128 minor_units = 0;
  if (!parse_decimal_to_minor_units(number, currency, minor_units, error))
  {
    return false;
  }
  
  out = { minor_units, currency };
  return true;
}
//----------------------------------------------------------------------------------------------------
bool format_tx_amount_for_uri(const tools::wallet::tx_amount &amt, std::string &out, std::string &error)
{
  if (amt.amount == (uint128)0)
  {
    out = std::string();
    return true;
  }

  if (amt.currency == "XMR") 
  {
    const uint64_t U64MAX = std::numeric_limits<uint64_t>::max();
    if (amt.amount > (uint128)U64MAX)
    {
      error = "XMR amount too large to encode in URI";
      return false;
    }
        
    std::string s = cryptonote::print_money(static_cast<uint64_t>(amt.amount));
    auto trim_trailing_zeros_from_decimal = [](std::string s) -> std::string {
      auto dot = s.find('.');
      if (dot == std::string::npos)
      {
        return s;
      }
      size_t last = s.find_last_not_of('0');
      if (last == std::string::npos || last < dot)
      {
        return s.substr(0, dot);
      }
      return s.substr(0, last + 1);
    };

    s = trim_trailing_zeros_from_decimal(s);
    out = s + std::string("XMR");
    return true;
  }
  
  out = format_minor_units_to_decimal_and_unit(amt.amount, amt.currency);
  return true;
}
} // anonymous namespace

namespace tools {
namespace wallet {

//----------------------------------------------------------------------------------------------------
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<tx_amount> &amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type)
{
  if (addresses.empty())
  {
    error = "No recipient information like addresses are provided.";
    return std::string();
  }

  std::vector<tx_amount> local_amounts;
  local_amounts.reserve(addresses.size());

  if (amounts.empty())
  {
    tx_amount zero = { (uint128)0, "XMR" };
    for (size_t i = 0; i < addresses.size(); ++i) 
    {
      local_amounts.push_back(zero);
    }
  }
  else
  {
    local_amounts = amounts;
    if (local_amounts.size() < addresses.size())
    {
      std::string pad_currency = local_amounts[0].currency.empty() ? std::string("XMR") : local_amounts[0].currency;
      tx_amount zero = { (uint128)0, pad_currency};
      for (size_t i = local_amounts.size(); i < addresses.size(); ++i) 
      {
        local_amounts.push_back(zero);
      }
    }
  }

  std::vector<std::string> local_recipient_names = recipient_names;
  if (local_recipient_names.size() < addresses.size())
  {
    local_recipient_names.resize(addresses.size(), std::string());
  }

  if (addresses.size() != local_amounts.size() || addresses.size() != local_recipient_names.size())
  {
    error = (boost::format("The counts of addresses (%1%), amounts (%2%), and recipient names (%3%) do not match (even after padding).") % addresses.size() % amounts.size() % recipient_names.size()).str();
    return std::string();
  }

  if (addresses.size() == 1)
  {
    const std::string &address = addresses[0];
    if (!validate_address(address, network_type))
    {
      error = std::string("wrong address: ") + address;
      return std::string();
    }

    std::string uri = "monero:" + address;
    unsigned int n_fields = 0;

    if (local_amounts[0].amount > 0)
    {
      if (local_amounts[0].currency != "XMR")
      {
        error = "Single recipient URI cannot have a currency apart from XMR due to compatibility issues.";
        return std::string();
      }

      const uint64_t U64MAX = std::numeric_limits<uint64_t>::max();
      if (local_amounts[0].amount > (uint128)U64MAX)
      {
        error = "XMR amount too large to encode in URI";
        return std::string();
      }

      // URI encoded amount is in decimal units, not atomic units
      uri += (n_fields++ ? "&" : "?") + std::string("tx_amount=") + cryptonote::print_money(local_amounts[0].amount);
    }

    if (!recipient_names[0].empty())
    {
      uri += (n_fields++ ? "&" : "?") + std::string("recipient_name=") + convert_to_url_format(local_recipient_names[0]);
    }

    if (!tx_description.empty())
    {
      uri += (n_fields++ ? "&" : "?") + std::string("tx_description=") + convert_to_url_format(tx_description);
    }

    return uri;
  }

  std::string uri = "monero:" + addresses[0];
  bool first_param = true;

  uri += (first_param ? "?" : "&");
  uri += "version=2.0";
  first_param = false;

  if (local_amounts[0].amount > 0)
  {
    uri += (first_param ? "?" : "&");
    first_param = false;
    std::string amount_str;
    if (!format_tx_amount_for_uri(local_amounts[0], amount_str, error))
    {
        return std::string();
    }
    uri += std::string("amount=") + amount_str;
  }

  if (!local_recipient_names[0].empty())
  {
    uri += (first_param ? "?" : "&");
    first_param = false;
    uri += std::string("label=") + convert_to_url_format(local_recipient_names[0]);
  }

  bool integrated_address_seen = false;
  // subsequent outputs
  for (size_t i = 1; i < addresses.size(); ++i)
  {
    const std::string &address = addresses[i];
    bool is_integrated = false;
    if (!validate_address(address, network_type, &is_integrated))
    {
      error = std::string("wrong address: ") + address;
      return std::string();
    }

    if (is_integrated)
    {
      if (integrated_address_seen)
      {
        error = "Multiple integrated addresses are not supported";
        return std::string();
      }
      integrated_address_seen = true;
    }

    uri += (first_param ? "?" : "&");
    first_param = false;
    uri += std::string("address=") + address;

    if (local_amounts[i].amount > 0)
    {
      std::string amount_str;
      if (!format_tx_amount_for_uri(local_amounts[i], amount_str, error))
      {
        return std::string();
      }
      uri += std::string("&amount=") + amount_str;
    }

    if (!local_recipient_names[i].empty())
    {
      uri += std::string("&label=") + convert_to_url_format(local_recipient_names[i]);
    }
  }

  if (!tx_description.empty())
  {
    uri += (first_param ? "?" : "&");
    uri += std::string("tx_description=") + convert_to_url_format(tx_description);
  }

  return uri;
}
//----------------------------------------------------------------------------------------------------
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<uint64_t> &xmr_amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, const cryptonote::network_type network_type)
{
  std::vector<tx_amount> amounts;
  amounts.reserve(xmr_amounts.size());

  for (auto amount : xmr_amounts)
  {
    tx_amount a = { static_cast<uint128>(amount), "XMR" };
    amounts.push_back(std::move(a));
  }

  return make_uri(addresses, amounts, recipient_names, tx_description, error, network_type);
}
//----------------------------------------------------------------------------------------------------
bool parse_uri(const std::string &uri, std::vector<std::string> &addresses, std::vector<tx_amount> &amounts, std::vector<std::string> &recipient_names, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type network_type)
{
  if (uri.rfind("monero:", 0) != 0) 
  {
    error = "URI has wrong scheme (expected \"monero:\"): " + uri;
    return false;
  }

  std::string remainder = uri.substr(7);
  std::size_t qpos = remainder.find('?');
  std::string path = (qpos == std::string::npos) ? remainder : remainder.substr(0, qpos);
  std::string query = (qpos == std::string::npos) ? "" : remainder.substr(qpos + 1);

  addresses.clear();
  amounts.clear();
  recipient_names.clear();
  tx_description.clear();
  unknown_parameters.clear();

  std::vector<std::string> args;
  if (!query.empty())
  {
    boost::split(args, query, boost::is_any_of("&"));
  }

  if (path.empty())
  {
    error = "URI missing initial monero address";
    return false;
  }

  bool integrated_address_seen = false;
  if (!validate_address(path, network_type, &integrated_address_seen))
  {
    error = std::string("URI contains improper address: ") + path;
    return false;
  }

  addresses.push_back(path);
  tx_amount zero_amount = { (uint128)0, "XMR" };
  amounts.push_back(zero_amount);
  recipient_names.push_back("");

  std::vector<bool> amount_set, label_set;
  amount_set.push_back(false);
  label_set.push_back(false);
  
  
  if (args.empty())
  {
    // no query: single bare address
    return true;
  }

  if (!args.empty())
  {
    std::string first = args[0];
    auto eq = first.find('=');
    if (eq != std::string::npos)
    {
      std::string key = first.substr(0, eq);
      std::string value = first.substr(eq + 1);
      if (key == "version" && value != "2.0")
      {
        error = std::string("Unsupported version: ") + value;
        return false;
      }
    }
  }

  size_t current_output = 0;
  bool tx_description_seen = false;

  for (size_t i = 0; i < args.size(); ++i)
  {
    const std::string &arg = args[i];
    if (arg.empty())
    {
      continue;
    }

    auto eq = arg.find('=');
    if (eq == std::string::npos)
    {
      error = "Bad parameter: " + arg;
      return false;
    }
    std::string key = arg.substr(0, eq);
    std::string value = arg.substr(eq + 1);

    if (key == "version")
    {
      if (i != 0)
      {
        error = "Version parameter must appear first when present";
        return false;
      }
      continue;
    }

    if (key == "address")
    {
      if (value.empty())
      {
        error = "Address parameter missing address value";
        return false;
      }

      bool is_integrated = false;
      if (!validate_address(value, network_type, &is_integrated))
      {
        error = std::string("URI contains improper address: ") + value;
        return false;
      }

      if (is_integrated)
      {
        if (integrated_address_seen)
        {
          error = "Multiple integrated addresses are not supported";
          return false;
        }
        integrated_address_seen = true;
      }

      addresses.push_back(value);
      amounts.push_back(zero_amount);
      recipient_names.push_back("");
      amount_set.push_back(false);
      label_set.push_back(false);
      current_output = addresses.size() - 1;
    }
    else if (key == "amount" || key == "tx_amount")
    {
      tx_amount t = { 0, "XMR" };
      if (!parse_amount_string_to_tx_amount(value, t, error))
      {
        return false;
      }

      if (amount_set[current_output]) 
      {
        error = "Duplicate amount for the same output"; 
        return false;
      }
      amounts[current_output] = t;
      amount_set[current_output] = true;
    }
    else if (key == "label" || key == "recipient_name")
    {
      if (label_set[current_output])
      {
        error = "Duplicate label for same output";
        return false;
      }
      recipient_names[current_output] = epee::net_utils::convert_from_url_format(value);
      label_set[current_output] = true;
    }
    else if (key == "tx_description")
    {
      if (tx_description_seen)
      {
        error = "Duplicate tx_description parameter";
        return false;
      }
      tx_description = epee::net_utils::convert_from_url_format(value);
      tx_description_seen = true;
    }
    else
    {
      unknown_parameters.push_back(arg);
    }
  }

  if (addresses.empty())
  {
    error = "At least one address required";
    return false;
  }

  if (addresses.size() != amounts.size() || addresses.size() != recipient_names.size())
  {
    error = "Internal error: parsed output vector sizes mismatch.";
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool parse_uri_to_dests(const std::string &uri, std::vector<cryptonote::tx_destination_entry> &destinations, std::optional<crypto::hash8> &short_payment_id, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, const cryptonote::network_type expected_network_type, std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm)
{
  std::vector<std::string> addresses;
  std::vector<tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string tx_payment_id;

  if (!parse_uri(uri, addresses, amounts, recipient_names, tx_description, unknown_parameters, error, expected_network_type))
  {
    return false;
  }

  destinations.clear();
  short_payment_id.reset();

  for (auto it = unknown_parameters.begin(); it != unknown_parameters.end(); )
  {
    if (boost::starts_with(*it, "tx_payment_id="))
    {
      tx_payment_id = it->substr(std::string("tx_payment_id=").size());
      it = unknown_parameters.erase(it);
    }
    else
    {
      ++it;
    }
  }

  for (size_t i = 0; i < addresses.size(); ++i)
  {
    cryptonote::address_parse_info info;
    if (dns_confirm) 
    {
      if (!cryptonote::get_account_address_from_str_or_url(info, expected_network_type, addresses[i], dns_confirm))
      {
        error = std::string("Failed to validate address: ") + addresses[i];
        return false;
      }
    }
    else if (!validate_address(addresses[i], expected_network_type, nullptr, &info))
    {
      error = std::string("Failed to validate address: ") + addresses[i];
      return false;
    }

    if (info.has_payment_id)
    {
      if (!short_payment_id)
      {
        short_payment_id = info.payment_id;
      }
      else if (*short_payment_id != info.payment_id)
      {
        error = "Multiple integrated addresses with different payment IDs are not supported";
        return false;
      }
    }

    if (amounts[i].currency != "XMR")
    {
      error = "parse_uri_to_dests only supports XMR amounts";
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

  if (!tx_payment_id.empty())
  {
    if (destinations.size() > 1)
    {
      error = "A single transaction-level payment id (tx_payment_id) cannot be used with multiple outputs";
      return false;
    }

    if (short_payment_id)
    {
      error = "Both integrated address and tx_payment_id parameter cannot be used together";
      return false;
    }

    crypto::hash8 short_pid;
    if (tools::wallet2::parse_short_payment_id(tx_payment_id, short_pid))
    {
      short_payment_id = short_pid;
    }
    else
    {
      crypto::hash long_pid;
      if (tools::wallet2::parse_long_payment_id(tx_payment_id, long_pid))
      {
        error = "Long payment IDs are obsolete and not supported";
        return false;
      }
      else
      {
        error = "Invalid payment ID format in tx_payment_id parameter";
        return false;
      }
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
} // namespace wallet
} // namespace tools
