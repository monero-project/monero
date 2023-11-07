// Copyright (c) 2016-2023, The Monero Project
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

#include "message.h"

#include "serialization/wire.h"
#include "serialization/wire/basic_value.h"
#include "serialization/wire/json.h"

namespace cryptonote
{

namespace rpc
{

const char* Message::STATUS_OK = "OK";
const char* Message::STATUS_RETRY = "Retry";
const char* Message::STATUS_FAILED = "Failed";
const char* Message::STATUS_BAD_REQUEST = "Invalid request type";
const char* Message::STATUS_BAD_JSON = "Malformed json";

namespace
{
constexpr const unsigned default_buffer_size = 256 * 1024;
constexpr const char json_rpc_version[] = "2.0";

struct json_rpc_request
{
  unsigned id;
  boost::string_ref jsonrpc;
  boost::string_ref method;
  const Message& params;

  WIRE_DEFINE_CONVERSIONS()
  WIRE_BEGIN_MAP(),
    WIRE_FIELD(id),
    WIRE_FIELD(jsonrpc),
    WIRE_FIELD(method),
    WIRE_FIELD(params)
  WIRE_END_MAP()
};

struct json_rpc_error
{
  int32_t code;
  std::string message;
  std::string error_str;

  WIRE_BEGIN_MAP(),
    WIRE_FIELD(code),
    WIRE_FIELD(message),
    WIRE_FIELD(error_str)
  WIRE_END_MAP()
};

struct json_rpc_response
{
  const wire::basic_value& id;
  boost::string_ref jsonrpc;
  boost::optional<const Message&> result;
  boost::optional<json_rpc_error> error;

  WIRE_DEFINE_CONVERSIONS()
  WIRE_BEGIN_MAP(),
    WIRE_FIELD(jsonrpc),
    WIRE_OPTIONAL_FIELD(id),
    WIRE_OPTIONAL_FIELD(result),
    WIRE_OPTIONAL_FIELD(error)
  WIRE_END_MAP()
};

template<typename F, typename T>
void default_message_map(F& format, T& self)
{
  wire::object(format, WIRE_FIELD(rpc_version));
}
}

void Message::write_bytes(wire::writer& dest) const
{
  default_message_map(dest, *this);
}
void Message::read_bytes(wire::reader& source)
{
  default_message_map(source, *this);
}

expect<epee::byte_slice> getJsonRequest(const std::string& request, const Message& message, const unsigned id)
{
  const json_rpc_request out{id, json_rpc_version, request, message};

  std::string buffer{};
  buffer.reserve(default_buffer_size);
  const std::error_code error = wire::json::to_bytes(buffer, out);
  if (error)
    return error;
  return epee::byte_slice{std::move(buffer)};
}


expect<epee::byte_slice> getJsonResponse(const Message& message, const wire::basic_value& id)
{
  json_rpc_response response{id, json_rpc_version};

  if (message.status == Message::STATUS_OK)
    response.result.emplace(message);
  else
  {
    response.error.emplace();
    response.error->message = message.error_details;
    response.error->error_str = message.status;
  }

  std::string buffer;
  buffer.reserve(default_buffer_size);
  const std::error_code error = wire::json::to_bytes(buffer, response);
  if (error)
    return error;
  return epee::byte_slice{std::move(buffer)};
}

// convenience functions for bad input
expect<epee::byte_slice> BAD_JSON_REQUEST(const std::string& request)
{
  return BAD_JSON_REQUEST(request, wire::basic_value{});
}

expect<epee::byte_slice> BAD_JSON_REQUEST(const std::string& request, const wire::basic_value& id)
{
  Message fail;
  fail.status = Message::STATUS_BAD_REQUEST;
  fail.error_details = std::string("\"") + request + "\" is not a valid request.";
  return getJsonResponse(fail, id);
}

expect<epee::byte_slice> BAD_JSON(const std::string& error_details)
{
  Message fail;
  fail.status = Message::STATUS_BAD_JSON;
  fail.error_details = error_details;
  return getJsonResponse(fail, wire::basic_value{});
}

}  // namespace rpc

}  // namespace cryptonote
