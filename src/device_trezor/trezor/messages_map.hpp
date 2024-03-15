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

#ifndef MONERO_MESSAGES_MAP_H
#define MONERO_MESSAGES_MAP_H

#include <string>
#include <type_traits>
#include <memory>
#include "exceptions.hpp"

#include "trezor_defs.hpp"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
#include "google/protobuf/descriptor.pb.h"

#include "messages/messages.pb.h"

namespace hw {
namespace trezor {

  class MessageMapper{
    public:
      MessageMapper() {

      }

    static ::google::protobuf::Message * get_message(int wire_number);
    static ::google::protobuf::Message * get_message(messages::MessageType);
    static ::google::protobuf::Message * get_message(const std::string & msg_name);
    static messages::MessageType get_message_wire_number(const google::protobuf::Message * msg);
    static messages::MessageType get_message_wire_number(const google::protobuf::Message & msg);
    static messages::MessageType get_message_wire_number(const std::string & msg_name);

    template<class t_message=google::protobuf::Message>
    static messages::MessageType get_message_wire_number() {
      BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);
      return get_message_wire_number(t_message::default_instance().GetDescriptor()->name());
    }
  };

  template<class t_message=google::protobuf::Message>
  std::shared_ptr<t_message> message_ptr_retype(std::shared_ptr<google::protobuf::Message> & in){
    BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);
    if (!in){
      return nullptr;
    }

    return std::dynamic_pointer_cast<t_message>(in);
  }

  template<class t_message=google::protobuf::Message>
  std::shared_ptr<t_message> message_ptr_retype_static(std::shared_ptr<google::protobuf::Message> & in){
    BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);
    if (!in){
      return nullptr;
    }

    return std::static_pointer_cast<t_message>(in);
  }

}}

#endif //MONERO_MESSAGES_MAP_H
