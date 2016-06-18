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

#pragma once

#include "rapidjson/document.h"

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
    protected:
      Message() { }

    public:
      virtual ~Message() { }

      virtual rapidjson::Value toJson(rapidjson::Document& doc) = 0;

      virtual void fromJson(rapidjson::Value& val) = 0;

      std::string error;
  };

  class FullMessage
  {
    public:

      FullMessage(int version, std::string& request, Message* message);
      FullMessage(const std::string& json_string);

      ~FullMessage() { }

      std::string getJson() const;

      std::string getRequestType() const;

      int getVersion() const;

      rapidjson::Value& getMessage();

    private:

      rapidjson::Document doc;
  };


}  // namespace rpc

}  // namespace cryptonote
