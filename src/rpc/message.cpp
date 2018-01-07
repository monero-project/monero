// Copyright (c) 2016-2018, The Monero Project
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

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

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
}

rapidjson::Value Message::toJson(rapidjson::Document& doc) const
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("status", rapidjson::StringRef(status.c_str()), al);
  val.AddMember("error_details", rapidjson::StringRef(error_details.c_str()), al);
  INSERT_INTO_JSON_OBJECT(val, doc, rpc_version, DAEMON_RPC_VERSION_ZMQ);

  return val;
}

void Message::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, status, status);
  GET_FROM_JSON_OBJECT(val, error_details, error_details);
  GET_FROM_JSON_OBJECT(val, rpc_version, rpc_version);
}


FullMessage::FullMessage(const std::string& request, Message* message)
{
  doc.SetObject();

  doc.AddMember(method_field, rapidjson::StringRef(request.c_str()), doc.GetAllocator());
  doc.AddMember(params_field, message->toJson(doc), doc.GetAllocator());

  // required by JSON-RPC 2.0 spec
  doc.AddMember("jsonrpc", rapidjson::Value("2.0"), doc.GetAllocator());
}

FullMessage::FullMessage(Message* message)
{
  doc.SetObject();

  // required by JSON-RPC 2.0 spec
  doc.AddMember("jsonrpc", "2.0", doc.GetAllocator());

  if (message->status == Message::STATUS_OK)
  {
    doc.AddMember(result_field, message->toJson(doc), doc.GetAllocator());
  }
  else
  {
    cryptonote::rpc::error err;

    err.error_str = message->status;
    err.message = message->error_details;

    INSERT_INTO_JSON_OBJECT(doc, doc, error, err);
  }
}

FullMessage::FullMessage(const std::string& json_string, bool request)
{
  doc.Parse(json_string.c_str());
  if (doc.HasParseError() || !doc.IsObject())
  {
    throw cryptonote::json::PARSE_FAIL();
  }

  OBJECT_HAS_MEMBER_OR_THROW(doc, "jsonrpc")

  if (request)
  {
    OBJECT_HAS_MEMBER_OR_THROW(doc, method_field)
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

std::string FullMessage::getJson()
{

  if (!doc.HasMember(id_field))
  {
    doc.AddMember(id_field, rapidjson::Value("unused"), doc.GetAllocator());
  }

  rapidjson::StringBuffer buf;

  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

  doc.Accept(writer);

  return std::string(buf.GetString(), buf.GetSize());
}

std::string FullMessage::getRequestType() const
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, method_field)
  return doc[method_field].GetString();
}

rapidjson::Value& FullMessage::getMessage()
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
  rapidjson::Value& val = getMessage();

  return rapidjson::Value(val, doc.GetAllocator());
}

rapidjson::Value& FullMessage::getID()
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, id_field)
  return doc[id_field];
}

void FullMessage::setID(rapidjson::Value& id)
{
  auto itr = doc.FindMember(id_field);
  if (itr != doc.MemberEnd())
  {
    itr->value = id;
  }
  else
  {
    doc.AddMember(id_field, id, doc.GetAllocator());
  }
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

FullMessage FullMessage::requestMessage(const std::string& request, Message* message)
{
  return FullMessage(request, message);
}

FullMessage FullMessage::requestMessage(const std::string& request, Message* message, rapidjson::Value& id)
{
  auto mes = requestMessage(request, message);
  mes.setID(id);
  return mes;
}

FullMessage FullMessage::responseMessage(Message* message)
{
  return FullMessage(message);
}

FullMessage FullMessage::responseMessage(Message* message, rapidjson::Value& id)
{
  auto mes = responseMessage(message);
  mes.setID(id);
  return mes;
}

FullMessage* FullMessage::timeoutMessage()
{
  auto *full_message = new FullMessage();

  auto& doc = full_message->doc;
  auto& al = full_message->doc.GetAllocator();

  doc.SetObject();

  // required by JSON-RPC 2.0 spec
  doc.AddMember("jsonrpc", "2.0", al);

  cryptonote::rpc::error err;

  err.error_str = "RPC request timed out.";
  INSERT_INTO_JSON_OBJECT(doc, doc, err, err);

  return full_message;
}

// convenience functions for bad input
std::string BAD_REQUEST(const std::string& request)
{
  Message fail;
  fail.status = Message::STATUS_BAD_REQUEST;
  fail.error_details = std::string("\"") + request + "\" is not a valid request.";

  FullMessage fail_response = FullMessage::responseMessage(&fail);

  return fail_response.getJson();
}

std::string BAD_REQUEST(const std::string& request, rapidjson::Value& id)
{
  Message fail;
  fail.status = Message::STATUS_BAD_REQUEST;
  fail.error_details = std::string("\"") + request + "\" is not a valid request.";

  FullMessage fail_response = FullMessage::responseMessage(&fail, id);

  return fail_response.getJson();
}

std::string BAD_JSON(const std::string& error_details)
{
  Message fail;
  fail.status = Message::STATUS_BAD_JSON;
  fail.error_details = error_details;

  FullMessage fail_response = FullMessage::responseMessage(&fail);

  return fail_response.getJson();
}


}  // namespace rpc

}  // namespace cryptonote
