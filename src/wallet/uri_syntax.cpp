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

#include "uri_syntax.h"

#include <set>
#include <utility>

namespace tools
{
namespace wallet
{
namespace
{
  bool fail(std::string& error, const std::string& message)
  {
    error = message;
    return false;
  }

  bool digit(unsigned char c)
  {
    return c >= '0' && c <= '9';
  }

  bool alphanumeric(unsigned char c)
  {
    return digit(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  }

  bool unreserved(unsigned char c)
  {
    return alphanumeric(c) || c == '-' || c == '.' || c == '_' || c == '~';
  }

  bool valid_address_token(const std::string& address)
  {
    if (address.empty())
      return false;
    for (unsigned char c : address)
      if (!alphanumeric(c))
        return false;
    return true;
  }

  bool valid_currency(const std::string& currency)
  {
    return currency == "XMR" || currency == "BTC" || currency == "ETH" ||
      currency == "USD" || currency == "EUR";
  }

  bool valid_decimal(const std::string& amount)
  {
    bool seen_digit = false;
    bool seen_point = false;
    for (unsigned char c : amount)
    {
      if (digit(c))
        seen_digit = true;
      else if (c == '.' && !seen_point)
        seen_point = true;
      else
        return false;
    }
    return seen_digit;
  }

  bool parse_amount(const std::string& value, bool allow_unit, uri_recipient& recipient,
    std::string& error)
  {
    const std::size_t unit_start = value.find_first_not_of("0123456789.");
    const std::string amount = value.substr(0, unit_start);
    const std::string currency = unit_start == std::string::npos ? "XMR" : value.substr(unit_start);
    if (!valid_decimal(amount))
      return fail(error, "Amount must contain ASCII digits and at most one decimal point");
    if ((!allow_unit && unit_start != std::string::npos) || !valid_currency(currency))
      return fail(error, allow_unit ? "Unsupported amount currency" : "Legacy tx_amount must be a bare XMR decimal");
    recipient.amount = amount;
    recipient.currency = currency;
    return true;
  }

  int hex_value(unsigned char c)
  {
    if (digit(c)) return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  }

  bool valid_query_value(const std::string& value)
  {
    for (std::size_t i = 0; i < value.size(); ++i)
    {
      const unsigned char c = value[i];
      if (c == '%')
      {
        if (i + 2 >= value.size() || hex_value(value[i + 1]) < 0 || hex_value(value[i + 2]) < 0)
          return false;
        i += 2;
      }
      else if (!unreserved(c) && c != '!' && c != '$' && c != '\'' && c != '(' &&
        c != ')' && c != '*' && c != '+' && c != ',' && c != ';' && c != ':' &&
        c != '@' && c != '/' && c != '?')
        return false;
    }
    return true;
  }

  // Called only after valid_query_value has checked every escape.
  std::string decode_text(const std::string& value)
  {
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i)
    {
      if (value[i] == '%')
      {
        result.push_back(static_cast<char>(16 * hex_value(value[i + 1]) + hex_value(value[i + 2])));
        i += 2;
      }
      else
        result.push_back(value[i]);
    }
    return result;
  }

  std::string encode_text(const std::string& value)
  {
    const char* hex = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : value)
    {
      if (unreserved(c))
        result.push_back(static_cast<char>(c));
      else
      {
        result.push_back('%');
        result.push_back(hex[c >> 4]);
        result.push_back(hex[c & 15]);
      }
    }
    return result;
  }

  bool split_parameter(const std::string& parameter, std::string& name, std::string& value,
    std::string& error)
  {
    const std::size_t equals = parameter.find('=');
    if (equals == std::string::npos || equals == 0 || parameter.find('=', equals + 1) != std::string::npos)
      return fail(error, "Each URI parameter must have one nonempty name and one '=' separator");
    name = parameter.substr(0, equals);
    value = parameter.substr(equals + 1);
    for (unsigned char c : name)
      if (!unreserved(c))
        return fail(error, "URI parameter names must contain only unreserved ASCII characters");
    if (!valid_query_value(value))
      return fail(error, "URI parameter contains an invalid raw character or percent escape");
    return true;
  }

  bool reserved_parameter(const std::string& name)
  {
    return name == "version" || name == "address" || name == "amount" || name == "tx_amount" ||
      name == "label" || name == "recipient_name" || name == "tx_description" || name == "tx_payment_id";
  }
}

  bool parse_uri_syntax(const std::string& uri, payment_uri& result, std::string& error)
  {
    result = payment_uri{};
    error.clear();
    if (uri.compare(0, 7, "monero:") != 0)
      return fail(error, "URI must start with monero:");

    const std::size_t question = uri.find('?', 7);
    uri_recipient first;
    first.address = uri.substr(7, question == std::string::npos ? std::string::npos : question - 7);
    if (!valid_address_token(first.address))
      return fail(error, "URI requires a nonempty ASCII alphanumeric address token");

    payment_uri parsed;
    parsed.recipients.push_back(std::move(first));
    bool version_2 = false;
    bool seen_description = false;
    bool seen_amount = false;
    bool seen_label = false;
    std::set<std::string> unknown_names;

    if (question != std::string::npos && question + 1 < uri.size())
    {
      std::size_t start = question + 1;
      std::size_t parameter_index = 0;
      while (true)
      {
        const std::size_t end = uri.find('&', start);
        const std::string parameter = uri.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::string name, value;
        if (!split_parameter(parameter, name, value, error))
          return false;
        if (name == "version")
        {
          if (parameter_index != 0 || version_2)
            return fail(error, "Version must occur once, as the first URI parameter");
          if (value != "2.0")
            return fail(error, "Unsupported URI version; expected 2.0");
          version_2 = true;
        }
        else if (name == "address")
        {
          if (!version_2)
            return fail(error, "Additional addresses require version=2.0 first");
          if (!valid_address_token(value))
            return fail(error, "Recipient requires a nonempty ASCII alphanumeric address token");
          uri_recipient recipient;
          recipient.address = value;
          parsed.recipients.push_back(std::move(recipient));
          seen_amount = false;
          seen_label = false;
        }
        else if (name == "amount" || name == "tx_amount")
        {
          if (name == "amount" && !version_2)
            return fail(error, "Amount requires version=2.0 first");
          if (seen_amount)
            return fail(error, "Recipient has more than one amount or conflicting amount aliases");
          if (!parse_amount(value, version_2, parsed.recipients.back(), error))
            return false;
          seen_amount = true;
        }
        else if (name == "label" || name == "recipient_name")
        {
          if (name == "label" && !version_2)
            return fail(error, "Label requires version=2.0 first");
          if (seen_label)
            return fail(error, "Recipient has more than one label or conflicting label aliases");
          parsed.recipients.back().label = decode_text(value);
          seen_label = true;
        }
        else if (name == "tx_description")
        {
          if (seen_description)
            return fail(error, "URI has more than one transaction description");
          parsed.tx_description = decode_text(value);
          seen_description = true;
        }
        else if (name == "tx_payment_id")
          return fail(error, "Standalone payment IDs are unsupported by the new URI API");
        else
        {
          if (!unknown_names.insert(name).second)
            return fail(error, "URI has a duplicate unknown parameter: " + name);
          parsed.unknown_parameters.push_back(parameter);
        }
        ++parameter_index;
        if (end == std::string::npos)
          break;
        start = end + 1;
      }
    }
    result = std::move(parsed);
    return true;
  }

  std::string make_uri_syntax(const payment_uri& uri, std::string& error)
  {
    error.clear();
    if (uri.recipients.empty())
    {
      error = "URI requires at least one recipient";
      return {};
    }
    std::string result;
    for (std::size_t i = 0; i < uri.recipients.size(); ++i)
    {
      const uri_recipient& recipient = uri.recipients[i];
      if (!valid_address_token(recipient.address))
      {
        error = "Recipient requires a nonempty ASCII alphanumeric address token";
        return {};
      }
      if (!valid_currency(recipient.currency))
      {
        error = "Unsupported amount currency";
        return {};
      }
      if (recipient.amount.empty() && recipient.currency != "XMR")
      {
        error = "An unspecified amount cannot carry a nondefault currency";
        return {};
      }
      if (!recipient.amount.empty() && !valid_decimal(recipient.amount))
      {
        error = "Amount must contain ASCII digits and at most one decimal point, without a unit suffix";
        return {};
      }
      if (i == 0)
        result = "monero:" + recipient.address + "?version=2.0";
      else
        result += "&address=" + recipient.address;
      if (!recipient.amount.empty())
        result += "&amount=" + recipient.amount + recipient.currency;
      if (!recipient.label.empty())
        result += "&label=" + encode_text(recipient.label);
    }
    if (!uri.tx_description.empty())
      result += "&tx_description=" + encode_text(uri.tx_description);

    std::set<std::string> unknown_names;
    for (const std::string& parameter : uri.unknown_parameters)
    {
      std::string name, value;
      if (!split_parameter(parameter, name, value, error))
        return {};
      if (reserved_parameter(name) || !unknown_names.insert(name).second)
      {
        error = "Unknown parameter collides with a reserved or duplicate name: " + name;
        return {};
      }
      result += "&" + parameter;
    }
    return result;
  }
}
}
