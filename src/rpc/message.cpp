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

FullMessage::FullMessage(int version, std::string& request, Message* message)
{
  doc.SetObject();

  doc.AddMember("version", version, doc.GetAllocator());
  doc.AddMember("request", rapidjson::StringRef(request.c_str()), doc.GetAllocator());
  doc.AddMember("message", message->toJson(doc), doc.GetAllocator());
}

FullMessage::FullMessage(const std::string& json_string)
{
  doc.Parse(json_string.c_str());
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
  return doc["request"].GetString();
}

int FullMessage::getVersion() const
{
  return doc["version"].GetInt();
}

rapidjson::Value& FullMessage::getMessage()
{
  return doc["message"];
}


}  // namespace rpc

}  // namespace cryptonote
