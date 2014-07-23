// Copyright (c) 2014, The Monero Project
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

#ifndef JSONRPC_STRUCTS_H
#define	JSONRPC_STRUCTS_H

#include <string>
#include <cstdint>
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_base.h"

namespace epee 
{
  namespace json_rpc
  {
    template<typename t_param>
    struct request
    {
      std::string jsonrpc;
      std::string method;
      epee::serialization::storage_entry id;
      t_param     params;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(method)
        KV_SERIALIZE(params)
      END_KV_SERIALIZE_MAP()
    };

    struct error
    {
      int64_t code;
      std::string message;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(code)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    
    struct dummy_error
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct dummy_result
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_param, typename t_error>
    struct response
    {
      std::string jsonrpc;
      t_param     result;
      epee::serialization::storage_entry id;
      t_error     error;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(result)
        KV_SERIALIZE(error)
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_param>
    struct response<t_param, dummy_error>
    {
      std::string jsonrpc;
      t_param     result;
      epee::serialization::storage_entry id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(result)
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_error>
    struct response<dummy_result, t_error>
    {
      std::string jsonrpc;
      t_error     error;
      epee::serialization::storage_entry id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(error)
      END_KV_SERIALIZE_MAP()
    };

    typedef response<dummy_result, error> error_response;
  }
}

#endif	/* JSONRPC_STRUCTS_H */
