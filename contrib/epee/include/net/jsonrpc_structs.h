#ifndef JSONRPC_STRUCTS_H
#define	JSONRPC_STRUCTS_H

#include <boost/optional/optional.hpp>
#include <cstdint>
#include <string>
#include "serialization/wire/basic_value.h"
#include "serialization/wire/field.h"
#include "serialization/wire/fwd.h"
#include "serialization/wire/json/base.h"

//! Must be done in the `epee::json_rpc` namespace!
#define EPEE_JSONRPC_DECLARE(command)				    \
  WIRE_JSON_DECLARE_CONVERSION(request_specific<command::request>); \
  WIRE_JSON_DECLARE_CONVERSION(client_request<command::request>);   \
  WIRE_JSON_DECLARE_CONVERSION(response<command::response>)

//! Must be done in the `epee::json_rpc` namespace (cpp)!
#define EPEE_JSONRPC_DEFINE(command)				   \
  WIRE_JSON_DEFINE_CONVERSION(request_specific<command::request>)  \
  WIRE_JSON_DEFINE_CONVERSION(client_request<command::request>)	   \
  WIRE_JSON_DEFINE_CONVERSION(response<command::response>)

namespace epee 
{
  namespace json_rpc
  {
    struct request_generic
    {
      std::string jsonrpc;
      std::string method;
      wire::basic_value id;

      request_generic(): jsonrpc(), method(), id{} {}

      WIRE_BEGIN_MAP(),
        WIRE_FIELD(jsonrpc),
        WIRE_OPTIONAL_FIELD(id),
        WIRE_FIELD(method)
      WIRE_END_MAP()
    };
    WIRE_JSON_DECLARE_CONVERSION(request_generic);

    template<typename t_param>
    struct request_specific
    {
      t_param params;

      request_specific() : params{} {}

      WIRE_BEGIN_MAP(),
        WIRE_FIELD(params)
      WIRE_END_MAP()
    };

    template<typename t_param>
    struct client_request
    {
      std::string jsonrpc;
      std::string method;
      wire::basic_value id;
      t_param params;

      client_request() : jsonrpc(), method(), id{}, params{} {}

      WIRE_BEGIN_MAP(),
        WIRE_FIELD(jsonrpc),
        WIRE_FIELD(method),
        WIRE_FIELD(id),
        WIRE_FIELD(params)
      WIRE_END_MAP()
    };

    struct error
    {
      int64_t code;
      std::string message;

      error(): code(0), message() {}

      WIRE_BEGIN_MAP(),
        WIRE_FIELD(code),
        WIRE_FIELD(message)
      WIRE_END_MAP()
    };

    template<typename t_param>
    struct response
    {
      std::string jsonrpc;
      wire::basic_value id;
      boost::optional<t_param> result;
      boost::optional<error> error_;

      response(): jsonrpc(), id(), result(), error_() {}

      WIRE_BEGIN_MAP(),
        WIRE_FIELD(jsonrpc),
        WIRE_OPTIONAL_FIELD(id),
        WIRE_OPTIONAL_FIELD(result),
        wire::optional_field("error", std::ref(self.error_))
      WIRE_END_MAP()
    };
    WIRE_JSON_DECLARE_CONVERSION(response<std::string>); // used as default error response
  }
}

#endif	/* JSONRPC_STRUCTS_H */
