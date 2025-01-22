// Copyright (c) 2016-2024, The Monero Project
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

#include "daemon_rpc_version.h"
#include "serialization/json_object.h"

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
constexpr const char error_field[] = "error";
constexpr const char id_field[] = "id";
constexpr const char method_field[] = "method";
constexpr const char params_field[] = "params";
constexpr const char result_field[] = "result";

const rapidjson::Value& get_method_field(const rapidjson::Value& src)
{
  const auto member = src.FindMember(method_field);
  if (member == src.MemberEnd())
    throw cryptonote::json::MISSING_KEY{method_field};
  if (!member->value.IsString())
    throw cryptonote::json::WRONG_TYPE{"Expected string"};
  return member->value;
}
}

void Message::toJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  dest.StartObject();
  INSERT_INTO_JSON_OBJECT(dest, rpc_version, DAEMON_RPC_VERSION_ZMQ);
  doToJson(dest);
  dest.EndObject();
}

void Message::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, rpc_version, rpc_version);
}

FullMessage::FullMessage(std::string&& json_string, bool request)
  : contents(std::move(json_string)), doc()
{
  /* Insitu parsing does not copy data from `contents` to DOM,
     accelerating string heavy content. */
  doc.ParseInsitu(std::addressof(contents[0]));
  if (doc.HasParseError() || !doc.IsObject())
  {
    throw cryptonote::json::PARSE_FAIL();
  }

  OBJECT_HAS_MEMBER_OR_THROW(doc, "jsonrpc")

  if (request)
  {
    get_method_field(doc); // throws on errors
    OBJECT_HAS_MEMBER_OR_THROW(doc, params_field)
  }
  else
  {
    if (!doc.HasMember(result_field) && !doc.HasMember(error_field))
    {
      throw cryptonote::json::MISSING_KEY("error/result");
    }
  }
}

std::string FullMessage::getRequestType() const
{
  return get_method_field(doc).GetString();
}

const rapidjson::Value& FullMessage::getMessage() const
{
  if (doc.HasMember(params_field))
  {
    return doc[params_field];
  }
  else if (doc.HasMember(result_field))
  {
    return doc[result_field];
  }

  //else
  OBJECT_HAS_MEMBER_OR_THROW(doc, error_field)
  return doc[error_field];

}

rapidjson::Value FullMessage::getMessageCopy()
{
  return rapidjson::Value(getMessage(), doc.GetAllocator());
}

const rapidjson::Value& FullMessage::getID() const
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, id_field)
  return doc[id_field];
}

cryptonote::rpc::error FullMessage::getError()
{
  cryptonote::rpc::error err;
  err.use = false;
  if (doc.HasMember(error_field))
  {
    GET_FROM_JSON_OBJECT(doc, err, error);
    err.use = true;
  }

  return err;
}

epee::byte_slice FullMessage::getRequest(const std::string& request, const Message& message, const unsigned id)
{
  epee::byte_stream buffer;
  {
    rapidjson::Writer<epee::byte_stream> dest{buffer};

    dest.StartObject();
    INSERT_INTO_JSON_OBJECT(dest, jsonrpc, (boost::string_ref{"2.0", 3}));

    dest.Key(id_field);
    json::toJsonValue(dest, id);

    dest.Key(method_field);
    json::toJsonValue(dest, request);

    dest.Key(params_field);
    message.toJson(dest);

    dest.EndObject();

    if (!dest.IsComplete())
      throw std::logic_error{"Invalid JSON tree generated"};
  }
  return epee::byte_slice{std::move(buffer)};
}


epee::byte_slice FullMessage::getResponse(const Message& message, const rapidjson::Value& id)
{
  epee::byte_stream buffer;
  {
    rapidjson::Writer<epee::byte_stream> dest{buffer};

    dest.StartObject();
    INSERT_INTO_JSON_OBJECT(dest, jsonrpc, (boost::string_ref{"2.0", 3}));

    dest.Key(id_field);
    json::toJsonValue(dest, id);

    if (message.status == Message::STATUS_OK)
    {
      dest.Key(result_field);
      message.toJson(dest);
    }
    else
    {
      cryptonote::rpc::error err;

      err.error_str = message.status;
      err.message = message.error_details;

      INSERT_INTO_JSON_OBJECT(dest, error, err);
    }
    dest.EndObject();

    if (!dest.IsComplete())
      throw std::logic_error{"Invalid JSON tree generated"};
  }
  return epee::byte_slice{std::move(buffer)};
}

// convenience functions for bad input
epee::byte_slice BAD_REQUEST(const std::string& request)
{
  rapidjson::Value invalid;
  return BAD_REQUEST(request, invalid);
}

epee::byte_slice BAD_REQUEST(const std::string& request, const rapidjson::Value& id)
{
  Message fail;
  fail.status = Message::STATUS_BAD_REQUEST;
  fail.error_details = std::string("\"") + request + "\" is not a valid request.";
  return FullMessage::getResponse(fail, id);
}

epee::byte_slice BAD_JSON(const std::string& error_details)
{
  rapidjson::Value invalid;
  Message fail;
  fail.status = Message::STATUS_BAD_JSON;
  fail.error_details = error_details;
  return FullMessage::getResponse(fail, invalid);
}


}  // namespace rpc

}  // namespace cryptonote
