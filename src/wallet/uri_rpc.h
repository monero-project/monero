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
#include "misc_language.h"
#include "serialization/keyvalue_serialization.h"

namespace tools
{
namespace wallet_rpc
{
  struct uri_multi_recipient
  {
    std::string address;
    std::string amount; // Decimal units, not atomic units; empty means unspecified.
    std::string currency = "XMR";
    std::string label;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)
      if (is_store)
      {
        KV_SERIALIZE(amount)
        KV_SERIALIZE(currency)
        KV_SERIALIZE(label)
      }
      else
      {
        KV_SERIALIZE_OPT(amount, std::string())
        KV_SERIALIZE_OPT(currency, std::string("XMR"))
        KV_SERIALIZE_OPT(label, std::string())
      }
    END_KV_SERIALIZE_MAP()
  };

  struct uri_multi_spec
  {
    std::vector<uri_multi_recipient> recipients;
    std::string tx_description;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(recipients)
      if (is_store)
      {
        KV_SERIALIZE(tx_description)
      }
      else
      {
        KV_SERIALIZE_OPT(tx_description, std::string())
      }
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MAKE_URI_MULTI
  {
    struct request_t : uri_multi_spec
    {
      std::string network_type = "mainnet";

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(recipients)
        KV_SERIALIZE_OPT(tx_description, std::string())
        KV_SERIALIZE_OPT(network_type, std::string("mainnet"))
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string uri;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_PARSE_URI_MULTI
  {
    struct request_t
    {
      std::string uri;
      std::string network_type = "mainnet";
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
        KV_SERIALIZE_OPT(network_type, std::string("mainnet"))
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uri_multi_spec uri;
      std::vector<std::string> unknown_parameters;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
        KV_SERIALIZE(unknown_parameters)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
}
}
