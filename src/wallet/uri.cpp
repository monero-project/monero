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
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp> 
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
static bool validate_address(const std::string &addr, cryptonote::network_type network_type)
{
  if (network_type == cryptonote::UNDEFINED)
  {
    cryptonote::address_parse_info info;

    const cryptonote::network_type nets[] = {
      cryptonote::network_type::MAINNET,
      cryptonote::network_type::TESTNET,
      cryptonote::network_type::STAGENET,
      cryptonote::network_type::FAKECHAIN
    };

    for (auto n : nets)
    {
      if (cryptonote::get_account_address_from_str(info, n, addr))
      {
        return true;
      }
    }
  }
  else 
  {
    cryptonote::address_parse_info info;
    return cryptonote::get_account_address_from_str(info, network_type, addr);
  }

  return false;
}
//----------------------------------------------------------------------------------------------------
std::string make_uri(const std::vector<std::string> &addresses, const std::vector<uint64_t> &amounts, const std::vector<std::string> &recipient_names, const std::string &tx_description, std::string &error, cryptonote::network_type network_type)
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

    if (amounts[0] > 0)
    {
      // URI encoded amount is in decimal units, not atomic units
      uri += (n_fields++ ? "&" : "?") + std::string("tx_amount=") + cryptonote::print_money(amounts[0]);
    }

    if (!recipient_names[0].empty())
    {
      uri += (n_fields++ ? "&" : "?") + std::string("recipient_name=") + convert_to_url_format(recipient_names[0]);
    }

    if (!tx_description.empty())
    {
      uri += (n_fields++ ? "&" : "?") + std::string("tx_description=") + convert_to_url_format(tx_description);
    }

    return uri;
  }


  std::string uri = "monero:";
  bool first_param = true;

  for (size_t i = 0; i < addresses.size(); ++i)
  {
    std::string out_val;
    out_val.reserve(256);
    
    const std::string &address = addresses[i];
    if (!validate_address(address, network_type))
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
static bool parse_output_param(const std::string &value, std::vector<std::string> &addresses, std::vector<uint64_t> &amounts, std::vector<std::string> &names, cryptonote::network_type network_type, std::string &error)
{
  std::vector<std::string> fields;
  boost::split(fields, value, boost::is_any_of(";"));
  while (fields.size() < 3) 
  {
    fields.emplace_back();
  }

  const std::string &address = fields[0];
  const std::string &amount_str = fields[1];
  const std::string &name = fields[2];

  if (address.empty())
  {
    error = "output parameter missing address";
    return false;
  }

  if (!validate_address(address, network_type))
  {
    error = std::string("URI contains improper address: ") + address;
    return false;
  }

  uint64_t amount = 0;
  if (!amount_str.empty() && !cryptonote::parse_amount(amount, amount_str)) 
  {
    error = "Invalid amount: " + amount_str;
    return false;
  }

  addresses.push_back(address);
  amounts.push_back(amount);
  names.push_back(name.empty() ? std::string() : epee::net_utils::convert_from_url_format(name));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool parse_uri(const std::string &uri, std::vector<std::string> &addresses, std::vector<uint64_t> &amounts, std::vector<std::string> &recipient_names, std::string &tx_description, std::vector<std::string> &unknown_parameters, std::string &error, cryptonote::network_type network_type)
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

  bool has_output = std::any_of(args.begin(), args.end(), [](const std::string &arg) {
    return arg.rfind("output=", 0) == 0;
  });

  if (has_output) 
  {
    // multiple recipient uri
    if (!path.empty())
    {
      error = "URI must not include a path address when using output= parameters (ambiguous)";
      return false;
    }
    
    std::set<std::string> have_arg;

    for (const std::string &arg : args) 
    {
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

      if (key != "output")
      {
        if (have_arg.find(key) != have_arg.end())
        {
          error = std::string("URI has more than one instance of ") + key;
          return false;
        }
        have_arg.insert(key);
      }

      if (key == "output") 
      {
        if (!parse_output_param(value, addresses, amounts, recipient_names, network_type, error))
        {
          return false;
        }
      }
      else if (key == "tx_description")
      {
        tx_description = epee::net_utils::convert_from_url_format(value);
      }
      else 
      {
        unknown_parameters.push_back(arg);
      }
    }
    if (addresses.empty()) 
    {
      error = "At least one output required";
      return false;
    }

    if (addresses.size() != amounts.size() || addresses.size() != recipient_names.size())
    {
      error = "Internal error: parsed output vector sizes mismatch.";
      return false;
    }
    return true;
  }

  if (!query.empty()) 
  {
    // single recipient uri
    if (!validate_address(path, network_type)) 
    {
      error = std::string("URI contains improper address: ") + path;
      return false;
    }

    uint64_t amount = 0;
    std::string name;
    std::set<std::string> have_arg;
    for (const std::string &arg : args) 
    {
      auto eq = arg.find('=');
      if (eq == std::string::npos)
      {
        error = "Bad parameter: " + arg;
        return false;
      }
      std::string key = arg.substr(0, eq);
      std::string value = arg.substr(eq + 1);

      if (have_arg.find(key) != have_arg.end())
      {
        error = std::string("URI has more than one instance of ") + key;
        return false;
      }
      have_arg.insert(key);

      if (key == "tx_amount") 
      {
        if (!cryptonote::parse_amount(amount, value)) 
        {
          error = "Invalid amount: " + value;
          return false;
        }
      }
      else if (key == "recipient_name") 
      {
        name = epee::net_utils::convert_from_url_format(value);
      }
      else if (key == "tx_description") 
      {
        tx_description = epee::net_utils::convert_from_url_format(value);
      }
      else 
      {
        unknown_parameters.push_back(arg);
      }
    }
    addresses.push_back(path);
    amounts.push_back(amount);
    recipient_names.push_back(name);
    return true;
  }

  // bare address
  if (!validate_address(path, network_type))
  {
    error = std::string("URI contains improper address: ") + path;
    return false;
  }

  addresses.push_back(path);
  amounts.push_back(0);
  recipient_names.push_back("");
  return true;
}
//----------------------------------------------------------------------------------------------------
