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
#include "messages/messages-thp.pb.h"

#ifdef WITH_TREZOR_DEBUGGING
#include "messages/messages-debug.pb.h"
#endif

using namespace std;
using namespace hw::trezor;

namespace hw{
namespace trezor
{

  // ThpCreateNewSession is `reserved 1000` in both MessageType and
  // ThpMessageType, so it has no generated enum name. We map it by its
  // wire number directly in two places below.
  static constexpr int THP_CREATE_NEW_SESSION_WIRE = 1000;

  const char * TYPE_PREFIX = "MessageType_";
  const std::string PACKAGES[] = {
      "hw.trezor.messages.",
      "hw.trezor.messages.common.",
      "hw.trezor.messages.management.",
#ifdef WITH_TREZOR_DEBUGGING
      "hw.trezor.messages.debug.",
#endif
      "hw.trezor.messages.monero.",
      "hw.trezor.messages.thp."
  };

  google::protobuf::Message * MessageMapper::get_message(int wire_number) {
    return MessageMapper::get_message(static_cast<messages::MessageType>(wire_number));
  }

  google::protobuf::Message * MessageMapper::get_message(messages::MessageType wire_number) {
    // ThpCreateNewSession is reserved (no enum name) in both MessageType
    // and ThpMessageType. Construct it directly to avoid the descriptor
    // lookup falling through to "Message descriptor not found".
    if (static_cast<int>(wire_number) == THP_CREATE_NEW_SESSION_WIRE) {
      return new hw::trezor::messages::thp::ThpCreateNewSession();
    }
    string messageTypeName = hw::trezor::messages::MessageType_Name(wire_number);
    string messageName;
    if (!messageTypeName.empty()) {
      messageName = messageTypeName.substr(strlen(TYPE_PREFIX));
    } else {
      // THP messages live in a separate enum (ThpMessageType) with its own
      // wire numbers in the 1008..1041 range. Fall back to it before
      // declaring the wire number unknown.
      const string &thpName = hw::trezor::messages::thp::ThpMessageType_Name(
          static_cast<hw::trezor::messages::thp::ThpMessageType>(wire_number));
      if (thpName.empty()) {
        throw exc::EncodingException(std::string("Message descriptor not found: ") + std::to_string(wire_number));
      }
      messageName = thpName.substr(strlen("ThpMessageType_"));
    }
    return MessageMapper::get_message(messageName);
  }

  google::protobuf::Message * MessageMapper::get_message(const std::string & msg_name) {
    // Force one symbol from each package's generated .pb.cc to be linked
    // in, so the protobuf DescriptorPool below can resolve message names
    // by package.  default_instance() is [[nodiscard]] in newer protobuf;
    // the (void) cast documents that we only want the side effect.
    (void)hw::trezor::messages::common::Success::default_instance();
    (void)hw::trezor::messages::management::Cancel::default_instance();
    (void)hw::trezor::messages::monero::MoneroGetAddress::default_instance();
    (void)hw::trezor::messages::thp::ThpPairingRequest::default_instance();

#ifdef WITH_TREZOR_DEBUGGING
    (void)hw::trezor::messages::debug::DebugLinkDecision::default_instance();
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
    if (r){
      return res;
    }

    // THP-specific message types live in ThpMessageType (1008..1041). The
    // wire numbers are unique across the two enums (THP reserves 0..999 and
    // 1100..max for the main MessageType enum), so we can return them as
    // messages::MessageType safely.
    string thpEnumName = std::string("ThpMessageType_") + msg_name;
    hw::trezor::messages::thp::ThpMessageType thp_res;
    if (hw::trezor::messages::thp::ThpMessageType_Parse(thpEnumName, &thp_res)) {
      return static_cast<messages::MessageType>(thp_res);
    }

    // ThpCreateNewSession is "reserved 1000" in both MessageType and
    // ThpMessageType, so neither Parse call above resolves it. The message
    // class is still generated and the firmware accepts wire type 1000.
    if (msg_name == "ThpCreateNewSession") {
      return static_cast<messages::MessageType>(THP_CREATE_NEW_SESSION_WIRE);
    }

    throw exc::EncodingException(std::string("Message ") + msg_name + " not found");
  }

#ifdef PROTOBUF_HAS_ABSEIL
  messages::MessageType MessageMapper::get_message_wire_number(const absl::string_view& msg_name) {
    return MessageMapper::get_message_wire_number(std::string{msg_name});
  }
#endif

}
}
