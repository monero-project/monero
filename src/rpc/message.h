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

#pragma once

#include "rapidjson/document.h"
#include "rpc/message_data_structs.h"
#include <string>

/* I normally hate using macros, but in this case it would be untenably
 * verbose to not use a macro.  This macro saves the trouble of explicitly
 * writing the below if block for every single RPC call.
 */
#define REQ_RESP_TYPES_MACRO( runtime_str, type, reqjson, resp_message_ptr, handler) \
  \
  if (runtime_str == type::name) \
  { \
    type::Request reqvar; \
    type::Response *respvar = new type::Response(); \
    \
    reqvar.fromJson(reqjson); \
    \
    handler(reqvar, *respvar); \
    \
    resp_message_ptr = respvar; \
  }

namespace cryptonote
{

namespace rpc
{

  class Message
  {
    public:
      static const char* STATUS_OK;
      static const char* STATUS_RETRY;
      static const char* STATUS_FAILED;
      static const char* STATUS_BAD_REQUEST;
      static const char* STATUS_BAD_JSON;

      Message() : status(STATUS_OK), rpc_version(0) { }

      virtual ~Message() { }

      virtual rapidjson::Value toJson(rapidjson::Document& doc) const;

      virtual void fromJson(rapidjson::Value& val);

      std::string status;
      std::string error_details;
      uint32_t rpc_version;
  };

  class FullMessage
  {
    public:
      ~FullMessage() { }

      FullMessage(FullMessage&& rhs) noexcept : doc(std::move(rhs.doc)) { }

      FullMessage(const std::string& json_string, bool request=false);

      std::string getJson();

      std::string getRequestType() const;

      rapidjson::Value& getMessage();

      rapidjson::Value getMessageCopy();

      rapidjson::Value& getID();

      void setID(rapidjson::Value& id);

      cryptonote::rpc::error getError();

      static FullMessage requestMessage(const std::string& request, Message* message);
      static FullMessage requestMessage(const std::string& request, Message* message, rapidjson::Value& id);

      static FullMessage responseMessage(Message* message);
      static FullMessage responseMessage(Message* message, rapidjson::Value& id);

      static FullMessage* timeoutMessage();
    private:

      FullMessage() = default;

      FullMessage(const std::string& request, Message* message);
      FullMessage(Message* message);

      rapidjson::Document doc;
  };


  // convenience functions for bad input
  std::string BAD_REQUEST(const std::string& request);
  std::string BAD_REQUEST(const std::string& request, rapidjson::Value& id);

  std::string BAD_JSON(const std::string& error_details);


}  // namespace rpc

}  // namespace cryptonote
