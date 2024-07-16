// Copyright (c) 2017-2024, The Monero Project
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
//

#include "messages_map.hpp"
#include "messages/messages.pb.h"
#include "messages/messages-common.pb.h"
#include "messages/messages-management.pb.h"
#include "messages/messages-monero.pb.h"

#ifdef WITH_TREZOR_DEBUGGING
#include "messages/messages-debug.pb.h"
#endif

using namespace std;
using namespace hw::trezor;

namespace hw{
namespace trezor
{

  const char * TYPE_PREFIX = "MessageType_";
  const char * PACKAGES[] = {
      "hw.trezor.messages.",
      "hw.trezor.messages.common.",
      "hw.trezor.messages.management.",
#ifdef WITH_TREZOR_DEBUGGING
      "hw.trezor.messages.debug.",
#endif
      "hw.trezor.messages.monero."
  };

  google::protobuf::Message * MessageMapper::get_message(int wire_number) {
    return MessageMapper::get_message(static_cast<messages::MessageType>(wire_number));
  }

  google::protobuf::Message * MessageMapper::get_message(messages::MessageType wire_number) {
    const string &messageTypeName = hw::trezor::messages::MessageType_Name(wire_number);
    if (messageTypeName.empty()) {
      throw exc::EncodingException(std::string("Message descriptor not found: ") + std::to_string(wire_number));
    }

    string messageName = messageTypeName.substr(strlen(TYPE_PREFIX));
    return MessageMapper::get_message(messageName);
  }

  google::protobuf::Message * MessageMapper::get_message(const std::string & msg_name) {
    // Each package instantiation so lookup works
    hw::trezor::messages::common::Success::default_instance();
    hw::trezor::messages::management::Cancel::default_instance();
    hw::trezor::messages::monero::MoneroGetAddress::default_instance();

#ifdef WITH_TREZOR_DEBUGGING
    hw::trezor::messages::debug::DebugLinkDecision::default_instance();
#endif

    google::protobuf::Descriptor const * desc = nullptr;
    for(const string &text : PACKAGES){
      desc = google::protobuf::DescriptorPool::generated_pool()
          ->FindMessageTypeByName(text + msg_name);
      if (desc != nullptr){
        break;
      }
    }

    if (desc == nullptr){
      throw exc::EncodingException(std::string("Message not found: ") + msg_name);
    }

    google::protobuf::Message* message =
        google::protobuf::MessageFactory::generated_factory()
            ->GetPrototype(desc)->New();

    return message;

//    // CODEGEN way, fast
//    switch(wire_number){
//      case 501:
//        return new messages::monero::MoneroTransactionSignRequest();
//      default:
//        throw std::runtime_error("not implemented");
//    }
//
//    // CODEGEN message -> number: specification
//    //    messages::MessageType get_message_wire_number(const messages::monero::MoneroTransactionSignRequest * msg) { return 501; }
//    //    messages::MessageType get_message_wire_number(const messages::management::ping * msg)
//
  }

  messages::MessageType MessageMapper::get_message_wire_number(const google::protobuf::Message * msg){
    return MessageMapper::get_message_wire_number(msg->GetDescriptor()->name());
  }

  messages::MessageType MessageMapper::get_message_wire_number(const google::protobuf::Message & msg){
    return MessageMapper::get_message_wire_number(msg.GetDescriptor()->name());
  }

  messages::MessageType MessageMapper::get_message_wire_number(const std::string & msg_name){
    string enumMessageName = std::string(TYPE_PREFIX) + msg_name;

    messages::MessageType res;
    bool r = hw::trezor::messages::MessageType_Parse(enumMessageName, &res);
    if (!r){
      throw exc::EncodingException(std::string("Message ") + msg_name + " not found");
    }

    return res;
  }

}
}
