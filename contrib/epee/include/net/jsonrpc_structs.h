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
