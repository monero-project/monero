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
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp> 
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "net/http_client.h"

static std::string convert_to_url_format(const std::string &uri)
{
  std::string s = epee::net_utils::conver_to_url_format(uri);

  // replace '=' with "%3D" and '?' with "%3F".
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
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<uint64_t> &amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error)
{
  if (addresses.empty())
  {
    error = "No recipient information like addresses are provided.";
    return std::string();
  }

  if (addresses.size() != amounts.size() || addresses.size() != recipient_names.size())
  {
    error = (boost::format("The counts of addresses (%1%), amounts (%2%), and recipient names (%3%) do not match.") % addresses.size() % amounts.size() % recipient_names.size()).str();
    return std::string();
  }

  std::string uri = "monero:";
  bool first_param = true;

  for (size_t i = 0; i < addresses.size(); ++i)
  {
    std::string out_val;
    out_val.reserve(256);
    
    const std::string &address = addresses[i];
    cryptonote::address_parse_info info;
    if (!get_account_address_from_str(info, cryptonote::MAINNET, address))
    {
      error = std::string("wrong address: ") + address;
      return std::string();
    }
    out_val.append(address);
    out_val.push_back(';');

    if (amounts[i] > 0)
      out_val += cryptonote::print_money(amounts[i]);
    out_val.push_back(';');

    if (!recipient_names[i].empty())
      out_val += convert_to_url_format(recipient_names[i]);

    uri += (first_param ? "?" : "&");
    first_param = false;
    uri.append("output=");
    uri += out_val;
  }

  if (!tx_description.empty())
  {
    uri += (first_param ? "?" : "&");
    uri += std::string("tx_description=") + convert_to_url_format(tx_description);
  }

  return uri;
}
//----------------------------------------------------------------------------------------------------
bool parse_uri(const std::string &uri, std::vector<std::string> &addresses, std::vector<uint64_t> &amounts, std::vector<std::string> &recipient_names, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error)
{
  if (uri.substr(0, 7) != "monero:")
  {
    error = std::string("URI has wrong scheme (expected \"monero:\"): ") + uri;
    return false;
  }

  std::string remainder = uri.substr(7);
  std::size_t qpos = remainder.find('?');
  std::string query = (qpos == std::string::npos) ? std::string() : remainder.substr(qpos + 1);

  addresses.clear();
  amounts.clear();
  recipient_names.clear();
  tx_description.clear();
  unknown_parameters.clear();

  // split query into key=value pairs
  std::vector<std::string> args;
  boost::split(args, query, boost::is_any_of("&"));

  for (const std::string &arg : args)
  {
    if (arg.empty())
      continue;

    std::size_t eq = arg.find('=');
    if (eq == std::string::npos)
    {
      error = std::string("URI has wrong parameter: ") + arg;
      return false;
    }
    std::string k = arg.substr(0, eq);
    std::string v = arg.substr(eq + 1);

    if (k == "output")
    {
      // expected form: <address>;<amount_dec_or_empty>;<recipient_name_or_empty>
      std::vector<std::string> fields;
      boost::split(fields, v, boost::is_any_of(";"));

      while (fields.size() < 3) fields.push_back(std::string());

      const std::string &address = fields[0];
      const std::string &amount_str = fields[1];
      const std::string &name = fields[2];

      if (address.empty())
      {
        error = "output parameter missing address";
        return false;
      }

      cryptonote::address_parse_info info;
      if (!get_account_address_from_str(info, cryptonote::MAINNET, address))
      {
        error = std::string("URI contains improper address: ") + address;
        return false;
      }
      addresses.push_back(address);

      uint64_t amount = 0;
      if (!amount_str.empty())
      {
        if (!cryptonote::parse_amount(amount, amount_str))
        {
          error = std::string("URI has invalid amount: ") + amount_str;
          return false;
        }
      }
      amounts.push_back(amount);

      if (!name.empty())
        recipient_names.push_back(epee::net_utils::convert_from_url_format(name));
      else
        recipient_names.push_back(std::string());
    }
    else if (k == "tx_description")
    {
      tx_description = epee::net_utils::convert_from_url_format(v);
    }
    else
    {
      unknown_parameters.push_back(arg);
    }
  }

  if (addresses.empty())
  {
    error = "URI must contain at least one `output` parameter.";
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
