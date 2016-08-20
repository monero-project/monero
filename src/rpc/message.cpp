// Copyright (c) 2016, The Monero Project
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

rapidjson::Value Message::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  // if status not explicitly set, assume OK.
  if (status.empty()) status = STATUS_OK;

  val.AddMember("status", rapidjson::StringRef(status.c_str()), al);
  val.AddMember("error_details", rapidjson::StringRef(error_details.c_str()), al);

  return val;
}

void Message::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "status")
  status = cryptonote::json::fromJsonValue<std::string>(val["status"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "error_details")
  error_details = cryptonote::json::fromJsonValue<std::string>(val["error_details"]);

}


FullMessage::FullMessage(int version, const std::string& request, Message* message)
{
  doc.SetObject();

  doc.AddMember("version", version, doc.GetAllocator());
  doc.AddMember("request", rapidjson::StringRef(request.c_str()), doc.GetAllocator());
  doc.AddMember("message", message->toJson(doc), doc.GetAllocator());
}

FullMessage::FullMessage(const std::string& json_string)
{
  doc.Parse(json_string.c_str());
  if (doc.HasParseError())
  {
    throw cryptonote::json::PARSE_FAIL();
  }
  OBJECT_HAS_MEMBER_OR_THROW(doc, "request")
  OBJECT_HAS_MEMBER_OR_THROW(doc, "version")
  OBJECT_HAS_MEMBER_OR_THROW(doc, "message")
}

std::string FullMessage::getJson() const
{
  rapidjson::StringBuffer buf;

  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

  doc.Accept(writer);

  return std::string(buf.GetString(), buf.GetSize());
}

std::string FullMessage::getRequestType() const
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, "request")
  return doc["request"].GetString();

}

int FullMessage::getVersion() const
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, "version")
  return doc["version"].GetInt();

}

rapidjson::Value& FullMessage::getMessage()
{
  OBJECT_HAS_MEMBER_OR_THROW(doc, "message")
  return doc["message"];

}


// convenience functions for bad input
std::string BAD_REQUEST(const std::string& request)
{
  Message fail;
  fail.status = Message::STATUS_BAD_REQUEST;
  fail.error_details = std::string("\"") + request + "\" is not a valid request.";

  FullMessage fail_response(1 /*TODO: constant */, request, &fail);

  return fail_response.getJson();
}

std::string BAD_JSON(const std::string& error_details)
{
  Message fail;
  fail.status = Message::STATUS_BAD_JSON;
  fail.error_details = error_details;

  FullMessage fail_response(1 /*TODO: constant */, "bad_json", &fail);

  return fail_response.getJson();
}


}  // namespace rpc

}  // namespace cryptonote
