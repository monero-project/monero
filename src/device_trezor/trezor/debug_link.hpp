// Copyright (c) 2017-2022, The Monero Project
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

#ifndef MONERO_DEBUG_LINK_H
#define MONERO_DEBUG_LINK_H

#include "transport.hpp"
#include "messages/messages-debug.pb.h"


namespace hw {
namespace trezor {

  class DebugLink {
  public:

    DebugLink();
    virtual ~DebugLink();

    void init(std::shared_ptr<Transport> & transport);
    void close();

    std::shared_ptr<messages::debug::DebugLinkState> state();
    void input_word(const std::string & word);
    void input_button(bool button);
    void input_swipe(messages::debug::DebugLinkDecision_DebugSwipeDirection direction);
    void press_yes() { input_button(true); }
    void press_no() { input_button(false); }
    void stop();

    template<class t_message=messages::debug::DebugLinkState>
    std::shared_ptr<t_message> call(
        const google::protobuf::Message & req,
        const boost::optional<messages::MessageType> &resp_type = boost::none,
        bool no_wait = false)
    {
      BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);

      m_transport->write(req);
      if (no_wait){
        return nullptr;
      }

      // Read the response
      std::shared_ptr<google::protobuf::Message> msg_resp;
      hw::trezor::messages::MessageType msg_resp_type;
      m_transport->read(msg_resp, &msg_resp_type);

      messages::MessageType required_type = resp_type ? resp_type.get() : MessageMapper::get_message_wire_number<t_message>();
      if (msg_resp_type == required_type) {
        return message_ptr_retype<t_message>(msg_resp);
      } else if (msg_resp_type == messages::MessageType_Failure){
        throw_failure_exception(dynamic_cast<messages::common::Failure*>(msg_resp.get()));
      } else {
        throw exc::UnexpectedMessageException(msg_resp_type, msg_resp);
      }
    };

  private:
    std::shared_ptr<Transport> m_transport;

  };

}
}

#endif //MONERO_DEBUG_LINK_H
