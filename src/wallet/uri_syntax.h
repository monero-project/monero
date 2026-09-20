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

#pragma once

#include <string>
#include <vector>

namespace tools
{
namespace wallet
{
  struct uri_recipient
  {
    std::string address;
    // Exact decimal spelling, without a currency suffix. Empty means unspecified;
    // "0" is an explicitly specified zero. No floating-point conversion is performed.
    std::string amount;
    std::string currency = "XMR";
    std::string label;
  };

  struct payment_uri
  {
    std::vector<uri_recipient> recipients;
    std::string tx_description;
    // Ordered raw name=value fields, with their original percent encoding retained.
    std::vector<std::string> unknown_parameters;
  };

  // Syntax only: address tokens must be nonempty ASCII alphanumeric strings, but
  // base58, checksum, address type, and network validation belong to the caller.
  //
  // New address/amount/label parameters require version=2.0 as the first parameter.
  // Without a version, tx_amount (bare XMR decimal), recipient_name, tx_description,
  // and unknown parameters are accepted. Standalone tx_payment_id is unsupported.
  // Version 2 also accepts tx_amount/recipient_name as aliases, once per recipient.
  //
  // Amount spelling is retained, including .5, 1., and leading/trailing zeros.
  // Supported units are exactly XMR, BTC, ETH, USD, and EUR; an omitted unit is XMR.
  // Text is percent-decoded once; '+' is a literal plus, not a space. Text byte
  // sequences are preserved, with no Unicode normalization. Raw spaces, controls,
  // non-ASCII bytes, fragments, and malformed percent escapes are rejected.
  //
  // On failure, result is empty and error contains the current diagnostic. On
  // success, result replaces any previous value and error is empty.
  bool parse_uri_syntax(const std::string& uri, payment_uri& result, std::string& error);

  // Generates version 2, with version first and an explicit unit for every amount.
  // Text is encoded using uppercase percent escapes (spaces are %20); decimal
  // spelling is unchanged. Empty labels/descriptions are omitted. An unspecified
  // amount must have the default currency XMR because the URI has no independent
  // unit field. Unknown fields are emitted after the known fields, in stored order;
  // their keys must not collide with any reserved parameter or each other.
  // Returns an empty string and a diagnostic on failure; clears error on success.
  std::string make_uri_syntax(const payment_uri& uri, std::string& error);
}
}
