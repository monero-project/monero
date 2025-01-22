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

#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <string>

#include "byte_slice.h"
#include "byte_stream.h"
#include "rpc/message_data_structs.h"

namespace cryptonote
{

namespace rpc
{

  class Message
  {
      virtual void doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
      {}

    public:
      static const char* STATUS_OK;
      static const char* STATUS_RETRY;
      static const char* STATUS_FAILED;
      static const char* STATUS_BAD_REQUEST;
      static const char* STATUS_BAD_JSON;

      Message() : status(STATUS_OK), rpc_version(0) { }

      virtual ~Message() { }

      void toJson(rapidjson::Writer<epee::byte_stream>& dest) const;

      virtual void fromJson(const rapidjson::Value& val);

      std::string status;
      std::string error_details;
      uint32_t rpc_version;
  };

  class FullMessage
  {
    public:
      ~FullMessage() { }

      FullMessage(std::string&& json_string, bool request=false);

      std::string getRequestType() const;

      const rapidjson::Value& getMessage() const;

      rapidjson::Value getMessageCopy();

      const rapidjson::Value& getID() const;

      cryptonote::rpc::error getError();

      static epee::byte_slice getRequest(const std::string& request, const Message& message, unsigned id);
      static epee::byte_slice getResponse(const Message& message, const rapidjson::Value& id);
    private:

      FullMessage() = default;
      FullMessage(const FullMessage&) = delete;
      FullMessage& operator=(const FullMessage&) = delete;

      FullMessage(const std::string& request, Message* message);
      FullMessage(Message* message);

      std::string contents;
      rapidjson::Document doc;
  };


  // convenience functions for bad input
  epee::byte_slice BAD_REQUEST(const std::string& request);
  epee::byte_slice BAD_REQUEST(const std::string& request, const rapidjson::Value& id);

  epee::byte_slice BAD_JSON(const std::string& error_details);


}  // namespace rpc

}  // namespace cryptonote
